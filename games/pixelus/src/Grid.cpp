#include "Grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>

using namespace Sexy;

// 0x425d40: an empty grid of the given size
Grid::Grid(int theRows, int theCols)
{
	mRows = theRows;
	mCols = theCols;
	mDifficulty = 0;
	Clear();
}

// 0x4256b0
Grid::~Grid()
{
	for (size_t i = 0; i < mObjects.size(); i++)
		delete mObjects[i];
}

// 0x4262c0: empty every cell and forget the objects and the history
void Grid::Clear()
{
	for (size_t i = 0; i < mObjects.size(); i++)
		delete mObjects[i];
	mObjects.clear();
	for (int r = 0; r < MAX_SIZE; r++)
		for (int c = 0; c < MAX_SIZE; c++)
		{
			Cell& aCell = mCells[r][c];
			aCell.mWanted = false;
			aCell.mFixed = false;
			aCell.mBouncy = false;
			aCell.mStart = false;
			aCell.mObject = NULL;
		}
	mSupply = 0;
	mCursor = 0;
	mSide = SIDE_UP;
	mDone = false;
	mMoveCount = 0;
	mTime = 0;
	mCanPlace = false;
	mSlidesOut = false;
	mLandCol = mLandRow = 0;
	mStartCol = mStartRow = 0;
	mCanPull = false;
	mStuckCol = mStuckRow = 0;
	mEmptyWanted = mStrayTiles = mWantedCells = 0;
	mHasStart = false;
	mHasPicture = false;
	memset(mPicture, 0, sizeof(mPicture));
	mHistory.clear();
}

// 0x4220a0: the cell of the edge a tile thrown from the side at the position enters at
void Grid::EdgeCell(int theSide, int theCursor, int* theCol, int* theRow) const
{
	switch (theSide)
	{
	case SIDE_UP:		*theCol = theCursor; *theRow = mRows - 1; break;
	case SIDE_RIGHT:	*theRow = theCursor; *theCol = 0; break;
	case SIDE_DOWN:		*theCol = theCursor; *theRow = 0; break;
	case SIDE_LEFT:		*theRow = theCursor; *theCol = mCols - 1; break;
	}
}

// the last cell of the line a tile flies along
bool Grid::AtFarEdge(int theCol, int theRow, int theSide) const
{
	switch (theSide)
	{
	case SIDE_UP:		return theRow < 1;
	case SIDE_RIGHT:	return theCol >= mCols - 1;
	case SIDE_DOWN:		return theRow >= mRows - 1;
	default:			return theCol < 1;
	}
}

// 0x4030a0
void Grid::Step(int* theCol, int* theRow, int theSide, int theCount) const
{
	switch (theSide)
	{
	case SIDE_UP:		*theRow -= theCount; break;
	case SIDE_RIGHT:	*theCol += theCount; break;
	case SIDE_DOWN:		*theRow += theCount; break;
	case SIDE_LEFT:		*theCol -= theCount; break;
	}
}

bool Grid::InGrid(int theRow, int theCol) const
{
	return theRow >= 0 && theRow < mRows && theCol >= 0 && theCol < mCols;
}

// 0x424390
GridObject* Grid::NewObject(int theRow, int theCol)
{
	GridObject* anObject = new GridObject();
	anObject->mRow = theRow;
	anObject->mCol = theCol;
	anObject->mFixed = false;
	anObject->mX = theCol * CELL_SIZE;
	anObject->mY = theRow * CELL_SIZE;
	anObject->mDir = 0;
	anObject->mVariant = 0;		// the Board picks it (CrtRand)
	anObject->mDead = false;
	anObject->mBounce = false;
	anObject->mTrailLen = 0;
	anObject->mState = 0;
	anObject->mIdle = 100;
	anObject->mTrail = 0;
	anObject->mFlash = 0;
	mObjects.push_back(anObject);
	return anObject;
}

// 0x424590 (the original also clears the editor's throw length +0xb0a4)
void Grid::SetCursor(int theSide, int theCursor)
{
	mCursor = theCursor;
	mSide = theSide;
	ComputeLanding();
}

// 0x423ee0 (the editor's limited throw, +0xb0a1, is left out): follow the line from the edge to the first taken
// cell; the tile stays in front of it. mStuck is that cell, mLand the cell in front of it.
void Grid::ComputeLanding()
{
	int aCol, aRow;
	EdgeCell(mSide, mCursor, &aCol, &aRow);
	mStartCol = aCol;
	mStartRow = aRow;
	mCanPlace = true;
	mSlidesOut = false;

	while (!AtFarEdge(aCol, aRow, mSide) && !HasObject(aRow, aCol))
		Step(&aCol, &aRow, mSide, 1);

	mStuckCol = aCol;
	mStuckRow = aRow;

	if (AtFarEdge(aCol, aRow, mSide) && !HasObject(aRow, aCol))
	{
		mCanPlace = false;
		mSlidesOut = true;
	}
	else if (aCol == mStartCol && aRow == mStartRow)
		mCanPlace = false;
	else
		Step(&aCol, &aRow, mSide, -1);
	mLandCol = aCol;
	mLandRow = aRow;

	const Cell& aStuck = mCells[mStuckRow][mStuckCol];
	mCanPull = aStuck.mObject != NULL && !aStuck.mFixed && !aStuck.mStart;
}

// 0x422320
void Grid::Recount()
{
	mEmptyWanted = mStrayTiles = mWantedCells = 0;
	for (int r = 0; r < mRows; r++)
		for (int c = 0; c < mCols; c++)
		{
			const Cell& aCell = mCells[r][c];
			if (aCell.mWanted)
			{
				if (aCell.mObject == NULL)
					mEmptyWanted++;
				mWantedCells++;
			}
			else if (!aCell.mFixed && aCell.mObject != NULL)
				mStrayTiles++;
		}
}

// 0x422590: every wanted cell holds a tile
void Grid::Complete()
{
	mDone = true;
}

// 0x421fb0: the tile would hit a bouncy block
bool Grid::IsBouncyAhead() const
{
	if (AtFarEdge(mLandCol, mLandRow, mSide))
		return false;
	int aCol = mLandCol, aRow = mLandRow;
	Step(&aCol, &aRow, mSide, 1);
	const Cell& aCell = mCells[aRow][aCol];
	return aCell.mFixed && aCell.mBouncy;
}

// 0x4245c0: take a pull back, a tile on a free cell (the Board's undo)
GridObject* Grid::PutTile(int theRow, int theCol)
{
	if (mDone || mSupply < 1 || !InGrid(theRow, theCol) || mCells[theRow][theCol].mObject != NULL)
		return NULL;
	GridObject* anObject = NewObject(theRow, theCol);
	mCells[theRow][theCol].mObject = anObject;
	mSupply--;
	Recount();
	ComputeLanding();
	return anObject;
}

// 0x424750: throw a tile from the side at the cursor. It stays at mLand, is thrown back by a bouncy block or flies
// out of the grid; any of the three costs a tile of the workbench.
bool Grid::PlaceTile(bool theRecord)
{
	if (mDone || (!mCanPlace && !mSlidesOut) || mSupply < 1)
		return false;

	bool aSlidesOut = !mCanPlace && mSlidesOut;
	bool aBounces = IsBouncyAhead() && !aSlidesOut;

	int aCol = mLandCol, aRow = mLandRow;
	if (aSlidesOut)
		Step(&aCol, &aRow, mSide, 1);
	GridObject* anObject = NewObject(aRow, aCol);
	if (!aBounces && !aSlidesOut)
		mCells[mLandRow][mLandCol].mObject = anObject;

	mSupply--;
	if (theRecord)
	{
		mMoveCount++;
		Move aMove;
		aMove.mSide = mSide;
		aMove.mCursor = mCursor;
		aMove.mPosition = -1;
		aMove.mRemove = false;
		mHistory.push_back(aMove);
	}

	Recount();
	if (mEmptyWanted == 0)
		Complete();
	ComputeLanding();
	return true;
}

// 0x424990: pull the tile the throw would hit back to the Tile Master. theForce lets it take a start cell's tile
// too (the hint's undo).
bool Grid::PullTile(bool theRecord, bool theForce)
{
	Cell& aCell = mCells[mStuckRow][mStuckCol];
	bool aForced = !mCanPull && aCell.mStart && !aCell.mFixed && theForce;
	if (mDone || (!mCanPull && !aForced) || aCell.mObject == NULL)
		return false;

	aCell.mObject = NULL;
	if (aCell.mFixed)
	{
		aCell.mFixed = false;
		aCell.mWanted = true;
	}
	aCell.mBouncy = false;
	mSupply++;

	ComputeLanding();
	if (theRecord)
	{
		// the position written is that of the next taken cell behind the tile, found by the new landing
		Move aMove;
		aMove.mSide = mSide;
		aMove.mCursor = mCursor;
		aMove.mPosition = (mSide == SIDE_UP || mSide == SIDE_DOWN) ? mStuckRow : mStuckCol;
		aMove.mRemove = true;
		mHistory.push_back(aMove);
		mMoveCount++;
	}
	Recount();
	return true;
}

// 0x425b40: "AU10-1" is {side U, 10, -1} of kind A (a thrown tile); "RD1003" a pulled one. B and F are the editor's.
bool Grid::ParseMove(const std::string& theString, Move* theMove)
{
	if (theString.size() < 6)
		return false;
	switch (theString[0])
	{
	case 'A': theMove->mRemove = false; break;
	case 'R': theMove->mRemove = true; break;
	default: return false;
	}
	switch (theString[1])
	{
	case 'U': theMove->mSide = SIDE_UP; break;
	case 'R': theMove->mSide = SIDE_RIGHT; break;
	case 'D': theMove->mSide = SIDE_DOWN; break;
	case 'L': theMove->mSide = SIDE_LEFT; break;
	default: return false;
	}
	theMove->mCursor = atoi(theString.substr(2, 2).c_str());
	theMove->mPosition = atoi(theString.substr(4, 2).c_str());
	return true;
}

// 0x4257e0
std::string Grid::MoveToString(const Move& theMove)
{
	char aBuf[16];
	sprintf(aBuf, "%c%c%02d%02d", theMove.mRemove ? 'R' : 'A', "URDL"[theMove.mSide & 3], theMove.mCursor, theMove.mPosition);
	return aBuf;
}

// the hint plays the moves of a solution back one by one
bool Grid::ReplayMove(const Move& theMove)
{
	SetCursor(theMove.mSide, theMove.mCursor);
	return theMove.mRemove ? PullTile(true, true) : PlaceTile(true);
}

// 0x425220 (pulling: the cell holds a tile that can be taken) and 0x4253f0 (throwing: the cell is free and the
// tile would stay there): the sides the Tile Master may throw from, the one it already stands on first.
int Grid::FindThrowSide(int theRow, int theCol, int thePreferredSide, int* theCursor) const
{
	if (!InGrid(theRow, theCol) || mCells[theRow][theCol].mObject != NULL)
		return -1;
	int aSides[4];
	int aCount = 0;
	for (int aSide = 0; aSide < 4; aSide++)
	{
		int aCol = theCol, aRow = theRow;
		Step(&aCol, &aRow, aSide, 1);
		if (!InGrid(aRow, aCol))
			continue;
		const Cell& aAhead = mCells[aRow][aCol];
		if (aAhead.mObject == NULL || aAhead.mBouncy)
			continue;

		// the line behind it, out to the edge, must be free
		aCol = theCol;
		aRow = theRow;
		Step(&aCol, &aRow, aSide, -1);
		bool aClear = true;
		while (InGrid(aRow, aCol))
		{
			if (mCells[aRow][aCol].mObject != NULL)
			{
				aClear = false;
				break;
			}
			Step(&aCol, &aRow, aSide, -1);
		}
		if (aClear)
			aSides[aCount++] = aSide;
	}
	if (aCount == 0)
		return -1;
	int aSide = aSides[0];
	for (int i = 0; i < aCount; i++)
		if (aSides[i] == thePreferredSide)
			aSide = thePreferredSide;
	*theCursor = (aSide == SIDE_RIGHT || aSide == SIDE_LEFT) ? theRow : theCol;
	return aSide;
}

int Grid::FindPullSide(int theRow, int theCol, int thePreferredSide, int* theCursor) const
{
	if (!InGrid(theRow, theCol))
		return -1;
	const Cell& aTarget = mCells[theRow][theCol];
	if (aTarget.mObject == NULL || aTarget.mFixed || aTarget.mStart)
		return -1;
	int aSides[4];
	int aCount = 0;
	for (int aSide = 0; aSide < 4; aSide++)
	{
		int aCol = theCol, aRow = theRow;
		Step(&aCol, &aRow, aSide, -1);
		bool aClear = true;
		while (InGrid(aRow, aCol))
		{
			if (mCells[aRow][aCol].mObject != NULL)
			{
				aClear = false;
				break;
			}
			Step(&aCol, &aRow, aSide, -1);
		}
		if (aClear)
			aSides[aCount++] = aSide;
	}
	if (aCount == 0)
		return -1;
	int aSide = aSides[0];
	for (int i = 0; i < aCount; i++)
		if (aSides[i] == thePreferredSide)
			aSide = thePreferredSide;
	*theCursor = (aSide == SIDE_RIGHT || aSide == SIDE_LEFT) ? theRow : theCol;
	return aSide;
}

// 0x426910: the tags #uuid#, #name#, #title#, #difficulty# and #data# (a run of one character per cell, with the line
// breaks left out); the text between two tags belongs to the first
bool Grid::LoadFromFile(const std::string& thePath, std::string* theError)
{
	std::ifstream aFile(thePath.c_str(), std::ios::binary);
	if (!aFile)
	{
		if (theError)
			*theError = "couldn't open level file";
		return false;
	}
	std::stringstream aBuf;
	aBuf << aFile.rdbuf();
	return LoadFromText(aBuf.str(), theError);
}

bool Grid::LoadFromText(const std::string& theText, std::string* theError)
{
	Clear();
	mDifficulty = 0;
	mTitle.clear();
	mName.clear();
	mUuid.clear();

	std::string aTag, aUuid, aName, aTitle, aDifficulty, aData;
	bool aInTag = false;
	bool aInValue = false;
	for (size_t i = 0; i < theText.size(); i++)
	{
		char c = theText[i];
		if (c == '\r' || c == '\n')
			continue;
		if (c == '#')
		{
			if (aInValue)
			{
				aInValue = false;
				aInTag = true;
				aTag.clear();
			}
			else if (aInTag)
			{
				aInTag = false;
				aInValue = true;
			}
			else
			{
				aInTag = true;
				aTag.clear();
			}
			continue;
		}
		if (aInTag)
			aTag += c;
		else if (aInValue)
		{
			if (aTag == "uuid")
				aUuid += c;
			else if (aTag == "name")
				aName += c;
			else if (aTag == "title")
				aTitle += c;
			else if (aTag == "difficulty")
				aDifficulty += c;
			else if (aTag == "data")
				aData += c;
		}
		else
		{
			if (theError)
				*theError = "found characters before tag";
			return false;
		}
	}

	mUuid = aUuid;
	mName = aName;
	mTitle = aTitle;
	mDifficulty = atoi(aDifficulty.c_str());
	if ((int)aData.size() != mRows * mCols)
	{
		if (theError)
			*theError = "#data# wrong size or not found";
		return false;
	}

	for (int r = 0; r < mRows; r++)
		for (int c = 0; c < mCols; c++)
		{
			Cell& aCell = mCells[r][c];
			switch (aData[r * mCols + c])
			{
			case '+':
				aCell.mWanted = true;
				aCell.mObject = NewObject(r, c);
				aCell.mFixed = false;
				break;
			case '-':
				aCell.mWanted = false;
				aCell.mObject = NewObject(r, c);
				aCell.mFixed = false;
				mSupply--;
				break;
			case '0':
				aCell.mWanted = false;
				break;
			case '?':
				aCell.mWanted = true;
				mSupply++;
				aCell.mFixed = false;
				break;
			case 'B':
				aCell.mObject = NewObject(r, c);
				aCell.mObject->mFixed = true;
				aCell.mFixed = true;
				aCell.mBouncy = true;
				break;
			case 'S':
				aCell.mWanted = true;
				mSupply++;
				aCell.mFixed = false;
				aCell.mStart = true;
				mHasStart = true;
				break;
			case 'X':
				aCell.mObject = NewObject(r, c);
				aCell.mObject->mFixed = true;
				aCell.mFixed = true;
				break;
			default:
				if (theError)
					*theError = "illegal char in #data# tag";
				return false;
			}
		}
	if (mSupply < 0)
		mSupply = 0;

	mDone = false;
	ComputeLanding();
	Recount();
	return true;
}
