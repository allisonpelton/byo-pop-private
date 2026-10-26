#include "R64Draw.h"
#include "R64Blit.h"
#include "R64Sys.h"

#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

DrawItem gDrawItems[2000];        // 0x6e6540
int gDrawCount = 0;               // 0x707880
const int kScreenClip[4] = {0, 0, 0x31f, 0x257}; // 0x460010

// 0x421670: the comb sort's next gap
int CombGap(int theGap)
{
	int aGap = (theGap * 10) / 0xd;
	if (aGap != 9 && aGap != 10)
	{
		if (aGap < 1)
			aGap = 1;
		return aGap;
	}
	return 0xb;
}

// 0x421530
int CompareDepth(const void* a, const void* b)
{
	return ((const DrawItem*)a)->depth > ((const DrawItem*)b)->depth ? 1 : -1;
}

void SwapBytes(uint8_t* a, uint8_t* b, size_t theWidth)
{
	if (a != b)
	{
		size_t n = theWidth;
		do
		{
			uint8_t t = *a;
			*a = *b;
			*b = t;
			a++;
			b++;
			n--;
		} while (n != 0);
	}
}

// 0x445000
void ShortSort(uint8_t* lo, uint8_t* hi, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	for (; lo < hi; hi -= theWidth)
	{
		uint8_t* aMax = lo;
		for (uint8_t* p = lo + theWidth; p <= hi; p += theWidth)
		{
			if (theCompare(p, aMax) > 0)
				aMax = p;
		}
		if (aMax != hi && theWidth != 0)
			SwapBytes(aMax, hi, theWidth);
	}
}

inline uint32_t PackColour(const PixelFormat* f, int r, int g, int b)
{
	return (uint32_t)((uint8_t)b >> (f->bLoss & 0x1f)) << (f->bShift & 0x1f) |
		(uint32_t)((uint8_t)g >> (f->gLoss & 0x1f)) << (f->gShift & 0x1f) |
		(uint32_t)((uint8_t)r >> (f->rLoss & 0x1f)) << (f->rShift & 0x1f);
}

} // namespace

// 0x445090
void CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	if (theBase == nullptr || theNum < 2 || theWidth == 0 || theCompare == nullptr)
		return;
	uint8_t* aLoStack[30];
	uint8_t* aHiStack[30];
	int aTop = 0;
	uint8_t* lo = (uint8_t*)theBase;
	uint8_t* hi = (uint8_t*)theBase + (theNum - 1) * theWidth;
	while (true)
	{
		size_t aSize = (size_t)(hi - lo) / theWidth + 1;
		if (aSize <= 8)
			ShortSort(lo, hi, theWidth, theCompare);
		else
		{
			uint8_t* mid = lo + (aSize / 2) * theWidth;
			if (theCompare(lo, mid) > 0)
				SwapBytes(lo, mid, theWidth);
			if (theCompare(lo, hi) > 0)
				SwapBytes(lo, hi, theWidth);
			if (theCompare(mid, hi) > 0)
				SwapBytes(mid, hi, theWidth);
			uint8_t* aLoGuy = lo;
			uint8_t* aHiGuy = hi;
			while (true)
			{
				if (mid > aLoGuy)
				{
					do
					{
						aLoGuy += theWidth;
					} while (aLoGuy < mid && theCompare(aLoGuy, mid) <= 0);
				}
				if (mid <= aLoGuy)
				{
					do
					{
						aLoGuy += theWidth;
					} while (aLoGuy <= hi && theCompare(aLoGuy, mid) <= 0);
				}
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > mid && theCompare(aHiGuy, mid) > 0);
				if (aHiGuy < aLoGuy)
					break;
				SwapBytes(aLoGuy, aHiGuy, theWidth);
				if (mid == aHiGuy)
					mid = aLoGuy;
			}
			aHiGuy += theWidth;
			if (mid < aHiGuy)
			{
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > mid && theCompare(aHiGuy, mid) == 0);
			}
			if (mid >= aHiGuy)
			{
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > lo && theCompare(aHiGuy, mid) == 0);
			}
			if (aHiGuy - lo >= hi - aLoGuy)
			{
				if (lo < aHiGuy)
				{
					aLoStack[aTop] = lo;
					aHiStack[aTop] = aHiGuy;
					aTop++;
				}
				if (aLoGuy < hi)
				{
					lo = aLoGuy;
					continue;
				}
			}
			else
			{
				if (aLoGuy < hi)
				{
					aLoStack[aTop] = aLoGuy;
					aHiStack[aTop] = hi;
					aTop++;
				}
				if (lo < aHiGuy)
				{
					hi = aHiGuy;
					continue;
				}
			}
		}
		aTop--;
		if (aTop < 0)
			return;
		lo = aLoStack[aTop];
		hi = aHiStack[aTop];
	}
}

// 0x421550
void DrawList_Add(Image* theImage, uint32_t theSeq, int x, int y, int theDepth, int theKind, uint8_t* theDst,
	int theDPitch, int r, int g, int b, int theAlpha, Palette* thePalette, const int* theClip)
{
	if (gDrawCount < 2000)
	{
		DrawItem& anItem = gDrawItems[gDrawCount];
		anItem.image = theImage;
		anItem.seq = theSeq;
		anItem.x = x;
		anItem.y = y;
		anItem.depth = theDepth;
		anItem.kind = theKind;
		anItem.dst = theDst;
		anItem.dpitch = theDPitch;
		anItem.r = r;
		anItem.g = g;
		anItem.b = b;
		anItem.alpha = theAlpha;
		anItem.palette = thePalette;
		const int* aClip = theClip != nullptr ? theClip : kScreenClip;
		anItem.clip[0] = aClip[0];
		anItem.clip[1] = aClip[1];
		anItem.clip[2] = aClip[2];
		anItem.clip[3] = aClip[3];
		gDrawCount = gDrawCount + 1;
	}
}

// 0x4216b0: a comb sort by depth then sequence, then the CRT's qsort by depth alone, then each sprite drawn with
// its palette swapped in
void DrawList_Flush()
{
	int aGap = gDrawCount;
	int aNum = gDrawCount;
	bool aSwapped;
	do
	{
		aGap = CombGap(aGap);
		int k = aNum - aGap;
		aSwapped = false;
		if (0 < k)
		{
			DrawItem* a = &gDrawItems[0];
			DrawItem* b = &gDrawItems[aGap];
			do
			{
				if (b->depth < a->depth || (a->depth == b->depth && b->seq < a->seq))
				{
					DrawItem t = *a;
					*a = *b;
					*b = t;
					aSwapped = true;
				}
				b++;
				a++;
				k--;
				aNum = gDrawCount;
			} while (k != 0);
		}
	} while (aGap != 1 || aSwapped);
	CrtQsort(gDrawItems, (size_t)aNum, sizeof(DrawItem), CompareDepth);
	int i = 0;
	if (gDrawCount < 1)
	{
		gDrawCount = 0;
		return;
	}
	const PixelFormat* f = Sys_ScreenFormat();
	do
	{
		DrawItem& it = gDrawItems[i];
		Palette* aSaved = it.image->palette;
		it.image->palette = it.palette;
		switch (it.kind)
		{
		case 0:
			Image_Blit(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y);
			break;
		case 1:
			Image_BlitAlpha(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y);
			break;
		case 2:
			Image_BlitBlend(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y);
			break;
		case 3:
			Image_BlitFill(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y, PackColour(f, it.r, it.g, it.b));
			break;
		case 4:
			Image_BlitAlphaFade(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y, (uint32_t)it.alpha);
			break;
		case 5:
			Image_BlitFillAlpha(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y, PackColour(f, it.r, it.g, it.b),
				(uint32_t)(uint8_t)it.alpha);
			break;
		case 6:
			Image_BlitAdd(it.image, it.dst, it.dpitch, f, it.clip, it.x, it.y);
			break;
		}
		i++;
		it.image->palette = aSaved;
	} while (i < gDrawCount);
	gDrawCount = 0;
}

// 0x421980
void DrawList_Clear()
{
	gDrawCount = 0;
}

int DrawList_Count()
{
	return gDrawCount;
}

// 0x422030
Dirty::Dirty()
{
	cells = nullptr;
	mode = 0;
	maxRects = 400;
	cellW = 800;
	cellH = 600;
	cols = 1;
	rows = 1;
	source = nullptr;
	srcRect[0] = srcRect[1] = srcRect[2] = srcRect[3] = 0;
	bytesPerPixel = 0;
}

Dirty::~Dirty()
{
	Dirty_Free(this);
}

// 0x421990
void Dirty_Free(Dirty* theDirty)
{
	if (theDirty->cells != nullptr)
	{
		int i = 0;
		if (0 < theDirty->rows * theDirty->cols)
		{
			do
			{
				std::free(theDirty->cells[i].rects);
				i++;
			} while (i < theDirty->rows * theDirty->cols);
		}
		std::free(theDirty->cells);
		theDirty->cells = nullptr;
	}
}

// 0x4219e0
void Dirty_Init(Dirty* theDirty, int theCellW, int theCellH, int theMaxRects, int theMode)
{
	theDirty->mode = theMode;
	theDirty->maxRects = theMaxRects;
	theDirty->cellH = theCellH;
	theDirty->cellW = theCellW;
	theDirty->cols = 800 / theCellW;
	theDirty->rows = 600 / theCellH;
}

// 0x421a20: the rectangle's part in each cell it touches. A cell full of rectangles becomes one rectangle of the
// whole cell, and the rest of that row of cells is skipped, as in the original.
void Dirty_AddRect(Dirty* theDirty, int x, int y, int w, int h)
{
	int aRow = y / theDirty->cellH;
	int aBottomEdge = h + y;
	if (aRow <= aBottomEdge / theDirty->cellH)
	{
		do
		{
			if (theDirty->rows <= aRow)
				return;
			int aCellW = theDirty->cellW;
			int aCol = x / aCellW;
			int aRightEdge = w + x;
			int aNextCol = aCol;
			if (aCol <= aRightEdge / aCellW)
			{
				do
				{
					aNextCol = aNextCol + 1;
					if (theDirty->cols <= aCol)
						break;
					int l = x;
					if (x <= aCellW * aCol)
						l = aCellW * aCol;
					int t = y;
					if (y <= theDirty->cellH * aRow)
						t = theDirty->cellH * aRow;
					int r = aRightEdge + -1;
					if (theDirty->cellW * aNextCol + -1 <= r)
						r = theDirty->cellW * aNextCol + -1;
					int b = aBottomEdge + -1;
					if (theDirty->cellH * (aRow + 1) + -1 <= b)
						b = theDirty->cellH * (aRow + 1) + -1;
					Dirty::Cell& aCell = theDirty->cells[theDirty->cols * aRow + aCol];
					if (theDirty->mode == 1)
					{
						int* q = aCell.rects;
						if (l < q[0])
							q[0] = l;
						if (q[1] <= t)
							t = q[1];
						q[1] = t;
						if (r <= q[2])
							r = q[2];
						q[2] = r;
						if (b <= q[3])
							b = q[3];
						q[3] = b;
						aCell.count = 1;
					}
					else
					{
						int aCount = aCell.count;
						if (theDirty->maxRects <= aCount)
						{
							aCell.count = 1;
							aCell.rects[0] = theDirty->cellW * aCol;
							aCell.rects[1] = theDirty->cellH * aRow;
							aCell.rects[2] = (aCol + 1) * theDirty->cellW + -1;
							aCell.rects[3] = (aRow + 1) * theDirty->cellH + -1;
							break;
						}
						int i = 0;
						int aFound = 0;
						if (0 < aCount)
						{
							int* q = aCell.rects;
							do
							{
								aFound = i;
								int qr = q[2];
								if (l <= qr && q[0] <= r && t <= q[3] && q[1] <= b)
								{
									int ul = q[0];
									if (l <= ul)
										ul = l;
									int ur = qr;
									if (qr <= r)
										ur = r;
									int ut = q[1];
									if (t <= ut)
										ut = t;
									int ub = q[3];
									if (ub <= b)
										ub = b;
									int aSum = (q[3] - q[1]) * (qr - q[0]) + (b - t) * (r - l);
									int aUnion = (ub - ut) * (ur - ul);
									if (aUnion - aSum == 0 || aUnion < aSum)
									{
										q[0] = ul;
										q[2] = ur;
										q[1] = ut;
										q[3] = ub;
										break;
									}
								}
								i++;
								aFound = i;
								q += 4;
							} while (i < aCount);
						}
						if (aFound == aCell.count)
						{
							int* q = aCell.rects + aFound * 4;
							q[0] = l;
							q[2] = r;
							q[1] = t;
							q[3] = b;
							aCell.count = aCell.count + 1;
						}
					}
					aCellW = theDirty->cellW;
					aCol = aCol + 1;
				} while (aCol <= aRightEdge / aCellW);
			}
			aRow = aRow + 1;
		} while (aRow <= aBottomEdge / theDirty->cellH);
	}
}

// 0x421e40
void Dirty_Clear(Dirty* theDirty)
{
	int i = 0;
	if (0 < theDirty->cols * theDirty->rows)
	{
		do
		{
			theDirty->cells[i].count = 0;
			if (theDirty->mode == 1)
			{
				theDirty->cells[i].rects[0] = 999999;
				theDirty->cells[i].rects[1] = 999999;
				theDirty->cells[i].rects[2] = -999999;
				theDirty->cells[i].rects[3] = -999999;
			}
			i++;
		} while (i < theDirty->cols * theDirty->rows);
	}
}

// 0x421eb0
void Dirty_AddClipped(Dirty* theDirty, int x, int y, int w, int h, const int* theClip)
{
	int ox, oy, sx, sy, ow, oh;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], x, y, w, h, &ox, &oy, &sx, &sy, &ow, &oh) == 1)
		Dirty_AddRect(theDirty, ox, oy, ow, oh);
}

// 0x421f40: copies each changed rectangle of the back buffer to theDst, tells the screen, and clears the grid
void Dirty_CopyTo(Dirty* theDirty, uint8_t* theDst, int theDPitch)
{
	int i = 0;
	theDirty->bytesPerPixel = (int)((uint32_t)theDirty->source->fmt.bpp >> 3);
	if (0 < theDirty->cols * theDirty->rows)
	{
		do
		{
			int k = 0;
			if (0 < theDirty->cells[i].count)
			{
				do
				{
					const int* q = theDirty->cells[i].rects + k * 4;
					int aX = q[0] * theDirty->bytesPerPixel;
					Copy8(theDst, aX, q[1], theDPitch, 0, theDirty->source->bits, aX, q[1], theDirty->source->pitch, 0,
						((q[2] - q[0]) + 1) * theDirty->bytesPerPixel, (q[3] - q[1]) + 1);
					q = theDirty->cells[i].rects + k * 4;
					Sys_AddUpdateRect(q[0], q[1], (q[2] - q[0]) + 1, (q[3] - q[1]) + 1);
					k++;
				} while (k < theDirty->cells[i].count);
			}
			i++;
		} while (i < theDirty->cols * theDirty->rows);
	}
	Dirty_Clear(theDirty);
}

// 0x422060
void Dirty_Alloc(Dirty* theDirty)
{
	if (theDirty->cells != nullptr)
		std::free(theDirty->cells);
	theDirty->cells = nullptr;
	theDirty->cells = (Dirty::Cell*)std::malloc((size_t)theDirty->rows * theDirty->cols * sizeof(Dirty::Cell));
	if (0 < theDirty->rows * theDirty->cols)
	{
		int i = 0;
		do
		{
			theDirty->cells[i].rects = (int*)std::malloc((size_t)theDirty->maxRects << 4);
			i++;
		} while (i < theDirty->rows * theDirty->cols);
	}
	Dirty_Clear(theDirty);
}

// 0x4220d0
void Dirty_SetSource(Dirty* theDirty, Image* theSource)
{
	theDirty->source = theSource;
	theDirty->srcRect[0] = 0;
	theDirty->srcRect[1] = 0;
	theDirty->srcRect[2] = theSource->w + -1;
	theDirty->srcRect[3] = theSource->h + -1;
	Dirty_Alloc(theDirty);
}

// 0x422100: theRect is {l, t, r, b}
void Dirty_AddClippedRect(Dirty* theDirty, const int* theRect, const int* theClip)
{
	int ox, oy, sx, sy, ow, oh;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], theRect[0], theRect[1], (theRect[2] - theRect[0]) + 1,
			(theRect[3] - theRect[1]) + 1, &ox, &oy, &sx, &sy, &ow, &oh) == 1)
	{
		Sys_AddUpdateRect(ox, oy, ow, oh);
		Dirty_AddRect(theDirty, ox, oy, ow, oh);
	}
}

// 0x4221b0
void Dirty_AddClippedShown(Dirty* theDirty, int x, int y, int w, int h, const int* theClip)
{
	int ox, oy, sx, sy, ow, oh;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], x, y, w, h, &ox, &oy, &sx, &sy, &ow, &oh) == 1)
	{
		Sys_AddUpdateRect(ox, oy, ow, oh);
		Dirty_AddRect(theDirty, ox, oy, ow, oh);
	}
}

// 0x422260: where theImage would be drawn at (x, y)
void Dirty_AddClippedImage(Dirty* theDirty, int x, int y, const Image* theImage, const int* theClip)
{
	int ox, oy, sx, sy, ow, oh;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], Image_DrawX(theImage) + x, Image_DrawY(theImage) + y,
			theImage->w, theImage->h, &ox, &oy, &sx, &sy, &ow, &oh) == 1)
	{
		Sys_AddUpdateRect(ox, oy, ow, oh);
		Dirty_AddRect(theDirty, ox, oy, ow, oh);
	}
}

} // namespace r64
