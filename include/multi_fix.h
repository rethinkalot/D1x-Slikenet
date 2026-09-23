#ifndef MULTI_FIX_H
#define MULTI_FIX_H

#ifndef SONG_EXT_MID
#define SONG_EXT_MID ".mid"
#endif
#ifndef SONG_EXT_OGG
#define SONG_EXT_OGG ".ogg"
#endif
#ifndef SONG_EXT_FLAC
#define SONG_EXT_FLAC ".flac"
#endif
#ifndef SONG_EXT_MP3
#define SONG_EXT_MP3 ".mp3"
#endif

#ifndef MAX_ROBOTS_CONTROLLED
#define MAX_ROBOTS_CONTROLLED 5
#endif

// Define the full layout for struct _sockaddr so net_udp.h has a complete type tracking size
struct _sockaddr {
    unsigned short sin_family;
    unsigned short sin_port;
    unsigned long  sin_addr;
    unsigned char  sin_zero[8];
};

#ifdef __cplusplus
extern "C" {
#endif
extern int robot_controlled[MAX_ROBOTS_CONTROLLED];
extern int robot_agitation[MAX_ROBOTS_CONTROLLED];
extern int robot_fired[MAX_ROBOTS_CONTROLLED];
#ifdef __cplusplus
}
#endif

struct object;

#ifdef __cplusplus
extern "C" {
#endif
void reset_respawnable_bots(void);
void observer_show_kill_list(void);
void hud_show_kill_list(void);
void maybe_show_observers(int y);
void check_robot_respawns(void);

void multi_do_boss_actions(unsigned char *buf);
void multi_do_create_robot_powerups(unsigned char *buf);
void multi_strip_robots(int playernum);
void multi_check_robot_timeout(void);
void multi_do_claim_robot(unsigned char *buf);
void multi_do_robot_position(unsigned char *buf);
void multi_do_robot_explode(unsigned char *buf);
void multi_do_release_robot(unsigned char *buf);
void multi_do_robot_fire(unsigned char *buf);
void multi_do_respawn_robot(unsigned char *buf);
void multi_do_create_robot(unsigned char *buf);

void net_udp_send_obs_quit(void);
#ifdef __cplusplus
}
#endif

#endif
