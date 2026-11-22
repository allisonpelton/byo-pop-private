// The activation hook (tools/origrun/activation.c) from end to end, for check.sh's origrun suite (tools/origrun/actest.sh). The
// stand-in is loaded the way a game's import of dsound.dll loads it; the exe shows a window without activating it, then calls
// SetForegroundWindow, SetActiveWindow, BringWindowToTop, ShowWindow with every command, SetWindowPos and DeferWindowPos on
// it, through its imports, through GetProcAddress and from a second DLL (actdll.dll). After each call neither
// GetForegroundWindow() nor GetActiveWindow() may be the window, and the window must be in the state the mapped command gives.
// With ORIGRUN_ACTIVATION=allow in the environment the calls must instead reach Windows: the window becomes the active one.
// Then the cases the nine hooks cannot catch and only the CBT block (activation.c) stops: a top-level window created with WS_VISIBLE,
// SetFocus on the test's own top-level window, win32u.dll's NtUserSetForegroundWindow through GetProcAddress (the system call itself,
// no user32 export in the way), and the same window creation on a thread that existed before the stand-in loaded and on one started
// after it. Two child runs (this exe with "first" as its argument, started before the stand-in loads) make the process's first
// ShowWindow(SW_SHOWNORMAL), once with the default startup information and once with STARTF_USESHOWWINDOW and SW_SHOWNORMAL: the first
// ShowWindow call of a process takes its command from the startup information, which overrides the hook's mapped command.
// actest.out beside the exe: one line per check, and "ACTTEST n of n as expected" last.
#include "actcheck.h"
#include <stdio.h>
#include <string.h>

static FILE* gOut;
static int gN, gOk;
static void check(const char* what, int ok)
{
	gN++; if (ok) gOk++;
	fprintf(gOut, "%s: %s\n", ok ? "ok" : "FAIL", what);
	fflush(gOut);
}
static void pump(void) { MSG m; while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageA(&m); } }
static int notmine(HWND w) { pump(); return GetForegroundWindow() != w && GetActiveWindow() != w; }
static int active(HWND w) { pump(); return GetActiveWindow() == w; }
#define CK(via, what, cond) do { char _b[200]; _snprintf(_b, sizeof(_b), "%s: %s", via, what); check(_b, (cond)); } while (0)

static void run_blocked(HWND w, const ActApi* a, const char* via)
{
	HWND before = GetActiveWindow();
	CK(via, "SetForegroundWindow returns TRUE", a->SetForegroundWindow(w) == TRUE);
	CK(via, "SetForegroundWindow leaves the window inactive", notmine(w));
	CK(via, "SetActiveWindow returns the active window it found", a->SetActiveWindow(w) == before);
	CK(via, "SetActiveWindow leaves the window inactive", notmine(w));
	CK(via, "BringWindowToTop returns TRUE", a->BringWindowToTop(w) == TRUE);
	CK(via, "BringWindowToTop leaves the window inactive", notmine(w));

	a->ShowWindow(w, SW_SHOWMINIMIZED);
	CK(via, "SW_SHOWMINIMIZED minimizes without activating (SW_SHOWMINNOACTIVE)", IsIconic(w) && IsWindowVisible(w) && notmine(w));
	a->ShowWindow(w, SW_RESTORE);
	CK(via, "SW_RESTORE restores without activating (SW_SHOWNOACTIVATE)", !IsIconic(w) && IsWindowVisible(w) && notmine(w));
	a->ShowWindow(w, SW_SHOWMINIMIZED);
	a->ShowWindow(w, SW_SHOWNORMAL);
	CK(via, "SW_SHOWNORMAL shows without activating (SW_SHOWNOACTIVATE)", !IsIconic(w) && IsWindowVisible(w) && notmine(w));
	a->ShowWindow(w, SW_HIDE);
	CK(via, "ShowWindow passes SW_HIDE through", !IsWindowVisible(w));
	a->ShowWindow(w, SW_SHOW);
	CK(via, "SW_SHOW shows without activating (SW_SHOWNA)", IsWindowVisible(w) && notmine(w));
	a->ShowWindow(w, SW_HIDE);
	a->ShowWindow(w, SW_SHOWDEFAULT);
	CK(via, "SW_SHOWDEFAULT shows without activating (SW_SHOWNOACTIVATE)", IsWindowVisible(w) && notmine(w));

	a->ShowWindow(w, SW_HIDE);
	a->SetWindowPos(w, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	CK(via, "SetWindowPos with SWP_SHOWWINDOW shows without activating", IsWindowVisible(w) && notmine(w));
	a->SetWindowPos(w, HWND_TOP, 30, 40, 0, 0, SWP_NOSIZE);
	RECT r; GetWindowRect(w, &r);
	CK(via, "SetWindowPos still moves the window", r.left == 30 && r.top == 40 && notmine(w));
	a->ShowWindow(w, SW_HIDE);
	HDWP d = a->BeginDeferWindowPos(1);
	d = a->DeferWindowPos(d, w, HWND_TOP, 50, 60, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	BOOL ended = d ? a->EndDeferWindowPos(d) : FALSE;
	GetWindowRect(w, &r);
	CK(via, "DeferWindowPos shows and moves without activating", ended && IsWindowVisible(w) && r.left == 50 && r.top == 60 && notmine(w));

	a->ShowWindowAsync(w, SW_SHOWMINIMIZED); pump();
	CK(via, "ShowWindowAsync SW_SHOWMINIMIZED minimizes without activating", IsIconic(w) && notmine(w));
	a->ShowWindowAsync(w, SW_RESTORE); pump();
	CK(via, "ShowWindowAsync SW_RESTORE restores without activating", !IsIconic(w) && IsWindowVisible(w) && notmine(w));
	a->ShowWindow(w, SW_HIDE);
	a->ShowWindowAsync(w, SW_SHOWNORMAL); pump();
	CK(via, "ShowWindowAsync SW_SHOWNORMAL shows without activating", IsWindowVisible(w) && notmine(w));

	WINDOWPLACEMENT wp; memset(&wp, 0, sizeof(wp)); wp.length = sizeof(wp);
	a->GetWindowPlacement(w, &wp);
	wp.showCmd = SW_SHOWMINIMIZED;
	CK(via, "SetWindowPlacement returns TRUE", a->SetWindowPlacement(w, &wp) == TRUE);
	CK(via, "SetWindowPlacement SW_SHOWMINIMIZED minimizes without activating", IsIconic(w) && notmine(w));
	CK(via, "SetWindowPlacement leaves the caller's structure alone", wp.showCmd == SW_SHOWMINIMIZED);
	wp.showCmd = SW_SHOWNORMAL; wp.rcNormalPosition.left = 70; wp.rcNormalPosition.right = 300;
	a->SetWindowPlacement(w, &wp);
	GetWindowRect(w, &r);
	CK(via, "SetWindowPlacement SW_SHOWNORMAL restores and places without activating", !IsIconic(w) && IsWindowVisible(w) && r.left == 70 && notmine(w));

	CK(via, "SwitchToThisWindow does nothing", (a->SwitchToThisWindow(w, TRUE), a->SwitchToThisWindow(w, FALSE), notmine(w)));
}

// SW_SHOWMAXIMIZED has no form that does not activate: it passes through (and the stand-in counts it on its own)
static void run_maximized(HWND w, const ActApi* a, const char* via)
{
	a->ShowWindow(w, SW_SHOWMAXIMIZED);
	CK(via, "SW_SHOWMAXIMIZED passes through: the window is maximized", IsZoomed(w) && IsWindowVisible(w));
	a->ShowWindow(w, SW_SHOWNORMAL);
	a->ShowWindowAsync(w, SW_SHOWMAXIMIZED); pump();
	CK(via, "ShowWindowAsync SW_SHOWMAXIMIZED passes through: the window is maximized", IsZoomed(w));
	a->ShowWindow(w, SW_SHOWNORMAL);
	WINDOWPLACEMENT wp; memset(&wp, 0, sizeof(wp)); wp.length = sizeof(wp);
	a->GetWindowPlacement(w, &wp); wp.showCmd = SW_SHOWMAXIMIZED; a->SetWindowPlacement(w, &wp);
	CK(via, "SetWindowPlacement SW_SHOWMAXIMIZED passes through: the window is maximized", IsZoomed(w));
	a->ShowWindow(w, SW_SHOWNORMAL);
	pump();
}

// ORIGRUN_ACTIVATION=allow: the calls reach Windows, which activates the window on this thread
static void run_allowed(HWND w, HWND other, const ActApi* a, const char* via)
{
	a->SetActiveWindow(other);
	CK(via, "allowed: SetActiveWindow activates the other window", active(other));
	a->ShowWindow(w, SW_HIDE);
	a->ShowWindow(w, SW_SHOWNORMAL);
	CK(via, "allowed: SW_SHOWNORMAL activates the window", active(w));
	a->SetActiveWindow(other);
	CK(via, "allowed: SetActiveWindow moves the activation away again", active(other));
	a->SetActiveWindow(w);
	CK(via, "allowed: SetActiveWindow activates the window", active(w));
	a->SetActiveWindow(other);
	a->ShowWindow(w, SW_HIDE);
	a->ShowWindowAsync(w, SW_SHOWNORMAL); pump();
	CK(via, "allowed: ShowWindowAsync SW_SHOWNORMAL activates the window", active(w));
	a->SetActiveWindow(other);
	a->ShowWindow(w, SW_HIDE);
	WINDOWPLACEMENT wp; memset(&wp, 0, sizeof(wp)); wp.length = sizeof(wp);
	a->GetWindowPlacement(w, &wp); wp.showCmd = SW_SHOWNORMAL; a->SetWindowPlacement(w, &wp);
	CK(via, "allowed: SetWindowPlacement SW_SHOWNORMAL activates the window", active(w));
}

static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM wp, LPARAM lp) { return DefWindowProcA(h, m, wp, lp); }

static int gAllow;
static HMODULE gWin32u;
static BOOL (WINAPI *gNtSetFg)(HWND);

// A window made visible by its creation, and SetFocus on a window shown inactive: no function the hooks cover is called
static void run_cbt(HINSTANCE inst, HWND other, const char* via)
{
	HWND v = CreateWindowExA(0, "ActTest", "visible", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 20, 200, 240, 160, NULL, NULL, inst, NULL);
	pump();
	CK(via, gAllow ? "allowed: a window created with WS_VISIBLE is active" : "a window created with WS_VISIBLE is not active or the foreground", v && (gAllow ? active(v) : notmine(v)));
	if (v) DestroyWindow(v);
	pump();

	HWND f = CreateWindowExA(0, "ActTest", "focus", WS_OVERLAPPEDWINDOW, 40, 220, 240, 160, NULL, NULL, inst, NULL);
	ShowWindow(f, SW_SHOWNOACTIVATE);
	SetActiveWindow(other); pump();
	SetFocus(f); pump();
	CK(via, gAllow ? "allowed: SetFocus on a top-level window activates it" : "SetFocus on a top-level window does not activate it or make it the foreground", f && (gAllow ? active(f) : notmine(f)));
	if (f) DestroyWindow(f);
	pump();

	if (!gNtSetFg) { fprintf(gOut, "skipped: win32u.dll!NtUserSetForegroundWindow is not exported on this Windows\n"); fflush(gOut); return; }
	HWND n = CreateWindowExA(0, "ActTest", "nt", WS_OVERLAPPEDWINDOW, 60, 240, 240, 160, NULL, NULL, inst, NULL);
	ShowWindow(n, SW_SHOWNOACTIVATE);
	gNtSetFg(n); pump();
	CK(via, gAllow ? "allowed: win32u.dll!NtUserSetForegroundWindow activates the window" : "win32u.dll!NtUserSetForegroundWindow (through GetProcAddress) does not activate the window or make it the foreground",
		n && (gAllow ? (active(n) || GetForegroundWindow() == n) : notmine(n)));
	if (n) DestroyWindow(n);
	pump();
}

// A thread that owns a window created with WS_VISIBLE; gGo holds a thread that started before the stand-in loaded
static HANDLE gGo, gReady;
static DWORD WINAPI thread_window(LPVOID arg)
{
	const char* via = (const char*)arg;
	if (gGo) { pump(); SetEvent(gReady); WaitForSingleObject(gGo, 30000); }   // pump: a thread with user32 state, the only kind a hook can be put on from outside
	HWND v = CreateWindowExA(0, "ActTest", "thread", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 80, 260, 240, 160, NULL, NULL, GetModuleHandleA(NULL), NULL);
	pump();
	CK(via, gAllow ? "allowed: a window created with WS_VISIBLE on this thread is active" : "a window created with WS_VISIBLE on this thread is not active or the foreground", v && (gAllow ? active(v) : notmine(v)));
	if (v) DestroyWindow(v);
	pump();
	return 0;
}

// The process's first ShowWindow call (the child runs of main): its command comes from the startup information
static int first_main(const char* tag)
{
	char path[MAX_PATH], outp[MAX_PATH], tmp[40];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	char* l = strrchr(path, '\\'); if (l) l[1] = 0;
	lstrcpyA(outp, path); lstrcatA(outp, "actest-first-"); lstrcatA(outp, tag); lstrcatA(outp, ".out");
	gOut = fopen(outp, "w");
	if (!gOut) return 2;
	gAllow = GetEnvironmentVariableA("ORIGRUN_ACTIVATION", tmp, sizeof(tmp)) && !lstrcmpiA(tmp, "allow");
	LoadLibraryA("dsound.dll");
	WNDCLASSA wc; memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = proc; wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "ActTest";
	RegisterClassA(&wc);
	HWND w = CreateWindowExA(0, "ActTest", "first", WS_OVERLAPPEDWINDOW, 10, 10, 240, 160, NULL, NULL, wc.hInstance, NULL);
	ShowWindow(w, SW_SHOWNORMAL);   // the first call: SW_SHOWNORMAL here, whatever the hook makes of it
	pump();
	CK(tag, gAllow ? "allowed: the first ShowWindow(SW_SHOWNORMAL) activates the window" : "the first ShowWindow(SW_SHOWNORMAL) does not activate the window or make it the foreground", w && (gAllow ? active(w) : notmine(w)));
	fprintf(gOut, "ACTTEST %d of %d as expected\n", gOk, gN);
	fclose(gOut);
	return gOk == gN ? 0 : 1;
}

// Runs this exe as a child with "first" and the tag, with the given startup information, before the stand-in is loaded here
static void run_child(const char* tag, int useShow)
{
	char exe[MAX_PATH], cmd[MAX_PATH + 40], outp[MAX_PATH];
	GetModuleFileNameA(NULL, exe, MAX_PATH);
	_snprintf(cmd, sizeof(cmd), "\"%s\" first %s", exe, tag);
	lstrcpyA(outp, exe); char* l = strrchr(outp, '\\'); if (l) l[1] = 0;
	lstrcatA(outp, "actest-first-"); lstrcatA(outp, tag); lstrcatA(outp, ".out");
	DeleteFileA(outp);
	STARTUPINFOA si; memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
	if (useShow) { si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_SHOWNORMAL; }
	PROCESS_INFORMATION pi;
	DWORD code = 99;
	if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		if (WaitForSingleObject(pi.hProcess, 60000) == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, &code);
		else TerminateProcess(pi.hProcess, 98);
		CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
	}
	char line[200] = "";
	FILE* f = fopen(outp, "r");
	int found = 0;
	if (f) { while (fgets(line, sizeof(line), f)) if (!strncmp(line, "ok:", 3) || !strncmp(line, "FAIL", 4)) { found = 1; fputs(line, gOut); } fclose(f); }
	CK(tag, "the child run made its check and passed", found && code == 0);
}

int main(int argc, char** argv)
{
	if (argc > 2 && !strcmp(argv[1], "first")) return first_main(argv[2]);
	char path[MAX_PATH], outp[MAX_PATH], tmp[40];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	char* l = strrchr(path, '\\'); if (l) l[1] = 0;
	lstrcpyA(outp, path); lstrcatA(outp, "actest.out");
	gOut = fopen(outp, "w");
	if (!gOut) return 2;
	int allow = GetEnvironmentVariableA("ORIGRUN_ACTIVATION", tmp, sizeof(tmp)) && !lstrcmpiA(tmp, "allow");
	gAllow = allow;
	// before the stand-in loads: the two child runs (its launch guard would refuse CreateProcess), and a thread that exists when it loads
	run_child("default-show", 0);
	run_child("show-normal", 1);
	gGo = CreateEventA(NULL, TRUE, FALSE, NULL); gReady = CreateEventA(NULL, TRUE, FALSE, NULL);
	HANDLE early = CreateThread(NULL, 0, thread_window, (void*)"thread started before the stand-in loaded", 0, NULL);
	WaitForSingleObject(gReady, 30000);

	HMODULE standIn = LoadLibraryA("dsound.dll");   // the stand-in, from the exe's folder
	{
		char p[MAX_PATH] = ""; if (standIn) GetModuleFileNameA(standIn, p, MAX_PATH);
		char* s = strrchr(p, '\\');
		check("the stand-in dsound.dll is the one beside the exe", standIn && s && !_strnicmp(p, path, (size_t)(s - p + 1)));
	}
	char dllp[MAX_PATH]; lstrcpyA(dllp, path); lstrcatA(dllp, "actdll.dll");
	HMODULE dll = LoadLibraryA(dllp);
	void (*dllapi)(ActApi*) = dll ? (void (*)(ActApi*))(void*)GetProcAddress(dll, "actdll_api") : NULL;
	check("actdll.dll loads", dllapi != NULL);

	WNDCLASSA wc; memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = proc; wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "ActTest";
	RegisterClassA(&wc);
	HWND w = CreateWindowExA(0, "ActTest", "actest", WS_OVERLAPPEDWINDOW, 10, 10, 240, 160, NULL, NULL, wc.hInstance, NULL);
	check("the window is created", w != NULL);
	if (!w || !dllapi) { fprintf(gOut, "ACTTEST %d of %d as expected\n", gOk, gN); fclose(gOut); return 1; }
	ShowWindow(w, SW_SHOWNOACTIVATE);   // shown inactive, as visrun launches a game
	HWND other = CreateWindowExA(0, "ActTest", "actest other", WS_OVERLAPPEDWINDOW, 300, 10, 240, 160, NULL, NULL, wc.hInstance, NULL);
	ShowWindow(other, SW_SHOWNOACTIVATE);
	check("the window starts inactive", notmine(w));

	ActApi imp; ACT_FILL_FROM_IMPORTS(&imp);
	ActApi gpa; HMODULE u = GetModuleHandleA("user32.dll");
	gpa.SetForegroundWindow = (void*)GetProcAddress(u, "SetForegroundWindow"); gpa.SetActiveWindow = (void*)GetProcAddress(u, "SetActiveWindow");
	gpa.BringWindowToTop = (void*)GetProcAddress(u, "BringWindowToTop"); gpa.ShowWindow = (void*)GetProcAddress(u, "ShowWindow");
	gpa.SetWindowPos = (void*)GetProcAddress(u, "SetWindowPos"); gpa.BeginDeferWindowPos = (void*)GetProcAddress(u, "BeginDeferWindowPos");
	gpa.DeferWindowPos = (void*)GetProcAddress(u, "DeferWindowPos"); gpa.EndDeferWindowPos = (void*)GetProcAddress(u, "EndDeferWindowPos");
	gpa.ShowWindowAsync = (void*)GetProcAddress(u, "ShowWindowAsync"); gpa.SetWindowPlacement = (void*)GetProcAddress(u, "SetWindowPlacement");
	gpa.GetWindowPlacement = (void*)GetProcAddress(u, "GetWindowPlacement"); gpa.SwitchToThisWindow = (void*)GetProcAddress(u, "SwitchToThisWindow");
	ActApi dl; dllapi(&dl);

	const struct { const ActApi* a; const char* via; } paths[] = { { &imp, "import table" }, { &gpa, "GetProcAddress" }, { &dl, "second DLL" } };
	for (int i = 0; i < 3; i++) {
		if (allow) run_allowed(w, other, paths[i].a, paths[i].via);
		else run_blocked(w, paths[i].a, paths[i].via);
	}
	if (!allow) for (int i = 0; i < 3; i++) run_maximized(w, paths[i].a, paths[i].via);
	gWin32u = GetModuleHandleA("win32u.dll"); if (!gWin32u) gWin32u = LoadLibraryA("win32u.dll");
	gNtSetFg = gWin32u ? (BOOL (WINAPI *)(HWND))(void*)GetProcAddress(gWin32u, "NtUserSetForegroundWindow") : NULL;
	run_cbt(wc.hInstance, other, "cbt");
	SetEvent(gGo);   // the early thread goes on now that the stand-in is loaded
	if (early) { WaitForSingleObject(early, 30000); CloseHandle(early); }
	HANDLE late = CreateThread(NULL, 0, thread_window, (void*)"thread started after the stand-in loaded", 0, NULL);
	if (late) { WaitForSingleObject(late, 30000); CloseHandle(late); }
	// the cap of the live record: far more calls than net.log holds lines for
	if (!allow) for (int i = 0; i < 300; i++) SetForegroundWindow(w);
	DestroyWindow(w);
	fprintf(gOut, "ACTTEST %d of %d as expected\n", gOk, gN);
	fclose(gOut);
	return gOk == gN ? 0 : 1;
}
