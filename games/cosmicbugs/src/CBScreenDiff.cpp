// port: see CBScreenDiff.h.

#include "CBScreenDiff.h"

#include <cstring>

namespace cb
{

namespace
{

// The 16-bit screen's pixels as 32 bits (565 widened with shifts: each channel's high bits repeated into the low ones), made
// once; an alpha of 0xff on every one
uint32_t gPixelsFromScreen16[65536];
bool gPixelsFromScreen16Built = false;

void BuildPixelsFromScreen16()
{
	for (uint32_t p = 0; p < 65536; p++)
	{
		uint32_t aRed = (p >> 11) & 0x1f;
		uint32_t aGreen = (p >> 5) & 0x3f;
		uint32_t aBlue = p & 0x1f;
		gPixelsFromScreen16[p] = 0xff000000 | (((aRed << 3) | (aRed >> 2)) << 16) |
			(((aGreen << 2) | (aGreen >> 4)) << 8) | ((aBlue << 3) | (aBlue >> 2));
	}
	gPixelsFromScreen16Built = true;
}

static_assert(r64::SCREEN_WIDTH % ScreenDiff::BLOCK == 0, "a row has whole blocks");
static_assert(ScreenDiff::BLOCK % 8 == 0, "a block is whole 16-byte vectors");

// Sixteen bytes (eight pixels) at a time, in the compiler's vector type (one SSE2 or WebAssembly SIMD register, else
// plain words), loaded from any address
typedef uint32_t PixelVector __attribute__((vector_size(16), aligned(4), may_alias));

// Whether two runs of the screen's pixels (a multiple of eight of them) are the same
bool SamePixels(const uint16_t* theFirst, const uint16_t* theSecond, int theCount)
{
	const PixelVector* aFirst = reinterpret_cast<const PixelVector*>(theFirst);
	const PixelVector* aSecond = reinterpret_cast<const PixelVector*>(theSecond);
	PixelVector aDifference = {0, 0, 0, 0};
	for (int i = 0; i < theCount / 8; i++)
		aDifference |= aFirst[i] ^ aSecond[i];
	return (aDifference[0] | aDifference[1] | aDifference[2] | aDifference[3]) == 0;
}

} // namespace

ScreenDiff::ScreenDiff()
{
	if (!gPixelsFromScreen16Built)
		BuildPixelsFromScreen16();
	std::memset(mChangedBlocks, 0, sizeof(mChangedBlocks));
	mWhole = false;
}

// The blocks of the screen that differ from the copy: a row at a time, and the rows that differ a block at a time
void ScreenDiff::FindChangedBlocks(const uint16_t* theScreen)
{
	std::memset(mChangedBlocks, 0, sizeof(mChangedBlocks));
	for (int y = 0; y < r64::SCREEN_HEIGHT; y++)
	{
		const uint16_t* aNow = theScreen + y * r64::SCREEN_WIDTH;
		const uint16_t* aBefore = mCopiedScreen.data() + y * r64::SCREEN_WIDTH;
		if (SamePixels(aNow, aBefore, r64::SCREEN_WIDTH))
			continue;
		for (int aColumn = 0; aColumn < COLUMNS; aColumn++)
		{
			int aLeft = aColumn * BLOCK;
			if (!SamePixels(aNow + aLeft, aBefore + aLeft, BLOCK))
				mChangedBlocks[y / BLOCK][aColumn] = 1;
		}
	}
}

// The rectangles to widen: each row of changed blocks gives its runs (runs a gap of at most GAP blocks apart are one), and a
// run of the same extent under another joins it, so that a frame is a few dozen rectangles
void ScreenDiff::CollectChangedRects()
{
	static std::vector<int> anOpen;      // the rectangles that end at the row above
	static std::vector<int> aNowOpen;    // the rectangles that end at this row
	mRects.clear();
	anOpen.clear();
	for (int aRow = 0; aRow < ROWS; aRow++)
	{
		aNowOpen.clear();
		int aTop = aRow * BLOCK;
		int aBottom = aTop + BLOCK < r64::SCREEN_HEIGHT ? aTop + BLOCK : r64::SCREEN_HEIGHT;
		int aColumn = 0;
		while (aColumn < COLUMNS)
		{
			if (mChangedBlocks[aRow][aColumn] == 0)
			{
				aColumn++;
				continue;
			}
			int aRunFirst = aColumn;
			int aRunLast = aColumn;
			for (int aNext = aColumn + 1; aNext < COLUMNS && aNext <= aRunLast + 1 + GAP; aNext++)
			{
				if (mChangedBlocks[aRow][aNext] != 0)
					aRunLast = aNext;
			}
			aColumn = aRunLast + 1;
			int aLeft = aRunFirst * BLOCK;
			int aRight = (aRunLast + 1) * BLOCK;
			int aRect = -1;
			for (int i : anOpen)
			{
				const ChangedRect& aAbove = mRects[i];
				if (aAbove.mX == aLeft && aAbove.mX + aAbove.mWidth == aRight)
				{
					aRect = i;
					mRects[i].mHeight = aBottom - aAbove.mY;
					break;
				}
			}
			if (aRect < 0)
			{
				aRect = (int)mRects.size();
				ChangedRect aNew = {aLeft, aTop, aRight - aLeft, aBottom - aTop};
				mRects.push_back(aNew);
			}
			aNowOpen.push_back(aRect);
		}
		anOpen.swap(aNowOpen);
	}
}

void ScreenDiff::Update(const uint16_t* theScreen, uint32_t* theDestination)
{
	if (mCopiedScreen.empty())
	{
		// the first picture is the whole screen
		for (int i = 0; i < r64::SCREEN_WIDTH * r64::SCREEN_HEIGHT; i++)
			theDestination[i] = gPixelsFromScreen16[theScreen[i]];
		mCopiedScreen.assign(theScreen, theScreen + r64::SCREEN_WIDTH * r64::SCREEN_HEIGHT);
		mRects.clear();
		mWhole = true;
		return;
	}
	mWhole = false;
	FindChangedBlocks(theScreen);
	CollectChangedRects();
	for (const ChangedRect& aRect : mRects)
	{
		for (int y = aRect.mY; y < aRect.mY + aRect.mHeight; y++)
		{
			const uint16_t* aSource = theScreen + y * r64::SCREEN_WIDTH;
			uint32_t* aDestination = theDestination + y * r64::SCREEN_WIDTH;
			for (int x = aRect.mX; x < aRect.mX + aRect.mWidth; x++)
				aDestination[x] = gPixelsFromScreen16[aSource[x]];
			std::memcpy(&mCopiedScreen[y * r64::SCREEN_WIDTH + aRect.mX], aSource + aRect.mX, aRect.mWidth * 2);
		}
	}
}

} // namespace cb
