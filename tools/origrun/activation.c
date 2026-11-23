// The activation hook: the copy's windows never take the foreground. visrun runs an original on the user's visible desktop and
// must never take their focus; the game's first ShowWindow is made non-activating by the launch (SW_SHOWNOACTIVATE), but its
// later calls activate the window, and Windows will not let visrun hand the foreground back (it is not the foreground process).
// The user allowed this hook on 2026-11-22 ("Yes, hook window activation calls"). It is on when the stand-in is loaded on the
// user's own desktop (not rundesk's private one, not Windows Sandbox) or with ORIGRUN_ACTIVATION=block, which visrun sets;
// ORIGRUN_ACTIVATION=allow turns it off everywhere for a run that needs the real behaviour, and origrun sets it in the sandbox,
// whose desktop is "Default" too (the user name below does not tell it apart: Bookworm Adventures' full-screen runs).
//
//   SetForegroundWindow, BringWindowToTop   return TRUE and do nothing.
//   SetActiveWindow                         returns GetActiveWindow() and changes nothing.
//   ShowWindow                              SW_SHOWNORMAL, SW_SHOWDEFAULT and SW_RESTORE become SW_SHOWNOACTIVATE, SW_SHOW becomes
//                                           SW_SHOWNA, SW_SHOWMINIMIZED becomes SW_SHOWMINNOACTIVE; the others pass through.
//                                           SW_SHOWMAXIMIZED has no form that does not activate: it passes through and is counted
//                                           on its own, so the log names it.
//   ShowWindowAsync, SetWindowPlacement     the same command mapping (SetWindowPlacement: on a copy of the WINDOWPLACEMENT's showCmd).
//   SwitchToThisWindow                      does nothing.
//   SetWindowPos, DeferWindowPos            get SWP_NOACTIVATE added.
//
// How the calls are caught: the way the last-input hook and the network block do it (vclock.c, lastinput_detour; net.c): five
// bytes at the start of user32's own function become a jump to the hook, so every caller in the process reaches it, ddraw.dll,
// the d3d DLLs and a function found with GetProcAddress included; an import patch would reach only the modules it scans. The
// hooks of ShowWindow, ShowWindowAsync, SetWindowPlacement, SetWindowPos and DeferWindowPos must still do the work, so each has a trampoline: the bytes the jump
// overwrote, copied to a small executable block, followed by the way on into the function. The copy is made for the prologues
// the 32-bit user32 starts these functions with (mov edi,edi / push ebp / mov ebp,esp: five bytes; an indirect jump
// through a pointer, FF 25: six; a jump E9 already there: five); another start makes the function "not reached" in the
// install line, and the log says so. What this can miss: a caller that makes the system call itself (win32u's NtUser*
// functions, or a syscall stub of its own); user32's own internal calls, which do not go through its exports (a dialog or a menu
// that activates its window); the CBT block (act_cbt, below) stops those too, whichever call asks. Left alone on purpose, because they do not make a background window the foreground window here: SetFocus (it
// moves the focus inside the calling thread's own queue, and the foreground window of another process stays), CreateWindowEx (a window
// created without WS_VISIBLE is shown by ShowWindow, which is hooked), AnimateWindow, the dialog functions (a dialog is shown by
// ShowWindow and a MessageBox is answered by msgbox.c) and input sent to a window; and what is not on the list at all.
// Written from the public Win32 documentation.

#include <windows.h>
#include <tlhelp32.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void net_line(const char* fmt, ...);   // net.c's log (logs/net.log of the run folder)

static LONG gOn;                  // the hook is installed
static LONG gCbtOn;               // the CBT block is installed (new threads get it from DLL_THREAD_ATTACH)
static LONG gCbtBlocked;          // HCBT_ACTIVATE requests the CBT block refused
static LONG gEvents;              // activation events seen (logged up to kMaxEvents)
static HMODULE gSelf;             // this DLL
#define kMaxEvents 200
static LONG gFg, gActive, gTop;   // calls answered with no activation
static LONG gShowMapped, gShowMax, gShowPassed;   // ShowWindow: made non-activating, SW_SHOWMAXIMIZED passed on, others passed on
static LONG gSwitch, gAsyncMapped, gAsyncMax, gPlaceMapped, gPlaceMax;   // SwitchToThisWindow answered; ShowWindowAsync and SetWindowPlacement commands changed / SW_SHOWMAXIMIZED passed on
static LONG gPosMapped, gDeferMapped;   // SetWindowPos and DeferWindowPos calls that got SWP_NOACTIVATE added

typedef BOOL (WINAPI *ShowWindowFn)(HWND, int);
typedef BOOL (WINAPI *SetWindowPosFn)(HWND, HWND, int, int, int, int, UINT);
typedef HDWP (WINAPI *DeferWindowPosFn)(HDWP, HWND, HWND, int, int, int, int, UINT);
static ShowWindowFn      gRealShow;
static SetWindowPosFn    gRealPos;
static DeferWindowPosFn  gRealDefer;
static ShowWindowFn      gRealShowAsync;
static BOOL (WINAPI *gRealPlace)(HWND, const WINDOWPLACEMENT*);


// ---- the live record -------------------------------------------------------------------------------------------------
// Every activation event (a hooked function called, or an HCBT_ACTIVATE refused) gets one line in net.log, written at once (net_line
// is an unbuffered WriteFile), so a run that is killed still shows what happened. At most kMaxEvents lines per process, then one
// "further events not logged" line.

static const char* base_name(const char* path) { const char* s = strrchr(path, '\\'); return s ? s + 1 : path; }

// "module+0xoffset" of an address, or "?" when it is in no module
static void mod_off(const void* addr, char* out, int cap)
{
	HMODULE m = NULL; char path[MAX_PATH];
	if (addr && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &m) && m
		&& GetModuleFileNameA(m, path, MAX_PATH))
		_snprintf(out, cap, "%s+0x%lx", base_name(path), (unsigned long)((const BYTE*)addr - (const BYTE*)m));
	else lstrcpynA(out, "?", cap);
	out[cap - 1] = 0;
}

// The process that owns the foreground window right now: "none", "this process" or "name.exe (pid n)"
static void fg_owner(char* out, int cap)
{
	HWND f = GetForegroundWindow();
	DWORD pid = 0;
	if (!f) { lstrcpynA(out, "none", cap); return; }
	GetWindowThreadProcessId(f, &pid);
	if (pid == GetCurrentProcessId()) { lstrcpynA(out, "this process", cap); return; }
	char nm[MAX_PATH] = "?";
	HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (p) { DWORD n = MAX_PATH; char full[MAX_PATH]; if (QueryFullProcessImageNameA(p, 0, full, &n)) lstrcpynA(nm, base_name(full), MAX_PATH); CloseHandle(p); }
	_snprintf(out, cap, "%s (pid %lu)", nm, (unsigned long)pid);
	out[cap - 1] = 0;
}

// `where` is the caller's address as text (module+offset)
static void act_event(const char* fn, HWND h, const char* where, const char* fmt, ...)
{
	LONG n = InterlockedIncrement(&gEvents);
	if (n > kMaxEvents + 1) return;
	DWORD pid = GetCurrentProcessId();
	if (n == kMaxEvents + 1) { net_line("pid %lu: activation: further events not logged (%d logged)", pid, kMaxEvents); return; }
	char cls[64] = "(no window)", detail[160] = "", fg[MAX_PATH + 32];
	if (h) { if (!GetClassNameA(h, cls, sizeof(cls))) lstrcpyA(cls, "(not a window)"); }
	va_list ap; va_start(ap, fmt); _vsnprintf(detail, sizeof(detail) - 1, fmt, ap); va_end(ap);
	fg_owner(fg, sizeof(fg));
	net_line("pid %lu: activation event %ld: %s window %p class \"%s\" %s; called from %s; the foreground was owned by %s",
		pid, (long)n, fn, (void*)h, cls, detail, where, fg);
}
#define EVENT(fn, h, ...) do { char _w[96]; mod_off(__builtin_return_address(0), _w, sizeof(_w)); act_event(fn, h, _w, __VA_ARGS__); } while (0)

// ---- the CBT block -----------------------------------------------------------------------------------------------------
// A thread-local WH_CBT hook whose procedure refuses HCBT_ACTIVATE, whichever call asked for it (a function the hooks above do not
// cover, win32u's NtUser* entry points, user32's own internal calls, a window created with WS_VISIBLE, Direct3D's CreateDevice).
// Written after Bejeweled Twist's 3D original took the foreground once in the first visrun run although all nine functions were
// hooked. hMod is NULL and the thread id is one of this process: no DLL has to be loaded into any process, so SetWindowsHookEx is safe
// under the loader lock (it makes one system call and touches no loader state; user32 is already attached because the stand-in
// imports it and is initialised first). The hook is put on every thread that exists when the stand-in loads (a toolhelp snapshot,
// from DllMain) and on each new thread from DLL_THREAD_ATTACH, which runs on the new thread itself.

static LRESULT CALLBACK act_cbt(int code, WPARAM wp, LPARAM lp)
{
	if (code == HCBT_ACTIVATE) {
		InterlockedIncrement(&gCbtBlocked);
		if (InterlockedCompareExchange(&gEvents, 0, 0) <= kMaxEvents) {
			// Who asked: the first frames of the stack outside the system's windowing DLLs and this one (the callback runs on the
			// stack of the thread that made the call; the frame-pointer walk can be short in code without frame pointers)
			void* fr[24]; USHORT n = RtlCaptureStackBackTrace(1, 24, fr, NULL);
			HMODULE skip[4] = { gSelf, GetModuleHandleA("user32.dll"), GetModuleHandleA("win32u.dll"), GetModuleHandleA("ntdll.dll") };
			char where[200] = "", one[96]; int shown = 0;
			for (USHORT i = 0; i < n && shown < 3; i++) {
				HMODULE m = NULL; int sk = 0;
				if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)fr[i], &m))
					for (int k = 0; k < 4; k++) if (m == skip[k]) sk = 1;
				if (sk) continue;
				mod_off(fr[i], one, sizeof(one));
				if (shown) strcat(where, " <- ");
				strncat(where, one, sizeof(where) - strlen(where) - 1);
				shown++;
			}
			if (!shown) lstrcpyA(where, "unknown (no frame outside user32/win32u/ntdll)");
			act_event("CBT activate", (HWND)wp, where, "refused (%s)", lp && ((CBTACTIVATESTRUCT*)lp)->fMouse ? "by mouse" : "not by mouse");
		}
		return 1;   // nonzero: the system does not activate the window
	}
	return CallNextHookEx(NULL, code, wp, lp);
}

static int cbt_on_thread(DWORD tid)
{
	return SetWindowsHookExW(WH_CBT, act_cbt, NULL, tid) != NULL;
}

// SetWindowsHookEx on another thread works only for a thread that has user32 state (a GUI thread: one that has made a user32 call that
// needs a queue); error 87 otherwise. Such a thread, when the stand-in loads, is one that has no window yet; the ids go in gPending
// and a helper thread of the stand-in (started outside the loader lock's wait: DllMain only creates it) tries again until each
// thread has the hook or the time (kRetryMs) is up. A game that imports dsound.dll loads it before any thread but the main one
// exists, so this is for a stand-in loaded late and for the loader's own worker threads, which never own windows.
#define kMaxPending 64
#define kRetryMs 5000
static DWORD gPending[kMaxPending];
static int gPendingN;
static DWORD gCbtErr;   // the last error of a failed SetWindowsHookEx in the snapshot

static void cbt_install_all(int* ok, int* failed)
{
	DWORD pid = GetCurrentProcessId();
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	*ok = *failed = 0;
	if (snap == INVALID_HANDLE_VALUE) { *failed = -1; return; }
	THREADENTRY32 te; te.dwSize = sizeof(te);
	for (BOOL more = Thread32First(snap, &te); more; more = Thread32Next(snap, &te)) {
		if (te.th32OwnerProcessID != pid) continue;
		if (cbt_on_thread(te.th32ThreadID)) (*ok)++;
		else { (*failed)++; gCbtErr = GetLastError(); if (gPendingN < kMaxPending) gPending[gPendingN++] = te.th32ThreadID; }
	}
	CloseHandle(snap);
}

static DWORD WINAPI cbt_retry_thread(LPVOID unused)
{
	DWORD me = GetCurrentThreadId(), start = GetTickCount();
	int left = gPendingN;
	for (int i = 0; i < gPendingN; i++) if (gPending[i] == me) { gPending[i] = 0; left--; }
	while (left > 0 && GetTickCount() - start < kRetryMs) {
		Sleep(20);
		for (int i = 0; i < gPendingN; i++)
			if (gPending[i] && cbt_on_thread(gPending[i])) { gPending[i] = 0; left--; }
	}
	net_line("pid %lu: activation CBT block: %d of the threads that had no user32 state when the stand-in loaded %s", GetCurrentProcessId(),
		left, left ? "still have none after the retry time (threads that never own windows, or ones that began to after it)" : "were hooked later, when they got it");
	return 0;
}

static void cbt_start_retry(void)
{
	if (!gPendingN) return;
	HANDLE t = CreateThread(NULL, 0, cbt_retry_thread, NULL, 0, NULL);
	if (t) CloseHandle(t);
}

// DLL_THREAD_ATTACH: this thread is new and may own windows
void act_thread_attach(void)
{
	if (gCbtOn) cbt_on_thread(GetCurrentThreadId());
}

static BOOL WINAPI act_SetForegroundWindow(HWND h) { InterlockedIncrement(&gFg); EVENT("SetForegroundWindow", h, "answered TRUE, nothing done"); return TRUE; }
static HWND WINAPI act_SetActiveWindow(HWND h) { InterlockedIncrement(&gActive); EVENT("SetActiveWindow", h, "answered with the active window, nothing done"); return GetActiveWindow(); }
static BOOL WINAPI act_BringWindowToTop(HWND h) { InterlockedIncrement(&gTop); EVENT("BringWindowToTop", h, "answered TRUE, nothing done"); return TRUE; }

// The command mapping of ShowWindow, ShowWindowAsync and SetWindowPlacement's showCmd; `mapped` counts the commands changed, `max`
// the SW_SHOWMAXIMIZED passes, `passed` the others (NULL: not counted).
static int map_show(int cmd, LONG* mapped, LONG* max, LONG* passed)
{
	int to = cmd;
	switch (cmd) {
	case SW_SHOWNORMAL: case SW_SHOWDEFAULT: case SW_RESTORE: to = SW_SHOWNOACTIVATE; break;   // SW_SHOWNORMAL = SW_NORMAL = 1
	case SW_SHOW:         to = SW_SHOWNA; break;
	case SW_SHOWMINIMIZED: to = SW_SHOWMINNOACTIVE; break;
	case SW_SHOWMAXIMIZED: if (max) InterlockedIncrement(max); break;
	default: if (passed) InterlockedIncrement(passed); break;
	}
	if (to != cmd && mapped) InterlockedIncrement(mapped);
	return to;
}

static BOOL WINAPI act_ShowWindow(HWND h, int cmd)
{
	int to = map_show(cmd, &gShowMapped, &gShowMax, &gShowPassed);
	if (to != cmd) EVENT("ShowWindow", h, "command %d made %d", cmd, to);
	else if (cmd == SW_SHOWMAXIMIZED) EVENT("ShowWindow", h, "SW_SHOWMAXIMIZED passed on (it activates)");
	return gRealShow(h, to);
}

static BOOL WINAPI act_ShowWindowAsync(HWND h, int cmd)
{
	int to = map_show(cmd, &gAsyncMapped, &gAsyncMax, NULL);
	if (to != cmd) EVENT("ShowWindowAsync", h, "command %d made %d", cmd, to);
	else if (cmd == SW_SHOWMAXIMIZED) EVENT("ShowWindowAsync", h, "SW_SHOWMAXIMIZED passed on (it activates)");
	return gRealShowAsync(h, to);
}

static BOOL WINAPI act_SetWindowPlacement(HWND h, const WINDOWPLACEMENT* p)
{
	if (!p || p->length != sizeof(WINDOWPLACEMENT)) return gRealPlace(h, p);   // the real function refuses it
	WINDOWPLACEMENT c = *p;
	c.showCmd = map_show((int)p->showCmd, &gPlaceMapped, &gPlaceMax, NULL);
	if (c.showCmd != p->showCmd) EVENT("SetWindowPlacement", h, "showCmd %u made %u", p->showCmd, c.showCmd);
	else if (p->showCmd == SW_SHOWMAXIMIZED) EVENT("SetWindowPlacement", h, "SW_SHOWMAXIMIZED passed on (it activates)");
	return gRealPlace(h, &c);
}

static void WINAPI act_SwitchToThisWindow(HWND h, BOOL altTab) { InterlockedIncrement(&gSwitch); EVENT("SwitchToThisWindow", h, "altTab %d, nothing done", (int)altTab); }

static BOOL WINAPI act_SetWindowPos(HWND h, HWND after, int x, int y, int cx, int cy, UINT flags)
{
	if (!(flags & SWP_NOACTIVATE)) { InterlockedIncrement(&gPosMapped); EVENT("SetWindowPos", h, "flags 0x%x got SWP_NOACTIVATE", flags); }
	return gRealPos(h, after, x, y, cx, cy, flags | SWP_NOACTIVATE);
}

static HDWP WINAPI act_DeferWindowPos(HDWP d, HWND h, HWND after, int x, int y, int cx, int cy, UINT flags)
{
	if (!(flags & SWP_NOACTIVATE)) { InterlockedIncrement(&gDeferMapped); EVENT("DeferWindowPos", h, "flags 0x%x got SWP_NOACTIVATE", flags); }
	return gRealDefer(d, h, after, x, y, cx, cy, flags | SWP_NOACTIVATE);
}

static int write_jump(BYTE* f, void* hook)
{
	DWORD old;
	if (!VirtualProtect(f, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
	LONG rel = (LONG)((BYTE*)hook - (f + 5));
	f[0] = 0xE9; memcpy(f + 1, &rel, 4);
	VirtualProtect(f, 5, old, &old);
	FlushInstructionCache(GetCurrentProcess(), f, 5);
	return 1;
}

// Puts a jump to hook at user32's function `name`. When `real` is not NULL, first builds the trampoline and stores its address
// there (the hook calls it as the function). Returns 1 when the jump is in; the install line says which were reached.
static int hook_user32(HMODULE u, const char* name, void* hook, void** real)
{
	BYTE* f = (BYTE*)GetProcAddress(u, name);
	if (!f) return 0;
	if (real) {
		int len = 0, tail = 1;
		BYTE* target = f;   // where the trampoline goes on to (after its copied bytes)
		if (f[0] == 0x8B && f[1] == 0xFF && f[2] == 0x55 && f[3] == 0x8B && f[4] == 0xEC) { len = 5; target = f + 5; }
		else if (f[0] == 0xFF && f[1] == 0x25) { len = 6; tail = 0; }   // jmp [address]: the copy is the whole function
		else if (f[0] == 0xE9) { len = 5; LONG rel; memcpy(&rel, f + 1, 4); target = f + 5 + rel; tail = 2; }   // a jump already there
		else return 0;
		BYTE* t = (BYTE*)VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!t) return 0;
		memcpy(t, f, len);
		if (tail == 2) { LONG rel = (LONG)(target - (t + 5)); memcpy(t + 1, &rel, 4); }   // the copied E9 jumps to the same place
		else if (tail) { t[len] = 0xE9; LONG rel = (LONG)(target - (t + len + 5)); memcpy(t + len + 1, &rel, 4); }
		FlushInstructionCache(GetCurrentProcess(), t, 16);
		*real = t;
	}
	return write_jump(f, hook);
}

// Called from the stand-in's DllMain on process attach, after net_install (which opened the log and pinned the DLL).
void act_install(void)
{
	char sw[16] = "";
	GetEnvironmentVariableA("ORIGRUN_ACTIVATION", sw, sizeof(sw));
	DWORD pid = GetCurrentProcessId();
	if (!lstrcmpiA(sw, "allow")) { net_line("pid %lu: activation hook off (ORIGRUN_ACTIVATION=allow): the copy's window activation calls reach Windows", pid); return; }
	// Only on the user's own desktop, where a copy's window would take the user's focus. On rundesk's private desktop and in
	// Windows Sandbox (its WDAGUtilityAccount) the copy's window is activated as in a plain run, so a game that waits for the
	// focus runs as it always has there; ORIGRUN_ACTIVATION=block puts the hook in anywhere (actest under rundesk), and visrun
	// sets it for every run it starts.
	if (lstrcmpiA(sw, "block")) {
		char desk[64] = "", user[64] = "";
		HDESK d = GetThreadDesktop(GetCurrentThreadId());
		if (!d || !GetUserObjectInformationA(d, UOI_NAME, desk, sizeof(desk), NULL)) desk[0] = 0;
		GetEnvironmentVariableA("USERNAME", user, sizeof(user));
		if (lstrcmpiA(desk, "Default") || !lstrcmpiA(user, "WDAGUtilityAccount")) {
			net_line("pid %lu: activation hook off (not the user's desktop: desktop \"%s\", user \"%s\"; ORIGRUN_ACTIVATION=block puts it in): the copy's window is activated as in a plain run", pid, desk, user);
			return;
		}
	}
	HMODULE u = GetModuleHandleA("user32.dll");   // the stand-in imports user32: it is loaded already
	if (!u) { net_line("pid %lu: user32.dll not loaded: no activation hook", pid); return; }
	static const struct { const char* name; void* hook; void** real; } fns[] = {
		{ "SetForegroundWindow", (void*)act_SetForegroundWindow, NULL },
		{ "SetActiveWindow",     (void*)act_SetActiveWindow,     NULL },
		{ "BringWindowToTop",    (void*)act_BringWindowToTop,    NULL },
		{ "ShowWindow",          (void*)act_ShowWindow,          (void**)&gRealShow },
		{ "SetWindowPos",        (void*)act_SetWindowPos,        (void**)&gRealPos },
		{ "DeferWindowPos",      (void*)act_DeferWindowPos,      (void**)&gRealDefer },
		{ "ShowWindowAsync",     (void*)act_ShowWindowAsync,     (void**)&gRealShowAsync },
		{ "SetWindowPlacement",  (void*)act_SetWindowPlacement,  (void**)&gRealPlace },
		{ "SwitchToThisWindow",  (void*)act_SwitchToThisWindow,  NULL },
	};
	char reached[200] = "", missed[200] = "";
	for (int i = 0; i < (int)(sizeof(fns) / sizeof(fns[0])); i++) {
		char* list = hook_user32(u, fns[i].name, fns[i].hook, fns[i].real) ? reached : missed;
		if (list[0]) strcat(list, ", ");
		strcat(list, fns[i].name);
	}
	gOn = 1;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)act_install, &gSelf);
	int ok, failed; cbt_install_all(&ok, &failed);
	gCbtOn = 1;
	cbt_start_retry();
	net_line("pid %lu: activation hook installed: reached %s%s%s%s (a window of the copy does not take the foreground through them; ORIGRUN_ACTIVATION=allow turns it off)",
		pid, reached[0] ? reached : "none", missed[0] ? "; not reached (an unknown start of the function): " : "", missed, "");
	net_line("pid %lu: activation CBT block installed on %d threads (%d failed, last error %lu); later threads get it when they start: HCBT_ACTIVATE is refused whichever call asks for it", pid, ok, failed, (unsigned long)gCbtErr);
}

// Called on process detach: the totals.
void act_detach(void)
{
	if (!gOn) return;
	net_line("pid %lu exits: activation hook answered SetForegroundWindow %ld, SetActiveWindow %ld, BringWindowToTop %ld; made non-activating "
		"ShowWindow %ld, SetWindowPos %ld, DeferWindowPos %ld; ShowWindow with SW_SHOWMAXIMIZED passed on %ld (it activates), other ShowWindow commands passed on %ld; "
		"SwitchToThisWindow answered %ld; ShowWindowAsync commands changed %ld (SW_SHOWMAXIMIZED passed on %ld); SetWindowPlacement commands changed %ld (SW_SHOWMAXIMIZED passed on %ld); CBT activations refused %ld; activation events %ld (the first %d are logged)",
		GetCurrentProcessId(), (long)gFg, (long)gActive, (long)gTop, (long)gShowMapped, (long)gPosMapped, (long)gDeferMapped, (long)gShowMax, (long)gShowPassed,
		(long)gSwitch, (long)gAsyncMapped, (long)gAsyncMax, (long)gPlaceMapped, (long)gPlaceMax, (long)gCbtBlocked, (long)gEvents, kMaxEvents);
}
