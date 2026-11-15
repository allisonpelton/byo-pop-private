#ifndef __SOFTWARESCREENDISPLAY_H__
#define __SOFTWARESCREENDISPLAY_H__

// port: the software screen of a 2D SexyApp port, as the original's DirectDraw path draws with 3D off. The screen image's
// own bits are the screen: every draw onto it goes through the framework's MemoryImage blitters (CheckDestination says no
// for the screen too, so DDImage's draws take their software branches), and SurfaceDisplay shows those bits. A port opts
// in by its CreateDDInterface (under #ifndef __EMSCRIPTEN__) returning SoftwareScreenDisplay::Create(this); every other
// port, and the web build, keep DDInterface.
//
// Which sizes it draws in software:
//   - The game's own size (1x): always, through the window's surface. There it costs less than the GPU path, which pays
//     the driver for every present (Dynomite, 2026-11-12: 1.49 M cycles an update against 1.80, the original 1.63).
//   - 2x to 4x on Windows, where the port asks (Create's theScaledInSoftware): in software too, GDI stretching the changed
//     rects into the window (SurfaceDisplay, PresentScaledByGdi). The window then holds no Direct3D device, 40 to 44 MB for
//     Big Money's scripted minute at 2x against 67 to 70 MB on the GPU, but it costs about a sixth more processor time than
//     the GPU there: 18.6 G cycles for that run against 15.6, and Dynomite's minute 10.6 G against 9.1 (2026-11-15,
//     docs/MEMORY.md, "Windows at 2x to 4x"); the stretch itself costs little (Dynomite's software screen 9.8 G at 1x).
//     The picture is the 1x software screen's, each pixel repeated; the GPU path's differs from it where its blending
//     rounds otherwise.
//     SOFTSCREEN_SCALED=1 or 0 in a test run gives the one or the other whatever the port asked.
//   - Any other size, with an accelerated renderer (a full screen that is no whole multiple, or 2x to 4x without GDI or
//     without theScaledInSoftware): not.
//     SurfaceDisplay is asked for its renderer (WantedSurfaceScale gives 0), and once a renderer made on the window is
//     accelerated while the window is not at 1x, the screen goes to DDInterface's own GPU path for good (DrawOnGpu), as a
//     window that has had a hardware renderer never shows its surface again. A window that goes there keeps the GPU path at
//     1x after it. A minimised window changes nothing (UpdateDisplaySize waits for the restore): the size SDL reports for it
//     is not one the screen is shown at.
//   - Any other size, with no accelerated renderer (a machine with no GPU, where SDL's software renderer stretches every
//     sprite on the processor: Bejeweled 2 at 2x took 48.5% of a core): always. The surface shows it at 2x to 4x and
//     SurfaceDisplay's renderer mode (the frame stretched once) at any other size.
//
// What changes for a port that takes it, while its screen is drawn in software:
//   - Its pictures come from the 2004 framework's software blitters, not from SDL's renderer.
//   - It has no 3D: mIs3D is false whatever the renderer could do (set again after every change of mode, which sets it),
//     so Is3DAccelerated() is false and the widget manager draws with fast stretch and no linear blend, as the original's
//     2D mode. On the GPU path the flags are the renderer's, as DDInterface's, unless the app wants 2D (mAppWants3D: a
//     game whose original starts in 2D, or 3D switched off).
//   - 3D, for a port whose original offers it (Create's theOffers3D): mCan3D is true while an accelerated renderer may be
//     had (until one made on the window turns out not to be), so the Options dialog's box can be ticked, and while the
//     app wants 3D (NoteWant3D: the saved choice at every display made, a change in the Options) the screen is handed to
//     the GPU path, at any size, for good: a window that has had a hardware renderer never shows its surface again, so 3D
//     switched off later leaves the screen on the GPU with mIs3D false (the GPU path's 2D mode, as DDInterface's) until
//     the next run starts in software. Without theOffers3D, mCan3D is false as well and the app's choice changes nothing.
//   - Only the dirty widgets are drawn (SexyAppBase::DrawDirtyStuff marks no others), as the original's 1.x framework
//     draws: the screen keeps its pixels from one frame to the next. A hand-over to the GPU starts its screen texture
//     from the last frame drawn in software, so that a screen kept between frames (mKeepScreen) loses nothing.
//   - What goes to the window is what was drawn, never found by comparing frames: each draw onto the screen marks the
//     16x16 blocks it covered (DDImage's software branches call ScreenDrawn with the draw's rect, bounded by its clip),
//     and at the present the blocks marked in the frame are joined into rects (a run of blocks in a block row, runs of
//     the same columns in the rows below made one). Those rects are what SurfaceDisplay shows (their bounding rect past
//     its rect limit), the whole screen when the framework asks for it (the first frame, a change of mode,
//     ScreenInvalidated). A game that wrote into the screen's bits itself would need to say so (ScreenDrawn); none that
//     takes this display does.
//   - A stretch onto the screen is drawn as the original's software screen draws one (mSoftwareStretch: the source
//     stretched into an image of the destination's size, then blitted), its fast stretch sampling pixel centres
//     (mFastStretchCentre), as the GPU path's nearest filter did. A game whose original samples otherwise sets the flags
//     after Create, and they hold through every change of mode (a time on the GPU, which samples by its own filter, puts
//     the game's choice back when the screen returns). The stretched images are kept while they are drawn
//     (DDInterface::GetStretchCopy); those not drawn in the last two frames go after each present, as the original kept
//     none.
//   - A drawn cursor (mCursorImage, a game's own cursors) is the renderer's to draw, as for any app of SurfaceDisplay: at
//     1x the window takes a renderer for it and keeps it (with a GPU SurfaceDisplay keeps it, mWindowHadRenderer; with
//     none, mCursorSeen), and the screen stays in software, its frames shown through the renderer's frame texture with
//     the cursor drawn over them. Only a window not at 1x hands the screen to the GPU.
// The screen's bits are black when they are made, whoever asks for them first (ScreenPixels, asked after Init and every
// change of mode, before anything draws).
// The screen's bits are read as they are: GetBits (ScreenOnCPU), ReadScreen and ReadScreenFull never read a display back
// (on the GPU path, DDInterface's own).
//
// Test runs only (SexyAppBase::IsTestRun): SURFACE_DISPLAY=off makes Create return DDInterface, the GPU path, so that a run
// can measure the two against each other; SOFTSCREEN_STATS=1 counts the cycles spent making the rects (QueryThreadCycleTime)
// and logs them with the draws noted and the area presented when the display goes.

#include "SurfaceDisplay.h"

#include <vector>

namespace Sexy
{

class SoftwareScreenDisplay : public SurfaceDisplay
{
public:
	// the display a port opts into (DDInterface instead with SURFACE_DISPLAY=off in a test run); theOffers3D: the port's
	// original has a 3D mode, which takes the screen to the GPU while it is on; theScaledInSoftware: a window of 2x to 4x
	// keeps the screen in software where GDI can stretch it (Windows), for no Direct3D device at the cost of the software
	// draws' processor time (below)
	static DDInterface* Create(SexyAppBase* theApp, bool theOffers3D = false, bool theScaledInSoftware = false);

	SoftwareScreenDisplay(SexyAppBase* theApp, bool theOffers3D = false, bool theScaledInSoftware = false);
	~SoftwareScreenDisplay() override;

	int Init(SDL_Window* theWindow, bool vsync) override;
	void Cleanup() override;
	void UpdateDisplaySize() override;
	bool CheckDestination(DDImage* theImage) override;
	bool ScreenOnCPU() const override { return !mGpuScreen; }
	void ScreenDrawn(const Rect& theRect) override;
	void BeginFrame() override;
	void Present() override;
	void NoteWant3D(bool theWant3D) override;
	bool ReadScreen(ulong* theBits) override;
	bool ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight) override;
	// the window as shown: on the GPU, the screen texture the screen is drawn into (the buffer is not kept up to date there)
	bool ReadWindow(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight) override;

	// true once the screen is drawn on the GPU (DDInterface's path), for good
	bool DrawsOnGpu() const { return mGpuScreen; }

protected:
	int WantedSurfaceScale() override;
	bool WantsStretch() override;
	// the software screen's settings (a new display, or after Cleanup): no 3D, the software stretch
	void SoftwareScreen();
	// no 3D, whatever the renderer could do
	void No3D();
	// after SurfaceDisplay has chosen a mode: a renderer made on the window says whether the screen goes to the GPU
	void NoteRenderer();
	// the window is the screen's own size (1x)
	bool WindowAtScreenSize();
	// the screen goes to DDInterface's GPU path for good (a renderer made on the window is accelerated)
	void DrawOnGpu(const char* theRendererName);
	bool mGpuScreen;      // the screen is drawn on the GPU, DDInterface's path
	bool mOffers3D;       // the port's original has a 3D mode (Create's theOffers3D)
	bool mWants3D;        // the app wants 3D (NoteWant3D), so the screen goes to the GPU where it can
	bool mSoftwareFastStretchCentre;  // mFastStretchCentre as the software screen had it when the GPU took the screen, put back when it returns
	bool mScaledSurface;  // the surface shows the software screen at 2x to 4x with a GPU too: Create's theScaledInSoftware where GDI stretches (SOFTSCREEN_SCALED=0|1 in a test run decides)
	bool mNoGpuRenderer;  // the window's renderer is not accelerated: the screen is drawn in software at every size
	bool mChangingMode;   // inside SurfaceDisplay's change of mode, whose DDInterface::Init asks UpdateDisplaySize again
	bool mCursorSeen;     // the app has drawn a cursor: the window keeps the renderer it took for it
	bool mCursorNoted;    // the log has said that the software screen goes through an accelerated renderer at 1x
	int mGpuPresents;
	// mStretchUse when this frame and the one before it began: the stretched copies drawn in neither go after the present
	unsigned int mStretchUseThisFrame;
	unsigned int mStretchUseLastFrame;
	// the screen image's bits (made, and cleared to black the first time they are seen), or NULL before there is a screen
	// image
	const uint32_t* ScreenPixels();
	const ulong* mClearedBits;  // the bits ScreenPixels last cleared
	// hands SurfaceDisplay the rects of the blocks drawn since the last present (or the whole screen) and clears them
	void HandOverDrawn(const uint32_t* thePixels);

	// the next present shows the whole screen (a new display or screen)
	bool mShowWhole;
	// the 16x16 blocks drawn since the last present, a byte each, row by row
	std::vector<uint8_t> mDrawnBlocks;
	int mBlocksWide;
	int mBlocksHigh;
	bool mAnyDrawn;
	std::vector<Rect> mDrawnRects;
	// the runs of drawn blocks of the block row above, still open (left block, right block, top row of the rect)
	struct OpenRun
	{
		int mLeft;
		int mRight;
		int mTop;
	};
	std::vector<OpenRun> mOpenRuns;
	std::vector<OpenRun> mNextRuns;

	// SOFTSCREEN_STATS=1 in a test run: the cycles making rects, the draws noted and what was handed over
	bool mStats;
	unsigned long long mStatCycles;
	int mStatFrames;
	int mStatWholes;
	long long mStatDraws;
	long long mStatRects;
	long long mStatArea;
};

}

#endif
