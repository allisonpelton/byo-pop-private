// BoardDrawBoard.cpp: Sexy::Board::DrawBoard 0x59df5c, the board itself: the frame around it, the time bar of the timed
// games, every gem (lit by the glows around it and by the explosions' light, flaming, shrinking, bombs, rocks and
// wildcards, in 2D, in 3D with the display's transforms and in the OLDSKOOL cheat's way), the bomb counters, and over them
// the hint's sparkles and arrows, the selection boxes, the power gem's glow and the tutorial's pointer.
//
// The gems are drawn back to front: by depth (mZ), through the C runtime's qsort, which the original calls. Gems of equal
// depth stay in the order that qsort's partitioning leaves them in, and that order decides which of two touching gems is
// on top, so the sort is the runtime's own (CrtQsort below; the other ports keep their copies of it too).

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "DDInterface.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "Graphics.h"
#include "Font.h"
#include "Color.h"
#include "Rect.h"
#include "SexyMatrix.h"
#include "TriVertex.h"
#include "Common.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace Sexy;

namespace
{

// 0x5e8950: the C runtime's qsort (an insertion-sort-like pass for 8 elements or fewer, median of three and a partition
// above that, the smaller part first and the larger one on a stack)
void CrtSwap(char* theA, char* theB, size_t theWidth)
{
	if (theA == theB)
		return;
	while (theWidth--)
	{
		char aByte = *theA;
		*theA++ = *theB;
		*theB++ = aByte;
	}
}

// moves the largest to the end, again and again
void CrtShortSort(char* theLo, char* theHi, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	while (theHi > theLo)
	{
		char* aMax = theLo;
		for (char* p = theLo + theWidth; p <= theHi; p += theWidth)
		{
			if (theCompare(p, aMax) > 0)
				aMax = p;
		}
		CrtSwap(aMax, theHi, theWidth);
		theHi -= theWidth;
	}
}

void CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	if (theNum < 2)
		return;
	char* aLoStack[30];
	char* aHiStack[30];
	int aStackPtr = 0;
	char* aLo = (char*)theBase;
	char* aHi = (char*)theBase + theWidth * (theNum - 1);
	for (;;)
	{
		size_t aSize = (size_t)(aHi - aLo) / theWidth + 1;
		if (aSize <= 8)
		{
			CrtShortSort(aLo, aHi, theWidth, theCompare);
		}
		else
		{
			// the median of three, then the partition
			char* aMid = aLo + (aSize / 2) * theWidth;
			if (theCompare(aLo, aMid) > 0)
				CrtSwap(aLo, aMid, theWidth);
			if (theCompare(aLo, aHi) > 0)
				CrtSwap(aLo, aHi, theWidth);
			if (theCompare(aMid, aHi) > 0)
				CrtSwap(aMid, aHi, theWidth);
			char* aLoGuy = aLo;
			char* aHiGuy = aHi;
			for (;;)
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
				CrtSwap(aLoGuy, aHiGuy, theWidth);
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
			// the smaller part next, the larger one on the stack
			if (aHiGuy - aLo >= aHi - aLoGuy)
			{
				if (aLo < aHiGuy)
				{
					aLoStack[aStackPtr] = aLo;
					aHiStack[aStackPtr] = aHiGuy;
					aStackPtr++;
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
					aLoStack[aStackPtr] = aLoGuy;
					aHiStack[aStackPtr] = aHi;
					aStackPtr++;
				}
				if (aLo < aHiGuy)
				{
					aHi = aHiGuy;
					continue;
				}
			}
		}
		aStackPtr--;
		if (aStackPtr < 0)
			return;
		aLo = aLoStack[aStackPtr];
		aHi = aHiStack[aStackPtr];
	}
}

// 0x5968f8 as qsort calls it: the elements are the pointers to the pieces
int ComparePieces(const void* theA, const void* theB)
{
	return Board::ComparePieceZ((Piece* const*)theA, (Piece* const*)theB);
}

// 0x66b0c8: the light level (the slot of mGlow) that each step of the glow's animation feeds, and 0x66b080: where, as a
// column and row step, the neighbour lies that the slot's light comes from (the last one, 8, is the gem itself)
const int gGlowSlot[10] = {0, 4, 8, 2, 6, 3, 7, 8, 1, 5};
const int gGlowStep[9][2] = {{0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, 0}};

// 0x66b054: the frames that a hint stays on a gem
const int HINT_TIME = 0x122;

// 0x66b058: a double that the original compares with zero before drawing a gem (it holds 1.0 and nothing sets it)
const double gDrawGems = 1.0;

}

// 0x59df5c: theSkipStatic leaves out the frame, theSkipDynamic everything but the frame (the 2D game draws the two halves
// separately: the frame once into a picture, the rest every frame)
void Board::DrawBoard(Graphics* g, bool theSkipStatic, bool theSkipDynamic)
{
	// the pictures of the gems fade with this as the whole board does
	float anAlpha = m15fb4 * m15fac;
	bool a3D = mApp->Is3DAccelerated();
	bool aStarDrawn = false;

	// the second graphics: the glow over each gem is drawn with it, as a plain copy of g
	Graphics aGlowGraphics(*g);
	aGlowGraphics.SetDrawMode(1);
	aGlowGraphics.SetColorizeImages(true);

	// the frame
	if (!mIsPuzzle || mIsSecret)
	{
		if (!theSkipStatic && !m165c0)
			g->DrawImage(IMAGE_BOARD_BORDER, mApp->ScaleNum(0x108, 0), mApp->ScaleNum(3, 0));
		if (m15f18)
		{
			if (theSkipDynamic)
				return;
			// the red lights in the corners
			g->SetDrawMode(1);
			g->DrawImage(IMAGE_BOARD_RED_LIGHT, mApp->ScaleNum(0x10d, 0), mApp->ScaleNum(2, 0));
			g->DrawImage(IMAGE_BOARD_RED_LIGHT, mApp->ScaleNum(0x3c7, 0), mApp->ScaleNum(2, 0));
			g->DrawImage(IMAGE_BOARD_RED_LIGHT, mApp->ScaleNum(0x10d, 0), mApp->ScaleNum(0x2a2, 0));
			g->DrawImage(IMAGE_BOARD_RED_LIGHT, mApp->ScaleNum(0x3c7, 0), mApp->ScaleNum(0x2a2, 0));
			g->SetDrawMode(0);
		}
	}
	else if (!theSkipStatic && !m165c0)
	{
		g->DrawImage(IMAGE_PUZZLE_FRAME, mApp->ScaleNum(0x12a, 0), mApp->ScaleNum(0x18, 0));
	}

	if (theSkipDynamic)
		return;

	// the time bar of the timed games: the two ends and, between them, the middle repeated
	if (!mIsPuzzle || mIsSecret)
	{
		int aTotal = mApp->ScaleNumUp(m1994 + 0x14, 0);
		int anEndWidth = IMAGE_BAR_LEFT->GetWidth();
		int aBarHeight = mApp->ScaleNumUp(0x3a, 0);
		int aBarX = mApp->ScaleNum(0x117, 0);
		int aBarY = mApp->ScaleNum(0x2c0, 0);
		g->SetColorizeImages(true);
		if (m15f18)
			g->SetColor(Color(0xff, 0x80, 0x80));
		else
			g->SetColor(Color(0xff, 0xff, 0xff));
		g->SetDrawMode(1);
		if (aTotal < anEndWidth * 2)
		{
			int aHalf = aTotal / 2;
			int aRest = aTotal - aHalf;
			Rect aSrc(0, 0, aHalf, aBarHeight);
			g->DrawImage(IMAGE_BAR_LEFT, aBarX, aBarY, aSrc);
			aSrc.mX = anEndWidth - aRest;
			aSrc.mY = 0;
			aSrc.mWidth = aRest;
			aSrc.mHeight = aBarHeight;
			g->DrawImage(IMAGE_BAR_RIGHT, aBarX + aHalf, aBarY, aSrc);
		}
		else
		{
			g->DrawImage(IMAGE_BAR_LEFT, aBarX, aBarY);
			g->DrawImage(IMAGE_BAR_RIGHT, (aBarX - anEndWidth) + aTotal, aBarY);
			int aMidHeight = IMAGE_BAR_MID->GetHeight();
			int aMidWidth = IMAGE_BAR_MID->GetWidth() - 2;
			int aMiddle = aTotal - anEndWidth * 2;
			Graphics aClipped(*g);
			aBarX = aBarX + anEndWidth;
			aClipped.ClipRect(aBarX, aBarY, aMiddle, aBarHeight);
			int aCount = (aMiddle - 1 + aMidWidth) / aMidWidth;
			if (0 < aCount)
			{
				do
				{
					Rect aSrc(1, 0, aMidWidth, aMidHeight);
					aClipped.DrawImage(IMAGE_BAR_MID, aBarX, aBarY, aSrc);
					aBarX += aMidWidth;
					aCount--;
				} while (aCount != 0);
			}
		}
		g->SetDrawMode(0);
		g->SetColorizeImages(false);
	}

	// the gems, back to front
	Piece* aPieces[64];
	int aPieceCount = 0;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			if (mBoard[aRow][aCol] != NULL)
			{
				aPieces[aPieceCount] = mBoard[aRow][aCol];
				aPieceCount++;
			}
		}
	}
	CrtQsort(aPieces, aPieceCount, sizeof(Piece*), ComparePieces);

	// the lights around every gem start at nothing; the wave that runs over the board (m1848) lights a gem as it passes
	for (int i = 0; i < aPieceCount; i++)
	{
		Piece* aPiece = aPieces[i];
		for (int aSlot = 0; aSlot < 9; aSlot++)
			aPiece->mGlow[aSlot] = 0.0f;
		if (m1848)
		{
			float aWave = m184c;
			float aPlace = (float)((double)(aPiece->mRow + aPiece->mCol) * 0.0625);
			aPlace = (float)((double)aWave - (double)aPlace);
			float aDistance = fabsf(aPlace);
			float aStrength = (float)(1.0 - (double)aDistance * 9.0);
			if (0.0f < aStrength)
			{
				aPiece->mGlow[3] = (float)((double)aStrength * (double)0.8f + (double)aPiece->mGlow[3]);
				double aShare = (double)aStrength * (double)0.6f;
				aPiece->mGlow[7] = (float)((double)aPiece->mGlow[7] + aShare);
				aPiece->mGlow[8] = (float)(aShare + (double)aPiece->mGlow[8]);
			}
		}
	}

	// the lights that the flaming gems, the gems the lightning goes through and the glowing gems throw on the gems around
	for (int i = 0; i < aPieceCount; i++)
	{
		Piece* aPiece = aPieces[i];
		if (aPiece->mType < 7 && aPiece->m70 && a3D)
		{
			float aFlame = (float)(mStarPhase + mStarPhase);
			aFlame = (float)((double)aFlame - 1.0);
			aFlame = fabsf(aFlame);
			float aLightY = (float)((double)aPiece->mY + 42.0);
			float aLightX = (float)(42.0 + (double)aPiece->mX);
			AddPointLight(aLightX, aLightY, 20.0f, 10.0f, aFlame, aPiece, true);
		}
		if (aPiece->m2C)
		{
			float aAngle = (float)((double)aPiece->m30 * 15.0);
			float aSine = sinf(aAngle);
			float aLight = (float)((double)fabsf(aSine) * (double)0.6f);
			aPiece->mGlow[8] = aLight;
			if (a3D)
			{
				float aLightY = (float)((double)aPiece->mY + 42.0);
				float aLightX = (float)(42.0 + (double)aPiece->mX);
				AddPointLight(aLightX, aLightY, 15.0f, 10.0f, aLight, aPiece, false);
			}
		}
		if (aPiece->m8C)
		{
			// the glow moves round the gem: m98 is the step (0 to 10) of the animation, shared between two slots
			int aStep = FTol((double)aPiece->m98);
			float aFraction = (float)((double)aPiece->m98 - (double)aStep);
			int aNextStep = (aStep + 1) % 10;
			double anInverse = 1.0 - (double)aFraction;
			float aGlow = (float)aPiece->m90;
			aPiece->mGlow[gGlowSlot[aStep]] = (float)((double)aGlow * anInverse + (double)aPiece->mGlow[gGlowSlot[aStep]]);
			aGlow = (float)aPiece->m90;
			aPiece->mGlow[gGlowSlot[aNextStep]] = (float)((double)aGlow * (double)aFraction + (double)aPiece->mGlow[gGlowSlot[aNextStep]]);
			int aWhich = 0;
			do
			{
				int aUsedStep = aStep;
				if (aWhich != 0)
					aUsedStep = aNextStep;
				int aDirection = gGlowSlot[aUsedStep];
				double aShare = anInverse;
				if (aWhich != 0)
					aShare = (double)aFraction;
				float aStrength = (float)(aShare * aPiece->m90);
				if (aDirection != 9)
				{
					int aNeighbourCol = gGlowStep[aDirection][0] + aPiece->mCol;
					int aNeighbourRow = gGlowStep[aDirection][1] + aPiece->mRow;
					if ((unsigned int)aNeighbourCol < 8 && (unsigned int)aNeighbourRow < 8)
					{
						Piece* aNeighbour = mBoard[aNeighbourRow][aNeighbourCol];
						if (aNeighbour != NULL)
						{
							int aSlot = (aDirection + 4) % 8;
							aNeighbour->mGlow[aSlot] = (float)((double)aStrength * (double)0.3f + (double)aNeighbour->mGlow[aSlot]);
						}
					}
				}
				aWhich++;
			} while (aWhich < 2);
		}
	}

	// and the shock waves of the explosions
	if (a3D)
	{
		for (int i = 0; i < (int)mShockWaves.size(); i++)
		{
			ShockWave& aWave = mShockWaves[i];
			AddPointLight(aWave.mX, aWave.mY, aWave.mRadius, aWave.mWidth, aWave.mProgress, NULL, false);
		}
	}

	for (int i = 0; i < aPieceCount; i++)
	{
		Piece* aPiece = aPieces[i];
		int aType = aPiece->mType;
		float aSavedY = aPiece->mY;

		if (aType < 7 && gDrawGems != 0.0)
		{
			if (m165ac)
			{
				// the OLDSKOOL cheat: the gems of the first Bejeweled
				if (aPiece->m9C)
				{
					int aSize = mApp->ScaleNumUp(((6 - aPiece->mA0) * 0x54) / 7, 0) & ~1;
					int aFrameHeight = mApp->ScaleNumUp(0x54, 0);
					int aFrameWidth = mApp->ScaleNumUp(0x54, 0);
					Rect aSrc(mApp->ScaleNum(aType * 0x54, 0), 0, aFrameWidth, aFrameHeight);
					int aHalf = aSize / 2;
					int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f)) - aHalf;
					aDestY = mApp->ScaleNum(0x2a, 0) + aDestY;
					int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f)) - aHalf;
					aDestX = mApp->ScaleNum(0x2a, 0) + aDestX;
					Rect aDest(aDestX, aDestY, aSize, aSize);
					g->DrawImage(IMAGE_OLD_GEMS, aDest, aSrc);
				}
				else
				{
					int aFrameHeight = mApp->ScaleNumUp(0x54, 0);
					int aFrameWidth = mApp->ScaleNumUp(0x54, 0);
					Rect aSrc(mApp->ScaleNum(aType * 0x54, 0), 0, aFrameWidth, aFrameHeight);
					int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
					int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
					g->DrawImage(IMAGE_OLD_GEMS, aDestX, aDestY, aSrc);
				}
			}
			else
			{
				// aSource: where the gem's frame starts in its picture (mA4 is the frame that the hint's gems use)
				int aSource = 0;
				if (aPiece->mA4 != 0)
					aSource = aPiece->mA4 * 0x54;

				if (aPiece->m70 && !aPiece->m9C && aPiece->mZ == 0.0f)
				{
					// a flaming gem lying still: it pulses between its picture and the lit one
					float aPulse = (float)fabs(mStarPhase + mStarPhase - 1.0);
					if (!a3D && aSource == 0)
					{
						if (mApp->mBombifiedStillStrip == NULL)
							mApp->GenerateBombifiedStillStrip();
						int aFrame = FTol((double)aPulse * 10.0 - 0.5);
						g->SetColorizeImages(true);
						g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
						int aSize = mApp->ScaleNum(0x54, 0);
						if (aFrame == -1)
						{
							int aSecond = mApp->ScaleNum(0x54, 0);
							Rect aSrc(mApp->ScaleNum(0, 0), 0, aSecond, aSize);
							int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
							int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
							g->DrawImage(GetImageById(aPiece->mType + 0x5c), aDestX, aDestY, aSrc);
						}
						else
						{
							int aHeight = mApp->ScaleNum(0x54, 0);
							Rect aSrc(mApp->ScaleNum(aFrame * 0x54, 0), mApp->ScaleNum(aPiece->mType * 0x54, 0), aHeight, aSize);
							int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
							int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
							g->DrawImage(mApp->mBombifiedStillStrip, aDestX, aDestY, aSrc);
						}
					}
					else
					{
						g->SetColorizeImages(true);
						g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
						int aWidth = mApp->ScaleNum(0x54, 0);
						int aHeight = mApp->ScaleNum(0x54, 0);
						Rect aSrc(mApp->ScaleNum(aSource, 0), 0, aHeight, aWidth);
						int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
						int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
						g->DrawImage(GetImageById(aPiece->mType + 0x5c), aDestX, aDestY, aSrc);

						// the lit picture added over it
						int aLit = FTol(((double)aPulse * 255.0) * (double)anAlpha);
						g->SetDrawMode(1);
						g->SetColor(Color(aLit, aLit, aLit));
						int aLitWidth = mApp->ScaleNumUp(0x54, 0);
						int aLitHeight = mApp->ScaleNumUp(0x54, 0);
						int aLitY = mApp->ScaleNumUp(0x54, 0) * aPiece->mType;
						Rect aLitSrc(mApp->ScaleNum(aSource, 0), aLitY, aLitHeight, aLitWidth);
						aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
						aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
						g->DrawImage(IMAGE_GEM_ADD, aDestX, aDestY, aLitSrc);
						g->SetDrawMode(0);
					}
					g->SetColorizeImages(false);
				}
				else if (aPiece->m9C)
				{
					// a gem shrinking away (a flaming gem too): the prepared picture of its size
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
					int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
					int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
					g->DrawImage(mApp->mShrunkenGems[aPiece->mType][aPiece->mA0], aDestX, aDestY);
					g->SetColorizeImages(false);
				}
				else if (aPiece->mZ == 0.0f || !a3D)
				{
					// a gem on the board
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
					int aWidth = mApp->ScaleNum(0x54, 0);
					int aHeight = mApp->ScaleNum(0x54, 0);
					Rect aSrc(mApp->ScaleNum(aSource, 0), 0, aHeight, aWidth);
					int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
					int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
					g->DrawImage(GetImageById(aPiece->mType + 0x5c), aDestX, aDestY, aSrc);
					g->SetColorizeImages(false);
				}
				else if (aPiece->mZ < 1023.99)
				{
					// a gem off the board, nearer to the eye (mZ grows towards 1024): the display's transform scales
					// it about the middle of the screen and turns it
					float aScale = (float)(1024.0 / (1024.0 - (double)aPiece->mZ));
					SexyTransform2D aTransform;
					int aMiddle = -(mApp->ScaleNum(0x54, 0) / 2);
					aTransform.Translate((float)aMiddle, (float)aMiddle);
					aTransform.Scale(aScale, aScale);
					aTransform.RotateRad(aPiece->mRot);
					float aShiftY = (float)(((double)aPiece->mY + 42.0 - 384.0) * (double)aScale);
					aShiftY = (float)(384.0 + (double)aShiftY);
					float aTransY = mApp->ScaleNum(aShiftY, 0.0f);
					float aShiftX = (float)(((double)aPiece->mX + 42.0 - 512.0) * (double)aScale);
					aShiftX = (float)(512.0 + (double)aShiftX);
					float aTransX = mApp->ScaleNum(aShiftX, 0.0f);
					aTransform.Translate(aTransX, aTransY);
					int aWidth = mApp->ScaleNumUp(0x54, 0);
					int aHeight = mApp->ScaleNumUp(0x54, 0);
					Rect aSrc(mApp->ScaleNum(aSource, 0), 0, aHeight, aWidth);
					mApp->mDDInterface->BltTransformed(GetImageById(aPiece->mType + 0x5c), &g->mClipRect, Color(0xff, 0xff, 0xff), 0,
						aSrc, aTransform, true, 0.0f, 0.0f, false);
				}
			}

			// the star behind a flaming gem
			if (aPiece->m70)
			{
				if (a3D)
				{
					g->SetDrawMode(1);
					g->SetColorizeImages(true);
					double aFull = (double)anAlpha * 255.0;
					double aFlicker = fabs(mStarPhase + mStarPhase - 1.0);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol(aFlicker * ((double)mFadeIn * aFull))));
					int aStarY = FTol((double)mApp->ScaleNum((float)((double)aPiece->mY - 20.0), 0.0f));
					int aStarX = FTol((double)mApp->ScaleNum((float)((double)aPiece->mX - 20.0), 0.0f));
					g->DrawImageRotated(IMAGE_BIGSTAR, aStarX, aStarY, mStarRot, NULL);
					aFlicker = fabs(mStarPhase + mStarPhase - 1.0);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((1.0 - aFlicker) * ((double)mFadeIn * aFull))));
					aStarY = FTol((double)mApp->ScaleNum((float)((double)aPiece->mY - 20.0), 0.0f));
					aStarX = FTol((double)mApp->ScaleNum((float)((double)aPiece->mX - 20.0), 0.0f));
					g->DrawImageRotated(IMAGE_BIGSTAR, aStarX, aStarY, mStarDrift, NULL);
					g->SetColorizeImages(false);
				}
				else
				{
					if (!aStarDrawn)
					{
						// the two turning stars are added together in a picture once for the whole board
						if (m15f64 == NULL)
						{
							m15f64 = new MemoryImage(mApp);
							m15f64->Create(IMAGE_BIGSTAR->mWidth, IMAGE_BIGSTAR->mHeight);
							m15f64->SetImageMode(true, true);
							m15f64->SetVolatile(true);
						}
						m15f64->Clear();
						Graphics aStarGraphics(m15f64);
						aStarGraphics.SetColor(Color(0, 0, 0));
						aStarGraphics.FillRect(0, 0, IMAGE_BIGSTAR->mWidth, IMAGE_BIGSTAR->mHeight);
						aStarGraphics.SetDrawMode(1);
						aStarGraphics.SetColorizeImages(true);
						double aFull = (double)anAlpha * 255.0;
						double aFlicker = fabs(mStarPhase + mStarPhase - 1.0);
						aStarGraphics.SetColor(Color(0xff, 0xff, 0xff, FTol(aFlicker * ((double)mFadeIn * aFull))));
						aStarGraphics.DrawImageRotated(IMAGE_BIGSTAR, 0, 0, mStarRot, NULL);
						aFlicker = fabs(mStarPhase + mStarPhase - 1.0);
						aStarGraphics.SetColor(Color(0xff, 0xff, 0xff, FTol((1.0 - aFlicker) * ((double)mFadeIn * aFull))));
						aStarGraphics.DrawImageRotated(IMAGE_BIGSTAR, 0, 0, mStarDrift, NULL);
						aStarGraphics.SetColorizeImages(false);
						aStarGraphics.SetDrawMode(0);
						aStarDrawn = true;
					}
					g->SetDrawMode(1);
					int aStarY = FTol((double)mApp->ScaleNum((float)((double)aPiece->mY - 20.0), 0.0f));
					int aStarX = FTol((double)mApp->ScaleNum((float)((double)aPiece->mX - 20.0), 0.0f));
					g->DrawImage(m15f64, aStarX, aStarY);
				}
				g->SetDrawMode(0);
			}
		}
		else if (aType == 8)
		{
			// a bomb (its counter is written over it below)
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
			int aWidth = mApp->ScaleNum(0x54, 0);
			int aHeight = mApp->ScaleNum(0x54, 0);
			Rect aSrc(0, 0, aHeight, aWidth);
			int aDestY = FTol((double)mApp->ScaleNum((float)((double)aPiece->m6C + (double)aPiece->mY), 0.0f));
			int aDestX = FTol((double)mApp->ScaleNum((float)((double)aPiece->m68 + (double)aPiece->mX), 0.0f));
			g->DrawImage(IMAGE_BOMB, aDestX, aDestY, aSrc);
			g->SetColorizeImages(false);
		}
		else if (aType == 7)
		{
			// a rock
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
			int aWidth = mApp->ScaleNum(0x54, 0);
			int aHeight = mApp->ScaleNum(0x54, 0);
			Rect aSrc(0, 0, aHeight, aWidth);
			int aDestY = FTol((double)mApp->ScaleNum((float)((double)aPiece->m6C + (double)aPiece->mY), 0.0f));
			int aDestX = FTol((double)mApp->ScaleNum((float)((double)aPiece->m68 + (double)aPiece->mX), 0.0f));
			g->DrawImage(IMAGE_ROCK, aDestX, aDestY, aSrc);
			g->SetColorizeImages(false);
		}
		else if (aType == 9)
		{
			// a wildcard (the destroyer): its picture has 40 frames; it ripples while it fades in or out (mB8 is 1)
			int aFrame = (mUpdateCnt / 6) % 0x28;
			if (aPiece->mZ == 0.0f || !a3D)
			{
				if (g->mDestImage == (Image*)mWidgetManager->mImage && 0.0f < mFadeIn && a3D)
				{
					// the 3D game draws it as the 6 by 6 squares of the water mesh (two triangles each), so that it can
					// ripple
					TriVertex aVerts[98][3];
					int aTriangleCount = 0;
					float aBaseX = (float)((double)mApp->ScaleNum(aPiece->mX, 0.0f) + (double)g->mTransX);
					float aBaseY = (float)((double)mApp->ScaleNum(aPiece->mY, 0.0f) + (double)g->mTransY);
					float aShrink = 1.0f;
					if (aPiece->mB8 == 1)
					{
						double aLeft = 1.0 - (double)aPiece->mC0 * 0.25;
						aShrink = (float)((0.0 < aLeft) ? aLeft : 0.0);
					}
					double aFrameU = (double)aFrame / 40.0;
					for (int aMeshRow = 0; aMeshRow < 6; aMeshRow++)
					{
						for (int aMeshCol = 0; aMeshCol < 6; aMeshCol++)
						{
							WaterPoint* aPoint = &mWaterMesh[aMeshRow][aMeshCol];
							WaterPoint* aRight = &mWaterMesh[aMeshRow][aMeshCol + 1];
							WaterPoint* aDown = &mWaterMesh[aMeshRow + 1][aMeshCol];
							WaterPoint* aDownRight = &mWaterMesh[aMeshRow + 1][aMeshCol + 1];
							TriVertex* aTriangle = aVerts[aTriangleCount];
							aTriangleCount++;
							aTriangle[0].x = (float)((double)mApp->ScaleNum(aPoint->mX, 0.0f) + (double)aBaseX);
							aTriangle[0].y = (float)((double)mApp->ScaleNum(aPoint->mY, 0.0f) + (double)aBaseY);
							aTriangle[0].u = (float)((double)aPoint->mU + aFrameU);
							aTriangle[0].v = aPoint->mV;
							aTriangle[1].x = (float)((double)mApp->ScaleNum(aRight->mX, 0.0f) + (double)aBaseX);
							aTriangle[1].y = (float)((double)mApp->ScaleNum(aRight->mY, 0.0f) + (double)aBaseY);
							aTriangle[1].u = (float)((double)aRight->mU + aFrameU);
							aTriangle[1].v = aRight->mV;
							aTriangle[2].x = (float)((double)mApp->ScaleNum(aDownRight->mX, 0.0f) + (double)aBaseX);
							aTriangle[2].y = (float)((double)mApp->ScaleNum(aDownRight->mY, 0.0f) + (double)aBaseY);
							aTriangle[2].u = (float)((double)aDownRight->mU + aFrameU);
							aTriangle[2].v = aDownRight->mV;
							aTriangle = aVerts[aTriangleCount];
							aTriangleCount++;
							aTriangle[0].x = (float)((double)mApp->ScaleNum(aPoint->mX, 0.0f) + (double)aBaseX);
							aTriangle[0].y = (float)((double)mApp->ScaleNum(aPoint->mY, 0.0f) + (double)aBaseY);
							aTriangle[0].u = (float)((double)aPoint->mU + aFrameU);
							aTriangle[0].v = aPoint->mV;
							aTriangle[1].x = (float)((double)mApp->ScaleNum(aDownRight->mX, 0.0f) + (double)aBaseX);
							aTriangle[1].y = (float)((double)mApp->ScaleNum(aDownRight->mY, 0.0f) + (double)aBaseY);
							aTriangle[1].u = (float)((double)aDownRight->mU + aFrameU);
							aTriangle[1].v = aDownRight->mV;
							aTriangle[2].x = (float)((double)mApp->ScaleNum(aDown->mX, 0.0f) + (double)aBaseX);
							aTriangle[2].y = (float)((double)mApp->ScaleNum(aDown->mY, 0.0f) + (double)aBaseY);
							aTriangle[2].u = (float)((double)aDown->mU + aFrameU);
							aTriangle[2].v = aDown->mV;
						}
					}

					// a fading wildcard shrinks towards its middle
					if (aShrink != 1.0f)
					{
						float aMiddleX = (float)(mApp->ScaleNum(42.0, 0.0) + (double)aBaseX);
						float aMiddleY = (float)(mApp->ScaleNum(42.0, 0.0) + (double)aBaseY);
						if (0 < aTriangleCount)
						{
							for (int aTriangle = 0; aTriangle < aTriangleCount; aTriangle++)
							{
								for (int aVertex = 0; aVertex < 3; aVertex++)
								{
									TriVertex& aV = aVerts[aTriangle][aVertex];
									aV.x = (float)(((double)aV.x - (double)aMiddleX) * (double)aShrink + (double)aMiddleX);
									aV.y = (float)(((double)aV.y - (double)aMiddleY) * (double)aShrink + (double)aMiddleY);
								}
							}
						}
					}

					float aFade = 1.0f;
					if (aPiece->mB8 == 1)
					{
						double aDouble = (1.0 - (double)aPiece->mC0) + (1.0 - (double)aPiece->mC0);
						aFade = (float)((1.0 <= aDouble) ? 1.0 : aDouble);
					}
					mApp->mDDInterface->DrawTrianglesTex(IMAGE_WILDCARD_DESTROYER, aVerts, aTriangleCount, NULL,
						Color(0xff, 0xff, 0xff, FTol(((double)aFade * (double)anAlpha) * 255.0)), 0, 0.0f, 0.0f, true);
				}
				else
				{
					float aFade = 1.0f;
					if (aPiece->mB8 == 1)
						aFade = (float)(1.0 - (double)aPiece->mC0);
					g->SetColorizeImages(true);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol(((double)aFade * (double)anAlpha) * 255.0)));
					int aWidth = mApp->ScaleNum(0x54, 0);
					int aHeight = mApp->ScaleNum(0x54, 0);
					Rect aSrc(mApp->ScaleNum(aFrame * 0x54, 0), 0, aWidth, aHeight);
					int aDestY = FTol((double)mApp->ScaleNum(aPiece->mY, 0.0f));
					int aDestX = FTol((double)mApp->ScaleNum(aPiece->mX, 0.0f));
					g->DrawImage(IMAGE_WILDCARD_DESTROYER, aDestX, aDestY, aSrc);
					g->SetColorizeImages(false);
				}
			}
			else if (aPiece->mZ < 1023.99)
			{
				// off the board: scaled and turned like the other gems
				float aScale = (float)(1024.0 / (1024.0 - (double)aPiece->mZ));
				SexyTransform2D aTransform;
				int aMiddle = mApp->ScaleNum(-0x2a, 0);
				aTransform.Translate((float)aMiddle, (float)aMiddle);
				aTransform.Scale(aScale, aScale);
				aTransform.RotateRad(aPiece->mRot);
				float aShiftY = (float)(((double)aPiece->mY + 42.0 - 384.0) * (double)aScale);
				aShiftY = (float)(384.0 + (double)aShiftY);
				float aTransY = mApp->ScaleNum(aShiftY, 0.0f);
				float aShiftX = (float)(((double)aPiece->mX + 42.0 - 512.0) * (double)aScale);
				aShiftX = (float)(512.0 + (double)aShiftX);
				float aTransX = mApp->ScaleNum(aShiftX, 0.0f);
				aTransform.Translate(aTransX, aTransY);
				int aWidth = mApp->ScaleNum(0x54, 0);
				int aHeight = mApp->ScaleNum(0x54, 0);
				Rect aSrc(mApp->ScaleNum(aFrame * 0x54, 0), 0, aWidth, aHeight);
				mApp->mDDInterface->BltTransformed(IMAGE_WILDCARD_DESTROYER, &g->mClipRect, Color(0xff, 0xff, 0xff), 0, aSrc,
					aTransform, true, 0.0f, 0.0f, false);
			}
		}

		// the glow over the gem: nine lights (the eight around it and its middle), each a frame of the lighting picture
		if (!m165ac && 0.0f < mFadeIn)
		{
			if (aPiece->mType <= 8)
			{
				if (!aPiece->m9C && aPiece->mA4 == 0 && aPiece != mSwapPiece[0])
				{
					int aSourceX = 0;
					float* aGlowSlot = &aPiece->mGlow[0];
					for (;;)
					{
						if (0.01 < (double)*aGlowSlot)
						{
							double aLight = ((double)mFadeIn * (double)anAlpha) * (double)*aGlowSlot;
							double aScaled = aLight * 255.0;
							int aGrey = FTol((255.0 <= aScaled) ? 255.0 : aScaled);
							aGlowGraphics.SetColor(Color(aGrey, aGrey, aGrey));
							int aWidth = mApp->ScaleNum(0x54, 0);
							int aHeight = mApp->ScaleNum(0x54, 0);
							int aSourceY = mApp->ScaleNum(aType * 0x54, 0);
							Rect aSrc(mApp->ScaleNum(aSourceX, 0), aSourceY, aHeight, aWidth);
							int aDestY = FTol((double)mApp->ScaleNum((float)((double)aPiece->m6C + (double)aPiece->mY), 0.0f));
							int aDestX = FTol((double)mApp->ScaleNum((float)((double)aPiece->m68 + (double)aPiece->mX), 0.0f));
							aGlowGraphics.DrawImage(IMAGE_GEM_LIGHTING, aDestX, aDestY, aSrc);
						}
						aSourceX += 0x54;
						aGlowSlot++;
						if (0x2f3 < aSourceX)
							break;
					}
				}
			}
		}

		// the counter of a bomb
		if (aType == 8)
		{
			g->SetFont(FONT_BOMB);
			char aBuf[256];
			sprintf(aBuf, "%d", aPiece->m58);
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * 255.0)));
			std::string aWidthText = aBuf;
			std::string aText = aBuf;
			int aTextX = FTol((double)mApp->ScaleNum((float)((double)aPiece->mX + 42.0), 0.0f));
			int aTextY = FTol((double)mApp->ScaleNum((float)((double)aPiece->mY + 56.0), 0.0f));
			int aTextWidth = FONT_BOMB->StringWidth(aWidthText);
			g->DrawString(aText, aTextX - aTextWidth / 2, aTextY);
		}

		if (mFlipped)
			aPiece->mY = aSavedY;
	}

	// the hint: sparkles round the gem, and an arrow that bounces over it, on the gem and on the one it is swapped with
	Piece** aSlotPtr = &mBoard[0][0];
	for (int aHintRow = 0; aHintRow < 8; aHintRow++)
	{
		for (int aHintCol = 0; aHintCol < 8; aHintCol++)
		{
			Piece* aPiece = *aSlotPtr;
			aSlotPtr++;
			if (aPiece == NULL || aPiece->mHintTime <= 0)
				continue;

			for (int aPass = 0; aPass < 2; aPass++)
			{
				int anOffsetX;
				int anOffsetY;
				if (aPass == 1 && aPiece->mB0 == 0 && aPiece->mB4 == 0)
					break;
				if (aPass == 0)
					anOffsetX = 0;
				else
					anOffsetX = aPiece->mB0 * 0x54;
				int aHintX = FTol((double)aPiece->mX + (double)anOffsetX);
				if (aPass == 0)
					anOffsetY = 0;
				else
					anOffsetY = aPiece->mB4 * 0x54;
				int aHintY = FTol((double)aPiece->mY + (double)anOffsetY);
				g->SetColorizeImages(true);
				if (aPass == 0)
				{
					int aSize = mApp->ScaleNumUp(0x28, 0);
					g->SetDrawMode(1);
					double aShine = (double)aPiece->mHintTime * (double)0.1f;
					if (1.0 <= aShine)
						aShine = 1.0;
					float aShineAlpha = (float)(aShine * (double)anAlpha);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aShineAlpha * 255.0)));
					if (0x50 <= aPiece->mHintTime)
					{
						int aSparkle = aPiece->mHintFrame % 0xe;
						Rect aSrc(aSparkle * aSize, 0, aSize, aSize);
						g->DrawImage(IMAGE_SPARKLE, mApp->ScaleNum(aHintX + 0xd, 0), mApp->ScaleNum(aHintY - 7, 0), aSrc);
					}
					if (8 <= aPiece->mHintFrame && 0x28 <= aPiece->mHintTime)
					{
						int aSparkle = (aPiece->mHintFrame - 8) % 0xe;
						Rect aSrc(aSparkle * aSize, 0, aSize, aSize);
						g->DrawImage(IMAGE_SPARKLE, mApp->ScaleNum(aHintX - 10, 0), mApp->ScaleNum(aHintY + 3, 0), aSrc);
					}
					if (0x10 <= aPiece->mHintFrame && 0 <= aPiece->mHintTime)
					{
						int aSparkle = (aPiece->mHintFrame - 0x10) % 0xe;
						Rect aSrc(aSparkle * aSize, 0, aSize, aSize);
						g->DrawImage(IMAGE_SPARKLE, mApp->ScaleNum(aHintX + 5, 0), mApp->ScaleNum(aHintY + 0x13, 0), aSrc);
					}
				}

				// the arrow that points at the gem, in the last 200 frames of the hint
				int anArrowStart = HINT_TIME - 200;
				if (anArrowStart <= aPiece->mHintTime)
				{
					g->SetDrawMode(0);
					int aPhaseTicks = (anArrowStart - aPiece->mHintTime) + 200;
					int aFlight = aPiece->mHintTime - anArrowStart;
					float aPhase = (float)((double)aPhaseTicks * 0.125 + 0.0);
					float aFlightScale = (float)(((double)aFlight / 200.0) * 2.0 - 1.0);
					double aArrowFade = (1.0 - (double)fabsf(aFlightScale)) * 3.5;
					if (!(aArrowFade < 1.0))
						aArrowFade = 1.0;
					float anArrowAlpha = (float)aArrowFade;
					float aBounce = (float)(1.0 - (double)cosf(aPhase));
					float aPulse = (float)((double)aBounce * 0.5);
					int aLift = FTol((double)aPulse * -10.0);
					double anArrowFull = (double)anArrowAlpha * 255.0;
					int anArrowY = (aHintY - aLift) - 0x31;
					g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)anAlpha * anArrowFull)));
					g->DrawImage(IMAGE_HELP_ARROW, mApp->ScaleNum(aHintX, 0) + mApp->ScaleNum(-8, 0), mApp->ScaleNum(anArrowY, 0));
					g->SetDrawMode(1);
					g->SetColor(Color(0xff, 0xff, 0xff, FTol(((double)aPulse * anArrowFull) * (double)anAlpha)));
					int aGlowHeight = mApp->ScaleNumUp(0x28, 0);
					int aGlowWidth = mApp->ScaleNumUp(0x28, 0);
					Rect aGlowSrc(0, 0, aGlowHeight, aGlowWidth);
					g->DrawImage(IMAGE_HELP_INDICATOR_ARROW, mApp->ScaleNum(aHintX, 0) + mApp->ScaleNum(0x15, 0),
						mApp->ScaleNum(anArrowY, 0) + mApp->ScaleNum(0x14, 0), aGlowSrc);
				}
				g->SetColorizeImages(false);
			}
		}
	}

	// the boxes round the selected gem and the two being swapped
	g->SetDrawMode(0);
	Piece* aSelection[3];
	aSelection[0] = mSelectedPiece;
	aSelection[1] = mSwapPiece[0];
	aSelection[2] = mSwapPiece[1];
	for (int i = 0; i < 3; i++)
	{
		if (aSelection[i] != NULL)
		{
			g->SetColorizeImages(true);
			double aBoxAlpha;
			if (i == 0)
				aBoxAlpha = (double)anAlpha * 255.0;
			else
				aBoxAlpha = (double)mSwapFade * ((double)anAlpha * 255.0);
			g->SetColor(Color(0xff, 0xff, 0xff, FTol(aBoxAlpha)));
			int aBoxY = FTol((double)aSelection[i]->mY);
			int aWidth = mApp->ScaleNum(0x54, 0);
			int aHeight = mApp->ScaleNum(0x54, 0);
			Rect aSrc(mApp->ScaleNum(0, 0), 0, aWidth, aHeight);
			int aDestY = mApp->ScaleNum(aBoxY, 0);
			int aDestX = FTol((double)mApp->ScaleNum(aSelection[i]->mX, 0.0f));
			g->DrawImage(IMAGE_SELECT, aDestX, aDestY, aSrc);
			g->SetColorizeImages(false);
		}
	}

	// the glow of the power gem
	if (m17d0)
	{
		g->SetDrawMode(1);
		g->SetColorizeImages(true);
		float aGlowScale = (float)((double)m17e0 + (double)m17e0 - 1.0);
		double aGlowFade = (1.0 - (double)fabsf(aGlowScale)) * 4.0;
		if (!(aGlowFade < 1.0))
			aGlowFade = 1.0;
		float aPowerAlpha = (float)aGlowFade;
		g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)aPowerAlpha * 255.0)));
		int aPowerFrame = FTol((double)m17e0 * 30.0) % 10;
		int aWidth = mApp->ScaleNumUp(0xf0, 0);
		int aHeight = mApp->ScaleNumUp(0xf0, 0);
		Rect aSrc(mApp->ScaleNum(aPowerFrame * 0xf0, 0), 0, aHeight, aWidth);
		g->DrawImage(IMAGE_POWER_GLOW, mApp->ScaleNum(m17d8 - 0x4e, 0), mApp->ScaleNum(m17dc - 0x4e, 0), aSrc);
		g->SetColorizeImages(false);
		g->SetDrawMode(0);
	}

	// the tutorial puzzle's pointer: it fades out over the move that was made and in over the next one
	if (mIsPlanetIntro && mTutorialMoves.size() != 0)
	{
		g->SetColorizeImages(true);
		if (0.0f < m1944)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)m1944 * 255.0)));
			int aHeight = mApp->ScaleNumUp(0xb7, 0);
			int aWidth = mApp->ScaleNumUp(0xcc, 0);
			Rect aSrc(0, 0, aWidth, aHeight);
			TutorialMove& aMove = mTutorialMoves[mTutorialStep - 1];
			g->DrawImage(IMAGE_HELP_INDICATOR, mApp->ScaleNum(GetColX(aMove.mCol) - 0x3a, 0),
				mApp->ScaleNum(GetRowY(aMove.mRow) - 0x9e, 0), aSrc);
		}
		if (mTutorialStep < (int)mTutorialMoves.size())
		{
			g->SetColor(Color(0xff, 0xff, 0xff, FTol((double)m1948 * 255.0)));
			int aHeight = mApp->ScaleNumUp(0xb7, 0);
			int aWidth = mApp->ScaleNumUp(0xcc, 0);
			Rect aSrc(0, 0, aWidth, aHeight);
			TutorialMove& aMove = mTutorialMoves[mTutorialStep];
			g->DrawImage(IMAGE_HELP_INDICATOR, mApp->ScaleNum(GetColX(aMove.mCol) - 0x3a, 0),
				mApp->ScaleNum(GetRowY(aMove.mRow) - 0x9e, 0), aSrc);
		}
		g->SetColorizeImages(false);
	}
}
