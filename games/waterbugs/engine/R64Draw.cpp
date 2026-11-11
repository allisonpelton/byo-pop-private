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
const int SCREEN_CLIP[4] = {0, 0, 0x31f, 0x257}; // 0x460010

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
	return ((const DrawItem*)a)->mDepth > ((const DrawItem*)b)->mDepth ? 1 : -1;
}

void SwapBytes(uint8_t* theFirst, uint8_t* theSecond, size_t theWidth)
{
	if (theFirst != theSecond)
	{
		size_t n = theWidth;
		do
		{
			uint8_t t = *theFirst;
			*theFirst = *theSecond;
			*theSecond = t;
			theFirst++;
			theSecond++;
			n--;
		} while (n != 0);
	}
}

// 0x445000
void ShortSort(uint8_t* theLo, uint8_t* theHi, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	for (; theLo < theHi; theHi -= theWidth)
	{
		uint8_t* aMax = theLo;
		for (uint8_t* p = theLo + theWidth; p <= theHi; p += theWidth)
		{
			if (theCompare(p, aMax) > 0)
				aMax = p;
		}
		if (aMax != theHi && theWidth != 0)
			SwapBytes(aMax, theHi, theWidth);
	}
}

inline uint32_t PackColour(const PixelFormat* f, int r, int g, int b)
{
	return (uint32_t)((uint8_t)b >> (f->mBLoss & 0x1f)) << (f->mBShift & 0x1f) |
		(uint32_t)((uint8_t)g >> (f->mGLoss & 0x1f)) << (f->mGShift & 0x1f) |
		(uint32_t)((uint8_t)r >> (f->mRLoss & 0x1f)) << (f->mRShift & 0x1f);
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
	uint8_t* aLo = (uint8_t*)theBase;
	uint8_t* aHi = (uint8_t*)theBase + (theNum - 1) * theWidth;
	while (true)
	{
		size_t aSize = (size_t)(aHi - aLo) / theWidth + 1;
		if (aSize <= 8)
			ShortSort(aLo, aHi, theWidth, theCompare);
		else
		{
			uint8_t* aMid = aLo + (aSize / 2) * theWidth;
			if (theCompare(aLo, aMid) > 0)
				SwapBytes(aLo, aMid, theWidth);
			if (theCompare(aLo, aHi) > 0)
				SwapBytes(aLo, aHi, theWidth);
			if (theCompare(aMid, aHi) > 0)
				SwapBytes(aMid, aHi, theWidth);
			uint8_t* aLoGuy = aLo;
			uint8_t* aHiGuy = aHi;
			while (true)
			{
				if (aMid > aLoGuy)
				{
					do
					{
						aLoGuy += theWidth;
					} while (aLoGuy < aMid && theCompare(aLoGuy, aMid) <= 0);
				}
				if (aMid <= aLoGuy)
				{
					do
					{
						aLoGuy += theWidth;
					} while (aLoGuy <= aHi && theCompare(aLoGuy, aMid) <= 0);
				}
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > aMid && theCompare(aHiGuy, aMid) > 0);
				if (aHiGuy < aLoGuy)
					break;
				SwapBytes(aLoGuy, aHiGuy, theWidth);
				if (aMid == aHiGuy)
					aMid = aLoGuy;
			}
			aHiGuy += theWidth;
			if (aMid < aHiGuy)
			{
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > aMid && theCompare(aHiGuy, aMid) == 0);
			}
			if (aMid >= aHiGuy)
			{
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > aLo && theCompare(aHiGuy, aMid) == 0);
			}
			if (aHiGuy - aLo >= aHi - aLoGuy)
			{
				if (aLo < aHiGuy)
				{
					aLoStack[aTop] = aLo;
					aHiStack[aTop] = aHiGuy;
					aTop++;
				}
				if (aLoGuy < aHi)
				{
					aLo = aLoGuy;
					continue;
				}
			}
			else
			{
				if (aLoGuy < aHi)
				{
					aLoStack[aTop] = aLoGuy;
					aHiStack[aTop] = aHi;
					aTop++;
				}
				if (aLo < aHiGuy)
				{
					aHi = aHiGuy;
					continue;
				}
			}
		}
		aTop--;
		if (aTop < 0)
			return;
		aLo = aLoStack[aTop];
		aHi = aHiStack[aTop];
	}
}

// 0x421550
void DrawList_Add(Image* theImage, uint32_t theSeq, int x, int y, int theDepth, int theKind, uint8_t* theDst,
	int theDPitch, int theRed, int theGreen, int theBlue, int theAlpha, Palette* thePalette, const int* theClip)
{
	if (gDrawCount < 2000)
	{
		DrawItem& anItem = gDrawItems[gDrawCount];
		anItem.mImage = theImage;
		anItem.mSeq = theSeq;
		anItem.mX = x;
		anItem.mY = y;
		anItem.mDepth = theDepth;
		anItem.mKind = theKind;
		anItem.mDst = theDst;
		anItem.mDstPitch = theDPitch;
		anItem.mR = theRed;
		anItem.mG = theGreen;
		anItem.mB = theBlue;
		anItem.mAlpha = theAlpha;
		anItem.mPalette = thePalette;
		const int* aClip = theClip != nullptr ? theClip : SCREEN_CLIP;
		anItem.mClip[0] = aClip[0];
		anItem.mClip[1] = aClip[1];
		anItem.mClip[2] = aClip[2];
		anItem.mClip[3] = aClip[3];
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
			DrawItem* anItem = &gDrawItems[0];
			DrawItem* b = &gDrawItems[aGap];
			do
			{
				if (b->mDepth < anItem->mDepth || (anItem->mDepth == b->mDepth && b->mSeq < anItem->mSeq))
				{
					DrawItem t = *anItem;
					*anItem = *b;
					*b = t;
					aSwapped = true;
				}
				b++;
				anItem++;
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
	const PixelFormat* aFormat = Sys_ScreenFormat();
	do
	{
		DrawItem& anIt = gDrawItems[i];
		Palette* aSaved = anIt.mImage->mPalette;
		anIt.mImage->mPalette = anIt.mPalette;
		switch (anIt.mKind)
		{
		case 0:
			Image_Blit(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY);
			break;
		case 1:
			Image_BlitAlpha(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY);
			break;
		case 2:
			Image_BlitBlend(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY);
			break;
		case 3:
			Image_BlitFill(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY, PackColour(aFormat, anIt.mR, anIt.mG, anIt.mB));
			break;
		case 4:
			Image_BlitAlphaFade(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY, (uint32_t)anIt.mAlpha);
			break;
		case 5:
			Image_BlitFillAlpha(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY, PackColour(aFormat, anIt.mR, anIt.mG, anIt.mB),
				(uint32_t)(uint8_t)anIt.mAlpha);
			break;
		case 6:
			Image_BlitAdd(anIt.mImage, anIt.mDst, anIt.mDstPitch, aFormat, anIt.mClip, anIt.mX, anIt.mY);
			break;
		}
		i++;
		anIt.mImage->mPalette = aSaved;
	} while (i < gDrawCount);
	gDrawCount = 0;
}

// 0x421980
void DrawList_Clear()
{
	gDrawCount = 0;
}

// 0x422030
Dirty::Dirty()
{
	mCells = nullptr;
	mMode = 0;
	mMaxRects = 400;
	mCellW = 800;
	mCellH = 600;
	mCols = 1;
	mRows = 1;
	mSource = nullptr;
	mSrcRect[0] = mSrcRect[1] = mSrcRect[2] = mSrcRect[3] = 0;
	mBytesPerPixel = 0;
}

Dirty::~Dirty()
{
	Dirty_Free(this);
}

// 0x421990
void Dirty_Free(Dirty* theDirty)
{
	if (theDirty->mCells != nullptr)
	{
		int i = 0;
		if (0 < theDirty->mRows * theDirty->mCols)
		{
			do
			{
				std::free(theDirty->mCells[i].mRects);
				i++;
			} while (i < theDirty->mRows * theDirty->mCols);
		}
		std::free(theDirty->mCells);
		theDirty->mCells = nullptr;
	}
}

// 0x4219e0
void Dirty_Init(Dirty* theDirty, int theCellW, int theCellH, int theMaxRects, int theMode)
{
	theDirty->mMode = theMode;
	theDirty->mMaxRects = theMaxRects;
	theDirty->mCellH = theCellH;
	theDirty->mCellW = theCellW;
	theDirty->mCols = 800 / theCellW;
	theDirty->mRows = 600 / theCellH;
}

// 0x421a20: the rectangle's part in each cell it touches. A cell full of rectangles becomes one rectangle of the
// whole cell, and the rest of that row of cells is skipped, as in the original.
void Dirty_AddRect(Dirty* theDirty, int x, int y, int theWidth, int h)
{
	int aRow = y / theDirty->mCellH;
	int aBottomEdge = h + y;
	if (aRow <= aBottomEdge / theDirty->mCellH)
	{
		do
		{
			if (theDirty->mRows <= aRow)
				return;
			int aCellW = theDirty->mCellW;
			int aCol = x / aCellW;
			int aRightEdge = theWidth + x;
			int aNextCol = aCol;
			if (aCol <= aRightEdge / aCellW)
			{
				do
				{
					aNextCol = aNextCol + 1;
					if (theDirty->mCols <= aCol)
						break;
					int aLeft = x;
					if (x <= aCellW * aCol)
						aLeft = aCellW * aCol;
					int aTop = y;
					if (y <= theDirty->mCellH * aRow)
						aTop = theDirty->mCellH * aRow;
					int aRight = aRightEdge + -1;
					if (theDirty->mCellW * aNextCol + -1 <= aRight)
						aRight = theDirty->mCellW * aNextCol + -1;
					int aBottom = aBottomEdge + -1;
					if (theDirty->mCellH * (aRow + 1) + -1 <= aBottom)
						aBottom = theDirty->mCellH * (aRow + 1) + -1;
					Dirty::Cell& aCell = theDirty->mCells[theDirty->mCols * aRow + aCol];
					if (theDirty->mMode == 1)
					{
						int* aRects = aCell.mRects;
						if (aLeft < aRects[0])
							aRects[0] = aLeft;
						if (aRects[1] <= aTop)
							aTop = aRects[1];
						aRects[1] = aTop;
						if (aRight <= aRects[2])
							aRight = aRects[2];
						aRects[2] = aRight;
						if (aBottom <= aRects[3])
							aBottom = aRects[3];
						aRects[3] = aBottom;
						aCell.mCount = 1;
					}
					else
					{
						int aCount = aCell.mCount;
						if (theDirty->mMaxRects <= aCount)
						{
							aCell.mCount = 1;
							aCell.mRects[0] = theDirty->mCellW * aCol;
							aCell.mRects[1] = theDirty->mCellH * aRow;
							aCell.mRects[2] = (aCol + 1) * theDirty->mCellW + -1;
							aCell.mRects[3] = (aRow + 1) * theDirty->mCellH + -1;
							break;
						}
						int i = 0;
						int aFound = 0;
						if (0 < aCount)
						{
							int* aRects = aCell.mRects;
							do
							{
								aFound = i;
								int aQr = aRects[2];
								if (aLeft <= aQr && aRects[0] <= aRight && aTop <= aRects[3] && aRects[1] <= aBottom)
								{
									int anUl = aRects[0];
									if (aLeft <= anUl)
										anUl = aLeft;
									int anUr = aQr;
									if (aQr <= aRight)
										anUr = aRight;
									int anUt = aRects[1];
									if (aTop <= anUt)
										anUt = aTop;
									int anUb = aRects[3];
									if (anUb <= aBottom)
										anUb = aBottom;
									int aSum = (aRects[3] - aRects[1]) * (aQr - aRects[0]) + (aBottom - aTop) * (aRight - aLeft);
									int aUnion = (anUb - anUt) * (anUr - anUl);
									if (aUnion - aSum == 0 || aUnion < aSum)
									{
										aRects[0] = anUl;
										aRects[2] = anUr;
										aRects[1] = anUt;
										aRects[3] = anUb;
										break;
									}
								}
								i++;
								aFound = i;
								aRects += 4;
							} while (i < aCount);
						}
						if (aFound == aCell.mCount)
						{
							int* q = aCell.mRects + aFound * 4;
							q[0] = aLeft;
							q[2] = aRight;
							q[1] = aTop;
							q[3] = aBottom;
							aCell.mCount = aCell.mCount + 1;
						}
					}
					aCellW = theDirty->mCellW;
					aCol = aCol + 1;
				} while (aCol <= aRightEdge / aCellW);
			}
			aRow = aRow + 1;
		} while (aRow <= aBottomEdge / theDirty->mCellH);
	}
}

// 0x421e40
void Dirty_Clear(Dirty* theDirty)
{
	int i = 0;
	if (0 < theDirty->mCols * theDirty->mRows)
	{
		do
		{
			theDirty->mCells[i].mCount = 0;
			if (theDirty->mMode == 1)
			{
				theDirty->mCells[i].mRects[0] = 999999;
				theDirty->mCells[i].mRects[1] = 999999;
				theDirty->mCells[i].mRects[2] = -999999;
				theDirty->mCells[i].mRects[3] = -999999;
			}
			i++;
		} while (i < theDirty->mCols * theDirty->mRows);
	}
}

// 0x421eb0
void Dirty_AddClipped(Dirty* theDirty, int x, int y, int w, int h, const int* theClip)
{
	int aDstX, aDstY, aSrcX, aSrcY, aBlitWidth, aBlitHeight;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], x, y, w, h, &aDstX, &aDstY, &aSrcX, &aSrcY, &aBlitWidth, &aBlitHeight) == 1)
		Dirty_AddRect(theDirty, aDstX, aDstY, aBlitWidth, aBlitHeight);
}

// 0x421f40: copies each changed rectangle of the back buffer to theDst, tells the screen, and clears the grid
void Dirty_CopyTo(Dirty* theDirty, uint8_t* theDst, int theDPitch)
{
	int i = 0;
	theDirty->mBytesPerPixel = (int)((uint32_t)theDirty->mSource->mFmt.mBpp >> 3);
	if (0 < theDirty->mCols * theDirty->mRows)
	{
		do
		{
			int k = 0;
			if (0 < theDirty->mCells[i].mCount)
			{
				do
				{
					const int* q = theDirty->mCells[i].mRects + k * 4;
					int aX = q[0] * theDirty->mBytesPerPixel;
					Copy8(theDst, aX, q[1], theDPitch, 0, theDirty->mSource->mBits, aX, q[1], theDirty->mSource->mPitch, 0,
						((q[2] - q[0]) + 1) * theDirty->mBytesPerPixel, (q[3] - q[1]) + 1);
					q = theDirty->mCells[i].mRects + k * 4;
					Sys_AddUpdateRect(q[0], q[1], (q[2] - q[0]) + 1, (q[3] - q[1]) + 1);
					k++;
				} while (k < theDirty->mCells[i].mCount);
			}
			i++;
		} while (i < theDirty->mCols * theDirty->mRows);
	}
	Dirty_Clear(theDirty);
}

// 0x422060
void Dirty_Alloc(Dirty* theDirty)
{
	if (theDirty->mCells != nullptr)
		std::free(theDirty->mCells);
	theDirty->mCells = nullptr;
	theDirty->mCells = (Dirty::Cell*)std::malloc((size_t)theDirty->mRows * theDirty->mCols * sizeof(Dirty::Cell));
	if (0 < theDirty->mRows * theDirty->mCols)
	{
		int i = 0;
		do
		{
			theDirty->mCells[i].mRects = (int*)std::malloc((size_t)theDirty->mMaxRects << 4);
			i++;
		} while (i < theDirty->mRows * theDirty->mCols);
	}
	Dirty_Clear(theDirty);
}

// 0x4220d0
void Dirty_SetSource(Dirty* theDirty, Image* theSource)
{
	theDirty->mSource = theSource;
	theDirty->mSrcRect[0] = 0;
	theDirty->mSrcRect[1] = 0;
	theDirty->mSrcRect[2] = theSource->mW + -1;
	theDirty->mSrcRect[3] = theSource->mH + -1;
	Dirty_Alloc(theDirty);
}

// 0x422100: theRect is {l, t, r, b}
void Dirty_AddClippedRect(Dirty* theDirty, const int* theRect, const int* theClip)
{
	int aDstX, aDstY, aSrcX, aSrcY, aBlitWidth, aBlitHeight;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], theRect[0], theRect[1], (theRect[2] - theRect[0]) + 1,
			(theRect[3] - theRect[1]) + 1, &aDstX, &aDstY, &aSrcX, &aSrcY, &aBlitWidth, &aBlitHeight) == 1)
	{
		Sys_AddUpdateRect(aDstX, aDstY, aBlitWidth, aBlitHeight);
		Dirty_AddRect(theDirty, aDstX, aDstY, aBlitWidth, aBlitHeight);
	}
}

// 0x4221b0
void Dirty_AddClippedShown(Dirty* theDirty, int x, int y, int w, int h, const int* theClip)
{
	int aDstX, aDstY, aSrcX, aSrcY, aBlitWidth, aBlitHeight;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], x, y, w, h, &aDstX, &aDstY, &aSrcX, &aSrcY, &aBlitWidth, &aBlitHeight) == 1)
	{
		Sys_AddUpdateRect(aDstX, aDstY, aBlitWidth, aBlitHeight);
		Dirty_AddRect(theDirty, aDstX, aDstY, aBlitWidth, aBlitHeight);
	}
}

// 0x422260: where theImage would be drawn at (x, y)
void Dirty_AddClippedImage(Dirty* theDirty, int x, int y, const Image* theImage, const int* theClip)
{
	int aDstX, aDstY, aSrcX, aSrcY, aBlitWidth, aBlitHeight;
	if (ClipBlit(theClip[0], theClip[1], theClip[2], theClip[3], Image_DrawX(theImage) + x, Image_DrawY(theImage) + y,
			theImage->mW, theImage->mH, &aDstX, &aDstY, &aSrcX, &aSrcY, &aBlitWidth, &aBlitHeight) == 1)
	{
		Sys_AddUpdateRect(aDstX, aDstY, aBlitWidth, aBlitHeight);
		Dirty_AddRect(theDirty, aDstX, aDstY, aBlitWidth, aBlitHeight);
	}
}

} // namespace r64
