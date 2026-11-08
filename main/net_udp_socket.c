#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "net_udp_socket.h"
#include "net_udp.h"
#include "timer.h"
#include "console.h"
#include "slikenet_bridge.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

int UDP_Socket[3] = { -1, -1, -1 };

/* Socket 0 is SLikeNet's own socket when this is set: the game sends through it,
 * receives from slikenet_plain_receive(), and slikenet_shutdown() closes it. */
static int UDP_Socket0_shared = 0;

/* Running totals since the last udp_traffic_stat() call. These count packets
 * rather than averaging packet sizes, so the periodic summary actually tracks
 * Netgame.PacketsPerSec - the per packet lines below only ever show the size
 * of a single packet. Both lanes, plain UDP and SLikeNet, count here. */
static unsigned int UDP_num_sendto = 0, UDP_len_sendto = 0;
static unsigned int UDP_num_recvfrom = 0, UDP_len_recvfrom = 0;
static unsigned int UDP_pdata_tx_updates = 0, UDP_pdata_rx_updates = 0;

/* Seconds between traffic summary lines. */
#define TRAFFIC_STAT_INTERVAL 5
/* The per packet [TX] / [RX] lines only sample one packet per interval, so
 * -debug doesn't cost a console write for every packet. The counters above
 * still count every packet. */
// #define PACKET_LOG_INTERVAL F1_0

// static fix64 last_packet_tx_log_time = 0, last_packet_rx_log_time = 0;
// static int packet_tx_logged = 0, packet_rx_logged = 0;

#ifndef IPv6
// Whitelist of the 4 packet IDs routed over SLikeNet, pinned to this header's
// actual numeric values so a future net_udp.h change, or a script/handoff
// written against stale numbers, can't silently shift this mapping.
#if UPID_PDATA != 16 || UPID_OBJECT_DATA != 11 || UPID_MDATA_PNORM != 17 || UPID_MDATA_PNEEDACK != 18
#error "SLikeNet whitelist in udp_send_slikenet() no longer matches net_udp.h's UPID_* values; update both together."
#endif

// Only core gameplay telemetry and combat actions go over the high-speed
// SLikeNet lane; native DXX plain UDP still carries everything else, so
// lobby state (sync, discovery, join, tracker) and secondary traffic
// (observers, ping, end-level) are unaffected by this lane's connect state.
static int udp_send_slikenet(int sockfd, const void *msg, int len, const struct sockaddr *to, socklen_t tolen)
{
    int mode;

    if (sockfd != UDP_Socket[0] || len <= 0)
        return -1;

    switch (((const ubyte *)msg)[0])
    {
        case UPID_PDATA: // = 16: live ship position/velocity/orientation
            mode = SLIKENET_SEQUENCED; // only the newest matters
            break;
        case UPID_OBJECT_DATA: // = 11: object sync for a joining player
            mode = SLIKENET_RELIABLE_ORDERED; // joiners usually aren't connected yet, so this normally falls back to plain UDP
            break;
        case UPID_MDATA_PNORM: // = 17: multi buffer, priority 0-1, no ACK
            mode = SLIKENET_UNRELIABLE; // same delivery as plain UDP: a lost packet is not resent
            break;
        case UPID_MDATA_PNEEDACK: // = 18: multi buffer, priority 2, ACKed
            mode = SLIKENET_RELIABLE; // the game's ACK layer still runs; net_udp_noloss_validate_mdata() drops duplicates
            break;
        default:
            return -1;
    }
    return slikenet_send_to_peer(to, (int)tolen, msg, len, mode);
}
#endif

ssize_t dxx_sendto(int sockfd, const void *msg, int len, unsigned int flags,
    const struct sockaddr *to, socklen_t tolen)
{
    ssize_t result;

#ifndef IPv6
    result = udp_send_slikenet(sockfd, msg, len, to, tolen);
    if (result < 0)
#endif
    result = sendto(sockfd, (const char*)msg, len, flags, to, tolen);
    if (result >= 0)
    {
        // fix64 now = timer_query();

        UDP_num_sendto++;
        UDP_len_sendto += (unsigned int)result;
        // if (!packet_tx_logged || now < last_packet_tx_log_time ||
        //     now - last_packet_tx_log_time >= PACKET_LOG_INTERVAL)
        // {
        //     con_printf(CON_NET_TX,
        //         "\033[38;5;208m[TX] SLikeNet Packet OUT: SENT %d Bytes Traffic\033[0m\n",
        //         (int)result);
        //     last_packet_tx_log_time = now;
        //     packet_tx_logged = 1;
        // }
    }
    return result;
}

void udp_count_received(int size)
{
    // fix64 now = timer_query();

    UDP_num_recvfrom++;
    UDP_len_recvfrom += (unsigned int)size;
    // if (!packet_rx_logged || now < last_packet_rx_log_time ||
    //     now - last_packet_rx_log_time >= PACKET_LOG_INTERVAL)
    // {
    //     con_printf(CON_NET_RX,
    //         "\033[34m[RX] SLikeNet Packet IN: Received %d Bytes Traffic\033[0m\n",
    //         size);
    //     last_packet_rx_log_time = now;
    //     packet_rx_logged = 1;
    // }
}

void udp_record_pdata_tx(void)
{
    UDP_pdata_tx_updates++;
}

void udp_record_pdata_rx(void)
{
    UDP_pdata_rx_updates++;
}

ssize_t dxx_recvfrom(int sockfd, void *buf, int len, unsigned int flags,
    struct sockaddr *from, socklen_t *fromlen)
{
    ssize_t result = recvfrom(sockfd, (char*)buf, len, flags, from, fromlen);
    if (result > 0)
        udp_count_received((int)result);
    return result;
}

/* Report the traffic that moved since the last report. Called once per frame
 * from net_udp_do_frame(), but only emits every TRAFFIC_STAT_INTERVAL
 * seconds. The per packet lines above can only ever show the size of a single
 * packet; this is the readout that shows how many actually went out, so it is
 * the one that responds to Netgame.PacketsPerSec. Colours are kept identical
 * to the per packet OUT / IN lines. */
void udp_traffic_stat(void)
{
    static fix64 last_traf_time = 0;
    fix64 now = timer_query();
    float elapsed_seconds;
    float pdata_tx_per_second, pdata_rx_per_second;

    if (now < last_traf_time + (F1_0 * TRAFFIC_STAT_INTERVAL))
        return;

    elapsed_seconds = (float)(now - last_traf_time) / (float)F1_0;
    if (elapsed_seconds <= 0)
        return;
    last_traf_time = now;
    pdata_tx_per_second = (float)UDP_pdata_tx_updates / elapsed_seconds;
    pdata_rx_per_second = (float)UDP_pdata_rx_updates / elapsed_seconds;

    if (UDP_num_sendto)
        con_printf(CON_NET_TX,
            "\033[38;5;208m[TX] SLikeNet Traffic OUT: %.2fKB/s %uPPS\033[0m\n",
            (float)UDP_len_sendto / (1024.0f * elapsed_seconds),
            (unsigned int)((float)UDP_num_sendto / elapsed_seconds + 0.5f));
    if (UDP_num_recvfrom)
        con_printf(CON_NET_RX,
            "\033[34m[RX] SLikeNet Traffic IN: %.2fKB/s %uPPS\033[0m\n",
            (float)UDP_len_recvfrom / (1024.0f * elapsed_seconds),
            (unsigned int)((float)UDP_num_recvfrom / elapsed_seconds + 0.5f));
    if (UDP_pdata_tx_updates || UDP_pdata_rx_updates)
        con_printf(CON_NET_STATUS,
            "PData %.1f/%.1f updates/s TX/RX (target %d)\n",
            pdata_tx_per_second, pdata_rx_per_second, Netgame.PacketsPerSec);

    UDP_num_sendto = UDP_len_sendto = 0;
    UDP_num_recvfrom = UDP_len_recvfrom = 0;
    UDP_pdata_tx_updates = UDP_pdata_rx_updates = 0;
}

int udp_dns_filladdr(char *host, int port, struct _sockaddr *sAddr)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    char service[16];

    if (!host || !sAddr || port <= 0 || port > 65535)
        return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(service, sizeof(service), "%d", port);

    if (getaddrinfo(host, service, &hints, &result) != 0 || !result)
        return -1;

    memset(sAddr, 0, sizeof(*sAddr));
    memcpy(sAddr, result->ai_addr,
        result->ai_addrlen < sizeof(*sAddr) ? result->ai_addrlen : sizeof(*sAddr));
    freeaddrinfo(result);
    return 0;
}

/* Proxy file: "proxy.cfg" in the working directory, one line "bind_ip=<ipv4>".
 * It names the local address this copy binds, so several copies on one PC can
 * all use port 42424 on their own address (127.0.0.2, 127.0.0.3, ...).
 * Returns NULL when there is no usable file: bind all addresses as usual. */
static const char *udp_proxy_bind_ip(void)
{
    static char ip[48];
    static int loaded = 0;
    char line[128];
    FILE *f;

    if (loaded)
        return ip[0] ? ip : NULL;
    loaded = 1;

    f = fopen("proxy.cfg", "r");
    if (!f)
        return NULL;
    while (fgets(line, sizeof(line), f))
    {
        char *p = line, *end;

        while (*p == ' ' || *p == '\t')
            p++;
        if (strncmp(p, "bind_ip", 7) != 0)
            continue;
        p += 7;
        while (*p == ' ' || *p == '\t' || *p == '=')
            p++;
        for (end = p; *end && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n'; end++);
        *end = '\0';
        if (*p && inet_addr(p) != INADDR_NONE && strlen(p) < sizeof(ip))
        {
            strcpy(ip, p);
            con_printf(CON_NET_STATUS, "\033[32mSLikeNet Proxy bind IP %s (proxy.cfg)\033[0m\n", ip);
        }
        else
            con_printf(CON_NET_DEALLOC, "\033[32mSLikeNet Proxy bind IP \033[31mINVALID\033[32m in proxy.cfg, binding all addresses\033[0m\n");
        break;
    }
    fclose(f);
    return ip[0] ? ip : NULL;
}

/* Describe this machine's address so that log lines read as "<host>:<port>".
 * Falls back to "*" when the local address cannot be determined. */
static const char *udp_local_addr_string(void)
{
    static char buf[64];
    struct addrinfo hints, *result = NULL;
    char hostname[256];
    int resolved = 0;

    if (udp_proxy_bind_ip())
        return udp_proxy_bind_ip();
    if (buf[0] != '\0')
        return buf;

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        hostname[sizeof(hostname) - 1] = '\0';

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if (getaddrinfo(hostname, NULL, &hints, &result) == 0 && result &&
            getnameinfo(result->ai_addr, (socklen_t)result->ai_addrlen,
                buf, sizeof(buf), NULL, 0, NI_NUMERICHOST) == 0)
            resolved = 1;
    }

    if (result)
        freeaddrinfo(result);

    if (!resolved)
        snprintf(buf, sizeof(buf), "*");

    return buf;
}

void udp_close_socket(int socknum)
{
    if (socknum < 0 || socknum >= 3)
        return;

    if (UDP_Socket[socknum] != -1)
    {
#ifndef IPv6
        if (socknum == 0)
            slikenet_shutdown();
#endif
        if (socknum == 0 && UDP_Socket0_shared)
            UDP_Socket0_shared = 0; /* SLikeNet closed it */
        else
#ifdef _WIN32
            closesocket(UDP_Socket[socknum]);
#else
            close(UDP_Socket[socknum]);
#endif
        UDP_Socket[socknum] = -1;
        con_printf(CON_NET_DEALLOC,
            "\033[32mSLikeNet Channel %d \033[31mDEALLOCATED\033[0m\n",
            socknum);
    }
}

int udp_open_socket(int socknum, int port)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    char service[16];
    int broadcast = 1;
    int reuse = 1;

    if (socknum < 0 || socknum >= 3 || port <= 0 || port > 65535)
        return -1;

    udp_close_socket(socknum);
#ifndef IPv6
    /* One socket, one port: SLikeNet binds it and the game shares it, every
     * time (host, client and observer, first start or restart after leaving a
     * game). There is no plain UDP fallback: right after the last game closed
     * the port can still be held for a moment, so retry before giving up. */
    if (socknum == 0)
    {
        int attempt;

        for (attempt = 0; attempt < 10; attempt++)
        {
            if (slikenet_startup((unsigned short)port, udp_proxy_bind_ip()) == 0 && slikenet_get_socket() >= 0)
            {
                UDP_Socket[0] = slikenet_get_socket();
                UDP_Socket0_shared = 1;
                con_printf(CON_NET_STATUS, "\033[32mSLikeNet Telemetry Active\033[0m\n");
                con_printf(CON_NET_STATUS,
                    "\033[32mSLikeNet is ACTIVE on %s:%d\033[0m\n",
                    udp_local_addr_string(), port);
                return 0;
            }
            slikenet_shutdown();
            timer_delay(F1_0 / 20);
        }
        con_printf(CON_NET_DEALLOC,
            "\033[32mSLikeNet Lane port %d \033[31mNOT STARTED\033[32m, not falling back to plain UDP\033[0m\n", port);
        return -1;
    }
#endif
    UDP_Socket[socknum] = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_Socket[socknum] < 0)
        return -1;

    setsockopt(UDP_Socket[socknum], SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(service, sizeof(service), "%d", port);

    if (getaddrinfo(socknum == 2 ? NULL : udp_proxy_bind_ip(), service, &hints, &result) != 0 || !result ||
        bind(UDP_Socket[socknum], result->ai_addr, result->ai_addrlen) < 0)
    {
        if (result)
            freeaddrinfo(result);
        udp_close_socket(socknum);
        return -1;
    }

    freeaddrinfo(result);
    setsockopt(UDP_Socket[socknum], SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, sizeof(broadcast));
    if (socknum == 0)
        con_printf(CON_NET_STATUS, "\033[32mSLikeNet Telemetry Active\033[0m\n");
    con_printf(CON_NET_STATUS,
        "\033[32mSLikeNet is ACTIVE on %s:%d\033[0m\n",
        udp_local_addr_string(), port);
    return 0;
}

int udp_general_packet_ready(int socknum)
{
    fd_set set;
    struct timeval timeout = { 0, 0 };

    if (socknum < 0 || socknum >= 3 || UDP_Socket[socknum] == -1)
        return 0;
    if (socknum == 0 && UDP_Socket0_shared)
        return slikenet_plain_ready();

    FD_ZERO(&set);
    FD_SET(UDP_Socket[socknum], &set);
    return select(UDP_Socket[socknum] + 1, &set, NULL, NULL, &timeout) > 0;
}

int udp_receive_packet(int socknum, ubyte *text, int len, struct _sockaddr *sender_addr)
{
    socklen_t sender_length = sizeof(struct _sockaddr);

    if (socknum < 0 || socknum >= 3 || UDP_Socket[socknum] == -1 || !text || len <= 0)
        return -1;

    if (!udp_general_packet_ready(socknum))
        return 0;

    if (socknum == 0 && UDP_Socket0_shared)
    {
        int size = slikenet_plain_receive(text, len, sender_addr, (int)sender_length);
        if (size > 0)
            udp_count_received(size);
        return size;
    }

    return (int)dxx_recvfrom(UDP_Socket[socknum], text, len, 0, (struct sockaddr *)sender_addr, &sender_length);
}

char *msg_name(int type)
{
    (void)type;
    return "UDP";
}

void net_log_comment(char *comment)
{
    if (comment)
        con_printf(CON_NET_STATUS, "\033[32mSLikeNet HandShake: %s\033[0m\n",
            comment);
}
