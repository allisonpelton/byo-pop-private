#ifndef __RETRO64_R64DRAW_H__
#define __RETRO64_R64DRAW_H__

// Retro64's draw list (0x421070..0x4214a0 (cosmicbugs), 0x421550..0x421980 (waterbugs)) and dirty rectangles
// (0x4214b0..0x421e00 (cosmicbugs), 0x421990..0x4222e0 (waterbugs)). The game queues its sprites
// with DrawList_Add and DrawList_Flush draws them by depth; the dirty grid records what changed in the back buffer
// so only that is copied to the screen.

#include "R64.h"

#include <cstddef>

namespace r64
{

// 0x44 bytes, 2000 of them at 0x6e6540 (cosmicbugs), 0x6e65c0 (waterbugs)
struct DrawItem
{
	Image* mImage;      // +0x00
	int mX;             // +0x04
	int mY;             // +0x08
	int mDepth;         // +0x0c  drawn in increasing depth
	int mKind;          // +0x10  0 blit, 1 alpha, 2 blend, 3 fill, 4 alpha faded, 5 fill faded, 6 additive
	uint8_t* mDst;      // +0x14
	int mDstPitch;        // +0x18
	int mR;             // +0x1c  the fill colour (its low bytes)
	int mG;             // +0x20
	int mB;             // +0x24
	int mAlpha;         // +0x28
	Palette* mPalette;  // +0x2c  the image's palette while it is drawn
	uint32_t mSeq;      // +0x30  orders equal depths before the final sort
	int mClip[4];       // +0x34
};

// 0x421070 (cosmicbugs), 0x421550 (waterbugs): queues a sprite (null clip: the whole screen); nothing past 2000
void DrawList_Add(Image* theImage, uint32_t theSeq, int x, int y, int theDepth, int theKind, uint8_t* theDst,
	int theDPitch, int theRed, int theGreen, int theBlue, int theAlpha, Palette* thePalette, const int* theClip);
// 0x4211d0 (cosmicbugs), 0x4216b0 (waterbugs): sorts by depth and draws with the screen's format
void DrawList_Flush();
// 0x4214a0 (cosmicbugs), 0x421980 (waterbugs)
void DrawList_Clear();

// 0x34 bytes (gDirty at 0x472a7c (cosmicbugs), 0x472a7c (waterbugs)): the 800x600 screen in cells, each with its
// changed rectangles (mode 1: one bounding rectangle per cell; else up to mMaxRects, merged where that does not grow
// them)
struct Dirty
{
	struct Cell
	{
		int* mRects;    // {l, t, r, b} inclusive, mMaxRects of them
		int mCount;
	};

	Cell* mCells;       // +0x00
	int mCellW;         // +0x04
	int mCellH;         // +0x08
	int mCols;          // +0x0c
	int mRows;          // +0x10
	int mMode;          // +0x14
	int mMaxRects;      // +0x18
	Image* mSource;     // +0x1c  the back buffer
	int mSrcRect[4];    // +0x20
	int mBytesPerPixel; // +0x30

	Dirty();  // 0x421b50 (cosmicbugs), 0x422030 (waterbugs)
	~Dirty(); // 0x4214b0 (cosmicbugs), 0x421990 (waterbugs)
	Dirty(const Dirty&) = delete;
	Dirty& operator=(const Dirty&) = delete;
};

// 0x4214b0 (cosmicbugs), 0x421990 (waterbugs)
void Dirty_Free(Dirty* theDirty);
// 0x421500 (cosmicbugs), 0x4219e0 (waterbugs)
void Dirty_Init(Dirty* theDirty, int theCellW, int theCellH, int theMaxRects, int theMode);
// 0x421540 (cosmicbugs), 0x421a20 (waterbugs)
void Dirty_AddRect(Dirty* theDirty, int x, int y, int theWidth, int theHeight);
// 0x421960 (cosmicbugs), 0x421e40 (waterbugs)
void Dirty_Clear(Dirty* theDirty);
// 0x4219d0 (cosmicbugs), 0x421eb0 (waterbugs)
void Dirty_AddClipped(Dirty* theDirty, int x, int y, int theWidth, int theHeight, const int* theClip);
// 0x421a60 (cosmicbugs), 0x421f40 (waterbugs)
void Dirty_CopyTo(Dirty* theDirty, uint8_t* theDst, int theDPitch);
// 0x421b80 (cosmicbugs), 0x422060 (waterbugs)
void Dirty_Alloc(Dirty* theDirty);
// 0x421bf0 (cosmicbugs), 0x4220d0 (waterbugs)
void Dirty_SetSource(Dirty* theDirty, Image* theSource);
// 0x421c20 (cosmicbugs), 0x422100 (waterbugs), 0x421cd0 (cosmicbugs), 0x4221b0 (waterbugs), 0x421d80 (cosmicbugs),
// 0x422260 (waterbugs): clip, tell the screen, add
void Dirty_AddClippedRect(Dirty* theDirty, const int* theRect, const int* theClip);
void Dirty_AddClippedShown(Dirty* theDirty, int x, int y, int theWidth, int theHeight, const int* theClip);
void Dirty_AddClippedImage(Dirty* theDirty, int x, int y, const Image* theImage, const int* theClip);

// 0x444c50 (cosmicbugs), 0x445090 (waterbugs) (and shortsort 0x444bc0 (cosmicbugs), 0x445000 (waterbugs)): the MSVC
// 2005 CRT's qsort, which DrawList_Flush relies on: its comparator
// never answers 0, so the order of equal depths is whatever this algorithm leaves
void CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*));

// R64Line.cpp. 0x42cd10 (cosmicbugs), 0x42d1f0 (waterbugs): the surface lines go to (clipped to all of it; a null
// format keeps the last one)
void Draw_SetTarget(uint8_t* theBits, int thePitch, int theW, int theH, const PixelFormat* theFmt);
// 0x42d820 (cosmicbugs), 0x42dd00 (waterbugs): a line blended toward (r, g, b) by theAlpha/256, both ends drawn; 16-bit
// surfaces only
void Draw_Line(int theX0, int theY0, int theX1, int theY1, int theRed, int theGreen, int theBlue, int theAlpha);
// 0x42d630 (cosmicbugs), 0x42db10 (waterbugs): a rectangle (inclusive) in a colour (black clears it)
void Draw_Fill(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int l, int t, int theRight, int theBottom,
	int theR, int theG, int theB);
// 0x42d670 (cosmicbugs), 0x42db50 (waterbugs): a rectangle (inclusive) blended toward a colour by theAlpha/256,
// unclipped
void Draw_FillBlend(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int l, int t, int r, int b, int theR,
	int theG, int theB, int theAlpha);
// 0x42d730 (cosmicbugs), 0x42dc10 (waterbugs): the same on the surface lines go to (Draw_SetTarget)
void Draw_FillBlendTarget(int l, int t, int r, int b, int theR, int theG, int theB, int theAlpha);
// 0x42d6b0 (cosmicbugs), 0x42db90 (waterbugs): the same on the part inside theClip {l, t, r, b}
void Draw_FillBlendClipped(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, const int* theClip, int l, int t,
	int r, int b, int theR, int theG, int theB, int theAlpha);
// 0x42d6f0 (cosmicbugs), 0x42dbd0 (waterbugs): Draw_Line's drawing on a given surface, unclipped
void Draw_LineOn(uint8_t* theBits, int thePitch, const PixelFormat* theFmt, int theX0, int theY0, int theX1, int theY1,
	int r, int g, int b, int theAlpha);

} // namespace r64

#endif
