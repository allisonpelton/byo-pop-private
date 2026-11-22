// The activation hook (tools/origrun/activation.c) from end to end, for check.sh's origrun suite (tools/origrun/actest.sh). The
// stand-in is loaded the way a game's import of dsound.dll loads it; the exe shows a window without activating it, then calls
// SetForegroundWindow, SetActiveWindow, BringWindowToTop, ShowWindow with every command, SetWindowPos and DeferWindowPos on
// it, through its imports, through GetProcAddress and from a second DLL (actdll.dll). After each call neither
// GetForegroundWindow() nor GetActiveWindow() may be the window, and the window must be in the state the mapped command gives.
// With ORIGRUN_ACTIVATION=allow in the environment the calls must instead reach Windows: the window becomes the active one.
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

int main(void)
{
	char path[MAX_PATH], outp[MAX_PATH], tmp[40];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	char* l = strrchr(path, '\\'); if (l) l[1] = 0;
	lstrcpyA(outp, path); lstrcatA(outp, "actest.out");
	gOut = fopen(outp, "w");
	if (!gOut) return 2;
	int allow = GetEnvironmentVariableA("ORIGRUN_ACTIVATION", tmp, sizeof(tmp)) && !lstrcmpiA(tmp, "allow");

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
	DestroyWindow(w);
	fprintf(gOut, "ACTTEST %d of %d as expected\n", gOk, gN);
	fclose(gOut);
	return gOk == gN ? 0 : 1;
}
