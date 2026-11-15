#ifndef __SURFACEDISPLAYGDI_H__
#define __SURFACEDISPLAYGDI_H__

// port: Windows' GDI as SurfaceDisplay's stretch mode uses it, kept in a file of its own so that windows.h is seen by nothing else in the
// framework. It shows a 32-bit screen buffer in a window of any size, by StretchDIBits with the nearest-neighbour mode (COLORONCOLOR): no
// renderer, no Direct3D device and no thread of a driver. Only the changed rects are painted: the whole buffer is stretched into the
// presentation rect with the changed rects (in window pixels) as the clip region, which puts every pixel exactly where the stretch of
// the whole buffer puts it (stretching a rect of the buffer alone does not at a size that is no whole scale: a third of the pixels of
// a 1.6x stretch differ; tools/display/present/stretchprobe.c measured it).

#ifdef _WIN32

#include <SDL.h>
#include <stdint.h>
#include <vector>

namespace Sexy
{

class GdiStretch
{
public:
	GdiStretch();
	~GdiStretch();

	// the window's handle; false when SDL has none for this window. theShadow: a test keeps a bitmap of the window's size that every present
	// paints the same way, for CheckShadow
	bool Attach(SDL_Window* theWindow, bool theShadow);
	void Detach();

	// Waits for the desktop compositor's next composition (DwmFlush, dwmapi.dll looked up at the first call), as a vsync present
	// waits for the vertical blank (SurfaceDisplay's SURFACE_PACE=dwm). False when DWM cannot be waited for.
	static bool WaitForComposition();

	// Paints a frame: thePixels, thePitch bytes a row, theWidth x theHeight 32-bit pixels 0x??RRGGBB (the top byte is not read), stretched
	// into theDest (window pixels, the screen's place in the window). theRects: the parts of theDest to paint (window pixels; a part
	// outside theDest is not painted); theCount 0: all of theDest, and the bars of the window of theWindowWidth x theWindowHeight pixels
	// around it black. False when the window's device context cannot be had.
	bool Present(const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest, const SDL_Rect* theRects,
		int theCount, int theWindowWidth, int theWindowHeight);

	// Test: the window as GDI reads it back after the present just made (its own pixels, not the bitmap's) against the window painted again
	// whole and read back (the window must be whole on the screen, nothing over it: a window of rundesk's desktop is). False at the first
	// pixel that differs, with where and the two colours. It paints the window whole, so a test only.
	bool CheckWindow(const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest, int theWindowWidth,
		int theWindowHeight, int* theX, int* theY, uint32_t* theShown, uint32_t* theWanted);

	// Test: the shadow bitmap against the whole of the frame stretched afresh. False at the first pixel that differs, with where and the two
	// colours.
	bool CheckShadow(const uint32_t* thePixels, int thePitch, int theWidth, int theHeight, const SDL_Rect& theDest, int theWindowWidth,
		int theWindowHeight, int* theX, int* theY, uint32_t* theShown, uint32_t* theWanted);

private:
	struct Impl;
	Impl* mImpl;
};

}

#endif

#endif
