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
// Milliseconds since an arbitrary epoch (replacement for timeGetTime/GetTickCount).
DWORD GetTickCountMS();
}

#define timeGetTime() Sexy::GetTickCountMS()
#define GetTickCount() Sexy::GetTickCountMS()

#ifndef RGB
#define RGB(r, g, b) ((DWORD)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#endif

#endif
