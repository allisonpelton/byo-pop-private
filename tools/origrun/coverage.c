// coverage.c -- origrun's --coverage recorder, in the sound stand-in: which of the original's functions a run reaches.
//
// origrun names a list of function addresses (ORIGRUN_COVERAGE, one hex address a line, from tools/coverage.py) and
// a file for the hits (ORIGRUN_COVERAGE_OUT). As the stand-in loads, the first byte of each listed function in the
// exe's executable sections becomes INT3, and a vectored exception handler takes each such breakpoint the first time it
// runs: it writes the address to the hits file at once (so a run that is killed keeps what it had), puts the original
// byte back and lets the function go on from its first instruction. Every function costs one exception at most, and
// after its first call it runs unchanged. Nothing in Windows is hooked: the breakpoints are in the game's own code.
//
// The recorder sees only what runs after the stand-in loads. origrun makes the copy's exe import the stand-in for a
// coverage run (coverage_import in origrun.c), so it loads with the exe's other DLLs, before the game's first
// instruction. Code that reads its own bytes (a checksum of the exe) would see the breakpoints; none of the games
// ported so far does.
#include <windows.h>
#include <stdlib.h>
#include "coverage.h"

static DWORD* gAddr;            // the listed functions, sorted
static BYTE* gOrig;             // each one's first byte
static BYTE* gHit;              // 1 once it ran
static int gN;
static LONG gCount;             // functions reached so far
static DWORD gMoved;            // how far Windows moved the exe from its preferred base (0 without ASLR)
static HANDLE gOut = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION gLock;


static void out_line(const char* s)
{
	DWORD w;
	if (gOut != INVALID_HANDLE_VALUE) WriteFile(gOut, s, lstrlenA(s), &w, NULL);
}

static int find_addr(DWORD a)
{
	int lo = 0, hi = gN - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (gAddr[mid] == a) return mid;
		if (gAddr[mid] < a) lo = mid + 1; else hi = mid - 1;
	}
	return -1;
}

static LONG CALLBACK coverage_handler(EXCEPTION_POINTERS* e)
{
	DWORD code = e->ExceptionRecord->ExceptionCode;
	if (code != EXCEPTION_BREAKPOINT && code != 0x4000001F) return EXCEPTION_CONTINUE_SEARCH;   // STATUS_WX86_BREAKPOINT too
	DWORD at = (DWORD)(ULONG_PTR)e->ExceptionRecord->ExceptionAddress;
	int i = find_addr(at);
	if (i < 0) {   // in case the address given is the next byte; only a breakpoint still in place can be ours
		at = e->ContextRecord->Eip - 1;
		i = find_addr(at);
		if (i >= 0 && gHit[i]) i = -1;
	}
	if (i < 0) return EXCEPTION_CONTINUE_SEARCH;                         // not ours: the game's own breakpoint
	EnterCriticalSection(&gLock);
	if (!gHit[i]) {
		DWORD old;
		if (VirtualProtect((void*)(ULONG_PTR)at, 1, PAGE_EXECUTE_READWRITE, &old)) {
			*(volatile BYTE*)(ULONG_PTR)at = gOrig[i];
			VirtualProtect((void*)(ULONG_PTR)at, 1, old, &old);
		}
		FlushInstructionCache(GetCurrentProcess(), (void*)(ULONG_PTR)at, 1);
		gHit[i] = 1;
		char line[64];
		wsprintfA(line, "%08lx %ld %lu\r\n", at - gMoved, ++gCount, GetCurrentThreadId());   // as the exe's file has it
		out_line(line);
	}
	LeaveCriticalSection(&gLock);
	e->ContextRecord->Eip = at;   // another thread may have put the byte back already; either way, run it from the start
	return EXCEPTION_CONTINUE_EXECUTION;
}

// The exe's preferred base, from its file: a moved image's header in memory has the base it got instead.
static DWORD file_image_base(void)
{
	char path[MAX_PATH];
	BYTE head[4096];
	DWORD got = 0;
	if (!GetModuleFileNameA(NULL, path, MAX_PATH)) return 0;
	HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	if (f == INVALID_HANDLE_VALUE) return 0;
	BOOL ok = ReadFile(f, head, sizeof(head), &got, NULL);
	CloseHandle(f);
	LONG at = ((IMAGE_DOS_HEADER*)head)->e_lfanew;
	if (!ok || got < sizeof(IMAGE_DOS_HEADER) || at < 0 || (DWORD)at + sizeof(IMAGE_NT_HEADERS32) > got) return 0;
	return ((IMAGE_NT_HEADERS32*)(head + at))->OptionalHeader.ImageBase;
}

static int cmp_dword(const void* a, const void* b)
{
	DWORD x = *(const DWORD*)a, y = *(const DWORD*)b;
	return x < y ? -1 : x > y;
}

// The section of the exe that holds a, if it is executable code.
static IMAGE_SECTION_HEADER* code_section(BYTE* base, IMAGE_NT_HEADERS32* nt, DWORD a)
{
	IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
	DWORD rva = a - (DWORD)(ULONG_PTR)base;
	for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
		DWORD size = s[i].Misc.VirtualSize ? s[i].Misc.VirtualSize : s[i].SizeOfRawData;
		if (rva >= s[i].VirtualAddress && rva < s[i].VirtualAddress + size)
			return (s[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) ? &s[i] : NULL;
	}
	return NULL;
}

void coverage_install(void)
{
	char list[MAX_PATH], outPath[MAX_PATH];
	if (!GetEnvironmentVariableA("ORIGRUN_COVERAGE", list, MAX_PATH)) return;
	if (!GetEnvironmentVariableA("ORIGRUN_COVERAGE_OUT", outPath, MAX_PATH)) return;
	gOut = CreateFileA(outPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (gOut == INVALID_HANDLE_VALUE) return;
	char line[MAX_PATH + 128];

	HANDLE f = CreateFileA(list, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	DWORD size = f != INVALID_HANDLE_VALUE ? GetFileSize(f, NULL) : 0;
	char* text = size && size != INVALID_FILE_SIZE ? (char*)HeapAlloc(GetProcessHeap(), 0, size + 1) : NULL;
	DWORD got = 0;
	if (text && !ReadFile(f, text, size, &got, NULL)) got = 0;
	if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
	if (!text || !got) { wsprintfA(line, "# coverage: cannot read the address list %s\r\n", list); out_line(line); return; }
	text[got] = 0;

	int cap = 1;
	for (DWORD k = 0; k < got; k++) if (text[k] == '\n') cap++;
	gAddr = (DWORD*)HeapAlloc(GetProcessHeap(), 0, cap * sizeof(DWORD));
	for (char* p = text; *p; ) {
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
		if (*p == '#') { while (*p && *p != '\n') p++; continue; }
		char* end;
		unsigned long a = strtoul(p, &end, 16);
		if (end == p) { while (*p && *p != '\n') p++; continue; }
		if (gN < cap) gAddr[gN++] = (DWORD)a;
		p = end;
		while (*p && *p != '\n') p++;
	}
	HeapFree(GetProcessHeap(), 0, text);
	qsort(gAddr, gN, sizeof(DWORD), cmp_dword);
	int n = 0;
	for (int i = 0; i < gN; i++) if (n == 0 || gAddr[i] != gAddr[n - 1]) gAddr[n++] = gAddr[i];
	gN = n;

	// The list has the addresses of the exe's file (its preferred base); an exe that Windows moved (ASLR) gets them
	// moved too. Then keep only the addresses in the exe's code: a breakpoint written into data would change what the
	// game reads.
	BYTE* base = (BYTE*)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
	DWORD preferred = file_image_base();
	gMoved = preferred ? (DWORD)(ULONG_PTR)base - preferred : 0;
	for (int i = 0; i < gN; i++) gAddr[i] += gMoved;
	int skipped = 0;
	n = 0;
	for (int i = 0; i < gN; i++) {
		if (code_section(base, nt, gAddr[i])) gAddr[n++] = gAddr[i];
		else skipped++;
	}
	gN = n;
	gOrig = (BYTE*)HeapAlloc(GetProcessHeap(), 0, gN ? gN : 1);
	gHit = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, gN ? gN : 1);
	InitializeCriticalSection(&gLock);

	// One VirtualProtect for each code section, then every breakpoint.
	IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
	for (int k = 0; k < nt->FileHeader.NumberOfSections; k++) {
		if (!(s[k].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
		BYTE* lo = base + s[k].VirtualAddress;
		DWORD len = s[k].Misc.VirtualSize ? s[k].Misc.VirtualSize : s[k].SizeOfRawData;
		DWORD old;
		if (!VirtualProtect(lo, len, PAGE_EXECUTE_READWRITE, &old)) continue;
		for (int i = 0; i < gN; i++) {
			BYTE* at = (BYTE*)(ULONG_PTR)gAddr[i];
			if (at < lo || at >= lo + len) continue;
			gOrig[i] = *at;
			*at = 0xCC;
		}
		VirtualProtect(lo, len, old, &old);
		FlushInstructionCache(GetCurrentProcess(), lo, len);
	}
	AddVectoredExceptionHandler(1, coverage_handler);
	wsprintfA(line, "# coverage: %d functions watched (%d listed addresses outside the exe's code left out), from %s\r\n",
		gN, skipped, list);
	out_line(line);
}

void coverage_detach(void)
{
	if (gOut == INVALID_HANDLE_VALUE) return;
	char line[96];
	wsprintfA(line, "# coverage: the game ended normally, %ld of %d functions reached\r\n", gCount, gN);
	out_line(line);
	CloseHandle(gOut);
	gOut = INVALID_HANDLE_VALUE;
}
