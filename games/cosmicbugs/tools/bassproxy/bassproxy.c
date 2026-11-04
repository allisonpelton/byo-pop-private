// A logging stand-in for Cosmic Bugs' BASS.dll (after games/bigmoney/tools/bassproxy), put beside a copy of
// CosmicBugs.exe (never an install) with the real BASS renamed to bass_real.dll: every BASS call the game makes is
// written to basslog.txt in the same folder with the game's update counter (0x460a1c, which origrun's virtual clock
// steps), its arguments and its result, then passed on unchanged. The game imports these 14 functions by name and
// plays both its sounds (samples) and its music (one MO3 at a time) through them.
//
// Each export is a thunk that needs no signature: it logs the first six words on the stack, calls the real function
// with the caller's arguments in place (BASS is stdcall, so the real function pops them) and logs the result on the
// way back. The caller's return address waits on a per-thread stack, since a sync callback may call BASS again.
//
// Under origrun BASS cannot open a real device (its DirectSound comes from COM, and BASS_Init fails on the host as in
// Windows Sandbox), and the game then loads no sounds at all. So BASS_Init is given BASS's "no sound" device (0), with
// which the music still loads and plays; BASS_SampleLoad fails on it, and the game would stop on its first sound (its
// vector of samples is empty), so the proxy answers the sample calls itself: every load gets a handle of its own, and
// BASS_SampleGetChannel, BASS_ChannelPlay, BASS_SampleStop and BASS_SampleFree of those handles and their channels
// succeed without reaching BASS. The log shows them as if BASS had answered.
//
// Build (32-bit): i686-w64-mingw32-clang -O2 -shared bassproxy.c bassproxy.def -o build/bassproxy-cb/bass.dll
#include <windows.h>
#include <stdio.h>

#define COUNT 14
static const char* const kNames[COUNT] = {
	"BASS_ChannelPause", "BASS_ChannelPlay", "BASS_Free", "BASS_Init", "BASS_MusicFree", "BASS_MusicLoad",
	"BASS_SampleFree", "BASS_SampleGetChannel", "BASS_SampleLoad", "BASS_SampleStop", "BASS_SetConfig",
	"BASS_SetVolume", "BASS_Start", "BASS_Stop",
};
enum
{
	kChannelPlay = 1, kInit = 3, kMusicLoad = 5, kSampleFree = 6, kSampleGetChannel = 7, kSampleLoad = 8,
	kSampleStop = 9
};

static FARPROC gReal[COUNT];
static FILE* gLog;
static CRITICAL_SECTION gLock;
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

// the game's update counter (the exe has no relocations, so it is always here)
static DWORD Update(void)
{
	return *(volatile DWORD*)0x460a1c;
}

// the proxy's own samples (0xfa0000nn) and their channels (0xfb0000nn)
static DWORD gSamples;

static BOOL IsFake(DWORD theHandle)
{
	return (theHandle >> 24) == 0xfa || (theHandle >> 24) == 0xfb;
}

static DWORD __stdcall FakeSampleLoad(DWORD theMem, DWORD theFile, DWORD theOffset, DWORD theLength, DWORD theMax,
	DWORD theFlags)
{
	return 0xfa000000 | ++gSamples;
}

static DWORD __stdcall FakeSampleGetChannel(DWORD theSample, DWORD theOnlyNew)
{
	return 0xfb000000 | (theSample & 0xffffff);
}

static DWORD __stdcall FakeTwo(DWORD theHandle, DWORD theFlag)
{
	return 1;
}

static DWORD __stdcall FakeOne(DWORD theHandle)
{
	return 1;
}

static FARPROC Fake(int i, DWORD* a)
{
	if (i == kSampleLoad)
		return (FARPROC)FakeSampleLoad;
	if (i == kSampleGetChannel && IsFake(a[0]))
		return (FARPROC)FakeSampleGetChannel;
	if (i == kChannelPlay && IsFake(a[0]))
		return (FARPROC)FakeTwo;
	if ((i == kSampleStop || i == kSampleFree) && IsFake(a[0]))
		return (FARPROC)FakeOne;
	return gReal[i];
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
	if (i == kInit && a[0] != 0)
	{
		if (gLog)
			fprintf(gLog, "# BASS_Init: device %ld becomes 0 (no sound)\n", (long)a[0]);
		a[0] = 0;
	}
	if (gLog)
	{
		fprintf(gLog, "%lu %lu %s %lx %lx %lx %lx %lx %lx", Update(), GetCurrentThreadId(), kNames[i], a[0], a[1],
			a[2], a[3], a[4], a[5]);
		// BASS_SampleLoad and BASS_MusicLoad from a file: (FALSE, name, ...)
		if ((i == kMusicLoad || i == kSampleLoad) && a[0] == 0 && a[1] != 0 &&
			!IsBadStringPtrA((const char*)a[1], MAX_PATH))
			fprintf(gLog, " file %s", (const char*)a[1]);
		fprintf(gLog, "\n");
		fflush(gLog);
	}
	LeaveCriticalSection(&gLock);
	return Fake(i, a);
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
		fprintf(gLog, "%lu %lu = %s %lx\n", Update(), GetCurrentThreadId(), kNames[i], *theResult);
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
THUNK(12) THUNK(13)

BOOL WINAPI DllMain(HINSTANCE theInstance, DWORD theReason, LPVOID theReserved)
{
	if (theReason != DLL_PROCESS_ATTACH)
		return TRUE;
	InitializeCriticalSection(&gLock);
	gTls = TlsAlloc();
	char aPath[MAX_PATH];
	DWORD n = GetModuleFileNameA(theInstance, aPath, MAX_PATH);
	while (n > 0 && aPath[n - 1] != '\\')
		n--;
	aPath[n] = 0;
	char aFile[MAX_PATH];
	wsprintfA(aFile, "%sbasslog.txt", aPath);
	gLog = fopen(aFile, "w");
	wsprintfA(aFile, "%sbass_real.dll", aPath);
	HMODULE aReal = LoadLibraryA(aFile);
	if (gLog)
		fprintf(gLog, "# bass proxy: %s %s\n", aFile, aReal ? "loaded" : "NOT LOADED");
	for (int i = 0; i < COUNT; i++)
		gReal[i] = aReal ? GetProcAddress(aReal, kNames[i]) : NULL;
	return TRUE;
}
