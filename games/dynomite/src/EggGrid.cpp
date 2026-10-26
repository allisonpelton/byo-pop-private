#include "EggGrid.h"
#include "Dynomite.h"
#include "Rect.h"

using namespace Sexy;

// 0x415770
EggGrid::EggGrid(FossilizedApp* theApp, Dynomite* theBoard)
{
	mBoard = theBoard;
	mApp = theApp;
	mCeiling = 0;
	mRows = 0;
	mField4688 = false;
	GridCell* aCell = &mCells[0][0];
	for (int i = 0x5dc; i != 0; i--)
	{
		aCell->mBits = 0;
		aCell++;
	}
	aCell = &mCells2[0][0];
	for (int i = 0x5dc; i != 0; i--)
	{
		aCell->mBits = 0;
		aCell++;
	}
	mList466c.Clear();
	mField10 = 4;
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

// 0x415a20
void EggGrid::Break(int x, int y, GridCell* theCell)
{
	// TODO
}
