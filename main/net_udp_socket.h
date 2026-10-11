/* Private interface for the UDP socket layer. */

#ifndef D1X_NET_UDP_SOCKET_H
#define D1X_NET_UDP_SOCKET_H

#include "multi.h"

#ifndef HAVE_SSIZE_T
#ifndef _SSIZE_T_
#define _SSIZE_T_
typedef long _ssize_t;
typedef _ssize_t ssize_t;
#endif
#endif

extern int UDP_Socket[3];

ssize_t dxx_sendto(int sockfd, const void *msg, int len, unsigned int flags,
	const struct sockaddr *to, socklen_t tolen);
ssize_t dxx_recvfrom(int sockfd, void *buf, int len, unsigned int flags,
	struct sockaddr *from, socklen_t *fromlen);
void udp_traffic_stat(void);
int udp_dns_filladdr(char *host, int port, struct _sockaddr *sAddr);
void udp_close_socket(int socknum);
int udp_open_socket(int socknum, int port);
int udp_general_packet_ready(int socknum);
int udp_receive_packet(int socknum, ubyte *text, int len,
	struct _sockaddr *sender_addr);
char *msg_name(int type);
void net_log_comment(char *comment);

#endif
