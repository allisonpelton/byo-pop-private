// A logging stand-in for BASS.dll (Bejeweled Twist's BASS 2.2; a copy of games/bookwormadv/tools/bassproxy with the one
// function more that Twist asks for, BASS_ChannelSeconds2Bytes), put beside a copy of popcapgame1.exe (never an install) with the real BASS renamed to bass_real.dll: every BASS call the game makes is written to basslog.txt in the same folder (the time,
// the arguments, the result, and a CRC of the data a sample or a music is loaded from), then passed on unchanged.
// The game loads BASS itself and asks for these 49 functions by name (SexyAppFramework's BassLoader); every one is
// passed on.
//
// Each export is a thunk that needs no signature: it logs the first six words on the stack, calls the real function
// with the caller's arguments in place (BASS is stdcall, so the real function pops them) and logs the result on the
// way back. The caller's return address waits on a per-thread stack, since a sync callback may call BASS again.
//
// BASS 2.2 finds no sound device in Windows Sandbox (origrun's DirectSound stand-in notwithstanding: BASS_Init fails, and
// the game then loads no music at all), so BASS_Init is given BASS's "no sound" device (0), which runs the music. A
// sample BASS could not load gets a handle of the proxy's own, and the calls on it are answered as BASS would answer
// for a channel that plays; Twist loads its samples through DirectSound, not BASS.
#include <windows.h>
#include <stdio.h>

#define COUNT 49
static const char* const kNames[COUNT] = {
	"BASS_ChannelGetAttributes", "BASS_ChannelGetData", "BASS_ChannelGetLength", "BASS_ChannelGetLevel",
	"BASS_ChannelGetPosition", "BASS_ChannelIsActive", "BASS_ChannelIsSliding", "BASS_ChannelPause",
	"BASS_ChannelPlay", "BASS_ChannelPreBuf", "BASS_ChannelRemoveFX", "BASS_ChannelRemoveSync",
	"BASS_ChannelSetAttributes", "BASS_ChannelSetFX", "BASS_ChannelSetFlags", "BASS_ChannelSetPosition",
	"BASS_ChannelSetSync", "BASS_ChannelSlideAttributes", "BASS_ChannelStop", "BASS_ErrorGetCode",
	"BASS_FXGetParameters", "BASS_FXSetParameters", "BASS_Free", "BASS_GetConfig",
	"BASS_GetInfo", "BASS_GetVersion", "BASS_GetVolume", "BASS_Init",
	"BASS_MusicFree", "BASS_MusicGetAttribute", "BASS_MusicGetOrderPosition", "BASS_MusicGetOrders",
	"BASS_MusicLoad", "BASS_MusicSetAttribute", "BASS_PluginLoad", "BASS_SampleFree",
	"BASS_SampleGetChannel", "BASS_SampleGetInfo", "BASS_SampleLoad", "BASS_SampleSetInfo",
	"BASS_SampleStop", "BASS_SetConfig", "BASS_SetGlobalVolumes", "BASS_SetVolume",
	"BASS_Start", "BASS_Stop", "BASS_StreamCreateFile", "BASS_StreamFree", "BASS_ChannelSeconds2Bytes",
};
enum
{
	kIsActive = 5, kSampleStop = 40, kPause = 7, kMusicLoad = 32, kGetChannel = 36, kStop = 18, kPlay = 8, kSampleLoad = 38, kInit = 27
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
	DWORD mArg0[64];
	int mDepth;
} Frames;

// The stand-in samples: handles 0x5a000000 + n, their channels 0x5b000000 + n, with the state BASS_ChannelIsActive
// gives (0 stopped, 1 playing, 3 paused); a stand-in sample never ends by itself
#define FAKE_SAMPLE 0x5a000000
#define FAKE_CHANNEL 0x5b000000
static int gFakeCount;
static DWORD gFakeState[4096];

static int FakeIndex(DWORD theHandle)
{
	DWORD aBase = theHandle & 0xff000000;
	int n = (int)(theHandle & 0xffffff);
	if ((aBase == FAKE_SAMPLE || aBase == FAKE_CHANNEL) && n < gFakeCount)
		return n;
	return -1;
}

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
		f->mArg0[f->mDepth] = theFrame[2];
	}
	f->mDepth++;
	DWORD* a = theFrame + 2;
	EnterCriticalSection(&gLock);
	if (i == kInit && a[0] != 0)
	{
		if (gLog)
			fprintf(gLog, "# BASS_Init: device %lu becomes 0 (no sound)\n", a[0]);
		a[0] = 0;
	}
	if (gLog)
	{
		fprintf(gLog, "%u %lu %s %lx %lx %lx %lx %lx %lx", Ms(), GetCurrentThreadId(), kNames[i], a[0], a[1], a[2],
			a[3], a[4], a[5]);
		// BASS_SampleLoad / BASS_MusicLoad from memory: (TRUE, data, offset, length, ...). The game's music gives
		// no length, so its CRC covers the first 32 KB (soundlog.py's port side does the same)
		if ((i == kSampleLoad || i == kMusicLoad) && a[0] != 0 && a[1] != 0)
		{
			DWORD aLength = i == kSampleLoad ? a[3] : 0x8000;
			if (aLength != 0 && aLength < 0x4000000 && !IsBadReadPtr((void*)a[1], aLength))
				fprintf(gLog, " crc %08lx len %lu", Crc32((const BYTE*)a[1], aLength), aLength);
		}
		fprintf(gLog, "\n");
		fflush(gLog);
	}
	LeaveCriticalSection(&gLock);
	return gReal[i];
}

// cdecl: answers for a stand-in sample, logs the result and returns the caller's return address
DWORD __cdecl ProxyLeave(DWORD* theResult)
{
	Frames* f = GetFrames();
	f->mDepth--;
	int d = f->mDepth < 64 ? f->mDepth : 63;
	int i = f->mIndex[d];
	EnterCriticalSection(&gLock);
	int n = FakeIndex(f->mArg0[d]);
	int isStandIn = n >= 0;
	if (i == kSampleLoad && *theResult == 0 && gFakeCount < 4096)
	{
		gFakeState[gFakeCount] = 0;
		*theResult = FAKE_SAMPLE + gFakeCount++;
		isStandIn = 1;
	}
	else if (n >= 0)
	{
		if (i == kGetChannel)
			*theResult = FAKE_CHANNEL + n;
		else if (i == kIsActive)
			*theResult = gFakeState[n];
		else
		{
			if (i == kPlay)
				gFakeState[n] = 1;
			else if (i == kPause && gFakeState[n] == 1)
				gFakeState[n] = 3;
			else if (i == kSampleStop || i == kStop)
				gFakeState[n] = 0;
			*theResult = 1;
		}
	}
	if (gLog)
	{
		fprintf(gLog, "%u %lu = %s %lx%s\n", Ms(), GetCurrentThreadId(), kNames[i], *theResult,
			isStandIn ? " (stand-in)" : "");
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
	"	pushl %eax\n"            // the result, which ProxyLeave may change
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
THUNK(12) THUNK(13) THUNK(14) THUNK(15) THUNK(16) THUNK(17) THUNK(18) THUNK(19) THUNK(20) THUNK(21) THUNK(22) THUNK(23)
THUNK(24) THUNK(25) THUNK(26) THUNK(27) THUNK(28) THUNK(29) THUNK(30) THUNK(31) THUNK(32) THUNK(33) THUNK(34) THUNK(35)
THUNK(36) THUNK(37) THUNK(38) THUNK(39) THUNK(40) THUNK(41) THUNK(42) THUNK(43) THUNK(44) THUNK(45) THUNK(46) THUNK(47)
THUNK(48)

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
