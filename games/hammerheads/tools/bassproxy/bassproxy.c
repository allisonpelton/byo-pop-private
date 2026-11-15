// A logging stand-in for BASS.dll (Hammer Heads Deluxe's BASS, the same 21 functions as Big Money's, from SexyAppFramework 1.2's
// BassLoader; after games/bigmoney/tools/bassproxy), put beside a copy of HammerHeadsGame.exe (never an install) with the real
// BASS renamed to bass_real.dll: every BASS call the game makes is written to basslog.txt in the same folder (the update count, the
// arguments, the result, and a CRC of the first 32 KB of the data a music or stream is made from, as the port's log has it), then
// passed on (a stream BASS's no-sound device cannot make gets a handle of the proxy's own, see kStreamCreateFile). Hammer Heads plays only its music through BASS (four .ogg files, made with BASS_StreamCreateFile from
// memory); its sounds go through DirectSound, which origrun's stand-in logs.
//
// Each export is a thunk that needs no signature: it logs the first six words on the stack, calls the real function
// with the caller's arguments in place (BASS is stdcall, so the real function pops them) and logs the result on the
// way back. The caller's return address waits on a per-thread stack, since a sync callback may call BASS again.
//
// Build (32-bit): i686-w64-mingw32-clang -O2 -shared bassproxy.c bassproxy.def -o build/bassproxy-hh/bass.dll
#include <windows.h>
#include <stdio.h>

#define COUNT 21
static const char* const kNames[COUNT] = {
	"BASS_ChannelIsActive", "BASS_ChannelPause", "BASS_ChannelResume", "BASS_ChannelSetAttributes",
	"BASS_ChannelSetPosition", "BASS_ChannelStop", "BASS_Free", "BASS_Init",
	"BASS_MusicFree", "BASS_MusicLoad", "BASS_MusicPlay", "BASS_MusicPlayEx",
	"BASS_MusicSetAmplify", "BASS_SetConfig", "BASS_SetGlobalVolumes", "BASS_SetVolume",
	"BASS_Start", "BASS_Stop", "BASS_StreamCreateFile", "BASS_StreamFree",
	"BASS_StreamPlay",
};
enum
{
	kIsActive = 0, kPause = 1, kResume = 2, kSetAttributes = 3, kSetPosition = 4, kChannelStop = 5, kInit = 7, kMusicLoad = 9,
	kStreamCreateFile = 18, kStreamFree = 19, kStreamPlay = 20
};

// BASS's "no sound" device cannot make a stream (error 37), so a stream it could not make gets a handle of the proxy's own,
// 0x5c000000 + n, and the calls on it are answered as BASS would answer for a stream that plays: the state
// BASS_ChannelIsActive gives is 0 stopped, 1 playing, 3 paused, and a stand-in stream never ends by itself.
#define FAKE_STREAM 0x5c000000
#define MAX_FAKE 256
static int gFakeCount;
static DWORD gFakeState[MAX_FAKE];
static int FakeIndex(DWORD theHandle)
{
	int n = (int)(theHandle & 0xffffff);
	return (theHandle & 0xff000000) == FAKE_STREAM && n < gFakeCount ? n : -1;
}

static FARPROC gReal[COUNT];
static FILE* gLog;
static CRITICAL_SECTION gLock;
static LARGE_INTEGER gFreq, gStart;
static DWORD gTls;
static char gDir[MAX_PATH];      // the game's folder (the DLL's): a relative file name is made whole against it
static char gPath[MAX_PATH];
static FARPROC gErrorCode;       // the real BASS_ErrorGetCode, to log why a call failed
static int gNoSound;      // nosound.txt beside the DLL: BASS_Init is given device 0 (BASS's "no sound" device) wherever it asked for a real one

typedef struct
{
	DWORD mRet[64];
	int mIndex[64];
	DWORD mArg0[64];
	int mDepth;
} Frames;

static Frames* GetFrames(void)
{
	Frames* f = (Frames*)TlsGetValue(gTls);
	if (f == NULL)
	{
		f = (Frames*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Frames));
		TlsSetValue(gTls, f);
	}
	return f;
}

static DWORD Crc32(const BYTE* theData, DWORD theSize)
{
	DWORD aCrc = 0xffffffff;
	for (DWORD i = 0; i < theSize; i++)
	{
		aCrc ^= theData[i];
		for (int k = 0; k < 8; k++)
			aCrc = (aCrc >> 1) ^ (0xedb88320 & (0 - (aCrc & 1)));
	}
	return ~aCrc;
}

// The log's time is the game's update count (the application's counter, +0x44c of the application the pointer at 0x5cea54 points
// to): origrun's virtual clock does not reach this DLL's own clock calls, and the counter is what the port's log and the state
// runs are timed by. A call made before the application exists, or from a thread that is not the game's, shows the count it finds.
static unsigned Ms(void)
{
	const void* const* aGlobal = (const void* const*)0x5cea54;
	if (IsBadReadPtr(aGlobal, 4) || *aGlobal == NULL)
		return 0u;
	const volatile int* aCounter = (const volatile int*)((const char*)*aGlobal + 0x44c);
	return IsBadReadPtr((const void*)aCounter, 4) ? 0u : (unsigned)*aCounter;
}

// cdecl, from the thunk: theFrame[0] the function's index, [1] the caller's return address, [2..] the arguments.
// Returns the real function's address.
FARPROC __cdecl ProxyEnter(DWORD* theFrame)
{
	int i = (int)theFrame[0];
	Frames* f = GetFrames();
	if (f->mDepth < 64)
	{
		f->mRet[f->mDepth] = theFrame[1];
		f->mIndex[f->mDepth] = i;
		f->mArg0[f->mDepth] = theFrame[2];
	}
	f->mDepth++;
	DWORD* a = theFrame + 2;
	EnterCriticalSection(&gLock);
	if (i == kInit && gNoSound && a[0] != 0)
	{
		if (gLog)
			fprintf(gLog, "# BASS_Init: device %ld becomes 0 (no sound)\n", (long)a[0]);
		a[0] = 0;
	}
	// the game runs with its own folder as the working directory; origrun's runs may not, so a music file's relative name is
	// made whole against the game's folder (the same file the game means)
	if ((i == kMusicLoad || i == kStreamCreateFile) && a[0] == 0 && a[1] != 0 && !IsBadStringPtrA((const char*)a[1], MAX_PATH))
	{
		const char* aName = (const char*)a[1];
		if (aName[0] != '\\' && aName[1] != ':')
		{
			wsprintfA(gPath, "%s%s", gDir, aName);
			a[1] = (DWORD)(size_t)gPath;
		}
	}
	if (gLog)
	{
		fprintf(gLog, "%u %lu %s %lx %lx %lx %lx %lx %lx", Ms(), GetCurrentThreadId(), kNames[i], a[0], a[1], a[2],
			a[3], a[4], a[5]);
		// BASS_MusicLoad from memory: (TRUE, data, offset, length, ...); the CRC covers the first 32 KB, as the
		// port's side does
		if ((i == kMusicLoad || i == kStreamCreateFile) && a[0] != 0 && a[1] != 0 && !IsBadReadPtr((void*)a[1], 0x8000))
			fprintf(gLog, " crc %08lx len %lu", Crc32((const BYTE*)a[1], 0x8000), 0x8000UL);
		// from a file: (FALSE, name, offset, length, ...)
		else if ((i == kMusicLoad || i == kStreamCreateFile) && a[0] == 0 && a[1] != 0 && !IsBadStringPtrA((const char*)a[1], MAX_PATH))
			fprintf(gLog, " file %s", (const char*)a[1]);
		fprintf(gLog, "\n");
		fflush(gLog);
	}
	LeaveCriticalSection(&gLock);
	return gReal[i];
}

// cdecl: answers for a stand-in stream, logs the result and returns the caller's return address
DWORD __cdecl ProxyLeave(DWORD* theResult)
{
	Frames* f = GetFrames();
	f->mDepth--;
	int d = f->mDepth < 64 ? f->mDepth : 63;
	int i = f->mIndex[d];
	EnterCriticalSection(&gLock);
	int aRealError = 0;
	if (*theResult == 0 && (i == kMusicLoad || i == kStreamCreateFile) && gErrorCode != NULL)
		aRealError = (int)((int (WINAPI*)(void))gErrorCode)();
	int n = FakeIndex(f->mArg0[d]);
	int isStandIn = n >= 0;
	if (i == kStreamCreateFile && *theResult == 0 && gFakeCount < MAX_FAKE)
	{
		gFakeState[gFakeCount] = 0;
		*theResult = FAKE_STREAM + gFakeCount++;
		isStandIn = 1;
	}
	else if (n >= 0)
	{
		if (i == kIsActive)
			*theResult = gFakeState[n];
		else
		{
			if (i == kStreamPlay)
				gFakeState[n] = 1;
			else if (i == kPause && gFakeState[n] == 1)
				gFakeState[n] = 3;
			else if (i == kResume && gFakeState[n] == 3)
				gFakeState[n] = 1;
			else if (i == kChannelStop)
				gFakeState[n] = 0;
			*theResult = 1;
		}
	}
	if (gLog)
	{
		fprintf(gLog, "%u %lu = %s %lx%s", Ms(), GetCurrentThreadId(), kNames[i], *theResult, isStandIn ? " (stand-in)" : "");
		if (aRealError != 0)
			fprintf(gLog, " error %d", aRealError);
		fprintf(gLog, "\n");
		fflush(gLog);
	}
	LeaveCriticalSection(&gLock);
	return f->mRet[d];
}

__asm__(
	".text\n"
	"_ProxyCommon:\n"
	"	pushl %esp\n"
	"	call _ProxyEnter\n"
	"	addl $4, %esp\n"
	"	addl $8, %esp\n"         // the index and the caller's return address (kept by ProxyEnter)
	"	call *%eax\n"            // the real function pops the arguments
	"	pushl %eax\n"            // the result
	"	pushl %esp\n"
	"	call _ProxyLeave\n"
	"	addl $4, %esp\n"
	"	movl %eax, %ecx\n"
	"	popl %eax\n"
	"	pushl %ecx\n"
	"	ret\n");

#define THUNK(n) \
	__asm__(".globl _Thunk" #n "\n_Thunk" #n ":\n	pushl $" #n "\n	jmp _ProxyCommon\n");
THUNK(0) THUNK(1) THUNK(2) THUNK(3) THUNK(4) THUNK(5) THUNK(6) THUNK(7) THUNK(8) THUNK(9) THUNK(10) THUNK(11)
THUNK(12) THUNK(13) THUNK(14) THUNK(15) THUNK(16) THUNK(17) THUNK(18) THUNK(19) THUNK(20)

BOOL WINAPI DllMain(HINSTANCE theInstance, DWORD theReason, LPVOID theReserved)
{
	if (theReason != DLL_PROCESS_ATTACH)
		return TRUE;
	InitializeCriticalSection(&gLock);
	gTls = TlsAlloc();
	QueryPerformanceFrequency(&gFreq);
	QueryPerformanceCounter(&gStart);
	char aPath[MAX_PATH];
	DWORD n = GetModuleFileNameA(theInstance, aPath, MAX_PATH);
	while (n > 0 && aPath[n - 1] != '\\')
		n--;
	aPath[n] = 0;
	char aFile[MAX_PATH];
	lstrcpyA(gDir, aPath);
	wsprintfA(aFile, "%snosound.txt", aPath);
	gNoSound = GetFileAttributesA(aFile) != INVALID_FILE_ATTRIBUTES;
	wsprintfA(aFile, "%sbasslog.txt", aPath);
	gLog = fopen(aFile, "w");
	wsprintfA(aFile, "%sbass_real.dll", aPath);
	HMODULE aReal = LoadLibraryA(aFile);
	if (gLog)
		fprintf(gLog, "# bass proxy: %s %s\n", aFile, aReal ? "loaded" : "NOT LOADED");
	gErrorCode = aReal ? GetProcAddress(aReal, "BASS_ErrorGetCode") : NULL;
	for (int i = 0; i < COUNT; i++)
		gReal[i] = aReal ? GetProcAddress(aReal, kNames[i]) : NULL;
	return TRUE;
}
