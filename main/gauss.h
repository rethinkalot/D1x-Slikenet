/*
 * "Vulcan becomes Gauss": with the option on, the Vulcan cannon plays as D2's Gauss (in its
 * "Disable Gauss Splash" form). Multiplayer follows the host's NETGAME_FLAG_GAUSS, single player
 * PlayerCfg.SPGauss. The original PIG is never changed: the art comes from gauss#0-15.png,
 * gauge21b.png and gauss.ase, the firing sound from gauss.wav, each falling back to the Vulcan's.
 */
#ifndef _GAUSS_H
#define _GAUSS_H

#include "vecmat.h"
#include "object.h"

int gauss_mode(void);                // is the Vulcan a Gauss in this game?
void gauss_init_assets(void);        // once, after the game data and sounds are read
void gauss_apply(void);              // at each level start: Vulcan stats, look and sound for this game
void gauss_fireball(object *weapon, vms_vector *pos); // a Gauss hit on a ship, reactor or debris: picture only
int gauss_png_always(const char *bitmapname);         // Gauss art loads even with custom textures off

#endif
