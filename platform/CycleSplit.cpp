// The cycle split behind -cyclesplit=<file> (see CycleSplit.h).

#include "CycleSplit.h"

#ifdef _WIN32

#include <SDL.h>
#include <windows.h>
#include <intrin.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace Platform
{

bool gCycleSplitOn = false;
static bool gCycleSplitStarted = false;     // Start() succeeded; gCycleSplitOn is false while counting is paused

namespace
{

struct Frame
{
	CyclePart mPart;
	uint64_t mStart;        // the thread's cycles when the part began, or when a snapshot last credited it
	uint64_t mChild;        // the inclusive cycles of the counted parts that finished inside it since then
};

const int kMaxDepth = 8;

struct State
{
	std::string mFileName;
	DWORD mLoopThread = 0;
	Frame mStack[kMaxDepth];
	int mDepth = 0;
	uint64_t mCycles[CYCLE_NUM_PARTS] = {};
	uint64_t mCalls[CYCLE_NUM_PARTS] = {};
	uint64_t mReads = 0;                // cycle readings made by Enter and Leave
	uint64_t mReadCycles = 0;           // what one reading costs in a run of them, measured at the start
	uint64_t mReadTsc = 0;              // the time-stamp counter's ticks spent in the readings Enter and Leave made, in the running game
	uint64_t mMisses = 0;               // a Leave with no part open, or a part nested deeper than the stack holds
	int64_t mCounterStart = 0;          // QueryPerformanceCounter at the start
	double mCounterFreq = 0;
	std::vector<std::string> mLines;
	bool mFinished = false;
};

State gState;

typedef LONG (WINAPI* NtQueryInformationThreadFn)(HANDLE, int, PVOID, ULONG, PULONG);
typedef HRESULT (WINAPI* GetThreadDescriptionFn)(HANDLE, PWSTR*);

inline uint64_t ThreadCycles()
{
	ULONG64 aCycles = 0;
	QueryThreadCycleTime(GetCurrentThread(), &aCycles);
	return aCycles;
}

// A reading as Enter and Leave make it, timed by the time-stamp counter (about 25 cycles each side). In the running game a reading
// costs several times what a run of them in a row does (the system call finds cold caches and predictors after the game's own
// code), so the file's cycles_each is this one.
inline uint64_t TimedThreadCycles()
{
	uint64_t aBefore = __rdtsc();
	uint64_t aCycles = ThreadCycles();
	gState.mReadTsc += __rdtsc() - aBefore;
	return aCycles;
}

std::string Escaped(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++)
	{
		unsigned char aChar = (unsigned char) theText[i];
		if ((aChar == '"') || (aChar == '\\'))
			aResult += '\\';
		if (aChar >= 32)
			aResult += (char) aChar;
	}
	return aResult;
}

// "name module+0xoffset" parts of one other thread: the name SetThreadDescription gave it, and where its start address is
void DescribeThread(HANDLE theThread, std::string* theName, std::string* theStart)
{
	static GetThreadDescriptionFn aGetDescription = (GetThreadDescriptionFn) GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetThreadDescription");
	static NtQueryInformationThreadFn aQuery = (NtQueryInformationThreadFn) GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");

	theName->clear();
	theStart->clear();
	if (aGetDescription != NULL)
	{
		PWSTR aDescription = NULL;
		if (SUCCEEDED(aGetDescription(theThread, &aDescription)) && (aDescription != NULL))
		{
			for (const wchar_t* aChar = aDescription; *aChar != 0; aChar++)
				theName->push_back((*aChar < 128) ? (char) *aChar : '?');
			LocalFree(aDescription);
		}
	}
	if (aQuery != NULL)
	{
		// ThreadQuerySetWin32StartAddress
		PVOID aStart = NULL;
		if (aQuery(theThread, 9, &aStart, sizeof(aStart), NULL) == 0)
		{
			HMODULE aModule = NULL;
			char aPath[MAX_PATH] = {};
			if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) aStart, &aModule) &&
				GetModuleFileNameA(aModule, aPath, MAX_PATH))
			{
				const char* aBase = strrchr(aPath, '\\');
				aBase = (aBase != NULL) ? aBase + 1 : aPath;
				char aBuf[MAX_PATH + 32];
				snprintf(aBuf, sizeof(aBuf), "%s+0x%x", aBase, (unsigned) ((char*) aStart - (char*) aModule));
				for (char* aChar = aBuf; *aChar != 0; aChar++)
					*aChar = (char) tolower((unsigned char) *aChar);
				*theStart = aBuf;
			}
			else
			{
				char aBuf[32];
				snprintf(aBuf, sizeof(aBuf), "0x%p", aStart);
				*theStart = aBuf;
			}
		}
	}
}

// The cycles the open parts have run since they began (or were last credited) go to their parts now and their clocks restart, so
// that a snapshot is exact however deep the loop is in a part when it is taken
void CreditOpenParts(uint64_t theNow)
{
	uint64_t aInner = 0;        // the inclusive cycles so far of the part inside the one being credited
	for (int i = gState.mDepth - 1; i >= 0; i--)
	{
		Frame& aFrame = gState.mStack[i];
		uint64_t aInclusive = theNow - aFrame.mStart;
		gState.mCycles[aFrame.mPart] += aInclusive - aFrame.mChild - aInner;
		aInner = aInclusive;
		aFrame.mStart = theNow;
		aFrame.mChild = 0;
	}
}

void Snapshot(const std::string& theLabel, int theUpdateCount)
{
	// first the loop's own counters and the process total, the other threads after
	LARGE_INTEGER aCounter;
	QueryPerformanceCounter(&aCounter);
	uint64_t aMain = ThreadCycles();
	CreditOpenParts(aMain);
	ULONG64 aProcess = 0;
	QueryProcessCycleTime(GetCurrentProcess(), &aProcess);

	char aBuf[1024];
	std::string aLine;
	snprintf(aBuf, sizeof(aBuf), "{\"label\":\"%s\",\"update\":%d,\"wall_s\":%.6f,\"process_cycles\":%llu,\"main_thread_id\":%lu,\"main_thread_cycles\":%llu,",
		Escaped(theLabel).c_str(), theUpdateCount, (aCounter.QuadPart - gState.mCounterStart) / gState.mCounterFreq,
		(unsigned long long) aProcess, (unsigned long) gState.mLoopThread, (unsigned long long) aMain);
	aLine += aBuf;
	static const char* const kNames[CYCLE_NUM_PARTS] = { "update", "draw", "present" };
	aLine += "\"parts\":{";
	for (int i = 0; i < CYCLE_NUM_PARTS; i++)
	{
		snprintf(aBuf, sizeof(aBuf), "%s\"%s\":{\"calls\":%llu,\"cycles\":%llu}", (i > 0) ? "," : "", kNames[i],
			(unsigned long long) gState.mCalls[i], (unsigned long long) gState.mCycles[i]);
		aLine += aBuf;
	}
	snprintf(aBuf, sizeof(aBuf), "},\"counter\":{\"calls\":%llu,\"cycles_each\":%llu,\"cycles_in_a_row\":%llu},\"misses\":%llu,\"threads\":[",
		(unsigned long long) gState.mReads, (unsigned long long) ((gState.mReads > 0) ? gState.mReadTsc / gState.mReads : gState.mReadCycles),
		(unsigned long long) gState.mReadCycles, (unsigned long long) gState.mMisses);
	aLine += aBuf;

	// every other thread of the process; ones that ended since the process began are in the process total only
	bool aFirst = true;
	HANDLE aSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (aSnap != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 anEntry;
		anEntry.dwSize = sizeof(anEntry);
		DWORD aPid = GetCurrentProcessId();
		for (BOOL ok = Thread32First(aSnap, &anEntry); ok; ok = Thread32Next(aSnap, &anEntry))
		{
			if ((anEntry.th32OwnerProcessID != aPid) || (anEntry.th32ThreadID == gState.mLoopThread))
				continue;
			HANDLE aThread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION, FALSE, anEntry.th32ThreadID);
			if (aThread == NULL)
				continue;
			ULONG64 aCycles = 0;
			if (QueryThreadCycleTime(aThread, &aCycles))
			{
				std::string aName, aStart;
				DescribeThread(aThread, &aName, &aStart);
				snprintf(aBuf, sizeof(aBuf), "%s{\"tid\":%lu,\"name\":\"%s\",\"start\":\"%s\",\"cycles\":%llu}", aFirst ? "" : ",",
					(unsigned long) anEntry.th32ThreadID, Escaped(aName).c_str(), Escaped(aStart).c_str(), (unsigned long long) aCycles);
				aLine += aBuf;
				aFirst = false;
			}
			CloseHandle(aThread);
		}
		CloseHandle(aSnap);
	}
	aLine += "]}";
	gState.mLines.push_back(aLine);

	// what the snapshot cost belongs to no part: the loop's clocks restart after it
	uint64_t aAfter = ThreadCycles();
	for (int i = 0; i < gState.mDepth; i++)
		gState.mStack[i].mStart = aAfter;
}

}   // namespace

bool CycleSplitStart(const std::string& theFileName)
{
	if (gCycleSplitStarted)
		return true;
	if (theFileName.empty())
	{
		SDL_Log("ERROR -cyclesplit needs a file name: -cyclesplit=<file>");
		return false;
	}
	ULONG64 aProbe = 0;
	if (!QueryThreadCycleTime(GetCurrentThread(), &aProbe))
	{
		SDL_Log("ERROR -cyclesplit: QueryThreadCycleTime is not available");
		return false;
	}
	gState.mFileName = theFileName;
	gState.mLoopThread = GetCurrentThreadId();
	LARGE_INTEGER aFreq, aNow;
	QueryPerformanceFrequency(&aFreq);
	QueryPerformanceCounter(&aNow);
	gState.mCounterFreq = (double) aFreq.QuadPart;
	gState.mCounterStart = aNow.QuadPart;

	// what one reading costs: a run of them, read by themselves
	const int kProbes = 2000;
	uint64_t aFirst = ThreadCycles();
	for (int i = 0; i < kProbes; i++)
		ThreadCycles();
	gState.mReadCycles = (ThreadCycles() - aFirst) / (kProbes + 1);

	gCycleSplitStarted = true;
	gCycleSplitOn = true;
	SDL_Log("cyclesplit: counting to %s (a cycle reading costs %llu cycles)", theFileName.c_str(), (unsigned long long) gState.mReadCycles);
	return true;
}

bool CycleSplitEnter(CyclePart thePart)
{
	if (GetCurrentThreadId() != gState.mLoopThread)
		return false;
	if (gState.mDepth >= kMaxDepth)
	{
		gState.mMisses++;
		return false;
	}
	Frame& aFrame = gState.mStack[gState.mDepth++];
	aFrame.mPart = thePart;
	aFrame.mChild = 0;
	gState.mReads++;
	aFrame.mStart = TimedThreadCycles();     // last, so that this function's own work is outside the part
	return true;
}

void CycleSplitLeave()
{
	uint64_t aNow = TimedThreadCycles();     // first, for the same reason
	gState.mReads++;
	if (gState.mDepth <= 0)
	{
		gState.mMisses++;
		return;
	}
	Frame aFrame = gState.mStack[--gState.mDepth];
	uint64_t aInclusive = aNow - aFrame.mStart;
	gState.mCycles[aFrame.mPart] += aInclusive - aFrame.mChild;
	gState.mCalls[aFrame.mPart]++;
	if (gState.mDepth > 0)
		gState.mStack[gState.mDepth - 1].mChild += aInclusive;
}

void CycleSplitMark(const std::string& theLabel, int theUpdateCount)
{
	if ((!gCycleSplitOn) || (gState.mFinished) || (GetCurrentThreadId() != gState.mLoopThread))
		return;
	Snapshot(theLabel, theUpdateCount);
}

bool CycleSplitPause(bool thePause)
{
	if (!gCycleSplitStarted)
		return false;
	gCycleSplitOn = !thePause;
	return true;
}

void CycleSplitFinish(int theUpdateCount)
{
	if ((!gCycleSplitStarted) || (gState.mFinished))
		return;
	Snapshot("exit", theUpdateCount);
	gState.mFinished = true;
	FILE* aFile = fopen(gState.mFileName.c_str(), "wb");
	if (aFile == NULL)
	{
		SDL_Log("ERROR -cyclesplit: cannot write %s", gState.mFileName.c_str());
		return;
	}
	for (size_t i = 0; i < gState.mLines.size(); i++)
		fprintf(aFile, "%s\n", gState.mLines[i].c_str());
	fclose(aFile);
	SDL_Log("cyclesplit: wrote %d snapshots to %s", (int) gState.mLines.size(), gState.mFileName.c_str());
}

}

#endif
