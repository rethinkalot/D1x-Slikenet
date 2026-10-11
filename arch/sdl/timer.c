/*
 *
 * SDL library timer functions
 *
 */

#include <SDL.h>

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include "maths.h"
#include "timer.h"
#include "config.h"

static fix64 F64_RunTime = 0;

void timer_update(void)
{
	static ubyte init = 1;
	static fix64 last_tv = 0;
	fix64 cur_tv = SDL_GetTicks()*F1_0/1000;

	if (init)
	{
		last_tv = cur_tv;
		init = 0;
	}

	if (last_tv < cur_tv) // in case SDL_GetTicks wraps, don't update and have a little hickup
		F64_RunTime += (cur_tv - last_tv); // increment! this value will overflow long after we are all dead... so why bother checking?
	last_tv = cur_tv;
}

fix64 timer_query(void)
{
	return (F64_RunTime);
}

// Raise the Windows system timer resolution to 1ms for the lifetime of the
// process.
//
// SDL 1.2 has no SDL_HINT_TIMER_RESOLUTION (that is SDL2), and by default the
// Windows scheduler quantum is ~15.6ms. SDL_Delay() is built on system timers,
// so any frame budget below ~15ms gets rounded up and an FPS cap undershoots
// badly (e.g. a 90 cap delivering ~84fps). timeBeginPeriod(1) is what SDL2's
// SDL_HINT_TIMER_RESOLUTION does internally on Windows.
//
// Called from gr_init() before any timing-sensitive work begins.
void timer_set_high_res(void)
{
#ifdef WIN32
	static int done = 0;

	if (!done)
	{
		timeBeginPeriod(1);
		done = 1;
	}
#else
	// Other platforms already provide a suitably fine timer resolution.
	(void)0;
#endif
}

// High resolution pacing clock.
//
// Deliberately kept SEPARATE from timer_query(). timer_update() only advances
// F64_RunTime when SDL_GetTicks() changes, i.e. it advances in ~1ms steps.
// Frame pacing at 100+fps needs sub-ms resolution to land on its deadline, so
// this uses QueryPerformanceCounter (SDL 1.2 exposes no equivalent of SDL2's
// SDL_GetPerformanceCounter).
//
// Returns a monotonically increasing fix64 count in the same F1_0-based units as
// timer_query(), but with much finer resolution. Because it is a different epoch
// than F64_RunTime, it must ONLY be used to measure *differences* (frame times
// and deadlines) -- never compared against timer_query().
//
// NOTE: do not use this to drive gameplay timing. Netplay (net_udp.c ping),
// AI, demos and menus all key off timer_query(); changing that resolution
// risks demo desync and netplay regressions.
fix64 timer_query_perf(void)
{
#ifdef WIN32
	static LARGE_INTEGER freq;
	LARGE_INTEGER cnt;

	if (!freq.QuadPart)
	{
		if (!QueryPerformanceFrequency(&freq) || !freq.QuadPart)
			return F64_RunTime;		// fall back to the coarse clock
		QueryPerformanceCounter(&cnt);
	}

	QueryPerformanceCounter(&cnt);

	// Split into whole seconds + remainder before scaling. Multiplying the raw
	// counter by F1_0 directly can overflow 64 bits on machines with a long
	// uptime and a 10MHz counter (~6.5e19 vs the ~1.8e19 int64 max).
	return (fix64)((cnt.QuadPart / freq.QuadPart) * (LONGLONG)F1_0)
			+ (fix64)(((LONGLONG)F1_0 * (cnt.QuadPart % freq.QuadPart)) / freq.QuadPart);
#else
	return F64_RunTime;
#endif
}

// Sleep for approximately 'ms' milliseconds without the f2i() truncation that
// timer_delay() suffers. Used by the frame pacer to avoid sleeping to within a
// fraction of a millisecond of the deadline, where quantisation dominates.
void timer_delay_ms(int ms)
{
	if (ms > 0)
		SDL_Delay((Uint32)ms);
}

void timer_delay(fix seconds)
{
	SDL_Delay(f2i(fixmul(seconds, i2f(1000))));
}

// Replacement for timer_delay which considers calc time the program needs between frames (not reentrant)
void timer_delay2(int fps)
{
	static u_int32_t FrameStart=0;
	u_int32_t FrameLoop=0;

	// Throttle to the caller's requested fps.
	//
	// GameCfg.VSync used to select between MAXIMUM_FPS and the passed-in fps, and
	// to decide whether to sleep at all. It never affected vsync (it was never
	// passed to the driver), so all it did was ignore the caller's requested rate
	// and, when set, skip the sleep entirely and spin. Removed: the caller's fps
	// is now always honoured.
	while (FrameLoop < 1000/fps)
	{
		u_int32_t tv_now = SDL_GetTicks();
		if (FrameStart > tv_now)
			FrameStart = tv_now;
		SDL_Delay(1);
		FrameLoop=tv_now-FrameStart;
	}

	FrameStart=SDL_GetTicks();
}
