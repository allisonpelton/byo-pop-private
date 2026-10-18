// lsprobe: finds and reads the LiveSplit block of a running Bejeweled Deluxe port process from outside, the
// way games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl does, and logs what it reads
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15, test 3). Built as a 64-bit exe (llvm-mingw x86_64), so it
// reads the 32-bit game across WOW64 as 64-bit LiveSplit does.
//
//   lsprobe [-log=<file>] [-interval=<ms>] [-scan=<ms>] -- <game exe> [game arguments...]
//       starts the game (its stdout and stderr are lsprobe's) and polls it until it exits; exits with the
//       game's exit status (2 for lsprobe's own errors)
//   lsprobe [-log=<file>] [-interval=<ms>] [-scan=<ms>] -attach=<pid>
//       polls a running game until it exits
//   lsprobe -dumplayout
//       prints the magic and the field table of layout 1 (for check_asl_offsets.py) and exits
//
// Polling, as the script does: every -interval ms (default 10: refreshRate 100, a Forms timer without
// timeBeginPeriod, so about 16 ms in practice; the stress run against a -fast game uses 1, so reads overlap
// the game's writes). Until the block is found, it looks again every -scan ms (default 1000, as the script;
// a short -fast game needs less). Finding: the main
// module (the first one EnumProcessModulesEx lists, as MainModuleWow64Safe), its PE header read from the
// game's memory (MZ, PE\0\0, positive sizes), each IMAGE_SCN_MEM_WRITE section in one read, at 4-byte
// steps: magic, game id, layout >= 1, size >= 0xB0, self 0 or the hit's own address. Reading: seq, the
// 0xB0 bytes, seq, in three ReadProcessMemory calls; a copy counts when both seqs are equal and even and
// the copy's own seq is the same, else up to 3 retries, then the poll is skipped.
//
// Log lines ("t" is milliseconds since lsprobe started, "now" its QueryPerformanceCounter at the read, the
// same clock as the block's qpc):
//   found t=<ms> addr=<hex> base=<hex> size=<hex> scans=<n>   the block (also after the game moved it: never)
//   snap t=<ms> now=<qpc> <the block's fields>                 each accepted copy whose seq is new
//   same t=<ms> now=<qpc>                                      an accepted copy with the last snap's seq
//   retry t=<ms> tries=<n>                                     a copy accepted after n failed tries
//   skip t=<ms>                                                no consistent copy in 4 tries
//   lost t=<ms>                                                the block failed its header check again
//   exit t=<ms> status=<n> polls=<n> snaps=<n> retries=<n> skips=<n> missed=<n>
// Once the block is found every poll logs one line (snap, same or skip), so lsdecide.py can replay the
// script's polls exactly. "missed": block writes the probe never saw (the gaps between the updates counters).

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>

#include "LiveSplitBlock.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{

FILE* gLog = NULL;
LARGE_INTEGER gStart;
LARGE_INTEGER gFrequency;

long long NowQpc()
{
	LARGE_INTEGER aNow;
	QueryPerformanceCounter(&aNow);
	return aNow.QuadPart;
}

long long ElapsedMs()
{
	return (NowQpc() - gStart.QuadPart) * 1000 / gFrequency.QuadPart;
}

void Log(const char* theFormat, ...)
{
	if (gLog == NULL)
		return;
	va_list anArgs;
	va_start(anArgs, theFormat);
	vfprintf(gLog, theFormat, anArgs);
	va_end(anArgs);
	fputc('\n', gLog);
}

bool ReadMemory(HANDLE theProcess, uint64_t theAddress, void* theBuffer, size_t theSize)
{
	SIZE_T aRead = 0;
	return ReadProcessMemory(theProcess, (LPCVOID) (uintptr_t) theAddress, theBuffer, theSize, &aRead) && (aRead == theSize);
}

uint16_t U16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	return (uint16_t) (theData[theOffset] | (theData[theOffset + 1] << 8));
}

uint32_t U32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	return (uint32_t) theData[theOffset] | ((uint32_t) theData[theOffset + 1] << 8) | ((uint32_t) theData[theOffset + 2] << 16) |
		((uint32_t) theData[theOffset + 3] << 24);
}

// The main module (the first module listed, as LiveSplit's MainModuleWow64Safe): false while the game is
// still starting
bool MainModule(HANDLE theProcess, uint64_t* theBase, uint32_t* theSize)
{
	HMODULE aModules[1024];
	DWORD aNeeded = 0;
	if (!EnumProcessModulesEx(theProcess, aModules, sizeof(aModules), &aNeeded, LIST_MODULES_ALL) || (aNeeded < sizeof(HMODULE)))
		return false;
	MODULEINFO anInfo;
	if (!GetModuleInformation(theProcess, aModules[0], &anInfo, sizeof(anInfo)))
		return false;
	*theBase = (uint64_t) (uintptr_t) anInfo.lpBaseOfDll;
	*theSize = anInfo.SizeOfImage;
	return true;
}

// The block's address, 0 if not found (the script's FindBlock)
uint64_t FindBlock(HANDLE theProcess, uint64_t* theModuleBase, uint32_t* theModuleSize)
{
	static const uint8_t aMagic[LIVESPLIT_MAGIC_SIZE] = LIVESPLIT_MAGIC_BYTES;
	uint64_t aBase;
	uint32_t aSize;
	if (!MainModule(theProcess, &aBase, &aSize) || (aSize == 0))
		return 0;
	*theModuleBase = aBase;
	*theModuleSize = aSize;
	std::vector<uint8_t> aHeader(aSize < 0x1000 ? aSize : 0x1000);
	if (!ReadMemory(theProcess, aBase, &aHeader[0], aHeader.size()) || (aHeader.size() < 0x40) || (aHeader[0] != 'M') || (aHeader[1] != 'Z'))
		return 0;
	uint32_t aPe = U32(aHeader, 0x3C);
	if ((aPe == 0) || (aPe + 24 > aHeader.size()) || (U32(aHeader, aPe) != 0x00004550))
		return 0;
	uint32_t aSections = U16(aHeader, aPe + 6);
	uint32_t aTable = aPe + 24 + U16(aHeader, aPe + 20);
	for (uint32_t i = 0; i < aSections; i++)
	{
		uint32_t anEntry = aTable + i * 40;
		if (anEntry + 40 > aHeader.size())
			break;
		int32_t aVirtualSize = (int32_t) U32(aHeader, anEntry + 8);
		int32_t aVirtualAddress = (int32_t) U32(aHeader, anEntry + 12);
		uint32_t aCharacteristics = U32(aHeader, anEntry + 36);
		if (((aCharacteristics & 0x80000000u) == 0) || (aVirtualSize <= 0) || (aVirtualAddress <= 0) ||
			((uint64_t) aVirtualAddress + (uint64_t) aVirtualSize > aSize))
			continue;
		std::vector<uint8_t> aData((size_t) aVirtualSize);
		if (!ReadMemory(theProcess, aBase + (uint32_t) aVirtualAddress, &aData[0], aData.size()))
			continue;
		for (size_t anAt = 0; anAt + LIVESPLIT_SIZE_V1 <= aData.size(); anAt += 4)
		{
			if ((memcmp(&aData[anAt], aMagic, LIVESPLIT_MAGIC_SIZE) != 0) || (U32(aData, anAt + 0x14) != LIVESPLIT_GAME_ID) ||
				(U16(aData, anAt + 0x18) < LIVESPLIT_LAYOUT_VERSION) || (U16(aData, anAt + 0x1A) < LIVESPLIT_SIZE_V1))
				continue;
			uint64_t anAddress = aBase + (uint32_t) aVirtualAddress + anAt;
			uint32_t aSelf = U32(aData, anAt + 0x1C);
			if ((aSelf != 0) && ((uint64_t) aSelf != anAddress))
				continue;
			return anAddress;
		}
	}
	return 0;
}

enum ReadResult { READ_OK, READ_TORN, READ_FAILED };

// One consistent copy (the script's ReadBlock): seq, the block, seq
ReadResult ReadBlock(HANDLE theProcess, uint64_t theAddress, LiveSplitBlockV1* theBlock, int* theTries)
{
	for (int aTry = 0; aTry < 4; aTry++)
	{
		uint32_t aSeq1 = 0;
		uint32_t aSeq2 = 0;
		if (!ReadMemory(theProcess, theAddress + offsetof(LiveSplitBlockV1, seq), &aSeq1, 4) ||
			!ReadMemory(theProcess, theAddress, theBlock, sizeof(LiveSplitBlockV1)) ||
			!ReadMemory(theProcess, theAddress + offsetof(LiveSplitBlockV1, seq), &aSeq2, 4))
			return READ_FAILED;
		if ((aSeq1 == aSeq2) && ((aSeq1 & 1) == 0) && (theBlock->seq == aSeq1))
		{
			*theTries = aTry;
			return READ_OK;
		}
	}
	return READ_TORN;
}

void DumpLayout()
{
	static const uint8_t aMagic[LIVESPLIT_MAGIC_SIZE] = LIVESPLIT_MAGIC_BYTES;
	printf("magic");
	for (uint32_t i = 0; i < LIVESPLIT_MAGIC_SIZE; i++)
		printf(" %02X", aMagic[i]);
	printf("\ngameId 0x%08X\nlayoutVersion %u\nsize 0x%X\n", (unsigned) LIVESPLIT_GAME_ID, (unsigned) LIVESPLIT_LAYOUT_VERSION,
		(unsigned) LIVESPLIT_SIZE_V1);
#define FIELD(name, type) printf("field %s 0x%X %s\n", #name, (unsigned) offsetof(LiveSplitBlockV1, name), type)
	FIELD(gameId, "u32");
	FIELD(layoutVersion, "u16");
	FIELD(size, "u16");
	FIELD(self, "u32");
	FIELD(seq, "u32");
	FIELD(active, "u32");
	FIELD(updates, "u32");
	FIELD(flags, "u32");
	FIELD(boardSerial, "u32");
	FIELD(goCount, "u32");
	FIELD(goSerial, "u32");
	FIELD(levelClears, "u32");
	FIELD(levelClearSerial, "u32");
	FIELD(gameOvers, "u32");
	FIELD(gameOverSerial, "u32");
	FIELD(deathsNoMoves, "u32");
	FIELD(deathsTimeUp, "u32");
	FIELD(deathsNewGame, "u32");
	FIELD(deathsOther, "u32");
	FIELD(tick, "i32");
	FIELD(state, "i32");
	FIELD(level, "i32");
	FIELD(displayLevel, "i32");
	FIELD(displayScore, "i32");
	FIELD(score, "i32");
	FIELD(boardLevelClears, "i32");
	FIELD(goTick, "i32");
	FIELD(displayScoreMax, "i32");
	FIELD(rulesId, "u8[32]");
	FIELD(qpc, "u64");
	FIELD(qpcFrequency, "u64");
#undef FIELD
	printf("flags timed=0x%02X resumed=0x%02X vanilla=0x%02X chosenSeed=0x%02X ownRng=0x%02X replay=0x%02X gameOver=0x%02X paused=0x%02X\n",
		(unsigned) LIVESPLIT_FLAG_TIMED, (unsigned) LIVESPLIT_FLAG_RESUMED, (unsigned) LIVESPLIT_FLAG_VANILLA,
		(unsigned) LIVESPLIT_FLAG_CHOSEN_SEED, (unsigned) LIVESPLIT_FLAG_OWN_RNG, (unsigned) LIVESPLIT_FLAG_REPLAY,
		(unsigned) LIVESPLIT_FLAG_GAME_OVER, (unsigned) LIVESPLIT_FLAG_PAUSED);
}

// argv[from..] as one command line (Windows quoting rules)
std::string CommandLine(int argc, char** argv, int theFrom)
{
	std::string aLine;
	for (int i = theFrom; i < argc; i++)
	{
		std::string anArg = argv[i];
		if (!aLine.empty())
			aLine += ' ';
		if (!anArg.empty() && (anArg.find_first_of(" \t\"") == std::string::npos))
		{
			aLine += anArg;
			continue;
		}
		aLine += '"';
		size_t aSlashes = 0;
		for (char c : anArg)
		{
			if (c == '\\')
			{
				aSlashes++;
				continue;
			}
			if (c == '"')
				aLine.append(aSlashes * 2 + 1, '\\');
			else
				aLine.append(aSlashes, '\\');
			aSlashes = 0;
			aLine += c;
		}
		aLine.append(aSlashes * 2, '\\');
		aLine += '"';
	}
	return aLine;
}

} // namespace

int main(int argc, char** argv)
{
	std::string aLogName;
	int anInterval = 10;
	int aScanInterval = 1000;
	DWORD anAttach = 0;
	int aCommand = -1;
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		if (!strcmp(a, "--"))
		{
			aCommand = i + 1;
			break;
		}
		if (!strcmp(a, "-dumplayout"))
		{
			DumpLayout();
			return 0;
		}
		if (!strncmp(a, "-log=", 5))
			aLogName = a + 5;
		else if (!strncmp(a, "-interval=", 10))
			anInterval = atoi(a + 10);
		else if (!strncmp(a, "-scan=", 6))
			aScanInterval = atoi(a + 6);
		else if (!strncmp(a, "-attach=", 8))
			anAttach = (DWORD) strtoul(a + 8, NULL, 10);
		else
		{
			fprintf(stderr, "lsprobe: unknown argument %s (see the comment at the top of lsprobe.cpp)\n", a);
			return 2;
		}
	}
	if ((anAttach == 0) && ((aCommand < 0) || (aCommand >= argc)))
	{
		fprintf(stderr, "usage: lsprobe [-log=<file>] [-interval=<ms>] [-scan=<ms>] (-- <game exe> [args...] | -attach=<pid>) | -dumplayout\n");
		return 2;
	}
	if (!aLogName.empty())
	{
		gLog = fopen(aLogName.c_str(), "wb");
		if (gLog == NULL)
		{
			fprintf(stderr, "lsprobe: cannot write %s\n", aLogName.c_str());
			return 2;
		}
	}
	QueryPerformanceFrequency(&gFrequency);
	QueryPerformanceCounter(&gStart);

	HANDLE aProcess = NULL;
	if (anAttach != 0)
	{
		aProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, anAttach);
		if (aProcess == NULL)
		{
			fprintf(stderr, "lsprobe: cannot open process %lu\n", (unsigned long) anAttach);
			return 2;
		}
	}
	else
	{
		std::string aLine = CommandLine(argc, argv, aCommand);
		std::vector<char> aBuffer(aLine.begin(), aLine.end());
		aBuffer.push_back(0);
		STARTUPINFOA aStartup;
		memset(&aStartup, 0, sizeof(aStartup));
		aStartup.cb = sizeof(aStartup);
		PROCESS_INFORMATION anInfo;
		if (!CreateProcessA(NULL, &aBuffer[0], NULL, NULL, TRUE, 0, NULL, NULL, &aStartup, &anInfo))
		{
			fprintf(stderr, "lsprobe: cannot start %s (error %lu)\n", aLine.c_str(), (unsigned long) GetLastError());
			return 2;
		}
		CloseHandle(anInfo.hThread);
		aProcess = anInfo.hProcess;
	}

	uint64_t aBlock = 0;
	uint64_t aModuleBase = 0;
	uint32_t aModuleSize = 0;
	long long aLastScan = -1000000000LL;
	int aScans = 0;
	uint32_t aLastSeq = 0;
	uint32_t aLastUpdates = 0;
	bool haveSnap = false;
	long long aPolls = 0;
	long long aSnaps = 0;
	long long aRetries = 0;
	long long aSkips = 0;
	long long aMissed = 0;
	char aText[600];
	for (;;)
	{
		bool hasExited = WaitForSingleObject(aProcess, 0) == WAIT_OBJECT_0;
		aPolls++;
		if (aBlock == 0)
		{
			if (ElapsedMs() - aLastScan >= aScanInterval)
			{
				aLastScan = ElapsedMs();
				aScans++;
				aBlock = FindBlock(aProcess, &aModuleBase, &aModuleSize);
				if (aBlock != 0)
					Log("found t=%lld addr=%llx base=%llx size=%x scans=%d", ElapsedMs(), (unsigned long long) aBlock,
						(unsigned long long) aModuleBase, (unsigned) aModuleSize, aScans);
			}
		}
		if (aBlock != 0)
		{
			LiveSplitBlockV1 aCopy;
			int aTries = 0;
			ReadResult aResult = ReadBlock(aProcess, aBlock, &aCopy, &aTries);
			long long aNow = NowQpc();
			if (aResult == READ_OK)
			{
				static const uint8_t aMagic[LIVESPLIT_MAGIC_SIZE] = LIVESPLIT_MAGIC_BYTES;
				if ((memcmp(aCopy.magic, aMagic, LIVESPLIT_MAGIC_SIZE) != 0) || (aCopy.gameId != LIVESPLIT_GAME_ID))
				{
					Log("lost t=%lld", ElapsedMs());
					aBlock = 0;
				}
				else
				{
					if (aTries > 0)
					{
						aRetries++;
						Log("retry t=%lld tries=%d", ElapsedMs(), aTries);
					}
					if (!haveSnap || (aCopy.seq != aLastSeq))
					{
						if (haveSnap && (aCopy.updates > aLastUpdates + 1))
							aMissed += aCopy.updates - aLastUpdates - 1;
						haveSnap = true;
						aLastSeq = aCopy.seq;
						aLastUpdates = aCopy.updates;
						aSnaps++;
						FormatLiveSplitBlock(aCopy, false, aText, sizeof(aText));
						Log("snap t=%lld now=%lld %s", ElapsedMs(), aNow, aText);
					}
					else
						Log("same t=%lld now=%lld", ElapsedMs(), aNow);
				}
			}
			else if (aResult == READ_TORN)
			{
				aSkips++;
				Log("skip t=%lld", ElapsedMs());
			}
		}
		if (hasExited)
			break;
		Sleep((anInterval > 0) ? anInterval : 1);
	}
	DWORD aStatus = 0;
	GetExitCodeProcess(aProcess, &aStatus);
	CloseHandle(aProcess);
	Log("exit t=%lld status=%lu polls=%lld snaps=%lld retries=%lld skips=%lld missed=%lld", ElapsedMs(), (unsigned long) aStatus,
		aPolls, aSnaps, aRetries, aSkips, aMissed);
	if (gLog != NULL)
		fclose(gLog);
	return (int) aStatus;
}
