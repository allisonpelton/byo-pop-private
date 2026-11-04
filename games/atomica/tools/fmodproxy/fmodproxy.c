// A logging stand-in for FMOD 3's fmod.dll (Atomica Deluxe), after games/bigmoney/tools/bassproxy: put beside a copy of Atomica.exe
// (never an install) with the real fmod.dll renamed to fmod_real.dll; every call of the 25 functions the game imports is written to
// fmodlog.txt in the same folder (the game's update counter at 0x4d0048, the arguments, the result, and the name of a song that
// is loaded), then passed on unchanged. Each export is a thunk that needs no signature (see bassproxy.c).
//
// Build (32-bit): i686-w64-mingw32-clang -O2 -shared fmodproxy.c fmodproxy.def -o fmod.dll
#include <windows.h>
#include <stdio.h>

#define COUNT 25
static const char* const kNames[COUNT] = {
	"_FSOUND_Init@12",
	"_FSOUND_SetHWND@4",
	"_FSOUND_Close@0",
	"_FSOUND_Sample_Load@16",
	"_FSOUND_Sample_Alloc@28",
	"_FSOUND_Sample_Free@4",
	"_FSOUND_Sample_GetLength@4",
	"_FSOUND_Sample_GetMode@4",
	"_FSOUND_Sample_GetDefaults@20",
	"_FSOUND_Sample_Lock@28",
	"_FSOUND_Sample_Unlock@20",
	"_FSOUND_Sample_SetLoopPoints@12",
	"_FSOUND_SetLoopMode@8",
	"_FSOUND_PlaySound@8",
	"_FSOUND_StopSound@4",
	"_FSOUND_IsPlaying@4",
	"_FSOUND_SetVolume@8",
	"_FSOUND_SetPaused@8",
	"_FMUSIC_LoadSong@4",
	"_FMUSIC_PlaySong@4",
	"_FMUSIC_StopSong@4",
	"_FMUSIC_SetOrder@8",
	"_FMUSIC_SetMasterVolume@8",
	"_FMUSIC_SetPaused@8",
	"_FMUSIC_IsPlaying@4"
};
enum
{
	kLoadSample = 3,
	kLoadSong = 18
};

static FARPROC gReal[COUNT];
static FILE* gLog;
static CRITICAL_SECTION gLock;
static LARGE_INTEGER gFreq, gStart;
static DWORD gTls;

typedef struct
{
	DWORD mRet[64];
	int mIndex[64];
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

static unsigned Ms(void)
{
	if (!IsBadReadPtr((void*)0x4d0048, 4))
		return *(volatile unsigned*)0x4d0048;
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return (unsigned)((t.QuadPart - gStart.QuadPart) * 1000 / gFreq.QuadPart);
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
	}
	f->mDepth++;
	DWORD* a = theFrame + 2;
	EnterCriticalSection(&gLock);
	if (gLog)
	{
		fprintf(gLog, "%u %lu %s %lx %lx %lx %lx %lx %lx", Ms(), GetCurrentThreadId(), kNames[i], a[0], a[1], a[2],
			a[3], a[4], a[5]);
		if (i == kLoadSong && a[0] != 0 && !IsBadStringPtrA((const char*)a[0], MAX_PATH))
			fprintf(gLog, " file %s", (const char*)a[0]);
		if (i == kLoadSample && a[1] != 0 && !IsBadStringPtrA((const char*)a[1], MAX_PATH))
			fprintf(gLog, " file %s", (const char*)a[1]);
		fprintf(gLog, "\n");
		fflush(gLog);
	}
	LeaveCriticalSection(&gLock);
	return gReal[i];
}

// cdecl: logs the result and returns the caller's return address
DWORD __cdecl ProxyLeave(DWORD* theResult)
{
	Frames* f = GetFrames();
	f->mDepth--;
	int d = f->mDepth < 64 ? f->mDepth : 63;
	int i = f->mIndex[d];
	EnterCriticalSection(&gLock);
	if (gLog)
	{
		fprintf(gLog, "%u %lu = %s %lx\n", Ms(), GetCurrentThreadId(), kNames[i], *theResult);
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
THUNK(0)
THUNK(1)
THUNK(2)
THUNK(3)
THUNK(4)
THUNK(5)
THUNK(6)
THUNK(7)
THUNK(8)
THUNK(9)
THUNK(10)
THUNK(11)
THUNK(12)
THUNK(13)
THUNK(14)
THUNK(15)
THUNK(16)
THUNK(17)
THUNK(18)
THUNK(19)
THUNK(20)
THUNK(21)
THUNK(22)
THUNK(23)
THUNK(24)

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
	wsprintfA(aFile, "%sfmodlog.txt", aPath);
	gLog = fopen(aFile, "w");
	wsprintfA(aFile, "%sfmod_real.dll", aPath);
	HMODULE aReal = LoadLibraryA(aFile);
	if (gLog)
		fprintf(gLog, "# fmod proxy: %s %s\n", aFile, aReal ? "loaded" : "NOT LOADED");
	for (int i = 0; i < COUNT; i++)
		gReal[i] = aReal ? GetProcAddress(aReal, kNames[i]) : NULL;
	return TRUE;
}
