// count.c -- origrun's --count counters, in the sound stand-in: the cycles the original's own threads spend inside chosen
// functions of the exe, one function's own time apart from the counted functions it calls (docs/ORIGRUN.md, "Counting cycles
// inside the original's functions"). The port counts the same boundaries in its game loop (platform/CycleSplit), so a port
// and its original are compared part by part and not only in total (docs/TEST_COMMANDS.md, "The cycle split").
//
// origrun names a list of functions (ORIGRUN_COUNT: a label, the function's address and its first instructions' bytes, from
// tools/origrun/prologue.py) and the name of a block of shared memory (ORIGRUN_COUNT_MAP, count.h). As the stand-in loads,
// before the game's first instruction (origrun makes the copy's exe import it, as for --coverage), the first instructions of
// each listed function are copied into a stub and replaced by a jump to the stub's entry:
//
//   function:  jmp  stub                          (5 bytes; the rest of the copied instructions become NOPs)
//   stub:      push index / jmp count_enter_common
//   tramp:     the copied instructions / jmp function+length
//
// count_enter_common saves flags, eax, ecx and edx, calls count_enter(index, the slot that holds the return address) and
// jumps to the tramp with the stack as it was at the function's entry. count_enter pushes a frame on the thread's shadow
// stack, puts count_exit_thunk in the return address's place and takes the cycle reading last. When the function returns it
// lands in count_exit_thunk, which takes the reading first, adds the function's inclusive cycles to its parent frame's
// callee time and its own (inclusive less callee time) to the function's total, and returns to the real caller. Every
// register the function returns a value in (eax, edx, the x87 stack) is left as it was.
//
// A thread cycle reading (QueryThreadCycleTime, a system call) costs about 700 cycles; one is made at each entry and each
// exit, half of it falling into the caller's own time. The cost is measured at the start and written to the block.
// A function that is left by an exception or a longjmp never returns to the thunk: its frame is dropped at the next entry or
// exit (the block counts these as misses) and its time stays in its parent.
//
// Not counted: the first instructions of a function must be whole instructions of 5 bytes or more with no relative branch
// or call (prologue.py picks them), nothing may jump into them from elsewhere, and the exe must be at its preferred base.
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "count.h"

// The handlers run inside the game, between its instructions, so they must not touch the vector registers (the compiler uses them to
// copy a 64-bit number): no SSE in this file.
#pragma clang attribute push(__attribute__((target("no-sse"))), apply_to = function)

#define MAXDEPTH 8

typedef struct { DWORD idx; DWORD slot; DWORD ret; uint64_t start, child; } Frame;
typedef struct { CountThread* sh; int depth; uint64_t tsc; Frame fr[MAXDEPTH]; } Priv;   // tsc: the ticks of the entry readings of frames still open

static CountShared* gSh;
static HANDLE gMap;
static DWORD gTls = TLS_OUT_OF_INDEXES;
static volatile LONG gNextThread;
DWORD count_tramp[COUNT_MAXFUNC];      // where each function's copied instructions are, for count_enter_common
extern void count_enter_common(void);
extern void count_exit_thunk(void);

__asm__(
".text\n"
".globl _count_enter_common\n"
"_count_enter_common:\n"                    // [esp] = the function's index, [esp+4] = its return address
"  pushfl\n"
"  pushl %eax\n"
"  pushl %ecx\n"
"  pushl %edx\n"                            // edx ecx eax flags index return
"  leal 20(%esp), %eax\n"
"  movl 16(%esp), %ecx\n"
"  pushl %eax\n"                            // the slot that holds the return address
"  pushl %ecx\n"                            // the index
"  call _count_enter\n"
"  addl $8, %esp\n"
"  movl 16(%esp), %eax\n"
"  movl _count_tramp(,%eax,4), %eax\n"
"  movl %eax, 16(%esp)\n"                   // the index's place now holds the address to go on at
"  popl %edx\n"
"  popl %ecx\n"
"  popl %eax\n"
"  popfl\n"
"  ret\n"                                   // to the copied instructions, with the stack as it was at the function's entry
".globl _count_exit_thunk\n"
"_count_exit_thunk:\n"                      // the function has returned: esp is where the caller's cleanup left it
"  pushl $0\n"                              // the real return address goes here
"  pushfl\n"
"  pushl %eax\n"
"  pushl %ecx\n"
"  pushl %edx\n"
"  leal 20(%esp), %eax\n"                   // esp as the thunk was entered
"  leal 16(%esp), %ecx\n"                   // the placeholder
"  pushl %ecx\n"
"  pushl %eax\n"
"  call _count_exit\n"
"  addl $8, %esp\n"
"  popl %edx\n"
"  popl %ecx\n"
"  popl %eax\n"
"  popfl\n"
"  ret\n"                                   // to the real caller
);

static inline uint64_t now_cycles(void)
{
	ULONG64 c;
	QueryThreadCycleTime((HANDLE)-2, &c);
	return c;
}

// The thread's TLS slot, read from the TEB (TlsGetValue would clear the last error the game may be about to read).
static inline void* tls_get(DWORD i)
{
	void* v;
	__asm__ volatile("movl %%fs:(%1), %0" : "=r"(v) : "r"(0xE10 + 4 * i));
	return v;
}

static Priv* new_thread(void)
{
	LONG k = __atomic_fetch_add(&gNextThread, 1, __ATOMIC_SEQ_CST);
	Priv* p = NULL;
	if (k < COUNT_MAXTHREAD) {
		p = (Priv*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Priv));
		if (p) {
			p->sh = &gSh->thread[k];
			p->sh->tid = GetCurrentThreadId();
			__atomic_fetch_add(&gSh->nthread, 1, __ATOMIC_SEQ_CST);
		}
	}
	DWORD le = GetLastError();
	TlsSetValue(gTls, p ? p : (Priv*)1);       // a thread that cannot be counted is marked, not tried at every call
	SetLastError(le);
	return p;
}

void __attribute__((used)) count_enter(DWORD idx, DWORD* slot)
{
	Priv* p = (Priv*)tls_get(gTls);
	if (!p) p = new_thread();
	if (!p || p == (Priv*)1 || gSh->paused) return;
	while (p->depth && p->fr[p->depth - 1].slot < (DWORD)slot) {   // a frame deeper than this call is one that was left without a return
		p->depth--;
		__atomic_fetch_add(&gSh->misses, 1, __ATOMIC_SEQ_CST);
	}
	if (p->depth == MAXDEPTH) { __atomic_fetch_add(&gSh->misses, 1, __ATOMIC_SEQ_CST); return; }
	Frame* f = &p->fr[p->depth++];
	f->idx = idx;
	f->slot = (DWORD)slot;
	f->ret = *slot;
	f->child = 0;
	*slot = (DWORD)count_exit_thunk;
	uint64_t t0 = __builtin_ia32_rdtsc();      // the reading is timed by the time-stamp counter: in the running game it costs several times a run of them in a row
	f->start = now_cycles();                    // last: what comes after is the function's own time
	p->tsc += __builtin_ia32_rdtsc() - t0;
}

void __attribute__((used)) count_exit(DWORD espAtThunk, DWORD* placeholder)
{
	uint64_t t0 = __builtin_ia32_rdtsc();
	uint64_t now = now_cycles();                // first
	uint64_t spent = __builtin_ia32_rdtsc() - t0;
	Priv* p = (Priv*)tls_get(gTls);
	if (!p || p == (Priv*)1 || p->depth <= 0) { *placeholder = 0; return; }   // cannot happen: the thunk is reached only through a frame
	// The frame that returned is the one nearest the bottom of the shadow stack whose slot lies below the stack pointer:
	// frames above it were left without a return, frames below it are its callers, whose slots lie above.
	int i = p->depth - 1;
	while (i > 0 && p->fr[i - 1].slot + 4 <= espAtThunk) i--;
	if (p->depth - 1 > i) __atomic_fetch_add(&gSh->misses, (uint32_t)(p->depth - 1 - i), __ATOMIC_SEQ_CST);
	Frame* f = &p->fr[i];
	p->depth = i;
	uint64_t incl = now - f->start;
	uint64_t excl = incl > f->child ? incl - f->child : 0;
	if (i > 0) p->fr[i - 1].child += incl;
	CountThread* t = p->sh;
	t->seq++;
	t->calls[f->idx]++;
	t->cycles[f->idx] += excl;
	t->readTsc += p->tsc + spent;
	p->tsc = 0;
	t->seq++;
	*placeholder = f->ret;
}

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

static int in_code(BYTE* base, IMAGE_NT_HEADERS32* nt, DWORD a, DWORD len)
{
	IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
	DWORD rva = a - (DWORD)(ULONG_PTR)base;
	for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
		DWORD size = s[i].Misc.VirtualSize ? s[i].Misc.VirtualSize : s[i].SizeOfRawData;
		if (rva >= s[i].VirtualAddress && rva + len <= s[i].VirtualAddress + size)
			return (s[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
	}
	return 0;
}

static void fail(const char* msg, const char* detail)
{
	wsprintfA(gSh->error, "%s%s%s", msg, detail && *detail ? ": " : "", detail ? detail : "");
	gSh->nfunc = 0;
}

static int hexval(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

void count_install(void)
{
	char list[MAX_PATH], mapName[64];
	if (!GetEnvironmentVariableA("ORIGRUN_COUNT", list, MAX_PATH)) return;
	if (!GetEnvironmentVariableA("ORIGRUN_COUNT_MAP", mapName, sizeof(mapName))) return;
	gMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(CountShared), mapName);
	if (!gMap) return;
	gSh = (CountShared*)MapViewOfFile(gMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CountShared));
	if (!gSh) return;
	memset(gSh, 0, sizeof(*gSh));
	gSh->magic = COUNT_MAGIC;

	gTls = TlsAlloc();
	if (gTls == TLS_OUT_OF_INDEXES || gTls >= 64) { fail("no TLS slot below 64 for the counters", ""); return; }

	BYTE* base = (BYTE*)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
	DWORD preferred = file_image_base();
	if (preferred && (DWORD)(ULONG_PTR)base != preferred) { fail("the exe was moved from its preferred base (ASLR): the listed addresses do not hold", ""); return; }

	HANDLE f = CreateFileA(list, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	DWORD size = f != INVALID_HANDLE_VALUE ? GetFileSize(f, NULL) : 0;
	char* text = size && size != INVALID_FILE_SIZE ? (char*)HeapAlloc(GetProcessHeap(), 0, size + 1) : NULL;
	DWORD got = 0;
	if (text && !ReadFile(f, text, size, &got, NULL)) got = 0;
	if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
	if (!text || !got) { fail("cannot read the function list", list); return; }
	text[got] = 0;

	BYTE* stubs = (BYTE*)VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!stubs) { fail("no memory for the stubs", ""); return; }
	BYTE* put = stubs;
	DWORD cost;
	{
		uint64_t c0 = now_cycles();
		for (int i = 0; i < 2000; i++) now_cycles();
		cost = (DWORD)((now_cycles() - c0) / 2001);
	}
	gSh->readCost = cost;

	int n = 0;
	for (char* p = text; *p; ) {
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
		if (!*p) break;
		char* eol = p;
		while (*eol && *eol != '\n') eol++;
		char saved = *eol;
		*eol = 0;
		if (*p != '#') {
			char label[COUNT_NAMELEN];
			int l = 0;
			while (*p && *p != ' ' && *p != '\t') { if (l < COUNT_NAMELEN - 1) label[l++] = *p; p++; }
			label[l] = 0;
			char* end;
			DWORD at = strtoul(p, &end, 16);
			BYTE want[32];
			int len = 0;
			if (end == p) { fail("a line with no address", label); return; }
			p = end;
			for (;;) {
				while (*p == ' ' || *p == '\t') p++;
				if (hexval(p[0]) < 0 || hexval(p[1]) < 0) break;
				if (len < (int)sizeof(want)) want[len] = (BYTE)(hexval(p[0]) * 16 + hexval(p[1]));
				len++;
				p += 2;
			}
			if (len < 5 || len > (int)sizeof(want)) { fail("the function's first instructions are not 5 to 32 bytes", label); return; }
			if (n >= COUNT_MAXFUNC) { fail("more functions than the counters hold", label); return; }
			if (!in_code(base, nt, at, len)) { fail("the address is not in the exe's code", label); return; }
			if (memcmp((void*)(ULONG_PTR)at, want, len)) { fail("the exe's bytes at the address differ from the list's (a different build of the game, or a function already patched)", label); return; }
			// the stub: push index, jump to the common entry; then the copied instructions and the jump back
			BYTE* stub = put;
			*put++ = 0x68; *(DWORD*)put = (DWORD)n; put += 4;
			*put++ = 0xE9; *(DWORD*)put = (DWORD)((BYTE*)count_enter_common - (put + 4)); put += 4;
			count_tramp[n] = (DWORD)(ULONG_PTR)put;
			memcpy(put, want, len); put += len;
			*put++ = 0xE9; *(DWORD*)put = (DWORD)((BYTE*)(ULONG_PTR)(at + len) - (put + 4)); put += 4;
			BYTE* fn = (BYTE*)(ULONG_PTR)at;
			DWORD old;
			if (!VirtualProtect(fn, len, PAGE_EXECUTE_READWRITE, &old)) { fail("cannot write the function", label); return; }
			fn[0] = 0xE9; *(DWORD*)(fn + 1) = (DWORD)(stub - (fn + 5));
			for (int k = 5; k < len; k++) fn[k] = 0x90;
			VirtualProtect(fn, len, old, &old);
			FlushInstructionCache(GetCurrentProcess(), fn, len);
			lstrcpynA(gSh->name[n], label, COUNT_NAMELEN);
			gSh->addr[n] = at;
			n++;
		}
		*eol = saved;
		p = eol;
	}
	FlushInstructionCache(GetCurrentProcess(), stubs, (SIZE_T)(put - stubs));
	HeapFree(GetProcessHeap(), 0, text);
	gSh->nfunc = n;
	if (!n) fail("the function list is empty", list);
}

void count_detach(void)
{
}

#pragma clang attribute pop
