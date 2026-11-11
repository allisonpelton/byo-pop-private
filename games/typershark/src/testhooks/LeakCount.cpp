// port: test-only allocation counter for the leak check (games/typershark/docs/LEAKS.md). The whole file is guarded by
// TS_LEAKCOUNT, which only the scratch build turns on (cmake -DTS_LEAKCOUNT=ON, build/ts-leak); the default build compiles an
// empty unit and its behaviour is unchanged.
//
// It replaces the global operator new / delete of the executable, and (with the linker's --wrap, see CMakeLists.txt) the
// executable's own calls of malloc / calloc / realloc / free, with versions that remember every live block in a side table
// (pointer, size, the call stack of its allocation) kept in a private Win32 heap, so the counter never allocates through itself.
// The scratch build links the C++ library statically (-static): with libc++.dll a block allocated inside the DLL and freed by the
// executable's operator delete (or the other way round) would show up as a false leak. What the other DLLs (SDL2.dll) allocate
// is not recorded; the report counts the process heap's blocks that the table does not know as "untracked". The `leak <tag> [sites]` test command (TestCommands.cpp) and the program's exit print
//   LEAK <tag> live=<blocks> bytes=<bytes> allocs=<total news> frees=<total deletes> heap=<blocks>/<bytes>
// where heap= is the process heap walked block by block (everything, also what malloc, SDL and the DLLs hold). With `sites` the
// live blocks grouped by allocation call stack go to leak_<tag>.sites (one line per site: live count, bytes, then up to 10
// frames as module+rva), which games/typershark/tools/leak/leaksites.py turns into function names and diffs between two tags.

#ifdef TS_LEAKCOUNT

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <new>
#include <string>
#include <SDL.h>

extern "C"
{
void* __real_malloc(size_t theSize);
void* __real_calloc(size_t theCount, size_t theSize);
void* __real_realloc(void* thePtr, size_t theSize);
void __real_free(void* thePtr);
}

namespace
{

thread_local bool gSkip = false;			// set while the counter itself runs (the report's printf allocates through malloc)

const int kFrames = 10;				// frames kept per site
const int kMaxSites = 1 << 16;

struct Site
{
	void*			mFrames[kFrames];
	unsigned		mLive;
	unsigned		mLiveBytes;
	unsigned		mTotal;
	unsigned		mHash;
	bool			mUsed;
};

struct Entry
{
	uintptr_t		mPtr;			// 0 empty, 1 deleted
	unsigned		mSize;
	unsigned		mSite;
};

Site gSites[kMaxSites];
Entry* gTable = nullptr;
size_t gTableSize = 0;				// power of two
size_t gTableUsed = 0;				// live entries plus tombstones
HANDLE gHeap = nullptr;
volatile LONG gLock = 0;
unsigned long long gLiveBlocks = 0, gLiveBytes = 0, gAllocs = 0, gFrees = 0;
unsigned long long gStale = 0;				// allocations of an address that the table still held (the free was not seen)
bool gReportAtExit = false;

struct Lock
{
	Lock() { while (InterlockedExchange(&gLock, 1)) SwitchToThread(); }
	~Lock() { InterlockedExchange(&gLock, 0); }
};

size_t HashPtr(uintptr_t thePtr) { return (size_t)((thePtr >> 3) * 2654435761u); }

void TableInit()
{
	if (gHeap)
		return;
	gHeap = HeapCreate(0, 1 << 20, 0);
	gTableSize = 1 << 16;
	gTable = (Entry*)HeapAlloc(gHeap, HEAP_ZERO_MEMORY, gTableSize * sizeof(Entry));
}

void TableGrow()
{
	size_t aNewSize = gTableSize;
	if (gLiveBlocks * 4 > aNewSize)
		aNewSize *= 2;
	Entry* aNew = (Entry*)HeapAlloc(gHeap, HEAP_ZERO_MEMORY, aNewSize * sizeof(Entry));
	for (size_t i = 0; i < gTableSize; i++)
	{
		if (gTable[i].mPtr > 1)
		{
			size_t j = HashPtr(gTable[i].mPtr) & (aNewSize - 1);
			while (aNew[j].mPtr)
				j = (j + 1) & (aNewSize - 1);
			aNew[j] = gTable[i];
		}
	}
	HeapFree(gHeap, 0, gTable);
	gTable = aNew;
	gTableSize = aNewSize;
	gTableUsed = (size_t)gLiveBlocks;
}

unsigned FindSiteIn(Site* gSites, void** theFrames, int theCount)
{
	unsigned aHash = 17;
	for (int i = 0; i < kFrames; i++)
	{
		uintptr_t aFrame = i < theCount ? (uintptr_t)theFrames[i] : 0;
		aHash = aHash * 31 + (unsigned)(aFrame >> 2) + (unsigned)aFrame;
	}
	unsigned aIndex = aHash & (kMaxSites - 1);
	for (int aProbe = 0; aProbe < kMaxSites; aProbe++)
	{
		Site& aSite = gSites[aIndex];
		if (!aSite.mUsed)
		{
			aSite.mUsed = true;
			aSite.mHash = aHash;
			for (int i = 0; i < kFrames; i++)
				aSite.mFrames[i] = i < theCount ? theFrames[i] : nullptr;
			return aIndex;
		}
		if (aSite.mHash == aHash)
		{
			bool aSame = true;
			for (int i = 0; i < kFrames; i++)
				if (aSite.mFrames[i] != (i < theCount ? theFrames[i] : nullptr))
					aSame = false;
			if (aSame)
				return aIndex;
		}
		aIndex = (aIndex + 1) & (kMaxSites - 1);
	}
	return 0;
}

unsigned FindSite(void** theFrames, int theCount)
{
	return FindSiteIn(gSites, theFrames, theCount);
}

uintptr_t gImageLo = 0, gImageHi = 0;

// The frame-pointer chain stops inside code compiled without frame pointers (SDL2.dll, the C++ library): when it came to fewer
// than four frames, the return addresses of the executable that lie on the stack (a call instruction ends just before them) are
// added after it. They may include stale ones; the report is for finding the caller, not for exact stacks.
__attribute__((noinline)) int ScanStack(void** theFrames, int theCount)
{
	if (theCount >= 4)
		return theCount;
	if (!gImageLo)
	{
		uintptr_t aBase = (uintptr_t)GetModuleHandleA(nullptr);
		IMAGE_NT_HEADERS* aHeaders = (IMAGE_NT_HEADERS*)(aBase + ((IMAGE_DOS_HEADER*)aBase)->e_lfanew);
		gImageHi = aBase + aHeaders->OptionalHeader.SizeOfImage;
		gImageLo = aBase;
	}
	uintptr_t* aSp = (uintptr_t*)__builtin_frame_address(0);
	uintptr_t* aEnd = (uintptr_t*)((NT_TIB*)NtCurrentTeb())->StackBase;
	for (int i = 0; aSp + i < aEnd && i < 600 && theCount < kFrames; i++)
	{
		uintptr_t aWord = aSp[i];
		if (aWord < gImageLo + 16 || aWord >= gImageHi)
			continue;
		const unsigned char* aCode = (const unsigned char*)aWord;
		bool aCall = aCode[-5] == 0xE8 || (aCode[-6] == 0xFF && aCode[-5] == 0x15) || (aCode[-2] == 0xFF && (aCode[-1] & 0xF8) == 0xD0);
		bool aDuplicate = false;
		for (int j = 0; j < theCount; j++)
			if ((uintptr_t)theFrames[j] == aWord)
				aDuplicate = true;
		if (aCall && !aDuplicate)
			theFrames[theCount++] = (void*)aWord;
	}
	return theCount;
}

__attribute__((noinline)) void Record(void* thePtr, size_t theSize)
{
	if (!thePtr || gSkip)
		return;
	void* aFrames[kFrames + 2];
	int aCount = (int)RtlCaptureStackBackTrace(2, kFrames, aFrames, nullptr);
	aCount = ScanStack(aFrames, aCount);
	Lock aLock;
	TableInit();
	if ((gTableUsed + 1) * 2 > gTableSize)
		TableGrow();
	unsigned aSite = FindSite(aFrames, aCount);
	size_t j = HashPtr((uintptr_t)thePtr) & (gTableSize - 1);
	size_t aTomb = (size_t)-1;
	while (gTable[j].mPtr)
	{
		if (gTable[j].mPtr == (uintptr_t)thePtr)
			break;
		if (gTable[j].mPtr == 1 && aTomb == (size_t)-1)
			aTomb = j;
		j = (j + 1) & (gTableSize - 1);
	}
	if (gTable[j].mPtr == (uintptr_t)thePtr)
	{
		// the address is allocated again though the table still holds it: the block was freed where the counter did not see it
		gSites[gTable[j].mSite].mLive--;
		gSites[gTable[j].mSite].mLiveBytes -= gTable[j].mSize;
		gLiveBlocks--;
		gLiveBytes -= gTable[j].mSize;
		gStale++;
	}
	else if (aTomb != (size_t)-1)
		j = aTomb;
	else
		gTableUsed++;
	gTable[j].mPtr = (uintptr_t)thePtr;
	gTable[j].mSize = (unsigned)theSize;
	gTable[j].mSite = aSite;
	gSites[aSite].mLive++;
	gSites[aSite].mLiveBytes += (unsigned)theSize;
	gSites[aSite].mTotal++;
	gLiveBlocks++;
	gLiveBytes += theSize;
	gAllocs++;
}

void Forget(void* thePtr)
{
	if (!thePtr || gSkip)
		return;
	Lock aLock;
	gFrees++;
	if (!gTable)
		return;
	size_t j = HashPtr((uintptr_t)thePtr) & (gTableSize - 1);
	while (gTable[j].mPtr)
	{
		if (gTable[j].mPtr == (uintptr_t)thePtr)
		{
			Site& aSite = gSites[gTable[j].mSite];
			aSite.mLive--;
			aSite.mLiveBytes -= gTable[j].mSize;
			gLiveBlocks--;
			gLiveBytes -= gTable[j].mSize;
			gTable[j].mPtr = 1;
			return;
		}
		j = (j + 1) & (gTableSize - 1);
	}
}

__attribute__((always_inline)) inline void* Allocate(size_t theSize)
{
	void* aPtr = __real_malloc(theSize ? theSize : 1);
	if (aPtr)
		Record(aPtr, theSize);
	return aPtr;
}

void* SdlMalloc(size_t theSize)
{
	void* aPtr = __real_malloc(theSize ? theSize : 1);
	Record(aPtr, theSize);
	return aPtr;
}
void* SdlCalloc(size_t theCount, size_t theSize)
{
	void* aPtr = __real_calloc(theCount ? theCount : 1, theSize ? theSize : 1);
	Record(aPtr, theCount * theSize);
	return aPtr;
}
void* SdlRealloc(void* thePtr, size_t theSize)
{
	void* aPtr = __real_realloc(thePtr, theSize);
	if (aPtr)
		Forget(thePtr);
	Record(aPtr, theSize);
	return aPtr;
}
void SdlFree(void* thePtr)
{
	Forget(thePtr);
	__real_free(thePtr);
}

bool IsTracked(uintptr_t thePtr)
{
	Lock aLock;
	if (!gTable)
		return false;
	size_t j = HashPtr(thePtr) & (gTableSize - 1);
	while (gTable[j].mPtr)
	{
		if (gTable[j].mPtr == thePtr)
			return true;
		j = (j + 1) & (gTableSize - 1);
	}
	return false;
}

// A second table of every block of the process heap with its call stack, kept by patching ntdll's RtlAllocateHeap /
// RtlFreeHeap / RtlReAllocateHeap (the first five bytes of each become a jump to the hooks below). The blocks the first table
// does not know (those that DLLs allocated on their own) are reported as H lines, grouped by stack. Scratch use only.
Site gHSites[kMaxSites];
const size_t kHTableSize = 1 << 20;
Entry* gHTable = nullptr;
HANDLE gHHeap = nullptr;
HANDLE gProcHeap = nullptr;
volatile LONG gHLock = 0;
DWORD gHBusySlot = TLS_OUT_OF_INDEXES;			// per thread: set while the hook records (a TLS slot, since thread_local would allocate)

struct HLock
{
	HLock() { while (InterlockedExchange(&gHLock, 1)) SwitchToThread(); }
	~HLock() { InterlockedExchange(&gHLock, 0); }
};

bool HBusy() { return TlsGetValue(gHBusySlot) != nullptr; }
void HSetBusy(bool theBusy) { TlsSetValue(gHBusySlot, theBusy ? (LPVOID)1 : nullptr); }

__attribute__((noinline)) void HRecord(void* thePtr, size_t theSize)
{
	if (!thePtr || !gHTable || HBusy())
		return;
	HSetBusy(true);
	void* aFrames[kFrames + 2];
	int aCount = (int)RtlCaptureStackBackTrace(2, kFrames, aFrames, nullptr);
	{
		HLock aLock;
		unsigned aSite = FindSiteIn(gHSites, aFrames, aCount);
		size_t j = HashPtr((uintptr_t)thePtr) & (kHTableSize - 1);
		while (gHTable[j].mPtr && gHTable[j].mPtr != (uintptr_t)thePtr)
			j = (j + 1) & (kHTableSize - 1);
		if (gHTable[j].mPtr)
		{
			gHSites[gHTable[j].mSite].mLive--;
			gHSites[gHTable[j].mSite].mLiveBytes -= gHTable[j].mSize;
		}
		gHTable[j].mPtr = (uintptr_t)thePtr;
		gHTable[j].mSize = (unsigned)theSize;
		gHTable[j].mSite = aSite;
		gHSites[aSite].mLive++;
		gHSites[aSite].mLiveBytes += (unsigned)theSize;
	}
	HSetBusy(false);
}

void HForget(void* thePtr)
{
	if (!thePtr || !gHTable || HBusy())
		return;
	HLock aLock;
	size_t aMask = kHTableSize - 1;
	size_t j = HashPtr((uintptr_t)thePtr) & aMask;
	while (gHTable[j].mPtr)
	{
		if (gHTable[j].mPtr == (uintptr_t)thePtr)
		{
			gHSites[gHTable[j].mSite].mLive--;
			gHSites[gHTable[j].mSite].mLiveBytes -= gHTable[j].mSize;
			// linear probing: close the gap by moving up the entries that may not stay behind it
			size_t i = j;
			for (;;)
			{
				gHTable[i].mPtr = 0;
				size_t k = i;
				bool aMoved = false;
				for (;;)
				{
					k = (k + 1) & aMask;
					if (!gHTable[k].mPtr)
						break;
					size_t aHome = HashPtr(gHTable[k].mPtr) & aMask;
					bool aStays = i <= k ? (i < aHome && aHome <= k) : (i < aHome || aHome <= k);
					if (aStays)
						continue;
					gHTable[i] = gHTable[k];
					i = k;
					aMoved = true;
					break;
				}
				if (!aMoved)
					return;
			}
		}
		j = (j + 1) & aMask;
	}
}

typedef PVOID (NTAPI* RtlAllocateHeapFn)(PVOID, ULONG, SIZE_T);
typedef BOOLEAN (NTAPI* RtlFreeHeapFn)(PVOID, ULONG, PVOID);
typedef PVOID (NTAPI* RtlReAllocateHeapFn)(PVOID, ULONG, PVOID, SIZE_T);
RtlAllocateHeapFn gRealAllocateHeap = nullptr;
RtlFreeHeapFn gRealFreeHeap = nullptr;
RtlReAllocateHeapFn gRealReAllocateHeap = nullptr;

PVOID NTAPI HookAllocateHeap(PVOID theHeap, ULONG theFlags, SIZE_T theSize)
{
	PVOID aPtr = gRealAllocateHeap(theHeap, theFlags, theSize);
	if (aPtr && theHeap == gProcHeap)
		HRecord(aPtr, theSize);
	return aPtr;
}

BOOLEAN NTAPI HookFreeHeap(PVOID theHeap, ULONG theFlags, PVOID thePtr)
{
	if (thePtr && theHeap == gProcHeap)
		HForget(thePtr);
	return gRealFreeHeap(theHeap, theFlags, thePtr);
}

PVOID NTAPI HookReAllocateHeap(PVOID theHeap, ULONG theFlags, PVOID thePtr, SIZE_T theSize)
{
	if (thePtr && theHeap == gProcHeap)
		HForget(thePtr);
	PVOID aPtr = gRealReAllocateHeap(theHeap, theFlags, thePtr, theSize);
	if (aPtr && theHeap == gProcHeap)
		HRecord(aPtr, theSize);
	return aPtr;
}

// patches the function to jump to theHook; returns the trampoline that runs the original (nullptr when the prologue is not the expected one)
void* HookFunction(const char* theName, void* theHook)
{
	unsigned char* aFunc = (unsigned char*)GetProcAddress(GetModuleHandleA("ntdll.dll"), theName);
	if (!aFunc)
		return nullptr;
	if (!(aFunc[0] == 0x8B && aFunc[1] == 0xFF && aFunc[2] == 0x55 && aFunc[3] == 0x8B && aFunc[4] == 0xEC))
	{
		fprintf(stderr, "leak: %s starts %02x %02x %02x %02x %02x %02x %02x %02x, not hooked\n", theName, aFunc[0], aFunc[1], aFunc[2], aFunc[3], aFunc[4], aFunc[5], aFunc[6], aFunc[7]);
		return nullptr;
	}
	unsigned char* aTramp = (unsigned char*)VirtualAlloc(nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memcpy(aTramp, aFunc, 5);
	aTramp[5] = 0xE9;
	*(int*)(aTramp + 6) = (int)((uintptr_t)(aFunc + 5) - (uintptr_t)(aTramp + 10));
	DWORD anOld;
	VirtualProtect(aFunc, 5, PAGE_EXECUTE_READWRITE, &anOld);
	aFunc[0] = 0xE9;
	*(int*)(aFunc + 1) = (int)((uintptr_t)theHook - (uintptr_t)(aFunc + 5));
	VirtualProtect(aFunc, 5, anOld, &anOld);
	FlushInstructionCache(GetCurrentProcess(), aFunc, 5);
	return aTramp;
}

void InstallHeapHooks()
{
	gProcHeap = GetProcessHeap();
	gHBusySlot = TlsAlloc();
	gHHeap = HeapCreate(0, 1 << 20, 0);
	gHTable = (Entry*)HeapAlloc(gHHeap, HEAP_ZERO_MEMORY, kHTableSize * sizeof(Entry));
	gRealAllocateHeap = (RtlAllocateHeapFn)HookFunction("RtlAllocateHeap", (void*)HookAllocateHeap);
	gRealFreeHeap = (RtlFreeHeapFn)HookFunction("RtlFreeHeap", (void*)HookFreeHeap);
	gRealReAllocateHeap = (RtlReAllocateHeapFn)HookFunction("RtlReAllocateHeap", (void*)HookReAllocateHeap);
	if (!gRealAllocateHeap || !gRealFreeHeap || !gRealReAllocateHeap)
		fprintf(stderr, "leak: heap hooks incomplete (%p %p %p)\n", (void*)gRealAllocateHeap, (void*)gRealFreeHeap, (void*)gRealReAllocateHeap);
}

// the blocks of the second table that the first does not know, summed by site
unsigned gHLiveBySite[kMaxSites];
unsigned gHBytesBySite[kMaxSites];

// the busy blocks of the process heap: all of them, and those the table does not know by size (rounded up to 16)
const int kHistSizes = 4096;				// sizes up to 64 KB in steps of 16; larger blocks are counted in the last slot
struct Sample
{
	unsigned mSize;
	unsigned mBytes[32];
};
struct HeapInfo
{
	unsigned long long mBlocks, mBytes, mUntrackedBlocks, mUntrackedBytes;
	unsigned mUntrackedBySize[kHistSizes];
	Sample mSamples[24];					// the first blocks of a few sizes (TS_LEAK_SAMPLES=64,80,...): what the untracked ones hold
	int mNumSamples;
};
HeapInfo gHeapInfo;
const int kMaxBaseline = 1 << 17;
uintptr_t gBaseline[kMaxBaseline];				// the untracked blocks at the first report, sorted (the later samples are of other blocks)
int gBaselineCount = -1;						// -1: no report yet

bool InBaseline(uintptr_t thePtr)
{
	int aLow = 0, aHigh = gBaselineCount - 1;
	while (aLow <= aHigh)
	{
		int aMid = (aLow + aHigh) / 2;
		if (gBaseline[aMid] == thePtr)
			return true;
		if (gBaseline[aMid] < thePtr)
			aLow = aMid + 1;
		else
			aHigh = aMid - 1;
	}
	return false;
}

void HeapTotals(HeapInfo* theInfo)
{
	memset(theInfo, 0, sizeof(*theInfo));
	HANDLE aHeap = GetProcessHeap();
	if (!HeapLock(aHeap))
		return;
	PROCESS_HEAP_ENTRY anEntry;
	memset(&anEntry, 0, sizeof(anEntry));
	while (HeapWalk(aHeap, &anEntry))
	{
		if (anEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
		{
			theInfo->mBlocks++;
			theInfo->mBytes += anEntry.cbData;
			if (!IsTracked((uintptr_t)anEntry.lpData))
			{
				theInfo->mUntrackedBlocks++;
				theInfo->mUntrackedBytes += anEntry.cbData;
				size_t aSlot = (anEntry.cbData + 15) / 16;
				theInfo->mUntrackedBySize[aSlot < kHistSizes ? aSlot : kHistSizes - 1]++;
				if (gBaselineCount < 0 && theInfo->mUntrackedBlocks <= (unsigned long long)kMaxBaseline)
					gBaseline[theInfo->mUntrackedBlocks - 1] = (uintptr_t)anEntry.lpData;
				const char* aWanted = getenv("TS_LEAK_SAMPLES");
				if (aWanted && theInfo->mNumSamples < 24 && anEntry.cbData >= 32 && gBaselineCount >= 0 && !InBaseline((uintptr_t)anEntry.lpData))
				{
					char aKey[16];
					snprintf(aKey, sizeof(aKey), ",%u,", (unsigned)aSlot * 16);
					std::string aList = std::string(",") + aWanted + ",";
					int aSeen = 0;
					for (int i = 0; i < theInfo->mNumSamples; i++)
						if (theInfo->mSamples[i].mSize == aSlot * 16)
							aSeen++;
					if (aList.find(aKey) != std::string::npos && aSeen < 4)
					{
						Sample& aSample = theInfo->mSamples[theInfo->mNumSamples++];
						aSample.mSize = (unsigned)aSlot * 16;
						memset(aSample.mBytes, 0, 128); memcpy(aSample.mBytes, anEntry.lpData, anEntry.cbData < 128 ? anEntry.cbData : 128);
					}
				}
			}
		}
	}
	HeapUnlock(aHeap);
	if (gBaselineCount < 0)
	{
		gBaselineCount = (int)(theInfo->mUntrackedBlocks < (unsigned long long)kMaxBaseline ? theInfo->mUntrackedBlocks : kMaxBaseline);
		qsort(gBaseline, gBaselineCount, sizeof(uintptr_t), [](const void* a, const void* b) -> int
		{
			uintptr_t x = *(const uintptr_t*)a, y = *(const uintptr_t*)b;
			return x < y ? -1 : (x > y ? 1 : 0);
		});
	}
}

void ModuleRva(void* theAddress, char* theOut, size_t theSize)
{
	HMODULE aModule = nullptr;
	if (!theAddress || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCSTR)theAddress, &aModule))
	{
		snprintf(theOut, theSize, "?+0x%x", (unsigned)(uintptr_t)theAddress);
		return;
	}
	char aPath[MAX_PATH];
	GetModuleFileNameA(aModule, aPath, MAX_PATH);
	const char* aName = strrchr(aPath, '\\');
	aName = aName ? aName + 1 : aPath;
	snprintf(theOut, theSize, "%s+0x%x", aName, (unsigned)((uintptr_t)theAddress - (uintptr_t)aModule));
}

void PrintReport(const char* theTag, bool theSites)
{
	gSkip = true;
	HSetBusy(true);
	HeapInfo& anInfo = gHeapInfo;
	HeapTotals(&anInfo);
	if (gHTable)
	{
		memset(gHLiveBySite, 0, sizeof(gHLiveBySite));
		memset(gHBytesBySite, 0, sizeof(gHBytesBySite));
		HLock aHLock;
		for (size_t i = 0; i < kHTableSize; i++)
			if (gHTable[i].mPtr && !IsTracked(gHTable[i].mPtr))
			{
				gHLiveBySite[gHTable[i].mSite]++;
				gHBytesBySite[gHTable[i].mSite] += gHTable[i].mSize;
			}
	}
	{
		Lock aLock;
		DWORD aHandles = 0;
		GetProcessHandleCount(GetCurrentProcess(), &aHandles);
		printf("LEAK %s live=%llu bytes=%llu allocs=%llu frees=%llu heap=%llu/%llu untracked=%llu/%llu handles=%lu stale=%llu\n", theTag, gLiveBlocks, gLiveBytes,
			gAllocs, gFrees, anInfo.mBlocks, anInfo.mBytes, anInfo.mUntrackedBlocks, anInfo.mUntrackedBytes, (unsigned long)aHandles, gStale);
		fflush(stdout);
		if (theSites)
		{
			char aName[MAX_PATH];
			snprintf(aName, sizeof(aName), "leak_%s.sites", theTag);
			FILE* aFile = fopen(aName, "w");
			if (aFile)
			{
				for (int i = 0; i < kMaxSites; i++)
				{
					const Site& aSite = gSites[i];
					if (!aSite.mUsed || aSite.mLive == 0)
						continue;
					fprintf(aFile, "%u %u", aSite.mLive, aSite.mLiveBytes);
					for (int f = 0; f < kFrames && aSite.mFrames[f]; f++)
					{
						char aFrame[MAX_PATH + 32];
						ModuleRva(aSite.mFrames[f], aFrame, sizeof(aFrame));
						fprintf(aFile, " %s", aFrame);
					}
					fprintf(aFile, "\n");
				}
				for (int i = 0; i < kMaxSites && gHTable; i++)
				{
					if (!gHLiveBySite[i])
						continue;
					fprintf(aFile, "H %u %u", gHLiveBySite[i], gHBytesBySite[i]);
					for (int f = 0; f < kFrames && gHSites[i].mFrames[f]; f++)
					{
						char aFrame[MAX_PATH + 32];
						ModuleRva(gHSites[i].mFrames[f], aFrame, sizeof(aFrame));
						fprintf(aFile, " %s", aFrame);
					}
					fprintf(aFile, "%c", 10);
				}
				for (int i = 0; i < anInfo.mNumSamples; i++)
				{
					fprintf(aFile, "S %u", anInfo.mSamples[i].mSize);
					for (int w = 0; w < 32; w++)
					{
						char aMod[MAX_PATH + 32];
						unsigned aWord = anInfo.mSamples[i].mBytes[w];
						HMODULE aWordModule = nullptr;
						if (aWord > 0x10000 && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(uintptr_t)aWord, &aWordModule))
						{
							ModuleRva((void*)(uintptr_t)aWord, aMod, sizeof(aMod));
							fprintf(aFile, " %s", aMod);
						}
						else
							fprintf(aFile, " %08x", aWord);
					}
					fprintf(aFile, "\n");
				}
				for (int i = 0; i < kHistSizes; i++)
					if (anInfo.mUntrackedBySize[i])
						fprintf(aFile, "U %d %u\n", i * 16, anInfo.mUntrackedBySize[i]);
				fclose(aFile);
			}
		}
	}
	HSetBusy(false);
	gSkip = false;
}

void ExitReport()
{
	if (gReportAtExit)
		PrintReport("exit", true);
}

struct ExitHook
{
	ExitHook()
	{
		gReportAtExit = true;
		atexit(ExitReport);
		// SDL2.dll's own allocations (surfaces, textures, audio) go through the counter too, with the stack that asked for them
		SDL_SetMemoryFunctions(SdlMalloc, SdlCalloc, SdlRealloc, SdlFree);
		if (getenv("TS_LEAK_HEAPHOOK"))
			InstallHeapHooks();
	}
};
ExitHook gExitHook;

}

namespace Sexy
{
void LeakCountReport(const char* theTag, bool theSites)
{
	PrintReport(theTag, theSites);
}
}

extern "C"
{
void* __wrap_malloc(size_t theSize)
{
	void* aPtr = __real_malloc(theSize);
	Record(aPtr, theSize);
	return aPtr;
}
void* __wrap_calloc(size_t theCount, size_t theSize)
{
	void* aPtr = __real_calloc(theCount, theSize);
	Record(aPtr, theCount * theSize);
	return aPtr;
}
void* __wrap_realloc(void* thePtr, size_t theSize)
{
	void* aPtr = __real_realloc(thePtr, theSize);
	if (aPtr || theSize == 0)
		Forget(thePtr);
	Record(aPtr, theSize);
	return aPtr;
}
void __wrap_free(void* thePtr)
{
	Forget(thePtr);
	__real_free(thePtr);
}
}

void* operator new(size_t theSize) { void* aPtr = Allocate(theSize); if (!aPtr) abort(); return aPtr; }
void* operator new[](size_t theSize) { void* aPtr = Allocate(theSize); if (!aPtr) abort(); return aPtr; }
void* operator new(size_t theSize, const std::nothrow_t&) noexcept { return Allocate(theSize); }
void* operator new[](size_t theSize, const std::nothrow_t&) noexcept { return Allocate(theSize); }
void operator delete(void* thePtr) noexcept { Forget(thePtr); __real_free(thePtr); }
void operator delete[](void* thePtr) noexcept { Forget(thePtr); __real_free(thePtr); }
void operator delete(void* thePtr, size_t) noexcept { Forget(thePtr); __real_free(thePtr); }
void operator delete[](void* thePtr, size_t) noexcept { Forget(thePtr); __real_free(thePtr); }
void operator delete(void* thePtr, const std::nothrow_t&) noexcept { Forget(thePtr); __real_free(thePtr); }
void operator delete[](void* thePtr, const std::nothrow_t&) noexcept { Forget(thePtr); __real_free(thePtr); }

#endif // TS_LEAKCOUNT
