#include <stdio.h>
#include <string.h>
#include "slikenet_bridge.h"
#include "slikenet/peerinterface.h"
#include "slikenet/MessageIdentifiers.h"
#include "slikenet/BitStream.h"
#include "slikenet/SocketIncludes.h"
#include "slikenet/PacketPriority.h"
#include "slikenet/GetTime.h"

// console.h pulls in pstypes.h, whose bool typedef doesn't compile as C++.
extern "C" void con_printf(int priority, const char *fmt, ...);
#define SLIKENET_CON_STATUS -6 // CON_NET_STATUS
#define SLIKENET_CON_DEALLOC -7 // CON_NET_DEALLOC

#define SLIKENET_MAX_PEERS 24 // MAX_PLAYERS + MAX_OBSERVERS
#define SLIKENET_MAX_PACKET 1024 // UPID_MAX_SIZE
#define SLIKENET_RETRY_MS 5000

enum {
    ID_DXX_DATA = ID_USER_PACKET_ENUM, // UPID values collide with SLikeNet's own IDs, so frame them
    ID_DXX_HELLO,
    ID_DXX_HELLO_ACK
};

struct slikenet_peer {
    bool used;
    bool ready;
    struct sockaddr_in addr; // plain UDP player address
    SLNet::SystemAddress system;
    SLNet::TimeMS last_attempt;
};

static SLNet::RakPeerInterface *peer = 0;
static slikenet_peer peers[SLIKENET_MAX_PEERS];
static SLNet::SystemAddress last_hello_sender;
static unsigned int packets_sent, packets_received, packets_fallback; // session totals, logged at shutdown

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

extern "C" int slikenet_startup(unsigned short game_port)
{
    unsigned short port = (unsigned short)(game_port + 1);

    slikenet_shutdown();
    peer = SLNet::RakPeerInterface::GetInstance();

    SLNet::SocketDescriptor sd(port, 0);
    SLNet::StartupResult result = peer->Startup(SLIKENET_MAX_PEERS, &sd, 1);
    if (result != SLNet::RAKNET_STARTED)
    {
        con_printf(SLIKENET_CON_DEALLOC, "\033[32mSLikeNet Lane port %u \033[31mNOT STARTED\033[32m (error %d), using plain UDP\033[0m\n", port, (int)result);
        SLNet::RakPeerInterface::DestroyInstance(peer);
        peer = 0;
        return -1;
    }
    peer->SetMaximumIncomingConnections(SLIKENET_MAX_PEERS);
    packets_sent = packets_received = packets_fallback = 0;
    con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Lane is ACTIVE on port %u\033[0m\n", port);
    return 0;
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
        p->system.FromStringExplicitPort(ip, (unsigned short)(ntohs(p->addr.sin_port) + 1));
    }

    SLNet::ConnectionState state = peer->GetConnectionState(p->system);
    if (state == SLNet::IS_CONNECTED)
    {
        send_control(ID_DXX_HELLO, hello, hello_len, p->system);
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
    peer->Connect(ip, p->system.GetPort(), 0, 0);
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
    if (!p)
        p = add_peer(addr, addr_len);
    if (!p)
        return 0;
    p->system = last_hello_sender;
    if (!p->ready)
        con_printf(SLIKENET_CON_STATUS, "\033[32mSLikeNet Peer READY: %s\033[0m\n", p->system.ToString(true));
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
    if (!p || !p->ready)
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
    if (!peer->Send((const char *)frame, len + 1, HIGH_PRIORITY, reliability, channel, p->system, false))
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
