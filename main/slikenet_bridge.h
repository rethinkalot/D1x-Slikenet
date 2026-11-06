#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/* In-game traffic runs over SLikeNet on the game port (42424), the same port
 * and socket as plain UDP. Peers are always
 * named by their plain UDP player address (struct _sockaddr), and the bridge
 * keeps the mapping to the SLikeNet address, which NAT may have changed. */

#define SLIKENET_PACKET_NONE  0
#define SLIKENET_PACKET_DATA  1
#define SLIKENET_PACKET_HELLO 2

/* Delivery modes for slikenet_send_to_peer(). UNRELIABLE behaves like plain
 * UDP, and SEQUENCED also drops anything older than the newest packet received.
 * RELIABLE_ORDERED packets all arrive, in order. RELIABLE packets all arrive,
 * in any order. */
#define SLIKENET_UNRELIABLE       0
#define SLIKENET_SEQUENCED        1
#define SLIKENET_RELIABLE_ORDERED 2
#define SLIKENET_RELIABLE         3

/* bind_ip is the local address to bind (NULL = all addresses). */
int  slikenet_startup(unsigned short game_port, const char *bind_ip);
void slikenet_shutdown(void);

/* The lane's UDP socket on game_port, shared with the game: SLikeNet owns it
 * (slikenet_shutdown() closes it) and the game sends plain UDP through it.
 * Datagrams that aren't SLikeNet's are queued for slikenet_plain_receive().
 * socket is -1 if the lane isn't running. */
int  slikenet_get_socket(void);
int  slikenet_plain_ready(void);
int  slikenet_plain_receive(void *buf, int maxlen, void *out_addr, int addr_len);

/* Connect to a peer, or re-send the hello once connected. Call periodically. */
void slikenet_connect_peer(const void *addr, int addr_len, const void *hello, int hello_len);

/* Bind the sender of the last SLIKENET_PACKET_HELLO to this player address.
 * Returns 0 when the sender's IP doesn't match the address. */
int  slikenet_accept_hello(const void *addr, int addr_len);

/* Returns the bytes sent, or -1 if the peer isn't ready and the caller should
 * use plain UDP. mode is one of the SLIKENET_* delivery modes above. */
int  slikenet_send_to_peer(const void *addr, int addr_len, const void *data, int len, int mode);

/* Totals for the current or most recent session. port is 0 if the lane didn't
 * start. Still valid after slikenet_shutdown(). */
void slikenet_get_stats(unsigned int *port, unsigned int *ready, unsigned int *sent, unsigned int *received, unsigned int *fallback);

/* Returns the packet length, or 0 once nothing is left. out_addr is filled for
 * SLIKENET_PACKET_DATA only. */
int  slikenet_receive(void *buf, int maxlen, void *out_addr, int addr_len, int *out_type);

#ifdef __cplusplus
}
#endif
