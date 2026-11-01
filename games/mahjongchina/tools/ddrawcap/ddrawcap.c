// A stand-in for DDRAW.dll, put beside a copy of MahjongEscapeAC.exe (never an install) for runs inside Windows
// Sandbox, so that origrun's PrintWindow shots show the game. Ancient Japan's window class has no background brush;
// Ancient China's has the black stock brush (0x4295a0). Inside the sandbox PrintWindow erases the client with that
// brush over the frame, so every shot of China's original came out black, windowed or full screen. The proxy takes the
// brush away from the class once the game has made its window, and otherwise passes every call to the real DirectDraw.
// Nothing the game draws changes: it draws its whole client every frame and never relies on the erase.
//
// The game loads DirectDraw itself (LoadLibrary "DDRAW.DLL", then DirectDrawCreateEx or DirectDrawCreate, in
// 0x42cc20), after it made its window, so the copy in the game's folder is the one it gets; the real one comes from the
// system folder. What happened goes to ddrawcap.txt beside the exe.
//
//   i686-w64-mingw32-clang -O2 -shared ddrawcap.c ddrawcap.def -o DDRAW.dll -luser32
#include <windows.h>
#include <stdio.h>

typedef HRESULT(WINAPI* CreateFn)(GUID*, void**, IUnknown*);
typedef HRESULT(WINAPI* CreateExFn)(GUID*, void**, REFIID, IUnknown*);

static HMODULE gReal;
static FILE* gLog;

static void Log(const char* theFormat, ...)
{
	if (!gLog)
	{
		char aPath[MAX_PATH];
		GetModuleFileNameA(NULL, aPath, MAX_PATH);
		char* aSlash = strrchr(aPath, '\\');
		strcpy(aSlash ? aSlash + 1 : aPath, "ddrawcap.txt");
		gLog = fopen(aPath, "w");
		if (!gLog)
			return;
	}
	va_list anArgs;
	va_start(anArgs, theFormat);
	vfprintf(gLog, theFormat, anArgs);
	fputc('\n', gLog);
	fflush(gLog);
	va_end(anArgs);
}

static BOOL CALLBACK TakeBrush(HWND theWnd, LPARAM theParam)
{
	char aClass[64] = "";
	GetClassNameA(theWnd, aClass, sizeof(aClass));
	HBRUSH aBrush = (HBRUSH)SetClassLongA(theWnd, GCL_HBRBACKGROUND, 0);
	Log("window %p (class %s): its class brush %p taken away", (void*)theWnd, aClass, (void*)aBrush);
	return TRUE;
}

static FARPROC Real(const char* theName)
{
	if (!gReal)
	{
		char aPath[MAX_PATH];
		GetSystemDirectoryA(aPath, MAX_PATH);
		strcat(aPath, "\\ddraw.dll");
		gReal = LoadLibraryA(aPath);
		Log("real DirectDraw: %s (%p)", aPath, (void*)gReal);
		EnumThreadWindows(GetCurrentThreadId(), TakeBrush, 0);
	}
	return gReal ? GetProcAddress(gReal, theName) : NULL;
}

HRESULT WINAPI ProxyCreateEx(GUID* theGuid, void** theDD, REFIID theIid, IUnknown* theOuter)
{
	CreateExFn aFn = (CreateExFn)Real("DirectDrawCreateEx");
	return aFn ? aFn(theGuid, theDD, theIid, theOuter) : E_FAIL;
}

HRESULT WINAPI ProxyCreate(GUID* theGuid, void** theDD, IUnknown* theOuter)
{
	CreateFn aFn = (CreateFn)Real("DirectDrawCreate");
	return aFn ? aFn(theGuid, theDD, theOuter) : E_FAIL;
}
