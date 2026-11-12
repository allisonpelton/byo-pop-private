// leakhook.cpp: counts the port's live allocations by the code that made them, for the leak check (GAME_NOTES.md,
// "Performance"). It replaces the global operator new and delete, keeps each live block's size and its caller's return
// address in a table of its own (malloc'd, so it never calls itself), and when the process exits writes the blocks still
// live to the file LEAKHOOK_OUT names: one line "<caller rva> <blocks> <bytes>" for each caller, the address relative to
// the exe's base. Never part of a game: it is linked only into a scratch build,
//   tools/build.sh's native() with -DCMAKE_EXE_LINKER_FLAGS=<this file's object>
// (games/dynomite/tools/profile/leakcheck.sh does it all). A leak is a caller whose count grows with play: two runs to
// the same screen, one game against two, and the callers that differ.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace
{
struct Block
{
	void*	mPtr;	// NULL: empty; TOMB: removed
	size_t	mSize;
	void*	mCaller;
};

void* const TOMB = (void*) 1;
Block* gTable = NULL;
size_t gCap = 0;
size_t gUsed = 0;	// live and tombstones
CRITICAL_SECTION gLock;
bool gReady = false;

size_t Hash(void* p)
{
	size_t h = (size_t) p;
	h ^= h >> 15;
	h *= 0x2c1b3c6dU;
	h ^= h >> 12;
	return h;
}

void Insert(Block* theTable, size_t theCap, void* p, size_t theSize, void* theCaller)
{
	size_t i = Hash(p) & (theCap - 1);
	while ((theTable[i].mPtr != NULL) && (theTable[i].mPtr != TOMB))
		i = (i + 1) & (theCap - 1);
	theTable[i].mPtr = p;
	theTable[i].mSize = theSize;
	theTable[i].mCaller = theCaller;
}

void Grow()
{
	size_t aCap = (gCap == 0) ? (1 << 16) : gCap * 2;
	Block* aTable = (Block*) calloc(aCap, sizeof(Block));
	size_t aLive = 0;
	for (size_t i = 0; i < gCap; i++)
		if ((gTable[i].mPtr != NULL) && (gTable[i].mPtr != TOMB))
		{
			Insert(aTable, aCap, gTable[i].mPtr, gTable[i].mSize, gTable[i].mCaller);
			aLive++;
		}
	free(gTable);
	gTable = aTable;
	gCap = aCap;
	gUsed = aLive;
}

void Init()
{
	if (!gReady)
	{
		InitializeCriticalSection(&gLock);
		gReady = true;
	}
}

void Add(void* p, size_t theSize, void* theCaller)
{
	if (p == NULL)
		return;
	Init();
	EnterCriticalSection(&gLock);
	if ((gUsed + 1) * 2 > gCap)
		Grow();
	Insert(gTable, gCap, p, theSize, theCaller);
	gUsed++;
	LeaveCriticalSection(&gLock);
}

void Remove(void* p)
{
	if ((p == NULL) || (!gReady))
		return;
	EnterCriticalSection(&gLock);
	if (gCap != 0)
	{
		size_t i = Hash(p) & (gCap - 1);
		while (gTable[i].mPtr != NULL)
		{
			if (gTable[i].mPtr == p)
			{
				gTable[i].mPtr = TOMB;
				break;
			}
			i = (i + 1) & (gCap - 1);
		}
	}
	LeaveCriticalSection(&gLock);
}

void* Alloc(size_t theSize, void* theCaller)
{
	void* p = malloc(theSize ? theSize : 1);
	Add(p, theSize, theCaller);
	return p;
}

void Dump()
{
	const char* aName = getenv("LEAKHOOK_OUT");
	if ((aName == NULL) || (!gReady))
		return;
	EnterCriticalSection(&gLock);
	// one row per caller: a second table keyed by the caller
	size_t aCap = 1 << 14;
	struct Row { void* mCaller; size_t mBlocks; size_t mBytes; };
	Row* aRows = (Row*) calloc(aCap, sizeof(Row));
	for (size_t i = 0; i < gCap; i++)
	{
		if ((gTable[i].mPtr == NULL) || (gTable[i].mPtr == TOMB))
			continue;
		size_t j = Hash(gTable[i].mCaller) & (aCap - 1);
		while ((aRows[j].mCaller != NULL) && (aRows[j].mCaller != gTable[i].mCaller))
			j = (j + 1) & (aCap - 1);
		aRows[j].mCaller = gTable[i].mCaller;
		aRows[j].mBlocks++;
		aRows[j].mBytes += gTable[i].mSize;
	}
	size_t aBase = (size_t) GetModuleHandleA(NULL);
	if (FILE* f = fopen(aName, "w"))
	{
		for (size_t j = 0; j < aCap; j++)
			if (aRows[j].mCaller != NULL)
				fprintf(f, "%08zx %zu %zu\n", (size_t) aRows[j].mCaller - aBase, aRows[j].mBlocks, aRows[j].mBytes);
		fclose(f);
	}
	free(aRows);
	LeaveCriticalSection(&gLock);
}

// written as the process ends: atexit runs after main returns, before the C runtime's own teardown
struct Dumper
{
	Dumper() { atexit(Dump); }
} gDumper;
}

void* operator new(size_t theSize) { return Alloc(theSize, __builtin_return_address(0)); }
void* operator new[](size_t theSize) { return Alloc(theSize, __builtin_return_address(0)); }
void* operator new(size_t theSize, const std::nothrow_t&) noexcept { return Alloc(theSize, __builtin_return_address(0)); }
void* operator new[](size_t theSize, const std::nothrow_t&) noexcept { return Alloc(theSize, __builtin_return_address(0)); }
void operator delete(void* p) noexcept { Remove(p); free(p); }
void operator delete[](void* p) noexcept { Remove(p); free(p); }
void operator delete(void* p, size_t) noexcept { Remove(p); free(p); }
void operator delete[](void* p, size_t) noexcept { Remove(p); free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { Remove(p); free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { Remove(p); free(p); }
