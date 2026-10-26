// The path finder (see PathFinder.h).

#include "PathFinder.h"
#include "AtomicaApp.h"
#include "Board.h"
#include "Grid.h"
#include "Graphics.h"
#include <cstdlib>

using namespace Sexy;

// 0x424900
PathFinder::PathFinder(Grid* theGrid) :
	mGrid(theGrid)
{
	for (int i = 0; i < 50; i++)
		mWayCol[i] = mWayRow[i] = 0;
	Reset();
}

PathFinder::~PathFinder()
{
	Reset();
}

// 0x424880
void PathFinder::Reset()
{
	mD34 = -1;
	mD38 = -1;
	mB3c = false;
	mPath.Clear();
	mPoints.Clear();
}

// 0x4248b0
bool PathFinder::Contains(int theCol, int theRow)
{
	mPath.Reset();
	int* aNode = mPath.Next();
	while (aNode != NULL)
	{
		if ((aNode[0] == theCol) && (aNode[1] == theRow))
			return true;
		aNode = mPath.Next();
	}
	return false;
}

// 0x4246a0: from a cell along the four straight lines, turning in every free cell (but only toward cells whose cost
// is lower than what reached them before); the route is the cells where it turned
bool PathFinder::Recurse(int theCol, int theRow, int theTargetCol, int theTargetRow, int theWays, int theMaxWays, int theCost,
	int* theBuffer)
{
	static const int aDirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
	bool aFound = false;
	for (int i = 0; i < 4; i++)
	{
		int aDX = aDirs[i][0];
		int aDY = aDirs[i][1];
		int aCol = aDX + theCol;
		int aRow = theRow + aDY;
		int aStep = 2;
		if (aCol >= 0)
		{
			int aBase = theCost + 2;
			do
			{
				if ((aRow < 0) || (aCol > 8) || (aRow > 7))
					break;
				mSearches++;
				if ((aCol == theTargetCol) && (aRow == theTargetRow))
				{
					if ((theBuffer != NULL) && (aStep + theCost < mBestCost))
					{
						mBestWays = theWays;
						for (int k = 0; k < theMaxWays; k++)
						{
							mWayCol[k] = theBuffer[k * 2];
							mWayRow[k] = theBuffer[k * 2 + 1];
						}
						mBestCost = aStep + theCost;
					}
					return true;
				}
				if (aCol >= 0)
				{
					Atom* anAtom = mGrid->mCells[aCol][aRow];
					if ((anAtom != NULL) && (!anAtom->mPre || ((mGrid->mApp->mMode == 0) && anAtom->mPreStarted)) &&
						(mGrid->mApp->mBoard->mI198 < 1))
						break;
				}
				if (theWays < theMaxWays)
				{
					if (theBuffer != NULL)
					{
						theBuffer[theWays * 2] = aCol;
						theBuffer[theWays * 2 + 1] = aRow;
					}
					if (aBase < mCost[aCol][aRow])
					{
						mCost[aCol][aRow] = aBase;
						if (Recurse(aCol, aRow, theTargetCol, theTargetRow, theWays + 1, theMaxWays, aBase, theBuffer))
						{
							if (theBuffer == NULL)
								return true;
							aFound = true;
						}
					}
				}
				aCol += aDX;
				aRow += aDY;
				aBase++;
				aStep++;
			} while (aCol >= 0);
		}
	}
	return aFound;
}

// 0x4249d0
bool PathFinder::Search(int theCol1, int theRow1, int theCol2, int theRow2, int theWantRoute, int theMaxWays, int theBuf[])
{
	for (int aCol = 0; aCol < 9; aCol++)
		for (int aRow = 0; aRow < 8; aRow++)
			mCost[aCol][aRow] = 999999;
	mBestCost = 100000;
	mBestWays = -1;
	mSearches = 0;
	if (!Recurse(theCol1, theRow1, theCol2, theRow2, 0, theMaxWays, 0, theWantRoute ? theBuf : NULL))
		return false;
	if (theWantRoute != 0)
	{
		if (mBestWays == -1)
			return false;
		mLength = 0;
		int aPrevX = theCol1 * 0x20 + 9;
		int aPrevY = theRow1 * 0x20 + 0x19;
		mPoints.Clear();
		for (int k = 0; k <= mBestWays; k++)
		{
			int aCol = (k == mBestWays) ? theCol2 : mWayCol[k];
			int aRow = (k == mBestWays) ? theRow2 : mWayRow[k];
			int aX = aCol * 0x20 + 9;
			int aY = aRow * 0x20 + 0x19;
			mLength += abs(aY - aPrevY) + abs(aX - aPrevX);
			int* aPoint = new int[2];
			aPoint[0] = aX;
			aPoint[1] = aY;
			mPoints.Add(aPoint);
			aPrevX = aX;
			aPrevY = aY;
		}
	}
	return true;
}

// 0x424e80: the route from one cell to another, as the cells of it (each with the shape of the turn there) for the
// atom to follow and for the arrows
bool PathFinder::Find(int theCol1, int theRow1, int theCol2, int theRow2)
{
	int aBuf[100];
	mPath.Clear();
	mPoints.Clear();
	Search(theCol1, theRow1, theCol2, theRow2, 1, 0x32, aBuf);
	if (mBestWays < 0)
		return false;
	mWayCol[mBestWays] = theCol2;
	mWayRow[mBestWays] = theRow2;

	int aCol = theCol1;
	int aRow = theRow1;
	int aNextCol = theCol1;
	int aNextRow = theRow1;
	int aPrevDX = 0;
	int aPrevDY = 0;
	bool aDone = false;
	int aDX = 0;
	int aDY = 0;
	int aWay = 0;
	bool aBlocked;
	if ((theCol2 < 0) || (theCol2 > 8) || (theRow2 < 0) || (theRow2 > 7))
		aBlocked = true;
	else
	{
		Atom* anAtom = mGrid->mCells[theCol2][theRow2];
		aBlocked = false;
		if ((anAtom != NULL) && (!anAtom->mPre || ((mGrid->mApp->mMode == 0) && anAtom->mPreStarted)))
			aBlocked = true;
	}
	while (!aBlocked)
	{
		if ((aCol == theCol2) && (aRow == theRow2))
			aDone = true;
		else
		{
			if ((aCol == aNextCol) && (aRow == aNextRow))
			{
				aNextCol = mWayCol[aWay];
				aNextRow = mWayRow[aWay];
				aWay++;
			}
			aDX = 0;
			aDY = 0;
			if (aCol < aNextCol)
				aDX = 1;
			else if (aNextCol < aCol)
				aDX = -1;
			if (aRow < aNextRow)
				aDY = 1;
			else if (aNextRow < aRow)
				aDY = -1;
		}
		int* aNode = new int[4];
		aNode[0] = aCol;
		aNode[1] = aRow;
		aNode[2] = 0;
		aNode[3] = 0;
		aCol += aDX;
		aRow += aDY;
		int aShape = 0;
		if ((aDX == 0) || (aPrevDX == 0))
		{
			if ((aDY == 0) || (aPrevDY == 0))
			{
				if (((aDX < 1) || (aPrevDY >= 0)) && ((aPrevDX >= 0) || (aDY < 1)))
				{
					if (((aDX < 1) || (aPrevDY < 1)) && ((aPrevDX >= 0) || (aDY >= 0)))
					{
						if (((aDX < 0) && (aPrevDY < 0)) || ((aPrevDX > 0) && (aDY > 0)))
							aShape = 3;
						else if (((aDX < 0) && (aPrevDY > 0)) || ((aPrevDX > 0) && (aDY < 0)))
							aShape = 5;
					}
					else
						aShape = 6;
				}
				else
					aShape = 4;
			}
			else
				aShape = 2;
		}
		else
			aShape = 1;
		int aDirection = 0;
		if (aDX < 0)
			aDirection = 3;
		else if (aDX < 1)
		{
			if (aDY < 0)
				aDirection = 1;
			else if (0 < aDY)
				aDirection = 2;
		}
		else
			aDirection = 4;
		if (aDone)
		{
			aNode[3] = aDirection;
			aNode[2] = 0;
		}
		else
		{
			aNode[2] = aShape;
			aNode[3] = 0;
		}
		if (aShape != 0)
			mPath.Add(aNode);
		else
			delete[] aNode;
		aPrevDX = aDX;
		aPrevDY = aDY;
		if (aDone)
			break;
	}
	return !aBlocked;
}

// 0x424b40: an arrow cell of the route, in the middle of its cell
void PathFinder::DrawNode(int* theNode, int theX, int theY, Graphics* g)
{
	g->SetColor(Color(200, 0xff, 200, 100));
	switch (theNode[2])
	{
	case 1:
		g->FillRect(theX - 0x18, theY - 10, 8, 0x14);
		g->FillRect(theX - 0xe, theY - 10, 8, 0x14);
		g->FillRect(theX - 4, theY - 10, 8, 0x14);
		g->FillRect(theX + 6, theY - 10, 8, 0x14);
		g->FillRect(theX + 0x10, theY - 10, 8, 0x14);
		break;
	case 2:
		g->FillRect(theX - 10, theY - 0x18, 0x14, 8);
		g->FillRect(theX - 10, theY - 0xe, 0x14, 8);
		g->FillRect(theX - 10, theY - 4, 0x14, 8);
		g->FillRect(theX - 10, theY + 6, 0x14, 8);
		g->FillRect(theX - 10, theY + 0x10, 0x14, 8);
		break;
	case 3:
		g->FillRect(theX - 0x18, theY - 10, 8, 0x14);
		g->FillRect(theX - 10, theY + 0x10, 0x14, 8);
		g->FillRect(theX - 0xe, theY - 10, 0x18, 0x14);
		break;
	case 4:
		g->FillRect(theX - 10, theY + 0x10, 0x14, 8);
		g->FillRect(theX + 0x10, theY - 10, 8, 0x14);
		g->FillRect(theX - 10, theY - 10, 0x18, 0x14);
		break;
	case 5:
		g->FillRect(theX - 10, theY - 0x18, 0x14, 8);
		g->FillRect(theX - 0x18, theY - 10, 8, 0x14);
		g->FillRect(theX - 0xe, theY - 10, 0x18, 0x14);
		break;
	case 6:
		g->FillRect(theX - 10, theY - 0x18, 0x14, 8);
		g->FillRect(theX + 0x10, theY - 10, 8, 0x14);
		g->FillRect(theX - 10, theY - 10, 0x18, 0x14);
		break;
	default:
		break;
	}
	Point aPoints[3];
	switch (theNode[3])
	{
	case 1:
		aPoints[1] = Point(theX - 0xf, theY + 0x1a);
		aPoints[2] = Point(theX + 0xf, theY + 0x1a);
		break;
	case 2:
		aPoints[1] = Point(theX - 0xf, theY - 0x1a);
		aPoints[2] = Point(theX + 0xf, theY - 0x1a);
		break;
	case 3:
		aPoints[1] = Point(theX + 0x1a, theY + 0xf);
		aPoints[2] = Point(theX + 0x1a, theY - 0xf);
		break;
	case 4:
		aPoints[1] = Point(theX - 0x1a, theY + 0xf);
		aPoints[2] = Point(theX - 0x1a, theY - 0xf);
		break;
	default:
		return;
	}
	aPoints[0] = Point(theX, theY);
	g->PolyFill(aPoints, 3);
}
