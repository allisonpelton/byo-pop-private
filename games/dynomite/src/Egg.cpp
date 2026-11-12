#include "Egg.h"
#include "Dynomite.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Sprite.h"
#include <cmath>

using namespace Sexy;

// port: the original reads the cell at app +0x13c0 + theColor * 4, where the bomb's cell is followed by the egg
// sheet's twenty
Cell* Sexy::EggCell(FossilizedApp* theApp, int theColor)
{
	if (theColor == 0)
		return theApp->mPicBomb_c0;
	return theApp->mEggCells[theColor - 1];
}

// 0x415600
void Sexy::DrawCellAt(Cell* theCell, float x, float y)
{
	int aHeight = theCell->mHeight;
	int aY = (int) y - (aHeight >> 1);
	int aWidth = theCell->mWidth;
	theCell->Draw((int) x - (aWidth >> 1), aY);
}

// 0x4155a0
Egg::Egg()
{
	mKind = 0;
	mSparkle = 0;
}

// 0x4155c0
Egg::~Egg()
{
}

// 0x4155f0
bool Egg::Update()
{
	return false;
}

// 0x4155d0
void Egg::Init(FossilizedApp* theApp, Dynomite* theBoard, int theColor)
{
	mApp = theApp;
	mBoard = theBoard;
	mColor = theColor;
}

// 0x415660: the egg (or the bomb), its bonus picture, and the bomb's fuse sparks
void Egg::Draw()
{
	float x = mX;
	float y = mY;
	Cell* aCell = EggCell(mApp, mColor);
	aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
	if (mKind != 0)
	{
		// port: the original's app +0x1830 + mKind * 4 is the bonus egg sheet's cell mKind - 1
		aCell = mApp->mBonusEggCells[mKind - 1];
		aCell->Draw((int) x - (aCell->mWidth >> 1), (int) y - (aCell->mHeight >> 1));
	}
	if (mColor == 0)
	{
		mSparkle = mSparkle + 0.2f;
		if (3.0f <= mSparkle)
			mSparkle = 0;
		aCell = mApp->mSparkCells[(int) mSparkle];
		aCell->Draw((int) (mX - 23.0f) - (aCell->mWidth >> 1), (int) (mY - 30.0f) - (aCell->mHeight >> 1));
	}
}

// 0x401000
LoadedEgg::LoadedEgg()
{
}

// 0x401020
LoadedEgg::~LoadedEgg()
{
}

// 0x414d10
FlyingEgg::FlyingEgg()
{
	mInFrontOfSling = true;
	mGhost = false;
}

// 0x414d30
FlyingEgg::~FlyingEgg()
{
}

// 0x4150b0: up to ten steps a frame, feeling the grid at eight points around the egg; the walls (bits 1 and 2)
// bounce it, an egg or the ceiling (bits 4 and 16) stops it, and bit 8 (the bomb's?) breaks what it touched
bool FlyingEgg::Update()
{
	int aStartX = (int) mX;
	unsigned int aHits = 0;
	int i = 0;
	do
	{
		mX = mVX + mX;
		mY = mY + mVY;
		EggGrid* aGrid = mBoard->mGrid;
		aHits |= aGrid->Probe((int) mX, (int) (mY - 15.0f), mGhost);
		aHits |= aGrid->Probe((int) mX - 7, (int) (mY - 9.0f), mGhost);
		aHits |= aGrid->Probe((int) mX + 7, (int) (mY - 9.0f), mGhost);
		aHits |= aGrid->Probe((int) mX - 9, (int) (mY - 1.0f), mGhost);
		aHits |= aGrid->Probe((int) mX + 9, (int) (mY - 1.0f), mGhost);
		aHits |= aGrid->Probe((int) mX - 5, (int) (mY + 5.0f), mGhost);
		aHits |= aGrid->Probe((int) mX + 5, (int) (mY + 5.0f), mGhost);
		aHits |= aGrid->Probe((int) mX, (int) (mY + 7.0f), mGhost);
		if ((aHits & 1) != 0)
			mVX = fabsf(mVX);
		if ((aHits & 2) != 0)
			mVX = -fabsf(mVX);
		if ((aHits & 0x14) != 0)
			break;
		if (mGhost == true)
			break;
		i++;
	} while (i < 10);
	int aMiddle = mBoard->mPlayMiddleX;
	if (((aMiddle < aStartX) && (mX < (float) aMiddle)) || ((aStartX < aMiddle) && ((float) aMiddle < mX)))
		mInFrontOfSling = false;
	if (((aHits & 4) != 0) || ((aHits & 0x10) != 0))
	{
		Land();
		return false;
	}
	if (((aHits & 8) != 0) && !mGhost && (mColor != 0))
	{
		mApp->mSndBreakegg->Play();
		// the original builds the cell in the stack slot that held the launcher's middle (250), keeping its
		// bits from 13 up (zero)
		GridCell aCell;
		aCell.mBits = (unsigned int) (mColor & 0xff);
		mBoard->mGrid->Break((int) (mX - 16.0f), (int) (mY - 16.0f), &aCell);
		mCol = -1;
		mRow = -1;
		return false;
	}
	return true;
}

// 0x414d60: backs out along its path (fifteen steps at a time, bouncing off the walls) until its cell is free,
// then a bomb blasts there and an egg is placed and matched
void FlyingEgg::Land()
{
	GridPoint aPoint;
	GridPoint aCell;
	mBoard->mGrid->ScreenToGrid((int) mX, (int) mY, &aPoint);
	EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
	char anEgg = (char) mBoard->mGrid->mCells[aCell.mX][aCell.mY].mBits;
	while (anEgg != 0)
	{
		unsigned int aHits = 0;
		int i = 0;
		do
		{
			mX = mX - mVX;
			mY = mY - mVY;
			EggGrid* aGrid = mBoard->mGrid;
			aHits |= aGrid->Probe((int) mX, (int) (mY - 24.0f), false);
			aHits |= aGrid->Probe((int) mX - 10, (int) (mY - 18.0f), false);
			aHits |= aGrid->Probe((int) mX + 10, (int) (mY - 18.0f), false);
			aHits |= aGrid->Probe((int) mX - 0x10, (int) (mY - 1.0f), false);
			aHits |= aGrid->Probe((int) mX + 0x10, (int) (mY - 1.0f), false);
			aHits |= aGrid->Probe((int) mX - 9, (int) (mY + 9.0f), false);
			aHits |= aGrid->Probe((int) mX + 9, (int) (mY + 9.0f), false);
			aHits |= aGrid->Probe((int) mX, (int) (mY + 12.0f), false);
			if ((aHits & 1) != 0)
				mVX = fabsf(mVX);
			if ((aHits & 2) != 0)
				mVX = -fabsf(mVX);
		} while ((mY <= 570.0f) && (++i < 0xf));
		mBoard->mGrid->ScreenToGrid((int) mX, (int) mY, &aPoint);
		EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
		anEgg = (char) mBoard->mGrid->mCells[aCell.mX][aCell.mY].mBits;
	}
	mCol = aCell.mX;
	mRow = aCell.mY;
	if (mGhost != true)
	{
		if (mColor == 0)
			mBoard->mGrid->Blast(aCell.mX, aCell.mY);
		else
		{
			mApp->mSndEgglands->Play();
			mBoard->mGrid->mLoneEgg = true;
			mBoard->mGrid->Place(aCell.mX, aCell.mY, mColor, mKind);
			mBoard->mGrid->Match(aCell.mX, aCell.mY, mColor);
			if (mBoard->mGrid->mLoneEgg == true)
			{
				mBoard->EggAlone(mColor);
				mBoard->EggLanded();
				return;
			}
		}
		mBoard->EggLanded();
	}
}
