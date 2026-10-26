#include "Fossil.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Sprite.h"

using namespace Sexy;

// 0x413cd0
Fossil::Fossil(FossilizedApp* theApp)
{
	mX = 0;
	mY = 0;
	mApp = theApp;
	mPic = NULL;
	mFragment = NULL;
	mGlow = NULL;
	mWhole = NULL;
	Free();
}

// 0x413d10
Fossil::~Fossil()
{
	Free();
}

// 0x413940
void Fossil::Free()
{
	if (mFragment != NULL)
	{
		delete mFragment;
		mFragment = NULL;
	}
	if (mGlow != NULL)
	{
		delete mGlow;
		mGlow = NULL;
	}
	if (mWhole != NULL)
	{
		delete mWhole;
		mWhole = NULL;
	}
}

// 0x413990
void Fossil::SetFossil(int theFossil)
{
	Free();
	mPic = mApp->mFossils[theFossil - 1];
	mFragment = new MaskedPic(*mPic);
	mWhole = new MaskedPic(*mPic);
	mWhole->AlphaFromColour(7);
}

// 0x413a50
void Fossil::SetPos(int x, int y)
{
	mX = x;
	mY = y;
}

// 0x413a70: the walk goes along the fragment's row in steps of a cell (33 pixels) from its left, and from each step
// up and to the left a row at a time (33 up, 16 left, the grid's offset rows); a cell whose middle and four points
// around it (10 left and right, 15 up and down) are mostly in the fragment becomes rock
void Fossil::Place(EggGrid* theGrid, int theCol, int theRow)
{
	int aX = mX;
	int aY = mY;
	GridPoint aPixel;
	EggGrid::CellToPixel(theCol, theRow, &aPixel);
	while (aX >= 0x21)
	{
		aX -= 0x21;
		aPixel.mX -= 0x21;
	}
	unsigned char* aBits = (unsigned char*) mFragment->GetBits();
	while (aX < mPic->mWidth * 2)
	{
		int aPixelX = aPixel.mX;
		int aPixelY = aPixel.mY;
		int aPicY = aY;
		int aPicX = aX;
		while (aPicY > 0xf)
		{
			if ((aPicY > 0xf) && (aPicY < mFragment->mHeight - 0xf) && (aPicX > 10) &&
				(aPicX < mFragment->mWidth - 10))
			{
				int aWidth = mFragment->mWidth;
				unsigned char* aPoint = aBits + (aWidth * aPicY + aPicX) * 4;
				int aCount = (aPoint[3] > 10) ? 1 : 0;
				if (aPoint[0x2b] > 10)
					aCount++;
				if (aPoint[-0x25] > 10)
					aCount++;
				if (aBits[((aPicY + 0xf) * aWidth + aPicX) * 4 + 3] > 10)
					aCount++;
				if (aBits[((aPicY - 0xf) * aWidth + aPicX) * 4 + 3] > 10)
					aCount++;
				if (aCount > 3)
				{
					GridPoint aCell;
					EggGrid::PixelToCell(aPixelX, aPixelY, &aCell);
					if (theGrid->mDynamite[aCell.mX][aCell.mY] != NULL)
					{
						theGrid->mDynamites.Remove(theGrid->mDynamite[aCell.mX][aCell.mY]);
						if (theGrid->mDynamite[aCell.mX][aCell.mY] != NULL)
							delete theGrid->mDynamite[aCell.mX][aCell.mY];
						theGrid->mDynamite[aCell.mX][aCell.mY] = NULL;
					}
					theGrid->mCells[aCell.mX][aCell.mY].mBits =
						(theGrid->mCells[aCell.mX][aCell.mY].mBits & 0xffffff00) | 0x33;
				}
			}
			aPicY -= 0x21;
			aPixelX -= 0x10;
			aPixelY -= 0x21;
			aPicX -= 0x10;
		}
		aPixel.mX += 0x21;
		aX += 0x21;
	}
	theGrid->mCells[theCol][theRow].mBits = (theGrid->mCells[theCol][theRow].mBits & 0xffffff00) | 0x32;
}

// 0x413c80
void Fossil::MakeGlowFor(const Color& theColor)
{
	if (mGlow != NULL)
	{
		delete mGlow;
		mGlow = NULL;
	}
	mGlow = mFragment->MakeGlow(theColor, 8);
}
