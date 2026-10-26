#include "Egg.h"
#include "Dynomite.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Sprite.h"
#include <cmath>

using namespace Sexy;

// port: the original reads the cell at app +0x13c0 + theColor * 4, where the bomb's cell is followed by the egg
// sheet's twenty
static Cell* EggCell(FossilizedApp* theApp, int theColor)
{
	if (theColor == 0)
		return theApp->mPicBomb_c0;
	return theApp->mEggCells[theColor - 1];
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
	mField20 = true;
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
	int aMiddle = mBoard->mField70;
	if (((aMiddle < aStartX) && (mX < (float) aMiddle)) || ((aStartX < aMiddle) && ((float) aMiddle < mX)))
		mField20 = false;
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
		mRow = -1;
		mCol = -1;
		return false;
	}
	return true;
}

// 0x414d60
void FlyingEgg::Land()
{
	// TODO: back up to a free cell of the grid and put the egg there (0x4158f0, 0x415890, 0x4169c0, 0x418100,
	// 0x416590, 0x417bd0; board slots 56 and 61)
}
