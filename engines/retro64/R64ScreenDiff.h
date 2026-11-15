#ifndef __R64SCREENDIFF_H__
#define __R64SCREENDIFF_H__

// port: what a host shows of the engine's 800x600 16-bit screen (one copy for Cosmic Bugs and Water Bugs). The engine says what it copies from its back buffer
// (Sys_AddUpdateRect), not everything it draws: sprites, beams, text and fades are drawn straight into the screen, and some
// of their pixels lie outside every rectangle it reported. So the changes are found in the screen itself: ScreenDiff keeps a
// copy of the screen as it last showed it, compares a row at a time and the rows that differ a block at a time, widens just
// the blocks that differ into the host's 32-bit pixels, and says which rectangles those were, for a display that takes a dirty
// rectangle (framework/SurfaceDisplay.h) or an image that is uploaded in parts. After every Update the host's pixels equal a
// widening of the whole screen (a check on every drawn frame of long plays is described in games/cosmicbugs/docs/README.md,
// "The processor-time target").
//
// It holds no framework type, so that it stays in the engine's library.

#include "R64.h"

#include <cstdint>
#include <vector>

namespace r64
{

// A rectangle of the screen's pixels
struct ChangedRect
{
	int mX, mY, mWidth, mHeight;
};

class ScreenDiff
{
public:
	// the unit of the comparison (pixels square), and how far apart (in blocks) two runs of changed blocks in a row may be and
	// still be one rectangle
	static constexpr int BLOCK = 16;
	static constexpr int GAP = 2;
	static constexpr int COLUMNS = (SCREEN_WIDTH + BLOCK - 1) / BLOCK;
	static constexpr int ROWS = (SCREEN_HEIGHT + BLOCK - 1) / BLOCK;

	ScreenDiff();

	// theScreen: the engine's screen (RGB565). theDestination: the same size in 32-bit pixels 0xffRRGGBB (565 widened with each
	// channel's high bits repeated into the low ones), which hold the screen as the last call left it. What differs from the
	// last call's screen is widened into it, and that screen is kept. The first call widens the whole screen.
	void Update(const uint16_t* theScreen, uint32_t* theDestination);

	// the rectangles the last Update widened, and whether it was the whole screen (the first one: no rectangles)
	const std::vector<ChangedRect>& Rects() const { return mRects; }
	bool Whole() const { return mWhole; }

private:
	void FindChangedBlocks(const uint16_t* theScreen);
	void CollectChangedRects();

	std::vector<uint16_t> mCopiedScreen;
	uint8_t mChangedBlocks[ROWS][COLUMNS];
	std::vector<ChangedRect> mRects;
	bool mWhole;
};

} // namespace r64

#endif
