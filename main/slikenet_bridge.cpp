#include "slikenet_bridge.h"
#include "slikenet/peerinterface.h"
#include "slikenet/MessageIdentifiers.h"
#include "slikenet/BitStream.h"
#include "slikenet/SocketIncludes.h"
#include "slikenet/PacketPriority.h"

static SLNet::RakPeerInterface *peer = 0;

extern "C" void slikenet_init(void) {
    if (peer) return;
    peer = SLNet::RakPeerInterface::GetInstance();
    SLNet::SocketDescriptor sd(42425, 0);
    peer->Startup(32, &sd, 1);
    peer->SetMaximumIncomingConnections(32);
}

extern "C" void slikenet_shutdown(void) {
    if (peer) {
        peer->Shutdown(100);
        SLNet::RakPeerInterface::DestroyInstance(peer);
        peer = 0;
    }
}

extern "C" int slikenet_send(const void *data, int len) {
    if (!peer) return -1;
    SLNet::BitStream bs((unsigned char*)data, len, true);
    peer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, SLNet::UNASSIGNED_SYSTEM_ADDRESS, true);
    return len;
}

extern "C" int slikenet_receive(void *buf, int maxlen, unsigned int *out_ip, unsigned short *out_port) {
    if (!peer) return -1;
    SLNet::Packet *p = peer->Receive();
    if (!p) return 0;
    int n = (int)p->length;
    if (n > maxlen) n = maxlen;
    memcpy(buf, p->data, n);
    if (out_ip) *out_ip = ntohl(p->systemAddress.address.addr4.sin_addr.s_addr);
    if (out_port) *out_port = p->systemAddress.GetPort();
    peer->DeallocatePacket(p);
    return n;
}

extern "C" int slikenet_connect(const char *host, unsigned short port) {
    if (!peer) slikenet_init();
    return peer->Connect(host, port, 0, 0, 0);
}

extern "C" void slikenet_disconnect(unsigned int ip, unsigned short port) {
    if (!peer) return;
    char buf[64];
    sprintf(buf, "%u.%u.%u.%u", (ip>>24)&0xFF, (ip>>16)&0xFF, (ip>>8)&0xFF, ip&0xFF);
    SLNet::SystemAddress sa(buf, port);
    peer->CloseConnection(sa, true, 0, (PacketPriority)0);
}
