#include "winstate.h"

#include <SDL.h>

#if defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <SDL_syswm.h>

static HWND GetHwnd(SDL_Window* theWindow)
{
	SDL_SysWMinfo anInfo;
	SDL_VERSION(&anInfo.version);
	return SDL_GetWindowWMInfo(theWindow, &anInfo) ? anInfo.info.win.window : NULL;
}

void FwResizeWindow(SDL_Window* theWindow, int theWidth, int theHeight)
{
	HWND aWnd = GetHwnd(theWindow);
	if (aWnd != NULL)
		SetWindowPos(aWnd, NULL, 0, 0, theWidth, theHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool FwGetWindowState(SDL_Window* theWindow, FwWindowState* theState)
{
	HWND aWnd = GetHwnd(theWindow);
	if (aWnd == NULL)
		return false;
	const LONG aStyle = GetWindowLongW(aWnd, GWL_STYLE);
	const LONG anExStyle = GetWindowLongW(aWnd, GWL_EXSTYLE);
	theState->mBorderless = ((aStyle & (WS_CAPTION | WS_THICKFRAME)) == 0) && ((aStyle & WS_POPUP) != 0);
	theState->mTopmost = (anExStyle & WS_EX_TOPMOST) != 0;
	RECT aRect;
	MONITORINFO aMonitor;
	aMonitor.cbSize = sizeof(aMonitor);
	GetWindowRect(aWnd, &aRect);
	theState->mCoversMonitor = GetMonitorInfoW(MonitorFromWindow(aWnd, MONITOR_DEFAULTTONEAREST), &aMonitor) &&
		EqualRect(&aRect, &aMonitor.rcMonitor);
	RECT aClip;
	GetClipCursor(&aClip);
	const int aLeft = GetSystemMetrics(SM_XVIRTUALSCREEN), aTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
	theState->mCursorFree = (aClip.left <= aLeft) && (aClip.top <= aTop) &&
		(aClip.right >= aLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN)) && (aClip.bottom >= aTop + GetSystemMetrics(SM_CYVIRTUALSCREEN));
	return true;
}
#else
void FwResizeWindow(SDL_Window*, int, int)
{
}

bool FwGetWindowState(SDL_Window*, FwWindowState*)
{
	return false;
}
#endif
