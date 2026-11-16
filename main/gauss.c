/*
 * "Vulcan becomes Gauss" (see gauss.h). The Vulcan keeps its slot, pickup, ammo and keys; with the
 * option on its weapon_info gets D2's Gauss numbers in D2's "Disable Gauss Splash" form, and its
 * pickup, HUD icon and firing sound swap to the Gauss art when those files are there.
 */
#include <string.h>
#include <stdio.h>
#include <SDL.h>
#include "pstypes.h"
#include "u_mem.h"
#include "console.h"
#include "physfsx.h"
#include "inferno.h"
#include "game.h"
#include "object.h"
#include "weapon.h"
#include "laser.h"
#include "robot.h"
#include "powerup.h"
#include "vclip.h"
#include "fireball.h"
#include "piggy.h"
#include "custom.h"
#include "sounds.h"
#include "digi.h"
#include "multi.h"
#include "playsave.h"
#include "gauss.h"
#include "customsnd.h"

#define GAUSS_DRONE_ROBOT	3	// D1's Class 1 drone: Gauss hits use the hit effects of its fireball
#define GAUSS_FRAMES		16	// gauss#0.png ... gauss#15.png

static weapon_info vulcan_original;
static int vulcan_saved;
static int vulcan_powerup_vclip = -1, gauss_powerup_vclip = -1;
static int gauss_sound = -1;	// game sound number of gauss.wav
static bitmap_index gauss_picture;	// HUD icon, its texture is gauge21b.png

int gauss_mode(void)
{
	if (Game_mode & GM_MULTI)
		return (Netgame.game_flags & NETGAME_FLAG_GAUSS) != 0;
	return PlayerCfg.SPGauss;
}

int gauss_png_always(const char *bitmapname)
{
	return bitmapname && (!strncmp(bitmapname, "gauss#", 6) || !strcmp(bitmapname, "gauge21b"));
}

// A new game bitmap with the pixels of 'like' as a placeholder, named so the PNG override finds the Gauss art.
static bitmap_index gauss_add_bitmap(bitmap_index like, const char *name)
{
	bitmap_index bi;
	grs_bitmap bm;

	bi.index = 0;
	if (!like.index || Num_bitmap_files >= MAX_BITMAP_FILES)
		return bi;
	bm = GameBitmaps[like.index];
#ifdef OGL
	bm.gltexture = NULL; // its own texture, not the Vulcan's
#endif
	bi = piggy_register_bitmap(&bm, (char *)name, 1);
	GameBitmapOffset[bi.index] = GameBitmapOffset[like.index]; // pages in the same placeholder pixels from the PIG
	GameBitmapFlags[bi.index] = GameBitmapFlags[like.index];
	return bi;
}

static void gauss_add_powerup_vclip(void)
{
	vclip *v;
	int i;

	vulcan_powerup_vclip = Powerup_info[POW_VULCAN_WEAPON].vclip_num;
	if (vulcan_powerup_vclip < 0 || vulcan_powerup_vclip >= Num_vclips || Num_vclips >= VCLIP_MAXNUM)
		return;
	v = &Vclip[vulcan_powerup_vclip];
	if (v->num_frames <= 0)
		return;
	Vclip[Num_vclips] = *v;
	for (i = 0; i < GAUSS_FRAMES; i++)
	{
		char name[16];
		bitmap_index bi;

		snprintf(name, sizeof(name), "gauss#%d", i);
		bi = gauss_add_bitmap(v->frames[i % v->num_frames], name);
		if (!bi.index)
			return;
		Vclip[Num_vclips].frames[i] = bi;
	}
	Vclip[Num_vclips].num_frames = GAUSS_FRAMES;
	Vclip[Num_vclips].play_time = v->frame_time * GAUSS_FRAMES;
	gauss_powerup_vclip = Num_vclips++;
}

void gauss_init_assets(void)
{
	vulcan_original = Weapon_info[VULCAN_ID];
	vulcan_saved = 1;
	gauss_add_powerup_vclip();
	gauss_picture = gauss_add_bitmap(Weapon_info[VULCAN_ID].picture, "gauge21b");
	gauss_sound = custom_sound_load("gauss.wav", 0);
}

void gauss_apply(void)
{
	weapon_info *w = &Weapon_info[VULCAN_ID];
	int i;

	if (!vulcan_saved)
		return;
	*w = vulcan_original;
	if (vulcan_powerup_vclip >= 0)
		Powerup_info[POW_VULCAN_WEAPON].vclip_num = vulcan_powerup_vclip;

	if (gauss_mode())
	{
		// D2's Gauss (descent2.ham weapon 32) with D2's "Disable Gauss Splash" changes: no splash,
		// 12.5 damage and a small push instead of 25 damage, splash radius 10 and mass 0.2.
		w->fire_wait = fl2f(0.15);
		w->ammo_usage = 2;
		for (i = 0; i < NDL; i++)
		{
			w->strength[i] = i2f(12) + F0_5;
			w->speed[i] = i2f(700);
		}
		w->mass = F1_0 / 20;
		w->damage_radius = 0;
		w->impact_size = fl2f(3.5);
		w->lifetime = i2f(3);
		if (GAUSS_DRONE_ROBOT < N_robot_types)
		{
			int id = Robot_info[GAUSS_DRONE_ROBOT].weapon_type;

			if (id >= 0 && id < N_weapon_types)
			{
				w->wall_hit_vclip = Weapon_info[id].wall_hit_vclip;
				w->wall_hit_sound = Weapon_info[id].wall_hit_sound;
				w->robot_hit_vclip = Weapon_info[id].robot_hit_vclip;
				w->robot_hit_sound = Weapon_info[id].robot_hit_sound;
			}
		}
		if (gauss_sound >= 0)
			w->flash_sound = gauss_sound;
		if (gauss_picture.index)
			w->picture = gauss_picture;
		if (gauss_powerup_vclip >= 0)
			Powerup_info[POW_VULCAN_WEAPON].vclip_num = gauss_powerup_vclip;
	}

	// Vulcan pickups already in the level take the look for this game
	for (i = 0; i <= Highest_object_index; i++)
	{
		object *o = &Objects[i];

		if (o->type == OBJ_POWERUP && o->id == POW_VULCAN_WEAPON && o->render_type == RT_POWERUP &&
		    Powerup_info[POW_VULCAN_WEAPON].vclip_num >= 0)
		{
			o->rtype.vclip_info.vclip_num = Powerup_info[POW_VULCAN_WEAPON].vclip_num;
			o->rtype.vclip_info.framenum = 0;
			o->rtype.vclip_info.frametime = Vclip[o->rtype.vclip_info.vclip_num].frame_time;
		}
	}
}

// D2's maybe_gauss_fireball: with no splash, a Gauss hit on a ship, reactor or debris still shows the fireball
void gauss_fireball(object *weapon, vms_vector *pos)
{
	if (weapon->id == VULCAN_ID && gauss_mode() && Weapon_info[VULCAN_ID].robot_hit_vclip >= 0)
		object_create_explosion(weapon->segnum, pos, Weapon_info[VULCAN_ID].impact_size, Weapon_info[VULCAN_ID].robot_hit_vclip);
}
