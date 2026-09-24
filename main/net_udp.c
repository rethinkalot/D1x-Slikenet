/*
 * 
 * Routines for managing UDP-protocol network play.
 * 
 */
#ifdef NETWORK
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef __unix__
#include <sys/time.h>
#endif

#include "pstypes.h"
#include "window.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "dxxerror.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "gameseq.h"
#include "fireball.h"
#include "net_udp_internal.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "cntrlcen.h"
#include "menu.h"
#include "sounds.h"
#include "text.h"
#include "kmatrix.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "vers_id.h"
#include "gamefont.h"
#include "playsave.h"
#include "rbaudio.h"
#include "byteswap.h"
#include "config.h"
#include "vers_id.h"

#ifdef _WIN32
#include <Windows.h>
#include <wincrypt.h>
#endif


// Prototypes
void net_udp_init();
void net_udp_close();
void net_udp_request_game_info(struct _sockaddr game_addr, int lite);
void net_udp_listen();
int net_udp_show_game_info();
int net_udp_do_join_game(ubyte join_as_obs);
int net_udp_can_join_netgame(netgame_info *game, ubyte join_as_obs);
void net_udp_flush();
void net_udp_update_netgame(void);
void net_udp_send_objects(void);
void net_udp_send_rejoin_sync(int player_num);
void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid, ubyte send_to_observers, uint player_token);
void net_udp_send_netgame_update();
void net_udp_do_refuse_stuff (UDP_sequence_packet *their);
void net_udp_read_sync_packet( ubyte * data, int data_len, struct _sockaddr sender_addr );
void net_udp_read_object_packet( ubyte *data );
void net_udp_ping_frame(fix64 time);
void net_udp_p2p_ping_frame(fix64 time); 
void net_udp_process_ping(ubyte *data, int data_len, struct _sockaddr sender_addr);
void net_udp_process_pong(ubyte *data, int data_len, struct _sockaddr sender_addr);
int  net_udp_process_game_info(ubyte *data, int data_len, struct _sockaddr game_addr, int lite_info, ubyte is_sync);
void net_udp_read_endlevel_packet( ubyte *data, int data_len, struct _sockaddr sender_addr );
void net_udp_send_mdata(int needack, fix64 time);
void net_udp_send_obs_mdata(fix64 time);
void net_udp_process_mdata (ubyte *data, int data_len, struct _sockaddr sender_addr, int needack);
void net_udp_process_obs_data (ubyte *data, int data_len, struct _sockaddr sender_addr);
void net_udp_send_pdata();
void net_udp_process_pdata ( ubyte *data, int data_len, struct _sockaddr sender_addr );
void net_udp_read_pdata_packet(UDP_frame_info *pd);
void net_udp_timeout_check(fix64 time);
int net_udp_get_new_player_num (UDP_sequence_packet *their);
void net_udp_noloss_add_queue_pkt(uint32_t pkt_num, fix64 time, ubyte *data, ushort data_size, ubyte pnum, ubyte player_ack[MAX_PLAYERS]);
void net_udp_noloss_obs_add_queue_pkt(uint32_t pkt_num, fix64 time, ubyte *data, ushort data_size, ubyte pnum, ubyte observer_ack[MAX_OBSERVERS]);
int net_udp_noloss_validate_mdata(uint32_t pkt_num, ubyte sender_pnum, struct _sockaddr sender_addr);
void net_udp_noloss_got_ack(ubyte *data, int data_len, struct _sockaddr sender_addr);
void net_udp_noloss_got_obs_ack(ubyte *data, int data_len);
void net_udp_noloss_init_mdata_queue(void);
void net_udp_noloss_clear_mdata_got(ubyte player_num);
void net_udp_noloss_process_queue(fix64 time);
void net_udp_send_extras ();
void net_udp_process_p2p_ping(ubyte *data, struct _sockaddr sender_addr, int data_len);
void net_udp_process_p2p_pong(ubyte *data, struct _sockaddr sender_addr, int data_len);
void net_udp_process_proxy(ubyte *data, struct _sockaddr sender_addr, int data_len);
void net_udp_send_p2p_pong(int to_player, fix64 time, int initiating_connection);
void net_udp_send_to_player(ubyte* data, int len, int to_player);
void net_udp_send_to_player_direct(ubyte* data, int len, int to_player);
void net_udp_send_to_player_proxy(ubyte* data, int len, int to_player, int through_player);
void reattemptDirect(int pnum); 
void resetProxy(int pnum); 
void update_address_for_player(int pnum, struct _sockaddr new_addr); 
void net_udp_send_p2p_reattempt_direct (int to_player, int connect_to_player);
void net_udp_process_p2p_reattempt_direct (ubyte *data, struct _sockaddr sender_addr, int data_len);
void drop_rx_packet(ubyte  *data, char* reason); 

void forward_to_observers(ubyte *data, int data_len, int needack);
void check_observers(fix64 now);
void add_message_to_obs_buffer(ubyte *data, int data_len, int needack);
void check_obs_buffer(fix64 now);
void forward_to_observers_nodelay(ubyte *data, int data_len, int needack);
void net_udp_process_obs_quit(ubyte *data, int data_len, struct _sockaddr sender_addr);

void net_udp_reset_connection_statuses(); 

void net_udp_broadcast_game_info(ubyte info_upid);

#define OBSERVER_DELAY 15
#define MAX_OBS_MESSAGES (OBSERVER_DELAY*8*60)
#define MAX_MESSAGE_SIZE 100 // Not really, but mdata bigger than that is rare

ubyte* observer_data_buffer;
fix64 observer_message_timestamps[MAX_OBS_MESSAGES];
int   observer_message_offsets[MAX_OBS_MESSAGES];
int   observer_message_lengths[MAX_OBS_MESSAGES];
int   observer_message_needack[MAX_OBS_MESSAGES];
int   cur_obs_msg = 0; 
int   next_obs_msg_to_send = 0; 

// Variables
UDP_mdata_info		UDP_MData;
UDP_sequence_packet UDP_Seq;
UDP_mdata_store UDP_mdata_queue[UDP_MDATA_STOR_QUEUE_SIZE];
UDP_mdata_obs_store UDP_mdata_obs_queue[UDP_MDATA_STOR_QUEUE_SIZE];
UDP_mdata_recv UDP_mdata_got[MAX_PLAYERS];
UDP_sequence_packet UDP_sync_player; // For rejoin object syncing
int UDP_sync_obsnum;
UDP_netgame_info_lite Active_udp_games[UDP_MAX_NETGAMES];
int num_active_udp_games = 0;
int num_active_udp_changed = 0;
extern char UDP_MyPort[6];
struct _sockaddr GBcast; // global Broadcast address clients and hosts will use for lite_info exchange over LAN
#ifdef IPv6
struct _sockaddr GMcast_v6; // same for IPv6-only
#endif
#ifdef USE_TRACKER
struct _sockaddr TrackerSocket;
int iTrackerVerified = 0;
#endif
extern obj_position Player_init[MAX_PLAYERS];

uint netgame_token = 0; 
uint my_player_token = 0; 
uint player_tokens[MAX_PLAYERS+4];

const struct connection_status CONNECTION_NONE = {CONNT_NONE, 0, 0};
const char MAX_CONNECTIONS = 8; 
const ubyte MAX_HOLEPUNCH_ATTEMPTS = 30; 

struct connection_status connection_statuses[8]; 

#define MAX_LOSS_BUFFER 128
#define MAX_LOSS_COUNTED 100
ubyte current_pdata = 0; 
ubyte pdata_received[MAX_PLAYERS][MAX_LOSS_BUFFER];
ubyte count_pdata_received[MAX_PLAYERS]; 
ubyte last_pdata_received[MAX_PLAYERS];
fix64 last_pdata_received_at[MAX_PLAYERS]; 

fix64 last_direct_attempt[MAX_PLAYERS][MAX_PLAYERS]; 

void clean_pdata(fix64 now); 

#define PRESET_EXT ".ngs"
#define GAME_PARAM_CHOICE_SHOW_AGAIN -3
int load_preset(newmenu *menu_settings);
void save_preset(void);

/* Tracker stuff, begin! */
#ifdef USE_TRACKER

/* Tracker defines: System stuff */
#define TRACKER_SYS_VERSION 0x00
#define TRACKER_PKT_REGISTER 0
#define TRACKER_PKT_UNREGISTER 1
#define TRACKER_PKT_GAMELIST 2

int udp_tracker_init()
{
	int tracker_port = d_rand() % 0xffff;

	while (tracker_port <= 1024)
		tracker_port = d_rand() % 0xffff;

	// Zero it out
	memset( &TrackerSocket, 0, sizeof( TrackerSocket ) );
	
	// Open the socket
	udp_open_socket( 2, tracker_port );
	
	// Fill the address
	if( udp_dns_filladdr( (char *)GameArg.MplTrackerAddr, GameArg.MplTrackerPort, &TrackerSocket ) < 0 )
		return -1;
	
	// Yay
	return 0;
}

/* Unregister from the tracker */
int udp_tracker_unregister()
{
	// Variables
	int iLen = 5;
	ubyte* pBuf;
	ssize_t result;

	pBuf = malloc(iLen);
	
	// Put the opcode
	pBuf[0] = TRACKER_PKT_UNREGISTER;
	
	// Put the GameID
	PUT_INTEL_INT( pBuf+1, Netgame.protocol.udp.GameID );
	
	// Send it off
	result = dxx_sendto( UDP_Socket[2], pBuf, iLen, 0, (struct sockaddr *)&TrackerSocket, sizeof( TrackerSocket ) );

	free(pBuf);

	return result;
}

/* Tell the tracker we're starting a game */
int udp_tracker_register()
{
	// Variables
	int iLen = 15; // Thanks Adam Gensler
	ubyte* pBuf;
	ssize_t result;

	pBuf = malloc(iLen);
	
	// Reset the last tracker message
	iTrackerVerified = 0;
	
	// Put the opcode
	pBuf[0] = TRACKER_PKT_REGISTER;
	
	// Put the protocol version
	pBuf[1] = TRACKER_SYS_VERSION;
	
	// Write the game version (d1 = 1, d2 = 2, x = oshiii)
	pBuf[2] = 0x01;
	
	// Write the port we're running on
	PUT_INTEL_SHORT( pBuf+3, atoi( UDP_MyPort ) );
	
	// Put the GameID
	PUT_INTEL_INT( pBuf+5, Netgame.protocol.udp.GameID );
	
	// Now, put the game version
	PUT_INTEL_SHORT( pBuf+9, DXX_VERSION_MAJORi );
	PUT_INTEL_SHORT( pBuf+11, DXX_VERSION_MINORi );
	PUT_INTEL_SHORT( pBuf+13, DXX_VERSION_MICROi );
	
	// Send it off
	result = dxx_sendto( UDP_Socket[2], pBuf, iLen, 0, (struct sockaddr *)&TrackerSocket, sizeof( TrackerSocket ) );

	free(pBuf);

	return result;
}

/* Ask the tracker to send us a list of games */
int udp_tracker_reqgames()
{
	// Variables
	int iLen = 3;
	ubyte* pBuf;
	ssize_t result;

	pBuf = malloc(iLen);
	
	// Put the opcode
	pBuf[0] = TRACKER_PKT_GAMELIST;
	
// 	// Put the game version (d1)
	pBuf[1] = 1;
	
	// If we're IPv6 ready, send that too
#ifdef IPv6
	pBuf[2] = 1;
#else
	pBuf[2] = 0;
#endif
	
	// Send it off
	result = dxx_sendto( UDP_Socket[2], pBuf, iLen, 0, (struct sockaddr *)&TrackerSocket, sizeof( TrackerSocket ) );

	free(pBuf);

	return result;
}

/* The tracker has sent us a game.  Let's list it. */
int udp_tracker_process_game( ubyte *data, int data_len )
{
	// All our variables
	struct _sockaddr sAddr;
	int iPos = 1;
	int iPort = 0;
	int bIPv6 = 0;
	char *sIP = NULL;
	
	// Zero it out
	memset( &sAddr, 0, sizeof( sAddr ) );
	
	// Get the IPv6 flag from the tracker
	bIPv6 = data[iPos++];
	(void)bIPv6; // currently unused
	
	// Get the IP
	sIP = (char *)&data[iPos];
	iPos += strlen( sIP ) + 1;
	
	// Get the port
	iPort = GET_INTEL_SHORT( &data[iPos] );
	iPos += 2;
	
	// Get the DNS stuff
	if( udp_dns_filladdr( sIP, iPort, &sAddr ) < 0 )
		return -1;
	
	// Now move on to BIGGER AND BETTER THINGS!
	net_udp_process_game_info( &data[iPos - 1], data_len - iPos, sAddr, 1 , 0);
	return 0;
}
#endif /* USE_TRACKER */



int generate_token() {
#ifdef _WIN32
  // WIN32 cryptographically secure random
  HCRYPTPROV prov;
  if (CryptAcquireContext(&prov, NULL, NULL,
                          PROV_RSA_FULL, 0)) {
    int li = 0;
    if (CryptGenRandom(prov, sizeof(li), (BYTE *)&li)) { 	
      return li; 
    }	
   }

   // Not cryptographically secure, but apparently we can't do that
   con_printf(CON_DEBUG, "Using cryptographically insecure token.\n"); 
   srand(time(NULL));
   return rand(); 
#else

    // POSIX secure random
    return random(); 
#endif


}

void drop_rx_packet(ubyte  *data, char* reason) {
	char comment[200];
	snprintf(comment, 199, "Dropped %s: %s\n", msg_name(data[0]), reason); 
	net_log_comment(comment);
	con_printf(CON_URGENT, comment); 
}

int is_same_addr(struct _sockaddr *addr1, struct _sockaddr *addr2) {
	return !memcmp(&addr1->sin_addr, &addr2->sin_addr, sizeof(struct in_addr)) &&
		addr1->sin_port == addr2->sin_port;
}

int is_master_ip(struct _sockaddr addr) {
	return is_same_addr(&Netgame.players[0].protocol.udp.addr, &addr);
}

int is_player_ip(struct _sockaddr addr, int pnum) {
	return is_same_addr(&Netgame.players[pnum].protocol.udp.addr, &addr);
}

int is_observer_ip(struct _sockaddr addr) {
	for(int i = 0; i < Netgame.max_numobservers; i++) {
		if (is_same_addr(&Netgame.observers[i].protocol.udp.addr, &addr)) {
			return 1;
		}
	}
	return 0;
}

int is_any_player_ip(struct _sockaddr addr) {
	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(Players[i].connected == CONNECT_DISCONNECTED) continue;
		if(i == Player_num) continue;

		if(is_player_ip(addr, i)) return 1;
	}

	return 0; 
}

int valid_sender(ubyte *data, int data_len, struct _sockaddr sender_addr) {
	ubyte pid = data[0];

	switch(pid) {

		// Can only be sent by master
		case UPID_VERSION_DENY: 
		case UPID_GAME_INFO: 
		case UPID_DUMP: 
		case UPID_ADDPLAYER:
		case UPID_SYNC: 
		case UPID_OBJECT_DATA:
		case UPID_PING: 
		case UPID_ENDLEVEL_H: 
		case UPID_REATTEMPT_DIRECT: 
			if(multi_i_am_master()) {
				drop_rx_packet(data, "received by game master"); 
				return 0; 
			}
			break;

		// Can only be received by master
		case UPID_GAME_INFO_REQ: 
		case UPID_GAME_INFO_LITE_REQ: 
		case UPID_REQUEST: 
		case UPID_QUIT_JOINING: 
		case UPID_PONG: 
		case UPID_ENDLEVEL_C: 
			if(! multi_i_am_master()) {
				drop_rx_packet(data, "received by non-game master"); 
				return 0; 				
			}

	}

	return 1;	
}


int valid_ip(ubyte *data, int data_len, struct _sockaddr sender_addr) {
	ubyte pid = data[0];

	switch(pid) {

		// Can only be sent by master
		case UPID_VERSION_DENY: 
		case UPID_GAME_INFO: 
		case UPID_DUMP: 
		case UPID_ADDPLAYER:
		// case UPID_SYNC: 
		// case UPID_OBJECT_DATA:
		case UPID_PING: 
		case UPID_ENDLEVEL_H: 
		case UPID_REATTEMPT_DIRECT: 
			if(! is_master_ip(sender_addr)) {
				drop_rx_packet(data, "sent from ip not belonging to game master"); 
				return 0; 
			}

		// Player-based IP checking done in handle_ / process_ functions 
	}

	return 1;
}

int valid_size(ubyte *data, int data_len, struct _sockaddr sender_addr) {
	ubyte pid = data[0];
	int rv = 1; 

	switch(pid) {
		case UPID_VERSION_DENY:  		if(data_len != UPID_VERSION_DENY_SIZE      )  { rv = 0; }  break;
		case UPID_GAME_INFO_REQ: 		if(data_len != UPID_GAME_INFO_REQ_SIZE     ) { rv = 0; }  break;
		//case UPID_GAME_INFO_LITE_REQ: 	if(data_len != UPID_GAME_INFO_LITE_REQ_SIZE)  { rv = 0; }  break;
		case UPID_GAME_INFO_LITE:    	if(data_len != UPID_GAME_INFO_LITE_SIZE    )  { rv = 0; }  break;
		case UPID_DUMP:   			 	if(data_len != UPID_DUMP_SIZE              )  { rv = 0; }  break;
		case UPID_QUIT_JOINING: 
		case UPID_REQUEST:   		 	if(data_len != UPID_SEQUENCE_SIZE          )  { rv = 0; }  break;
		case UPID_OBJECT_DATA:   	 	if(data_len > UPID_MAX_SIZE         	   )  { rv = 0; }  break;
		case UPID_PING:   	 			if(data_len != UPID_PING_SIZE         	   )  { rv = 0; }  break;
		case UPID_PONG:   	 			if(data_len != UPID_PONG_SIZE        	   )  { rv = 0; }  break;
		case UPID_P2P_PING: 			if(data_len != UPID_P2P_PING_SIZE          )  { rv = 0; }  break;
		case UPID_P2P_PONG: 			if(data_len != UPID_P2P_PONG_SIZE          )  { rv = 0; }  break;
		case UPID_REATTEMPT_DIRECT: 	if(data_len != UPID_REATTEMPT_DIRECT_SIZE  )  { rv = 0; }  break;

		// Special cases
		case UPID_GAME_INFO:     		rv = 1; break; // Don't check, it varies
		case UPID_SYNC: 	    		rv = 1; break; 
		case UPID_ADDPLAYER:   			rv = 1; break;

		default: rv = 1; 
	}

	if(! rv ) {
		char err_mess[100];
		snprintf(err_mess, 100, "illegal packet size (%d bytes)", data_len); 
		drop_rx_packet(data, err_mess); 
	}

	return rv;
}

int valid_token(ubyte *data, int data_len, struct _sockaddr sender_addr) {
	ubyte pid = data[0];
	char err_mess[200];
	int rv; 

	switch(pid) {
		case UPID_ADDPLAYER:
		case UPID_ENDLEVEL_H:
		case UPID_ENDLEVEL_C:
		case UPID_PDATA:	
		case UPID_MDATA_PNORM:
		case UPID_OBSDATA:
		case UPID_OBSQUIT:
		case UPID_MDATA_PNEEDACK:
		case UPID_P2P_PING: 
		case UPID_P2P_PONG: 
		case UPID_PROXY:
		case UPID_REATTEMPT_DIRECT:		
		// case UPID_SYNC: // Special case is handled in sync processing
			rv = GET_INTEL_INT(data + 1) == netgame_token; 

			if(! rv) {				
				sprintf(err_mess, "token %d != %d", GET_INTEL_INT(data + 1), netgame_token );
				drop_rx_packet(data, err_mess);
			}
			return rv; 


		case UPID_OBJECT_DATA:
			rv = GET_INTEL_INT(data + 1) == my_player_token; 

			if(! rv) {				
				sprintf(err_mess, "token %d != %d", GET_INTEL_INT(data + 1), my_player_token );
				drop_rx_packet(data, err_mess);
			}
			return rv; 

		case UPID_DUMP:		
			rv = (GET_INTEL_INT(data + 1) == netgame_token) || (GET_INTEL_INT(data + 1) == my_player_token);

			if(! rv) {				
				sprintf(err_mess, "token %d != %d || %d", GET_INTEL_INT(data + 1), netgame_token, my_player_token );
				drop_rx_packet(data, err_mess);
			}
			return rv; 
	}

	return 1;
}

int valid_netgame_status(ubyte *data, int data_len, struct _sockaddr sender_addr) {
	ubyte pid = data[0];

	int rv = 1; 
	switch(pid) {
		case UPID_DUMP:
			if((Network_status == NETSTAT_WAITING) || (Network_status == NETSTAT_PLAYING)) {
				rv = 1;
			} else {
				rv = 0; 
			}
		break;

		//case UPID_SYNC:
		//	if(Network_status == NETSTAT_WAITING) {
		//		rv = 1;
		//	} else {
		//		rv = 0; 
		//	}
		//break;		

		case UPID_ENDLEVEL_H:
		case UPID_ENDLEVEL_C: 
			if((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING)) {
				rv = 1;
			} else {
				rv = 0; 
			}
		break;				
	}

	if(! rv ) {
		drop_rx_packet(data, "illegal netgame status"); 
	}

	return rv; 
}

int pass_security_check(ubyte *data, struct _sockaddr sender_addr, int data_len, int check_ip) {
	return ((! check_ip) || valid_ip(data, data_len, sender_addr)) &&
	       valid_size(data, data_len, sender_addr) &&
	       valid_token(data, data_len, sender_addr) &&
	       valid_sender(data, data_len, sender_addr) &&
	       valid_netgame_status(data, data_len, sender_addr); 
}



// Connect to a game host and get full info. Eventually we join!
int net_udp_game_connect(direct_join *dj)
{
	// Get full game info so we can show it.

	// Timeout after 10 seconds
	if (timer_query() >= dj->start_time + (F1_0*10))
	{
		nm_messagebox(TXT_ERROR,1,TXT_OK,"No response by host.\n\nPossible reasons:\n* No game on this IP (anymore)\n* Port of Host not open\n  or different\n* Host uses a game version\n  I do not understand");
		dj->connecting = 0;
		return 0;
	}
	
	if (Netgame.protocol.udp.valid == -1)
	{
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Version mismatch! Cannot join Game.\nHost game version: %i.%i.%i\nHost game protocol: %i\nYour game version: %s\nYour game protocol: %i",Netgame.protocol.udp.program_iver[0],Netgame.protocol.udp.program_iver[1],Netgame.protocol.udp.program_iver[2],Netgame.protocol.udp.program_iver[3],VERSION, MULTI_PROTO_VERSION);
		dj->connecting = 0;
		return 0;
	}
	
	if (timer_query() >= dj->last_time + F1_0)
	{
		net_udp_request_game_info(dj->host_addr, 0);
		dj->last_time = timer_query();
	}
	timer_delay2(5);
	net_udp_listen();

	if (Netgame.protocol.udp.valid != 1)
		return 0;		// still trying to connect

	if (dj->connecting == 1)
	{
		int show_info_result = net_udp_show_game_info();
		if (!show_info_result) // show info menu and check if we join
		{
			dj->connecting = 0;
			return 0;
		}
		else
		{
			// Get full game info again as it could have changed since we entered the info menu.
			dj->connecting = 2;
			Netgame.protocol.udp.valid = 0;
			dj->start_time = timer_query();

			if(show_info_result == 2) {
				dj->join_as_obs = 1;
			} else {
				dj->join_as_obs = 0;
			}

			return 0;
		}
	}
		
	dj->connecting = 0;

	return net_udp_do_join_game(dj->join_as_obs);
}


int color_used(int wingcolor, int missilecolor, int ignore) {
	for(int i = (Netgame.host_is_obs ? 1 : 0); i < N_players; i++) {
		if(i == ignore) continue;

		if(Netgame.players[i].color == wingcolor &&
		   Netgame.players[i].missilecolor == missilecolor )
			return 1;
	}

	return 0;
}

void resolve_color_conflicts(int np) { // New Player
	int new_wing_color = Netgame.players[np].color;
	int new_missile_color = Netgame.players[np].missilecolor;

	while(color_used(new_wing_color, new_missile_color, np)) {

		// Fix it
		new_wing_color    = rand() % 8;
		new_missile_color = rand() % 8;

		Netgame.players[np].color = new_wing_color;
		Netgame.players[np].missilecolor = new_missile_color;
	}
}

void net_udp_send_sequence_packet(UDP_sequence_packet seq, struct _sockaddr recv_addr)
{
	int len = 0;
	ubyte buf[UPID_SEQUENCE_SIZE];

	len = 0;
	memset(buf, 0, sizeof(buf));
	buf[0] = seq.type;						len++;
	PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
	memcpy(&buf[len], seq.player.callsign, CALLSIGN_LEN+1);		len += CALLSIGN_LEN+1;
	buf[len] = seq.player.connected;				len++;
	buf[len] = seq.player.rank;					len++;
	buf[len] = seq.player.color;				len++;
	buf[len] = seq.player.missilecolor;				len++;
	buf[len] = is_observer() ? 1 : 0;     len++; 
	memcpy(buf + len, &seq.player.protocol.udp.addr, sizeof(struct _sockaddr));  len += sizeof(struct _sockaddr); 
	
	dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&recv_addr, sizeof(struct _sockaddr));
}

void net_udp_receive_sequence_packet(ubyte *data, UDP_sequence_packet *seq, struct _sockaddr sender_addr)
{
	int len = 0;
	
	seq->type = data[0];						len++;
	seq->token = GET_INTEL_INT(data + len); len += 4;  
	memcpy(seq->player.callsign, &(data[len]), CALLSIGN_LEN+1);	len += CALLSIGN_LEN+1;
	seq->player.connected = data[len];				len++;   
	memcpy (&(seq->player.rank),&(data[len]),1);			len++;
	seq->player.color = data[len]; len++; 
	seq->player.missilecolor = data[len]; len++; 
	seq->player.observer = data[len]; len++;  

	if(seq->type == UPID_ADDPLAYER ) {
		memcpy(&(seq->player.protocol.udp.addr), data + len, sizeof(struct _sockaddr)); len += sizeof(struct _sockaddr);

		struct sockaddr_in *addrin = (struct sockaddr_in*) &seq->player.protocol.udp.addr;
		char *ip = inet_ntoa(addrin->sin_addr); 
		ushort port = SWAPSHORT(addrin->sin_port);
		con_printf(CON_DEBUG, "New player joined from ip %s port %d\n", ip, port); 

	} else if (multi_i_am_master()) {
		memcpy(&seq->player.protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr));
	}

}

void net_udp_init()
{
	// So you want to play a netgame, eh?  Let's a get a few things straight

#ifdef _WIN32
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSACleanup();
	if (WSAStartup( wVersionRequested, &wsaData))
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "Cannot init Winsock!"); // no break here... game will fail at socket creation anyways...
}
#endif

	if( UDP_Socket[0] != -1 )
		udp_close_socket(0);
	if( UDP_Socket[1] != -1 )
		udp_close_socket(1);

	memset(&Netgame, 0, sizeof(netgame_info));
	memset(&UDP_Seq, 0, sizeof(UDP_sequence_packet));
	memset(&UDP_MData, 0, sizeof(UDP_mdata_info));
	net_udp_noloss_init_mdata_queue();
	UDP_Seq.type = UPID_REQUEST;
	memcpy(UDP_Seq.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);

	UDP_Seq.player.rank=GetMyNetRanking();	
	UDP_Seq.player.color = PlayerCfg.ShipColor; 
	UDP_Seq.player.missilecolor = PlayerCfg.MissileColor; 

	multi_new_game();
	net_udp_reset_connection_statuses();
	net_udp_flush();
	
	UDP_Seq.token = netgame_token;
#ifdef USE_TRACKER
	// Initialize the tracker info
	udp_tracker_init();
#endif
}
void net_udp_close()
{
#ifdef _WIN32
#endif

	if( UDP_Socket[0] != -1 )
		udp_close_socket(0);
	if( UDP_Socket[1] != -1 )
		udp_close_socket(1);
	if( UDP_Socket[2] != -1 )
		udp_close_socket(2);
}

// Send PID_ENDLEVEL in regular intervals and listen for them (host also does the packets for playing clients)

int net_udp_endlevel(int *secret)
{
	// Do whatever needs to be done between levels

	int i;

	// We do not really check if a player has actually found a secret level... yeah, I am too lazy! So just go there and pretend we did!
	for (i = 0; i < N_secret_levels; i++)
	{
		if (Current_level_num == Secret_level_table[i])
		{
			*secret = 1;
			break;
		}
	}

	Network_status = NETSTAT_ENDLEVEL; // We are between levels
	net_udp_listen();
	if (!is_observer())
		net_udp_send_endlevel_packet();

	for (i=0; i<N_players; i++) 
	{
		Netgame.players[i].LastPacketTime = timer_query();
	}

	net_udp_update_netgame();

	return(0);
}

int 
net_udp_can_join_netgame(netgame_info *game, ubyte join_as_obs)
{
	// Can this player rejoin a netgame in progress?

	int i, num_players;

	if (game->game_status == NETSTAT_STARTING)
		return 1;

	if (game->game_status != NETSTAT_PLAYING)
		return 0;

	if (join_as_obs) 
		return 1; 

	// Game is in progress, figure out if this guy can re-join it

	num_players = game->numplayers;

	if (!(game->game_flags & NETGAME_FLAG_CLOSED)) {
		// Look for player that is not connected
		
		if (game->numconnected==game->max_numplayers)
			return (2);
		
		if (game->RefusePlayers)
			return (3);
		
		if (game->numplayers < game->max_numplayers)
			return 1;

		if (game->numconnected<num_players)
			return 1;
	}

	// Search to see if we were already in this closed netgame in progress

	for (i = 0; i < num_players; i++)
		if ( (!d_stricmp(Players[Player_num].callsign, game->players[i].callsign)) && game->players[i].protocol.udp.isyou )
			break;

	if (i != num_players)
		return 1;

	return 0;
}

// do UDP stuff to disconnect a player. Should ONLY be called from multi_disconnect_player()
void net_udp_disconnect_player(int playernum)
{
	// A player has disconnected from the net game, take whatever steps are
	// necessary 

	if (playernum == Player_num) 
	{
		Int3(); // Weird, see Rob
		return;
	}

	if (VerifyPlayerJoined==playernum)
		VerifyPlayerJoined=-1;

	net_udp_noloss_clear_mdata_got(playernum);
}

void
net_udp_new_player(UDP_sequence_packet *their)
{
	int pnum;

	pnum = their->player.connected;

	Assert(pnum >= 0);
	Assert(pnum < Netgame.max_numplayers);
	
	if (Newdemo_state == ND_STATE_RECORDING) {
		int new_player;

		if (pnum == N_players)
			new_player = 1;
		else
			new_player = 0;
		newdemo_record_multi_connect(pnum, new_player, their->player.callsign);
	}

	memcpy(Players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
	memcpy(Netgame.players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
	if(Netgame.AllowPreferredColors) { 
		Netgame.players[pnum].color = their->player.color; 
		if(their->player.color > 7) { Netgame.players[pnum].color = pnum; }

		Netgame.players[pnum].missilecolor = their->player.missilecolor; 		
		if(their->player.missilecolor > 7) { Netgame.players[pnum].missilecolor = Netgame.players[pnum].color; }

		resolve_color_conflicts(pnum);
	} else {
		Netgame.players[pnum].color = pnum; 
		Netgame.players[pnum].missilecolor = pnum; 
	}
	//memcpy(&Netgame.players[pnum].protocol.udp.addr, &their->player.protocol.udp.addr, sizeof(struct _sockaddr));
	update_address_for_player(pnum, their->player.protocol.udp.addr);

	if(multi_i_am_master()) {
		connection_statuses[pnum].type = CONNT_DIRECT;
		Netgame.players[pnum].ping = 0; 
	} else {
		connection_statuses[pnum].type = CONNT_PROXY;
		connection_statuses[pnum].proxy_through = 0; // host
		connection_statuses[pnum].holepunch_attempts = 0;
	}

	struct sockaddr_in *addrin = (struct sockaddr_in*) &their->player.protocol.udp.addr;
	char *ip = inet_ntoa(addrin->sin_addr); 
	ushort port = SWAPSHORT(addrin->sin_port);
	con_printf(CON_DEBUG, "Received new player num %d at ip %s port %d\n", pnum, ip, port); 

	ClipRank (&their->player.rank);
	Netgame.players[pnum].rank=their->player.rank;

	Players[pnum].connected = CONNECT_PLAYING;
	Players[pnum].net_kills_total = 0;
	Players[pnum].net_killed_total = 0;
	memset(kill_matrix[pnum], 0, MAX_PLAYERS*sizeof(short)); 
	Players[pnum].score = 0;
	Players[pnum].flags = 0;
	Players[pnum].KillGoalCount=0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	ClipRank (&their->player.rank);

	if (PlayerCfg.NoRankings)
		HUD_init_message(HM_MULTI, "'%s' %s\n",their->player.callsign, TXT_JOINING);
	else
		HUD_init_message(HM_MULTI, "%s'%s' %s\n",RankStrings[their->player.rank],their->player.callsign, TXT_JOINING);
	
	multi_make_ghost_player(pnum);

	multi_send_score();

	net_udp_noloss_clear_mdata_got(pnum);
}

void net_udp_welcome_player(UDP_sequence_packet *their)
{
	// Add a player to a game already in progress
	int player_num;
	int i;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Endlevel_sequence) || (Control_center_destroyed))
	{
		net_log_comment("new player dumped due to endlevel sequence");
		net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_ENDLEVEL);
		return; 
	}

	if (Network_send_objects || Network_sending_extras)
	{
		// Ignore silently, we're already responding to someone and we can't
		// do more than one person at a time.  If we don't dump them they will
		// re-request in a few seconds.
		net_log_comment("new player ignored due to player already joining");
		return;
	}

	if (their->player.connected != Current_level_num)
	{
		net_log_comment("new player dumped due to wrong level number");
		net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_LEVEL);
		return;
	}

	if(their->player.observer) {
		if(Netgame.numobservers >= Netgame.max_numobservers) {
			net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_FULL);
			return;
		}
	}

	player_num = -1;
	memset(&UDP_sync_player, 0, sizeof(UDP_sequence_packet));
	Network_player_added = 0;

	if(their->player.observer) {
		Netgame.numobservers++;

		int obsnum = 0;

		while (Netgame.observers[obsnum].connected == 1) {
			obsnum++;
		}

		UDP_sync_player = *their;
		UDP_sync_player.player.connected = OBSERVER_PLAYER_ID;
		UDP_sync_obsnum = obsnum;
		Network_send_objects = 1;
		Network_send_objnum = -1;
		Netgame.observers[obsnum].LastPacketTime = timer_query();
		Netgame.observers[obsnum].connected = 0; // Not yet connected, see net_udp_send_rejoin_sync
		Netgame.observers[obsnum].protocol.udp.addr = their->player.protocol.udp.addr;
		strncpy((char*) &Netgame.observers[obsnum].callsign, (char*) &their->player.callsign, 8); 

		HUD_init_message(HM_MULTI, "%s is now observing.", UDP_sync_player.player.callsign);

		net_udp_send_objects();

		return;
	}

	for (i = 0; i < N_players; i++)
	{
		if ((!d_stricmp(Players[i].callsign, their->player.callsign )) && !memcmp((struct _sockaddr *)&their->player.protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			player_num = i;
			break;
		}
	}

	if (player_num == -1)
	{
		// Player is new to this game

		if ( !(Netgame.game_flags & NETGAME_FLAG_CLOSED) && (N_players < Netgame.max_numplayers))
		{
			// Add player in an open slot, game not full yet

			player_num = N_players;
			Network_player_added = 1;
		}
		else if (Netgame.game_flags & NETGAME_FLAG_CLOSED)
		{
			// Slots are open but game is closed

			net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_CLOSED);
			return;
		}
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix64 oldest_time = timer_query();
			int activeplayers = 0;

			Assert(N_players == Netgame.max_numplayers);

			for (i = 0; i < Netgame.numplayers; i++)
				if (Netgame.players[i].connected)
					activeplayers++;

			if (activeplayers == Netgame.max_numplayers)
			{
				// Game is full.
				net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_FULL);
				return;
			}

			for (i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (Netgame.players[i].LastPacketTime < oldest_time))
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}

			if (oldest_player == -1)
			{
				// Everyone is still connected 

				net_udp_dump_player(their->player.protocol.udp.addr, their->token,  DUMP_FULL);
				return;
			}
			else
			{
				// Found a slot!

				player_num = oldest_player;
				Network_player_added = 1;
			}
		}
	}
	else
	{
		// Player is reconnecting
		
		if (Players[player_num].connected)
		{
			return;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);

		Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		if (PlayerCfg.NoRankings)
			HUD_init_message(HM_MULTI, "'%s' %s", Players[player_num].callsign, TXT_REJOIN);
		else
			HUD_init_message(HM_MULTI, "%s'%s' %s", RankStrings[Netgame.players[player_num].rank],Players[player_num].callsign, TXT_REJOIN);

		multi_send_score();

		net_udp_noloss_clear_mdata_got(player_num);
	}

	Players[player_num].KillGoalCount=0;

	// Send updated Objects data to the new/returning player

	UDP_sync_player = *their;
	UDP_sync_player.player.connected = player_num;
	player_tokens[player_num] = UDP_sync_player.token; 	
	Network_send_objects = 1;
	Network_send_objnum = -1;
	Netgame.players[player_num].LastPacketTime = timer_query();

	net_udp_send_objects();
}

int net_udp_objnum_is_past(int objnum)
{
	// determine whether or not a given object number has already been sent
	// to a re-joining player.
	
	int player_num = UDP_sync_player.player.connected;
	int obj_mode = !((object_owner[objnum] == -1) || (object_owner[objnum] == player_num));

	if (!Network_send_objects)
		return 0; // We're not sending objects to a new player

	if (obj_mode > Network_send_object_mode)
		return 0;
	else if (obj_mode < Network_send_object_mode)
		return 1;
	else if (objnum < Network_send_objnum)
		return 1;
	else
		return 0;
}

void net_udp_send_door_updates(void)
{
	// Send door status when new player joins
	
	int i;

	for (i = 0; i < Num_walls; i++)
	{
		if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING)))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum,0);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum,0);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
	}

}	

void net_udp_process_monitor_vector(int vector)
{
	int i, j;
	int count = 0;
	segment *seg;
	
	for (i=0; i <= Highest_segment_index; i++)
	{
		int tm, ec, bm;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ( ((tm = seg->sides[j].tmap_num2) != 0) &&
				  ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
				  ((bm = Effects[ec].dest_bm_num) != -1) )
			{
				if (vector & (1 << count))
				{
					seg->sides[j].tmap_num2 = bm | (tm&0xc000);
				}
				count++;
				Assert(count < 32);
			}
		}
	}
}

int net_udp_create_monitor_vector(void)
{
	int i, j, k;
	int num_blown_bitmaps = 0;
	int monitor_num = 0;
	int blown_bitmaps[7];
	int vector = 0;
	segment *seg;

	for (i=0; i < Num_effects; i++)
	{
		if (Effects[i].dest_bm_num > 0) {
			for (j = 0; j < num_blown_bitmaps; j++)
				if (blown_bitmaps[j] == Effects[i].dest_bm_num)
					break;
			if (j == num_blown_bitmaps)
				blown_bitmaps[num_blown_bitmaps++] = Effects[i].dest_bm_num;
		}
	}
		

	Assert(num_blown_bitmaps <= 7);

	for (i=0; i <= Highest_segment_index; i++)
	{
		int tm, ec;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ((tm = seg->sides[j].tmap_num2) != 0) 
			{
				if ( ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
					  (Effects[ec].dest_bm_num != -1) )
				{
					monitor_num++;
					Assert(monitor_num < 32);
				}
				else
				{
					for (k = 0; k < num_blown_bitmaps; k++)
					{
						if ((tm&0x3fff) == blown_bitmaps[k])
						{
							vector |= (1 << monitor_num);
							monitor_num++;
							Assert(monitor_num < 32);
							break;
						}
					}
				}
			}
		}
	}
	return(vector);
}

void net_udp_stop_resync(UDP_sequence_packet *their)
{
	if ( (!memcmp((struct _sockaddr *)&UDP_sync_player.player.protocol.udp.addr, (struct _sockaddr *)&their->player.protocol.udp.addr, sizeof(struct _sockaddr))) &&
		(!d_stricmp(UDP_sync_player.player.callsign, their->player.callsign)) )
	{
		Network_send_objects = 0;
		Network_sending_extras=0;
		Network_rejoined=0;
		Player_joining_extras=-1;
		Network_send_objnum = -1;
	}
}

ubyte object_buffer[UPID_MAX_SIZE];

void net_udp_send_objects(void)
{
	sbyte owner, player_num = UDP_sync_player.player.connected;

	if (UDP_sync_player.player.observer) {
		player_num = OBSERVER_PLAYER_ID;
	}
	
	static int obj_count = 0;
	int loc = 0, i = 0, remote_objnum = 0, obj_count_frame = 0;
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(UDP_sync_player.player.observer || player_num < Netgame.max_numplayers);

	if (Endlevel_sequence || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.
		net_log_comment("sending objects stopped due to end level");
		net_udp_dump_player(UDP_sync_player.player.protocol.udp.addr, UDP_sync_player.token, DUMP_ENDLEVEL);
		Network_send_objects = 0; 
		return;
	}

	memset(object_buffer, 0, UPID_MAX_SIZE);
	object_buffer[0] = UPID_OBJECT_DATA;
	PUT_INTEL_INT(object_buffer + 1, UDP_sync_player.token); 
	loc = 9;

	if (Network_send_objnum == -1)
	{
		obj_count = 0;
		Network_send_object_mode = 0;
		PUT_INTEL_INT(object_buffer+loc, -1);                       loc += 4;
		object_buffer[loc] = player_num;                            loc += 1;
		/* Placeholder for remote_objnum, not used here */          loc += 4;
		Network_send_objnum = 0;
		obj_count_frame = 1;
	}
	
	for (i = Network_send_objnum; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type != OBJ_POWERUP) && (Objects[i].type != OBJ_PLAYER) &&
				(Objects[i].type != OBJ_CNTRLCEN) && (Objects[i].type != OBJ_GHOST) &&
				(Objects[i].type != OBJ_ROBOT) && (Objects[i].type != OBJ_HOSTAGE))
			continue;
		if ((Network_send_object_mode == 0) && ((object_owner[i] != -1) && (object_owner[i] != player_num)))
			continue;
		if ((Network_send_object_mode == 1) && ((object_owner[i] == -1) || (object_owner[i] == player_num)))
			continue;

		if ( loc + sizeof(object_rw) + 9 > UPID_MAX_SIZE-1 )
			break; // Not enough room for another object

		obj_count_frame++;
		obj_count++;

		remote_objnum = objnum_local_to_remote(i, &owner);
		Assert(owner == object_owner[i]);

		PUT_INTEL_INT(object_buffer+loc, i);                        loc += 4;
		object_buffer[loc] = owner;                                 loc += 1;
		PUT_INTEL_INT(object_buffer+loc, remote_objnum);            loc += 4;
		// use object_rw to send objects for now. if object sometime contains some day contains something useful the client should know about, we should use it. but by now it's also easier to use object_rw because then we also do not need fix64 timer values.
		multi_object_to_object_rw(&Objects[i], (object_rw *)&object_buffer[loc]);
#ifdef WORDS_BIGENDIAN
		object_rw_swap((object_rw *)&object_buffer[loc], 1);
#endif
		loc += sizeof(object_rw);
	}

	if (obj_count_frame) // Send any objects we've buffered
	{
		Network_send_objnum = i;
		PUT_INTEL_INT(object_buffer+5, obj_count_frame);

		Assert(loc <= UPID_MAX_SIZE);

		dxx_sendto (UDP_Socket[0], object_buffer, loc, 0, (struct sockaddr *)&UDP_sync_player.player.protocol.udp.addr, sizeof(struct _sockaddr));
	}

	if (i > Highest_object_index)
	{
		if (Network_send_object_mode == 0)
		{
			Network_send_objnum = 0;
			Network_send_object_mode = 1; // go to next mode
		}
		else 
		{
			Assert(Network_send_object_mode == 1); 

			// Send count so other side can make sure he got them all
			object_buffer[0] = UPID_OBJECT_DATA;
			PUT_INTEL_INT(object_buffer+5, 1);
			PUT_INTEL_INT(object_buffer+9, -2);
			object_buffer[13] = player_num;
			PUT_INTEL_INT(object_buffer+14, obj_count);
			dxx_sendto (UDP_Socket[0], object_buffer, 18, 0, (struct sockaddr *)&UDP_sync_player.player.protocol.udp.addr, sizeof(struct _sockaddr));

			// Send sync packet which tells the player who he is and to start!
			net_udp_send_rejoin_sync(player_num);

			// Turn off send object mode
			Network_send_objnum = -1;
			Network_send_objects = 0;
			obj_count = 0;

			Network_sending_extras=3; // start to send extras
			VerifyPlayerJoined = Player_joining_extras = player_num;

			if(UDP_sync_player.player.observer) {
				VerifyPlayerJoined = -1; 
			}

			return;
		} // mode == 1;
	} // i > Highest_object_index
}

int net_udp_verify_objects(int remote, int local)
{
	int i, nplayers = 0;

	if ((remote-local) > 10)
		return(2);

	for (i = 0; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST))
			nplayers++;
	}

	if (Netgame.max_numplayers<=nplayers)
		return(0);

	return(1);
}

void net_udp_read_object_packet( ubyte *data )
{
	multi_received_objects = 1; 

	// Object from another net player we need to sync with
	object *obj;
	sbyte obj_owner;
	static int mode = 0, object_count = 0, my_pnum = 0;
	int i = 0, segnum = 0, objnum = 0, remote_objnum = 0, nobj = 0, loc = 9;
	
	nobj = GET_INTEL_INT(data + 5);

	for (i = 0; i < nobj; i++)
	{
		objnum = GET_INTEL_INT(data + loc);                         loc += 4;
		obj_owner = data[loc];                                      loc += 1;
		remote_objnum = GET_INTEL_INT(data + loc);                  loc += 4;

		if (objnum == -1) 
		{
			// Clear object array
			init_objects();
			Network_rejoined = 1;
			my_pnum = obj_owner;
			if(!is_observer()) { change_playernum_to(my_pnum); }
			mode = 1;
			object_count = 0;
		}
		else if (objnum == -2)
		{
			// Special debug checksum marker for entire send
			if (mode == 1)
			{
				special_reset_objects();
				mode = 0;
			}
			if (remote_objnum != object_count) {
				Int3();
			}
			if (net_udp_verify_objects(remote_objnum, object_count))
			{
				// Failed to sync up 
				nm_messagebox(NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
				Network_status = NETSTAT_MENU;                          
				return;
			}
		}
		else 
		{
			object_count++;
			if ((obj_owner == my_pnum) || (obj_owner == -1)) 
			{
				if (mode != 1)
					Int3(); // SEE ROB
				objnum = remote_objnum;
			}
			else {
				if (mode == 1)
				{
					special_reset_objects();
					mode = 0;
				}
				objnum = obj_allocate();
			}
			if (objnum != -1) {
				obj = &Objects[objnum];
				if (obj->segnum != -1)
					obj_unlink(objnum);
				Assert(obj->segnum == -1);
				Assert(objnum < MAX_OBJECTS);
#ifdef WORDS_BIGENDIAN
				object_rw_swap((object_rw *)&data[loc], 1);
#endif
				multi_object_rw_to_object((object_rw *)&data[loc], obj);
				loc += sizeof(object_rw);
				segnum = obj->segnum;
				obj->next = obj->prev = obj->segnum = -1;
				obj->attached_obj = -1;
				if (segnum > -1)
					obj_link(obj-Objects,segnum);
				if (obj_owner == my_pnum) 
					map_objnum_local_to_local(objnum);
				else if (obj_owner != -1)
					map_objnum_local_to_remote(objnum, remote_objnum, obj_owner);
				else
					object_owner[objnum] = -1;
			}
		} // For a standard onbject
	} // For each object in packet
}

// Finished sending objects
void net_udp_send_rejoin_sync(int player_num)
{
	int i, j;

	if (Netgame.max_numobservers > 0 && player_num == OBSERVER_PLAYER_ID)
	{
		Network_player_added = 0;
		Netgame.observers[UDP_sync_obsnum].connected = 1;
		multi_send_obs_update(0, UDP_sync_obsnum);
	}
	else
	{
		Players[player_num].connected = CONNECT_PLAYING; // connect the new guy
		Netgame.players[player_num].LastPacketTime = timer_query();
	}

	if (Endlevel_sequence || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		net_log_comment("rejoin dumped due to end level sequence");
		net_udp_dump_player(UDP_sync_player.player.protocol.udp.addr, UDP_sync_player.token, DUMP_ENDLEVEL);

		Network_send_objects = 0; 
		Network_sending_extras=0;
		return;
	}

	if (Network_player_added)
	{
		UDP_sync_player.type = UPID_ADDPLAYER;
		UDP_sync_player.player.connected = player_num;
		net_udp_new_player(&UDP_sync_player);

		for (i = 0; i < N_players; i++)
		{
			if ((i != player_num) && (i != Player_num) && (Players[i].connected))
				net_udp_send_sequence_packet( UDP_sync_player, Netgame.players[i].protocol.udp.addr);
		}
	}

	// Send sync packet to the new guy

	net_udp_update_netgame();

	// Fill in the kill list
	for (j=0; j<MAX_PLAYERS; j++)
	{
		for (i=0; i<MAX_PLAYERS;i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.player.protocol.udp.addr, UPID_SYNC, 1, UDP_sync_player.token);
	net_udp_send_door_updates();

	return;
}

void net_udp_resend_sync_due_to_packet_loss()
{
	int i,j;

	if (!multi_i_am_master())
		return;

	net_udp_update_netgame();

	// Fill in the kill list
	for (j=0; j<MAX_PLAYERS; j++)
	{
		for (i=0; i<MAX_PLAYERS;i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.player.protocol.udp.addr, UPID_SYNC, 0, UDP_sync_player.token);
}

char * net_udp_get_player_name( int objnum )
{
	if ( objnum < 0 ) return NULL; 
	if ( Objects[objnum].type != OBJ_PLAYER ) return NULL;
	if ( Objects[objnum].id >= MAX_PLAYERS ) return NULL;
	if ( Objects[objnum].id >= N_players ) return NULL;
	
	return Players[Objects[objnum].id].callsign;
}


void net_udp_add_player(UDP_sequence_packet *p)
{
	int i;

	if (p->player.observer) {
		return;
	}

	for (i=0; i<N_players; i++ )
	{
		if ( !memcmp( (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, (struct _sockaddr *)&p->player.protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			Netgame.players[i].LastPacketTime = timer_query();
			if(Netgame.RetroProtocol && (! multi_i_am_master()) && (! multi_who_is_master() == i)) {
				//memcpy(&Netgame.players[i].protocol.udp.addr, &p->player.protocol.udp.addr, sizeof(struct _sockaddr)); 
				update_address_for_player(i, p->player.protocol.udp.addr);
				player_tokens[i] = p->token;

				resetProxy(i); 
				reattemptDirect(i);
				if(Netgame.AllowPreferredColors) { 
					Netgame.players[i].color = p->player.color;
					if(Netgame.players[i].color > 7) { Netgame.players[i].color = i; }

					Netgame.players[i].missilecolor = p->player.missilecolor; 		
					if(p->player.missilecolor > 7) { Netgame.players[i].missilecolor = Netgame.players[i].color; }
				
					resolve_color_conflicts(i);
				} else { 
					Netgame.players[i].color = i; 
					Netgame.players[i].missilecolor = i; 
				}
				
			}
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )
	{
		return;		// too many of em
	}

	ClipRank (&p->player.rank);
	memcpy( Netgame.players[N_players].callsign, p->player.callsign, CALLSIGN_LEN+1 );
	if(Netgame.AllowPreferredColors) {
		Netgame.players[N_players].color = p->player.color;
		if(Netgame.players[N_players].color > 7) { Netgame.players[N_players].color = N_players; }

		Netgame.players[N_players].missilecolor = p->player.missilecolor;
		if(Netgame.players[N_players].missilecolor > 7) { Netgame.players[N_players].missilecolor = Netgame.players[N_players].color; }

		resolve_color_conflicts(N_players);
	} else {
		Netgame.players[N_players].color = N_players;
		Netgame.players[N_players].missilecolor = N_players;
	}

	//memcpy( (struct _sockaddr *)&Netgame.players[N_players].protocol.udp.addr, (struct _sockaddr *)&p->player.protocol.udp.addr, sizeof(struct _sockaddr) );
	update_address_for_player(N_players, p->player.protocol.udp.addr);
	Netgame.players[N_players].rank=p->player.rank;
	Netgame.players[N_players].connected = CONNECT_PLAYING;
	Players[N_players].KillGoalCount=0;
	Players[N_players].connected = CONNECT_PLAYING;
	Netgame.players[N_players].LastPacketTime = timer_query();
	player_tokens[N_players] = p->token;

	N_players++;
	Netgame.numplayers = N_players;



	if(Netgame.RetroProtocol && (! multi_i_am_master()) && (! multi_who_is_master() == N_players)) {
		//memcpy(&Netgame.players[i].protocol.udp.addr, &p->player.protocol.udp.addr, sizeof(struct _sockaddr)); 
		update_address_for_player(i, p->player.protocol.udp.addr);
		resetProxy(i);
		reattemptDirect(i);
	}

	net_udp_send_netgame_update();
}

// One of the players decided not to join the game

void net_udp_remove_player(UDP_sequence_packet *p)
{
	int i,pn;
	
	pn = -1;
	for (i=0; i<N_players; i++ )
	{
		if (!memcmp((struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, (struct _sockaddr *)&p->player.protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			pn = i;
			break;
		}
	}
	
	if (pn < 0 )
		return;

	for (i=pn; i<N_players-1; i++ )
	{
		memcpy( Netgame.players[i].callsign, Netgame.players[i+1].callsign, CALLSIGN_LEN+1 );
		//memcpy( (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i+1].protocol.udp.addr, sizeof(struct _sockaddr) );
		update_address_for_player(i, Netgame.players[i+1].protocol.udp.addr);
		Netgame.players[i].rank=Netgame.players[i+1].rank;
		Netgame.players[i].color=Netgame.players[i+1].color;
		Netgame.players[i].missilecolor=Netgame.players[i+1].missilecolor;
		ClipRank (&Netgame.players[i].rank);
	}
		
	N_players--;
	Netgame.numplayers = N_players;

	net_udp_send_netgame_update();
}

void net_udp_dump_player(struct _sockaddr dump_addr, int their_token, int why)
{
	// Inform player that he was not chosen for the netgame

	ubyte buf[UPID_DUMP_SIZE];
	int i;
	
	if(their_token == 0) { 
		net_log_comment("Sending dump using netgame token"); 
		their_token = netgame_token; 
	} else {
		net_log_comment("Sending dump using player token"); 
	}

	buf[0] = UPID_DUMP;
	PUT_INTEL_INT(buf + 1, their_token); 
	buf[5] = why;
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&dump_addr, sizeof(struct _sockaddr));

	if (multi_i_am_master())
		for (i = 1; i < N_players; i++)
			if (!memcmp((struct _sockaddr *)&dump_addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
				multi_disconnect_player(i);
}

void net_udp_update_netgame(void)
{
	// Update the netgame struct with current game variables

	int i, j;

	Netgame.numconnected=0;
	for (i=0;i<N_players;i++)
		if (Players[i].connected)
			Netgame.numconnected++;
	
	if (Network_status == NETSTAT_STARTING)
		return;

	Netgame.numplayers = N_players;
	Netgame.game_status = Network_status;

	for (i = 0; i < MAX_PLAYERS; i++) 
	{
		Netgame.players[i].connected = Players[i].connected;
		for(j = 0; j < MAX_PLAYERS; j++)
			Netgame.kills[i][j] = kill_matrix[i][j];
		Netgame.killed[i] = Players[i].net_killed_total;
		Netgame.player_kills[i] = Players[i].net_kills_total;
	}

	Netgame.team_kills[0] = team_kills[0];
	Netgame.team_kills[1] = team_kills[1];
	Netgame.levelnum = Current_level_num;
}

/* Send an updated endlevel status to everyone (if we are host) or host (if we are client)  */
void net_udp_send_endlevel_packet(void)
{
	int i = 0, j = 0, len = 0;

	if (is_observer()) {
		// Don't send this packet as observer.
		return;
	}

	if (multi_i_am_master())
	{
		ubyte buf[UPID_MAX_SIZE];

		memset(buf, 0, sizeof(buf));

		buf[len] = UPID_ENDLEVEL_H;											len++;
		PUT_INTEL_INT(buf + len, netgame_token); 							len += 4; 
		buf[len] = Countdown_seconds_left;									len++;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			buf[len] = Players[i].connected;								len++;
			PUT_INTEL_SHORT(buf + len, Players[i].net_kills_total);			len += 2;
			PUT_INTEL_SHORT(buf + len, Players[i].net_killed_total);		len += 2;
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				PUT_INTEL_SHORT(buf + len, kill_matrix[i][j]);				len += 2;
			}
		}

		for (i = 1; i < MAX_PLAYERS; i++)
			if (Players[i].connected != CONNECT_DISCONNECTED)
				dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
		
		forward_to_observers(buf, len, 1);
	}
	else
	{
		ubyte buf[UPID_MAX_SIZE];

		memset(buf, 0,  sizeof(buf));

		buf[len] = UPID_ENDLEVEL_C;											len++;
		PUT_INTEL_INT(buf + len, netgame_token); 							len += 4; 
		buf[len] = Player_num;												len++;
		buf[len] = Players[Player_num].connected;							len++;
		buf[len] = Countdown_seconds_left;									len++;
		PUT_INTEL_SHORT(buf + len, Players[Player_num].net_kills_total);	len += 2;
		PUT_INTEL_SHORT(buf + len, Players[Player_num].net_killed_total);	len += 2;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_SHORT(buf + len, kill_matrix[Player_num][i]);			len += 2;
		}

		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr));
	}
}

void net_udp_send_version_deny(struct _sockaddr sender_addr)
{
	ubyte buf[UPID_VERSION_DENY_SIZE];
	
	buf[0] = UPID_VERSION_DENY;
	PUT_INTEL_SHORT(buf + 1, DXX_VERSION_MAJORi);
	PUT_INTEL_SHORT(buf + 3, DXX_VERSION_MINORi);
	PUT_INTEL_SHORT(buf + 5, DXX_VERSION_MICROi);
	PUT_INTEL_SHORT(buf + 7, MULTI_PROTO_VERSION);
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
}

void net_udp_process_version_deny(ubyte *data, struct _sockaddr sender_addr)
{
	Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&data[1]);
	Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&data[3]);
	Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&data[5]);
	Netgame.protocol.udp.program_iver[3] = GET_INTEL_SHORT(&data[7]);
	Netgame.protocol.udp.valid = -1;
}

void net_udp_request_game_info(struct _sockaddr game_addr, int lite)
{
	ubyte buf[UPID_GAME_INFO_REQ_SIZE];
	
	buf[0] = (lite?UPID_GAME_INFO_LITE_REQ:UPID_GAME_INFO_REQ);
	memcpy(&(buf[1]), UDP_REQ_ID, 4);
	PUT_INTEL_SHORT(buf + 5, DXX_VERSION_MAJORi);
	PUT_INTEL_SHORT(buf + 7, DXX_VERSION_MINORi);
	PUT_INTEL_SHORT(buf + 9, DXX_VERSION_MICROi);
	if (!lite)
		PUT_INTEL_SHORT(buf + 11, MULTI_PROTO_VERSION);
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&game_addr, sizeof(struct _sockaddr));
}

// Check request for game info. Return 1 if sucessful; -1 if version mismatch; 0 if wrong game or some other error - do not process
int net_udp_check_game_info_request(ubyte *data, int lite)
{
	short sender_iver[4] = { 0, 0, 0, 0 };
	char sender_id[4] = "";

	memcpy(&sender_id, &(data[1]), 4);
	sender_iver[0] = GET_INTEL_SHORT(&(data[5]));
	sender_iver[1] = GET_INTEL_SHORT(&(data[7]));
	sender_iver[2] = GET_INTEL_SHORT(&(data[9]));
	if (!lite)
		sender_iver[3] = GET_INTEL_SHORT(&(data[11]));
	
	if (memcmp(&sender_id, UDP_REQ_ID, 4))
		return 0;
	
	if ((sender_iver[0] != DXX_VERSION_MAJORi) || (sender_iver[1] != DXX_VERSION_MINORi) || (sender_iver[2] != DXX_VERSION_MICROi) || (!lite && sender_iver[3] != MULTI_PROTO_VERSION))
		return -1;
		
	return 1;
}

extern fix ThisLevelTime;

void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid, ubyte send_to_observers, uint player_token)
{
	//static fix64 last_full_req_time = 0;
	//if (timer_query() < last_full_req_time+(F1_0/5)) // answer 5 times per second max
	//	break;
	//last_full_req_time = timer_query();

	//static fix64 last_lite_req_time = 0;
	//if (timer_query() < last_lite_req_time+(F1_0/8))// answer 8 times per second max
	//	break;
	//last_lite_req_time = timer_query();	

	// Send game info to someone who requested it

	int len = 0;
	
	net_udp_update_netgame(); // Update the values in the netgame struct
	
	if (info_upid == UPID_GAME_INFO_LITE)
	{
		ubyte buf[UPID_GAME_INFO_LITE_SIZE];
		int tmpvar = 0;

		memset(buf, 0, sizeof(buf));
		
		buf[0] = info_upid;								len++;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MAJORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MINORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MICROi); 						len += 2;
		PUT_INTEL_INT(buf + len, Netgame.protocol.udp.GameID);				len += 4;
		memcpy(&(buf[len]), Netgame.game_name, NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_title, MISSION_NAME_LEN+1);			len += (MISSION_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_name, 9);				len += 9;
		PUT_INTEL_INT(buf + len, Netgame.levelnum);					len += 4;
		buf[len] = Netgame.gamemode;							len++;
		buf[len] = Netgame.RefusePlayers;						len++;
		buf[len] = Netgame.difficulty;							len++;
		tmpvar = Netgame.game_status;
		if (Endlevel_sequence || Control_center_destroyed)
			tmpvar = NETSTAT_ENDLEVEL;
		if (Netgame.PlayTimeAllowed)
		{
			if ( (f2i((i2f (Netgame.PlayTimeAllowed*5*60))-ThisLevelTime)) < 30 )
			{
				tmpvar = NETSTAT_ENDLEVEL;
			}
		}
		buf[len] = tmpvar;								len++;
		buf[len] = Netgame.numconnected;						len++;
		buf[len] = Netgame.max_numplayers;						len++;
		buf[len] = Netgame.game_flags;							len++;
		
		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
	}
	else
	{
		ubyte buf[UPID_GAME_INFO_SIZE];
		int i = 0, j = 0, tmpvar = 0;
		
		memset(buf, 0, sizeof(buf));

		buf[0] = info_upid;								len++;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MAJORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MINORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MICROi); 						len += 2;
		//PUT_INTEL_INT(buf + len, Netgame.protocol.udp.GameID);				len += 4;
		for (i = 0; i < MAX_PLAYERS+4; i++)
		{
			memcpy(&buf[len], Netgame.players[i].callsign, CALLSIGN_LEN+1); 	len += CALLSIGN_LEN+1;
			buf[len] = Netgame.players[i].connected;				len++;
			buf[len] = Netgame.players[i].rank;					len++;
			buf[len] = Netgame.players[i].color;				len++; 
			buf[len] = Netgame.players[i].missilecolor;				len++;
			if (!memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr))) {
				buf[len] = 1; len++; 
			} else {
				buf[len] = 0;							len++;
			}

			if(info_upid == UPID_SYNC && Netgame.RetroProtocol) {
				memcpy(&buf[len], &Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)); len += sizeof(struct _sockaddr);
			} //else {
				//memset(buf + len, 0, sizeof(struct _sockaddr)); len += sizeof(struct _sockaddr);
			//}
		}
		memcpy(&(buf[len]), Netgame.game_name, NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_title, MISSION_NAME_LEN+1);			len += (MISSION_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_name, 9);				len += 9;
		PUT_INTEL_INT(buf + len, Netgame.levelnum);					len += 4;
		buf[len] = Netgame.gamemode;							len++;
		buf[len] = Netgame.RefusePlayers;						len++;
		buf[len] = Netgame.difficulty;							len++;
		tmpvar = Netgame.game_status;
		if (Endlevel_sequence || Control_center_destroyed)
			tmpvar = NETSTAT_ENDLEVEL;
		if (Netgame.PlayTimeAllowed)
		{
			if ( (f2i((i2f (Netgame.PlayTimeAllowed*5*60))-ThisLevelTime)) < 30 )
			{
				tmpvar = NETSTAT_ENDLEVEL;
			}
		}
		buf[len] = tmpvar;								len++;
		buf[len] = Netgame.numplayers;							len++;
		buf[len] = Netgame.max_numplayers;						len++;
		buf[len] = Netgame.numconnected;						len++;
		buf[len] = Netgame.game_flags;							len++;
		buf[len] = Netgame.team_vector;							len++;
		PUT_INTEL_INT(buf + len, Netgame.AllowedItems);					len += 4;
		PUT_INTEL_SHORT(buf + len, Netgame.Allow_marker_view);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.AlwaysLighting);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.ShowEnemyNames);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.BrightPlayers);				len += 2;
		len += 2; // Spawn invul -- no longer used, but don't break tools
		memcpy(&buf[len], Netgame.team_name, 2*(CALLSIGN_LEN+1));			len += 2*(CALLSIGN_LEN+1);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_INT(buf + len, Netgame.locations[i]);				len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				PUT_INTEL_SHORT(buf + len, Netgame.kills[i][j]);		len += 2;
			}
		}
		PUT_INTEL_SHORT(buf + len, Netgame.segments_checksum);			len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.team_kills[0]);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.team_kills[1]);				len += 2;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_SHORT(buf + len, Netgame.killed[i]);				len += 2;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_SHORT(buf + len, Netgame.player_kills[i]);			len += 2;
		}
		PUT_INTEL_INT(buf + len, Netgame.KillGoal);					len += 4;
		PUT_INTEL_INT(buf + len, Netgame.PlayTimeAllowed);				len += 4;
		PUT_INTEL_INT(buf + len, Netgame.level_time);					len += 4;
		PUT_INTEL_INT(buf + len, Netgame.control_invul_time);				len += 4;
		PUT_INTEL_INT(buf + len, Netgame.monitor_vector);				len += 4;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_INT(buf + len, Netgame.player_score[i]);			len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			buf[len] = Netgame.player_flags[i];					len++;
		}
		PUT_INTEL_SHORT(buf + len, Netgame.PacketsPerSec);				len += 2;
		buf[len] = Netgame.ShortPackets;						len++;
		buf[len] = Netgame.PacketLossPrevention;					len++;
		buf[len] = Netgame.NoFriendlyFire;						len++;
		buf[len] = Netgame.RetroProtocol;						len++;
		buf[len] = Netgame.RespawnConcs;						len++;
		buf[len] = Netgame.AllowColoredLighting; 				len++; 
		buf[len] = Netgame.FairColors;			 				len++; 		
		buf[len] = Netgame.BlackAndWhitePyros; 				len++; 
		buf[len] = Netgame.SpawnStyle;		                len++; 
		buf[len] = Netgame.PrimaryDupFactor;                len++; 
		buf[len] = Netgame.SecondaryDupFactor;                len++; 
		buf[len] = Netgame.SecondaryCapFactor;                len++; 
		buf[len] = Netgame.DarkSmartBlobs;					len++; 
		buf[len] = Netgame.LowVulcan;					len++;
		buf[len] = Netgame.AllowPreferredColors;        len++; 
		buf[len] = Netgame.obs_min; len++;
		buf[len] = Netgame.host_is_obs; len++;
		buf[len] = Netgame.HomingUpdateRate; len++;
		buf[len] = Netgame.RemoteHitSpark; len++;
		buf[len] = Netgame.AllowCustomModelsTextures; len++;
		buf[len] = Netgame.ReducedFlash; len++;
		buf[len] = Netgame.GaussAmmoStyle; len++;
		buf[len] = Netgame.team_color[0];						len++;
		buf[len] = Netgame.team_color[1];						len++;
		buf[len] = Netgame.NewSpawnAlgorithm; len++;

		if(info_upid == UPID_SYNC) {
			PUT_INTEL_INT(buf + len, player_token); len += 4; 
			PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
		}

		if(info_upid == UPID_GAME_INFO) {
			PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
		}

		Assert(len <= sizeof(buf));

		if (send_to_observers != 2)
			dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));

		if (send_to_observers != 0)
			forward_to_observers(buf, len, 0);
	}
}

void net_udp_broadcast_game_info(ubyte info_upid)
{
	net_udp_send_game_info(GBcast, info_upid, 0, 0);
#ifdef IPv6
	net_udp_send_game_info(GMcast_v6, info_upid, 0, 0);
#endif
}

/* Send game info to all players in this game. Also send lite_info for people watching the netlist */
void net_udp_send_netgame_update()
{
	int i = 0;
	
	for (i=1; i<N_players; i++ )
	{
		if (Players[i].connected == CONNECT_DISCONNECTED)
			continue;
		net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO, 0, 0);
	}
	net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
}

int net_udp_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

	if (Netgame.numplayers < 1)
	 return 1;

	for (i = 0; i < MAX_PLAYERS; i++)
		if (Netgame.players[i].connected)
			break;

	Assert(i < MAX_PLAYERS);

	UDP_Seq.type = UPID_REQUEST;
	UDP_Seq.player.connected = Current_level_num;

	net_udp_send_sequence_packet(UDP_Seq, Netgame.players[0].protocol.udp.addr);

	return i;
}

int net_udp_process_game_info(ubyte *data, int data_len, struct _sockaddr game_addr, int lite_info, ubyte is_sync)
{
	int len = 0, i = 0, j = 0;
	
	if (lite_info)
	{
		UDP_netgame_info_lite recv_game;
		
		memcpy(&recv_game, &game_addr, sizeof(struct _sockaddr));
												len++; // skip UPID byte
		recv_game.program_iver[0] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		recv_game.program_iver[1] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		recv_game.program_iver[2] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		
		recv_game.GameID = GET_INTEL_INT(&(data[len]));					len += 4;
		memcpy(&recv_game.game_name, &(data[len]), NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&recv_game.mission_title, &(data[len]), MISSION_NAME_LEN+1);		len += (MISSION_NAME_LEN+1);
		memcpy(&recv_game.mission_name, &(data[len]), 9);				len += 9;
		recv_game.levelnum = GET_INTEL_INT(&(data[len]));				len += 4;
		recv_game.gamemode = data[len];							len++;
		recv_game.RefusePlayers = data[len];						len++;
		recv_game.difficulty = data[len];						len++;
		recv_game.game_status = data[len];						len++;
		recv_game.numconnected = data[len];						len++;
		recv_game.max_numplayers = data[len];						len++;
		recv_game.game_flags = data[len];						len++;
	
		num_active_udp_changed = 1;
		
		for (i = 0; i < num_active_udp_games; i++)
			if (!d_stricmp(Active_udp_games[i].game_name, recv_game.game_name) && Active_udp_games[i].GameID == recv_game.GameID)
				break;

		if (i == UDP_MAX_NETGAMES)
		{
			return 0;
		}
		
		memcpy(&Active_udp_games[i], &recv_game, sizeof(UDP_netgame_info_lite));
		
		if (i == num_active_udp_games)
			num_active_udp_games++;

		if (Active_udp_games[i].numconnected == 0)
		{
			// Delete this game
			for (j = i; j < num_active_udp_games-1; j++)
				memcpy(&Active_udp_games[j], &Active_udp_games[j+1], sizeof(UDP_netgame_info_lite));
			num_active_udp_games--;
		}
	}
	else
	{
		memcpy((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&game_addr, sizeof(struct _sockaddr));

												len++; // skip UPID byte
		Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&(data[len]));		len += 2;

		//Netgame.GameID = GET_INTEL_INT(&(data[len]));					len += 4;

		for (i = 0; i < MAX_PLAYERS+4; i++)
		{
			memcpy(&Netgame.players[i].callsign, &(data[len]), CALLSIGN_LEN+1);	len += CALLSIGN_LEN+1;
			Netgame.players[i].connected = data[len];				len++;
			Netgame.players[i].rank = data[len];					len++;
			Netgame.players[i].color = data[len];					len++;
			Netgame.players[i].missilecolor = data[len];					len++;
			Netgame.players[i].protocol.udp.isyou = data[len];			len++;

			if(is_sync && Netgame.RetroProtocol) {
				if(i != 0) { // Don't ever overwrite host addr
					//memcpy(&Netgame.players[i].protocol.udp.addr, data + len, sizeof(struct _sockaddr)); 
					struct _sockaddr new_address;
					memcpy(&new_address, data + len,  sizeof(struct _sockaddr) ); 
					update_address_for_player(i, new_address); 

					struct sockaddr_in *addrin = (struct sockaddr_in*) &Netgame.players[i].protocol.udp.addr;
					char *ip = inet_ntoa(addrin->sin_addr); 
					ushort port = SWAPSHORT(addrin->sin_port);
					con_printf(CON_DEBUG, "Received new player num (in game info) %d at ip %s port %d\n", i, ip, port); 	
				}		

				len += sizeof(struct _sockaddr);
			}
		}
		memcpy(&Netgame.game_name, &(data[len]), NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);	
		memcpy(&Netgame.mission_title, &(data[len]), MISSION_NAME_LEN+1);		len += (MISSION_NAME_LEN+1);
		memcpy(&Netgame.mission_name, &(data[len]), 9);					len += 9;
		Netgame.levelnum = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.gamemode = data[len];							len++;
		Netgame.RefusePlayers = data[len];						len++;
		Netgame.difficulty = data[len];							len++;
		Netgame.game_status = data[len];						len++;
		Netgame.numplayers = data[len];							len++;
		Netgame.max_numplayers = data[len];						len++;
		Netgame.numconnected = data[len];						len++;
		Netgame.game_flags = data[len];							len++;
		Netgame.team_vector = data[len];						len++;
		Netgame.AllowedItems = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.Allow_marker_view = GET_INTEL_SHORT(&(data[len]));			len += 2;
		Netgame.AlwaysLighting = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.ShowEnemyNames = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.BrightPlayers = GET_INTEL_SHORT(&(data[len]));				len += 2;
		len += 2; // Spawn invul -- no longer used, but don't break tools
		memcpy(Netgame.team_name, &(data[len]), 2*(CALLSIGN_LEN+1));			len += 2*(CALLSIGN_LEN+1);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.locations[i] = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				Netgame.kills[i][j] = GET_INTEL_SHORT(&(data[len]));		len += 2;
			}
		}
		Netgame.segments_checksum = GET_INTEL_SHORT(&(data[len]));			len += 2;
		Netgame.team_kills[0] = GET_INTEL_SHORT(&(data[len]));				len += 2;	
		Netgame.team_kills[1] = GET_INTEL_SHORT(&(data[len]));				len += 2;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.killed[i] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.player_kills[i] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		Netgame.KillGoal = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.PlayTimeAllowed = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.level_time = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.control_invul_time = GET_INTEL_INT(&(data[len]));			len += 4;
		Netgame.monitor_vector = GET_INTEL_INT(&(data[len]));				len += 4;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.player_score[i] = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.player_flags[i] = data[len];					len++;
		}
		Netgame.PacketsPerSec = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.ShortPackets = data[len];						len++;
		Netgame.PacketLossPrevention = data[len];					len++;
		Netgame.NoFriendlyFire = data[len];						len++;
		Netgame.RetroProtocol = data[len];						len++; 
		Netgame.RespawnConcs  = data[len];						len++; 
		Netgame.AllowColoredLighting = data[len];				len++; 
		Netgame.FairColors = data[len];							len++; 
		Netgame.BlackAndWhitePyros = data[len];				len++; 
		Netgame.SpawnStyle = data[len];				len++; 
		Netgame.PrimaryDupFactor = data[len];                len++; 
		Netgame.SecondaryDupFactor = data[len];                len++; 
		Netgame.SecondaryCapFactor = data[len];                len++; 
		Netgame.DarkSmartBlobs = data[len];                    len++; 
		Netgame.LowVulcan = data[len];                    len++; 
		Netgame.AllowPreferredColors = data[len];         len++; 
		Netgame.obs_min = data[len]; len++;
		Netgame.host_is_obs = data[len]; len++;
		Netgame.HomingUpdateRate = data[len]; len++;
		Netgame.RemoteHitSpark = data[len]; len++;
		Netgame.AllowCustomModelsTextures = data[len]; len++;
		Netgame.ReducedFlash = data[len]; len++;
		Netgame.GaussAmmoStyle = data[len]; len++;
		Netgame.team_color[0] = data[len];						len++;
		Netgame.team_color[1] = data[len];						len++;
		Netgame.NewSpawnAlgorithm = data[len]; len++;

		if (Netgame.host_is_obs) {
			multi_make_player_ghost(0);
		}

		if(is_sync && ! multi_i_am_master()) {
			uint my_token = GET_INTEL_INT(data + len);  len += 4;
			if(my_token == my_player_token || is_observer()) {	
				netgame_token = GET_INTEL_INT(data + len); len += 4;
				con_printf(CON_DEBUG, "Set token %d in net_udp_process_game_info\n", netgame_token); 
			} else {
				char err_mess[200];
				snprintf(err_mess, 200, "player token incorrect; received %u, expected %u",  my_token, my_player_token);
				drop_rx_packet(data, err_mess); 
				return 0; 
			}
		}

		if (data[0] == UPID_GAME_INFO) {
			netgame_token = my_player_token = GET_INTEL_INT(data + len); len += 4;
			con_printf(CON_DEBUG, "Set token %d for UPID_GAME_INFO in net_udp_process_game_info\n", netgame_token); 
		}

		if (len > data_len) {
			char err_mess[200];
			snprintf(err_mess, sizeof(err_mess), "game info size incorrect; received %d, expected %d",  data_len, len);
			drop_rx_packet(data, err_mess);
			return 0;
		}

		Netgame.protocol.udp.valid = 1; // This game is valid! YAY!
	}

	return 1; 
}

void net_udp_process_dump(ubyte *data, int len, struct _sockaddr sender_addr)
{
	// Our request for join was denied.  Tell the user why.

	switch (data[5])
	{
		case DUMP_PKTTIMEOUT:
		case DUMP_KICKED:
			if (Network_status==NETSTAT_PLAYING)
				multi_leave_game();
			if (Game_wind)
				window_set_visible(Game_wind, 0);
			if (data[5] == DUMP_PKTTIMEOUT)
				nm_messagebox(NULL, 1, TXT_OK, "You were removed from the game.\nYou failed receiving important\npackets. Sorry.");
			if (data[5] == DUMP_KICKED)
				nm_messagebox(NULL, 1, TXT_OK, "You were kicked by Host!");
			if (Game_wind)
				window_set_visible(Game_wind, 1);
			multi_quit_game = 1;
			game_leave_menus();
			multi_reset_stuff();
			break;
		default:
			if (data[5] > DUMP_LEVEL) // invalid dump... heh
				break;
			Network_status = NETSTAT_MENU; // stop us from sending before message
			nm_messagebox(NULL, 1, TXT_OK, NET_DUMP_STRINGS(data[5]));
			Network_status = NETSTAT_MENU;
			multi_reset_stuff();
			break;
	}
}

void net_udp_process_request(UDP_sequence_packet *their)
{
	if(their->player.observer) { return; }

	// Player is ready to receieve a sync packet
	int i;

	for (i = 0; i < N_players; i++)
		if ( is_player_ip(their->player.protocol.udp.addr, i) && 
			(!d_stricmp(their->player.callsign, Netgame.players[i].callsign)))
		{
			Players[i].connected = CONNECT_PLAYING;
			Netgame.players[i].LastPacketTime = timer_query();

			break;
		}
}



void net_udp_process_packet(ubyte *data, struct _sockaddr sender_addr, int length, int is_proxy )
{
	UDP_sequence_packet their;
	memset(&their, 0, sizeof(UDP_sequence_packet));

	if(! pass_security_check(data, sender_addr, length, ! is_proxy)) {
		con_printf(CON_URGENT, "Dropped pid %s: failed security checks.\n", msg_name(data[0])); 
		return;
	}

	if (multi_i_am_master() && !is_any_player_ip(sender_addr) && is_observer_ip(sender_addr)) {
		switch (data[0]) {
			case UPID_P2P_PING:
			case UPID_REQUEST:
			case UPID_MDATA_ACK:
			case UPID_MDATA_PNEEDACK:
            case UPID_OBSDATA:
            case UPID_OBSQUIT:
				break;
			default:
				con_printf(CON_URGENT, "Dropped pid %s: observer sent disallowed packet.\n", msg_name(data[0])); 
				return;
		}
	}

    if (multi_i_am_master()) {
        switch (data[0])
        {
            case UPID_PDATA:
            case UPID_MDATA_PNORM:
                forward_to_observers(data, length, 0); 
                break; 

            case UPID_OBSDATA:
                forward_to_observers_nodelay(data, length, 0); 
                break; 

            case UPID_OBSQUIT:
                net_udp_process_obs_quit(data, length, sender_addr);
                break;

            case UPID_MDATA_PNEEDACK:
                forward_to_observers(data, length, 1); 
                break; 

            default: 
                break;			
        }
    }

	int result;
	switch (data[0])
	{
		case UPID_VERSION_DENY:
			net_udp_process_version_deny(data, sender_addr);
			break;

		case UPID_GAME_INFO_REQ:		
			result = net_udp_check_game_info_request(data, 0);
			if (result == -1)
				net_udp_send_version_deny(sender_addr);
			else if (result == 1)
				net_udp_send_game_info(sender_addr, UPID_GAME_INFO, 0, 0);
			break;
		
		case UPID_GAME_INFO:
			net_udp_process_game_info(data, length, sender_addr, 0, 0);
			break;

		case UPID_GAME_INFO_LITE_REQ:		
			if (net_udp_check_game_info_request(data, 1) == 1)
				net_udp_send_game_info(sender_addr, UPID_GAME_INFO_LITE, 0, 0);
			break;
		
		case UPID_GAME_INFO_LITE:
			net_udp_process_game_info(data, length, sender_addr, 1, 0);
			break;

		case UPID_DUMP:
			net_udp_process_dump(data, length, sender_addr);
			break;

		case UPID_ADDPLAYER:
			net_udp_receive_sequence_packet(data, &their, sender_addr);
			net_udp_new_player(&their);
			break;
		
		case UPID_REQUEST:
			net_udp_receive_sequence_packet(data, &their, sender_addr);
			if (Network_status == NETSTAT_STARTING) 
			{
				// Someone wants to join our game!
				net_udp_add_player(&their);
			}
			else if (Network_status == NETSTAT_WAITING)
			{
				// Someone is ready to recieve a sync packet
				net_udp_process_request(&their);
			}
			else if (Network_status == NETSTAT_PLAYING)
			{
				// Someone wants to join a game in progress!
				if (Netgame.RefusePlayers)
					net_udp_do_refuse_stuff (&their);
				else
					net_udp_welcome_player(&their);
			}
			break;

		case UPID_QUIT_JOINING:
			net_udp_receive_sequence_packet(data, &their, sender_addr);
			if (Network_status == NETSTAT_STARTING)
				net_udp_remove_player( &their );
			else if ((Network_status == NETSTAT_PLAYING) && (Network_send_objects))
				net_udp_stop_resync( &their );
			break;

		case UPID_SYNC:
			net_udp_read_sync_packet(data, length, sender_addr);
			break;

		case UPID_OBJECT_DATA:
			net_udp_read_object_packet(data);
			break;

		case UPID_PING:
			net_udp_process_ping(data, length, sender_addr);
			break;

		case UPID_PONG:
			net_udp_process_pong(data, length, sender_addr);
			break;

		case UPID_ENDLEVEL_H:
			net_udp_read_endlevel_packet( data, length, sender_addr );
			break;

		case UPID_ENDLEVEL_C:
			net_udp_read_endlevel_packet( data, length, sender_addr );
			break;

		case UPID_PDATA:
			net_udp_process_pdata( data, length, sender_addr );
			break;

		case UPID_MDATA_PNORM:
			net_udp_process_mdata( data, length, sender_addr, 0 );
			break;

        case UPID_OBSDATA:
            net_udp_process_obs_data( data, length, sender_addr );
            break;

		case UPID_MDATA_PNEEDACK:
			net_udp_process_mdata( data, length, sender_addr, 1 );
			break;

		case UPID_MDATA_ACK:
			net_udp_noloss_got_ack(data, length, sender_addr);
			break;

#ifdef USE_TRACKER
		case UPID_TRACKER_VERIFY:
			iTrackerVerified = 1;
			break;

		case UPID_TRACKER_INCGAME:
			udp_tracker_process_game( data, length );
			break;
#endif

		case UPID_P2P_PING:
			net_udp_process_p2p_ping( data, sender_addr, length);
			break;

		case UPID_P2P_PONG:
			net_udp_process_p2p_pong( data, sender_addr, length);
			break;			

		case UPID_PROXY:
			net_udp_process_proxy( data, sender_addr, length);
			break;

		case UPID_REATTEMPT_DIRECT:
			net_udp_process_p2p_reattempt_direct( data, sender_addr, length);
			break; 

		default:
			con_printf(CON_DEBUG, "unknown packet type received - type %i\n", data[0]);
			break;
	}
}

// Packet for end of level syncing
void net_udp_read_endlevel_packet( ubyte *data, int data_len, struct _sockaddr sender_addr )
{
	int len = 0, i = 0, j = 0;
	ubyte tmpvar = 0;
	
	if (multi_i_am_master())
	{
		ubyte pnum = data[5];
		if(pnum < 1 || pnum > MAX_PLAYERS || pnum == multi_who_is_master()) {
			drop_rx_packet(data, "invalid player number"); 
			return; 
		}

		if (! is_player_ip(sender_addr, pnum)) {
			drop_rx_packet(data, "received from incorrect player ip"); 			
			return;
		}

		len += 6;

		if ((int)data[len] == CONNECT_DISCONNECTED)
			multi_disconnect_player(pnum);

		if (Current_obs_player == pnum) {
			reset_obs();
		}

		Players[pnum].connected = data[len];					len++;
		tmpvar = data[len];							len++;
		if ((Network_status != NETSTAT_PLAYING) && (Players[pnum].connected == CONNECT_PLAYING) && (tmpvar < Countdown_seconds_left))
			Countdown_seconds_left = tmpvar;
		Players[pnum].net_kills_total = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Players[pnum].net_killed_total = GET_INTEL_SHORT(&(data[len]));		len += 2;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			kill_matrix[pnum][i] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		if (Players[pnum].connected)
			Netgame.players[pnum].LastPacketTime = timer_query();
	}
	else
	{

		len++;
		len += 4; 

		tmpvar = data[len];							len++;
		if (is_observer() || (Network_status != NETSTAT_PLAYING) && (tmpvar < Countdown_seconds_left))
			Countdown_seconds_left = tmpvar;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (i == Player_num)
			{
				len += 5;
				continue;
			}

			if ((int)data[len] == CONNECT_DISCONNECTED)
				multi_disconnect_player(i);

			Players[i].connected = data[len];				len++;
			Players[i].net_kills_total = GET_INTEL_SHORT(&(data[len]));	len += 2;
			Players[i].net_killed_total = GET_INTEL_SHORT(&(data[len]));	len += 2;

			if (Players[i].connected)
				Netgame.players[i].LastPacketTime = timer_query();
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				if (i != Player_num)
				{
					kill_matrix[i][j] = GET_INTEL_SHORT(&(data[len]));
				}
											len += 2;
			}
		}
	}
}

/*
 * Polling loop waiting for sync packet to start game after having sent request
 */



void net_udp_reset_connection_statuses() {
	for(int i = 0; i < MAX_PLAYERS; i++) {
		connection_statuses[i] = CONNECTION_NONE;
		connection_statuses[i].holepunch_attempts = 0;
		connection_statuses[i].proxy_through = 0;
		connection_statuses[i].last_direct_pong = 0;  

		count_pdata_received[i] = 0;
		last_pdata_received_at[0] = 0; 
		for(int j = 0; j < MAX_LOSS_BUFFER; j++) {
			pdata_received[i][j] = 0;
		}

		for(int j = 0; j < MAX_PLAYERS; j++) {
			last_direct_attempt[i][j] = 0; 
		}
	}

	netgame_token = my_player_token = generate_token(); 
	con_printf(CON_NET_STATUS, "\033[32mSLikeNet Generated Token #%d\033[0m\n", netgame_token); 
}

void
net_udp_set_game_mode(int gamemode, ubyte join_as_obs)
{
	Show_kill_list = 1;

	if ( gamemode == NETGAME_ANARCHY )
		Game_mode = GM_NETWORK;
	else if ( gamemode == NETGAME_ROBOT_ANARCHY )
		Game_mode = GM_NETWORK | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_COOPERATIVE ) 
		Game_mode = GM_NETWORK | GM_MULTI_COOP | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_TEAM_ANARCHY )
	{
		Game_mode = GM_NETWORK | GM_TEAM;
		Show_kill_list = 3;
	}
	else if( gamemode == NETGAME_BOUNTY )
		Game_mode = GM_NETWORK | GM_BOUNTY;
	else
		Int3();

	if(join_as_obs) {
		Game_mode |= GM_OBSERVER;
		change_playernum_to(OBSERVER_PLAYER_ID);
	}
}

void net_udp_read_sync_packet( ubyte * data, int data_len, struct _sockaddr sender_addr )
{

	int i, j;

	char temp_callsign[CALLSIGN_LEN+1];

	int prev_status = Network_status;
	
	// This function is now called by all people entering the netgame.

	if (data)
	{
		int packet_valid = net_udp_process_game_info(data, data_len, sender_addr, 0, 1);
		if(! packet_valid ) {
			con_printf(CON_URGENT, "Dropped invalid sync packet.\n");
			return; 
		}
	}

	N_players = Netgame.numplayers;
	Difficulty_level = Netgame.difficulty;
	Network_status = Netgame.game_status;

	// New code, 11/27

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = NETSTAT_MENU;
		if (prev_status != NETSTAT_MENU)
			nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
#ifdef NDEBUG
		return;
#endif
	}

	// Discover my player number

	memcpy(temp_callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	
	Player_num = (Game_mode & GM_OBSERVER) ? OBSERVER_PLAYER_ID : -1;

	for (i=0; i<MAX_PLAYERS; i++ )
	{
		Players[i].net_kills_total = 0;
		Players[i].net_killed_total = 0;
	}


	for (i=0; i<N_players; i++ )
	{
		if ( Netgame.players[i].protocol.udp.isyou == 1 && (!d_stricmp( Netgame.players[i].callsign, temp_callsign)) )
		{
			if(!is_observer()) {
				Assert(Player_num == -1); // Make sure we don't find ourselves twice!  Looking for interplay reported bug
				change_playernum_to(i);
			}
		}
		memcpy( Players[i].callsign, Netgame.players[i].callsign, CALLSIGN_LEN+1 );

		Players[i].connected = Netgame.players[i].connected;

		if (Current_obs_player == i && Players[i].connected != CONNECT_PLAYING) {
			reset_obs();
		}

		Players[i].net_kills_total = Netgame.player_kills[i];
		Players[i].net_killed_total = Netgame.killed[i];
		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = Netgame.player_score[i];
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			kill_matrix[i][j] = Netgame.kills[i][j];
		}

		if(! Netgame.players[i].protocol.udp.isyou) {
			if(multi_i_am_master()) {
				connection_statuses[i].type = CONNT_DIRECT; 			
			} else if (i == multi_who_is_master()) {
				connection_statuses[i].type = CONNT_DIRECT; 							
			} else {
				connection_statuses[i].type = CONNT_PROXY;
				connection_statuses[i].proxy_through = 0; 	
				connection_statuses[i].holepunch_attempts = 0;		
				connection_statuses[i].last_direct_pong = 0; 			
			}
		}
	}

	if ( Player_num < 0 && !is_observer()) {
		Network_status = NETSTAT_MENU;
		return;
	}

	if (Network_rejoined)
		for (i=0; i<N_players;i++)
			Players[i].net_killed_total = Netgame.killed[i];

	PlayerCfg.NetlifeKills -= Players[Player_num].net_kills_total;
	PlayerCfg.NetlifeKilled -= Players[Player_num].net_killed_total;

	if (Network_rejoined)
	{
		net_udp_process_monitor_vector(Netgame.monitor_vector);
		if (Player_num != -1)
			Players[Player_num].time_level = Netgame.level_time;
	}

	team_kills[0] = Netgame.team_kills[0];
	team_kills[1] = Netgame.team_kills[1];
	
	if(!is_observer()) {
		Players[Player_num].connected = CONNECT_PLAYING;
		Netgame.players[Player_num].connected = CONNECT_PLAYING;
		Netgame.players[Player_num].rank=GetMyNetRanking();


		if (!Network_rejoined)
		{
			for (i=0; i<NumNetPlayerPositions; i++)
			{
				Objects[Players[i].objnum].pos = Player_init[Netgame.locations[i]].pos;
				Objects[Players[i].objnum].orient = Player_init[Netgame.locations[i]].orient;
				obj_relink(Players[i].objnum,Player_init[Netgame.locations[i]].segnum);
			}
		}

		Objects[Players[Player_num].objnum].type = OBJ_PLAYER;
	} else {
		if (Host_is_obs) {
			Player_num = 0;
		} else {
			Player_num = OBSERVER_PLAYER_ID; // Kluge to prevent crashes
		}
	}

	Network_status = NETSTAT_PLAYING;
	multi_sort_kill_list();
}

int net_udp_send_sync(void)
{
	int i, j, np;

	// Check if there are enough starting positions
	if (NumNetPlayerPositions < Netgame.max_numplayers)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Not enough start positions\n(set %d got %d)\nNetgame aborted", Netgame.max_numplayers, NumNetPlayerPositions);
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (i=1; i<N_players; i++)
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			net_udp_dump_player(Netgame.players[i].protocol.udp.addr, player_tokens[i], DUMP_ABORTED);
			net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO, 0, 0);
		}
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
		return -1;
	}

	// Randomize their starting locations...
	d_srand( (fix)timer_query() );
	for (i=0; i<NumNetPlayerPositions; i++ )        
	{
		if (Players[i].connected)
			Players[i].connected = CONNECT_PLAYING; // Get rid of endlevel connect statuses

		if (Game_mode & GM_MULTI_COOP)
			Netgame.locations[i] = i;
		else {
			do 
			{
				np = d_rand() % NumNetPlayerPositions;
				for (j=0; j<i; j++ )    
				{
					if (Netgame.locations[j]==np)   
					{
						np =-1;
						break;
					}
				}
			} while (np<0);
			// np is a location that is not used anywhere else..
			Netgame.locations[i]=np;
		}
	}

	// Push current data into the sync packet

	net_udp_update_netgame();
	Netgame.game_status = NETSTAT_PLAYING;
	Netgame.segments_checksum = my_segments_checksum;

	if (multi_i_am_master())
		net_udp_send_game_info(Netgame.players[0].protocol.udp.addr, UPID_SYNC, 2, player_tokens[0]);

	for (i=0; i<N_players; i++ )
	{
		if ((!Players[i].connected) || (i == Player_num))
			continue;

		net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_SYNC, 0, player_tokens[i]);
		connection_statuses[i].type = CONNT_DIRECT; 
	}

	net_udp_read_sync_packet(NULL, 0, Netgame.players[0].protocol.udp.addr); // Read it myself, as if I had sent it

	return 0;
}


int net_udp_start_game(void)
{
	int i;

	i = udp_open_socket(0, atoi(UDP_MyPort));

	if (i != 0)
		return 0;
	
	if (atoi(UDP_MyPort) != UDP_PORT_DEFAULT)
		i = udp_open_socket(1, UDP_PORT_DEFAULT); // Default port open for Broadcasts

	if (i != 0)
		return 0;

	// prepare broadcast address to announce our game
	memset(&GBcast, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_BCAST_ADDR, UDP_PORT_DEFAULT, &GBcast);
#ifdef IPv6
	memset(&GMcast_v6, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_MCASTv6_ADDR, UDP_PORT_DEFAULT, &GMcast_v6);
#endif
	d_srand( (fix)timer_query() );
	Netgame.protocol.udp.GameID=d_rand();


	N_players = 0;

	Endlevel_sequence = Control_center_destroyed = 0; 
    Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	Netgame.numobservers = 0; 
	net_udp_set_game_mode(Netgame.gamemode, 0);
	Netgame.players[0].protocol.udp.isyou = 1; // I am Host. I need to know that y'know? For syncing later.

	Network_status = NETSTAT_STARTING;

	netgame_token = generate_token(); 
	con_printf(CON_DEBUG, "\033[32mSLikeNet Generated Token %d in net_udp_start_game \033[0m\n", netgame_token); 

	if(net_udp_select_players())
	{
		StartNewLevel(Netgame.levelnum);
	}
	else
	{
		Game_mode = GM_GAME_OVER;
		return 0;	// see if we want to tweak the game we setup
	}
	net_udp_broadcast_game_info(UPID_GAME_INFO_LITE); // game started. broadcast our current status to everyone who wants to know

	return 1;	// don't keep params menu or mission listbox (may want to join a game next time)
}


void net_udp_leave_game()
{
	int nsave, i;

	net_udp_do_frame(1, 1);

	if ((multi_i_am_master()))
	{
		while (Network_sending_extras>1 && Player_joining_extras!=-1)
		{
			timer_update();
			net_udp_send_extras();
		}

		Netgame.numplayers = 0;
		nsave=N_players;
		N_players=0;
		for (i=1; i<nsave; i++ )
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO, 0, 0);
		}
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
		N_players=nsave;
#ifdef USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif
	}

	Players[Player_num].connected = CONNECT_DISCONNECTED;

	if (Current_obs_player == Player_num) {
		reset_obs();
	}

	change_playernum_to(0);
	net_udp_flush();
	net_udp_close();
}

void net_udp_flush()
{
	ubyte packet[UPID_MAX_SIZE];
	struct _sockaddr sender_addr; 

	if (UDP_Socket[0] != -1)
		while (udp_receive_packet( 0, packet, UPID_MAX_SIZE, &sender_addr) > 0);

	if (UDP_Socket[1] != -1)
		while (udp_receive_packet( 1, packet, UPID_MAX_SIZE, &sender_addr) > 0);
}

void net_udp_listen()
{
	int size;
	ubyte packet[UPID_MAX_SIZE];
	struct _sockaddr sender_addr;

	if (UDP_Socket[0] != -1)
	{
		size = udp_receive_packet( 0, packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size, 0 );
			size = udp_receive_packet( 0, packet, UPID_MAX_SIZE, &sender_addr );
		}
	}

	if (UDP_Socket[1] != -1)
	{
		size = udp_receive_packet( 1, packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size, 0 );
			size = udp_receive_packet( 1, packet, UPID_MAX_SIZE, &sender_addr );
		}
	}
#ifdef USE_TRACKER
	if( UDP_Socket[2] != -1 )
	{
		size = udp_receive_packet( 2, packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size, 0 );
			size = udp_receive_packet( 2, packet, UPID_MAX_SIZE, &sender_addr );
		}
	}
#endif
}

void net_udp_send_data(const ubyte * ptr, int len, int priority )
{
	char check;

	if (Endlevel_sequence)
		return;

	if ((UDP_MData.mbuf_size+len) > UPID_MDATA_BUF_SIZE )
	{
		check = ptr[0];
		net_udp_send_mdata(0, timer_query());
		if (UDP_MData.mbuf_size != 0)
			Int3();
		Assert(check == ptr[0]);
		(void)check;
	}

	Assert(UDP_MData.mbuf_size+len <= UPID_MDATA_BUF_SIZE);

	memcpy( &UDP_MData.mbuf[UDP_MData.mbuf_size], ptr, len );
	UDP_MData.mbuf_size += len;

	if (priority)
		net_udp_send_mdata((priority==2)?1:0, timer_query());
}

void net_udp_timeout_check(fix64 time)
{
	if(!multi_i_am_master() && is_observer()) { return; }

	int i = 0;
	static fix64 last_timeout_time = 0;
	
	if (time>=last_timeout_time+F1_0)
	{
		// Check for player timeouts
		for (i = 0; i < N_players; i++)
		{
			if ((i != Player_num) && (Players[i].connected != CONNECT_DISCONNECTED))
			{
				if ((Netgame.players[i].LastPacketTime == 0) || (Netgame.players[i].LastPacketTime > time))
				{
					Netgame.players[i].LastPacketTime = time;
				}
				else if ((time - Netgame.players[i].LastPacketTime) > UDP_TIMEOUT)
				{
					if((! Netgame.RetroProtocol) || multi_i_am_master() || i == 0) {
						multi_disconnect_player(i);
					} else if ((time - Netgame.players[i].LastPacketTime) > UDP_TIMEOUT*2) {
						multi_disconnect_player(i);
					} else {
						if(connection_statuses[i].type == CONNT_DIRECT) {
							connection_statuses[i].type = CONNT_PROXY;
							connection_statuses[i].proxy_through = 0;  // Start looking for efficient proxy?
						}
					}
				}
			}
		}
		last_timeout_time = time;
	}
}

void net_udp_do_frame(int force, int listen)
{
	fix64 time = 0;
	static fix64 last_pdata_time = 0, last_mdata_time = 16, last_endlevel_time = 32, last_bcast_time = 48, last_resync_time = 64;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	time = timer_query();

	if (WaitForRefuseAnswer && time>(RefuseTimeLimit+(F1_0*12)))
		WaitForRefuseAnswer=0;

	// Send positional update either in the regular PPS interval OR if forced AND at least every 66.6ms (nice for firing)
	if ((force && time >= (last_pdata_time+(F1_0/15))) || (time >= (last_pdata_time+(F1_0/Netgame.PacketsPerSec))))
	{
		last_pdata_time = time;
		net_udp_send_pdata();
	}
	
	if (force || (time >= (last_mdata_time+(F1_0/10))))
	{
		last_mdata_time = time;
		multi_send_robot_frame(0);
		net_udp_send_mdata(0, time);
	}

	net_udp_noloss_process_queue(time);

	if (VerifyPlayerJoined!=-1 && time >= last_resync_time+F1_0)
	{
		last_resync_time = time;
		net_udp_resend_sync_due_to_packet_loss(); // This will resend to UDP_sync_player
	}

	if ((time>=last_endlevel_time+F1_0) && Control_center_destroyed)
	{
		last_endlevel_time = time;
		net_udp_send_endlevel_packet();
	}

	// broadcast lite_info every 10 seconds
	if (multi_i_am_master() && time>=last_bcast_time+(F1_0*10))
	{
		last_bcast_time = time;
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
	}

#ifdef USE_TRACKER
	// If we use the tracker, tell the tracker about us every 10 seconds
	if( Netgame.Tracker )
	{
		// Static variable... the last time we sent to the tracker
		static fix64 iLastQuery = 0;
		static int iAttempts = 0;
		fix64 iNow = timer_query();
		
		// Set the last query to now if we must
		if( iLastQuery == 0 )
			iLastQuery = iNow;
		
		// Test it
		if( iTrackerVerified == 0 && iNow >= iLastQuery + ( F1_0 * 3 ) )
		{
			// Update it
			iLastQuery = iNow;
			iAttempts++;
		}
		
		// Have we had all our attempts?
		if( iTrackerVerified == 0 && iAttempts > 3 )
		{
			// Turn off tracker
			Netgame.Tracker = 0;
			
			// Reset the static variables for next time
			iLastQuery = 0;
			iAttempts = 0;
			
			// Warn
			nm_messagebox( TXT_WARNING, 1, TXT_OK, "No response from tracker!\nPossible causes:\nTracker is down\nYour port is likely not open!\n\nTracker: %s\nGame port: %s", GameArg.MplTrackerAddr, UDP_MyPort );
		}
	}
#endif

	

	if(Netgame.RetroProtocol) {
		net_udp_p2p_ping_frame(time); 
	} else {
		net_udp_ping_frame(time);
	}

	clean_pdata(time); 

	check_observers(time); 
	check_obs_buffer(time);

	if (listen)
	{
		net_udp_timeout_check(time);
		net_udp_listen();
		if (Network_send_objects)
			net_udp_send_objects();
		if (Network_sending_extras && VerifyPlayerJoined==-1)
			net_udp_send_extras();
	}

	udp_traffic_stat();
}

/* CODE FOR PACKET LOSS PREVENTION - START */
/*
 * Adds a packet to our queue. Should be called when an IMPORTANT mdata packet is created.
 * player_ack is an array which should contain 0 for each player that needs to send an ACK signal.
 */
void net_udp_noloss_add_queue_pkt(uint32_t pkt_num, fix64 time, ubyte *data, ushort data_size, ubyte pnum, ubyte player_ack[MAX_PLAYERS])
{
	int i, found = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++) // look for unused or oldest slot
	{
		if (UDP_mdata_queue[i].used)
		{
			if (UDP_mdata_queue[i].pkt_initial_timestamp > UDP_mdata_queue[found].pkt_initial_timestamp)
				found = i;
		}
		else
		{
			found = i;
			break;
		}
	}

	if (UDP_mdata_queue[found].used) // seems the slot we found is used (list is full) so screw  those who still need ack's.
	{
		con_printf(CON_DEBUG, "P#%i: MData store list is full!\n", Player_num);
		if (multi_i_am_master())
		{
			for ( i=1; i<N_players; i++ )
				if (UDP_mdata_queue[found].player_ack[i] == 0)
					net_udp_dump_player(Netgame.players[i].protocol.udp.addr, player_tokens[i], DUMP_PKTTIMEOUT);
		}
		else
		{
			Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
			if (Network_status==NETSTAT_PLAYING)
				multi_leave_game();
			if (Game_wind)
				window_set_visible(Game_wind, 0);
			nm_messagebox(NULL, 1, TXT_OK, "You left the game. You failed\nsending important packets (queue full).\nSorry.");
			if (Game_wind)
				window_set_visible(Game_wind, 1);
			multi_quit_game = 1;
			game_leave_menus();
			multi_reset_stuff();
		}
	}

	con_printf(CON_DEBUG, "P#%i: Adding MData pkt_num %i, type %i from P#%i to MData store list\n", Player_num, pkt_num, data[0], pnum);
	UDP_mdata_queue[found].used = 1;
	UDP_mdata_queue[found].pkt_initial_timestamp = time;
	for (i = 0; i < MAX_PLAYERS; i++)
		UDP_mdata_queue[found].pkt_timestamp[i] = time;
	UDP_mdata_queue[found].pkt_num = pkt_num;
	UDP_mdata_queue[found].Player_num = pnum;
	memcpy( &UDP_mdata_queue[found].player_ack, player_ack, sizeof(ubyte)*MAX_PLAYERS); 
	memcpy( &UDP_mdata_queue[found].data, data, sizeof(char)*data_size );
	UDP_mdata_queue[found].data_size = data_size;
}

void net_udp_noloss_obs_add_queue_pkt(uint32_t pkt_num, fix64 time, ubyte *data, ushort data_size, ubyte pnum, ubyte observer_ack[MAX_OBSERVERS])
{
	int i, found = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++) // look for unused or oldest slot
	{
		if (UDP_mdata_obs_queue[i].used)
		{
			if (UDP_mdata_obs_queue[i].pkt_initial_timestamp > UDP_mdata_obs_queue[found].pkt_initial_timestamp)
				found = i;
		}
		else
		{
			found = i;
			break;
		}
	}

	if (UDP_mdata_obs_queue[found].used) // seems the slot we found is used (list is full) so screw  those who still need ack's.
	{
		con_printf(CON_DEBUG, "P#%i: MData store list is full!\n", Player_num);
		if (multi_i_am_master())
		{
			for ( i=1; i<Netgame.max_numobservers; i++ )
				if (UDP_mdata_obs_queue[found].observer_ack[i] == 0)
					net_udp_dump_player(Netgame.observers[i].protocol.udp.addr, 0, DUMP_PKTTIMEOUT);
		}
		else
		{
			Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
			if (Network_status==NETSTAT_PLAYING)
				multi_leave_game();
			if (Game_wind)
				window_set_visible(Game_wind, 0);
			nm_messagebox(NULL, 1, TXT_OK, "You left the game. You failed\nsending important packets (queue full).\nSorry.");
			if (Game_wind)
				window_set_visible(Game_wind, 1);
			multi_quit_game = 1;
			game_leave_menus();
			multi_reset_stuff();
		}
	}

	con_printf(CON_DEBUG, "Observer: Adding MData pkt_num %i, type %i from P#%i to MData store list\n", pkt_num, data[0], pnum);
	UDP_mdata_obs_queue[found].used = 1;
	UDP_mdata_obs_queue[found].pkt_initial_timestamp = time;
	for (i = 0; i < MAX_PLAYERS; i++)
		UDP_mdata_obs_queue[found].pkt_timestamp[i] = time;
	UDP_mdata_obs_queue[found].pkt_num = pkt_num;
	UDP_mdata_obs_queue[found].Player_num = pnum;
	memcpy( &UDP_mdata_obs_queue[found].observer_ack, observer_ack, sizeof(ubyte)*MAX_OBSERVERS); 
	memcpy( &UDP_mdata_obs_queue[found].data, data, sizeof(char)*data_size );
	UDP_mdata_obs_queue[found].data_size = data_size;
}

/*
 * We have received a MDATA packet. Send ACK response to sender!
 * Also check in our UDP_mdata_got list, if we got this packet already. If yes, return 0 so do not process it!
 */
int net_udp_noloss_validate_mdata(uint32_t pkt_num, ubyte sender_pnum, struct _sockaddr sender_addr)
{
	ubyte buf[7];
	int i = 0, len = 0;

	// Check if this comes from a valid IP
	/*
	if (multi_i_am_master())
	{
		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[sender_pnum].protocol.udp.addr, sizeof(struct _sockaddr)))
			return 0;
	}
	else
	{
		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr)))
			return 0;
	}
	*/
	
	con_printf(CON_DEBUG, "P#%i: Sending MData ACK for pkt %i - pnum %i\n",Player_num, pkt_num, sender_pnum);
	memset(&buf,0,sizeof(buf));
	buf[len] = UPID_MDATA_ACK;													len++;
	buf[len] = Player_num;														len++;
	buf[len] = sender_pnum;														len++;
	PUT_INTEL_INT(buf + len, pkt_num);											len += 4;
	dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
	
	for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++)
	{
		if (pkt_num == UDP_mdata_got[sender_pnum].pkt_num[i])
			return 0; // we got this packet already
	}
	UDP_mdata_got[sender_pnum].cur_slot++;
	if (UDP_mdata_got[sender_pnum].cur_slot >= UDP_MDATA_STOR_QUEUE_SIZE)
		UDP_mdata_got[sender_pnum].cur_slot = 0;
	UDP_mdata_got[sender_pnum].pkt_num[UDP_mdata_got[sender_pnum].cur_slot] = pkt_num;
	return 1;
}

/* We got an ACK by a player. Set this player slot to positive! */
void net_udp_noloss_got_ack(ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	int i = 0, len = 0;
	uint32_t pkt_num = 0;
	ubyte sender_pnum = 0, dest_pnum = 0;

	if (data_len != 7)
		return;

																				len++;
	sender_pnum = data[len];													len++;
	dest_pnum = data[len];														len++;
	pkt_num = GET_INTEL_INT(&data[len]);										len += 4;
	if (Netgame.max_numobservers > 0 && sender_pnum == OBSERVER_PLAYER_ID) {
		for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++)
		{
			if ((pkt_num == UDP_mdata_obs_queue[i].pkt_num) && (dest_pnum == UDP_mdata_obs_queue[i].Player_num))
			{
				int obsnum = -1;
				for(int j = 0; j < Netgame.max_numobservers; j++) {
					if(! memcmp(&Netgame.observers[j].protocol.udp.addr, &sender_addr, sizeof(struct _sockaddr))) {
						obsnum = j;
						break;
					}
				}
				if (obsnum == -1) {
					return;
				}
				
				con_printf(CON_DEBUG, "P#%i: Got MData ACK for pkt_num %i from observer %i for pnum %i\n",Player_num, pkt_num, obsnum, dest_pnum);
				UDP_mdata_obs_queue[i].observer_ack[obsnum] = 1;
				break;
			}
		}
	} else {
		for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++)
		{
			if ((pkt_num == UDP_mdata_queue[i].pkt_num) && (dest_pnum == UDP_mdata_queue[i].Player_num))
			{
				con_printf(CON_DEBUG, "P#%i: Got MData ACK for pkt_num %i from pnum %i for pnum %i\n",Player_num, pkt_num, sender_pnum, dest_pnum);
				UDP_mdata_queue[i].player_ack[sender_pnum] = 1;
				break;
			}
		}
	}
}

/* Init/Free the queue. Call at start and end of a game or level. */
void net_udp_noloss_init_mdata_queue(void)
{
	con_printf(CON_DEBUG, "P#%i: Clearing MData store/GOT list\n",Player_num);
	memset(&UDP_mdata_queue,0,sizeof(UDP_mdata_store)*UDP_MDATA_STOR_QUEUE_SIZE);
	memset(&UDP_mdata_obs_queue,0,sizeof(UDP_mdata_obs_store)*UDP_MDATA_STOR_QUEUE_SIZE);
	memset(&UDP_mdata_got,0,sizeof(UDP_mdata_recv)*MAX_PLAYERS);
}

/* Reset the trace list for given player when (dis)connect happens */
void net_udp_noloss_clear_mdata_got(ubyte player_num)
{
	con_printf(CON_DEBUG, "P#%i: Clearing GOT list for %i\n",Player_num, player_num);
	memset(&UDP_mdata_got[player_num].pkt_num,0,sizeof(uint32_t)*UDP_MDATA_STOR_QUEUE_SIZE);
	UDP_mdata_got[player_num].cur_slot = 0;
}

/*
 * The main queue-process function.
 * Check if we can remove a packet from queue, and check if there are packets in queue which we need to re-send
 */
void net_udp_noloss_process_queue(fix64 time)
{
	int queuec = 0, plc = 0, total_len = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	for (queuec = 0; queuec < UDP_MDATA_STOR_QUEUE_SIZE; queuec++)
	{
		int needack = 0;
		
		if (!UDP_mdata_queue[queuec].used)
			continue;

		// Check if at least one connected player has not ACK'd the packet
		for (plc = 0; plc < MAX_PLAYERS; plc++)
		{
			// If player is not playing anymore, we can remove him from list. Also remove *me* (even if that should have been done already). Also make sure Clients do not send to anyone else than Host
			if ((Players[plc].connected != CONNECT_PLAYING || plc == Player_num) || (!multi_i_am_master() && plc > 0))
				UDP_mdata_queue[queuec].player_ack[plc] = 1;

			if (!UDP_mdata_queue[queuec].player_ack[plc])
			{
				// Resend if enough time has passed.
				if (UDP_mdata_queue[queuec].pkt_timestamp[plc] + (F1_0/3) <= time)
				{
					ubyte buf[sizeof(UDP_mdata_info)];
					int len = 0;
					
					con_printf(CON_DEBUG, "P#%i: Resending pkt_num %i from pnum %i to pnum %i\n",Player_num, UDP_mdata_queue[queuec].pkt_num, UDP_mdata_queue[queuec].Player_num, plc);
					
					UDP_mdata_queue[queuec].pkt_timestamp[plc] = time;
					memset(&buf, 0, sizeof(UDP_mdata_info));
					
					// Prepare the packet and send it
					buf[len] = UPID_MDATA_PNEEDACK;													len++;
					PUT_INTEL_INT(buf + len, netgame_token); 	len += 4; 
					buf[len] = UDP_mdata_queue[queuec].Player_num;								len++;
					PUT_INTEL_INT(buf + len, UDP_mdata_queue[queuec].pkt_num);					len += 4;
					memcpy(&buf[len], UDP_mdata_queue[queuec].data, sizeof(char)*UDP_mdata_queue[queuec].data_size);
																								len += UDP_mdata_queue[queuec].data_size;
					dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[plc].protocol.udp.addr, sizeof(struct _sockaddr));
					total_len += len;
				}
				needack++;
			}
		}

		// Check if we can remove that packet due to to it had no resend's or Timeout
		if (needack==0 || (UDP_mdata_queue[queuec].pkt_initial_timestamp + UDP_TIMEOUT <= time))
		{
			if (needack) // packet timed out but still not all have ack'd. SCREW THEM NOW!
			{
				if (multi_i_am_master())
				{
					for ( plc=1; plc<N_players; plc++ )
						if (UDP_mdata_queue[queuec].player_ack[plc] == 0)
							net_udp_dump_player(Netgame.players[plc].protocol.udp.addr, player_tokens[plc], DUMP_PKTTIMEOUT);
				}
				else
				{
					Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
					if (Network_status==NETSTAT_PLAYING)
						multi_leave_game();
					if (Game_wind)
						window_set_visible(Game_wind, 0);
					nm_messagebox(NULL, 1, TXT_OK, "You left the game. You failed\nsending important packets (no ack).\nSorry.");
					if (Game_wind)
						window_set_visible(Game_wind, 1);
					multi_quit_game = 1;
					game_leave_menus();
					multi_reset_stuff();
				}
			}
			con_printf(CON_DEBUG, "P#%i: Removing stored pkt_num %i - missing ACKs: %i\n",Player_num, UDP_mdata_queue[queuec].pkt_num, needack);
			memset(&UDP_mdata_queue[queuec],0,sizeof(UDP_mdata_store));
		}

		// Send up to half our max packet size
		if (total_len >= (UPID_MAX_SIZE/2))
			break;
	}

	for (queuec = 0; queuec < UDP_MDATA_STOR_QUEUE_SIZE; queuec++)
	{
		int needack = 0;
		
		if (!UDP_mdata_obs_queue[queuec].used)
			continue;

		// Check if at least one connected observer has not ACK'd the packet
		for (plc = 0; plc < Netgame.max_numobservers; plc++)
		{
			// If observer is not connected anymore, we can remove him from list.
			if (Netgame.observers[plc].connected == 0)
				UDP_mdata_obs_queue[queuec].observer_ack[plc] = 1;

			if (!UDP_mdata_obs_queue[queuec].observer_ack[plc])
			{
				// Resend if enough time has passed.
				if (UDP_mdata_obs_queue[queuec].pkt_timestamp[plc] + (F1_0/3) <= time)
				{
					ubyte buf[sizeof(UDP_mdata_info)];
					int len = 0;
					
					con_printf(CON_DEBUG, "P#%i: Resending pkt_num %i from pnum %i to observer %i\n", Player_num, UDP_mdata_queue[queuec].pkt_num, UDP_mdata_queue[queuec].Player_num, plc);
					
					UDP_mdata_obs_queue[queuec].pkt_timestamp[plc] = time;
					memset(&buf, 0, sizeof(UDP_mdata_info));
					
					// Prepare the packet and send it
					buf[len] = UPID_MDATA_PNEEDACK;													len++;
					PUT_INTEL_INT(buf + len, netgame_token); 	len += 4; 
					buf[len] = UDP_mdata_obs_queue[queuec].Player_num;								len++;
					PUT_INTEL_INT(buf + len, UDP_mdata_obs_queue[queuec].pkt_num);					len += 4;
					memcpy(&buf[len], UDP_mdata_obs_queue[queuec].data, sizeof(char)*UDP_mdata_obs_queue[queuec].data_size);
																								len += UDP_mdata_obs_queue[queuec].data_size;
					dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.observers[plc].protocol.udp.addr, sizeof(struct _sockaddr));
					total_len += len;
				}
				needack++;
			}
		}

		// Check if we can remove that packet due to to it had no resend's or Timeout
		if (needack==0 || (UDP_mdata_obs_queue[queuec].pkt_initial_timestamp + UDP_TIMEOUT <= time))
		{
			if (needack) // packet timed out but still not all have ack'd. SCREW THEM NOW!
			{
				if (multi_i_am_master())
				{
					for ( plc=0; plc < Netgame.max_numobservers; plc++ )
						if (UDP_mdata_obs_queue[queuec].observer_ack[plc] == 0)
							net_udp_dump_player(Netgame.observers[plc].protocol.udp.addr, 0, DUMP_PKTTIMEOUT);
				}
				else
				{
					Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
					if (Network_status==NETSTAT_PLAYING)
						multi_leave_game();
					if (Game_wind)
						window_set_visible(Game_wind, 0);
					nm_messagebox(NULL, 1, TXT_OK, "You left the game. You failed\nsending important packets (no ack).\nSorry.");
					if (Game_wind)
						window_set_visible(Game_wind, 1);
					multi_quit_game = 1;
					game_leave_menus();
					multi_reset_stuff();
				}
			}
			con_printf(CON_DEBUG, "P#%i: Removing stored pkt_num %i - missing ACKs: %i\n",Player_num, UDP_mdata_obs_queue[queuec].pkt_num, needack);
			memset(&UDP_mdata_obs_queue[queuec],0,sizeof(UDP_mdata_obs_store));
		}

		// Send up to half our max packet size
		if (total_len >= (UPID_MAX_SIZE/2))
			break;
	}
}
/* CODE FOR PACKET LOSS PREVENTION - END */


void net_udp_send_mdata_direct(ubyte *data, int data_len, int pnum, int needack)
{
	ubyte buf[sizeof(UDP_mdata_info)];
	ubyte pack[MAX_PLAYERS];
	int len = 0;
	
	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!(data_len > 0))
		return;

	//if (!multi_i_am_master() && pnum != 0)
	//	Error("Client sent direct data to non-Host in net_udp_send_mdata_direct()!\n");

	if (!Netgame.PacketLossPrevention)
		needack = 0;

	memset(&buf, 0, sizeof(UDP_mdata_info));
	memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);

	pack[pnum] = 0;

	if (needack)
		buf[len] = UPID_MDATA_PNEEDACK;
	else if (data[0] == MULTI_OBS_MESSAGE)
		buf[len] = UPID_OBSDATA;
    else
		buf[len] = UPID_MDATA_PNORM;
																				len++;

	PUT_INTEL_INT(buf + len, netgame_token);									len += 4; 																				
	buf[len] = Player_num;														len++;
	if (needack)
	{
		UDP_MData.pkt_num++;
		Assert(UDP_MDATA_STOR_QUEUE_SIZE*100 < INT_MAX);
		if (UDP_MData.pkt_num > UDP_MDATA_STOR_QUEUE_SIZE*100) // roll over at some point
			UDP_MData.pkt_num = 0;
		PUT_INTEL_INT(buf + len, UDP_MData.pkt_num);							len += 4;
	}
	memcpy(&buf[len], data, sizeof(char)*data_len);								len += data_len;

	if (pnum == Player_num && multi_i_am_master())
		forward_to_observers(buf, len, needack); 
	else
		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[pnum].protocol.udp.addr, sizeof(struct _sockaddr));

	if (needack)
		net_udp_noloss_add_queue_pkt(UDP_MData.pkt_num, timer_query(), data, data_len, Player_num, pack);
}


void net_udp_send_mdata(int needack, fix64 time)
{
 
	ubyte buf[sizeof(UDP_mdata_info)];
	ubyte pack[MAX_PLAYERS];
	int len = 0, i = 0;
	
	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!(UDP_MData.mbuf_size > 0))
		return;

	if (!Netgame.PacketLossPrevention)
		needack = 0;

	memset(&buf, 0, sizeof(UDP_mdata_info));
	memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);

	if (needack)
		buf[len] = UPID_MDATA_PNEEDACK;
	else
		buf[len] = UPID_MDATA_PNORM;
																				len++;
	PUT_INTEL_INT(buf + len, netgame_token);									len += 4; 																				
	buf[len] = Player_num;														len++;
	if (needack)
	{
		UDP_MData.pkt_num++;
		PUT_INTEL_INT(buf + len, UDP_MData.pkt_num);							len += 4;
	}
	memcpy(buf + len, UDP_MData.mbuf, sizeof(char)*UDP_MData.mbuf_size);		len += UDP_MData.mbuf_size;

	if(Netgame.RetroProtocol && ! needack) {
		for (i = 0; i < MAX_PLAYERS; i++) {
			if (Players[i].connected == CONNECT_PLAYING && i != Player_num)
			{
				net_udp_send_to_player(buf, len, i); 
				//dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
				pack[i] = 0;
			}
		}

		if(multi_i_am_master()) {
			forward_to_observers(buf, len, needack); 
		}
	} else {
		if (multi_i_am_master())
		{
			for (i = 1; i < MAX_PLAYERS; i++)
			{
				if (Players[i].connected == CONNECT_PLAYING)
				{
					dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
					pack[i] = 0;
				}
			}

			if(Netgame.RetroProtocol) {
				forward_to_observers(buf, len, needack); 
			}
		}
		else
		{
			dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr));
			pack[0] = 0;
		}
	}

	if (needack)
		net_udp_noloss_add_queue_pkt(UDP_MData.pkt_num, time, UDP_MData.mbuf, UDP_MData.mbuf_size, Player_num, pack);

	// Clear UDP_MData except pkt_num. That one must not be deleted so we can clearly keep track of important packets.
	UDP_MData.type = 0;
	UDP_MData.Player_num = 0;
	UDP_MData.mbuf_size = 0;
	memset(&UDP_MData.mbuf, 0, sizeof(ubyte)*UPID_MDATA_BUF_SIZE);
}

void net_udp_process_mdata (ubyte *data, int data_len, struct _sockaddr sender_addr, int needack)
{
	int pnum = data[5], dataoffset = (needack?10:6);

	// Check if packet might be bogus
	if ((pnum < 0) || (data_len > sizeof(UDP_mdata_info)))
		return;

	// Check if it came from valid IP
	if (Netgame.RetroProtocol) {
		// For Retro Protocol, all we care about is that the sender's IP is from any player.
		if (!is_any_player_ip(sender_addr)) {
			if (!multi_i_am_master() || Netgame.max_numobservers == 0 || pnum != OBSERVER_PLAYER_ID || !is_observer_ip(sender_addr)) {
				drop_rx_packet(data, "not received from any player ip"); 
				return;
			}
		}
	} else {
		if (multi_i_am_master())
		{
			if (! is_player_ip(sender_addr, pnum))
			{
				if (Netgame.max_numobservers == 0 || pnum != OBSERVER_PLAYER_ID || ! is_observer_ip(sender_addr))
				{
					drop_rx_packet(data, "not received from player ip"); 
					return;
				}
			}
		}
		else
		{
			if (! is_master_ip(sender_addr))
			{
				drop_rx_packet(data, "not received from master ip"); 
				return;
			}
		}
	}

	// Add needack packet and check for possible redundancy
	if (needack)
	{
		if (!net_udp_noloss_validate_mdata(GET_INTEL_SHORT(&data[6]), pnum, sender_addr))
			return;
	}

	if(Netgame.RetroProtocol && ! needack) {
	} else {
		// send this to everyone else (if master)
		if (multi_i_am_master())
		{
			ubyte pack[MAX_PLAYERS];
			int i = 0;

			memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);
			
			for (i = 1; i < MAX_PLAYERS; i++)
			{
				if ((i != pnum) && Players[i].connected == CONNECT_PLAYING)
				{
					dxx_sendto (UDP_Socket[0], data, data_len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
					pack[i] = 0;
				}
			}

			if (needack && N_players > 2)
			{
				net_udp_noloss_add_queue_pkt(GET_INTEL_SHORT(&data[6]), timer_query(), data+dataoffset, data_len-dataoffset, pnum, pack);
			}
		}
	}

	// Check if we are in correct state to process the packet
	if (!((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING))
		return;

	// Process
	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL))
	{
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		multi_process_bigdata(data+dataoffset, data_len-dataoffset);
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	multi_process_bigdata( data+dataoffset, data_len-dataoffset );
}

void net_udp_process_obs_data (ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	if (!Netgame.RetroProtocol) {
        return;
    }

	int pnum = data[5], dataoffset = 6;

	// Check if packet might be bogus
	if ((pnum < 0) || (data_len > sizeof(UDP_mdata_info)))
		return;

    // If we are a non-master player, this is a bad packet
    if (!is_observer()) {
        drop_rx_packet(data, "received by non-observer"); 
        return;
    }

	// Check if it came from valid IP
    if (!is_player_ip(sender_addr, 0) && !is_observer_ip(sender_addr)) {
        drop_rx_packet(data, "not received from master or observer ip"); 
        return;
    }

	// Check if we are in correct state to process the packet
	if (!((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING))
		return;

	// Process
	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL))
	{
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		multi_process_bigdata(data+dataoffset, data_len-dataoffset);
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	multi_process_bigdata( data+dataoffset, data_len-dataoffset );
}

// Would like observer info packets to avoid delay, but as mdata they're indistinguishable from things
// we really do want to delay.  The options are deep packet inspection (difficult with mdata) and sending these
// things in one packet, or building an mdata_nodelay packet which needs to snuggle in with the needack infrastructure
// For now, I'm accepting the glitch that observer info is outdated for observers by broadcast_delay
// It isn't outdated for players, which is the main thing.
void forward_to_observers(ubyte *data, int data_len, int needack) {
	if(Netgame.max_numobservers == 0) { return; }

	if(Netgame.obs_delay) {
		add_message_to_obs_buffer(data, data_len, needack);
	} else {
		forward_to_observers_nodelay(data, data_len, needack); 
	}
}

void add_message_to_obs_buffer(ubyte *data, int data_len, int needack) {
	//con_printf(CON_NORMAL, "Queueing obs message; next: %d, cur: %d, max: %d\n", next_obs_msg_to_send, cur_obs_msg, MAX_OBS_MESSAGES); 
	if(observer_data_buffer == 0) {
		observer_data_buffer = d_malloc(MAX_OBS_MESSAGES*MAX_MESSAGE_SIZE); 
		next_obs_msg_to_send = cur_obs_msg = 0; 
	}

	if( (next_obs_msg_to_send == cur_obs_msg + 1) ||
		((next_obs_msg_to_send == 0) && (cur_obs_msg == MAX_OBS_MESSAGES -1)) )  {
		con_printf(CON_URGENT, "Observer message queue full!  Message dropped.\n"); 
		return;
	}

	// Find a spot for the message
	int bufslot = observer_message_offsets[cur_obs_msg] + observer_message_lengths[cur_obs_msg];
	if(bufslot + data_len >= MAX_OBS_MESSAGES * MAX_MESSAGE_SIZE) {
		bufslot = 0;
	}

	if( 
		// Come up on it from behind
		((observer_message_offsets[cur_obs_msg] < observer_message_offsets[next_obs_msg_to_send]) ||

		// Hit it during wraparound
		(bufslot == 0 && observer_message_offsets[cur_obs_msg] != 0))

		 &&
		(bufslot + data_len >= observer_message_offsets[next_obs_msg_to_send])
		
	  ) {
		con_printf(CON_URGENT, "Observer message queue out of space!  Message dropped.\n"); 
		return;
	}

	cur_obs_msg++; 
	if(cur_obs_msg >= MAX_OBS_MESSAGES) {
		cur_obs_msg = 0; 
	}
	memcpy(observer_data_buffer + bufslot, data, data_len);
	observer_message_offsets[cur_obs_msg] = bufslot;
	observer_message_lengths[cur_obs_msg] = data_len;
	observer_message_needack[cur_obs_msg] = needack;
	observer_message_timestamps[cur_obs_msg] = timer_query();

	//con_printf(CON_NORMAL, "Put message length %d in slot %d, buffer offset %d\n", data_len, cur_obs_msg, bufslot); 
}

void check_obs_buffer(fix64 now) {
	if (! multi_i_am_master()) { return; }

	//con_printf(CON_NORMAL, "Checking obs buffer; %d != %d && %f < %f\n", next_obs_msg_to_send, cur_obs_msg, (double)(observer_message_timestamps[next_obs_msg_to_send])/(double)(F1_0), (double)(now - OBSERVER_DELAY*F1_0)/(double)(F1_0)) ;

	while( (next_obs_msg_to_send != cur_obs_msg) &&
		   (observer_message_timestamps[next_obs_msg_to_send] < now - OBSERVER_DELAY*F1_0)) {

		forward_to_observers_nodelay(observer_data_buffer + observer_message_offsets[next_obs_msg_to_send], 
			observer_message_lengths[next_obs_msg_to_send], observer_message_needack[next_obs_msg_to_send]);

	    next_obs_msg_to_send++;

	    if(next_obs_msg_to_send >= MAX_OBS_MESSAGES) {
	    	next_obs_msg_to_send = 0; 
	    }

	    //con_printf(CON_NORMAL, "Sent obs message; next: %d, cur: %d, max: %d\n", next_obs_msg_to_send, cur_obs_msg, MAX_OBS_MESSAGES); 
	
	}
}

void forward_to_observers_nodelay(ubyte *data, int data_len, int needack) {
	if (multi_i_am_master()) {
		ubyte pack[MAX_OBSERVERS];
		for (int i = 0; i < Netgame.max_numobservers; i++) {
			if (Netgame.observers[i].connected) {
				dxx_sendto (UDP_Socket[0], data, data_len, 0, (struct sockaddr *)&Netgame.observers[i].protocol.udp.addr, sizeof(struct _sockaddr));
				pack[i] = 1;
			}
		}
	}
}

void net_udp_send_pdata()
{
	if(is_observer()) { return; }

	ubyte buf[UPID_PDATA_U_SIZE];
	int len = 0, i = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;
	if (Players[Player_num].connected != CONNECT_PLAYING)
		return;

	current_pdata = (current_pdata + 1) % MAX_LOSS_BUFFER; 

	memset(&buf, 0, sizeof(UDP_frame_info));
	
	buf[len] = UPID_PDATA;										len++;
	PUT_INTEL_INT(buf + len, netgame_token);					len += 4; 
	buf[len] = Player_num;										len++;
	buf[len] = Players[Player_num].connected;							len++; // 3
	if(Netgame.RetroProtocol) 
	{
		object* player = Objects+Players[Player_num].objnum;

		PUT_INTEL_INT(buf + len, player->orient.rvec.x); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.rvec.y); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.rvec.z); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.uvec.x); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.uvec.y); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.uvec.z); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.fvec.x); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.fvec.y); 	len += 4; 
		PUT_INTEL_INT(buf + len, player->orient.fvec.z); 	len += 4; 		
		PUT_INTEL_INT(buf+len, player->pos.x);					len += 4;
		PUT_INTEL_INT(buf+len, player->pos.y);					len += 4;
		PUT_INTEL_INT(buf+len, player->pos.z);					len += 4;
		PUT_INTEL_INT(buf+len, player->mtype.phys_info.velocity.x);					len += 4;
		PUT_INTEL_INT(buf+len, player->mtype.phys_info.velocity.y);					len += 4;
		PUT_INTEL_INT(buf+len, player->mtype.phys_info.velocity.z);					len += 4;
		PUT_INTEL_INT(buf+len, player->mtype.phys_info.rotvel.x);				len += 4;
		PUT_INTEL_INT(buf+len, player->mtype.phys_info.rotvel.y);				len += 4;
		PUT_INTEL_INT(buf+len, player->mtype.phys_info.rotvel.z);				len += 4; 		
		buf[len] = current_pdata; len++;

	} else if (Netgame.ShortPackets)
	{
		shortpos spp;
		memset(&spp, 0, sizeof(shortpos));
		create_shortpos(&spp, Objects+Players[Player_num].objnum, 0);
		memcpy(buf + len, &spp.bytemat, 9);							len += 9;
		PUT_INTEL_SHORT(buf+len, spp.xo);							len += 2;
		PUT_INTEL_SHORT(buf+len, spp.yo);							len += 2;
		PUT_INTEL_SHORT(buf+len, spp.zo);							len += 2;
		PUT_INTEL_SHORT(buf+len, spp.segment);							len += 2;
		PUT_INTEL_SHORT(buf+len, spp.velx);							len += 2;
		PUT_INTEL_SHORT(buf+len, spp.vely);							len += 2;
		PUT_INTEL_SHORT(buf+len, spp.velz);							len += 2; // 23 + 3 = 26
		buf[len] = current_pdata; len++;
	}
	else
	{
		quaternionpos qpp;
		memset(&qpp, 0, sizeof(quaternionpos));
		create_quaternionpos(&qpp, Objects+Players[Player_num].objnum, 0);
		PUT_INTEL_SHORT(buf+len, qpp.orient.w);							len += 2;
		PUT_INTEL_SHORT(buf+len, qpp.orient.x);							len += 2;
		PUT_INTEL_SHORT(buf+len, qpp.orient.y);							len += 2;
		PUT_INTEL_SHORT(buf+len, qpp.orient.z);							len += 2;
		PUT_INTEL_INT(buf+len, qpp.pos.x);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.pos.y);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.pos.z);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.vel.x);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.vel.y);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.vel.z);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.rotvel.x);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.rotvel.y);							len += 4;
		PUT_INTEL_INT(buf+len, qpp.rotvel.z);							len += 4; // 44 + 3 = 47
		buf[len] = current_pdata; len++;
	}

	if(Netgame.RetroProtocol) {
		for (i = 0; i < MAX_PLAYERS; i++) {
			if (Players[i].connected && i != Player_num) {
				net_udp_send_to_player(buf, len, i); 
			}
		}

		if(multi_i_am_master()) {
			forward_to_observers(buf, len, 0); 
		}
	} else {
		if (multi_i_am_master())
		{
			for (i = 1; i < MAX_PLAYERS; i++)
				if (Players[i].connected != CONNECT_DISCONNECTED) {
					dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
				}
		}
		else
		{
			dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr));
		}
	}

	if(Send_ship_status && GameTime64 >= Next_ship_status_time)
	{
		multi_send_ship_status_for_frame();

		// Send at most 6 ship statuses per second.
		Next_ship_status_time = GameTime64 + F1_0 / 6;
		Send_ship_status = 0;
	}
}

void received_pdata_num(int player, ubyte num) {
	if(count_pdata_received[player] < MAX_LOSS_COUNTED) {
		count_pdata_received[player]++;
	}

	pdata_received[player][num%MAX_LOSS_BUFFER] = 1; 
	last_pdata_received[player] = num;
	last_pdata_received_at[player] = timer_query(); 
}

void clean_pdata(fix64 now) {
	static fix64 last_cleaned = 0; 
	fix64 clean_interval = F1_0/10; 

	if(now - last_cleaned < clean_interval) {
		return; 
	}

	last_cleaned = now;

	for(int player = 0; player < MAX_PLAYERS; player++) {
		if(player == Player_num) continue; 

		if(! Netgame.players[player].connected) {
			// Wipe everything! 
			memset(pdata_received + player, 0, MAX_LOSS_BUFFER); 
			last_pdata_received[player] = 0;
			count_pdata_received[player] = 0;
			last_pdata_received_at[player] = 0; 
		} else {
			int packets_expected_since_last = f2i( (now - last_pdata_received_at[player]) * Netgame.PacketsPerSec);

			int last_entry = last_pdata_received[player]; 
			int num_cleaned = 0; 
			for(int i = 0; i < packets_expected_since_last + 5 && i < MAX_LOSS_BUFFER; i++) {
				pdata_received[player][(last_entry + i + 1)%MAX_LOSS_BUFFER] = 0; 
				num_cleaned++; 
			}

			int current_expected_entry = last_entry + (packets_expected_since_last > 2 ? packets_expected_since_last : 0);
			int start_counting_at = (current_expected_entry - MAX_LOSS_COUNTED + 1 + MAX_LOSS_BUFFER) % MAX_LOSS_BUFFER; 
			int lost = 0;

			for(int i = 0; i < MAX_LOSS_COUNTED; i++) {
				if(! pdata_received[player][(start_counting_at + i) % MAX_LOSS_BUFFER]) {
					lost++; 
				}
			}

			int total = MAX_LOSS_COUNTED;
			if(count_pdata_received[player] < MAX_LOSS_COUNTED) {
				total = count_pdata_received[player];
				lost -= (MAX_LOSS_COUNTED - total);  
			}

			if(Network_status == NETSTAT_PLAYING) {
				if(total < 10) {
					Netgame.players[player].loss = 0;
				} else {
					Netgame.players[player].loss = lost * 100 / total; 
				}
			}

	
		}
	}

}

void check_observers(fix64 now) {
	if(! multi_i_am_master() ) { return; }

	int changed_something = 0; 
	for(int i = 0; i < Netgame.max_numobservers; i++) {
		if (Netgame.observers[i].connected == 1) {
			if(now - Netgame.observers[i].LastPacketTime > F1_0*10) {
				memset(&Netgame.observers[i], 0, sizeof(netplayer_info));
				Netgame.numobservers--;

				changed_something = 1; 
			}
		}
	}

	if(changed_something) {
		multi_send_obs_update(1, 0); 
	}

}

void net_udp_process_pdata ( ubyte *data, int data_len, struct _sockaddr sender_addr )
{
	UDP_frame_info pd;
	int len = 0, i = 0;

	if ( !( Game_mode & GM_NETWORK && ( Network_status == NETSTAT_PLAYING || Network_status == NETSTAT_ENDLEVEL ||  Network_status==NETSTAT_WAITING ) ) )
		return;

	len++;
	len += 4; // token 

	memset(&pd, 0, sizeof(UDP_frame_info));
	

	if(! Netgame.RetroProtocol ) {
		if ((Netgame.ShortPackets && data_len != UPID_PDATA_S_SIZE) || (!Netgame.ShortPackets && data_len != UPID_PDATA_Q_SIZE))
			return;

		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[((multi_i_am_master())?(data[len]):(0))].protocol.udp.addr, sizeof(struct _sockaddr)))
			return;
	}

	pd.Player_num = data[len];									len++;
	pd.connected = data[len];									len++;

	// No remote control, please
	if(pd.Player_num == Player_num) {
		drop_rx_packet(data, "attempted remote control");
		return;
	}

	// That would be bad
	if(pd.Player_num < 0 || pd.Player_num >= MAX_PLAYERS) { 
		drop_rx_packet(data, "invalid player number");
		return; 
	}

	// Can't do this check -- proxies
	//if(! is_player_ip(sender_addr, pd.Player_num)) {
	//
	//}

	ubyte packet_num = 0; 
	if(Netgame.RetroProtocol) 
	{
		pd.ptype.upp.orient.rvec.x = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.rvec.y = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.rvec.z = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.uvec.x = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.uvec.y = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.uvec.z = GET_INTEL_INT(&data[len]); len += 4; 	
		pd.ptype.upp.orient.fvec.x = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.fvec.y = GET_INTEL_INT(&data[len]); len += 4; 
		pd.ptype.upp.orient.fvec.z = GET_INTEL_INT(&data[len]); len += 4; 		
		pd.ptype.upp.pos.x = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.upp.pos.y = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.upp.pos.z = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.upp.vel.x = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.upp.vel.y = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.upp.vel.z = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.upp.rotvel.x = GET_INTEL_INT(&data[len]);					len += 4;
		pd.ptype.upp.rotvel.y = GET_INTEL_INT(&data[len]);					len += 4;
		pd.ptype.upp.rotvel.z = GET_INTEL_INT(&data[len]);					len += 4;		
		packet_num = data[len]; len++; 
	}
	else if (Netgame.ShortPackets)
	{
		memcpy(&pd.ptype.spp.bytemat, &(data[len]), 9);						len += 9;
		pd.ptype.spp.xo = GET_INTEL_SHORT(&data[len]);						len += 2;
		pd.ptype.spp.yo = GET_INTEL_SHORT(&data[len]);						len += 2;
		pd.ptype.spp.zo = GET_INTEL_SHORT(&data[len]);						len += 2;
		pd.ptype.spp.segment = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.spp.velx = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.spp.vely = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.spp.velz = GET_INTEL_SHORT(&data[len]);					len += 2;
		packet_num = data[len]; len++; 
	}
	else
	{
		pd.ptype.qpp.orient.w = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.qpp.orient.x = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.qpp.orient.y = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.qpp.orient.z = GET_INTEL_SHORT(&data[len]);					len += 2;
		pd.ptype.qpp.pos.x = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.qpp.pos.y = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.qpp.pos.z = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.qpp.vel.x = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.qpp.vel.y = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.qpp.vel.z = GET_INTEL_INT(&data[len]);						len += 4;
		pd.ptype.qpp.rotvel.x = GET_INTEL_INT(&data[len]);					len += 4;
		pd.ptype.qpp.rotvel.y = GET_INTEL_INT(&data[len]);					len += 4;
		pd.ptype.qpp.rotvel.z = GET_INTEL_INT(&data[len]);					len += 4;
		packet_num = data[len]; len++;  
	}
	

	// Drop out of order packets

	int last_received = last_pdata_received[pd.Player_num];
	int modular_packet_num = packet_num - last_received; 
	if(modular_packet_num < 0) {
		modular_packet_num += MAX_LOSS_BUFFER; 
	}
	if(modular_packet_num > MAX_LOSS_BUFFER - 10) {
		if(last_pdata_received_at != 0 && last_received != 0) {
			received_pdata_num(pd.Player_num, packet_num);
			char err_mess[200]; 
			snprintf(err_mess, 200, "out-of-order packet %d (last received %d).", packet_num, last_received); 
			drop_rx_packet(data, err_mess); 
			return; 
		}
	}
	

	received_pdata_num(pd.Player_num, packet_num); 

	if(! Netgame.RetroProtocol) {
		if (multi_i_am_master()) // I am host - must relay this packet to others!
		{
			if (pd.Player_num > 0 && pd.Player_num <= N_players && Players[pd.Player_num].connected == CONNECT_PLAYING) // some checking wether this packet is legal
			{
				for (i = 1; i < MAX_PLAYERS; i++)
				{
					if (i != pd.Player_num && Players[i].connected != CONNECT_DISCONNECTED) // not to sender or disconnected players - right.
						dxx_sendto (UDP_Socket[0], data, data_len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
				}
			}
		}
	}

	net_udp_read_pdata_packet (&pd);
}

void net_udp_read_pdata_packet(UDP_frame_info *pd)
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;

	TheirPlayernum = pd->Player_num;
	TheirObjnum = Players[pd->Player_num].objnum;


	if (multi_i_am_master())
	{
		// latecoming player seems to successfully have synced
		if ( VerifyPlayerJoined != -1 && TheirPlayernum == VerifyPlayerJoined )
			VerifyPlayerJoined=-1;
		// we say that guy is disconnected so we do not want him/her in game
		if ( Players[TheirPlayernum].connected == CONNECT_DISCONNECTED )
			return;
	}
	else
	{
		// only by reading pdata a client can know if a player reconnected. So do that here.
		// NOTE: we might do this somewhere else - maybe with a sync packet like when adding a fresh player.
		if ( Players[TheirPlayernum].connected == CONNECT_DISCONNECTED && pd->connected == CONNECT_PLAYING )
		{
			Players[TheirPlayernum].connected = CONNECT_PLAYING;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_reconnect(TheirPlayernum);

			digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
			ClipRank (&Netgame.players[TheirPlayernum].rank);
			
			if (PlayerCfg.NoRankings)
				HUD_init_message(HM_MULTI,  "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );
			else
				HUD_init_message(HM_MULTI,  "%s'%s' %s", RankStrings[Netgame.players[TheirPlayernum].rank],Players[TheirPlayernum].callsign, TXT_REJOIN );

			multi_send_score();

			net_udp_noloss_clear_mdata_got(TheirPlayernum);
		}
	}

	if (Players[TheirPlayernum].connected != CONNECT_PLAYING || TheirPlayernum == Player_num)
		return;

	if (!multi_quit_game && (TheirPlayernum >= N_players))
	{
		if (Network_status!=NETSTAT_WAITING)
		{
			//Int3(); // We missed an important packet!
			//multi_consistency_error(0);
			net_log_comment("Join game consistency error"); 
			return;
		}
		else
			return;
	}

	TheirObj = &Objects[TheirObjnum];
	Netgame.players[TheirPlayernum].LastPacketTime = timer_query();

	//------------ Read the player's ship's object info ----------------------

	if(Netgame.RetroProtocol)
		extract_uncompressedpos(TheirObj, &pd->ptype.upp, 0);
	else if (Netgame.ShortPackets)
		extract_shortpos(TheirObj, &pd->ptype.spp, 0);
	else
		extract_quaternionpos(TheirObj, &pd->ptype.qpp, 0);
	if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);
}

void net_udp_send_p2p_reattempt_direct (int to_player, int connect_to_player) {
	
	ubyte buf[UPID_REATTEMPT_DIRECT_SIZE]; 
	int len = 0;

	buf[len] = UPID_REATTEMPT_DIRECT; len++;
	PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
	buf[len] = connect_to_player; len++;
	memcpy(buf + len, &Netgame.players[connect_to_player].protocol.udp.addr, sizeof(struct _sockaddr)); 

	net_udp_send_to_player_direct(buf, sizeof(buf), to_player); 
}

void net_udp_process_p2p_reattempt_direct (ubyte *data, struct _sockaddr sender_addr, int data_len) {
	int len = 0;  len++; // header
	len += 4; // token 

	int pnum = data[len]; len++;

	if(pnum == multi_who_is_master()) {
		net_log_comment("Attempting reconnect to master, illegal."); 
		return;
	}

	if(pnum == Player_num) {
		net_log_comment("Attempting reconnect to self, illegal."); 
		return;
	}  

	if(pnum >= MAX_PLAYERS) {
		net_log_comment("Attempting connection to illegal player num.");
		return;
	}

	struct _sockaddr new_address;	
	memcpy(&new_address, data + len, sizeof(struct _sockaddr)); 

	update_address_for_player(pnum, new_address); 
	reattemptDirect(pnum); 
}

void net_udp_send_p2p_ping (int to_player, int force_direct, fix64 time) {
	ubyte buf[UPID_P2P_PING_SIZE]; 
	int len = 0;

	if((! multi_i_am_master()) &&
		(! to_player == multi_who_is_master()) && 
		(connection_statuses[to_player].type == CONNT_DIRECT) &&
	   (timer_query() - connection_statuses[to_player].last_direct_pong < F1_0 * 20) && 
	   (timer_query() - connection_statuses[to_player].last_direct_pong > F1_0 * 5)
	  ) {
	  	net_log_comment("Reverting connection to proxy (pong timeout).");
		resetProxy(to_player); 
	}

	if(connection_statuses[to_player].type == CONNT_DIRECT) {
		force_direct = 1; 
	}

	memset(&buf, 0, UPID_P2P_PING_SIZE);

	buf[len] = UPID_P2P_PING; len++; 
	PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
	buf[len] = Player_num; len++;
	memcpy(buf + len, &time, 8); len += 8;
	buf[len] = force_direct; len++; 
	buf[len] = Netgame.players[to_player].loss; len++; 

	if(force_direct) {
		net_udp_send_to_player_direct(buf, sizeof(buf), to_player); 
	} else {
		net_udp_send_to_player_proxy(buf, sizeof(buf), to_player, connection_statuses[to_player].proxy_through); 
	}

}

char* ip_from_sockaddr(struct _sockaddr addr) {
	struct sockaddr_in *addrin = (struct sockaddr_in*) &addr;
	return inet_ntoa(addrin->sin_addr); 
}

ushort port_from_sockaddr(struct _sockaddr addr) {
	struct sockaddr_in *addrin = (struct sockaddr_in*) &addr;
	return SWAPSHORT(addrin->sin_port);
}


void net_udp_process_p2p_ping(ubyte *data, struct _sockaddr sender_addr, int data_len) {
	int len = 0;
	len++; // Skip packet id
	len += 4; // token
	int from_player = data[len]; len++; 
	fix64 time;
	memcpy(&time, data + len, 8); len += 8; 
	int direct_ping = data[len]; len++;
	Netgame.players[from_player].rx_loss = data[len]; len++; 

	// This is an observer heartbeat	

	if(Netgame.max_numobservers > 0 && from_player == OBSERVER_PLAYER_ID && multi_i_am_master()) {
		for(int i = 0; i < Netgame.max_numobservers; i++) {
			if (Netgame.observers[i].connected == 1) {
				if(! memcmp(&Netgame.observers[i].protocol.udp.addr, &sender_addr, sizeof(struct _sockaddr))) {
					Netgame.observers[i].LastPacketTime = timer_query();
					if(i == 0 && multi_i_am_master()) {
						multi_send_obs_update(1, 0); 
					}
					return;
				}
			}
		}
		return;
	}

	// Since the host doesn't send packets as an observer, ensure that clients don't disconnect thinking the host is dead.
	if (from_player == 0 && Netgame.host_is_obs) {
		Netgame.players[0].LastPacketTime = timer_query();
	}
	
	// If I can hear a direct ping, I can probably reply
	if(direct_ping) {
		// Don't update master, non-existent player, or me
		if( (from_player == multi_who_is_master()) || 
			(from_player > MAX_PLAYERS) || 
			(from_player == Player_num)) {

			char log_comment[100];
			snprintf(log_comment, 100, "Cannot update address -- illegal player num %d (==%d, >%d, == %d)", from_player,
				multi_who_is_master(), MAX_PLAYERS, Player_num); 
			net_log_comment(log_comment); 
		} else {
			update_address_for_player(from_player, sender_addr);
		}

		// Restablish direct attempt, if we aren't already doing that
		if(connection_statuses[from_player].type == CONNT_PROXY) {
			char comment[100];
			sprintf(comment, "Received direct ping from proxy player %d connection status %d", from_player, Netgame.players[from_player].connected); 
			net_log_comment(comment); 

			net_log_comment("Received direct ping on proxy connection, reattempting direct.");
			reattemptDirect(from_player); 
		}
		
	}	


	net_udp_send_p2p_pong(from_player, time, direct_ping); 
}


void net_udp_send_p2p_pong (int to_player, fix64 time, int direct_ping) {
	ubyte buf[UPID_P2P_PONG_SIZE]; 
	int len = 0;

	memset(&buf, 0, UPID_P2P_PONG_SIZE);

	buf[len] = UPID_P2P_PONG; len++; 
	PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
	buf[len] = Player_num; len++;
	memcpy(buf + len, &time, 8); len += 8;
	buf[len] = direct_ping; len++; 

	if(direct_ping) {
		net_udp_send_to_player_direct(buf, sizeof(buf), to_player); 
	} else {
		net_udp_send_to_player(buf, sizeof(buf), to_player); 
	}

}

void net_udp_process_p2p_pong(ubyte *data, struct _sockaddr sender_addr, int data_len) {
	int len = 1; // Skip pid
	len += 4; // token 
	int from_player = data[len]; len++;
	fix64 sent_time;
	memcpy(&sent_time, data + len, 8); len += 8;
	int direct_pong = data[len]; len++;

	// Get the ping time
	Netgame.players[from_player].ping = f2i(fixmul(timer_query() - sent_time,i2f(1000)));
	
	if (Netgame.players[from_player].ping < 0)
		Netgame.players[from_player].ping = 0;
		
	if (Netgame.players[from_player].ping > 9999)
		Netgame.players[from_player].ping = 9999;

	// Don't update master, non-existent player, or me
	if(from_player < 1 || from_player > MAX_PLAYERS || from_player == Player_num) {
		return;
	}

	// If this was direct, update the address!	
	if(direct_pong) {
		if(connection_statuses[from_player].type != CONNT_DIRECT) {
			net_log_comment("Received direct pong, connection upgraded to direct.");
		}

		connection_statuses[from_player].last_direct_pong = timer_query(); 
		connection_statuses[from_player].type = CONNT_DIRECT; 		

	    if(memcmp(&Netgame.players[from_player].protocol.udp.addr, &sender_addr, sizeof(struct _sockaddr))) {
			update_address_for_player(from_player, sender_addr);
		}
	}	
}


void update_address_for_player(int pnum, struct _sockaddr new_addr) {
	if(!multi_i_am_master() && is_observer()) { return; }

	char logcomment[200]; 
	snprintf(logcomment, 200, "Requested update to address for player %d to %s:%u\n", pnum, 
		ip_from_sockaddr(new_addr), port_from_sockaddr(new_addr)); 
	net_log_comment(logcomment); 

	if( (port_from_sockaddr(new_addr) != 0) && // Not null
	   memcmp(&new_addr,  &Netgame.players[pnum].protocol.udp.addr, sizeof(struct _sockaddr))    // New address

	) {	
		char logentry[200];
		snprintf(logentry, 200, "   Updating IP address for player %d from %s:%u", 
			pnum, ip_from_sockaddr(Netgame.players[pnum].protocol.udp.addr), port_from_sockaddr(Netgame.players[pnum].protocol.udp.addr)); 
		snprintf(logentry, 200, "   to %s:%u", 
			ip_from_sockaddr(new_addr), port_from_sockaddr(new_addr)); 		
		
		net_log_comment(logentry); 

		memcpy(&Netgame.players[pnum].protocol.udp.addr, &new_addr, sizeof(struct _sockaddr)); 
	} else {
		net_log_comment("   IP address not updated (old or null)."); 
	}
}

void resetProxy(int pnum) {
	if(pnum == multi_who_is_master()) return;
	if(multi_i_am_master()) return;

	connection_statuses[pnum].type = CONNT_PROXY;
	connection_statuses[pnum].proxy_through = 0;

}

void reattemptDirect(int pnum) {
	char comment_string[100];
	sprintf(comment_string, "reattemptDirect: %d", pnum); 
	net_log_comment(comment_string); 

	if(pnum == multi_who_is_master()) return;
	if(multi_i_am_master()) return;

	net_log_comment("   (reset)"); 

	connection_statuses[pnum].holepunch_attempts = 0;
	connection_statuses[pnum].last_direct_pong = timer_query(); 
	
}

void net_udp_send_to_player(ubyte* data, int len, int to_player) {
	if(connection_statuses[to_player].type == CONNT_DIRECT) {
		net_udp_send_to_player_direct(data, len, to_player); 
	} else if (connection_statuses[to_player].type == CONNT_PROXY) {
		net_udp_send_to_player_proxy(data, len, to_player, connection_statuses[to_player].proxy_through); 
	} else {
		// Shouldn't happen, default to host
		net_udp_send_to_player_proxy(data, len, to_player, 0); 
	}
}

void net_udp_send_to_player_direct(ubyte* data, int len, int to_player) {
	dxx_sendto(UDP_Socket[0], data, len, 0, (struct sockaddr *)&Netgame.players[to_player].protocol.udp.addr, sizeof(struct _sockaddr));
}

void net_udp_send_to_player_proxy(ubyte* data, int data_len, int to_player, int through_player) {
	// Only proxy through direct connections; drop the packet if we try something else
	if(connection_statuses[through_player].type != CONNT_DIRECT) { return; }

	ubyte* buf;
	int len = 0;
	buf = malloc(data_len + UPID_PROXY_HEADER_SIZE);

	buf[len] = UPID_PROXY; len++; 
	PUT_INTEL_INT(buf + len, netgame_token); len += 4; 
	buf[len] = to_player; len++;
	buf[len] = Player_num; len++; 
	memcpy(buf + len, data, data_len); len += data_len; 

	dxx_sendto(UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[through_player].protocol.udp.addr, sizeof(struct _sockaddr));
	free(buf);
}

void net_udp_process_proxy(ubyte* data, struct _sockaddr sender_addr, int data_len) {
	int from_player = data[6];
	if (from_player < 0 || from_player > MAX_PLAYERS - 1) {
		drop_rx_packet(data, "from invalid player");
		return;
	}

	if (!is_any_player_ip(sender_addr)) {
	 	drop_rx_packet(data, "from non-player ip"); 
	 	return; 
	}

	int to_player = data[5];
	//int from_player = data[2];

	if(to_player == Player_num) {
		// For me?? :)
		ubyte* contents = data + UPID_PROXY_HEADER_SIZE; 

		net_udp_process_packet(contents, sender_addr, data_len - UPID_PROXY_HEADER_SIZE, 1 );

	} else {
		// Oh, we're forwarding.  
		// Ok, but it must be to a player I have a direct connection to
		if(connection_statuses[to_player].type != CONNT_DIRECT) {
			drop_rx_packet(data, "proxy to non-direct player"); 
			return; // just drop the packet.  He'll figure it out.
		}

		// Off you go, little packet		
		net_udp_send_to_player_direct(data, data_len, to_player); 


		// How long has it been since you two spoke?
		fix64 last_attempt = last_direct_attempt[from_player][to_player];
		if(timer_query() - last_attempt > F1_0 * 15) {
			net_udp_send_p2p_reattempt_direct(from_player, to_player);
			net_udp_send_p2p_reattempt_direct(to_player, from_player);

			last_direct_attempt[from_player][to_player] = timer_query();
			last_direct_attempt[to_player][from_player] = timer_query();
		}
	}
}


void net_udp_p2p_ping_frame(fix64 time)
{
	static fix64 lastPing[8] = {0,0,0,0,0,0,0,0};
	fix64 pingTimeSetup = F1_0/10;
	fix64 pingTimeHeartbeat = F1_0;

	for(int i = 0; i < MAX_PLAYERS; i++) {
		if(i == Player_num) continue; 
		if(! Players[i].connected) continue; 

		if(is_observer() && !multi_i_am_master() && i > 0) { return; }

		int sentping = 0; 

		// Initiate punchthrough if that's what we're trying to do
		if( connection_statuses[i].holepunch_attempts < MAX_HOLEPUNCH_ATTEMPTS) {

			if(time > lastPing[i] + pingTimeSetup) {	
				net_udp_send_p2p_ping(i, 1, time); 
				sentping = 1; 
				connection_statuses[i].holepunch_attempts++; 
			}
		}

		// Connection measurement / heartbeat ping
		if(time > lastPing[i] + pingTimeHeartbeat) {	
			net_udp_send_p2p_ping(i, 0, time); 
			sentping = 1; 
		}

		if(sentping) {			
			lastPing[i] = time;
		}

	}

}

// Send the ping list in regular intervals
void net_udp_ping_frame(fix64 time)
{
	static fix64 PingTime = 0;
	
	if ((PingTime + F1_0) < time)
	{
		ubyte buf[UPID_PING_SIZE];
		int len = 0, i = 0;
		
		memset(&buf, 0, sizeof(ubyte)*UPID_PING_SIZE);
		buf[len] = UPID_PING;							len++;
		memcpy(&buf[len], &time, 8);						len += 8;
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_INT(buf + len, Netgame.players[i].ping);		len += 4;
		}
		
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
		}
		PingTime = time;
	}
}

// Got a PING from host. Apply the pings to our players and respond to host.
void net_udp_process_ping(ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	fix64 host_ping_time = 0;
	ubyte buf[UPID_PONG_SIZE];
	int i, len = 0;
// 
	if (memcmp((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr)))
		return;

										len++; // Skip UPID byte;
	memcpy(&host_ping_time, &data[len], 8);					len += 8;
	for (i = 1; i < MAX_PLAYERS; i++)
	{
		Netgame.players[i].ping = GET_INTEL_INT(&(data[len]));		len += 4;
	}

	// Since the host doesn't send packets as an observer, ensure that clients don't disconnect thinking the host is dead.
	if (Netgame.host_is_obs) {
		Netgame.players[0].LastPacketTime = timer_query();
	}
	
	buf[0] = UPID_PONG;
	buf[1] = Player_num;
	memcpy(&buf[2], &host_ping_time, 8);
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
}

// Got a PONG from a client. Check the time and add it to our players.
void net_udp_process_pong(ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	fix64 client_pong_time = 0;
	int i = 0;
	
	if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[data[1]].protocol.udp.addr, sizeof(struct _sockaddr)))
		return;

	if (data[1] >= MAX_PLAYERS || data[1] < 1)
		return;

	if (i == MAX_PLAYERS)
		return;
	
	memcpy(&client_pong_time, &data[2], 8);
	Netgame.players[data[1]].ping = f2i(fixmul(timer_query() - client_pong_time,i2f(1000)));
	
	if (Netgame.players[data[1]].ping < 0)
		Netgame.players[data[1]].ping = 0;
		
	if (Netgame.players[data[1]].ping > 9999)
		Netgame.players[data[1]].ping = 9999;
}

void net_udp_do_refuse_stuff (UDP_sequence_packet *their)
{
	int i,new_player_num;

	ClipRank (&their->player.rank);
	
	if(their->player.observer) {
		if(Netgame.numobservers < Netgame.max_numobservers) {
			net_udp_welcome_player(their);
		} else {
			net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_FULL);
		}
		return;
	}

	for (i=0;i<MAX_PLAYERS;i++)
	{
		if ((!d_stricmp(Players[i].callsign, their->player.callsign )) && !memcmp((struct _sockaddr *)&their->player.protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			net_udp_welcome_player(their);
			return;
		}
	}

	if (!WaitForRefuseAnswer)
	{
		int activeplayers = 0; 
		for (i = 0; i < Netgame.numplayers; i++)
			if (Netgame.players[i].connected)
				activeplayers++;

		if(activeplayers < Netgame.max_numplayers) {

			for (i=0;i<MAX_PLAYERS;i++)
			{
				if ((!d_stricmp(Players[i].callsign, their->player.callsign )) && !memcmp((struct _sockaddr *)&their->player.protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
				{
					net_udp_welcome_player(their);
					return;
				}
			}
		
			digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
		
			if (Game_mode & GM_TEAM)
			{
				if (!PlayerCfg.NoRankings)
				{
					HUD_init_message(HM_MULTI, "%s %s wants to join",RankStrings[their->player.rank],their->player.callsign);
				}
				else
				{
					HUD_init_message(HM_MULTI, "%s wants to join",their->player.callsign);
				}
				HUD_init_message(HM_MULTI, "Alt-1 assigns to team %s. Alt-2 to team %s",Netgame.team_name[0],Netgame.team_name[1]);
			}
			else
			{
				HUD_init_message(HM_MULTI, "%s wants to join (accept: F6)",their->player.callsign);
			}
		
			strcpy (RefusePlayerName,their->player.callsign);
			RefuseTimeLimit=timer_query();
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=1;
		} else {			
			net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_FULL);
		}
	}
	else
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if ((!d_stricmp(Players[i].callsign, their->player.callsign )) && !memcmp((struct _sockaddr *)&their->player.protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
			{
				net_udp_welcome_player(their);
				return;
			}
		}
	
		if (strcmp(their->player.callsign,RefusePlayerName))
			return;
	
		if (RefuseThisPlayer)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (Game_mode & GM_TEAM)
			{
				new_player_num=net_udp_get_new_player_num (their);
	
				Assert (RefuseTeam==1 || RefuseTeam==2);        
			
				if (RefuseTeam==1)      
					Netgame.team_vector &=(~(1<<new_player_num));
				else
					Netgame.team_vector |=(1<<new_player_num);
				net_udp_welcome_player(their);
				net_udp_send_netgame_update();
			}
			else
			{
				net_udp_welcome_player(their);
			}
			return;
		}

		if ((timer_query()) > RefuseTimeLimit+REFUSE_INTERVAL)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (!strcmp (their->player.callsign,RefusePlayerName))
			{
				net_udp_dump_player(their->player.protocol.udp.addr, their->token, DUMP_DORK);
			}
			return;
		}
	}
}

int net_udp_get_new_player_num (UDP_sequence_packet *their)
  {
	 int i;
	
	 their=their;
	
		if ( N_players < Netgame.max_numplayers)
			return (N_players);
		
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix64 oldest_time = timer_query();

			Assert(N_players == Netgame.max_numplayers);

			for (i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (Netgame.players[i].LastPacketTime < oldest_time))
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}
	    return (oldest_player);
	  }
  }

void net_udp_send_extras ()
{
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	Assert (Player_joining_extras>-1);

	if (Network_sending_extras==3 && (Netgame.PlayTimeAllowed || Netgame.KillGoal))
		multi_send_kill_goal_counts();
	if (Network_sending_extras==2)
		multi_send_powcap_update();
	if (Network_sending_extras==1 && Game_mode & GM_BOUNTY)
		multi_send_bounty();

	Network_sending_extras--;
	if (!Network_sending_extras)
		Player_joining_extras=-1;
}


void net_udp_send_obs_quit()
{
	ubyte buf[UPID_OBSQUIT_SIZE];
	int len = 0;

	memset(&buf, 0, UPID_OBSQUIT_SIZE);

	buf[len] = UPID_OBSQUIT; len++;
	PUT_INTEL_INT(buf + len, netgame_token); len += 4;
	PUT_INTEL_INT(buf + len, my_player_token); len += 4;

	Assert(len == sizeof(buf));

	net_udp_send_to_player_direct(buf, sizeof(buf), 0);
}

void net_udp_process_obs_quit(ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	int obsnum = -1;
	for(int i = 0; i < Netgame.max_numobservers; i++) {
		if (is_same_addr(&Netgame.observers[i].protocol.udp.addr, &sender_addr)) {
			obsnum = i;
		}
	}
	if (obsnum == -1) {
		drop_rx_packet(data, "not received from any observer ip");
		return;
	}
	memset(&Netgame.observers[obsnum], 0, sizeof(netplayer_info));
	Netgame.numobservers--;
	multi_send_obs_update(1, 0);
}

#endif
