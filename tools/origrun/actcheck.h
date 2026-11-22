// The activation hook's table of functions, shared by actest.c (the exe: calls through its imports and through GetProcAddress)
// and actdll.c (a second module that fills the table from its own imports).
#include <windows.h>
typedef struct {
	BOOL (WINAPI *SetForegroundWindow)(HWND);
	HWND (WINAPI *SetActiveWindow)(HWND);
	BOOL (WINAPI *BringWindowToTop)(HWND);
	BOOL (WINAPI *ShowWindow)(HWND, int);
	BOOL (WINAPI *SetWindowPos)(HWND, HWND, int, int, int, int, UINT);
	HDWP (WINAPI *BeginDeferWindowPos)(int);
	HDWP (WINAPI *DeferWindowPos)(HDWP, HWND, HWND, int, int, int, int, UINT);
	BOOL (WINAPI *EndDeferWindowPos)(HDWP);
	BOOL (WINAPI *ShowWindowAsync)(HWND, int);
	BOOL (WINAPI *SetWindowPlacement)(HWND, const WINDOWPLACEMENT*);
	BOOL (WINAPI *GetWindowPlacement)(HWND, WINDOWPLACEMENT*);
	void (WINAPI *SwitchToThisWindow)(HWND, BOOL);
} ActApi;
#define ACT_FILL_FROM_IMPORTS(a) do { (a)->SetForegroundWindow = SetForegroundWindow; (a)->SetActiveWindow = SetActiveWindow; \
	(a)->BringWindowToTop = BringWindowToTop; (a)->ShowWindow = ShowWindow; (a)->SetWindowPos = SetWindowPos; \
	(a)->BeginDeferWindowPos = BeginDeferWindowPos; (a)->DeferWindowPos = DeferWindowPos; (a)->EndDeferWindowPos = EndDeferWindowPos; \
	(a)->ShowWindowAsync = ShowWindowAsync; (a)->SetWindowPlacement = SetWindowPlacement; (a)->GetWindowPlacement = GetWindowPlacement; (a)->SwitchToThisWindow = SwitchToThisWindow; } while (0)
