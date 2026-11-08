// fwtest's look at the native window through Win32 (winstate.cpp), kept apart from fwtest.cpp: the framework's
// PortCompat.h defines DWORD and LONG its own way, and <windows.h> cannot be included beside it.

#ifndef __FWTEST_WINSTATE_H__
#define __FWTEST_WINSTATE_H__

struct SDL_Window;

struct FwWindowState
{
	bool mBorderless = false;     // WS_POPUP, no caption, no sizing frame
	bool mTopmost = false;        // WS_EX_TOPMOST
	bool mCoversMonitor = false;  // the window's rect is its monitor's
	bool mCursorFree = false;     // the cursor's clip is the whole virtual screen
};

// Resizes the window as a drag of its frame would (SetWindowPos, which DefWindowProc checks against WM_GETMINMAXINFO).
void FwResizeWindow(SDL_Window* theWindow, int theWidth, int theHeight);
// False off Windows, or when the window has no HWND.
bool FwGetWindowState(SDL_Window* theWindow, FwWindowState* theState);

#endif
