// The activation hook: the copy's windows never take the foreground. visrun runs an original on the user's visible desktop and
// must never take their focus; the game's first ShowWindow is made non-activating by the launch (SW_SHOWNOACTIVATE), but its
// later calls activate the window, and Windows will not let visrun hand the foreground back (it is not the foreground process).
// The user allowed this hook on 2026-11-22 ("Yes, hook window activation calls"). It is on whenever the stand-in is loaded;
// ORIGRUN_ACTIVATION=allow turns it off for a run that needs the real behaviour.
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
// that activates its window). Left alone on purpose, because they do not make a background window the foreground window here: SetFocus (it
// moves the focus inside the calling thread's own queue, and the foreground window of another process stays), CreateWindowEx (a window
// created without WS_VISIBLE is shown by ShowWindow, which is hooked), AnimateWindow, the dialog functions (a dialog is shown by
// ShowWindow and a MessageBox is answered by msgbox.c) and input sent to a window; and what is not on the list at all.
// Written from the public Win32 documentation.

#include <windows.h>
#include <stdio.h>
#include <string.h>

void net_line(const char* fmt, ...);   // net.c's log (logs/net.log of the run folder)

static LONG gOn;                  // the hook is installed
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

static BOOL WINAPI act_SetForegroundWindow(HWND h) { InterlockedIncrement(&gFg); return TRUE; }
static HWND WINAPI act_SetActiveWindow(HWND h) { InterlockedIncrement(&gActive); return GetActiveWindow(); }
static BOOL WINAPI act_BringWindowToTop(HWND h) { InterlockedIncrement(&gTop); return TRUE; }

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
	return gRealShow(h, map_show(cmd, &gShowMapped, &gShowMax, &gShowPassed));
}

static BOOL WINAPI act_ShowWindowAsync(HWND h, int cmd)
{
	return gRealShowAsync(h, map_show(cmd, &gAsyncMapped, &gAsyncMax, NULL));
}

static BOOL WINAPI act_SetWindowPlacement(HWND h, const WINDOWPLACEMENT* p)
{
	if (!p || p->length != sizeof(WINDOWPLACEMENT)) return gRealPlace(h, p);   // the real function refuses it
	WINDOWPLACEMENT c = *p;
	c.showCmd = map_show((int)p->showCmd, &gPlaceMapped, &gPlaceMax, NULL);
	return gRealPlace(h, &c);
}

static void WINAPI act_SwitchToThisWindow(HWND h, BOOL altTab) { InterlockedIncrement(&gSwitch); }

static BOOL WINAPI act_SetWindowPos(HWND h, HWND after, int x, int y, int cx, int cy, UINT flags)
{
	if (!(flags & SWP_NOACTIVATE)) InterlockedIncrement(&gPosMapped);
	return gRealPos(h, after, x, y, cx, cy, flags | SWP_NOACTIVATE);
}

static HDWP WINAPI act_DeferWindowPos(HDWP d, HWND h, HWND after, int x, int y, int cx, int cy, UINT flags)
{
	if (!(flags & SWP_NOACTIVATE)) InterlockedIncrement(&gDeferMapped);
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
	net_line("pid %lu: activation hook installed: reached %s%s%s%s (a window of the copy does not take the foreground through them; ORIGRUN_ACTIVATION=allow turns it off)",
		pid, reached[0] ? reached : "none", missed[0] ? "; not reached (an unknown start of the function): " : "", missed, "");
}

// Called on process detach: the totals.
void act_detach(void)
{
	if (!gOn) return;
	net_line("pid %lu exits: activation hook answered SetForegroundWindow %ld, SetActiveWindow %ld, BringWindowToTop %ld; made non-activating "
		"ShowWindow %ld, SetWindowPos %ld, DeferWindowPos %ld; ShowWindow with SW_SHOWMAXIMIZED passed on %ld (it activates), other ShowWindow commands passed on %ld; "
		"SwitchToThisWindow answered %ld; ShowWindowAsync commands changed %ld (SW_SHOWMAXIMIZED passed on %ld); SetWindowPlacement commands changed %ld (SW_SHOWMAXIMIZED passed on %ld)",
		GetCurrentProcessId(), (long)gFg, (long)gActive, (long)gTop, (long)gShowMapped, (long)gPosMapped, (long)gDeferMapped, (long)gShowMax, (long)gShowPassed,
		(long)gSwitch, (long)gAsyncMapped, (long)gAsyncMax, (long)gPlaceMapped, (long)gPlaceMax);
}
