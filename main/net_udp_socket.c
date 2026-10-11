#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "net_udp_socket.h"
#include "net_udp.h"
#include "timer.h"

int UDP_Socket[3] = { -1, -1, -1 };

/* Running totals since the last udp_traffic_stat() call. These count packets
 * rather than averaging packet sizes, so the periodic summary actually tracks
 * Netgame.PacketsPerSec - the per packet lines below only ever show the size
 * of a single packet. */
static unsigned int UDP_num_sendto = 0, UDP_len_sendto = 0;
static unsigned int UDP_num_recvfrom = 0, UDP_len_recvfrom = 0;

/* Seconds between traffic summary lines. */
#define TRAFFIC_STAT_INTERVAL 5

ssize_t dxx_sendto(int sockfd, const void *msg, int len, unsigned int flags,
	const struct sockaddr *to, socklen_t tolen)
{
	ssize_t result = sendto(sockfd, msg, len, flags, to, tolen);
	if (result >= 0)
	{
		UDP_num_sendto++;
		UDP_len_sendto += (unsigned int)result;

		con_printf(CON_NET_TX,
			"\033[38;5;208m[TX] SLikeNet Packet OUT: SENT %d Bytes Traffic\033[0m\n",
			(int)result);
	}
	return result;
}

ssize_t dxx_recvfrom(int sockfd, void *buf, int len, unsigned int flags,
	struct sockaddr *from, socklen_t *fromlen)
{
	ssize_t result = recvfrom(sockfd, buf, len, flags, from, fromlen);
	if (result > 0)
	{
		UDP_num_recvfrom++;
		UDP_len_recvfrom += (unsigned int)result;

		con_printf(CON_NET_RX,
			"\033[34m[RX] SLikeNet Packet IN: Received %d Bytes Traffic\033[0m\n",
			(int)result);
	}
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

	if (now < last_traf_time + (F1_0 * TRAFFIC_STAT_INTERVAL))
		return;

	last_traf_time = now;

	if (UDP_num_sendto)
	{
		con_printf(CON_NET_TX,
			"\033[38;5;208m[TX] SLikeNet Traffic OUT: %.2fKB/s %uPPS\033[0m\n",
			(float)UDP_len_sendto / 1024, UDP_num_sendto);
		UDP_num_sendto = 0;
		UDP_len_sendto = 0;
	}

	if (UDP_num_recvfrom)
	{
		con_printf(CON_NET_RX,
			"\033[34m[RX] SLikeNet Traffic IN: %.2fKB/s %uPPS\033[0m\n",
			(float)UDP_len_recvfrom / 1024, UDP_num_recvfrom);
		UDP_num_recvfrom = 0;
		UDP_len_recvfrom = 0;
	}
}

int udp_dns_filladdr(char *host, int port, struct _sockaddr *sAddr)
{
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	char service[16];

	if (!host || !sAddr || port <= 0 || port > 65535)
		return -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = _af;
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

/* Describe this machine's address so that log lines read as "<host>:<port>".
 * Falls back to "*" when the local address cannot be determined. */
static const char *udp_local_addr_string(void)
{
	static char buf[64];
	struct addrinfo hints, *result = NULL;
	char hostname[256];
	int resolved = 0;

	if (buf[0] != '\0')
		return buf;

	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		hostname[sizeof(hostname) - 1] = '\0';

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = _af;
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

	if (socknum < 0 || socknum >= 3 || port <= 0 || port > 65535)
		return -1;

	udp_close_socket(socknum);
	UDP_Socket[socknum] = socket(_af, SOCK_DGRAM, 0);
	if (UDP_Socket[socknum] < 0)
		return -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = _af;
	hints.ai_socktype = SOCK_DGRAM;
	snprintf(service, sizeof(service), "%d", port);

	if (getaddrinfo(NULL, service, &hints, &result) != 0 || !result ||
		bind(UDP_Socket[socknum], result->ai_addr, result->ai_addrlen) < 0)
	{
		if (result)
			freeaddrinfo(result);
		udp_close_socket(socknum);
		return -1;
	}

	freeaddrinfo(result);
	setsockopt(UDP_Socket[socknum], SOL_SOCKET, SO_BROADCAST,
		(const char *)&broadcast, sizeof(broadcast));
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

	FD_ZERO(&set);
	FD_SET(UDP_Socket[socknum], &set);
	return select(UDP_Socket[socknum] + 1, &set, NULL, NULL, &timeout) > 0;
}

int udp_receive_packet(int socknum, ubyte *text, int len,
	struct _sockaddr *sender_addr)
{
	socklen_t sender_length = sizeof(struct _sockaddr);

	if (socknum < 0 || socknum >= 3 || UDP_Socket[socknum] == -1 ||
		!text || len <= 0)
		return -1;

	if (!udp_general_packet_ready(socknum))
		return 0;

	return (int)dxx_recvfrom(UDP_Socket[socknum], text, len, 0,
		(struct sockaddr *)sender_addr, &sender_length);
}

char *msg_name(int type)
{
	return "UDP";
}

void net_log_comment(char *comment)
{
	if (comment)
		con_printf(CON_NET_STATUS, "\033[32mSLikeNet HandShake: %s\033[0m\n",
			comment);
}
