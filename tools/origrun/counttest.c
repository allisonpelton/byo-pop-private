// counttest: a small 32-bit program for check.sh's origrun suite (tools/origrun/counttest.sh): origrun --count must count the
// cycles of its functions, one function's own time apart from the counted functions it calls, and leave what each one takes and
// returns as it was. The functions are written in assembly so that their first instructions are known (55 89 E5 53 56:
// push ebp; mov ebp, esp; push ebx; push esi, five bytes) and each takes a different calling convention:
//   ct_leaf   int __stdcall (int x): burns about 2 million cycles, returns x + 1, pops its argument (ret 4)
//   ct_outer  unsigned long long __cdecl (int a, int b): burns 1 million, calls ct_leaf(a) twice and ct_leaf(b), returns the sum of
//             their results in eax and 0x12345678 in edx
//   ct_this   int thiscall (this in ecx): returns this + 7, so an ecx the stub lost shows
//   ct_rec    int __cdecl (int n): burns half a million, calls itself with n - 1 until 0, returns n
//   ct_f      double (): returns 2.0 in the x87 stack
// It runs them once and checks their results, then, when the stand-in's counters are on (ORIGRUN_COUNT_MAP), that they counted the
// calls exactly and that each function's own cycles are what it burned, not what its callees did. It then keeps a window open for
// about two seconds so that the host can take its own snapshot ("count" in the script). No sound: it is not a game.
//
//   counttest --list <file>     the function list for --count (addresses at the exe's preferred base: it is built without ASLR)
//   counttest [<result file>]   runs and writes ok, FAIL and PASSED or FAILED lines to the file (COUNTTEST_RESULT names it too)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "count.h"

extern int __stdcall ct_leaf(int x) __asm__("_ct_leaf");     // the label as the assembly below spells it, without stdcall's @4
extern unsigned long long __cdecl ct_outer(int a, int b);
extern int __attribute__((thiscall)) ct_this(void* self);
extern int __cdecl ct_rec(int n);
extern double ct_f(void);

__asm__(
".text\n"
".globl _ct_leaf\n"
"_ct_leaf:\n"
"  pushl %ebp\n  movl %esp, %ebp\n  pushl %ebx\n  pushl %esi\n"
"  movl $2000000, %ecx\n"
"1: decl %ecx\n  jnz 1b\n"
"  movl 8(%ebp), %eax\n  incl %eax\n"
"  popl %esi\n  popl %ebx\n  popl %ebp\n  ret $4\n"
".globl _ct_outer\n"
"_ct_outer:\n"
"  pushl %ebp\n  movl %esp, %ebp\n  pushl %ebx\n  pushl %esi\n"
"  movl $1000000, %ecx\n"
"1: decl %ecx\n  jnz 1b\n"
"  pushl 8(%ebp)\n  call _ct_leaf\n  movl %eax, %ebx\n"
"  pushl 8(%ebp)\n  call _ct_leaf\n  addl %eax, %ebx\n"
"  pushl 12(%ebp)\n  call _ct_leaf\n  addl %eax, %ebx\n"
"  movl %ebx, %eax\n  movl $0x12345678, %edx\n"
"  popl %esi\n  popl %ebx\n  popl %ebp\n  ret\n"
".globl _ct_this\n"
"_ct_this:\n"
"  pushl %ebp\n  movl %esp, %ebp\n  pushl %ebx\n  pushl %esi\n"
"  movl %ecx, %eax\n  addl $7, %eax\n"
"  popl %esi\n  popl %ebx\n  popl %ebp\n  ret\n"
".globl _ct_rec\n"
"_ct_rec:\n"
"  pushl %ebp\n  movl %esp, %ebp\n  pushl %ebx\n  pushl %esi\n"
"  movl $500000, %ecx\n"
"1: decl %ecx\n  jnz 1b\n"
"  movl 8(%ebp), %eax\n  testl %eax, %eax\n  jz 2f\n"
"  decl %eax\n  pushl %eax\n  call _ct_rec\n  addl $4, %esp\n  incl %eax\n"
"2: popl %esi\n  popl %ebx\n  popl %ebp\n  ret\n"
".globl _ct_f\n"
"_ct_f:\n"
"  pushl %ebp\n  movl %esp, %ebp\n  pushl %ebx\n  pushl %esi\n"
"  fld1\n  fadd %st(0), %st\n"
"  popl %esi\n  popl %ebx\n  popl %ebp\n  ret\n"
);

static FILE* gOut;
static int gBad;

static void check(int ok, const char* what, double got, double want)
{
	if (ok) { fprintf(gOut, "ok: %s\n", what); return; }
	gBad++;
	fprintf(gOut, "FAIL: %s (%g, expected %g)\n", what, got, want);
}

static const volatile CountShared* block(void)
{
	char name[64];
	if (!GetEnvironmentVariableA("ORIGRUN_COUNT_MAP", name, sizeof(name))) return NULL;
	HANDLE m = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
	return m ? (const volatile CountShared*)MapViewOfFile(m, FILE_MAP_READ, 0, 0, sizeof(CountShared)) : NULL;
}

static void totals(const volatile CountShared* sh, const char* label, unsigned long long* calls, unsigned long long* cycles)
{
	*calls = *cycles = 0;
	for (unsigned k = 0; k < sh->nfunc; k++) {
		if (strcmp((const char*)sh->name[k], label)) continue;
		for (int t = 0; t < COUNT_MAXTHREAD; t++) { *calls += sh->thread[t].calls[k]; *cycles += sh->thread[t].cycles[k]; }
	}
}

int main(int argc, char** argv)
{
	if (argc > 2 && strcmp(argv[1], "--list") == 0) {
		FILE* f = fopen(argv[2], "w");
		if (!f) return 2;
		fprintf(f, "# counttest's functions: push ebp; mov ebp, esp; push ebx; push esi (five bytes) in each\n");
		fprintf(f, "ct_leaf 0x%08lx len=5\nct_outer 0x%08lx len=5\nct_this 0x%08lx len=5\nct_rec 0x%08lx len=5\nct_f 0x%08lx len=5\n",
			(unsigned long)(void*)ct_leaf, (unsigned long)(void*)ct_outer, (unsigned long)(void*)ct_this, (unsigned long)(void*)ct_rec, (unsigned long)(void*)ct_f);
		fclose(f);
		return 0;
	}
	const char* resultFile = getenv("COUNTTEST_RESULT");
	if (!resultFile && argc > 1) resultFile = argv[1];
	if (!resultFile) return 2;
	gOut = fopen(resultFile, "w");
	if (!gOut) return 2;
	const volatile CountShared* sh = block();

	unsigned long long r = ct_outer(5, 9);
	check((unsigned)r == 22 && (unsigned)(r >> 32) == 0x12345678u, "ct_outer's 64-bit result (eax and edx) is as returned", (double)(unsigned)r, 22);
	int leaf = ct_leaf(41), self = ct_this((void*)100), rec = ct_rec(3);
	double two = ct_f();
	check(leaf == 42, "ct_leaf's stdcall result and stack (ret 4)", leaf, 42);
	check(self == 107, "ct_this finds its ecx", self, 107);
	check(rec == 3, "ct_rec's recursion returns through every level", rec, 3);
	check(two == 2.0, "ct_f's x87 result is untouched", two, 2.0);
	if (!sh) fprintf(gOut, "note: no counters (ORIGRUN_COUNT_MAP is not set): results only\n");
	else if (sh->error[0]) { gBad++; fprintf(gOut, "FAIL: the stand-in could not count: %s\n", (const char*)sh->error); }
	else {
		unsigned long long calls, cycles;
		// the calls made above: ct_outer 1; ct_leaf 3 (in ct_outer) + 1; ct_this 1; ct_rec 4 (n = 3 down to 0); ct_f 1
		// the loops burn 2, 1 and half a million iterations; a cycle count of one is whatever a core does with them (about half a cycle
		// an iteration), so the checks are on the ratios: a function that included its callees would be seven times its own loop
		unsigned long long outerCalls, outerOwn, leafCalls, leafCycles, recCalls, recCycles;
		totals(sh, "ct_outer", &outerCalls, &outerOwn);
		totals(sh, "ct_leaf", &leafCalls, &leafCycles);
		totals(sh, "ct_rec", &recCalls, &recCycles);
		double leafEach = (double)leafCycles / (leafCalls ? leafCalls : 1), recEach = (double)recCycles / (recCalls ? recCalls : 1);
		check(outerCalls == 1, "ct_outer counted 1 call", (double)outerCalls, 1);
		check(leafCalls == 4, "ct_leaf counted 4 calls", (double)leafCalls, 4);
		check(recCalls == 4, "ct_rec counted 4 calls (n = 3 down to 0)", (double)recCalls, 4);
		check(outerOwn > 200000, "ct_outer's own cycles are counted", (double)outerOwn, 1e6);
		check(leafEach > 1.4 * outerOwn && leafEach < 2.8 * outerOwn, "ct_outer's own cycles are its loop's, half a ct_leaf call's, not its callees' too", (double)outerOwn, leafEach / 2);
		check(outerOwn > 1.4 * recEach && outerOwn < 2.8 * recEach, "ct_rec's cycles a call are its own loop's (half of ct_outer's), not the levels below", recEach, outerOwn / 2.0);
		totals(sh, "ct_this", &calls, &cycles);
		check(calls == 1, "ct_this counted 1 call", (double)calls, 1);
		totals(sh, "ct_f", &calls, &cycles);
		check(calls == 1, "ct_f counted 1 call", (double)calls, 1);
		check(sh->misses == 0, "no frame was dropped", (double)sh->misses, 0);
	}
	fprintf(gOut, gBad ? "FAILED\n" : "PASSED\n");
	fclose(gOut);

	// stay up for the host's snapshot: a window for it to find, closed by the script's quit or after 4 seconds
	HWND w = CreateWindowExA(0, "STATIC", "counttest", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 200, 100, NULL, NULL, GetModuleHandleA(NULL), NULL);
	DWORD t0 = GetTickCount();
	MSG msg;
	while (IsWindow(w) && GetTickCount() - t0 < 4000) {
		while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
		Sleep(10);
	}
	return gBad ? 1 : 0;
}
