#ifndef D1X_DIGI_MIXER_MUSIC_H
#define D1X_DIGI_MIXER_MUSIC_H

/* Framework audio structure fallbacks to bypass native mixer lookups cleanly */
typedef struct {
    int volume_level;
    int playing_state;
} digi_mixer_music_t;

#endif
