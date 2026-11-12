#ifndef __SOFTWARESCREENDISPLAY_H__
#define __SOFTWARESCREENDISPLAY_H__

// port: the software screen of a 2D SexyApp port, as the original's DirectDraw path draws with 3D off. The screen image's
// own bits are the screen: every draw onto it goes through the framework's MemoryImage blitters (CheckDestination says no
// for the screen too, so DDImage's draws take their software branches), and SurfaceDisplay shows those bits, through the
// window's surface at 1x to 4x and through its renderer mode at any other size. A port opts in by its CreateDDInterface
// (under #ifndef __EMSCRIPTEN__) returning SoftwareScreenDisplay::Create(this); every other port, and the web build, keep
// DDInterface.
//
// What changes for a port that takes it:
//   - Its pictures come from the 2004 framework's software blitters, not from SDL's renderer.
//   - It has no 3D: mCan3D and mIs3D are false whatever the renderer could do (set again after every change of mode, which
//     sets them), so Is3DAccelerated() is false and the widget manager draws with fast stretch and no linear blend, as the
//     original's 2D mode. A port whose original offers 3D would need the screen handed back to the GPU path while 3D is on;
//     none that takes this display has that yet.
//   - Only the dirty widgets are drawn (SexyAppBase::DrawDirtyStuff marks no others), as the original's 1.x framework
//     draws: the screen keeps its pixels from one frame to the next.
//   - Each draw onto the screen tells the display what it covered (DDImage's software branches call ScreenDrawn); the
//     16x16 blocks drawn in a frame are joined into rects at the present (a run of blocks in a block row, runs of the same
//     columns in the rows below made one), and those rects are what SurfaceDisplay shows (their bounding rect past its
//     rect limit), the whole screen when the framework asks for it (the first frame, a change of mode,
//     ScreenInvalidated). A game that wrote into the screen's bits itself would need to say so (ScreenDrawn); none that
//     takes this display does. Comparing each frame with the last one shown instead cost Dynomite about 0.8 M cycles a
//     frame, near the original's whole present.
//   - A stretch onto the screen is drawn as the original's software screen draws one (mSoftwareStretch: the source
//     stretched into an image of the destination's size, then blitted), its fast stretch sampling pixel centres
//     (mFastStretchCentre), as the GPU path's nearest filter did. A game whose original samples otherwise sets the flags
//     after Create.
//   - A drawn cursor (mCursorImage) keeps SurfaceDisplay in renderer mode, as for any app of it.
// The screen's bits are read as they are: GetBits (ScreenOnCPU), ReadScreen and ReadScreenFull never read a display back.
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
	// the display a port opts into (DDInterface instead with SURFACE_DISPLAY=off in a test run)
	static DDInterface* Create(SexyAppBase* theApp);

	SoftwareScreenDisplay(SexyAppBase* theApp);
	~SoftwareScreenDisplay() override;

	int Init(SDL_Window* theWindow, bool vsync) override;
	void Cleanup() override;
	void UpdateDisplaySize() override;
	bool CheckDestination(DDImage* theImage) override;
	bool ScreenOnCPU() const override { return true; }
	void ScreenDrawn(const Rect& theRect) override;
	void BeginFrame() override;
	void Present() override;
	bool ReadScreen(ulong* theBits) override;
	bool ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight) override;

protected:
	// no 3D, whatever the renderer could do
	void No3D();
	// the screen image's bits (made, and cleared to black, the first time), or NULL before there is a screen image
	const uint32_t* ScreenPixels();
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
	// the runs of changed blocks of the block row above, still open (left block, right block, top row of the rect)
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
