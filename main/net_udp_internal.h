/* Private shared declarations for the UDP implementation units. */

#ifndef D1X_NET_UDP_INTERNAL_H
#define D1X_NET_UDP_INTERNAL_H

#include "net_udp.h"
#include "net_udp_socket.h"

#include "args.h"
#include "byteswap.h"
#include "config.h"
#include "dxxerror.h"
#include "game.h"
#include "gamefont.h"
#include "gamemine.h"
#include "gameseq.h"
#include "gamesave.h"
#include "newdemo.h"
#include "newmenu.h"
#include "physics.h"
#include "playsave.h"
#include "strutil.h"
#include "text.h"
#include "timer.h"
#include "window.h"

extern char UDP_MyPort[6];
extern struct _sockaddr GBcast;
#ifdef IPv6
extern struct _sockaddr GMcast_v6;
#endif

extern UDP_netgame_info_lite Active_udp_games[UDP_MAX_NETGAMES];
extern int num_active_udp_games;
extern int num_active_udp_changed;

extern UDP_sequence_packet UDP_Seq;
extern UDP_mdata_info UDP_MData;
extern UDP_mdata_store UDP_mdata_queue[UDP_MDATA_STOR_QUEUE_SIZE];
extern UDP_mdata_obs_store UDP_mdata_obs_queue[UDP_MDATA_STOR_QUEUE_SIZE];
extern UDP_mdata_recv UDP_mdata_got[MAX_PLAYERS];
extern UDP_sequence_packet UDP_sync_player;
extern int UDP_sync_obsnum;

extern uint netgame_token;
extern uint my_player_token;
extern uint player_tokens[MAX_PLAYERS+4];
extern struct connection_status connection_statuses[8];
extern const struct connection_status CONNECTION_NONE;
extern const char MAX_CONNECTIONS;
extern const ubyte MAX_HOLEPUNCH_ATTEMPTS;

extern ubyte current_pdata;
extern ubyte pdata_received[MAX_PLAYERS][128];
extern ubyte count_pdata_received[MAX_PLAYERS];
extern ubyte last_pdata_received[MAX_PLAYERS];
extern fix64 last_pdata_received_at[MAX_PLAYERS];
extern fix64 last_direct_attempt[MAX_PLAYERS][MAX_PLAYERS];

extern ubyte *observer_data_buffer;
extern fix64 observer_message_timestamps[15 * 8 * 60];
extern int observer_message_offsets[15 * 8 * 60];
extern int observer_message_lengths[15 * 8 * 60];
extern int observer_message_needack[15 * 8 * 60];
extern int cur_obs_msg;
extern int next_obs_msg_to_send;

typedef struct direct_join
{
	struct _sockaddr host_addr;
	int connecting;
	fix64 start_time, last_time;
	char addrbuf[128];
	char portbuf[6];
	ubyte join_as_obs;
} direct_join;

void net_udp_init(void);
void net_udp_close(void);
void net_udp_reset_connection_statuses(void);
void net_udp_set_game_mode(int gamemode, ubyte join_as_obs);
int net_udp_start_game(void);
int net_udp_wait_for_sync(void);
int net_udp_wait_for_requests(void);
int net_udp_select_teams(void);
int net_udp_select_players(void);
int net_udp_send_sync(void);
int net_udp_send_request(void);
void net_udp_send_endlevel_packet(void);
void net_udp_flush(void);
void net_udp_listen(void);
int generate_token(void);
void clean_pdata(fix64 now);
int net_udp_game_connect(direct_join *dj);
void net_udp_send_sequence_packet(UDP_sequence_packet seq,
	struct _sockaddr recv_addr);
void net_udp_broadcast_game_info(ubyte info_upid);
void net_udp_add_player(UDP_sequence_packet *p);
int net_udp_can_join_netgame(netgame_info *game, ubyte join_as_obs);
void net_udp_timeout_check(fix64 time);
#ifdef USE_TRACKER
int udp_tracker_register(void);
int udp_tracker_unregister(void);
int udp_tracker_reqgames(void);
#endif

int net_udp_more_options_handler(newmenu *menu, d_event *event, void *userdata);
void net_udp_more_game_options(void);
int net_udp_sync_poll(newmenu *menu, d_event *event, void *userdata);
int net_udp_start_poll(newmenu *menu, d_event *event, void *userdata);
int net_udp_request_poll(newmenu *menu, d_event *event, void *userdata);
int net_udp_kmatrix_poll1(newmenu *menu, d_event *event, void *userdata);
int net_udp_kmatrix_poll2(newmenu *menu, d_event *event, void *userdata);
int net_udp_show_game_info(void);

void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid,
	ubyte send_to_observers, uint player_token);
void net_udp_request_game_info(struct _sockaddr game_addr, int lite);
int net_udp_process_game_info(ubyte *data, int data_len,
	struct _sockaddr game_addr, int lite_info, ubyte is_sync);
void net_udp_send_netgame_update(void);
void net_udp_send_data(const ubyte *ptr, int len, int priority);
void net_udp_send_mdata(int needack, fix64 time);
void net_udp_send_obs_mdata(fix64 time);
void net_udp_process_packet(ubyte *data, struct _sockaddr sender_addr,
	int length, int is_proxy);
void net_udp_noloss_init_mdata_queue(void);
void net_udp_noloss_process_queue(fix64 time);
void net_udp_noloss_clear_mdata_got(ubyte player_num);

#endif
