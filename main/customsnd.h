/*
 * Extra sounds from WAV files (gauss.wav, slikenetmsg.wav) that are not in the game data.
 */
#ifndef _CUSTOMSND_H
#define _CUSTOMSND_H

// Load a WAV by name from Data or any DXA (root, or Sounds/<name>). Any 8/16-bit PCM, mono or stereo,
// any rate; it is turned into the game's own 8-bit mono format. Returns a game sound number for
// digi_play_sample(), or -1 if the file is missing or unreadable (the caller keeps its usual sound).
// 'loud' raises a quiet recording to near full volume (no clipping).
int custom_sound_load(const char *name, int loud);

extern int Sound_slikenet_msg; // slikenetmsg.wav: SLikeNet link and observer chat lines, -1 if missing
extern int Sound_briefing_printing, Sound_briefing_hum; // br_beep2.wav, de_hum_2.wav: D2's briefing sounds, -1 if missing
void custom_sounds_init(void); // once, after the game sounds are read
void hd_sounds_init(void); // once, after the game sounds are read: D2 quality replacements from Sounds/HD/
// WAV 'name' replaces game sound 'soundnum' at 16 bits: for good if 'fixed', else only with D2 Sound Occlusion on
void hd_sound_attach(int soundnum, const char *name, int fixed);
void hd_sounds_set(int on); // HD Sounds switch: 1 plays the HD versions, 0 the originals

#endif
