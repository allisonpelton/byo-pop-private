#ifndef __SEXY_PORTCOMPAT_H__
#define __SEXY_PORTCOMPAT_H__

// Minimal stand-ins for the Win32 types/functions the original SexyAppFramework sources use, so the
// platform-neutral parts compile unchanged on SDL2 desktop builds and Emscripten.

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cwctype>
#include <cwchar>
#include <string>

typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef int BOOL;
typedef void* HANDLE;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef __forceinline
#if defined(_MSC_VER)
#else
#define __forceinline inline __attribute__((always_inline))
#endif
#endif

#ifndef _MSC_VER
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _snwprintf swprintf
#define _vsnwprintf vswprintf
#define _strdup strdup
#endif

#define _stricmp sexy_stricmp
#define stricmp sexy_stricmp
#define _strnicmp sexy_strnicmp
#define strnicmp sexy_strnicmp
#define _wcsicmp sexy_wcsicmp
#define wcsicmp sexy_wcsicmp

inline int sexy_stricmp(const char* a, const char* b)
{
	for (;; a++, b++)
	{
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
		if (ca != cb || !ca)
			return ca - cb;
	}
}

inline int sexy_strnicmp(const char* a, const char* b, size_t n)
{
	for (size_t i = 0; i < n; i++, a++, b++)
	{
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
		if (ca != cb || !ca)
			return ca - cb;
	}
	return 0;
}

inline int sexy_wcsicmp(const wchar_t* a, const wchar_t* b)
{
	for (;; a++, b++)
	{
		wint_t ca = towlower(*a), cb = towlower(*b);
		if (ca != cb || !ca)
			return (int)ca - (int)cb;
	}
}

namespace Sexy
{
// Milliseconds since an arbitrary epoch (replacement for timeGetTime/GetTickCount). port: this is GAME time, the original's
// clock as the game would have seen it at the chosen game speed (SetGameClock): real time scaled by the speed, or the -fast
// virtual clock. At speed 1 without -fast it is real time, as before. Everything that paces the framework itself (draws,
// the update accumulator, sleeps) reads GetRealTickCountMS() instead.
DWORD GetTickCountMS();
// The real (wall) clock in the same epoch; unaffected by speed and by -fast.
DWORD GetRealTickCountMS();
// The game clock without the epoch (what GetTickCountMS counts from): 0 at the start of a -fast run.
DWORD GetGameTimeMS();
// Sets what drives the game clock: theVirtualSource, a clock in milliseconds that the caller advances (-fast's and the
// unlimited speed's), or NULL for the real clock; theSpeed is how many game ms pass per source ms. The elapsed game time is
// settled at the old speed first, so the clock stays continuous when the speed changes mid-game. theRestart sets game time 0.
void SetGameClock(const DWORD* theVirtualSource, double theSpeed, bool theRestart = false);
}

#define timeGetTime() Sexy::GetTickCountMS()
#define GetTickCount() Sexy::GetTickCountMS()

#ifndef RGB
#define RGB(r, g, b) ((DWORD)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#endif

#endif
