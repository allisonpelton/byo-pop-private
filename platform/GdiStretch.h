// GDI's own stretch of a frame into a window, and the wait for the desktop compositor: framework/SurfaceDisplay.cpp's window
// surface on Windows (docs/MEMORY.md, "Windows at 2x to 4x"). A file of its own because it needs the real Windows headers,
// which the framework's own Windows types clash with.

#ifndef PLATFORM_GDISTRETCH_H
#define PLATFORM_GDISTRETCH_H

#include <stdint.h>

struct SDL_Window;
struct SDL_Rect;

namespace Platform
{

// Stretches each of theRects (in the frame's pixels) of the frame thePixels (theWidth x theHeight 32-bit 0x??RRGGBB pixels,
// thePitch bytes a row, top row first) into the window at theScale times its place and size, one StretchDIBits each with
// nearest sampling (COLORONCOLOR). theDiffering, when not NULL: the window's pixels are read back afterwards and the number
// that are not the frame's at their place is put there. False when the window has no device context (not Windows) or a
// stretch failed.
bool StretchToWindow(SDL_Window* theWindow, const uint32_t* thePixels, int thePitch, int theWidth, int theHeight,
	const SDL_Rect* theRects, int theCount, int theScale, int* theDiffering);

// Waits for the desktop compositor's next composition (DwmFlush, from dwmapi.dll, loaded at the first call), so that a frame
// written to a window just now is shown at the next vertical blank and the next one is not drawn before it (SurfaceDisplay's
// pace with SURFACE_PACE=dwm). False when there is no compositor to wait for (not Windows, or DwmFlush failed), and the caller
// paces itself.
bool WaitForComposition();

}

#endif
