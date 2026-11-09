// port: the heap part of the memory report (MemReport.cpp), apart since <windows.h> clashes with the framework's types
#include <cstdarg>
#include <cstdio>
#include <string>
#include <algorithm>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#elif defined(__EMSCRIPTEN__)
#include <malloc.h>
#endif

static std::string Format(const char* theFormat, ...)
{
	char aBuf[256];
	va_list anArgs;
	va_start(anArgs, theFormat);
	vsnprintf(aBuf, sizeof(aBuf), theFormat, anArgs);
	va_end(anArgs);
	return aBuf;
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
