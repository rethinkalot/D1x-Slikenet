/*
 * UDP game setup, advanced options, and preset UI.
 */

#ifdef NETWORK

#include "net_udp_internal.h"
#include "pstypes.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "laser.h"
#include "player.h"
#include "endlevel.h"
#include "palette.h"
#include "cntrlcen.h"
#include "menu.h"
#include "sounds.h"
#include "kmatrix.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "rbaudio.h"
#include "vers_id.h"

#define PRESET_EXT ".ngs"
#define GAME_PARAM_CHOICE_SHOW_AGAIN -3
#define OBSERVER_DELAY 15

char UDP_MyPort[6] = "";

/* Port to fall back on. -udp_myport from the command line or d1x.ini takes
 * precedence over the built in UDP_PORT_DEFAULT. */
static void net_udp_my_port_default(void)
{
	if (GameArg.MplUdpMyPort != 0)
		snprintf(UDP_MyPort, sizeof(UDP_MyPort), "%d", GameArg.MplUdpMyPort);
	else
		snprintf(UDP_MyPort, sizeof(UDP_MyPort), "%d", UDP_PORT_DEFAULT);
}

/* Seed UDP_MyPort the first time it is needed. Deliberately not re-seeded on
 * every menu entry, so that a port typed in game is not silently discarded
 * and reverted to the default. */
static void net_udp_my_port_init(void)
{
	if (UDP_MyPort[0] == '\0')
		net_udp_my_port_default();
}

int load_preset(newmenu *menu_settings);
void save_preset(void);



static int opt_cinvul, opt_show_on_map;
static int opt_show_on_map, opt_difficulty, opt_setpower, opt_playtime, opt_killgoal, opt_port, opt_packets, opt_shortpack, opt_show_names, opt_bright, opt_ffire, opt_retroproto, opt_respawnconcs, opt_allowcolor, opt_faircolors, opt_blackwhite;
static int opt_primary_dup, opt_secondary_dup, opt_secondary_cap; 
static int opt_spawn_no_invul, opt_spawn_short_invul, opt_spawn_long_invul, opt_spawn_preview; 
static int opt_spawn_algorithm;
//static int opt_dark_smarts;
static int opt_allowprefcolor; 
static int opt_low_vulcan;
static int opt_homing_update_rate;
static int opt_remote_hit_spark;
static int opt_allow_custom_models_textures;
static int opt_reduced_flash;
static int opt_gauss_duplicating, opt_gauss_depleting, opt_gauss_steady_recharge, opt_gauss_steady_respawn;

#ifdef USE_TRACKER
static int opt_tracker;
#endif

void net_udp_set_power (void)
{
	newmenu_item m[MULTI_ALLOW_POWERUP_MAX];
	int i;

	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
	{
		m[i].type = NM_TYPE_CHECK; m[i].text = multi_allow_powerup_text[i]; m[i].value = (Netgame.AllowedItems >> i) & 1;
	}

	newmenu_do1( NULL, "Objects to allow", MULTI_ALLOW_POWERUP_MAX, m, NULL, NULL, 0 );

	Netgame.AllowedItems &= ~NETFLAG_DOPOWERUP;
	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
		if (m[i].value)
			Netgame.AllowedItems |= (1 << i);
}

int net_udp_more_options_handler( newmenu *menu, d_event *event, void *userdata );

void net_udp_more_game_options ()
{
	int opt=0,i=0;
	char PlayText[80],KillText[80],srinvul[50],packstring[5];
	char PrimDupText[80],SecDupText[80],SecCapText[80]; 
	char HomingUpdateRateText[80];
#ifdef USE_TRACKER
	newmenu_item m[45];
#else
	newmenu_item m[44];
#endif

	snprintf(packstring,sizeof(char)*4,"%d",Netgame.PacketsPerSec);
	
	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.difficulty; m[opt].text=TXT_DIFFICULTY; m[opt].min_value=0; m[opt].max_value=(NDL-1); opt++;

	opt_cinvul = opt;
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, Netgame.control_invul_time/F1_0/60, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.control_invul_time/5/F1_0/60; m[opt].text= srinvul; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_playtime=opt;
	sprintf( PlayText, "Max time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.PlayTimeAllowed; m[opt].text= PlayText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_killgoal=opt;
	sprintf( KillText, "Kill Goal: %d kills", Netgame.KillGoal*10);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.KillGoal; m[opt].text= KillText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_primary_dup=opt;
	char xp[5];
	sprintf(xp, "x%d", Netgame.PrimaryDupFactor); 
	sprintf( PrimDupText, "Extra Primaries: %s", Netgame.PrimaryDupFactor < 2 ? "None" : xp);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.PrimaryDupFactor - 1; m[opt].text= PrimDupText; m[opt].min_value=0; m[opt].max_value=7; opt++;

	opt_secondary_dup=opt;
	sprintf(xp, "x%d", Netgame.SecondaryDupFactor); 
	sprintf( SecDupText, "Extra Secondaries: %s", Netgame.SecondaryDupFactor < 2 ? "None" : xp);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.SecondaryDupFactor - 1; m[opt].text= SecDupText; m[opt].min_value=0; m[opt].max_value=7; opt++;

	opt_secondary_cap=opt;
	sprintf( SecCapText, "Cap Secondaries: %s", Netgame.SecondaryCapFactor == 0 ? "Uncapped" : (Netgame.SecondaryCapFactor == 1 ? "Max Six" : "Max Two"));
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.SecondaryCapFactor; m[opt].text= SecCapText; m[opt].min_value=0; m[opt].max_value=2; opt++;

	opt_low_vulcan = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Low Vulcan Ammo"; m[opt].value = Netgame.LowVulcan; opt++;	


	opt_setpower = opt;
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Set Objects allowed..."; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Spawn Style"; opt++;
	opt_spawn_no_invul = opt; 
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "No Invuln"; m[opt].value = Netgame.SpawnStyle == SPAWN_STYLE_NO_INVUL; m[opt].group = 0; opt++;
	opt_spawn_short_invul = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Half Second Invuln"; m[opt].value = Netgame.SpawnStyle == SPAWN_STYLE_SHORT_INVUL; m[opt].group = 0; opt++;
	opt_spawn_long_invul = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Two Second Invuln"; m[opt].value = Netgame.SpawnStyle == SPAWN_STYLE_LONG_INVUL; m[opt].group = 0; opt++;
	opt_spawn_preview = opt; 
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Preview"; m[opt].value = Netgame.SpawnStyle == SPAWN_STYLE_PREVIEW; m[opt].group = 0; opt++;
	opt_spawn_algorithm = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Use New Spawn Location Algorithm"; m[opt].value = Netgame.NewSpawnAlgorithm; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Vulcan Ammo Style"; opt++;
	opt_gauss_duplicating = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Duplicating (D2)"; m[opt].value = Netgame.GaussAmmoStyle == GAUSS_STYLE_DUPLICATING; m[opt].group = 1; opt++;
	opt_gauss_depleting = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Original (Depleting)"; m[opt].value = Netgame.GaussAmmoStyle == GAUSS_STYLE_DEPLETING; m[opt].group = 1; opt++;
	opt_gauss_steady_recharge = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Dropping Picked Up"; m[opt].value = Netgame.GaussAmmoStyle == GAUSS_STYLE_STEADY_RECHARGING; m[opt].group = 1; opt++;
	opt_gauss_steady_respawn = opt;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Respawning"; m[opt].value = Netgame.GaussAmmoStyle == GAUSS_STYLE_STEADY_RESPAWNING; m[opt].group = 1; opt++;


	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;

	opt_respawnconcs = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Respawn Concussions"; m[opt].value = Netgame.RespawnConcs; opt++;	

	opt_faircolors = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "All Players Blue"; m[opt].value = Netgame.FairColors; opt++;		

	opt_allowcolor = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Allow Colored Dynamic Lighting"; m[opt].value = Netgame.AllowColoredLighting; opt++;	

	opt_allowprefcolor = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Allow Players To Choose Their Colors"; m[opt].value = Netgame.AllowPreferredColors; opt++;	


	//opt_dark_smarts = opt;
	//m[opt].type = NM_TYPE_CHECK; m[opt].text = "Dark Smart Blobs"; m[opt].value = Netgame.DarkSmartBlobs; opt++;	



	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Packets per second (2 - 40)"; opt++;
	opt_packets=opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text=packstring; m[opt].text_len=2; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Network port"; opt++;
	opt_port = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = UDP_MyPort; m[opt].text_len=5; opt++;

#ifdef USE_TRACKER
	opt_tracker = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Track this game"; m[opt].value = Netgame.Tracker; opt++;
#endif

	opt_retroproto = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Retro Protocol (p2p, etc.)"; m[opt].value = Netgame.RetroProtocol; opt++;


	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;	


	opt_show_on_map=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP); opt_show_on_map=opt; opt++;

	opt_bright = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Bright player ships"; m[opt].value=Netgame.BrightPlayers; opt++;

	opt_show_names=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Show enemy names on HUD"; m[opt].value=Netgame.ShowEnemyNames; opt++;

	opt_ffire=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "No friendly fire (Team, Coop)"; m[opt].value=Netgame.NoFriendlyFire; opt++;


	//opt_shortpack=opt;
	//m[opt].type = NM_TYPE_CHECK; m[opt].text = "Short Packets (saves traffic)"; m[opt].value = Netgame.ShortPackets; opt++;

	opt_blackwhite = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Alternate Colors (Ships 6 and 7)"; m[opt].value = Netgame.BlackAndWhitePyros; opt++;	

	opt_homing_update_rate=opt;
	sprintf( HomingUpdateRateText, "Homing Update Rate: %d", Netgame.HomingUpdateRate);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=max(0, Netgame.HomingUpdateRate - 20); m[opt].text= HomingUpdateRateText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_remote_hit_spark=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Only Show Confirmed Hit Sparks"; m[opt].value = Netgame.RemoteHitSpark; opt++;

	opt_allow_custom_models_textures=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Allow custom models and textures"; m[opt].value = Netgame.AllowCustomModelsTextures; opt++;

	opt_reduced_flash=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Reduced flash effects"; m[opt].value = Netgame.ReducedFlash; opt++;

	Assert(opt <= SDL_arraysize(m));

menu:
	i = newmenu_do1( NULL, "Advanced netgame options", opt, m, net_udp_more_options_handler, NULL, 0 );

	Netgame.control_invul_time = m[opt_cinvul].value*5*F1_0*60;

	if (i==opt_setpower)
	{
		net_udp_set_power ();
		goto menu;
	}

	Netgame.PacketsPerSec=atoi(packstring);
	
	if (Netgame.PacketsPerSec>40)
	{
		Netgame.PacketsPerSec=40;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 40");
	}

	if (Netgame.PacketsPerSec<10)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 10");
		Netgame.PacketsPerSec=10;
	}
	Netgame.ShortPackets=m[opt_shortpack].value;

	if ((atoi(UDP_MyPort)) <= 0 ||(atoi(UDP_MyPort)) > 65535)
	{
		net_udp_my_port_default();
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal port");
	}

	Netgame.BrightPlayers=m[opt_bright].value;
	Netgame.ShowEnemyNames=m[opt_show_names].value;
	Netgame.difficulty=Difficulty_level = m[opt_difficulty].value;
	if (m[opt_show_on_map].value)
		Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
	else
		Netgame.game_flags &= ~NETGAME_FLAG_SHOW_MAP;
	Netgame.NoFriendlyFire = m[opt_ffire].value;
#ifdef USE_TRACKER
	Netgame.Tracker = m[opt_tracker].value;
#endif

	Netgame.RetroProtocol = m[opt_retroproto].value;
	Netgame.RespawnConcs  = m[opt_respawnconcs].value;
	Netgame.AllowColoredLighting  = m[opt_allowcolor].value;
	Netgame.FairColors  = m[opt_faircolors].value;
	Netgame.BlackAndWhitePyros  = m[opt_blackwhite].value;
	//Netgame.DarkSmartBlobs = m[opt_dark_smarts].value;
	Netgame.LowVulcan = m[opt_low_vulcan].value;
	Netgame.AllowPreferredColors = m[opt_allowprefcolor].value;
	Netgame.HomingUpdateRate = m[opt_homing_update_rate].value + 20;
	Netgame.RemoteHitSpark = m[opt_remote_hit_spark].value;
	Netgame.AllowCustomModelsTextures = m[opt_allow_custom_models_textures].value;
	Netgame.ReducedFlash = m[opt_reduced_flash].value;
	Netgame.NewSpawnAlgorithm = m[opt_spawn_algorithm].value;
}


int net_udp_more_options_handler( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	
	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt_cinvul)
				sprintf( menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value*5, TXT_MINUTES_ABBREV );
			else if (citem == opt_playtime)
			{
				if (Game_mode & GM_MULTI_COOP)
				{
					nm_messagebox ("Sorry",1,TXT_OK,"You can't change those for coop!");
					menus[opt_playtime].value=0;
					return 0;
				}
				
				Netgame.PlayTimeAllowed=menus[opt_playtime].value;
				sprintf( menus[opt_playtime].text, "Max Time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
			}
			else if (citem == opt_killgoal)
			{
				if (Game_mode & GM_MULTI_COOP)
				{
					nm_messagebox ("Sorry",1,TXT_OK,"You can't change those for coop!");
					menus[opt_killgoal].value=0;
					return 0;
				}
				
				Netgame.KillGoal=menus[opt_killgoal].value;
				sprintf( menus[opt_killgoal].text, "Kill Goal: %d kills", Netgame.KillGoal*10);
			}
			else if (citem == opt_primary_dup)
			{
				
				Netgame.PrimaryDupFactor=menus[opt_primary_dup].value + 1;
				char xp[5];
				sprintf(xp, "x%d", Netgame.PrimaryDupFactor); 
				sprintf( menus[opt_primary_dup].text, "Extra Primaries: %s", Netgame.PrimaryDupFactor == 1 ? "None" : xp);

			}
			else if (citem == opt_secondary_dup)
			{
				char xp[5];
				Netgame.SecondaryDupFactor=menus[opt_secondary_dup].value + 1;
				sprintf(xp, "x%d", Netgame.SecondaryDupFactor); 
				sprintf( menus[opt_secondary_dup].text, "Extra Secondaries: %s", Netgame.SecondaryDupFactor == 1 ? "None" : xp);

			}
			else if (citem == opt_secondary_cap)
			{
				
				Netgame.SecondaryCapFactor=menus[opt_secondary_cap].value;

				sprintf( menus[opt_secondary_cap].text, "Cap Secondaries: %s", Netgame.SecondaryCapFactor == 0 ? "Uncapped" : (Netgame.SecondaryCapFactor == 1 ? "Max Six" : "Max Two"));

			}
			else if (citem == opt_homing_update_rate)
			{
				Netgame.HomingUpdateRate=menus[opt_homing_update_rate].value + 20;
				sprintf( menus[opt_homing_update_rate].text, "Homing Update Rate: %d", Netgame.HomingUpdateRate);
			} else if (citem == opt_spawn_no_invul) {
				Netgame.SpawnStyle = SPAWN_STYLE_NO_INVUL;
			} else if (citem == opt_spawn_short_invul) {
				Netgame.SpawnStyle = SPAWN_STYLE_SHORT_INVUL;
			} else if (citem == opt_spawn_long_invul) {
				Netgame.SpawnStyle = SPAWN_STYLE_LONG_INVUL;
			} else if (citem == opt_spawn_preview) {
				Netgame.SpawnStyle = SPAWN_STYLE_PREVIEW;
			} else if (citem == opt_gauss_duplicating) {
				Netgame.GaussAmmoStyle = GAUSS_STYLE_DUPLICATING;
			}  else if (citem == opt_gauss_depleting) {
				Netgame.GaussAmmoStyle = GAUSS_STYLE_DEPLETING;
			}  else if (citem == opt_gauss_steady_recharge) {
				Netgame.GaussAmmoStyle = GAUSS_STYLE_STEADY_RECHARGING;
			}  else if (citem == opt_gauss_steady_respawn) {
				Netgame.GaussAmmoStyle = GAUSS_STYLE_STEADY_RESPAWNING;
			}

			break;
			
		default:
			break;
	}
	
	userdata = userdata;
	
	return 0;
}

typedef struct param_opt
{
	int start_game, load_preset, save_preset, name, level, mode, mode_end, moreopts;
	int closed, refuse, maxnet, maxobs, obsdelay, obsmin, anarchy, team_anarchy, robot_anarchy, coop, bounty;
} param_opt;

int net_udp_start_game(void);

int net_udp_game_param_handler( newmenu *menu, d_event *event, param_opt *opt )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt->team_anarchy)
			{
				menus[opt->closed].value = 1;
				menus[opt->closed-1].value = 0;
				menus[opt->closed+1].value = 0;
			}
			
			if (menus[opt->coop].value)
			{
				if (menus[opt->maxnet].value>2) 
				{
					menus[opt->maxnet].value=2;
				}
				
				if (menus[opt->maxnet].max_value>2)
				{
					menus[opt->maxnet].max_value=2;
				}
				sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
				Netgame.max_numplayers = menus[opt->maxnet].value+2;
				
				//if (!(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP))
				//	Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;

				if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
				{
					Netgame.PlayTimeAllowed=0;
					Netgame.KillGoal=0;
				}
			}
			else // if !Coop game
			{
				int max_players = 6;
				if(Netgame.max_numobservers > 0) {
					max_players = 5; 
				}
				if (menus[opt->maxnet].max_value<max_players)
				{
					menus[opt->maxnet].value=max_players;
					menus[opt->maxnet].max_value=max_players;
					sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
					Netgame.max_numplayers = menus[opt->maxnet].value+2;
				}
			}
			
			if (citem == opt->level)
			{
				char *slevel = menus[opt->level].text;

				Netgame.levelnum = atoi(slevel);
				
				if (!d_strnicmp(slevel, "s", 1))
					Netgame.levelnum = -atoi(slevel+1);
				else
					Netgame.levelnum = atoi(slevel);
				
// 				if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
// 				{
// 					nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
// 					sprintf(slevel, "1");
// 					return 0;
// 				}
			}
			
			if (citem == opt->maxnet)
			{
				sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
				Netgame.max_numplayers = menus[opt->maxnet].value+2;
			}

			if (citem == opt->maxobs)
			{
				sprintf( menus[opt->maxobs].text, "Maximum observers: %d", menus[opt->maxobs].value*2 );
				Netgame.max_numobservers = menus[opt->maxobs].value*2;

				if(Netgame.max_numobservers > 0) {
					if(menus[opt->maxnet].max_value > 5) {
						menus[opt->maxnet].max_value = 5;
					}

					if(Netgame.max_numplayers > 7) {
						Netgame.max_numplayers = 7; 
					}

					sprintf( menus[opt->maxnet].text, "Maximum players: %d", Netgame.max_numplayers);
				} else {
					if (menus[opt->coop].value) {
						menus[opt->maxnet].max_value = 2;
					} else {
						menus[opt->maxnet].max_value = 6;
					}
				}
			}			

			if ((citem >= opt->mode) && (citem <= opt->mode_end))
			{
				if ( menus[opt->anarchy].value )
					Netgame.gamemode = NETGAME_ANARCHY;
				
				else if (menus[opt->team_anarchy].value) {
					Netgame.gamemode = NETGAME_TEAM_ANARCHY;
				}
// 		 		else if (ANARCHY_ONLY_MISSION) {
// 					int i = 0;
// 		 			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
// 					for (i = opt->mode; i <= opt->mode_end; i++)
// 						menus[i].value = 0;
// 					menus[opt->anarchy].value = 1;
// 		 			return 0;
// 		 		}
				else if ( menus[opt->robot_anarchy].value ) 
					Netgame.gamemode = NETGAME_ROBOT_ANARCHY;
				else if ( menus[opt->coop].value ) 
					Netgame.gamemode = NETGAME_COOPERATIVE;
				else if ( menus[opt->bounty].value )
					Netgame.gamemode = NETGAME_BOUNTY;
				else Int3(); // Invalid mode -- see Rob
			}

			if (menus[opt->closed].value)
				Netgame.game_flags |= NETGAME_FLAG_CLOSED;
			else
				Netgame.game_flags &= ~NETGAME_FLAG_CLOSED;
			Netgame.RefusePlayers=menus[opt->refuse].value;
			Netgame.obs_delay = menus[opt->obsdelay].value;
			Netgame.obs_min = menus[opt->obsmin].value;
			break;
			
		case EVENT_NEWMENU_SELECTED:
			if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
			{
				char *slevel = menus[opt->level].text;
				nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
				sprintf(slevel, "1");
				return 1;
			}

			if (citem==opt->moreopts)
			{
				if ( menus[opt->coop].value )
					Game_mode=GM_MULTI_COOP;
				net_udp_more_game_options();
				Game_mode=0;
				return 1;
			}

			if (citem==opt->start_game)
				return !net_udp_start_game();

			if (citem==opt->load_preset) {
				load_preset(menu);
				return 1;
			}

			if (citem==opt->save_preset) {
				save_preset();
				return 1;
			}

			return 1;
			
		default:
			break;
	}
	
	return 0;
}

void netgame_set_defaults(void)
{
	Netgame.gamemode = 0;
	Netgame.max_numplayers = MAX_PLAYERS;
	Netgame.max_numobservers = 0;
	Netgame.obs_delay = 0;
	Netgame.obs_min = 0;
	Netgame.host_is_obs = 0;
	Netgame.game_flags = 0;
	Netgame.control_invul_time = 0;
	Netgame.KillGoal=0;
	Netgame.PlayTimeAllowed=0;
	Netgame.RefusePlayers=0;
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.PacketsPerSec=20;
	Netgame.ShortPackets=0;
	Netgame.ShowEnemyNames = 0;
	Netgame.BrightPlayers = 1;
	Netgame.SpawnStyle = SPAWN_STYLE_NO_INVUL;
	Netgame.AllowedItems = 0;
	Netgame.AllowedItems |= NETFLAG_DOPOWERUP;
	Netgame.PacketLossPrevention = 1;
	Netgame.NoFriendlyFire = 0;
	Netgame.RetroProtocol = 1;
	Netgame.RespawnConcs = 0;
	Netgame.AllowColoredLighting = 0;
	Netgame.FairColors = 0;
	Netgame.BlackAndWhitePyros = 1;
	Netgame.PrimaryDupFactor = 0;
	Netgame.SecondaryDupFactor = 0;
	Netgame.SecondaryCapFactor = 0;
	Netgame.DarkSmartBlobs = 0;
	Netgame.LowVulcan = 0;
	Netgame.AllowPreferredColors = 1; 
	Netgame.HomingUpdateRate = 25;
	Netgame.RemoteHitSpark = 0;
	Netgame.AllowCustomModelsTextures = 0;
	Netgame.ReducedFlash = 0;
	Netgame.GaussAmmoStyle = GAUSS_STYLE_DEPLETING;
	Netgame.NewSpawnAlgorithm = 0;

#ifdef USE_TRACKER
	Netgame.Tracker = 1;
#endif
}

int load_preset_menu_handler( listbox *lb, d_event *event, void *userdata )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);
	char filename[PATH_MAX];

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			if (event_key_get(event) == KEY_CTRLED+KEY_R) {
				netgame_set_defaults();
				window_close(listbox_get_window(lb));
				newmenu_set_rval((newmenu *)userdata, GAME_PARAM_CHOICE_SHOW_AGAIN);
				window_close(newmenu_get_window((newmenu *)userdata));
				return 0;
			}
			if (event_key_get(event) == KEY_CTRLED+KEY_D && citem >= 0) {
				if (nm_messagebox(NULL, 2, TXT_YES, TXT_NO, "Delete preset %s?", items[citem]) != 0)
					return 1;
				snprintf(filename, sizeof(filename), "%s%s", items[citem], PRESET_EXT);
				if (PHYSFS_delete(filename))
					listbox_delete_item(lb, citem);
				else
					nm_messagebox(NULL, 1, TXT_OK, "Couldn't delete preset %s", items[citem]);
				return 1;
			}
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem < 0)
				return 0;		// shouldn't happen

			netgame_set_defaults();
			snprintf(filename, sizeof(filename), "%s%s", items[citem], PRESET_EXT);
			if (read_netgame_settings_file(filename, &Netgame, 1)) {
				nm_messagebox(NULL, 1, TXT_OK, "Failed to read preset file\n%s", filename);
				return 1;
			}
			newmenu_set_rval((newmenu *)userdata, GAME_PARAM_CHOICE_SHOW_AGAIN);
			window_close(newmenu_get_window((newmenu *)userdata));
			return 0;

		case EVENT_WINDOW_CLOSE:
			PHYSFS_freeList(items);
			break;

		default:
			break;
	}

	return 0;
}

int load_preset(newmenu *menu_settings)
{
	char **list, **newlist, *p;
	static const char *const types[] = { PRESET_EXT, NULL };
	int NumItems;

	list = PHYSFSX_findFiles("", types);
	if (!list)
		return 0;	// memory error

	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {
		if ((p = strrchr(list[NumItems], '.')))
			*p = 0;
	}

	// Sort by name
	qsort(list, NumItems, sizeof(char *), (int (*)( const void *, const void * ))string_array_sort_func);

	newmenu_listbox1("Select preset\nCtrl+D to delete\nCtrl+R for defaults", NumItems, list, 1, 0, load_preset_menu_handler, menu_settings);

	return 1;
}

void save_preset()
{
	static char name[PATH_MAX - 4];
	char filename[PATH_MAX];
	newmenu_item m[1];
	int menu_ret;
	int len;

	m[0].type=NM_TYPE_INPUT; m[0].text_len = sizeof(name) - 1; m[0].text = name;
	menu_ret = newmenu_do( NULL, "Save preset as", 1, m, NULL, NULL );
	if (menu_ret == -1)
		return;

	snprintf(filename, sizeof(filename), "%s%s", name, PRESET_EXT);

	if (PHYSFS_exists(filename) &&
		nm_messagebox(NULL, 2, TXT_YES, TXT_NO, "Preset %s already exists.\nOverwrite?", name) != 0)
		return;

	if (write_netgame_settings_file(filename, &Netgame, 1))
		nm_messagebox(NULL, 1, TXT_OK, "Failed to write preset file\n%s", filename);
}

int net_udp_setup_game()
{
	int i;
	int optnum;
	param_opt opt;
	newmenu_item m[24];
	char slevel[5];
	char level_text[32];
	char srmaxnet[50];
	char srmaxobs[50];
	char srbdelay[50];
	int numplayers_limit;
	int choice;

	net_udp_init();

	net_udp_reset_connection_statuses();

	change_playernum_to(0);

	for (i=0;i<MAX_PLAYERS;i++)
		if (i!=Player_num)
			Players[i].callsign[0]=0;

	sprintf( Netgame.game_name, "%s%s", Players[Player_num].callsign, TXT_S_GAME );
	net_udp_my_port_init();

	netgame_set_defaults();

	read_netgame_profile(&Netgame);

	if (Netgame.gamemode == NETGAME_COOPERATIVE) // did we restore Coop as default? then fix max players right now!
		Netgame.max_numplayers = 4;

	strcpy(Netgame.mission_name, Current_mission_filename);
	strcpy(Netgame.mission_title, Current_mission_longname);

	sprintf( slevel, "1" ); Netgame.levelnum = 1;

	choice = 0;
	for (;;) {
		optnum = 0;
		opt.start_game=optnum;
		m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Start Game"; optnum++;

		opt.load_preset=optnum;
		m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Load Preset"; optnum++;

		opt.save_preset=optnum;
		m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Save Preset"; optnum++;

		m[optnum].type = NM_TYPE_TEXT; m[optnum].text = TXT_DESCRIPTION; optnum++;

		opt.name = optnum;
		m[optnum].type = NM_TYPE_INPUT; m[optnum].text = Netgame.game_name; m[optnum].text_len = NETGAME_NAME_LEN; optnum++;

		sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
		if (Last_secret_level < -1)
			sprintf(level_text+strlen(level_text)-1, ", S1-S%d)", -Last_secret_level);
		else if (Last_secret_level == -1)
			sprintf(level_text+strlen(level_text)-1, ", S1)");

		Assert(strlen(level_text) < 32);

		m[optnum].type = NM_TYPE_TEXT; m[optnum].text = level_text; optnum++;

		opt.level = optnum;
		m[optnum].type = NM_TYPE_INPUT; m[optnum].text = slevel; m[optnum].text_len=4; optnum++;
		m[optnum].type = NM_TYPE_TEXT; m[optnum].text = TXT_OPTIONS; optnum++;

		opt.mode = optnum;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_ANARCHY; m[optnum].value=(Netgame.gamemode == NETGAME_ANARCHY); m[optnum].group=0; opt.anarchy=optnum; optnum++;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_TEAM_ANARCHY; m[optnum].value=(Netgame.gamemode == NETGAME_TEAM_ANARCHY); m[optnum].group=0; opt.team_anarchy=optnum; optnum++;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_ANARCHY_W_ROBOTS; m[optnum].value=(Netgame.gamemode == NETGAME_ROBOT_ANARCHY); m[optnum].group=0; opt.robot_anarchy=optnum; optnum++;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_COOPERATIVE; m[optnum].value=(Netgame.gamemode == NETGAME_COOPERATIVE); m[optnum].group=0; opt.coop=optnum; optnum++;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Bounty"; m[optnum].value = ( Netgame.gamemode & NETGAME_BOUNTY ); m[optnum].group = 0; opt.mode_end=opt.bounty=optnum; optnum++;

		m[optnum].type = NM_TYPE_TEXT; m[optnum].text = ""; optnum++;

		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Open game"; m[optnum].group=1; m[optnum].value=(!Netgame.RefusePlayers && !Netgame.game_flags & NETGAME_FLAG_CLOSED); optnum++;
		opt.closed = optnum;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_CLOSED_GAME; m[optnum].group=1; m[optnum].value=Netgame.game_flags & NETGAME_FLAG_CLOSED; optnum++;
		opt.refuse = optnum;
		m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Restricted Game              "; m[optnum].group=1; m[optnum].value=Netgame.RefusePlayers; optnum++;

		numplayers_limit = Netgame.gamemode == NETGAME_COOPERATIVE ? 4 : Netgame.max_numobservers ? 7 : 8;
		if (Netgame.max_numplayers > numplayers_limit)
			Netgame.max_numplayers = numplayers_limit;

		opt.maxnet = optnum;
		sprintf( srmaxnet, "Maximum players: %d", Netgame.max_numplayers);
		m[optnum].type = NM_TYPE_SLIDER; m[optnum].value=Netgame.max_numplayers-2; m[optnum].text= srmaxnet; m[optnum].min_value=0;
		m[optnum].max_value=numplayers_limit-2; optnum++;

		opt.maxobs = optnum;
		sprintf( srmaxobs, "Maximum observers: %d", Netgame.max_numobservers);
		m[optnum].type = NM_TYPE_SLIDER; m[optnum].value=Netgame.max_numobservers/2; m[optnum].text= srmaxobs; m[optnum].min_value=0;
		m[optnum].max_value=MAX_OBSERVERS/2; optnum++;

		opt.obsdelay = optnum;
		sprintf( srbdelay, "Broadcast delay %d seconds", OBSERVER_DELAY);
		m[optnum].type = NM_TYPE_CHECK; m[optnum].text = srbdelay; m[optnum].value = Netgame.obs_delay;
		optnum++;

		opt.obsmin = optnum;
		m[optnum].type = NM_TYPE_CHECK; m[optnum].text = "Minimal Observer Info"; m[optnum].value = Netgame.obs_min;
		optnum++;

		opt.moreopts = optnum;
		m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Advanced options"; optnum++;

		Assert(optnum <= SDL_arraysize(m));

		choice = newmenu_do1( NULL, TXT_NETGAME_SETUP, optnum, m, (int (*)( newmenu *, d_event *, void * ))net_udp_game_param_handler, &opt, choice );

		if (choice != GAME_PARAM_CHOICE_SHOW_AGAIN)
			break;
		choice = opt.load_preset;
	}

	if (choice < 0)
		net_udp_close();

	write_netgame_profile(&Netgame);

	return choice >= 0;
}

static char *connecting_txt = "Connecting...";
static char *blank = "";

static int manual_join_game_handler(newmenu *menu, d_event *event, direct_join *dj)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			if (dj->connecting && event_key_get(event) == KEY_ESC)
			{
				dj->connecting = 0;
				items[6].text = blank;
				return 1;
			}
			break;
			
		case EVENT_IDLE:
			if (dj->connecting)
			{
				if (net_udp_game_connect(dj))
					return -2;	// Success!
				else if (!dj->connecting)
					items[6].text = blank;
			}
			break;

		case EVENT_NEWMENU_SELECTED:
		{
			int sockres = -1;

			net_udp_init(); // yes, redundant call but since the menu does not know any better it would allow any IP entry as long as Netgame-entry looks okay... my head hurts...
			
			if ((atoi(UDP_MyPort)) <= 0 ||(atoi(UDP_MyPort)) > 65535)
			{
				net_udp_my_port_default();
				nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal port");
				return 1;
			}
			
			sockres = udp_open_socket(0, atoi(UDP_MyPort));
			
			if (sockres != 0)
			{
				return 1;
			}
			
			// Resolve address
			if (udp_dns_filladdr(dj->addrbuf, atoi(dj->portbuf), &dj->host_addr) < 0)
			{
				return 1;
			}
			else
			{
				multi_new_game();
				net_udp_reset_connection_statuses();
				N_players = 0;
				change_playernum_to(1);
				dj->start_time = timer_query();
				dj->last_time = 0;
				
				memcpy((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&dj->host_addr, sizeof(struct _sockaddr));
				
				dj->connecting = 1;
				items[6].text = connecting_txt;
				return 1;
			}

			break;
		}
			
		case EVENT_WINDOW_CLOSE:
			if (!Game_wind) // they cancelled
				net_udp_close();
			d_free(dj);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void net_udp_manual_join_game()
{
	direct_join *dj;
	newmenu_item m[7];
	int nitems = 0;

	MALLOC(dj, direct_join, 1);
	if (!dj)
		return;
	dj->connecting = 0;
	dj->addrbuf[0] = '\0';
	dj->portbuf[0] = '\0';
	
	net_udp_init();

	memset(&dj->addrbuf,'\0', sizeof(char)*128);
	snprintf(dj->addrbuf, sizeof(dj->addrbuf), "%s", GameArg.MplUdpHostAddr);

	if (GameArg.MplUdpHostPort != 0)
		snprintf(dj->portbuf, sizeof(dj->portbuf), "%d", GameArg.MplUdpHostPort);
	else
		snprintf(dj->portbuf, sizeof(dj->portbuf), "%d", UDP_PORT_DEFAULT);

	net_udp_my_port_init();

	nitems = 0;
	
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text="GAME ADDRESS OR HOSTNAME:";     	nitems++;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text=dj->addrbuf; m[nitems].text_len=128; 	nitems++;
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text="GAME PORT:";                    	nitems++;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text=dj->portbuf; m[nitems].text_len=5;   	nitems++;
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text="MY PORT:";	                    	nitems++;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text=UDP_MyPort; m[nitems].text_len=5;	nitems++;
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text=blank;								nitems++;	// for connecting_txt

	newmenu_do1( NULL, "ENTER GAME ADDRESS", nitems, m, (int (*)(newmenu *, d_event *, void *))manual_join_game_handler, dj, 0 );
}

static char *ljtext;

int net_udp_list_join_poll( newmenu *menu, d_event *event, direct_join *dj )
{
	// Polling loop for Join Game menu
	int i, newpage = 0;
	static int NLPage = 0;
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
		{
			Netgame.protocol.udp.valid = 0;
			memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);
			num_active_udp_changed = 1;
			num_active_udp_games = 0;
			net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
			net_udp_request_game_info(GMcast_v6, 1);
#endif
#ifdef USE_TRACKER
			udp_tracker_reqgames();
#endif
			break;
		}
		case EVENT_IDLE:
			if (dj->connecting)
			{
				if (net_udp_game_connect(dj))
					return -2;	// Success!
			}
			break;
		case EVENT_KEY_COMMAND:
		{
			int key = event_key_get(event);
			if (key == KEY_PAGEUP)
			{
				NLPage--;
				newpage++;
				if (NLPage < 0)
					NLPage = UDP_NETGAMES_PAGES-1;
				key = 0;
				break;
			}
			if (key == KEY_PAGEDOWN)
			{
				NLPage++;
				newpage++;
				if (NLPage >= UDP_NETGAMES_PAGES)
					NLPage = 0;
				key = 0;
				break;
			}
			if( key == KEY_F4 )
			{
				// Empty the list
				memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				
				// Request LAN games
				net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
				net_udp_request_game_info(GMcast_v6, 1);
#endif
				
#ifdef USE_TRACKER
				udp_tracker_reqgames();
#endif
				// All done
				break;
			}
			if (key == KEY_F5)
			{
				memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
				net_udp_request_game_info(GMcast_v6, 1);
#endif
				break;
			}
#ifdef USE_TRACKER
			if( key == KEY_F6 )
			{
				// Zero the list
				memset( Active_udp_games, 0, sizeof( UDP_netgame_info_lite ) * UDP_MAX_NETGAMES );
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				
				// Request from the tracker
				udp_tracker_reqgames();
				
				// Break off
				break;
			}
#endif
			if (key == KEY_ESC)
			{
				if (dj->connecting)
				{
					dj->connecting = 0;
					return 1;
				}
				break;
			}
			break;
		}
		case EVENT_NEWMENU_SELECTED:
		{
			if (((citem+(NLPage*UDP_NETGAMES_PPAGE)) >= 4) && (((citem+(NLPage*UDP_NETGAMES_PPAGE))-4) <= num_active_udp_games-1))
			{
				multi_new_game();
				net_udp_reset_connection_statuses();
				N_players = 0;
				change_playernum_to(1);
				dj->start_time = timer_query();
				dj->last_time = 0;
				memcpy((struct _sockaddr *)&dj->host_addr, (struct _sockaddr *)&Active_udp_games[(citem+(NLPage*UDP_NETGAMES_PPAGE))-4].game_addr, sizeof(struct _sockaddr));
				memcpy((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&dj->host_addr, sizeof(struct _sockaddr));
				dj->connecting = 1;
				return 1;
			}
			else
			{
				nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
				return -1; // invalid game selected - stay in the menu
			}
			break;
		}
		case EVENT_WINDOW_CLOSE:
		{
			d_free(ljtext);
			d_free(menus);
			d_free(dj);
			if(observer_data_buffer != 0) {
				d_free(observer_data_buffer); 
			}
			if (!Game_wind)
			{
				net_udp_close();
				Network_status = NETSTAT_MENU;	// they cancelled
			}
			return 0;
		}
		default:
			break;
	}

	net_udp_listen();

	if (!num_active_udp_changed && !newpage)
		return 0;

	num_active_udp_changed = 0;

	// Copy the active games data into the menu options
	for (i = 0; i < UDP_NETGAMES_PPAGE; i++)
	{
		int game_status = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_status;
		int j,x, k,tx,ty,ta,nplayers = 0;
		char levelname[8],MissName[25],GameName[25],thold[2],status[9];
		thold[1]=0;

		if ((i+(NLPage*UDP_NETGAMES_PPAGE)) >= num_active_udp_games)
		{
			snprintf(menus[i+4].text, sizeof(char)*74, "%d.                                                                      ",(i+(NLPage*UDP_NETGAMES_PPAGE))+1);
			continue;
		}

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		gr_set_curfont(GAME_FONT);

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j]=='\t')
				continue;
			thold[0]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
			{
				MissName[k]=MissName[k+1]=MissName[k+2]='.';
				k+=3;
				break;
			}

			MissName[k++]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j];
		}
		MissName[k]=0;

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_name[j]=='\t')
				continue;
			thold[0]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_name[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
			{
				GameName[k]=GameName[k+1]=GameName[k+2]='.';
				k+=3;
				break;
			}
			GameName[k++]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_name[j];
		}
		GameName[k]=0;

		nplayers = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].numconnected;

		if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].levelnum < 0)
			snprintf(levelname, sizeof(levelname), "S%d", -Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].levelnum);
		else
			snprintf(levelname, sizeof(levelname), "%d", Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].levelnum);

		if (game_status == NETSTAT_STARTING)
			snprintf(status, sizeof(status), "FORMING ");
		else if (game_status == NETSTAT_PLAYING)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].RefusePlayers)
				snprintf(status, sizeof(status), "RESTRICT");
			else if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_flags & NETGAME_FLAG_CLOSED)
				snprintf(status, sizeof(status), "CLOSED  ");
			else
				snprintf(status, sizeof(status), "OPEN    ");
		}
		else
			snprintf(status, sizeof(status), "BETWEEN ");
		
		unsigned gamemode = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].gamemode;
		snprintf (menus[i+4].text,sizeof(char)*74,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",(i+(NLPage*UDP_NETGAMES_PPAGE))+1,GameName,(gamemode < sizeof(GMNamesShrt) / sizeof(GMNamesShrt[0])) ? GMNamesShrt[gamemode] : "INVALID",nplayers, Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].max_numplayers,MissName,levelname,status);
			
		Assert(strlen(menus[i+4].text) < 75);
	}
	return 0;
}

void net_udp_list_join_game()
{
	int i = 0;
	newmenu_item *m;
	direct_join *dj;

	MALLOC(m, newmenu_item, ((UDP_NETGAMES_PPAGE+4)*2)+1);
	if (!m)
		return;
	MALLOC(ljtext, char, (((UDP_NETGAMES_PPAGE+4)*2)+1)*74);
	if (!ljtext)
	{
		d_free(m);
		return;
	}
	MALLOC(dj, direct_join, 1);
	if (!dj)
		return;
	dj->connecting = 0;
	dj->addrbuf[0] = '\0';
	dj->portbuf[0] = '\0';

	net_udp_init();
	if (udp_open_socket(0, GameArg.MplUdpMyPort != 0?GameArg.MplUdpMyPort:UDP_PORT_DEFAULT) < 0)
		return;

	if (GameArg.MplUdpMyPort != 0)
		if (udp_open_socket(1, UDP_PORT_DEFAULT) < 0)
			nm_messagebox(TXT_WARNING, 1, TXT_OK, "Cannot open default port!\nYou can only scan for games\nmanually.");

	// prepare broadcast address to discover games
	memset(&GBcast, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_BCAST_ADDR, UDP_PORT_DEFAULT, &GBcast);
#ifdef IPv6
	memset(&GMcast_v6, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_MCASTv6_ADDR, UDP_PORT_DEFAULT, &GMcast_v6);
#endif

	change_playernum_to(1);
	N_players = 0;
	Network_send_objects = 0;
	Network_sending_extras=0;
	Network_rejoined=0;

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	net_udp_flush();
	net_udp_listen();  // Throw out old info

	num_active_udp_games = 0;

	memset(m, 0, sizeof(newmenu_item)*(UDP_NETGAMES_PPAGE+2));
	memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);

	gr_set_fontcolor(BM_XRGB(15,15,23),-1);

	m[0].text = ljtext;
	m[0].type = NM_TYPE_TEXT;
	snprintf( m[0].text, sizeof(char)*74, "\tF4/F5/F6: (Re)Scan for all/LAN/Tracker Games." );
	m[1].text = ljtext + 74*1;
	m[1].type = NM_TYPE_TEXT;
	snprintf( m[1].text, sizeof(char)*74, "\tPgUp/PgDn: Flip Pages." );
	m[2].text = ljtext + 74*2;
	m[2].type = NM_TYPE_TEXT;
	snprintf( m[2].text, sizeof(char)*74, " " );
	m[3].text = ljtext + 74*3;
	m[3].type = NM_TYPE_TEXT;
	snprintf (m[3].text, sizeof(char)*74, "\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

	for (i = 0; i < UDP_NETGAMES_PPAGE; i++) {
		m[i+4].text = ljtext + 74 * (i+4);
		m[i+4].type = NM_TYPE_MENU;
		snprintf(m[i+4].text,sizeof(char)*74,"%d.                                                                      ", i+1);
	}

	num_active_udp_changed = 1;
	newmenu_dotiny("NETGAMES", NULL,(UDP_NETGAMES_PPAGE+4), m, 1, (int (*)(newmenu *, d_event *, void *))net_udp_list_join_poll, dj);
}

int net_udp_kmatrix_poll1( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for End-of-level menu
	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;

	net_udp_do_frame(0, 1);
	
	return 0;
}

// Same as above but used when player pressed ESC during kmatrix (host also does the packets for playing clients)
extern fix64 StartAbortMenuTime;
int net_udp_kmatrix_poll2( newmenu *menu, d_event *event, void *userdata )
{
	int rval = 0;

	// Polling loop for End-of-level menu
	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;
	
	if (timer_query() > (StartAbortMenuTime+(F1_0*3)))
		rval = -2;

	net_udp_do_frame(0, 1);
	
	return rval;
}

int net_udp_sync_poll( newmenu *menu, d_event *event, void *userdata )
{
	static fix64 t1 = 0;
	int rval = 0;

	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;
	
	net_udp_listen();

	// Leave if Host disconnects
	if (Netgame.players[0].connected == CONNECT_DISCONNECTED)
		rval = -2;

	if (Network_status != NETSTAT_WAITING)	// Status changed to playing, exit the menu
		rval = -2;

	if (Network_status != NETSTAT_MENU && !Network_rejoined && (timer_query() > t1+F1_0*2))
	{
		int i;

		// Poll time expired, re-send request
		
		t1 = timer_query();

		i = net_udp_send_request();
		if (i < 0)
			rval = -2;
	}
	
	return rval;
}

int net_udp_start_poll( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int nitems = newmenu_get_nitems(menu);
	int i,n,nm;

	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	userdata = userdata;
	
	Assert(Network_status == NETSTAT_STARTING);

	for (i=1; i<nitems; i++ ) {
		if ( (i>= N_players) && (menus[i].value) ) {
			menus[i].value = 0;
		}
	}

	nm = 0;
	for (i=0; i<nitems; i++ ) {
		if ( menus[i].value ) {
			nm++;
			if ( nm > N_players ) {
				menus[i].value = 0;
			}
		}
	}

	if ( nm > Netgame.max_numplayers ) {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN );
		// Turn off the last player highlighted
		for (i = N_players; i > 0; i--)
			if (menus[i].value == 1) 
			{
				menus[i].value = 0;
				break;
			}
	}

   //added/killed by Victor Rachels to eventually add msging
           //since nitems should not be changing, anyway
//        if (nitems > MAX_PLAYERS ) return;
   //end this section kill - VR
	
	n = Netgame.numplayers;
	net_udp_listen();

	if (n < Netgame.numplayers )
	{
		if (PlayerCfg.NoRankings)
	      sprintf( menus[N_players-1].text, "%d. %-20s", N_players,Netgame.players[N_players-1].callsign );
		else
	      sprintf( menus[N_players-1].text, "%d. %s%-20s", N_players, RankStrings[Netgame.players[N_players-1].rank],Netgame.players[N_players-1].callsign );
		//Begin addition by GF
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);  //A noise to alert you when someone joins a starting game...
		//End addition by GF

		if (N_players <= Netgame.max_numplayers)
		{
			menus[N_players-1].value = 1;
		}
	} 
	else if ( n > Netgame.numplayers )
	{
		// One got removed...

		//Begin addition by GF
		// <Taken out for now due to lack of testing> digi_play_sample(SOUND_HUD_KILL, F1_0);  //A noise to alert you when someone leaves a starting game...
		//End addition by GF

		for (i=0; i<N_players; i++ )
		{
	 if (PlayerCfg.NoRankings)	
		 sprintf( menus[i].text, "%d. %-20s", i+1, Netgame.players[i].callsign );
	 else
		 sprintf( menus[i].text, "%d. %s%-20s", i+1, RankStrings[Netgame.players[i].rank],Netgame.players[i].callsign );
			if (i < Netgame.max_numplayers)
				menus[i].value = 1;
			else
				menus[i].value = 0;
		}
		for (i=N_players; i<n; i++ )
		{
			sprintf( menus[i].text, "%d. ", i+1 );          // Clear out the deleted entries...
			menus[i].value = 0;
		}
   }

	return 0;
}

static int show_game_rules_handler(window *wind, d_event *event, netgame_info *netgame)
{
	int k;
	int w = FSPACX(280), h = FSPACY(130);
	int y;
	int label_color, value_color;
	const char *ammo_style[] = {"Dupl", "Depl", "Drop", "Respawn"};
	
	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;
			
		case EVENT_KEY_COMMAND:
			k = event_key_get(event);
			switch (k)
			{
				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					window_close(wind);
					return 1;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			timer_delay2(50);

			gr_set_current_canvas(NULL);
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);
			
			gr_set_current_canvas(window_get_canvas(wind));
			
			grd_curcanv->cv_font = MEDIUM3_FONT;
			
			label_color = gr_find_closest_color_current(29,29,47);
			value_color = gr_find_closest_color_current(255,255,255);

			gr_set_fontcolor(label_color,-1);
			gr_string( 0x8000, FSPACY(35), "NETGAME INFO" );
			
			grd_curcanv->cv_font = GAME_FONT;
			gr_printf( FSPACX( 25),FSPACY( 55), "Reactor Life:");
			gr_printf( FSPACX( 25),FSPACY( 61), "Max Time:");
			gr_printf( FSPACX( 25),FSPACY( 67), "Kill Goal:");
			gr_printf( FSPACX( 25),FSPACY( 73), "Packets per sec.:");
			gr_printf( FSPACX( 25),FSPACY( 79), "Homing Rate:");
			gr_printf( FSPACX(155),FSPACY( 55), "Spawn Style:");
			gr_printf( FSPACX(155),FSPACY( 61), "Bright player ships:");
			gr_printf( FSPACX(155),FSPACY( 67), "Show enemy names on hud:");
			gr_printf( FSPACX(155),FSPACY( 73), "Show players on automap:");
			gr_printf( FSPACX(155),FSPACY( 79), "No friendly Fire:");
			
			gr_set_fontcolor(value_color,-1);
			gr_printf( FSPACX(115),FSPACY( 55), "%i Min", netgame->control_invul_time/F1_0/60);
			gr_printf( FSPACX(115),FSPACY( 61), "%i Min", netgame->PlayTimeAllowed*5);
			gr_printf( FSPACX(115),FSPACY( 67), "%i", netgame->KillGoal*10);
			gr_printf( FSPACX(115),FSPACY( 73), "%i", netgame->PacketsPerSec);
			gr_printf( FSPACX(115),FSPACY( 79), "%i", netgame->HomingUpdateRate);
			gr_printf( FSPACX(275),FSPACY( 55), netgame->SpawnStyle == SPAWN_STYLE_NO_INVUL ? "NoInv" : (
												netgame->SpawnStyle == SPAWN_STYLE_SHORT_INVUL ? "Short" : (
												netgame->SpawnStyle == SPAWN_STYLE_LONG_INVUL ? "Long" : "Preview")));
			gr_printf( FSPACX(275),FSPACY( 61), netgame->BrightPlayers?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 67), netgame->ShowEnemyNames?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 73), netgame->game_flags&NETGAME_FLAG_SHOW_MAP?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 79), netgame->NoFriendlyFire?"ON":"OFF");

			y = 85;

			gr_set_fontcolor(label_color,-1);
			gr_printf( FSPACX( 25),FSPACY(y+ 0), "Confirmed Sparks:");
			gr_printf( FSPACX( 25),FSPACY(y+ 6), "Custom Mods:");
			gr_printf( FSPACX( 25),FSPACY(y+12), "New Spawns:");
			gr_printf( FSPACX(155),FSPACY(y+ 0), "Reduced Flash:");
			gr_printf( FSPACX(155),FSPACY(y+ 6), "Vulcan Ammo Style:");

			gr_set_fontcolor(value_color,-1);
			gr_printf( FSPACX(115),FSPACY(y+ 0), netgame->RemoteHitSpark?"ON":"OFF");
			gr_printf( FSPACX(115),FSPACY(y+ 6), netgame->AllowCustomModelsTextures?"ON":"OFF");
			gr_printf( FSPACX(115),FSPACY(y+12), netgame->NewSpawnAlgorithm ? "ON" : "OFF");
			gr_printf( FSPACX(275),FSPACY(y+ 0), netgame->ReducedFlash?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY(y+ 6), ammo_style[netgame->GaussAmmoStyle]);
			
			y += 12 + 9;

			gr_set_fontcolor(label_color,-1);
			gr_printf( FSPACX( 25),FSPACY(y+ 0), "Allowed Objects");
			gr_printf( FSPACX( 25),FSPACY(y+10), "Laser Upgrade:");
			gr_printf( FSPACX( 25),FSPACY(y+16), "Quad Laser:");
			gr_printf( FSPACX( 25),FSPACY(y+22), "Vulcan Cannon:");
			gr_printf( FSPACX( 25),FSPACY(y+28), "Spreadfire Cannon:");
			gr_printf( FSPACX( 25),FSPACY(y+34), "Plasma Cannon:");
			gr_printf( FSPACX( 25),FSPACY(y+40), "Fusion Cannon:");
			gr_printf( FSPACX(170),FSPACY(y+10), "Homing Missile:");
			gr_printf( FSPACX(170),FSPACY(y+16), "Proximity Bomb:");
			gr_printf( FSPACX(170),FSPACY(y+22), "Smart Missile:");
			gr_printf( FSPACX(170),FSPACY(y+28), "Mega Missile:");
			gr_printf( FSPACX( 25),FSPACY(y+50), "Invulnerability:");
			gr_printf( FSPACX( 25),FSPACY(y+56), "Cloak:");

			gr_set_fontcolor(value_color,-1);
			gr_printf( FSPACX(130),FSPACY(y+10), netgame->AllowedItems&NETFLAG_DOLASER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+16), netgame->AllowedItems&NETFLAG_DOQUAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+22), netgame->AllowedItems&NETFLAG_DOVULCAN?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+28), netgame->AllowedItems&NETFLAG_DOSPREAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+34), netgame->AllowedItems&NETFLAG_DOPLASMA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+40), netgame->AllowedItems&NETFLAG_DOFUSION?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(y+10), netgame->AllowedItems&NETFLAG_DOHOMING?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(y+16), netgame->AllowedItems&NETFLAG_DOPROXIM?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(y+22), netgame->AllowedItems&NETFLAG_DOSMART?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(y+28), netgame->AllowedItems&NETFLAG_DOMEGA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+50), netgame->AllowedItems&NETFLAG_DOINVUL?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(y+56), netgame->AllowedItems&NETFLAG_DOCLOAK?"YES":"NO");

			gr_set_current_canvas(NULL);
			break;

		default:
			break;
	}
	
	return 0;
}

void net_udp_show_game_rules(netgame_info *netgame)
{
	gr_set_current_canvas(NULL);

	window_create(&grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200), 
				  (int (*)(window *, d_event *, void *))show_game_rules_handler, netgame);
}

static int show_game_info_handler(newmenu *menu, d_event *event, netgame_info *netgame)
{
	if (event->type != EVENT_NEWMENU_SELECTED)
		return 0;
	
	if (newmenu_get_citem(menu) != 2)
		return 0;

	net_udp_show_game_rules(netgame);
	
	return 1;
}

int net_udp_show_game_info()
{
	char rinfo[512],*info=rinfo;
	int c;
	netgame_info *netgame = &Netgame;

	memset(info,0,sizeof(char)*256);

	info+=sprintf(info,"\nConnected to\n\"%s\"\n",netgame->game_name);

	if(!netgame->mission_title)
		info+=sprintf(info,"Descent: First Strike");
	else
		info+=sprintf(info,"%s",netgame->mission_title);

   if( netgame->levelnum >= 0 )
   {
	   info+=sprintf (info," - Lvl %i",netgame->levelnum);
   }
   else
   {
      info+=sprintf (info," - Lvl S%i",(netgame->levelnum*-1));
   }

	info+=sprintf (info,"\n\nDifficulty: %s",MENU_DIFFICULTY_TEXT(netgame->difficulty));
	unsigned gamemode = netgame->gamemode;
	info+=sprintf (info,"\nGame Mode: %s",gamemode < (sizeof(GMNames) / sizeof(GMNames[0])) ? GMNames[gamemode] : "INVALID");
	info+=sprintf (info,"\nPlayers: %i/%i",netgame->numplayers,netgame->max_numplayers);

	c=nm_messagebox1("WELCOME", (int (*)(newmenu *, d_event *, void *))show_game_info_handler, netgame, 3, "JOIN GAME", "OBSERVE", "GAME INFO", rinfo);

	if (c==0)
		return 1;
	if(c==1)
		return 2; 
	//else if (c==1)
	// handled in above callback
	else
		return 0;
}

int net_udp_menu_select_teams_handler(newmenu* menu, d_event* event, void* userdata);

struct select_teams_menu_data {
	char team_names[2][CALLSIGN_LEN + 1];
	ubyte team_colors[2];
	int opt_team_a_color;
	int opt_team_b_color;
};

int
net_udp_select_teams(void)
{
	newmenu_item m[MAX_PLAYERS+7];
	int choice, opt, opt_team_b;
	ubyte team_vector = 0;
	struct select_teams_menu_data menu_data;
	int i;
	int pnums[MAX_PLAYERS+2];

	// One-time initialization

	for (i = N_players/2; i < N_players; i++) // Put first half of players on team A
	{
		team_vector |= (1 << i);
	}

	sprintf(menu_data.team_names[0], "%s", TXT_BLUE);
	sprintf(menu_data.team_names[1], "%s", TXT_RED);
	menu_data.team_colors[0] = menu_data.team_colors[1] = 8; // 8 = default

	// Here comes da menu
	while (1) {
		opt = 0;

		menu_data.opt_team_a_color = opt;
		m[opt].type = NM_TYPE_SLIDER;
		m[opt].value = menu_data.team_colors[0];
		m[opt].min_value = 0;
		m[opt].max_value = 8;
		m[opt].text = "Team 1 color: ";
		opt++;

		m[opt].type = NM_TYPE_INPUT;
		m[opt].text = menu_data.team_names[0];
		m[opt].text_len = CALLSIGN_LEN;
		opt++;

		// Team A player list
		for (i = 0; i < N_players; i++)
		{
			if (!(team_vector & (1 << i)))
			{
				m[opt].type = NM_TYPE_MENU;
				m[opt].text = Netgame.players[i].callsign;
				pnums[opt] = i;
				opt++;
			}
		}

		opt_team_b = opt;

		m[opt].type = NM_TYPE_TEXT;
		m[opt].text = "";
		opt++;

		menu_data.opt_team_b_color = opt;
		m[opt].type = NM_TYPE_SLIDER;
		m[opt].value = menu_data.team_colors[1];
		m[opt].min_value = 0;
		m[opt].max_value = 8;
		m[opt].text = "Team 2 color: ";
		opt++;

		m[opt].type = NM_TYPE_INPUT;
		m[opt].text = menu_data.team_names[1];
		m[opt].text_len = CALLSIGN_LEN;
		opt++;

		// Team B player list
		for (i = 0; i < N_players; i++)
		{
			if (team_vector & (1 << i))
			{
				m[opt].type = NM_TYPE_MENU;
				m[opt].text = Netgame.players[i].callsign;
				pnums[opt] = i;
				opt++;
			}
		}

		m[opt].type = NM_TYPE_TEXT;
		m[opt].text = "";
		opt++;

		m[opt].type = NM_TYPE_MENU;
		m[opt].text = TXT_ACCEPT;
		opt++;

		Assert(opt <= SDL_arraysize(m));
	
		choice = newmenu_do(NULL, TXT_TEAM_SELECTION, opt, m, net_udp_menu_select_teams_handler, &menu_data);

		if (choice == opt-1)
		{
			Netgame.team_vector = team_vector;
			strcpy(Netgame.team_name[0], menu_data.team_names[0]);
			strcpy(Netgame.team_name[1], menu_data.team_names[1]);
			Netgame.team_color[0] = menu_data.team_colors[0];
			Netgame.team_color[1] = menu_data.team_colors[1];
			return 1;
		}

		else if ((choice > 0) && (choice < opt_team_b)) {
			team_vector |= (1 << pnums[choice]);
		}
		else if ((choice > opt_team_b) && (choice < opt-2)) {
			team_vector &= ~(1 << pnums[choice]);
		}
		else if (choice == -1)
			return 0;
	}
}

int net_udp_menu_select_teams_handler(newmenu* menu, d_event* event, void* userdata)
{
	newmenu_item* menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	struct select_teams_menu_data* menu_data = (struct select_teams_menu_data*)userdata;

	if (event->type == EVENT_NEWMENU_CHANGED) {
		if (citem == menu_data->opt_team_a_color) {
			menu_data->team_colors[0] = menus[citem].value;
			if (menu_data->team_colors[0] == 8)
				sprintf(menu_data->team_names[0], "%s", TXT_BLUE);
			else
				get_color_name(menu_data->team_names[0], SDL_arraysize(menu_data->team_names[0]),
					menu_data->team_colors[0], Netgame.BlackAndWhitePyros);
		} else if (citem == menu_data->opt_team_b_color) {
			menu_data->team_colors[1] = menus[citem].value;
			if (menu_data->team_colors[1] == 8)
				sprintf(menu_data->team_names[1], "%s", TXT_RED);
			else
				get_color_name(menu_data->team_names[1], SDL_arraysize(menu_data->team_names[1]),
					menu_data->team_colors[1], Netgame.BlackAndWhitePyros);
		}
	}

	return 0;
}

int
net_udp_select_players(void)
{
        int i, j, opts, opt_msg;
        newmenu_item m[MAX_PLAYERS+1];
	char text[MAX_PLAYERS][45];
	char title[50];
	int save_nplayers;

	net_udp_add_player( &UDP_Seq );
		
	for (i=0; i< MAX_PLAYERS; i++ )	{
		sprintf( text[i], "%d.  %-20s", i+1, "" );
		m[i].type = NM_TYPE_CHECK; m[i].text = text[i]; m[i].value = 0;
	}
//added/edited on 11/7/98 by Victor Rachels in an attempt to get msgs going.
        opts=MAX_PLAYERS;
        opt_msg = opts;
//killed for now to not raise people's hopes - 11/10/98 - VR
//        m[opts].type = NM_TYPE_MENU; m[opts].text = "Send message..."; opts++;

	m[0].value = 1;                         // Assume server will play...

	if (PlayerCfg.NoRankings)
		sprintf( text[0], "%d. %-20s", 1, Players[Player_num].callsign );
	else
		sprintf( text[0], "%d. %s%-20s", 1, RankStrings[Netgame.players[Player_num].rank],Players[Player_num].callsign );
	sprintf( title, "%s %d %s", TXT_TEAM_SELECT, Netgame.max_numplayers, TXT_TEAM_PRESS_ENTER );

GetPlayersAgain:
#ifdef USE_TRACKER
	if( Netgame.Tracker )
		udp_tracker_register();
#endif

        j=opt_msg;
         while(j==opt_msg)
          {
		  timer_update();
            j=newmenu_do1( NULL, title, opts, m, net_udp_start_poll, NULL, 1 );

            if(j==opt_msg)
             {
              multi_send_message_dialog();
               if (Network_message_reciever != -1)
                multi_send_message();
             }
          }
//end this section addition
	save_nplayers = N_players;

	if (j<0) 
	{
		// Aborted!
		// Dump all players and go back to menu mode
#ifdef USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif
abort:
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (i=1; i<save_nplayers; i++) {
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			net_udp_dump_player(Netgame.players[i].protocol.udp.addr, player_tokens[i], DUMP_ABORTED);
			net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO, 0, 0);
		}
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
		Netgame.numplayers = save_nplayers;

		Network_status = NETSTAT_MENU;
		return(0);
	}

	// Count number of players chosen

	N_players = 0;
	for (i=0; i<save_nplayers; i++ )
	{
		if (m[i].value)
			N_players++;
	}
	
	if ( N_players > Netgame.max_numplayers) {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}

// Let host join without Client available. Let's see if our players like that
#if 0 //def RELEASE
	if ( N_players < 2 )    {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

// Let host join without Client available. Let's see if our players like that
#if 0 //def RELEASE
	if ( (Netgame.gamemode == NETGAME_TEAM_ANARCHY) && (N_players < 3) ) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_THREE );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

	// Remove players that aren't marked.
	N_players = 0;
    Host_is_obs = 0;
	for (i=0; i<save_nplayers; i++ )	{
		if (m[i].value)
		{
			if (i > N_players)
			{
				memcpy(Netgame.players[N_players].callsign, Netgame.players[i].callsign, CALLSIGN_LEN+1);
				Netgame.players[N_players].rank=Netgame.players[i].rank;
				ClipRank (&Netgame.players[N_players].rank);
				player_tokens[N_players] = player_tokens[i];
			}
			Players[N_players].connected = CONNECT_PLAYING;
			N_players++;
		}
		else
		{
			if (i == 0) {
				Netgame.host_is_obs = 1;
                Host_is_obs = 1;
				N_players++;
				Game_mode |= GM_OBSERVER;
				Current_obs_player = 0;
			} else {
				net_udp_dump_player(Netgame.players[i].protocol.udp.addr, player_tokens[i], DUMP_DORK);
			}
		}
	}

	for (i = N_players; i < MAX_PLAYERS; i++) {
		memset(Netgame.players[i].callsign, 0, CALLSIGN_LEN+1);
		Netgame.players[i].rank=0;
	}

	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY)
		if (!net_udp_select_teams())
			goto abort;

	return(1);
}

int
net_udp_wait_for_sync(void)
{
	char text[60];
	newmenu_item m[2];
	int i, choice=0;
	
	Network_status = NETSTAT_WAITING;
	m[0].type=NM_TYPE_TEXT; m[0].text = text;
	m[1].type=NM_TYPE_TEXT; m[1].text = TXT_NET_LEAVE;
	
	i = net_udp_send_request();

	if (i < 0)
		return(-1);

	sprintf( m[0].text, "%s\n'%s' %s", TXT_NET_WAITING, Netgame.players[i].callsign, TXT_NET_TO_ENTER );

	while (choice > -1)
	{		
		timer_update();
		choice=newmenu_do( NULL, TXT_WAIT, 2, m, net_udp_sync_poll, NULL );
	}


	if (Network_status != NETSTAT_PLAYING)	
	{
		UDP_sequence_packet me;

		memset(&me, 0, sizeof(UDP_sequence_packet));
		me.type = UPID_QUIT_JOINING;
		memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		me.player.color = PlayerCfg.ShipColor;
		me.player.missilecolor = PlayerCfg.MissileColor;
		net_udp_send_sequence_packet( me, Netgame.players[0].protocol.udp.addr );
		N_players = 0;
		Game_mode = GM_GAME_OVER;
		return(-1);     // they cancelled
	}
	return(0);
}

int net_udp_request_poll( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int num_ready = 0;

	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;
	
	net_udp_listen();
	net_udp_timeout_check(timer_query());

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected == CONNECT_PLAYING) || (Players[i].connected == CONNECT_DISCONNECTED))
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		return -2;
	}
	
	return 0;
}

int net_udp_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	newmenu_item m[1];
	
	Network_status = NETSTAT_WAITING;

	m[0].type=NM_TYPE_TEXT; m[0].text = TXT_NET_LEAVE;

	net_udp_flush();

	Players[Player_num].connected = CONNECT_PLAYING;

menu:
	choice = newmenu_do(NULL, TXT_WAIT, 1, m, net_udp_request_poll, NULL);	

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox(NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return 0;
		if (choice != 0)
			goto menu;
		
		// User confirmed abort
		
		for (i=0; i < N_players; i++)
			if ((Players[i].connected != CONNECT_DISCONNECTED) && (i != Player_num))
				net_udp_dump_player(Netgame.players[i].protocol.udp.addr, player_tokens[i], DUMP_ABORTED);

		return -1;
	}
	else if (choice != -2)
		goto menu;

	return 0;
}

int
net_udp_level_sync(void)
{
 	// Do required syncing between (before) levels

	int result = 0;

	memset(&UDP_MData, 0, sizeof(UDP_mdata_info));
	net_udp_noloss_init_mdata_queue();

//	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment)*(Highest_segment_index+1));

	net_udp_flush(); // Flush any old packets

	if (N_players == 0)
		result = net_udp_wait_for_sync();
	else if (multi_i_am_master())
	{
		result = net_udp_wait_for_requests();
		if (!result)
			result = net_udp_send_sync();
	}
	else
		result = net_udp_wait_for_sync();

	multi_powcap_count_powerups_in_mine();

	if (result)
	{
		Players[Player_num].connected = CONNECT_DISCONNECTED;

		if (Current_obs_player == Player_num) {
			reset_obs();
		}

		net_udp_send_endlevel_packet();
		if (Game_wind)
			window_close(Game_wind);
		show_menus();
		net_udp_close();
		return -1;
	}
	return(0);
}

int net_udp_do_join_game(ubyte join_as_obs)
{
	
	if (Netgame.game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return 0;
	}

	if (!load_mission_by_name(Netgame.mission_name))
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
		return 0;
	}

	switch (net_udp_can_join_netgame(&Netgame, join_as_obs))
	{
		case 0:
			if (Netgame.numplayers == Netgame.max_numplayers)
				nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
			else
				nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
			return 0;
	}

	// Choice is valid, prepare to join in
	Difficulty_level = Netgame.difficulty;
	if(! join_as_obs) { change_playernum_to(1); }

	net_udp_set_game_mode(Netgame.gamemode, join_as_obs);
	
	StartNewLevel(Netgame.levelnum);

	return 1;     // look ma, we're in a game!!!
}

#endif /* NETWORK */
