// origrun's virtual clock, inside the game (--clock virtual; docs/ORIGRUN.md, "The virtual clock"). Built into the
// dsound stand-in, which origrun places beside the copy, so it runs in the game's process from the moment the game
// loads dsound.dll. The user allowed this hook on 2026-11-02 ("Yes, hook them"): it replaces the time functions the
// copy imports, and nothing else.
//
// What it does, when ORIGRUN_CLOCK=virtual:
//   - It patches the import tables of the game's exe and of the other DLLs in the copy's folder (not origrun's own
//     stand-ins and wrappers, not Steam.dll), in memory: timeGetTime, GetTickCount(64), QueryPerformanceCounter and
//     -Frequency, GetSystemTime(AsFileTime), GetLocalTime and their like read a virtual clock, and Sleep(Ex) and
//     MsgWaitForMultipleObjects(Ex) on no handles with a timeout return at once and move the clock on by the timeout,
//     when the game's main thread calls them and no message (or APC) is ready. The clock starts at the real time of the
//     install and moves only that way, so the game runs as fast as it can draw, update for update as in real time.
//   - The main thread's timed waits on handles (WaitForSingleObject(Ex), WaitForMultipleObjects(Ex), a
//     MsgWaitForMultipleObjects(Ex) with handles) go to Windows for real, since another thread may signal what they
//     wait for at any moment, and move the clock by the real time they took.
//   - A main thread that reads the clock more than kPollFree times without waiting (a busy loop) moves it on by
//     kPollStep per read, so a loop that polls the clock still gets out.
//   - Other threads' waits stay on the wall clock; their reads see the virtual clock. While a thread the exe started
//     (a loader) works, the main thread's waits take their real time, still moving the clock by their timeouts (the
//     guard, below), so the main thread does not run ahead of what its loader has done.
//   - origrun holds the main thread at a wait through the shared block (vclock.h): at an update, at a virtual time, or
//     at the next wait. It also reads the update counter and the --watch values here, at each wait and at each clock
//     read once the counter has moved, so none is missed however fast the game runs.
// The stand-in pins itself (a FreeLibrary of dsound.dll would leave the patched entries pointing nowhere). It assumes
// one main thread drives the game: the one that started at the exe's entry point.
// The same import patcher serves the scripted cursor (--cursor scripted, ORIGRUN_CURSOR=scripted; cursor.c) and the
// message-box hook (on unless --msgbox show, ORIGRUN_MSGBOX=hook; msgbox.c), with the clock or without it: it points the
// copy's GetCursorPos imports at cursor.c, and its MessageBox and MessageBeep imports at msgbox.c. Each hook belongs to
// one of the three, and only the ones that are on are patched. Without the clock, DLLs the copy loads later are patched
// from the other hooks' own calls (hooks_rescan).
// It also serves the registry map (--reg-map popcap, ORIGRUN_REGMAP=popcap; regmap.c), off unless a run asks for it:
// the copy's RegCreateKeyExA, RegOpenKeyExA, RegSetValueExA and RegQueryValueExA imports point at regmap.c.
// Written from the public Win32 and PE documentation.

#define PSAPI_VERSION 2   // K32EnumProcessModules from kernel32, no psapi.lib
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <intrin.h>
#include "vclock.h"

#define kPollFree 200          // clock reads the main thread may make between two waits before reads move the clock
#define kPollStep 100          // how far each further read moves it: 10 microseconds
#define kFirstHoldMs 5000      // how long a hold waits for origrun to connect before the game runs free (until it does)
#define kHoldCapMs 60000       // the longest a hold lasts once origrun follows the clock; then holds are off for good
#define kNoWaitMs 5000         // a main thread that has not waited this long after the install is logged

static int        gOn;              // the virtual clock is on
static int        gCursorOn;        // the scripted cursor is on (cursor.c)
static int        gKeysOn;          // the scripted keys are on (cursor.c)
static int        gLastInputOn;     // the last-input hook is on (H_GetLastInputInfo)
static LONG       gLastInputReads;  // this process's GetLastInputInfo calls answered
static int        gMsgboxOn;        // the message-box hook is on (msgbox.c)
static int        gRegMapOn;        // the registry map is on (regmap.c)
static VcShared*  gSh;
static HANDLE     gMap, gHeldEv, gRelEv, gHostProc;
static DWORD      gDriver;          // the main thread: its waits drive the clock
static int        gInHold;          // the driver is inside a hold (a sent message it dispatches there must not hold again)
static LONG       gReads;           // the driver's clock reads since its last wait
static ULONGLONG  gInstallTick;     // GetTickCount64 at the install
static int        gDriverWaited;    // the driver has called a wait
static LONG       gNoWaitLogged;
static LONGLONG   gQpf, gQpc0, gFt0, gLft0;
static DWORD      gTgt0, gGtc0;
static ULONGLONG  gGtc64_0;
static char       gCounterExpr[256];
static char       gWatchExpr[4][256];
static int        gWatchN;
static int        gSeen[4], gLastOk[4];
static DWORD      gLastVal[4];
static FILE*      gClog;
static char       gExeDir[MAX_PATH];
static HMODULE    gSelf;
static HMODULE    gPatched[64];
static int        gPatchedN;
static ULONGLONG  gModScanAt;       // when the module list was last looked at, and its count and checksum then
static DWORD      gModCnt, gModSum;

static void vclog(const char* fmt, va_list ap)
{
	if (!gClog) return;
	SYSTEMTIME st; GetLocalTime(&st);   // our own import: the wall clock
	fprintf(gClog, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	vfprintf(gClog, fmt, ap); fputc('\n', gClog); fflush(gClog);
}
static void clog(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vclog(fmt, ap); va_end(ap); }
void hooks_log(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vclog(fmt, ap); va_end(ap); }   // msgbox.c's

static LONGLONG real100(void) { FILETIME f; GetSystemTimeAsFileTime(&f); return ((LONGLONG)f.dwHighDateTime << 32) | f.dwLowDateTime; }
static LONGLONG qpc100(void)   // the real time for durations, in 100 ns (our own import: the real counter)
{
	LARGE_INTEGER c; QueryPerformanceCounter(&c);
	return (c.QuadPart / gQpf) * 10000000LL + ((c.QuadPart % gQpf) * 10000000LL) / gQpf;
}
static LONGLONG vget(void) { return InterlockedCompareExchange64((LONGLONG volatile*)&gSh->v, 0, 0); }
static void vadd(LONGLONG d)   // only the driver thread moves the clock; the exchange keeps other threads' reads whole
{
	LONGLONG o; do { o = gSh->v; } while (InterlockedCompareExchange64((LONGLONG volatile*)&gSh->v, o + d, o) != o);
}

// The update counter and --watch values: address expressions as origrun's ("0x48b124", "[0x579e30]+0x4c0", nested
// brackets), read from this process with ReadProcessMemory, which fails instead of faulting on a bad address.
static int self_u32(DWORD a, DWORD* out) { SIZE_T g = 0; return ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(DWORD_PTR)a, out, 4, &g) && g == 4; }
static int eval_addr(const char** ps, DWORD* out)
{
	const char* s = *ps; while (*s == ' ') s++;
	DWORD v;
	if (*s == '[') {
		s++; DWORD at; if (!eval_addr(&s, &at)) return 0;
		while (*s == ' ') s++; if (*s != ']') return 0; s++;
		if (!self_u32(at, &v) || !v) return 0;
	} else {
		char* e; v = (DWORD)strtoul(s, &e, 16); if (e == s) return 0; s = e;
	}
	for (;;) {
		while (*s == ' ') s++;
		if (*s != '+' && *s != '-') break;
		int neg = *s == '-'; s++; while (*s == ' ') s++;
		char* e; DWORD o = (DWORD)strtoul(s, &e, 16); if (e == s) return 0; s = e;
		v = neg ? v - o : v + o;
	}
	*ps = s; *out = v; return 1;
}
static int read_expr(const char* e, DWORD* out) { if (!e[0]) return 0; const char* s = e; DWORD a; return eval_addr(&s, &a) && self_u32(a, out); }

static void ring_push(LONG upd, int idx, DWORD val, int ok)
{
	LONG h = gSh->ringHead;
	if (h - gSh->ringTail >= VC_RING) { InterlockedIncrement(&gSh->ringDropped); return; }
	VcWatchRec* r = &gSh->ring[h % VC_RING];
	r->update = upd; r->idx = idx; r->value = (LONG)val; r->ok = ok;
	MemoryBarrier();
	InterlockedExchange(&gSh->ringHead, h + 1);
}

// Read the counter; read the watches when asked to (each wait) or when the counter moved, and hand over each change.
static void vc_sample(int force)
{
	DWORD c = 0; int ok = gCounterExpr[0] && read_expr(gCounterExpr, &c);
	LONG cv = ok ? (LONG)c : -1;
	int moved = ok != gSh->counterOk || cv != gSh->counter;
	gSh->counter = cv; gSh->counterOk = ok;
	if (!gWatchN || !(force || moved)) return;
	for (int i = 0; i < gWatchN; i++) {
		DWORD v = 0; int wok = read_expr(gWatchExpr[i], &v);
		if (!gSeen[i] || wok != gLastOk[i] || (wok && v != gLastVal[i])) {
			ring_push(cv, i, v, wok);
			gSeen[i] = 1; gLastOk[i] = wok; gLastVal[i] = v;
		}
	}
}

// Holds go off for the rest of the run, and the block says so (origrun logs it: a script's later at and wait land
// wherever the game is).
static void barrier_off(int why, const char* what)
{
	VcShared* s = gSh;
	s->barrierOffCounter = s->counterOk ? s->counter : -1;
	s->barrierOffWhy = why;
	MemoryBarrier();
	InterlockedExchange(&s->barrierOff, 1);
	InterlockedExchange(&s->bar, VC_BAR_FREE);
	clog("%s: no more holds in this run (update counter %ld); the clock stays virtual", what, (long)s->barrierOffCounter);
}

// Hold the driver thread while the barrier it claimed stays HELD. Sent messages are answered meanwhile (a PrintWindow
// from origrun's capture, a SendMessage from one of the game's own threads); posted ones wait for the game's own
// message loop. The release event only wakes the thread: it goes on when origrun has moved bar off HELD.
static void vc_hold(int where, DWORD timeout)
{
	VcShared* s = gSh;
	s->heldCounter = s->counterOk ? s->counter : -1;
	s->heldWhere = where; s->heldV = vget(); s->heldTimeout = (LONG)timeout;
	s->held = 1;
	InterlockedIncrement(&s->heldSeq); InterlockedIncrement(&s->holds);
	SetEvent(gHeldEv);
	gInHold = 1;
	LONGLONG r0 = qpc100();
	ULONGLONG t0 = GetTickCount64();
	while (s->bar == VC_BAR_HELD) {
		DWORD cap = s->connected ? kHoldCapMs : kFirstHoldMs;
		ULONGLONG el = GetTickCount64() - t0;
		if (el >= cap) {
			if (s->connected) barrier_off(VC_OFF_CAP, "a hold lasted 60 s with no release from origrun");
			else {   // origrun has not come (yet): run free, and hold again once it arms a barrier
				InterlockedCompareExchange(&s->bar, VC_BAR_FREE, VC_BAR_HELD);
				clog("origrun did not follow the clock within %d s: the game runs free until it does", kFirstHoldMs / 1000);
			}
			break;
		}
		HANDLE hs[2] = { gRelEv, gHostProc }; DWORD n = gHostProc ? 2 : 1;
		DWORD r = MsgWaitForMultipleObjects(n, hs, FALSE, (DWORD)(cap - el), QS_SENDMESSAGE);
		if (n == 2 && r == WAIT_OBJECT_0 + 1) { barrier_off(VC_OFF_HOST, "origrun ended while the game was held"); break; }
		if (r == WAIT_OBJECT_0 + n) { MSG m; PeekMessageA(&m, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE); continue; }
		if (r == WAIT_FAILED) { clog("a hold's wait failed (err %lu)", GetLastError()); barrier_off(VC_OFF_FAILED, "a hold's wait failed"); break; }
	}
	gInHold = 0;
	s->held = 0;
	InterlockedExchange64((LONGLONG volatile*)&s->heldReal100, s->heldReal100 + (qpc100() - r0));
}

// At a wait of the driver (before the clock moves): does an armed barrier hold it here? wouldBe is the clock after
// the wait. The game claims the barrier (ARMED to HELD) before it holds, so origrun can take back one not claimed yet.
static void vc_barrier(int where, LONGLONG wouldBe, DWORD timeout)
{
	VcShared* s = gSh;
	if (s->bar != VC_BAR_ARMED || gInHold || s->barrierOff) return;
	LONG k = s->holdKind;
	int hit = k == VC_HOLD_NEXT
		|| (k == VC_HOLD_UPDATE && s->counterOk && (LONG)(s->counter - s->holdUpdate) >= 0)
		|| (k == VC_HOLD_TIME && wouldBe >= s->holdV);
	if (hit && InterlockedCompareExchange(&s->bar, VC_BAR_HELD, VC_BAR_ARMED) == VC_BAR_ARMED) vc_hold(where, timeout);
}

// A thread other than the driver calls the clock: say so once if the driver has not waited since the install (it may
// not be the game's main thread, and then the clock does not move).
static void no_wait_check(void)
{
	if (gDriverWaited || gNoWaitLogged || GetTickCount64() - gInstallTick < kNoWaitMs) return;
	if (InterlockedExchange(&gNoWaitLogged, 1)) return;
	clog("the main thread %lu has not waited in the %d s since the install, while thread %lu calls the time functions: "
		"if %lu is not the thread that runs the game, the clock does not move and origrun's holds never come",
		gDriver, kNoWaitMs / 1000, GetCurrentThreadId(), gDriver);
}

static int on_driver(void) { return gOn && GetCurrentThreadId() == gDriver && !gInHold; }

void hooks_rescan(void);

// A clock read. On the driver, past kPollFree reads with no wait the read moves the clock (a busy loop), and then it is
// also where a barrier can hold the game.
static void vc_read(void)
{
	if (!on_driver()) { if (gOn && GetCurrentThreadId() != gDriver) no_wait_check(); return; }
	// The watches are read here too once the counter has moved, so a game that runs several updates between two waits
	// has each update's changes logged at that update. Only then: a change the game's input handling makes between
	// update N and N+1 is logged at N+1 (at the read or wait after N+1 began), which is what origrun's 1 ms poll of a
	// real-time run sees when the handler is quick (Bejeweled's swaps); a slow handler (Bejeweled's Play, which builds
	// the board) is caught by that poll before N+1 and logged at N there.
	if (gWatchN) vc_sample(0);
	if (++gReads <= kPollFree) return;
	if (gReads == kPollFree + 1 && gSh->polls == 0) clog("the main thread read the clock %d times without waiting: from here its reads move the clock on (a busy loop)", kPollFree);
	vadd(kPollStep);
	InterlockedIncrement(&gSh->polls);
	vc_sample(0);
	vc_barrier(VC_AT_POLL, vget(), 0);
}

// The driver enters a timed wait that is not ready at once: sample, then a barrier may hold it here.
static void vc_wait_enter(int where, DWORD ms)
{
	if (gReads > gSh->maxReads) gSh->maxReads = gReads;
	gReads = 0;
	InterlockedExchange64((LONGLONG volatile*)&gSh->nowReal100, real100());
	hooks_rescan();   // a DLL of the copy loaded since
	vc_sample(1);
	vc_barrier(where, vget() + (LONGLONG)ms * VC_UNITS_PER_MS, ms);
}
// Where the driver's timed waits come from (the return address into the game), for the log at exit: the ones the
// clock skipped, and the waits on handles that went to Windows (real).
typedef struct { void* at; int real; const char* fn; LONG n, timeouts; LONGLONG t100; LONG c0, c1; } WaitSite;
static WaitSite gSites[32];
static int gSiteN;
static void site_note(void* at, int real, const char* fn, LONGLONG t100, int timedOut)
{
	int i = 0; while (i < gSiteN && (gSites[i].at != at || gSites[i].real != real)) i++;
	if (i == gSiteN && gSiteN < 32) { gSites[i].at = at; gSites[i].real = real; gSites[i].fn = fn; gSites[i].c0 = gSh->counter; gSiteN++; }
	if (i < gSiteN) { gSites[i].n++; gSites[i].t100 += t100; gSites[i].timeouts += timedOut; gSites[i].c1 = gSh->counter; }
}
static void vc_wait_skip(DWORD ms, void* at, const char* fn)
{
	vadd((LONGLONG)ms * VC_UNITS_PER_MS); InterlockedIncrement(&gSh->waits);
	site_note(at, 0, fn, (LONGLONG)ms * VC_UNITS_PER_MS, 1);
}

// ------------------------------------------------------------------------------------------------ the clock reads

static DWORD WINAPI H_timeGetTime(void) { vc_read(); return gTgt0 + (DWORD)(vget() / VC_UNITS_PER_MS); }
static DWORD WINAPI H_GetTickCount(void) { vc_read(); return gGtc0 + (DWORD)(vget() / VC_UNITS_PER_MS); }
static ULONGLONG WINAPI H_GetTickCount64(void) { vc_read(); return gGtc64_0 + (ULONGLONG)(vget() / VC_UNITS_PER_MS); }
static BOOL WINAPI H_QueryPerformanceCounter(LARGE_INTEGER* p)
{
	if (!p) return FALSE;
	vc_read();
	LONGLONG v = vget();
	p->QuadPart = gQpc0 + (v / 10000000LL) * gQpf + ((v % 10000000LL) * gQpf) / 10000000LL;
	return TRUE;
}
static BOOL WINAPI H_QueryPerformanceFrequency(LARGE_INTEGER* p) { if (!p) return FALSE; p->QuadPart = gQpf; return TRUE; }
static void ft_put(LONGLONG t, FILETIME* f) { f->dwLowDateTime = (DWORD)t; f->dwHighDateTime = (DWORD)(t >> 32); }
static VOID WINAPI H_GetSystemTimeAsFileTime(LPFILETIME f) { vc_read(); if (f) ft_put(gFt0 + vget(), f); }
static VOID WINAPI H_GetSystemTimePreciseAsFileTime(LPFILETIME f) { vc_read(); if (f) ft_put(gFt0 + vget(), f); }
static VOID WINAPI H_GetSystemTime(LPSYSTEMTIME st) { vc_read(); FILETIME f; ft_put(gFt0 + vget(), &f); if (st) FileTimeToSystemTime(&f, st); }
static VOID WINAPI H_GetLocalTime(LPSYSTEMTIME st) { vc_read(); FILETIME f; ft_put(gLft0 + vget(), &f); if (st) FileTimeToSystemTime(&f, st); }
static MMRESULT WINAPI H_timeGetSystemTime(LPMMTIME t, UINT cb)
{
	if (!t || cb < sizeof(MMTIME)) return TIMERR_STRUCT;
	t->wType = TIME_MS; t->u.ms = H_timeGetTime(); return TIMERR_NOERROR;
}

// ------------------------------------------------------------------------------------------------- the waits
// The driver's wait with a timeout (not 0, not INFINITE) first asks Windows with a zero timeout; if what it waits for is
// ready, that is the answer and the clock stays. Otherwise it may be held (vc_wait_enter) and asks again (origrun may
// have posted input while it was held). Then a Sleep(Ex), or a MsgWaitForMultipleObjects(Ex) on no handles, returns the
// timeout at once with the clock moved on by it. A wait on handles goes to Windows for real and moves the clock by the
// time it took: another thread (a loader, a library's, the CRT's) may signal the handle any moment, and a timeout
// returned at once would change what the game does next. Other threads, and waits with no timeout, wait on the wall
// clock as before.
//
// The one exception is the guard: while another thread the game's exe started (a loader) is working, the driver's
// skipped waits take their real time, and the clock still moves by each wait's timeout. A main thread that runs ahead
// of its loader would otherwise reach update N long before the loader has finished what it finishes by update N in a
// real-time run (Bejeweled's title screen still loading at the update its Play button is clicked). Threads of the
// libraries (BASS, FMOD, DirectX) and of Windows are not watched. ORIGRUN_CLOCK_GUARD=0 turns the guard off.

#define MAXOT 64
typedef LONG (NTAPI* NtQIT_t)(HANDLE, int, PVOID, ULONG, PULONG);
static NtQIT_t pNtQIT;
static BYTE *gExeLo, *gExeHi;
static struct { DWORD tid; HANDLE h; ULONG64 cyc; } gOt[MAXOT];
static int gOtN;
static ULONGLONG gOtScanAt;
static unsigned long long gWinTsc, gWinOther;
static int gBusy, gGuardOff;
static LONG gBusyFlips;

// Look for threads the game's exe started (their start address is in the exe), and drop the ones that ended.
static void ot_scan(void)
{
	for (int i = 0; i < gOtN; ) {
		DWORD code = 0;
		if (!GetExitCodeThread(gOt[i].h, &code) || code != STILL_ACTIVE) { clog("thread %lu ended (update counter %ld)", gOt[i].tid, (long)gSh->counter); CloseHandle(gOt[i].h); gOt[i] = gOt[--gOtN]; continue; }
		i++;
	}
	HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (sn == INVALID_HANDLE_VALUE) return;
	DWORD pid = GetCurrentProcessId();
	THREADENTRY32 te; te.dwSize = sizeof(te);
	for (BOOL ok = Thread32First(sn, &te); ok; te.dwSize = sizeof(te), ok = Thread32Next(sn, &te)) {
		if (te.th32OwnerProcessID != pid || te.th32ThreadID == gDriver || gOtN >= MAXOT) continue;
		int known = 0;
		for (int i = 0; i < gOtN; i++) if (gOt[i].tid == te.th32ThreadID) { known = 1; break; }
		if (known) continue;
		HANDLE h = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
		if (!h) continue;
		void* start = NULL;
		if (!pNtQIT || pNtQIT(h, 9 /* ThreadQuerySetWin32StartAddress */, &start, sizeof(start), NULL) != 0
			|| (BYTE*)start < gExeLo || (BYTE*)start >= gExeHi) { CloseHandle(h); continue; }
		ULONG64 c = 0; QueryThreadCycleTime(h, &c);
		gOt[gOtN].tid = te.th32ThreadID; gOt[gOtN].h = h; gOt[gOtN].cyc = c; gOtN++;
		clog("thread %lu (started by the exe at %p) is watched: while it works, the main thread's waits take their real time", te.th32ThreadID, start);
	}
	CloseHandle(sn);
}

// Did the exe's other threads use more than a quarter of a core over the last window (about 10 ms)?
static int others_busy(void)
{
	if (gGuardOff) return 0;
	ULONGLONG now = GetTickCount64();
	if (now - gOtScanAt >= 200) { gOtScanAt = now; ot_scan(); }
	ULONG64 other = 0;
	for (int i = 0; i < gOtN; i++) { ULONG64 c = 0; if (QueryThreadCycleTime(gOt[i].h, &c)) { other += c - gOt[i].cyc; gOt[i].cyc = c; } }
	unsigned long long t = __rdtsc();
	gWinOther += other;
	if (t - gWinTsc >= (1ull << 25)) {
		int b = gWinOther * 4 > (t - gWinTsc);
		if (b != gBusy && ++gBusyFlips <= 40)
			clog("the exe's other threads %s (update counter %ld, game time %.2f s)", b ? "work: the main thread's waits take their real time"
				: "rest: its waits are skipped again", (long)gSh->counter, (double)vget() / 1e7);
		gBusy = b; gWinTsc = t; gWinOther = 0;
	}
	return gBusy;
}

typedef struct { int kind; HANDLE h; DWORD n; const HANDLE* hs; BOOL all, alertable; DWORD mask, flags; } WaitCall;
enum { W_SLEEP, W_SLEEPEX, W_WFSO, W_WFSOEX, W_WFMO, W_WFMOEX, W_MWFMO, W_MWFMOEX };
static DWORD do_wait(const WaitCall* c, DWORD ms)   // the real call
{
	switch (c->kind) {
	case W_SLEEP:   Sleep(ms); return 0;
	case W_SLEEPEX: return SleepEx(ms, c->alertable);
	case W_WFSO:    return WaitForSingleObject(c->h, ms);
	case W_WFSOEX:  return WaitForSingleObjectEx(c->h, ms, c->alertable);
	case W_WFMO:    return WaitForMultipleObjects(c->n, c->hs, c->all, ms);
	case W_WFMOEX:  return WaitForMultipleObjectsEx(c->n, c->hs, c->all, ms, c->alertable);
	case W_MWFMO:   return MsgWaitForMultipleObjects(c->n, c->hs, c->all, ms, c->mask);
	default:        return MsgWaitForMultipleObjectsEx(c->n, c->hs, ms, c->mask, c->flags);
	}
}
// Is what the wait waits for ready now? (A Sleep never is; an alertable SleepEx is when an APC ran.)
static int ready_now(const WaitCall* c, DWORD* r)
{
	if (c->kind == W_SLEEP || (c->kind == W_SLEEPEX && !c->alertable)) return 0;
	if (c->kind == W_SLEEPEX) { *r = SleepEx(0, TRUE); return *r == WAIT_IO_COMPLETION; }
	*r = do_wait(c, 0);
	return *r != WAIT_TIMEOUT;
}
static const char* const kWaitName[] = { "Sleep", "SleepEx", "WaitForSingleObject", "WaitForSingleObjectEx",
	"WaitForMultipleObjects", "WaitForMultipleObjectsEx", "MsgWaitForMultipleObjects", "MsgWaitForMultipleObjectsEx" };
static DWORD vwait(const WaitCall* c, DWORD ms, int where, void* at)
{
	int multi = c->kind == W_WFMO || c->kind == W_WFMOEX;
	int drv = on_driver();
	if (drv) gDriverWaited = 1;
	else if (gOn && GetCurrentThreadId() != gDriver) no_wait_check();
	if (!drv || ms == 0 || ms == INFINITE || (multi && c->n == 0)) {
		if (gOn && ms && ms != INFINITE && GetCurrentThreadId() != gDriver) InterlockedIncrement(&gSh->otherWaits);
		return do_wait(c, ms);
	}
	DWORD r;
	if (ready_now(c, &r)) { InterlockedIncrement(&gSh->passWaits); return r; }
	vc_wait_enter(where, ms);
	if (ready_now(c, &r)) return r;   // signalled meanwhile, or input origrun posted while the game was held
	int skip = c->kind == W_SLEEP || c->kind == W_SLEEPEX || ((c->kind == W_MWFMO || c->kind == W_MWFMOEX) && c->n == 0);
	if (!skip || others_busy()) {   // a wait on handles, or the guard: Windows waits, the clock moves by the time it took
		LONGLONG t0 = qpc100();
		r = do_wait(c, ms);
		LONGLONG el = qpc100() - t0, full = (LONGLONG)ms * VC_UNITS_PER_MS;
		int timedOut = c->kind == W_SLEEP || (c->kind == W_SLEEPEX && r == 0) || r == WAIT_TIMEOUT;
		LONGLONG adv = timedOut ? full : (el < full ? el : full);
		vadd(adv);
		if (skip) {
			InterlockedIncrement(&gSh->realWaits);
			InterlockedExchange64((LONGLONG volatile*)&gSh->realWait100, gSh->realWait100 + adv);
		} else {
			InterlockedIncrement(&gSh->handleWaits);
			if (timedOut) InterlockedIncrement(&gSh->handleTimeouts);
			InterlockedExchange64((LONGLONG volatile*)&gSh->handleWait100, gSh->handleWait100 + adv);
			site_note(at, 1, kWaitName[c->kind], adv, timedOut);
		}
		return r;
	}
	vc_wait_skip(ms, at, kWaitName[c->kind]);
	return c->kind == W_SLEEP || c->kind == W_SLEEPEX ? 0 : WAIT_TIMEOUT;
}

#define RA __builtin_return_address(0)
static VOID WINAPI H_Sleep(DWORD ms) { WaitCall c = { W_SLEEP }; vwait(&c, ms, VC_AT_SLEEP, RA); }
static DWORD WINAPI H_SleepEx(DWORD ms, BOOL alertable) { WaitCall c = { W_SLEEPEX }; c.alertable = alertable; return vwait(&c, ms, VC_AT_SLEEP, RA); }
static DWORD WINAPI H_WaitForSingleObject(HANDLE h, DWORD ms) { WaitCall c = { W_WFSO }; c.h = h; return vwait(&c, ms, VC_AT_WAIT, RA); }
static DWORD WINAPI H_WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable)
{ WaitCall c = { W_WFSOEX }; c.h = h; c.alertable = alertable; return vwait(&c, ms, VC_AT_WAIT, RA); }
static DWORD WINAPI H_WaitForMultipleObjects(DWORD n, const HANDLE* hs, BOOL all, DWORD ms)
{ WaitCall c = { W_WFMO }; c.n = n; c.hs = hs; c.all = all; return vwait(&c, ms, VC_AT_WAIT, RA); }
static DWORD WINAPI H_WaitForMultipleObjectsEx(DWORD n, const HANDLE* hs, BOOL all, DWORD ms, BOOL alertable)
{ WaitCall c = { W_WFMOEX }; c.n = n; c.hs = hs; c.all = all; c.alertable = alertable; return vwait(&c, ms, VC_AT_WAIT, RA); }
static DWORD WINAPI H_MsgWaitForMultipleObjects(DWORD n, const HANDLE* hs, BOOL all, DWORD ms, DWORD mask)
{ WaitCall c = { W_MWFMO }; c.n = n; c.hs = hs; c.all = all; c.mask = mask; return vwait(&c, ms, VC_AT_MSGWAIT, RA); }
static DWORD WINAPI H_MsgWaitForMultipleObjectsEx(DWORD n, const HANDLE* hs, DWORD ms, DWORD mask, DWORD flags)
{ WaitCall c = { W_MWFMOEX }; c.n = n; c.hs = hs; c.mask = mask; c.flags = flags; return vwait(&c, ms, VC_AT_MSGWAIT, RA); }

// GetLastInputInfo: the copy's last input is now, on its own clock (the virtual one when that is on). A SexyAppFramework
// original asks it once a second, with the screensaver's timeout, and draws nothing while the machine has had no
// keyboard or mouse input for that long, rundesk's private desktop or not (Alchemy, 2026-11-09: 0x462a60 sets the flag
// DrawDirtyStuff tests); on the virtual clock its time runs ahead of Windows', so a long run stopped drawing even while
// the user typed. The user allowed this hook on 2026-11-09 ("Yes, hook it"). It reads the clock without moving it (no
// vc_read: it is not a poll of the time). The framework finds the function with GetProcAddress (it is not in Windows 95's
// user32), so an import patch never reaches it: lastinput_detour puts a jump to this one at the start of user32's own
// GetLastInputInfo, in the copy's process only, and this one never calls Windows'.
static BOOL WINAPI H_GetLastInputInfo(PLASTINPUTINFO p)
{
	hooks_rescan();
	if (!p || p->cbSize != sizeof(LASTINPUTINFO)) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
	p->dwTime = gOn ? gGtc0 + (DWORD)(vget() / VC_UNITS_PER_MS) : GetTickCount();
	InterlockedIncrement(&gLastInputReads);
	return TRUE;
}
// The jump: five bytes at the start of user32!GetLastInputInfo, written once at install. Every caller in the process then
// reaches H_GetLastInputInfo, the ones that found the function with GetProcAddress too.
static int lastinput_detour(void)
{
	HMODULE u = GetModuleHandleA("user32.dll");
	BYTE* f = u ? (BYTE*)GetProcAddress(u, "GetLastInputInfo") : NULL;
	DWORD old;
	if (!f || !VirtualProtect(f, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
	LONG rel = (LONG)((BYTE*)(void*)H_GetLastInputInfo - (f + 5));
	f[0] = 0xE9; memcpy(f + 1, &rel, 4);
	VirtualProtect(f, 5, old, &old);
	FlushInstructionCache(GetCurrentProcess(), f, 5);
	return 1;
}

// ---------------------------------------------------------------------------------------------- import patching

BOOL WINAPI cursor_GetCursorPos(LPPOINT p);   // cursor.c
int  cursor_open(void);
void cursor_installed(void);
void cursor_stats(LONG* reads, LONG* real);
SHORT WINAPI keys_GetKeyState(int vk);   // cursor.c
void keys_installed(void);
void keys_stats(LONG* reads, LONG* real);
int WINAPI mbx_MessageBoxA(HWND, LPCSTR, LPCSTR, UINT);   // msgbox.c
int WINAPI mbx_MessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT);
int WINAPI mbx_MessageBoxExA(HWND, LPCSTR, LPCSTR, UINT, WORD);
int WINAPI mbx_MessageBoxExW(HWND, LPCWSTR, LPCWSTR, UINT, WORD);
int WINAPI mbx_MessageBoxIndirectA(const MSGBOXPARAMSA*);
int WINAPI mbx_MessageBoxIndirectW(const MSGBOXPARAMSW*);
int WINAPI mbx_MessageBoxTimeoutA(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);
int WINAPI mbx_MessageBoxTimeoutW(HWND, LPCWSTR, LPCWSTR, UINT, WORD, DWORD);
BOOL WINAPI mbx_MessageBeep(UINT);
int  msgbox_open(void);
void msgbox_installed(void);
void msgbox_patched(void);
void msgbox_stats(LONG* boxes, LONG* beeps);
LSTATUS WINAPI reg_RegCreateKeyExA(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, const LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);   // regmap.c
LSTATUS WINAPI reg_RegOpenKeyExA(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
LSTATUS WINAPI reg_RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD);
LSTATUS WINAPI reg_RegQueryValueExA(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
void regmap_stats(LONG* creates, LONG* opens, LONG* misses, LONG* writes, LONG* reads, LONG* found);

// Each hook belongs to the virtual clock (H_CLOCK), the scripted cursor (H_CURSOR), the message-box hook (H_MSGBOX), the
// scripted keys (H_KEYS), the last-input hook (H_LASTINPUT) or the registry map (H_REGMAP); only the ones that are on are
// patched.
enum { H_CLOCK, H_CURSOR, H_MSGBOX, H_KEYS, H_LASTINPUT, H_REGMAP, H_GROUPS };
static const char* const kGrpName[] = { "the virtual clock", "the scripted cursor", "the message-box hook", "the scripted keys", "the last-input hook", "the registry map" };
typedef struct { const char* dll; const char* name; void* hook; int grp; void* real; LONG patched; } HookDef;
static HookDef gHooks[] = {
	{ "winmm.dll",    "timeGetTime",                    (void*)H_timeGetTime,                    H_CLOCK },
	{ "winmm.dll",    "timeGetSystemTime",              (void*)H_timeGetSystemTime,              H_CLOCK },
	{ "kernel32.dll", "GetTickCount",                   (void*)H_GetTickCount,                   H_CLOCK },
	{ "kernel32.dll", "GetTickCount64",                 (void*)H_GetTickCount64,                 H_CLOCK },
	{ "kernel32.dll", "QueryPerformanceCounter",        (void*)H_QueryPerformanceCounter,        H_CLOCK },
	{ "kernel32.dll", "QueryPerformanceFrequency",      (void*)H_QueryPerformanceFrequency,      H_CLOCK },
	{ "kernel32.dll", "GetSystemTimeAsFileTime",        (void*)H_GetSystemTimeAsFileTime,        H_CLOCK },
	{ "kernel32.dll", "GetSystemTimePreciseAsFileTime", (void*)H_GetSystemTimePreciseAsFileTime, H_CLOCK },
	{ "kernel32.dll", "GetSystemTime",                  (void*)H_GetSystemTime,                  H_CLOCK },
	{ "kernel32.dll", "GetLocalTime",                   (void*)H_GetLocalTime,                   H_CLOCK },
	{ "kernel32.dll", "Sleep",                          (void*)H_Sleep,                          H_CLOCK },
	{ "kernel32.dll", "SleepEx",                        (void*)H_SleepEx,                        H_CLOCK },
	{ "kernel32.dll", "WaitForSingleObject",            (void*)H_WaitForSingleObject,            H_CLOCK },
	{ "kernel32.dll", "WaitForSingleObjectEx",          (void*)H_WaitForSingleObjectEx,          H_CLOCK },
	{ "kernel32.dll", "WaitForMultipleObjects",         (void*)H_WaitForMultipleObjects,         H_CLOCK },
	{ "kernel32.dll", "WaitForMultipleObjectsEx",       (void*)H_WaitForMultipleObjectsEx,       H_CLOCK },
	{ "user32.dll",   "MsgWaitForMultipleObjects",      (void*)H_MsgWaitForMultipleObjects,      H_CLOCK },
	{ "user32.dll",   "MsgWaitForMultipleObjectsEx",    (void*)H_MsgWaitForMultipleObjectsEx,    H_CLOCK },
	{ "user32.dll",   "GetCursorPos",                   (void*)cursor_GetCursorPos,              H_CURSOR },
	{ "user32.dll",   "GetKeyState",                    (void*)keys_GetKeyState,                 H_KEYS },
	{ "user32.dll",   "GetLastInputInfo",               (void*)H_GetLastInputInfo,               H_LASTINPUT },
	{ "user32.dll",   "MessageBoxA",                    (void*)mbx_MessageBoxA,                  H_MSGBOX },
	{ "user32.dll",   "MessageBoxW",                    (void*)mbx_MessageBoxW,                  H_MSGBOX },
	{ "user32.dll",   "MessageBoxExA",                  (void*)mbx_MessageBoxExA,                H_MSGBOX },
	{ "user32.dll",   "MessageBoxExW",                  (void*)mbx_MessageBoxExW,                H_MSGBOX },
	{ "user32.dll",   "MessageBoxIndirectA",            (void*)mbx_MessageBoxIndirectA,          H_MSGBOX },
	{ "user32.dll",   "MessageBoxIndirectW",            (void*)mbx_MessageBoxIndirectW,          H_MSGBOX },
	{ "user32.dll",   "MessageBoxTimeoutA",             (void*)mbx_MessageBoxTimeoutA,           H_MSGBOX },
	{ "user32.dll",   "MessageBoxTimeoutW",             (void*)mbx_MessageBoxTimeoutW,           H_MSGBOX },
	{ "user32.dll",   "MessageBeep",                    (void*)mbx_MessageBeep,                  H_MSGBOX },
	{ "advapi32.dll", "RegCreateKeyExA",                (void*)reg_RegCreateKeyExA,              H_REGMAP },
	{ "advapi32.dll", "RegOpenKeyExA",                  (void*)reg_RegOpenKeyExA,                H_REGMAP },
	{ "advapi32.dll", "RegSetValueExA",                 (void*)reg_RegSetValueExA,               H_REGMAP },
	{ "advapi32.dll", "RegQueryValueExA",               (void*)reg_RegQueryValueExA,             H_REGMAP },
};
#define NHOOK ((int)(sizeof(gHooks) / sizeof(gHooks[0])))
static int hook_on(int k) { int g = gHooks[k].grp; return g == H_CLOCK ? gOn : g == H_CURSOR ? gCursorOn : g == H_KEYS ? gKeysOn : g == H_LASTINPUT ? gLastInputOn : g == H_REGMAP ? gRegMapOn : gMsgboxOn; }

// The functions a game may import that the clock cannot stand in for; logged when a module imports them.
static const char* const kUncovered[] = { "timeSetEvent", "SetTimer", "CreateWaitableTimerA", "CreateWaitableTimerW",
	"SetWaitableTimer", "CreateTimerQueueTimer", "SignalObjectAndWait", "GetMessageTime", "timeGetDevCaps", NULL };

// Patch one module's import address table: an entry whose address is one of the hooked functions, or that names one
// (by its import name table), is pointed at the stand-in. The table's page is made writable for the write only, and
// keeps its execute right if it had one (a packed exe may keep code on the same page). Delay-loaded imports and
// imports through api-ms-win-* sets are not looked at.
static int patch_module(HMODULE m, const char* leaf)
{
	BYTE* base = (BYTE*)m;
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
	IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*)(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return 0;
	IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (!dd.VirtualAddress || !dd.Size) return 0;
	int n = 0;
	char uncovered[256] = "";
	for (IMAGE_IMPORT_DESCRIPTOR* d = (IMAGE_IMPORT_DESCRIPTOR*)(base + dd.VirtualAddress); d->Name; d++) {
		const char* dll = (const char*)(base + d->Name);
		DWORD* iat = (DWORD*)(base + d->FirstThunk);
		DWORD* names = d->OriginalFirstThunk ? (DWORD*)(base + d->OriginalFirstThunk) : NULL;
		for (int i = 0; iat[i]; i++) {
			const char* nm = NULL;
			if (names && names[i] && !(names[i] & IMAGE_ORDINAL_FLAG32)) nm = (const char*)((IMAGE_IMPORT_BY_NAME*)(base + names[i]))->Name;
			int h = -1;
			for (int k = 0; k < NHOOK && h < 0; k++) if (hook_on(k) && gHooks[k].real && (void*)(DWORD_PTR)iat[i] == gHooks[k].real) h = k;
			for (int k = 0; k < NHOOK && h < 0 && nm; k++) if (hook_on(k) && !lstrcmpiA(dll, gHooks[k].dll) && !strcmp(nm, gHooks[k].name)) h = k;
			if (h < 0) {
				for (int u = 0; gOn && nm && kUncovered[u]; u++)
					if (!strcmp(nm, kUncovered[u]) && strlen(uncovered) + strlen(nm) + 2 < sizeof(uncovered)) { if (uncovered[0]) strcat(uncovered, " "); strcat(uncovered, nm); }
				continue;
			}
			if ((void*)(DWORD_PTR)iat[i] == gHooks[h].hook) continue;
			if ((void*)(DWORD_PTR)iat[i] != gHooks[h].real)
				clog("  %s: %s!%s holds %08lX, not the function's address %p (patched anyway)", leaf, dll, gHooks[h].name, (unsigned long)iat[i], gHooks[h].real);
			DWORD old, prot = PAGE_READWRITE;
			MEMORY_BASIC_INFORMATION mi;
			if (VirtualQuery(&iat[i], &mi, sizeof(mi)) && (mi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
				prot = PAGE_EXECUTE_READWRITE;
			if (!VirtualProtect(&iat[i], 4, prot, &old)) { clog("  %s: %s!%s could not be made writable (err %lu)", leaf, dll, gHooks[h].name, GetLastError()); continue; }
			iat[i] = (DWORD)(DWORD_PTR)gHooks[h].hook;
			VirtualProtect(&iat[i], 4, old, &old);
			InterlockedIncrement(&gHooks[h].patched);
			if (gHooks[h].grp == H_MSGBOX) msgbox_patched();
			n++;
			clog("  %s: %s!%s -> %s", leaf, dll, gHooks[h].name, kGrpName[gHooks[h].grp]);
		}
	}
	if (uncovered[0]) clog("  %s also imports what the clock does not cover: %s", leaf, uncovered);
	return n;
}

// The copy's own modules: the exe, and every DLL loaded from its folder but origrun's own stand-ins and wrappers and
// Steam.dll (the client's, not the game's).
static int module_is_games(HMODULE m, char* leaf, int cap)
{
	char path[MAX_PATH];
	if (m == gSelf || !GetModuleFileNameA(m, path, MAX_PATH)) return 0;
	char* l = strrchr(path, '\\'); if (!l) return 0;
	lstrcpynA(leaf, l + 1, cap);
	if (m == GetModuleHandleA(NULL)) return 1;
	*l = 0;
	size_t dl = strlen(gExeDir);
	if (_strnicmp(path, gExeDir, dl) != 0 || (path[dl] != 0 && path[dl] != '\\')) return 0;
	static const char* const skip[] = { "dsound.dll", "ddraw.dll", "d3dimm.dll", "d3d8.dll", "d3d9.dll", "dxgi.dll", "d3d11.dll",
		"opengl32.dll", "steam.dll", "dgvoodoocpl.exe", NULL };
	for (int i = 0; skip[i]; i++) if (!lstrcmpiA(leaf, skip[i])) return 0;
	return 1;
}
static void patch_new_modules(void)
{
	HMODULE mods[512]; DWORD need = 0;
	if (!K32EnumProcessModules(GetCurrentProcess(), mods, sizeof(mods), &need)) return;
	int cnt = (int)(need / sizeof(HMODULE)); if (cnt > 512) cnt = 512;
	DWORD sum = 0;
	for (int i = 0; i < cnt; i++) sum = (sum << 5 | sum >> 27) ^ (DWORD)(DWORD_PTR)mods[i];
	if ((DWORD)cnt == gModCnt && sum == gModSum) return;   // the same modules as last time
	gModCnt = (DWORD)cnt; gModSum = sum;
	for (int i = 0; i < cnt; i++) {
		int seen = 0;
		for (int j = 0; j < gPatchedN; j++) if (gPatched[j] == mods[i]) { seen = 1; break; }
		char leaf[MAX_PATH];
		if (seen || !module_is_games(mods[i], leaf, MAX_PATH)) continue;
		if (gPatchedN < 64) gPatched[gPatchedN++] = mods[i];
		int n = patch_module(mods[i], leaf);
		char grps[160] = ""; int on[H_GROUPS] = { gOn, gCursorOn, gMsgboxOn, gKeysOn, gLastInputOn, gRegMapOn };
		for (int g = 0; g < H_GROUPS; g++) if (on[g]) { if (grps[0]) lstrcatA(grps, ", "); lstrcatA(grps, kGrpName[g]); }
		clog("%s: %d import%s on %s", leaf, n, n == 1 ? "" : "s", grps);
	}
}
// A DLL of the copy loaded since the last look: the module list is looked at from the clock's waits and the cursor's,
// message-box and registry hooks' calls, whichever thread they come from (one at a time), at most once per tick of the
// wall clock (about 16 ms), and only after the loader has said a DLL was mapped since the last look. Looking at the list
// costs a few hundred thousand cycles, which a copy that asks for the cursor every 10 ms paid on each call: Typer
// Shark's cursor thread, about 0.7 G cycles in 20 s, plus 0.35 G on its main thread (2026-11-12). Without the loader's
// notice (it could not be registered) the list is looked at once per tick as before.
static LONG gScanBusy;   // a scan of the module list is running (the install's own, or a rescan)
static volatile LONG gDllsMapped = 1;   // DLLs the loader has mapped since the last look (1: look once)
static int gDllNotice;                  // the loader's notice is registered
typedef VOID (CALLBACK *LdrNotify_t)(ULONG reason, const void* data, PVOID ctx);
typedef LONG (NTAPI *LdrRegister_t)(ULONG flags, LdrNotify_t fn, PVOID ctx, PVOID* cookie);
// The loader's own notice of a DLL that has been mapped (Vista and later; net.c registers one of its own): the loader
// lock is held, so it only counts, and the next rescan does the work.
static VOID CALLBACK dll_mapped(ULONG reason, const void* data, PVOID ctx)
{
	(void)data; (void)ctx;
	if (reason == 1 /* LDR_DLL_NOTIFICATION_REASON_LOADED */) InterlockedIncrement(&gDllsMapped);
}
static void dll_notice_install(void)
{
	HMODULE nt = GetModuleHandleA("ntdll.dll");
	LdrRegister_t reg = nt ? (LdrRegister_t)(void*)GetProcAddress(nt, "LdrRegisterDllNotification") : NULL;
	PVOID cookie = NULL;
	gDllNotice = reg && reg(0, dll_mapped, NULL, &cookie) == 0;
	if (!gDllNotice) clog("the loader's DLL notice could not be registered: the module list is looked at every 15 ms");
}
void net_rescan(void);   // net.c: the network block's own look at the DLLs mapped since
void hooks_rescan(void)
{
	if (gDllNotice && gDllsMapped == 0) return;
	ULONGLONG t = GetTickCount64();
	if (t - gModScanAt < 15 || InterlockedExchange(&gScanBusy, 1)) return;
	gModScanAt = t;
	if (gDllNotice) InterlockedExchange(&gDllsMapped, 0);   // a DLL mapped from here on is counted again for the next look
	patch_new_modules();
	net_rescan();
	InterlockedExchange(&gScanBusy, 0);
}

// The process's first thread, which runs the game's main loop: the one that started at the exe's entry point (asked
// with NtQueryInformationThread), or, when none did, the one created earliest (logged: creation times are coarse).
static DWORD main_thread(void)
{
	DWORD pid = GetCurrentProcessId(), best = 0, atEntry = 0; ULONGLONG bestT = ~0ULL;
	IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*)(gExeLo + ((IMAGE_DOS_HEADER*)gExeLo)->e_lfanew);
	void* entry = gExeLo + nt->OptionalHeader.AddressOfEntryPoint;
	HANDLE sn = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (sn == INVALID_HANDLE_VALUE) { clog("no thread list (err %lu): the thread that loaded the stand-in drives the clock", GetLastError()); return GetCurrentThreadId(); }
	THREADENTRY32 te; te.dwSize = sizeof(te);
	if (Thread32First(sn, &te)) do {
		if (te.th32OwnerProcessID != pid) continue;
		HANDLE h = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
		void* start = NULL;
		if (h && !atEntry && pNtQIT && pNtQIT(h, 9 /* ThreadQuerySetWin32StartAddress */, &start, sizeof(start), NULL) == 0 && start == entry)
			atEntry = te.th32ThreadID;
		FILETIME c, e, k, u;
		if (h && GetThreadTimes(h, &c, &e, &k, &u)) {
			ULONGLONG t = ((ULONGLONG)c.dwHighDateTime << 32) | c.dwLowDateTime;
			if (t < bestT) { bestT = t; best = te.th32ThreadID; }
		}
		if (h) CloseHandle(h);
		te.dwSize = sizeof(te);
	} while (Thread32Next(sn, &te));
	CloseHandle(sn);
	if (atEntry) return atEntry;
	clog("no thread started at the exe's entry point %p: the main thread is taken to be the one created earliest (%lu)", entry, best ? best : GetCurrentThreadId());
	return best ? best : GetCurrentThreadId();
}

// ---------------------------------------------------------------------------------------------------- the API

// The virtual clock in ms since it was installed, for the sound stand-in's log and play cursor; 0 when it is off.
int vclock_ms(double* out)
{
	if (!gOn) return 0;
	*out = (double)vget() / (double)VC_UNITS_PER_MS;
	return 1;
}

// The clock's half of the install (ORIGRUN_CLOCK=virtual): its block and events, its bases, the main thread and the
// guard. 1 when the clock can run; its hooks are patched once the caller sets gOn.
static int clock_install(const char* exe)
{
	char id[64] = "";
	GetEnvironmentVariableA("ORIGRUN_CLOCK_ID", id, sizeof(id));
	DWORD pid = GetCurrentProcessId();
	char name[160];
	wsprintfA(name, "Local\\origrun-vclock-%s-%lu", id, pid);
	gMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(VcShared), name);
	gSh = gMap ? (VcShared*)MapViewOfFile(gMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(VcShared)) : NULL;
	if (!gSh) { clog("pid %lu: no shared block %s (err %lu): no virtual clock", pid, name, GetLastError()); return 0; }
	char ev[192];
	wsprintfA(ev, "%s-held", name); gHeldEv = CreateEventA(NULL, FALSE, FALSE, ev);
	wsprintfA(ev, "%s-release", name); gRelEv = CreateEventA(NULL, FALSE, FALSE, ev);
	if (!gHeldEv || !gRelEv) { clog("pid %lu: no events for %s (err %lu): no virtual clock", pid, name, GetLastError()); return 0; }
	char host[32] = "";
	if (GetEnvironmentVariableA("ORIGRUN_CLOCK_HOST", host, sizeof(host))) gHostProc = OpenProcess(SYNCHRONIZE, FALSE, strtoul(host, NULL, 10));
	GetEnvironmentVariableA("ORIGRUN_CLOCK_COUNTER", gCounterExpr, sizeof(gCounterExpr));
	char w[1100] = "";
	if (GetEnvironmentVariableA("ORIGRUN_CLOCK_WATCH", w, sizeof(w)) && w[0]) {
		for (char* t = strtok(w, ";"); t && gWatchN < 4; t = strtok(NULL, ";")) lstrcpynA(gWatchExpr[gWatchN++], t, 256);
	}

	// The clock's bases: the real values now, so the game sees no jump.
	LARGE_INTEGER q; QueryPerformanceFrequency(&q); gQpf = q.QuadPart; QueryPerformanceCounter(&q); gQpc0 = q.QuadPart;
	gTgt0 = timeGetTime(); gGtc0 = GetTickCount(); gGtc64_0 = GetTickCount64();
	gFt0 = real100();
	{ SYSTEMTIME lt; FILETIME lf; GetLocalTime(&lt); SystemTimeToFileTime(&lt, &lf); gLft0 = ((LONGLONG)lf.dwHighDateTime << 32) | lf.dwLowDateTime; }

	VcShared* s = gSh;
	s->pid = pid;
	s->counter = -1; s->counterOk = 0;
	s->installReal100 = gFt0; s->nowReal100 = gFt0;
	gInstallTick = GetTickCount64();
	// the exe's image range (a thread starting there is the game's own, for the guard; the main thread starts at its
	// entry point) and how to ask a thread's start
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS32* nt = (IMAGE_NT_HEADERS32*)((BYTE*)dos + dos->e_lfanew);
	gExeLo = (BYTE*)dos; gExeHi = gExeLo + nt->OptionalHeader.SizeOfImage;
	pNtQIT = (NtQIT_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
	gDriver = main_thread();
	s->driverTid = gDriver;
	// The first wait holds the game until origrun follows the clock (or kFirstHoldMs passes), so a script's first
	// command finds the game where it started, however fast it runs.
	char hs[8] = ""; GetEnvironmentVariableA("ORIGRUN_CLOCK_HOLDSTART", hs, sizeof(hs));
	if (hs[0] == '1') { s->holdKind = VC_HOLD_NEXT; s->bar = VC_BAR_ARMED; }

	clog("pid %lu %s: virtual clock installed (thread %lu loaded the stand-in; the main thread %lu drives the clock); "
		"counter %s; %d watch%s; host %s; first hold %s", pid, exe, GetCurrentThreadId(), gDriver,
		gCounterExpr[0] ? gCounterExpr : "(none)", gWatchN, gWatchN == 1 ? "" : "es",
		gHostProc ? "watched" : "not watchable", s->bar == VC_BAR_ARMED ? "armed" : "off");
	{   // the guard
		char g[8] = ""; GetEnvironmentVariableA("ORIGRUN_CLOCK_GUARD", g, sizeof(g));
		gGuardOff = g[0] == '0';
		gWinTsc = __rdtsc(); gOtScanAt = GetTickCount64();
		ot_scan();
		if (gGuardOff) clog("the guard is off (ORIGRUN_CLOCK_GUARD=0): the main thread's waits are skipped even while the exe's other threads work");
	}
	return 1;
}

// Called from the stand-in's DllMain on process attach: the virtual clock (ORIGRUN_CLOCK=virtual), the scripted cursor
// (ORIGRUN_CURSOR=scripted), the scripted keys (ORIGRUN_KEYS=scripted), the message-box hook (ORIGRUN_MSGBOX=hook) and the
// last-input hook (ORIGRUN_LASTINPUT=now) and the registry map (ORIGRUN_REGMAP=popcap), any of them. Nothing here loads
// a DLL that is not loaded already (winmm, user32 and advapi32 are the stand-in's own imports), so it is safe under the
// loader lock.
void vclock_install(HMODULE self)
{
	char mode[32] = "", cur[32] = "", mbx[32] = "", kys[32] = "", lin[32] = "", rgm[32] = "";
	GetEnvironmentVariableA("ORIGRUN_CLOCK", mode, sizeof(mode));
	GetEnvironmentVariableA("ORIGRUN_CURSOR", cur, sizeof(cur));
	GetEnvironmentVariableA("ORIGRUN_MSGBOX", mbx, sizeof(mbx));
	GetEnvironmentVariableA("ORIGRUN_KEYS", kys, sizeof(kys));
	GetEnvironmentVariableA("ORIGRUN_LASTINPUT", lin, sizeof(lin));
	GetEnvironmentVariableA("ORIGRUN_REGMAP", rgm, sizeof(rgm));
	int clock = !lstrcmpiA(mode, "virtual"), cursor = !lstrcmpiA(cur, "scripted"), msgbox = !lstrcmpiA(mbx, "hook"), keys = !lstrcmpiA(kys, "scripted"), lastin = !lstrcmpiA(lin, "now"), regmap = !lstrcmpiA(rgm, "popcap");
	if (!clock && !cursor && !msgbox && !keys && !lastin && !regmap) return;
	gSelf = self;
	char dir[MAX_PATH] = "", path[MAX_PATH];
	DWORD dn = GetEnvironmentVariableA("ORIGRUN_DIR", dir, sizeof(dir));
	if (dn && dn < sizeof(dir)) {   // the clock's log; cursor.log when the cursor is on without it; else msgbox.log
		wsprintfA(path, "%s\\logs", dir); CreateDirectoryA(path, NULL);
		wsprintfA(path, "%s\\logs\\%s.log", dir, clock ? "clock" : cursor || keys ? "cursor" : "msgbox"); gClog = fopen(path, "a");
	}
	DWORD pid = GetCurrentProcessId();
	char exe[MAX_PATH]; GetModuleFileNameA(NULL, exe, MAX_PATH);
	lstrcpynA(gExeDir, exe, MAX_PATH); { char* l = strrchr(gExeDir, '\\'); if (l) *l = 0; }

	// Pinned before any import points here: a FreeLibrary of dsound.dll (by the game, BASS or FMOD) must not unmap it.
	HMODULE pin = NULL;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)(void*)vclock_install, &pin)) {
		clog("pid %lu: the stand-in could not be pinned (err %lu): no virtual clock, no scripted cursor, no message-box hook", pid, GetLastError());
		return;
	}
	if (cursor || keys) {   // one block for both (cursor.h)
		if (cursor_open()) {
			gCursorOn = cursor; gKeysOn = keys;
			if (cursor) clog("pid %lu %s: scripted cursor installed: GetCursorPos returns origrun's last posted mouse position", pid, exe);
			if (keys) clog("pid %lu %s: scripted keys installed: GetKeyState returns the key and button state origrun's posted edges left", pid, exe);
		}
		else clog("pid %lu: no block from origrun, or another version (err %lu): no scripted cursor or keys", pid, GetLastError());
	}
	if (msgbox) {
		if (msgbox_open()) { gMsgboxOn = 1; clog("pid %lu %s: message-box hook installed: MessageBox* answers with no window, MessageBeep with no sound", pid, exe); }
		else clog("pid %lu: no message-box block from origrun, or another version (err %lu): boxes are shown", pid, GetLastError());
	}
	if (lastin) {
		if (lastinput_detour()) { gLastInputOn = 1; clog("pid %lu %s: last-input hook installed: GetLastInputInfo says the last input is now", pid, exe); }
		else clog("pid %lu: GetLastInputInfo could not be reached (err %lu): no last-input hook", pid, GetLastError());
	}
	if (regmap) {
		gRegMapOn = 1;
		clog("pid %lu %s: registry map installed: a create or open of HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap or a key under it goes to "
			"HKEY_CURRENT_USER with the same path (RegCreateKeyExA, RegOpenKeyExA; RegSetValueExA and RegQueryValueExA only count)", pid, exe);
	}
	int clockOk = clock && clock_install(exe);
	if (!clockOk && !gCursorOn && !gMsgboxOn && !gKeysOn && !gLastInputOn && !gRegMapOn) return;

	for (int k = 0; k < NHOOK; k++) {
		HMODULE m = GetModuleHandleA(gHooks[k].dll);
		gHooks[k].real = m ? (void*)GetProcAddress(m, gHooks[k].name) : NULL;
	}
	gOn = clockOk;
	dll_notice_install();
	InterlockedExchange(&gScanBusy, 1);   // nothing is patched yet, so no rescan holds it; a hook's rescan meanwhile skips
	patch_new_modules();
	InterlockedExchange(&gScanBusy, 0);
	if (gCursorOn) cursor_installed();
	if (gKeysOn) keys_installed();
	if (gMsgboxOn) msgbox_installed();
	if (gOn) {
		VcShared* s = gSh;
		InterlockedExchange(&s->installed, 1);
		s->version = VC_VERSION; s->size = sizeof(VcShared);
		MemoryBarrier();
		s->magic = VC_MAGIC;   // last: origrun follows a block only once it carries this
	}
}

// Called on process detach: the totals.
void vclock_detach(void)
{
	if (!gOn && !gCursorOn && !gMsgboxOn && !gKeysOn && !gLastInputOn && !gRegMapOn) return;
	if (gRegMapOn) {
		LONG creates, opens, misses, writes, reads, found; regmap_stats(&creates, &opens, &misses, &writes, &reads, &found);
		clog("pid %lu exits: registry map: %ld create%s and %ld open%s of HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap keys went to HKEY_CURRENT_USER "
			"(%ld more found no such key there); about %ld value%s written and %ld read through them, %ld of the reads found a value",
			GetCurrentProcessId(), (long)creates, creates == 1 ? "" : "s", (long)opens, opens == 1 ? "" : "s", (long)misses,
			(long)writes, writes == 1 ? "" : "s", (long)reads, (long)found);
	}
	if (gLastInputOn)
		clog("pid %lu exits: GetLastInputInfo answered %ld time%s with the last input now", GetCurrentProcessId(), (long)gLastInputReads, gLastInputReads == 1 ? "" : "s");
	if (gKeysOn) {
		LONG reads, real; keys_stats(&reads, &real);
		clog("pid %lu exits: GetKeyState answered %ld time%s with origrun's key state, %ld passed to Windows (before origrun's first key or button edge)",
			GetCurrentProcessId(), (long)reads, reads == 1 ? "" : "s", (long)real);
	}
	if (gCursorOn) {
		LONG reads, real; cursor_stats(&reads, &real);
		clog("pid %lu exits: GetCursorPos answered %ld time%s with origrun's mouse position, %ld passed to Windows (before origrun's first mouse input)",
			GetCurrentProcessId(), (long)reads, reads == 1 ? "" : "s", (long)real);
	}
	if (gMsgboxOn) {
		LONG boxes, beeps; msgbox_stats(&boxes, &beeps);
		clog("pid %lu exits: %ld message box%s answered with no window, %ld MessageBeep call%s with no sound",
			GetCurrentProcessId(), (long)boxes, boxes == 1 ? "" : "es", (long)beeps, beeps == 1 ? "" : "s");
	}
	if (gOn && gSh) {
		gOn = 0;   // from here the hooks pass through (the block stays mapped: their reads still see the virtual clock)
		VcShared* s = gSh;
		s->gone = 1;
		double vs = (double)s->v / 1e7, rs = (double)(real100() - s->installReal100) / 1e7;
		clog("pid %lu exits: virtual %.1f s in %.1f s of wall time (%.1fx); update counter %ld; %ld timed waits skipped, %ld passed on, "
			"%ld clock reads moved the clock (polling), at most %ld reads between two waits; %ld timed waits took their real time while the "
			"exe's other threads worked (%.1f s of game time); %ld timed waits on handles went to Windows (%ld ran out; %.1f s of game "
			"time); %ld holds (%.1f s)%s; %ld timed waits of other threads on the wall clock; %ld watch changes (%ld dropped)",
			(unsigned long)s->pid, vs, rs, rs > 0 ? vs / rs : 0.0, (long)s->counter, (long)s->waits, (long)s->passWaits, (long)s->polls,
			(long)s->maxReads, (long)s->realWaits, (double)s->realWait100 / 1e7, (long)s->handleWaits, (long)s->handleTimeouts,
			(double)s->handleWait100 / 1e7, (long)s->holds, (double)s->heldReal100 / 1e7, s->barrierOff ? ", then holds were off" : "",
			(long)s->otherWaits, (long)s->ringHead, (long)s->ringDropped);
		for (int i = 0; i < gSiteN; i++) {
			WaitSite* w = &gSites[i];
			if (w->real) clog("  %s on handles from %p went to Windows: %ld, %ld ran out, %.2f s of game time (update counter %ld to %ld)",
				w->fn, w->at, (long)w->n, (long)w->timeouts, (double)w->t100 / 1e7, (long)w->c0, (long)w->c1);
			else clog("  %s skipped from %p: %ld, %.1f s of game time (update counter %ld to %ld)", w->fn, w->at, (long)w->n,
				(double)w->t100 / 1e7, (long)w->c0, (long)w->c1);
		}
	}
	for (int k = 0; k < NHOOK; k++) if (gHooks[k].patched) clog("  %s: %ld import table entr%s", gHooks[k].name, (long)gHooks[k].patched, gHooks[k].patched == 1 ? "y" : "ies");
	if (gClog) { fclose(gClog); gClog = NULL; }
}
