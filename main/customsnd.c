/*
 * Extra sounds from WAV files (see customsnd.h).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pstypes.h"
#include "u_mem.h"
#include "console.h"
#include "physfsx.h"
#include "gr.h"
#include "piggy.h"
#include "sounds.h"
#include "digi.h"
#include "customsnd.h"

#define CUSTOM_SOUND_RATE	11025	// D1's own sounds are 8-bit mono 11 kHz

extern int Num_sound_files; // piggy.c

int Sound_slikenet_msg = -1;

// Read 'name' from the search path into memory.
static ubyte *custom_sound_read(const char *name, int *len)
{
	char path[64];
	PHYSFS_file *fp = PHYSFSX_openReadBuffered(name);
	ubyte *buf;

	if (!fp)
	{
		snprintf(path, sizeof(path), "Sounds/%s", name);
		fp = PHYSFSX_openReadBuffered(path);
	}
	if (!fp)
		return NULL;
	*len = PHYSFS_fileLength(fp);
	buf = *len > 0 ? d_malloc(*len) : NULL;
	if (buf && PHYSFS_read(fp, buf, 1, *len) != *len)
	{
		d_free(buf);
		buf = NULL;
	}
	PHYSFS_close(fp);
	return buf;
}

static unsigned rd16(const ubyte *p) { return p[0] | (p[1] << 8); }
static unsigned rd32(const ubyte *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }

// Find the PCM format and samples in a RIFF WAVE file. Takes plain and "extensible" headers and skips any
// extra chunks (SDL 1.2's reader turns those down as "complex"). Returns 0 if it is not 8 or 16-bit PCM.
static int custom_sound_parse(const ubyte *f, int len, int *channels, int *rate, int *bytes, const ubyte **data, int *frames)
{
	int o = 12, have_fmt = 0, data_len = 0;

	*data = NULL;
	if (len < 12 || memcmp(f, "RIFF", 4) || memcmp(f + 8, "WAVE", 4))
		return 0;
	while (o + 8 <= len)
	{
		unsigned size = rd32(f + o + 4);
		const ubyte *c = f + o + 8;

		if (size > (unsigned)(len - o - 8))
			size = len - o - 8;
		if (!memcmp(f + o, "fmt ", 4) && size >= 16)
		{
			unsigned tag = rd16(c);

			if (tag == 0xFFFE && size >= 26)
				tag = rd16(c + 24); // extensible: the real format is the sub-format's first two bytes
			if (tag != 1)
				return 0;
			*channels = rd16(c + 2);
			*rate = rd32(c + 4);
			*bytes = rd16(c + 14) / 8;
			have_fmt = 1;
		}
		else if (!memcmp(f + o, "data", 4))
		{
			*data = c;
			data_len = size;
		}
		o += 8 + size + (size & 1);
	}
	if (!have_fmt || !*data || *channels < 1 || *rate <= 0 || (*bytes != 1 && *bytes != 2))
		return 0;
	*frames = data_len / (*bytes * *channels);
	return *frames > 0;
}

// One sample as signed 16-bit (8-bit WAV samples are unsigned, 16-bit ones signed little-endian)
static int custom_sound_sample(const ubyte *p, int bytes)
{
	return bytes == 1 ? ((int)p[0] - 128) * 256 : (short)rd16(p);
}

int custom_sound_load(const char *name, int loud)
{
	ubyte *file;
	const ubyte *wav;
	int file_len = 0, channels, rate, bytes, frame, frames, out_len, i, num, peak = 1;
	int *mono;
	double step;
	digi_sound snd;

	if (!(file = custom_sound_read(name, &file_len)))
		return -1;
	if (!custom_sound_parse(file, file_len, &channels, &rate, &bytes, &wav, &frames))
	{
		con_printf(CON_NORMAL, "%s: unsupported WAV (use 8 or 16-bit PCM)\n", name);
		d_free(file);
		return -1;
	}
	frame = bytes * channels;
	for (num = MAX_SOUNDS - 1; num >= 0 && Sounds[num] != 255; num--) // a game sound number nothing uses
		;
	if (num < 0 || Num_sound_files >= MAX_SOUND_FILES)
	{
		d_free(file);
		return -1;
	}

	// Down-mix to mono and resample, averaging the source frames each output sample covers
	step = (double)rate / CUSTOM_SOUND_RATE;
	out_len = (int)(frames / step);
	if (out_len < 1)
		out_len = 1;
	snd.data = d_malloc(out_len);
	mono = d_malloc(out_len * sizeof(int));
	for (i = 0; i < out_len; i++)
	{
		int a = (int)(i * step), b = (int)((i + 1) * step), j, c;
		long sum = 0, count = 0;

		if (b <= a)
			b = a + 1;
		if (b > frames)
			b = frames;
		for (j = a; j < b; j++)
			for (c = 0; c < channels; c++, count++)
				sum += custom_sound_sample(wav + j * frame + c * bytes, bytes);
		mono[i] = count ? sum / count : 0;
		if (abs(mono[i]) > peak)
			peak = abs(mono[i]);
	}
	for (i = 0; i < out_len; i++)
	{
		long v = loud ? (long)((double)mono[i] * 29000 / peak) : mono[i]; // 'loud': the peak goes to ~90% of full scale
		v = v / 256 + 128;
		snd.data[i] = v < 0 ? 0 : v > 255 ? 255 : v;
	}
	d_free(mono);
	d_free(file);
	snd.bits = 8;
	snd.freq = CUSTOM_SOUND_RATE;
	snd.length = out_len;
	{
		char short_name[9]; // the sound list keeps 8-character names, like the PIG's

		snprintf(short_name, sizeof(short_name), "%.*s", (int)strcspn(name, "."), name);
		Sounds[num] = AltSounds[num] = piggy_register_sound(&snd, short_name, 1);
	}
	return num;
}

void custom_sounds_init(void)
{
	Sound_slikenet_msg = custom_sound_load("slikenetmsg.wav", 1); // a quiet recording, so raise it
}

// HD sounds: D2's 22 kHz recordings of the sounds D1 shares with D2, converted to 44.1 kHz 16-bit mono,
// replace D1's 8-bit 11 kHz originals. Matched by sound number through the robot, weapon and named-sound
// tables of both games. Each file is Sounds/HD/<D2 name>.wav (in D1-SFX.DXA); a missing file keeps D1's own.
// The reactor countdown voices are left out on purpose: D2 renumbered them.
static const struct { short id; const char *name; } hd_sound_map[] = {
	{ 10, "laser03" }, { 11, "explode1" }, { 12, "laser05" }, { 13, "laser03" }, { 14, "laser04" },
	{ 15, "laser02" }, { 16, "laser01" }, { 17, "ramfast" }, { 18, "laser06" }, { 19, "laser12" },
	{ 20, "explode1" }, { 21, "explode1" }, { 22, "laser07d" }, { 23, "flare" }, { 24, "fusionc" },
	{ 25, "plasma3" }, { 26, "dropbomb" }, { 27, "doorhit" }, { 28, "explode1" }, { 30, "explode1" },
	{ 31, "explode2" }, { 32, "siren01" }, { 33, "nuke01" }, { 34, "fusion2a" }, { 62, "power04" },
	{ 70, "ramfast" }, { 71, "shit01" }, { 80, "power03" }, { 81, "power03" }, { 82, "exlife" },
	{ 83, "power03" }, { 91, "gothost" }, { 92, "mtrl01" }, { 93, "mtrl02" }, { 115, "vulcan" },
	{ 117, "message" }, { 118, "death" }, { 121, "fan01alt" }, { 122, "misslock" }, { 130, "missile1" },
	{ 131, "missile1" }, { 132, "missle3c" }, { 140, "opdor02b" }, { 141, "cldoor02" }, { 142, "ddoorop5" },
	{ 143, "ddoorcl8" }, { 144, "opdoor05" }, { 145, "ddoorcl2" }, { 146, "vdoorop5" }, { 147, "vdoorcl5" },
	{ 148, "opdor02b" }, { 149, "cldoor02" }, { 151, "lava" }, { 153, "change1" }, { 154, "change2" },
	{ 155, "message" }, { 156, "beep02" }, { 157, "monitor" }, { 160, "cloakon" }, { 161, "cloakoff" },
	{ 162, "invon" }, { 163, "invoff" }, { 168, "robot19" }, { 169, "robot15" }, { 170, "robot01" },
	{ 171, "thiefbep" }, { 172, "robot03" }, { 173, "robot04" }, { 176, "robot07" }, { 178, "fourclaw" },
	{ 180, "minotaur" }, { 181, "robot12" }, { 183, "boss3seb" }, { 184, "boss1_2" }, { 185, "boss02" },
	{ 186, "boss2_1" }, { 187, "boss2_2" }, { 188, "bossdied" }, { 190, "tear01" }, { 200, "cheater" },
	{ 201, "robot17" }, { 202, "robot16" }, { 203, "robot19" }, { 204, "robot20" }, { 205, "robot21" },
	{ 209, "robot25" }, { 211, "sloth" }, { 219, "frog" },
};

extern int digi_using_mixer(void);

void hd_sounds_init(void)
{
	ubyte done[MAX_SOUND_FILES];
	int i, n = 0;

	if (!digi_using_mixer()) // only SDL_mixer plays 16-bit sounds
		return;
	memset(done, 0, sizeof(done));
	for (i = 0; i < (int)(sizeof(hd_sound_map) / sizeof(hd_sound_map[0])); i++)
	{
		int f = Sounds[hd_sound_map[i].id], file_len = 0, channels, rate, bytes, frames, k;
		char path[64];
		ubyte *file;
		const ubyte *wav;
		short *pcm;

		if (f == 255 || f >= MAX_SOUND_FILES || done[f])
			continue;
		snprintf(path, sizeof(path), "HD/%s.wav", hd_sound_map[i].name);
		if (!(file = custom_sound_read(path, &file_len)))
			continue;
		if (!custom_sound_parse(file, file_len, &channels, &rate, &bytes, &wav, &frames))
		{
			d_free(file);
			continue;
		}
		pcm = d_malloc(frames * sizeof(short)); // mono 16-bit at the file's own rate
		for (k = 0; k < frames; k++)
		{
			int c, sum = 0;
			for (c = 0; c < channels; c++)
				sum += custom_sound_sample(wav + (k * channels + c) * bytes, bytes);
			pcm[k] = sum / channels;
		}
		d_free(file);
		GameSounds[f].data = (ubyte *)pcm; // the old data may live inside the PIG buffer, so it is left alone
		GameSounds[f].length = frames * sizeof(short);
		GameSounds[f].bits = 16;
		GameSounds[f].freq = rate;
		done[f] = 1;
		n++;
	}
	if (n)
		con_printf(CON_NORMAL, "HD sounds: %d D1 sounds replaced with D2's higher quality recordings\n", n);
}
