#include <stdio.h>
#include <string.h>
#include "slikenet_bridge.h"
#include "slikenet/peerinterface.h"
#include "slikenet/MessageIdentifiers.h"
#include "slikenet/BitStream.h"
#include "slikenet/SocketIncludes.h"
#include "slikenet/PacketPriority.h"
#include "slikenet/GetTime.h"
#include "slikenet/socket2.h"
#include "slikenet/SimpleMutex.h"

// console.h pulls in pstypes.h, whose bool typedef doesn't compile as C++.
extern "C" void con_printf(int priority, const char *fmt, ...);
#define SLIKENET_CON_STATUS -6 // CON_NET_STATUS
#define SLIKENET_CON_DEALLOC -7 // CON_NET_DEALLOC

#define SLIKENET_MAX_PEERS 24 // MAX_PLAYERS + MAX_OBSERVERS
#define SLIKENET_MAX_PACKET 1024 // UPID_MAX_SIZE
#define SLIKENET_RETRY_MS 5000
#define SLIKENET_HELLO_MS 1000 // resend an unanswered hello at most this often
#define SLIKENET_PLAIN_QUEUE 128 // plain game datagrams buffered between frames
#define SLIKENET_UNRELIABLE_TIMEOUT_MS 100 // drop queued positions this stale instead of sending them late in a burst

enum {
    ID_DXX_DATA = ID_USER_PACKET_ENUM, // UPID values collide with SLikeNet's own IDs, so frame them
    ID_DXX_HELLO,
    ID_DXX_HELLO_ACK
};

struct slikenet_peer {
    bool used;
    bool ready;
    bool failed; // the last connect attempt got no answer
    struct sockaddr_in addr; // plain UDP player address
    SLNet::SystemAddress system;
    SLNet::TimeMS last_attempt;
    SLNet::TimeMS last_hello;
};

static SLNet::RakPeerInterface *peer = 0;
static slikenet_peer peers[SLIKENET_MAX_PEERS];
static SLNet::SystemAddress last_hello_sender;
static unsigned int packets_sent, packets_received, packets_fallback, peers_ready; // session totals
static unsigned short lane_port; // 0 if the lane didn't start

// The lane and the game's plain UDP share one socket on one port (42424 by default):
// SLikeNet owns the socket, and datagrams that aren't SLikeNet's are queued here for the game.
struct plain_datagram {
    int len;
    struct sockaddr_in from;
    unsigned char data[SLIKENET_MAX_PACKET];
};
static plain_datagram plain_queue[SLIKENET_PLAIN_QUEUE];
static int plain_head, plain_count;
static SLNet::SimpleMutex plain_mutex;

// SLikeNet's own handshake datagrams (not connected traffic), counted for the status line in slikenet_receive().
// Written on SLikeNet's receive thread and read on the game thread, both under plain_mutex.
#define SLIKENET_HANDSHAKE_REPORT_MS 5000
static unsigned int hs_count, hs_last_id, hs_last_len;
static SLNet::SystemAddress hs_last_from;
static SLNet::TimeMS hs_last_report;

static const unsigned char OFFLINE_MAGIC[16] = {0x00,0xFF,0xFF,0x00,0xFE,0xFE,0xFE,0xFE,0xFD,0xFD,0xFD,0xFD,0x12,0x34,0x56,0x78};

// SLikeNet connected datagrams have the top bit set, and its offline messages
// carry OFFLINE_MAGIC near the start. Everything else is a game packet (UPID 1-30).
static bool is_game_datagram(const char *data, int len)
{
    if (len <= 2 || len > SLIKENET_MAX_PACKET || ((unsigned char)data[0] & 0x80))
        return false;
    for (int i = 1; i + (int)sizeof(OFFLINE_MAGIC) <= len && i <= 40; i++)
        if (!memcmp(data + i, OFFLINE_MAGIC, sizeof(OFFLINE_MAGIC)))
            return false;
    return true;
}

// Runs on SLikeNet's receive thread. Return false to claim the datagram for the game.
static bool datagram_filter(SLNet::RNS2RecvStruct *rs)
{
    if (!is_game_datagram(rs->data, rs->bytesRead) || rs->systemAddress.GetIPVersion() != 4)
    {
        // Top bit clear means SLikeNet's connection handshake, not connected traffic
        if (rs->bytesRead > 0 && !((unsigned char)rs->data[0] & 0x80))
        {
            plain_mutex.Lock();
            hs_count++;
            hs_last_id = (unsigned char)rs->data[0];
            hs_last_len = (unsigned int)rs->bytesRead;
            hs_last_from = rs->systemAddress;
            plain_mutex.Unlock();
        }
        return true;
    }

    plain_mutex.Lock();
    if (plain_count < SLIKENET_PLAIN_QUEUE) // full: drop, like a lost UDP packet
    {
        plain_datagram *d = &plain_queue[(plain_head + plain_count) % SLIKENET_PLAIN_QUEUE];
        d->len = rs->bytesRead;
        d->from = rs->systemAddress.address.addr4;
        memcpy(d->data, rs->data, rs->bytesRead);
        plain_count++;
    }
    plain_mutex.Unlock();
    return false;
}

static slikenet_peer *find_peer_by_addr(const void *addr, int addr_len)
{
    if (addr_len != (int)sizeof(struct sockaddr_in))
        return 0;
    for (int i = 0; i < SLIKENET_MAX_PEERS; i++)
        if (peers[i].used && !memcmp(&peers[i].addr, addr, sizeof(struct sockaddr_in)))
            return &peers[i];
    return 0;
}

static slikenet_peer *find_peer_by_system(const SLNet::SystemAddress &system)
{
    for (int i = 0; i < SLIKENET_MAX_PEERS; i++)
        if (peers[i].used && peers[i].system == system)
            return &peers[i];
    return 0;
}

static slikenet_peer *add_peer(const void *addr, int addr_len)
{
    if (addr_len != (int)sizeof(struct sockaddr_in))
        return 0;
    for (int i = 0; i < SLIKENET_MAX_PEERS; i++)
        if (!peers[i].used)
        {
            peers[i].used = true;
            peers[i].ready = false;
            memcpy(&peers[i].addr, addr, sizeof(struct sockaddr_in));
            peers[i].last_attempt = 0;
            peers[i].last_hello = 0;
            peers[i].failed = false;
            return &peers[i];
        }
    return 0;
}

static void send_control(unsigned char id, const void *data, int len, const SLNet::SystemAddress &system)
{
    SLNet::BitStream bs;
    bs.Write((SLNet::MessageID)id);
    if (len > 0)
        bs.WriteAlignedBytes((const unsigned char *)data, len);
    peer->Send(&bs, HIGH_PRIORITY, RELIABLE, 0, system, false);
}

extern "C" int slikenet_startup(unsigned short game_port, const char *bind_ip)
{
    unsigned short port = game_port;

    slikenet_shutdown();
    packets_sent = packets_received = packets_fallback = peers_ready = 0;
    plain_head = plain_count = 0;
    hs_count = 0;
    hs_last_report = 0;
    lane_port = 0;
    peer = SLNet::RakPeerInterface::GetInstance();

    SLNet::SocketDescriptor sd(port, bind_ip);
    SLNet::StartupResult result = peer->Startup(SLIKENET_MAX_PEERS, &sd, 1);
    if (result != SLNet::RAKNET_STARTED)
    {
        con_printf(SLIKENET_CON_DEALLOC, "\033[32mSLikeNet Lane port %u \033[31mNOT STARTED\033[32m (error %d), retrying\033[0m\n", port, (int)result);
        SLNet::RakPeerInterface::DestroyInstance(peer);
        peer = 0;
        return -1;
    }
    peer->SetMaximumIncomingConnections(SLIKENET_MAX_PEERS);
    peer->SetIncomingDatagramEventHandler(datagram_filter);
    peer->SetUnreliableTimeout(SLIKENET_UNRELIABLE_TIMEOUT_MS);
    lane_port = port;
    con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Lane is ACTIVE on port %u\033[0m\n", port);
    return 0;
}

// The lane's socket, shared with the game for plain UDP sends. -1 if the lane isn't running.
extern "C" int slikenet_get_socket(void)
{
    if (!peer)
        return -1;
    DataStructures::List<SLNet::RakNetSocket2 *> sockets;
    peer->GetSockets(sockets);
    if (sockets.Size() == 0 || !sockets[0]->IsBerkleySocket())
        return -1;
    return (int)static_cast<SLNet::RNS2_Berkley *>(sockets[0])->GetSocket();
}

extern "C" int slikenet_plain_ready(void)
{
    plain_mutex.Lock();
    int n = plain_count;
    plain_mutex.Unlock();
    return n > 0;
}

// Returns the datagram length, or 0 if none is waiting.
extern "C" int slikenet_plain_receive(void *buf, int maxlen, void *out_addr, int addr_len)
{
    int n = 0;

    if (addr_len < (int)sizeof(struct sockaddr_in))
        return 0;
    plain_mutex.Lock();
    if (plain_count > 0)
    {
        plain_datagram *d = &plain_queue[plain_head];
        n = d->len < maxlen ? d->len : maxlen;
        memcpy(buf, d->data, n);
        memcpy(out_addr, &d->from, sizeof(struct sockaddr_in));
        plain_head = (plain_head + 1) % SLIKENET_PLAIN_QUEUE;
        plain_count--;
    }
    plain_mutex.Unlock();
    return n;
}

// Where the lane to this player address stands: SLIKENET_LINK_READY once its handshake finished,
// SLIKENET_LINK_FAILED after a connect attempt got no answer, otherwise SLIKENET_LINK_PENDING
extern "C" int slikenet_peer_state(const void *addr, int addr_len)
{
    slikenet_peer *p = find_peer_by_addr(addr, addr_len);

    if (p && p->ready)
        return SLIKENET_LINK_READY;
    if (p && p->failed)
        return SLIKENET_LINK_FAILED;
    return SLIKENET_LINK_PENDING;
}

extern "C" void slikenet_get_stats(unsigned int *port, unsigned int *ready, unsigned int *sent, unsigned int *received, unsigned int *fallback)
{
    *port = lane_port;
    *ready = peers_ready;
    *sent = packets_sent;
    *received = packets_received;
    *fallback = packets_fallback;
}

extern "C" void slikenet_shutdown(void)
{
    if (peer)
    {
        peer->Shutdown(100);
        SLNet::RakPeerInterface::DestroyInstance(peer);
        peer = 0;
        con_printf(SLIKENET_CON_DEALLOC, "\033[32mSLikeNet Lane \033[31mDEALLOCATED\033[32m (sent %u, received %u, %u sent by plain UDP while peer not ready)\033[0m\n", packets_sent, packets_received, packets_fallback);
    }
    for (int i = 0; i < SLIKENET_MAX_PEERS; i++)
        peers[i].used = peers[i].ready = false;
}

extern "C" void slikenet_connect_peer(const void *addr, int addr_len, const void *hello, int hello_len)
{
    if (!peer)
        return;

    slikenet_peer *p = find_peer_by_addr(addr, addr_len);
    if (p && p->ready)
        return;
    if (!p)
    {
        p = add_peer(addr, addr_len);
        if (!p)
            return;
        char ip[INET_ADDRSTRLEN];
        snprintf(ip, sizeof(ip), "%s", inet_ntoa(p->addr.sin_addr));
        p->system.FromStringExplicitPort(ip, ntohs(p->addr.sin_port));
    }

    SLNet::ConnectionState state = peer->GetConnectionState(p->system);
    if (state == SLNet::IS_CONNECTED)
    {
        // The other side may keep refusing the hello (wrong token, player not marked connected yet), so don't resend it every tick
        SLNet::TimeMS now = SLNet::GetTimeMS();
        if (!p->last_hello || now - p->last_hello >= SLIKENET_HELLO_MS)
        {
            p->last_hello = now;
            send_control(ID_DXX_HELLO, hello, hello_len, p->system);
        }
        return;
    }
    if (state != SLNet::IS_NOT_CONNECTED && state != SLNet::IS_DISCONNECTED)
        return; // attempt in progress

    SLNet::TimeMS now = SLNet::GetTimeMS();
    if (p->last_attempt && now - p->last_attempt < SLIKENET_RETRY_MS)
        return;
    p->last_attempt = now;

    char ip[INET_ADDRSTRLEN];
    snprintf(ip, sizeof(ip), "%s", inet_ntoa(p->addr.sin_addr));
    con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Handshake OUT: connecting to %s:%u\033[0m\n", ip, (unsigned int)p->system.GetPort());
    SLNet::ConnectionAttemptResult result = peer->Connect(ip, p->system.GetPort(), 0, 0);
    if (result != SLNet::CONNECTION_ATTEMPT_STARTED)
        con_printf(SLIKENET_CON_DEALLOC, "\033[32mSLikeNet Connect to %s:%u \033[31mFAILED\033[32m (error %d), retrying\033[0m\n", ip, (unsigned int)p->system.GetPort(), (int)result);
}

extern "C" int slikenet_accept_hello(const void *addr, int addr_len)
{
    if (!peer || addr_len != (int)sizeof(struct sockaddr_in))
        return 0;

    struct sockaddr_in in;
    memcpy(&in, addr, sizeof(in));
    if (in.sin_addr.s_addr != last_hello_sender.address.addr4.sin_addr.s_addr)
        return 0;

    // The same player may reconnect from a new SLikeNet address
    slikenet_peer *p = find_peer_by_addr(addr, addr_len);
    // That player's lane is still up on another address: another player behind the same IP must not take it over
    if (p && p->ready && !(p->system == last_hello_sender) && peer->GetConnectionState(p->system) == SLNet::IS_CONNECTED)
        return 0;
    if (!p)
        p = add_peer(addr, addr_len);
    if (!p)
        return 0;
    p->system = last_hello_sender;
    if (!p->ready)
    {
        peers_ready++;
        con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Peer READY: %s\033[0m\n", p->system.ToString(true));
    }
    p->ready = true;
    send_control(ID_DXX_HELLO_ACK, 0, 0, p->system);
    return 1;
}

extern "C" int slikenet_send_to_peer(const void *addr, int addr_len, const void *data, int len, int mode)
{
    unsigned char frame[SLIKENET_MAX_PACKET + 1];
    PacketReliability reliability;

    if (!peer || len <= 0 || len > SLIKENET_MAX_PACKET)
        return -1;
    slikenet_peer *p = find_peer_by_addr(addr, addr_len);
    if (!p)
        return -1; // not a lane peer (observers, strangers): plain UDP, and not counted as a fallback
    if (!p->ready)
    {
        packets_fallback++;
        return -1;
    }

    unsigned char channel = 0;
    switch (mode)
    {
        case SLIKENET_SEQUENCED:        reliability = UNRELIABLE_SEQUENCED; break;
        case SLIKENET_RELIABLE_ORDERED: reliability = RELIABLE_ORDERED;     channel = 1; break;
        case SLIKENET_RELIABLE:         reliability = RELIABLE;             break;
        default:                        reliability = UNRELIABLE;          break;
    }

    frame[0] = ID_DXX_DATA;
    memcpy(frame + 1, data, len);
    // Channels only apply to ordered and sequenced packets. Ordered data gets
    // its own so a resend there can't interact with the sequenced movement stream.
    if (!peer->Send((const char *)frame, len + 1, mode == SLIKENET_SEQUENCED ? IMMEDIATE_PRIORITY : HIGH_PRIORITY, reliability, channel, p->system, false))
    {
        packets_fallback++;
        return -1;
    }
    packets_sent++;
    return len;
}

extern "C" int slikenet_receive(void *buf, int maxlen, void *out_addr, int addr_len, int *out_type)
{
    SLNet::Packet *packet;

    *out_type = SLIKENET_PACKET_NONE;
    if (!peer)
        return 0;

    {
        SLNet::TimeMS now = SLNet::GetTimeMS();
        unsigned int n, id, len;
        SLNet::SystemAddress from;

        plain_mutex.Lock();
        n = hs_count;
        id = hs_last_id;
        len = hs_last_len;
        from = hs_last_from;
        if (n && now - hs_last_report >= SLIKENET_HANDSHAKE_REPORT_MS)
        {
            hs_count = 0;
            hs_last_report = now;
        }
        else
            n = 0;
        plain_mutex.Unlock();
        if (n)
            con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Handshake IN: %u packets, last id %u, %u bytes, from %s\033[0m\n", n, id, len, from.ToString(true));
    }

    for (; (packet = peer->Receive()) != 0; peer->DeallocatePacket(packet))
    {
        slikenet_peer *p;
        int n = (int)packet->length - 1;

        switch (packet->data[0])
        {
            case ID_DXX_DATA:
                p = find_peer_by_system(packet->systemAddress);
                if (!p || !p->ready || n <= 0 || n > maxlen || addr_len != (int)sizeof(struct sockaddr_in))
                    break;
                memcpy(buf, packet->data + 1, n);
                memcpy(out_addr, &p->addr, sizeof(struct sockaddr_in));
                *out_type = SLIKENET_PACKET_DATA;
                packets_received++;
                peer->DeallocatePacket(packet);
                return n;
            case ID_DXX_HELLO:
                if (n <= 0 || n > maxlen)
                    break;
                memcpy(buf, packet->data + 1, n);
                last_hello_sender = packet->systemAddress;
                *out_type = SLIKENET_PACKET_HELLO;
                peer->DeallocatePacket(packet);
                return n;
            case ID_DXX_HELLO_ACK:
                p = find_peer_by_system(packet->systemAddress);
                if (p && !p->ready)
                {
                    p->ready = true;
                    peers_ready++;
                    con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Peer READY: %s\033[0m\n", packet->systemAddress.ToString(true));
                }
                break;
            case ID_CONNECTION_REQUEST_ACCEPTED:
                con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Connected to %s\033[0m\n", packet->systemAddress.ToString(true));
                break;
            case ID_NEW_INCOMING_CONNECTION:
                con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Incoming connection from %s\033[0m\n", packet->systemAddress.ToString(true));
                break;
            case ID_CONNECTION_ATTEMPT_FAILED:
            case ID_NO_FREE_INCOMING_CONNECTIONS:
            case ID_ALREADY_CONNECTED:
                p = find_peer_by_system(packet->systemAddress);
                if (p && packet->data[0] != ID_ALREADY_CONNECTED)
                    p->failed = true;
                con_printf(SLIKENET_CON_DEALLOC, "\033[32mSLikeNet Connection to %s \033[31mFAILED\033[32m (id %d)\033[0m\n", packet->systemAddress.ToString(true), (int)packet->data[0]);
                break;
            case ID_DISCONNECTION_NOTIFICATION:
            case ID_CONNECTION_LOST:
                p = find_peer_by_system(packet->systemAddress);
                if (p)
                    p->used = p->ready = false;
                con_printf(SLIKENET_CON_DEALLOC, "\033[32mSLikeNet Connection to %s \033[31mLOST\033[0m\n", packet->systemAddress.ToString(true));
                break;
            default:
                break;
        }
    }
    return 0;
}
