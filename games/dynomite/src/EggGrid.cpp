#include "EggGrid.h"
#include "Dynomite.h"
#include "Effects.h"
#include "FossilizedApp.h"
#include "AngleTable.h"
#include "Rect.h"
#include "Sprite.h"

using namespace Sexy;

// 0x415770
EggGrid::EggGrid(FossilizedApp* theApp, Dynomite* theBoard)
{
	mBoard = theBoard;
	mApp = theApp;
	mCeiling = 0;
	mRows = 0;
	mField4688 = false;
	mLoneEgg = false; // port: not set by the original constructor
	GridCell* aCell = &mCells[0][0];
	for (int i = 0x5dc; i != 0; i--)
	{
		aCell->mBits = 0;
		aCell++;
	}
	GridDynamite** aSlot = &mDynamite[0][0];
	for (int i = 0x5dc; i != 0; i--)
	{
		*aSlot = NULL;
		aSlot++;
	}
	mDynamites.Clear();
	mMaxColor = 4;
	mField4690 = false;
	mField4691 = false;
	mField46d0 = true;
	mField46ac = true;
	mField46b4 = 200;
	mField46b0 = 200;
	mField46d1 = false;
	mField46e0 = 100;
	mField46e5 = false;
	mTargets.Clear();
	mList46b8.Clear();
}

// 0x417420
EggGrid::~EggGrid()
{
	// TODO: the lists' items
}

// 0x415890: the cell under a point of the grid
void EggGrid::PixelToCell(int x, int y, GridPoint* theCell)
{
	theCell->mY = (y + 0x10) / 0x21;
	theCell->mX = x;
	if ((theCell->mY % 2) != 0)
		theCell->mX = x - 0x10;
	theCell->mX = theCell->mX / 0x21;
}

// 0x4158f0
void EggGrid::ScreenToGrid(int x, int y, GridPoint* thePoint)
{
	thePoint->mX = (mBoard->mField6c - mBoard->mField70) + x;
	thePoint->mY = (int) ((float) y - mCeiling);
}

// 0x415920: a cell's corner in the grid
void EggGrid::CellToPixel(int theCol, int theRow, GridPoint* thePoint)
{
	thePoint->mY = theRow * 0x21;
	thePoint->mX = theCol * 0x21;
	if ((theRow % 2) != 0)
		thePoint->mX = theCol * 0x21 + 0x10;
}

// 0x415950
void EggGrid::GridToScreen(int x, int y, GridPoint* thePoint)
{
	thePoint->mX = (mBoard->mField70 - mBoard->mField6c) + x;
	thePoint->mY = (int) ((float) y + mCeiling);
}

// 0x415980: the deepest row with an egg (of the rows in use)
void EggGrid::CountRows()
{
	int aDeepest = 0;
	int aRow = 0;
	if (0.0f < mRows)
	{
		do
		{
			int aCol = 0;
			do
			{
				if ((mCells[aCol][aRow].mBits & 0xff) != 0)
				{
					if (aDeepest < aRow)
						aDeepest = aRow;
					break;
				}
				aCol++;
			} while (aCol < 0xf);
			aRow++;
		} while ((float) aRow < mRows);
	}
	mRows = (float) aDeepest;
}

// 0x4159f0
void EggGrid::ResetCeiling()
{
	mField4691 = false;
	mCeiling = -((mRows + 1.0f) * 33.0f + 16.0f);
}

// 0x4169c0: 1 the left wall, 2 the right wall, 4 the ceiling, a target's bits, 16 an egg
unsigned int EggGrid::Probe(int x, int y, bool theGhost)
{
	GridPoint aPoint;
	ScreenToGrid(x, y, &aPoint);
	int gx = aPoint.mX;
	int gy = aPoint.mY;
	if (gx < 9)
		return 1;
	if (mBoard->mField68 - 0x17 <= gx)
		return 2;
	if (gy < 1)
		return 4;
	mTargets.ResetIter();
	GridTarget* aTarget = mTargets.Next();
	while (aTarget != NULL)
	{
		if ((aTarget->mX1 <= x) && (x <= aTarget->mX2) && (aTarget->mY1 <= y) && (y <= aTarget->mY2))
		{
			if (!theGhost)
				aTarget->mHit = true;
			return aTarget->mHitBits;
		}
		aTarget = mTargets.Next();
	}
	PixelToCell(gx, gy, &aPoint);
	if ((mCells[aPoint.mX][aPoint.mY].mBits & 0xff) != 0)
	{
		CellToPixel(aPoint.mX, aPoint.mY, &aPoint);
		Rect aRect(aPoint.mX, aPoint.mY, 0x21, 0x21);
		if (aRect.Contains(gx, gy))
			return 0x10;
	}
	return 0;
}

// 0x415a20: x, y the cell's corner on the screen; confetti while a bonus multiplies, a bonus egg's prize, a baby
// (bit 0x100) or a yolk, egg white and the shell's four quarters
void EggGrid::Break(int x, int y, GridCell* theCell)
{
	int aColor = theCell->mBits & 0xff;
	if ((mBoard->mBonusBits & 0xe) != 0)
	{
		int i = 0x28;
		do
		{
			ConfettiEffect* aConfetti = new ConfettiEffect();
			aConfetti->Init(x + 0x10, y + 0x10, mApp);
			mBoard->mEffects.Add(aConfetti);
			i--;
		} while (i != 0);
	}
	if ((theCell->mBits & 0x1e00) != 0)
	{
		// port: the original leaves the target to the mode's slot 43 (the base's is empty)
		int aTargetX = 0;
		int aTargetY = 0;
		mBoard->GetPowerUpCenter(&aTargetX, &aTargetY);
		BonusItemEffect* anItem = new BonusItemEffect();
		anItem->Init(x + 0x10, y + 6, theCell->mBits >> 9 & 0xf, aTargetX, aTargetY, mBoard);
		mBoard->mEffects.Add(anItem);
		mApp->mSndPopbonus->Play();
	}
	if ((theCell->mBits & 0x100) == 0)
	{
		YolkEffect* aYolk = new YolkEffect();
		FossilizedApp* anApp = mApp;
		aYolk->Init(x + 0x10, y + 0x10, true, anApp->mRand.NextNoAssert() & 1, anApp);
		mBoard->mEffects.Add(aYolk);
	}
	else
	{
		BabyEffect* aBaby = new BabyEffect();
		FossilizedApp* anApp = mApp;
		aBaby->Init(x + 0x10, y + 6, anApp->mRand.NextNoAssert() % 3, anApp);
		mBoard->mEffects.Add(aBaby);
		// port: the original's app +0x1a18 + (rand & 1) * 4 picks one of the two loads of mama1
		if ((mApp->mRand.NextNoAssert() & 1) == 0)
			mApp->mSndMama1->Play();
		else
			mApp->mSndMama1_1a1c->Play();
		mBoard->mBabies = mBoard->mBabies + 1;
		mBoard->AddPoints(x + 0x10, y + 6, 0x19, 0, true);
	}
	int aCenterX = x + 0x10;
	int i = mApp->mRand.NextNoAssert() % 10 + 2;
	if (i != 0)
	{
		do
		{
			EggWhiteEffect* aWhite = new EggWhiteEffect();
			FossilizedApp* anApp = mApp;
			aWhite->Init(aCenterX, y + 0x10, anApp->mRand.NextNoAssert() % 0x168, anApp);
			mBoard->mEffects.Add(aWhite);
			i--;
		} while (i != 0);
	}
	int aCenterY = y + 0x10;
	ShellEffect* aShell = new ShellEffect();
	aShell->Init(aCenterX, aCenterY, -0x2d, aColor, 0, mApp);
	mBoard->mEffects.Add(aShell);
	aShell = new ShellEffect();
	aShell->Init(aCenterX, aCenterY, 0x2d, aColor, 1, mApp);
	mBoard->mEffects.Add(aShell);
	aShell = new ShellEffect();
	aShell->Init(aCenterX, aCenterY, 0xe1, aColor, 2, mApp);
	mBoard->mEffects.Add(aShell);
	aShell = new ShellEffect();
	aShell->Init(aCenterX, aCenterY, 0x87, aColor, 3, mApp);
	mBoard->mEffects.Add(aShell);
}

// 0x416590: an egg of theColor (and bonus theKind) at a cell; a stick of dynamite there when it touches an egg of
// its colour that has none
void EggGrid::Place(int theCol, int theRow, int theColor, int theKind)
{
	GridCell* aCell = &mCells[theCol][theRow];
	aCell->mBits = (aCell->mBits & 0xffffff00) | (theColor & 0xff);
	aCell->mBits = (theKind & 0xf) << 9 | (aCell->mBits & 0xffffe0ff);
	if (mRows < (float) theRow)
		mRows = (float) theRow;
	if (mMaxColor < theColor)
		mMaxColor = theColor;
	GridPoint aPoint;
	CellToPixel(theCol, theRow, &aPoint);
	unsigned int aColor = (unsigned int) theColor;
	int aSame = 0;
	if ((0 < theCol) && ((mCells[theCol - 1][theRow].mBits & 0xff) == aColor) && (mDynamite[theCol - 1][theRow] == NULL))
		aSame = 1;
	if ((theCol < 0xe) && ((mCells[theCol + 1][theRow].mBits & 0xff) == aColor) && (mDynamite[theCol + 1][theRow] == NULL))
		aSame++;
	if ((theRow % 2) == 0)
	{
		if (0 < theRow)
		{
			if ((0 < theCol) && ((mCells[theCol - 1][theRow - 1].mBits & 0xff) == aColor) && (mDynamite[theCol - 1][theRow - 1] == NULL))
				aSame++;
			if (((mCells[theCol][theRow - 1].mBits & 0xff) == aColor) && (mDynamite[theCol][theRow - 1] == NULL))
				aSame++;
		}
		if (theRow <= 0x62)
		{
			if ((0 < theCol) && ((mCells[theCol - 1][theRow + 1].mBits & 0xff) == aColor) && (mDynamite[theCol - 1][theRow + 1] == NULL))
				aSame++;
			if (((mCells[theCol][theRow + 1].mBits & 0xff) == aColor) && (mDynamite[theCol][theRow + 1] == NULL))
				aSame++;
		}
	}
	else
	{
		if (0 < theRow)
		{
			if (((mCells[theCol][theRow - 1].mBits & 0xff) == aColor) && (mDynamite[theCol][theRow - 1] == NULL))
				aSame++;
			if ((theCol < 0xe) && ((mCells[theCol + 1][theRow - 1].mBits & 0xff) == aColor) && (mDynamite[theCol + 1][theRow - 1] == NULL))
				aSame++;
		}
		if (theRow <= 0x62)
		{
			if (((mCells[theCol][theRow + 1].mBits & 0xff) == aColor) && (mDynamite[theCol][theRow + 1] == NULL))
				aSame++;
			if ((theCol < 0xe) && ((mCells[theCol + 1][theRow + 1].mBits & 0xff) == aColor) && (mDynamite[theCol + 1][theRow + 1] == NULL))
				aSame++;
		}
	}
	if ((aSame != 0) && (theColor < 0x12))
	{
		mLoneEgg = false;
		// the old stick leaves the list but is not deleted
		if (mDynamite[theCol][theRow] != NULL)
			mDynamites.Remove(mDynamite[theCol][theRow]);
		GridDynamite* aDynamite = new GridDynamite(this, aPoint.mX + 0x10, aPoint.mY);
		mDynamites.Add(aDynamite);
		mDynamite[theCol][theRow] = aDynamite;
	}
}

// 0x417bd0 (TODO)
void EggGrid::Match(int theCol, int theRow, int theColor)
{
}

// 0x418100 (TODO)
void EggGrid::Blast(int theCol, int theRow)
{
}

// 0x438270
GridDynamite::GridDynamite(EggGrid* theGrid, int x, int y)
{
	mX = (float) x;
	mGrid = theGrid;
	mTime = 0;
	mY = (float) y;
	unsigned int aRand = theGrid->mApp->mRand.NextNoAssert() % 3;
	if (aRand == 0)
	{
		mAngle = 0x13b;
		mWhich = 0;
	}
	else if (aRand == 1)
	{
		mAngle = 0x14f;
		mWhich = 1;
	}
	else if (aRand == 2)
	{
		mAngle = 5;
		mWhich = 2;
	}
	mSpark = 0;
	mDX = theGrid->mApp->mAngles->mDX[mAngle];
	mDY = theGrid->mApp->mAngles->mDY[mAngle];
}

// 0x438200
GridDynamite::~GridDynamite()
{
}

// 0x438210
void GridDynamite::Update()
{
	mTime = (float) (mTime + 0.25);
	if (mTime <= 5.0f)
	{
		mX = mDX + mX;
		mY = mDY + mY;
	}
	else
		mTime = 5.0f;
	mSpark = mSpark + 0.15f;
	if (3.0f <= mSpark)
		mSpark = 0;
}

// 0x438350: the stick, and the spark 29 pixels along at its fuse
void GridDynamite::Draw()
{
	GridPoint aPoint;
	mGrid->GridToScreen((int) mX, (int) mY, &aPoint);
	Cell* aCell = mGrid->mApp->mDynamiteCells[mWhich];
	aCell->Draw(aPoint.mX - (aCell->mWidth >> 1), aPoint.mY - (aCell->mHeight >> 1));
	aCell = mGrid->mApp->mSparkCells[(int) mSpark];
	aCell->Draw((int) (mDX * 29.0f + aPoint.mX) - (aCell->mWidth >> 1), (int) (mDY * 29.0f + aPoint.mY) - (aCell->mHeight >> 1));
}
