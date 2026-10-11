/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * prototype definitions for descent.cfg reading/writing
 *
 */


#ifndef _CONFIG_H
#define _CONFIG_H

#include "player.h"
#include "mission.h"

typedef struct Cfg
{
	ubyte DigiVolume;
	ubyte MusicVolume;
	int ReverseStereo;
	int OrigTrackOrder;
	int MusicType;
	int CMLevelMusicPlayOrder;
	int CMLevelMusicTrack[2];
	char CMLevelMusicPath[PATH_MAX+1];
	char CMMiscMusic[5][PATH_MAX+1];
	int GammaLevel;
	char LastPlayer[CALLSIGN_LEN+1];
	char LastMission[MISSION_NAME_LEN+1];
	int ResolutionX;
	int ResolutionY;
	int AspectX;
	int AspectY;
	int WindowMode;
	int TexFilt;
	// VSync -- VESTIGIAL, IGNORED.
//
// This used to be an in-game "VSync" checkbox, but it never actually controlled
// vsync: GameCfg.VSync was never passed to SDL or to the graphics driver, so
// every use of it was just
// `GameCfg.VSync ? MAXIMUM_FPS : PlayerCfg.maxFps`. Checking the box therefore
// only overrode the user's Framerate setting (and could push the rate past their
// chosen cap). The menu option has been removed; see menu.c.
//
// Real vsync is the graphics driver's concern and applies when VSync is switched
// on in the GPU control panel, entirely independent of this field.
//
// The field is retained only so that existing d1x.cfg files containing a
// "VSync=" line still parse without error. The value is never read for behaviour.
int VSync;
	int Multisample;
	int UseCustomModels;
	int UseCustomTextures;
	// Reduced palette flash effects (damage flashes, weapon pickup flashes and
	// the fusion charge flash). Local graphics setting, independent of the
	// multiplayer netgame flag of the same name -- see reduced_flash_wanted()
	// in game.c for how the two combine.
	int ReducedFlash;
	int FPSIndicator;
	int Grabinput;
	int ClassicDepth;
	int BorderlessWindow;
} __pack__ Cfg;

extern struct Cfg GameCfg;

//#ifdef USE_SDLMIXER
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook || GameCfg.JukeboxOn)
//#else
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook)		// JukeboxOn shouldn't do anything if it's not supported
//#endif

extern int ReadConfigFile(void);
extern int WriteConfigFile(void);

#endif
