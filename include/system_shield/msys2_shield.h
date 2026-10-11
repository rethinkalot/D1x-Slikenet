#ifndef MSYS2_SYSTEM_SHIELD_H
#define MSYS2_SYSTEM_SHIELD_H

// If any prior track or header cache turned TCHAR into a macro, kill it immediately
#ifdef TCHAR
#undef TCHAR
#endif

#ifdef _TCHAR_DEFINED
#undef _TCHAR_DEFINED
#endif

#ifdef __TCHAR_DEFINED
#undef __TCHAR_DEFINED
#endif

// Hard-lock the Win32 lean environment parameters
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// Prevent downstream files from hijacking standard string macros
#define _TCHAR_DEFINED
#define __TCHAR_DEFINED

#endif
