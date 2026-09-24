#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "net_udp_socket.h"
#include "net_udp.h"

int UDP_Socket[3] = { -1, -1, -1 };

ssize_t dxx_sendto(int sockfd, const void *msg, int len, unsigned int flags,
	const struct sockaddr *to, socklen_t tolen)
{
	ssize_t result = sendto(sockfd, msg, len, flags, to, tolen);
	if (result >= 0)
		con_printf(CON_NET_TX,
			"\033[38;5;208m[TX] SLikeNet Packet OUT: SENT %d Bytes Traffic\033[0m\n",
			(int)result);
	return result;
}

ssize_t dxx_recvfrom(int sockfd, void *buf, int len, unsigned int flags,
	struct sockaddr *from, socklen_t *fromlen)
{
	ssize_t result = recvfrom(sockfd, buf, len, flags, from, fromlen);
	if (result > 0)
		con_printf(CON_NET_RX,
			"\033[34m[RX] SLikeNet Packet IN: Received %d Bytes Traffic\033[0m\n",
			(int)result);
	return result;
}

void udp_traffic_stat(void) {}

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
		"\033[32mSLikeNet is ACTIVE on PORT: %d\033[0m\n", port);
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
