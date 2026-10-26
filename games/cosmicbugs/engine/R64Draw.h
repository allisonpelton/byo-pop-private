#ifndef __COSMICBUGS_R64DRAW_H__
#define __COSMICBUGS_R64DRAW_H__

// Retro64's draw list (0x421070..0x4214a0) and dirty rectangles (0x4214b0..0x421e00). The game queues its sprites
// with DrawList_Add and DrawList_Flush draws them by depth; the dirty grid records what changed in the back buffer
// so only that is copied to the screen.

#include "R64.h"

#include <cstddef>

namespace r64
{

// 0x44 bytes, 2000 of them at 0x6e6540
struct DrawItem
{
	Image* image;      // +0x00
	int x;             // +0x04
	int y;             // +0x08
	int depth;         // +0x0c  drawn in increasing depth
	int kind;          // +0x10  0 blit, 1 alpha, 2 blend, 3 fill, 4 alpha faded, 5 fill faded, 6 additive
	uint8_t* dst;      // +0x14
	int dpitch;        // +0x18
	int r;             // +0x1c  the fill colour (its low bytes)
	int g;             // +0x20
	int b;             // +0x24
	int alpha;         // +0x28
	Palette* palette;  // +0x2c  the image's palette while it is drawn
	uint32_t seq;      // +0x30  orders equal depths before the final sort
	int clip[4];       // +0x34
};

// 0x421070: queues a sprite (null clip: the whole screen); nothing past 2000
void DrawList_Add(Image* theImage, uint32_t theSeq, int x, int y, int theDepth, int theKind, uint8_t* theDst,
	int theDPitch, int r, int g, int b, int theAlpha, Palette* thePalette, const int* theClip);
// 0x4211d0: sorts by depth and draws with the screen's format
void DrawList_Flush();
// 0x4214a0
void DrawList_Clear();
int DrawList_Count();

// 0x34 bytes (gDirty at 0x472a7c): the 800x600 screen in cells, each with its changed rectangles (mode 1: one
// bounding rectangle per cell; else up to maxRects, merged where that does not grow them)
struct Dirty
{
	struct Cell
	{
		int* rects;    // {l, t, r, b} inclusive, maxRects of them
		int count;
	};

	Cell* cells;       // +0x00
	int cellW;         // +0x04
	int cellH;         // +0x08
	int cols;          // +0x0c
	int rows;          // +0x10
	int mode;          // +0x14
	int maxRects;      // +0x18
	Image* source;     // +0x1c  the back buffer
	int srcRect[4];    // +0x20
	int bytesPerPixel; // +0x30

	Dirty();  // 0x421b50
	~Dirty(); // 0x4214b0
	Dirty(const Dirty&) = delete;
	Dirty& operator=(const Dirty&) = delete;
};

void Dirty_Free(Dirty* theDirty);                                                     // 0x4214b0
void Dirty_Init(Dirty* theDirty, int theCellW, int theCellH, int theMaxRects, int theMode); // 0x421500
void Dirty_AddRect(Dirty* theDirty, int x, int y, int w, int h);                       // 0x421540
void Dirty_Clear(Dirty* theDirty);                                                     // 0x421960
void Dirty_AddClipped(Dirty* theDirty, int x, int y, int w, int h, const int* theClip); // 0x4219d0
void Dirty_CopyTo(Dirty* theDirty, uint8_t* theDst, int theDPitch);                    // 0x421a60
void Dirty_Alloc(Dirty* theDirty);                                                     // 0x421b80
void Dirty_SetSource(Dirty* theDirty, Image* theSource);                               // 0x421bf0
// 0x421c20, 0x421cd0, 0x421d80: clip, tell the screen, add
void Dirty_AddClippedRect(Dirty* theDirty, const int* theRect, const int* theClip);
void Dirty_AddClippedShown(Dirty* theDirty, int x, int y, int w, int h, const int* theClip);
void Dirty_AddClippedImage(Dirty* theDirty, int x, int y, const Image* theImage, const int* theClip);

// 0x444c50 (and shortsort 0x444bc0): the MSVC 2005 CRT's qsort, which DrawList_Flush relies on: its comparator
// never answers 0, so the order of equal depths is whatever this algorithm leaves
void CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*));

} // namespace r64

#endif
