// The memory report behind the "memreport" test command (see MemReport.h).

#include "MemReport.h"
#include "Audio.h"

#include <SDL.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define PSAPI_VERSION 2     // GetProcessMemoryInfo is then kernel32's K32GetProcessMemoryInfo: no psapi library to link
#include <windows.h>
#include <psapi.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten/heap.h>
#include <malloc.h>
#endif

namespace Platform
{

static bool gStartNoted = false;
static size_t gStartPrivateBytes = 0;

static std::string Format(const char* theFormat, ...)
{
	char aBuf[256];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuf, sizeof(aBuf), theFormat, anArgs);
	va_end(anArgs);
	return aBuf;
}

bool GetProcessMemory(size_t* theWorkingSet, size_t* thePrivateBytes)
{
	*theWorkingSet = 0;
	*thePrivateBytes = 0;
#ifdef _WIN32
	PROCESS_MEMORY_COUNTERS_EX aCounters;
	memset(&aCounters, 0, sizeof(aCounters));
	aCounters.cb = sizeof(aCounters);
	if (!GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*) &aCounters, sizeof(aCounters)))
		return false;
	*theWorkingSet = aCounters.WorkingSetSize;
	*thePrivateBytes = aCounters.PrivateUsage;
	return true;
#elif defined(__EMSCRIPTEN__)
	*theWorkingSet = emscripten_get_heap_size();
	*thePrivateBytes = *theWorkingSet;
	return true;
#else
	return false;
#endif
}

void NoteStartMemory()
{
	if (gStartNoted)
		return;
	gStartNoted = true;
	size_t aWorkingSet = 0;
	GetProcessMemory(&aWorkingSet, &gStartPrivateBytes);
}

size_t GetStartPrivateBytes()
{
	return gStartPrivateBytes;
}

void AddSongMemory(MemReport* theReport)
{
	int aCount = 0;
	size_t aTrackBytes = 0, aModuleFileBytes = 0;
	Mixer::Get().GetSongMemory(&aCount, &aTrackBytes, &aModuleFileBytes);
	theReport->mSongCount += aCount;
	theReport->mSongTrackBytes += aTrackBytes;
	theReport->mSongModuleFileBytes += aModuleFileBytes;
}

std::string FormatMemReport(const MemReport& theReport)
{
	size_t aWorkingSet = 0, aPrivateBytes = 0;
	GetProcessMemory(&aWorkingSet, &aPrivateBytes);
	std::string aLine = Format("{\"images\":{\"count\":%d,\"textured\":%d,\"pixels\":%llu,\"copies\":%llu,\"textures\":%llu}", theReport.mImageCount,
		theReport.mImageTexturedCount, (unsigned long long) theReport.mImagePixelBytes, (unsigned long long) theReport.mImageCopyBytes,
		(unsigned long long) theReport.mImageTextureBytes);
	aLine += Format(",\"sounds\":{\"count\":%d,\"bytes\":%llu}", theReport.mSoundCount, (unsigned long long) theReport.mSoundBytes);
	aLine += Format(",\"music\":{\"count\":%d,\"tracks\":%llu,\"moduleFiles\":%llu}", theReport.mSongCount,
		(unsigned long long) theReport.mSongTrackBytes, (unsigned long long) theReport.mSongModuleFileBytes);
	aLine += ",\"others\":{";
	for (size_t i = 0; i < theReport.mOthers.size(); i++)
	{
		// a holder's name is the engine's own word; anything that would break the line becomes '_'
		std::string aName = theReport.mOthers[i].first;
		for (size_t j = 0; j < aName.size(); j++)
			if (aName[j] == '"' || aName[j] == '\\' || (unsigned char) aName[j] < 0x20)
				aName[j] = '_';
		aLine += std::string((i > 0) ? "," : "") + "\"" + aName + "\":" + Format("%llu", (unsigned long long) theReport.mOthers[i].second);
	}
	aLine += Format("},\"process\":{\"workingSet\":%llu,\"private\":%llu,\"privateAtStart\":%llu}}", (unsigned long long) aWorkingSet,
		(unsigned long long) aPrivateBytes, (unsigned long long) gStartPrivateBytes);
	return aLine;
}

void LogMemReport(const MemReport& theReport)
{
	SDL_Log("MEMREPORT %s", FormatMemReport(theReport).c_str());
}

std::string DescribeHeaps()
{
	const double MB = 1024.0 * 1024.0;
	std::string aText;
#ifdef _WIN32
	HANDLE aHeaps[64];
	DWORD aHeapCount = GetProcessHeaps(64, aHeaps);
	for (DWORD i = 0; (i < aHeapCount) && (i < 64); i++)
	{
		size_t aBusy = 0, aFree = 0, aSmall[3] = {0, 0, 0};
		std::vector<size_t> aLarge;
		int aBlocks = 0;
		PROCESS_HEAP_ENTRY anEntry;
		anEntry.lpData = NULL;
		if (!HeapLock(aHeaps[i]))
			continue;
		while (HeapWalk(aHeaps[i], &anEntry))
		{
			if (anEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			{
				aBusy += anEntry.cbData;
				aBlocks++;
				if (anEntry.cbData >= 64 * 1024)
					aLarge.push_back(anEntry.cbData);
				else
					aSmall[(anEntry.cbData < 256) ? 0 : (anEntry.cbData < 4096) ? 1 : 2] += anEntry.cbData;
			}
			else if ((anEntry.wFlags & (PROCESS_HEAP_REGION | PROCESS_HEAP_UNCOMMITTED_RANGE)) == 0)
				aFree += anEntry.cbData;
		}
		HeapUnlock(aHeaps[i]);
		std::sort(aLarge.rbegin(), aLarge.rend());
		std::string aSizes;
		size_t aLargeSum = 0;
		for (size_t aSize : aLarge)
		{
			aLargeSum += aSize;
			aSizes += Format(" %zu", aSize);
		}
		aText += Format("heap %d%s: %.2f MB in %d blocks, %.2f MB free\n", (int) i, (aHeaps[i] == GetProcessHeap()) ? " (process)" : "",
			aBusy / MB, aBlocks, aFree / MB);
		if (aBusy > 0)
			aText += Format("  blocks under 256 bytes %.2f MB, under 4 KB %.2f MB, under 64 KB %.2f MB; %d of 64 KB or more, %.2f MB:\n",
				aSmall[0] / MB, aSmall[1] / MB, aSmall[2] / MB, (int) aLarge.size(), aLargeSum / MB) + " " + aSizes + "\n";
	}
#elif defined(__EMSCRIPTEN__)
	struct mallinfo anInfo = mallinfo();
	aText += Format("heap: %.2f MB in use, %.2f MB free\n", anInfo.uordblks / MB, anInfo.fordblks / MB);
#endif
	return aText;
}

}
