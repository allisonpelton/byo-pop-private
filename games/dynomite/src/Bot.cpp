// port: the bot behind the `move` test command (Bot.h)
#include <cmath>

#include "AngleTable.h"
#include "Bot.h"
#include "Dynomite.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Launcher.h"

using namespace Sexy;

namespace
{

const int kProbes[8][2] = { { 0, -15 }, { -7, -9 }, { 7, -9 }, { -9, -1 }, { 9, -1 }, { -5, 5 }, { 5, 5 }, { 0, 7 } };
const int kLandProbes[8][2] = { { 0, -24 }, { -10, -18 }, { 10, -18 }, { -16, -1 }, { 16, -1 }, { -9, 9 }, { 9, 9 }, { 0, 12 } };

unsigned int ProbeAll(EggGrid* theGrid, float x, float y, const int (*theProbes)[2])
{
	unsigned int aHits = 0;
	for (int i = 0; i < 8; i++)
		aHits |= theGrid->Probe((int) x + theProbes[i][0], (int) (y + (float) theProbes[i][1]), true);
	return aHits;
}

int CellColor(EggGrid* theGrid, int theCol, int theRow)
{
	if ((theCol < 0) || (theCol > 14) || (theRow < 0) || (theRow > 99))
		return -1;
	return (int) (theGrid->mCells[theCol][theRow].mBits & 0xff);
}

// the six neighbours of a cell, the odd rows shifted right by half a cell
void Neighbours(int theCol, int theRow, int theOut[6][2])
{
	int aShift = ((theRow % 2) != 0) ? 0 : -1;
	int aList[6][2] = { { theCol - 1, theRow }, { theCol + 1, theRow }, { theCol + aShift, theRow - 1 },
		{ theCol + aShift + 1, theRow - 1 }, { theCol + aShift, theRow + 1 }, { theCol + aShift + 1, theRow + 1 } };
	for (int i = 0; i < 6; i++)
	{
		theOut[i][0] = aList[i][0];
		theOut[i][1] = aList[i][1];
	}
}

// the group of theColor an egg landing at the cell joins, itself counted (Match: colours above 0x11 never match)
int GroupSize(EggGrid* theGrid, int theCol, int theRow, int theColor)
{
	if (theColor > 0x11)
		return 1;
	static bool aSeen[15][100];
	for (int c = 0; c < 15; c++)
		for (int r = 0; r < 100; r++)
			aSeen[c][r] = false;
	int aStack[1500][2];
	int aTop = 0;
	aStack[aTop][0] = theCol;
	aStack[aTop][1] = theRow;
	aTop++;
	aSeen[theCol][theRow] = true;
	int aCount = 0;
	while (aTop > 0)
	{
		aTop--;
		int aCol = aStack[aTop][0];
		int aRow = aStack[aTop][1];
		aCount++;
		int aNext[6][2];
		Neighbours(aCol, aRow, aNext);
		for (int i = 0; i < 6; i++)
		{
			int c = aNext[i][0];
			int r = aNext[i][1];
			if ((CellColor(theGrid, c, r) == theColor) && !aSeen[c][r] && ((float) r <= theGrid->mRows))
			{
				aSeen[c][r] = true;
				aStack[aTop][0] = c;
				aStack[aTop][1] = r;
				aTop++;
			}
		}
	}
	return aCount;
}

// the eggs around a cell, for a bomb
int Around(EggGrid* theGrid, int theCol, int theRow)
{
	int aCount = 0;
	for (int c = theCol - 2; c <= theCol + 2; c++)
		for (int r = theRow - 2; r <= theRow + 2; r++)
			if (CellColor(theGrid, c, r) > 0)
				aCount++;
	return aCount;
}

// flies an egg from (x, y) at theAngle until it lands, the grid coming down theDrop pixels an update meanwhile; false
// when it breaks on a target or never lands
bool FlyOnce(Dynomite* theBoard, int theAngle, float x, float y, float theDrop, int* theCol, int* theRow)
{
	EggGrid* aGrid = theBoard->mGrid;
	AngleTable* anAngles = theBoard->mApp->mAngles;
	float aVX = anAngles->mDX[theAngle];
	float aVY = anAngles->mDY[theAngle];
	for (int anUpdate = 0; anUpdate < 400; anUpdate++)
	{
		if (anUpdate > 0)
			aGrid->mCeiling += theDrop;
		unsigned int aHits = 0;
		for (int i = 0; i < 10; i++)
		{
			x = aVX + x;
			y = y + aVY;
			aHits |= ProbeAll(aGrid, x, y, kProbes);
			if ((aHits & 1) != 0)
				aVX = fabsf(aVX);
			if ((aHits & 2) != 0)
				aVX = -fabsf(aVX);
			if ((aHits & 0x14) != 0)
				break;
		}
		if ((aHits & 0x14) != 0)
		{
			GridPoint aPoint;
			GridPoint aCell;
			aGrid->ScreenToGrid((int) x, (int) y, &aPoint);
			EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
			for (int aTries = 0; (CellColor(aGrid, aCell.mX, aCell.mY) != 0) && (aTries < 50); aTries++)
			{
				int i = 0;
				aHits = 0;
				do
				{
					x = x - aVX;
					y = y - aVY;
					aHits |= ProbeAll(aGrid, x, y, kLandProbes);
					if ((aHits & 1) != 0)
						aVX = fabsf(aVX);
					if ((aHits & 2) != 0)
						aVX = -fabsf(aVX);
				} while ((y <= 570.0f) && (++i < 0xf));
				aGrid->ScreenToGrid((int) x, (int) y, &aPoint);
				EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
			}
			if (CellColor(aGrid, aCell.mX, aCell.mY) != 0)
				return false;
			*theCol = aCell.mX;
			*theRow = aCell.mY;
			return true;
		}
		if ((aHits & 8) != 0)
			return false;
	}
	return false;
}

bool Fly(Dynomite* theBoard, int theAngle, float x, float y, float theDrop, int* theCol, int* theRow)
{
	// the ceiling is moved only while the flight is worked out, and put back
	float aCeiling = theBoard->mGrid->mCeiling;
	bool aLanded = FlyOnce(theBoard, theAngle, x, y, theDrop, theCol, theRow);
	theBoard->mGrid->mCeiling = aCeiling;
	return aLanded;
}

// how far the grid comes down an update, as seen between this call and the last within two seconds (a push of new rows
// or a kick is left out)
float GridDrop(Dynomite* theBoard)
{
	static Dynomite* aLastBoard = NULL;
	static float aLastCeiling = 0.0f;
	static int aLastUpdate = -1;
	static float aDrop = 0.0f;
	int anUpdate = theBoard->mApp->mUpdateCount;
	float aCeiling = theBoard->mGrid->mCeiling;
	if ((aLastBoard == theBoard) && (anUpdate > aLastUpdate) && (anUpdate - aLastUpdate <= 200))
	{
		float aSeen = (aCeiling - aLastCeiling) / (float) (anUpdate - aLastUpdate);
		aDrop = ((aSeen >= 0.0f) && (aSeen <= 3.0f)) ? aSeen : 0.0f;
	}
	else if (aLastBoard != theBoard)
		aDrop = 0.0f;
	aLastBoard = theBoard;
	aLastCeiling = aCeiling;
	aLastUpdate = anUpdate;
	return aDrop;
}

}

bool Sexy::BotChooseShot(Dynomite* theBoard, BotShot* theShot, bool toTheLine)
{
	if ((theBoard == NULL) || (theBoard->mGrid == NULL) || (theBoard->mLauncher == NULL))
		return false;
	Launcher* aLauncher = theBoard->mLauncher;
	if ((aLauncher->mLoaded == NULL) || aLauncher->mFrozen || theBoard->mField131 || theBoard->mField132 ||
		(theBoard->mPauseCount > 0) || theBoard->mField244 || theBoard->mField80 || (theBoard->mThunderbolt != NULL) ||
		theBoard->mField130)
		return false;
	AngleTable* anAngles = theBoard->mApp->mAngles;
	EggGrid* aGrid = theBoard->mGrid;
	int aColor = aLauncher->mLoaded->mColor;
	float aDrop = GridDrop(theBoard);
	// each angle the mouse can give (Aim: the point is held at least 10 pixels above the sling), with the first point
	// of the board found for it
	bool aHave[360] = {};
	int aPointX[360];
	int aPointY[360];
	static const int kRise[] = { 200, 120, 60, 30, 10 };
	for (int k = 0; k < 5; k++)
	{
		int y = aLauncher->mSlingY - kRise[k];
		if (y < 5)
			continue;
		for (int x = 2; x < theBoard->mWidth - 2; x++)
		{
			int anAngle = anAngles->AngleTo(aLauncher->mSlingX, aLauncher->mSlingY, x, y);
			if ((anAngle >= 0) && (anAngle < 360) && !aHave[anAngle])
			{
				aHave[anAngle] = true;
				aPointX[anAngle] = x;
				aPointY[anAngle] = y;
			}
		}
	}
	bool aFound = false;
	int aBestScore = -1;
	for (int i = 0; i < 360; i++)
	{
		// from straight up outwards, so that a tie goes to the plainest shot
		int anAngle = ((i % 2) == 0) ? (i / 2) : (359 - i / 2);
		if (!aHave[anAngle])
			continue;
		// the egg starts where PlaceEgg puts it for this angle
		float x = (float) ((double) aLauncher->mSlingX - (double) anAngles->mDX[anAngle] * 25.0);
		float y = (float) ((double) aLauncher->mFieldAac + ((double) aLauncher->mSlingY - (double) anAngles->mDY[anAngle] * 25.0));
		int aCol, aRow;
		if (!Fly(theBoard, anAngle, x, y, aDrop, &aCol, &aRow))
			continue;
		int aGroup = (aColor == 0) ? Around(aGrid, aCol, aRow) : GroupSize(aGrid, aCol, aRow, aColor);
		// a match (three or more) first, the bigger the better; else the most of its colour touched; then the higher
		// landing (it keeps the eggs from the line)
		int aScore = aGroup * 1000 + (99 - aRow);
		if (toTheLine)
			aScore = (aGroup >= 3) ? 0 : (aRow * 10 + 10 - aGroup);
		if (aScore > aBestScore)
		{
			aBestScore = aScore;
			aFound = true;
			theShot->mAngle = anAngle;
			theShot->mCol = aCol;
			theShot->mRow = aRow;
			theShot->mColor = aColor;
			theShot->mGroup = aGroup;
			theShot->mClickX = aPointX[anAngle] + theBoard->mX;
			theShot->mClickY = aPointY[anAngle] + theBoard->mY;
		}
	}
	return aFound;
}
