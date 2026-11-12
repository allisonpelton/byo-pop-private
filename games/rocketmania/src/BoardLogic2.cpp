// The rules of the Board, second half (RocketManiaGame 0x422460-0x4285b0, 0x424730 and Update 0x426f40): the first
// board, the moving pieces, the launch of a connection, the timers of the three modes. See BoardLogic.cpp.

#include "Board.h"
#include "RocketApp.h"
#include "TalkingHead.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "MTRand.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

// port: the original's float to int conversion (__ftol): truncation
static int Ftol(double theValue)
{
	return (int) theValue;
}

// ---------------------------------------------------------------------------------------------------------
// the lists of moving pieces

// 0x427f90
void Board::AddMover(Piece* thePiece)
{
	thePiece->mInMoverList = true;
	mObjects.push_back(thePiece);
}

// 0x427ff0 and 0x428040: takes a piece out of the list (it stays alive)
void Board::EraseMover(Piece* thePiece)
{
	thePiece->mInMoverList = false;
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
	{
		if (*anItr == thePiece)
		{
			mObjects.erase(anItr);
			return;
		}
	}
}

// 0x428100: a flash of a kind where a piece was
void Board::AddFlash(Piece* thePiece, int theKind)
{
	Piece* aFlash = new Piece(mUpdateCnt);
	aFlash->mX = thePiece->mX;
	aFlash->mY = thePiece->mY;
	aFlash->mType = 3;
	aFlash->mKind = theKind;
	aFlash->mFrame = 0;
	AddMover(aFlash);
}

// 0x427d00: takes a piece off the board and deletes it. theFlag is passed to DestroySpecial for its special piece
void Board::RemovePiece(Piece* thePiece, bool theFlag)
{
	if (mPressedPiece == thePiece)
		mPressedPiece = NULL;
	switch (thePiece->mType)
	{
	case 0:
	case 1:
	case 2:
	case 4:
		if (thePiece->mRow >= 0 && thePiece->mRow < mRows && thePiece->mColumn >= 0 && thePiece->mColumn < mCols &&
			mGrid[thePiece->mRow][thePiece->mColumn] == thePiece)
			mGrid[thePiece->mRow][thePiece->mColumn] = NULL;
		break;
	case 5:
		if (thePiece->mColumn == -1)
		{
			if (mLeft[thePiece->mRow] == thePiece)
				mLeft[thePiece->mRow] = NULL;
		}
		else if (mRight[thePiece->mRow] == thePiece)
			mRight[thePiece->mRow] = NULL;
		break;
	case 6:
		if (thePiece->mKind == 4)
			mFlameCount--;
		if (thePiece->mFlash != NULL)
			mEffects->Remove(thePiece->mFlash);
		if ((mState == 4 || mState == 5) &&
			(thePiece->mKind == 0 || thePiece->mKind == 1 || thePiece->mKind == 0xc || thePiece->mKind == 6 || thePiece->mKind == 2))
		{
			Effect* anEffect = mEffects->Add(0xb, true);
			anEffect->mLife = 1;
			anEffect->mDelay = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
			anEffect->mSpeed = 5.5;
			anEffect->mParticleLife = 0x32;
			anEffect->mCount = 0x46;
			anEffect->mDrag = 0.9;
		}
		if (thePiece->mKind == 0xd)
		{
			Effect* anEffect = mEffects->Add(5, true);
			anEffect->mAccelY = 0;
			anEffect->mDirY = 0;
			anEffect->mDrag = 0.93;
			anEffect->mLife = 1;
			anEffect->mDelay = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
		}
		break;
	}
	if (thePiece->mType != 3 && thePiece->mType != 6 && thePiece->mType != 5 && mState == 0)
		AddFlash(thePiece, 2);
	if (thePiece->mInMoverList)
		EraseMover(thePiece);
	if (thePiece->mSpecial != NULL)
	{
		Piece* aSpecial = thePiece->mSpecial;
		aSpecial->mHost = NULL;
		DestroySpecial(aSpecial, theFlag);
		thePiece->mSpecial = NULL;
	}
	if (thePiece->mHost != NULL)
	{
		thePiece->mHost->mSpecial = NULL;
		thePiece->mHost = NULL;
	}
	delete thePiece;
}

// ---------------------------------------------------------------------------------------------------------
// the grid

// 0x427ab0: the tile under a point
Piece* Board::PieceAtPoint(int theX, int theY)
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mX <= (double) theX && aPiece->mY <= (double) theY &&
				(double) theX < aPiece->mX + 42.0 && (double) theY < aPiece->mY + 42.0)
				return aPiece;
		}
	}
	return NULL;
}

// 0x427b70: the lowest empty row of a column, from row 8 up (-1 when the column is full)
int Board::LowestEmpty(int theColumn)
{
	int aRow = 8;
	do
	{
		if (mGrid[aRow][theColumn] == NULL)
			return aRow;
		aRow--;
	}
	while (-1 < aRow);
	return -1;
}

// 0x427ba0: a tile is put at another cell (the tile that was moved keeps shaking for a while)
void Board::MovePiece(Piece* thePiece, int theRow, int theColumn)
{
	thePiece->mMovedFromRow = thePiece->mRow;
	thePiece->mMovedFromColumn = thePiece->mColumn;
	thePiece->mTrailLife = 100;
	mGrid[thePiece->mRow][thePiece->mColumn] = NULL;
	mGrid[theRow][theColumn] = thePiece;
	thePiece->mRow = theRow;
	thePiece->mColumn = theColumn;
	thePiece->mX = (double) CellX(theColumn);
	thePiece->mY = (double) CellY(theRow);
	if (thePiece->mType == 4)
		CancelBurn(thePiece->mPath);
	MarkDirty();
	mMovedSinceDown = true;
}

// 0x427c50: removes the tiles near a cell, up to a squared distance
void Board::Shock(int theRow, int theColumn, int theRadius2)
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			int aDRow = aRow - theRow;
			int aDColumn = aColumn - theColumn;
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType == 1 && (aDRow != 0 || aDColumn != 0) &&
				aDRow * aDRow + aDColumn * aDColumn <= theRadius2)
				RemovePiece(aPiece, true);
		}
	}
}

// 0x422a90: a tile with empty cells below it falls to the lowest one it can reach
void Board::DropPiece(Piece* thePiece)
{
	int aTarget = -1;
	int aRow = thePiece->mRow + 1;
	bool aFreeFound = false;
	if (aRow < mRows)
	{
		do
		{
			Piece* aCell = mGrid[aRow][thePiece->mColumn];
			if (aCell == NULL)
				aFreeFound = true;
			else
			{
				int aType = aCell->mType;
				if (aType == 1 || aType == 2 || aType == 4)
				{
					aTarget = aRow - 1;
					break;
				}
			}
			aRow++;
		}
		while (aRow < mRows);
		if (aFreeFound && (aTarget != -1 || (aTarget = mRows - 1, aTarget != -1)) && aTarget != thePiece->mRow)
		{
			AddMover(thePiece);
			thePiece->mType = 2;
			mGrid[thePiece->mRow][thePiece->mColumn] = NULL;
			mGrid[aTarget][thePiece->mColumn] = thePiece;
			int aOldRow = thePiece->mRow;
			thePiece->mRow = aTarget;
			thePiece->mMoving = true;
			thePiece->mSlideStep = 0.2 / (double) (aTarget - aOldRow);
			thePiece->mSlideStartX = Ftol(thePiece->mX);
			thePiece->mSlideStartY = Ftol(thePiece->mY);
			thePiece->mSlideEndX = CellX(thePiece->mColumn);
			thePiece->mSlideEndY = CellY(aTarget);
			thePiece->mSlideProgress = 0;
		}
	}
}

// 0x422bc0: every tile with an empty cell under it falls
void Board::SettleAll(bool theFlag)
{
	mSettleFlag = theFlag;
	mSettledAt = mUpdateCnt;
	for (int aColumn = 0; aColumn < mCols; aColumn++)
	{
		bool aFound = false;
		for (int aRow = mRows - 1; aRow >= 0; aRow--)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece == NULL)
				aFound = true;
			else if (aPiece->mType == 1 && aFound)
				DropPiece(aPiece);
		}
	}
}

// 0x426eb0: every cell holds a tile (in arcade mode: every cell that holds anything holds a tile)
bool Board::AllTiles()
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (mApp->mMode == 0)
			{
				if (aPiece != NULL && aPiece->mType != 1)
					return false;
			}
			else if (aPiece == NULL || aPiece->mType != 1)
				return false;
		}
	}
	return true;
}

// 0x427830: the pieces of a lost game fall and bounce about the board
void Board::Bounce(Piece* thePiece)
{
	if (thePiece == NULL)
		return;
	double aLeft = (double) CellX(0);
	double aRight = (double) CellX(5);
	double aSpeed = thePiece->mSpeedY + 0.13;
	thePiece->mSpeedY = aSpeed;
	thePiece->mX = thePiece->mSpeedX + thePiece->mX;
	double aY = aSpeed + thePiece->mY;
	thePiece->mY = aY;
	if (432.0 <= aY)
	{
		thePiece->mY = 432.0 - (aY - 432.0);
		thePiece->mSpeedY = aSpeed * -0.5;
		thePiece->mSpeedX = thePiece->mSpeedX * 0.93;
	}
	if (thePiece->mX <= aLeft)
	{
		thePiece->mX = (aLeft - thePiece->mX) + aLeft;
		thePiece->mSpeedX = thePiece->mSpeedX * -0.8;
	}
	if (aRight <= thePiece->mX)
	{
		thePiece->mX = aRight - (thePiece->mX - aRight);
		thePiece->mSpeedX = thePiece->mSpeedX * -0.8;
	}
}

// 0x427910: strategy mode, a launch hits the bombs on tiles: each loses a hit and the last one explodes
void Board::HitBombs()
{
	for (int aColumn = 0; aColumn < mCols; aColumn++)
	{
		for (int aRow = mRows - 1; aRow >= 0; aRow--)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType != 4 && aPiece->mSpecial != NULL &&
				aPiece->mSpecial->mType == 6 && aPiece->mSpecial->mKind == 10)
			{
				Piece* aBomb = aPiece->mSpecial;
				aBomb->mHitsLeft--;
				if (aBomb->mHitsLeft == 0)
				{
					Effect* anEffect = mEffects->Add(0x15, true);
					anEffect->mLife = 1;
					anEffect->mDelay = 0;
					anEffect->mX = aBomb->mX;
					anEffect->mY = aBomb->mY;
					mApp->PlaySample(0x32);
					RemovePiece(aBomb, false);
				}
				else
				{
					Effect* anEffect = mEffects->Add(0x16, true);
					anEffect->mLife = 1;
					anEffect->mDelay = 0;
					anEffect->mX = aBomb->mX;
					anEffect->mY = aBomb->mY;
					mApp->PlaySample(0x32);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------
// the first board

// 0x422460: removes every tile and edge piece (theKeepPieces set: only forgets them; the original deletes the pieces
// when the flag is clear, and the moving list's pieces in both cases)
void Board::ClearBoard(bool theForget)
{
	// (the original clears the grid only if the mode is not arcade, or theForget is set, or a second argument is set; its two
	// callers make that always true: the destructor passes the second argument as 1, and 0x422620 calls it only with
	// theForget set)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aColumn = 0; aColumn < mCols; aColumn++)
			{
				if (!theForget)
				{
					if (mGrid[aRow][aColumn] != NULL)
						RemovePiece(mGrid[aRow][aColumn], false);
				}
				else
					mGrid[aRow][aColumn] = NULL;
			}
		}
	}
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		if (!theForget)
		{
			if (mLeft[aRow] != NULL)
				RemovePiece(mLeft[aRow], false);
			if (mRight[aRow] != NULL)
				RemovePiece(mRight[aRow], false);
		}
		else
		{
			mLeft[aRow] = NULL;
			mRight[aRow] = NULL;
		}
		mRightTouched[aRow] = false;   // board+0x2d1.. and +0x2c8..: the flags that say an edge is connected
		mLeftTouched[aRow] = false;
	}
	if (!theForget)
	{
		if (mSpare != NULL)
		{
			RemovePiece(mSpare, false);
			mSpare = NULL;
		}
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			RemovePiece(mTop[aColumn], false);
			mTop[aColumn] = NULL;
		}
	}
	else
	{
		mSpare = NULL;
		for (int aColumn = 0; aColumn < mCols; aColumn++)
			mTop[aColumn] = NULL;
	}
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); )
	{
		Piece* aPiece = *anItr;
		aPiece->mInMoverList = false;
		anItr = mObjects.erase(anItr);
		delete aPiece;
	}
}

// 0x422620: the first board of a game (not yet translated into a loaded game's pieces)
void Board::SetupBoard(bool theKeep, bool theLoaded)
{
	mRefilling = false;
	mJustRefilled = false;
	if (!theKeep)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			if (mRight[aRow] == NULL)
				AddRight(aRow);
			else
			{
				unsigned int aKind;
				int aB;
				mGen->Seed(&aKind, &aB, this, aRow);
				mRight[aRow]->mIndex = (int) aKind;
				mRight[aRow]->mIndex2 = 0;
			}
		}
		return;
	}
	ClearBoard(theKeep);
	for (int i = 0; i < 30; i++)
	{
		mPathEdges[i] = 0;
		mPathLeftLive[i] = false;
		mPathRightLive[i] = false;
	}
	if (theLoaded)
		return;
	for (int i = 0; i < mLeftCount; i++)
	{
		Piece* aPiece = new Piece(mUpdateCnt);
		aPiece->mColumn = -1;
		unsigned int aRow = (unsigned int) mRand->Next() % (unsigned int) mRows;
		while (mLeft[aRow] != NULL)
			aRow = (aRow + 1) % mRows;
		aPiece->mRow = (int) aRow;
		aPiece->mX = (double) CellX(aPiece->mColumn);
		aPiece->mY = (double) CellY(aPiece->mRow);
		aPiece->mKind = 0;
		aPiece->mType = 5;
		mLeft[aRow] = aPiece;
	}
	for (int i = 0; i < mRightCount; i++)
	{
		unsigned int aRow = (unsigned int) mRand->Next() % (unsigned int) mRows;
		while (mRight[aRow] != NULL)
			aRow = (aRow + 1) % mRows;
		AddRight(aRow);
	}
	int aMode = mApp->mMode;
	if (aMode == 0)
	{
		mDropColumn = 0;
		mSpare = NewTop(0);
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = new Piece(mUpdateCnt);
			aPiece->mColumn = aColumn;
			aPiece->mRow = 0;
			aPiece->mX = (double) CellX(aColumn);
			aPiece->mY = (double) (CellY(0) - 0x4a);
			aPiece->mKind = mGen->DrawKind();
			aPiece->mType = 1;
			aPiece->mRotation = (int) (mRand->Next() & 3);
			mTop[aColumn] = aPiece;
		}
		// with the hints on (the app's flag at 0x549, the Options hints checkbox, is clear) the first top line has fixed tiles
		if (!mApp->mNoHints)
		{
			static const int sKinds[6] = { 0, 0, 2, 0, 2, 0 };
			static const int sRotations[6] = { 1, 1, 1, 1, 2, 1 };
			for (int aColumn = 0; aColumn < mCols; aColumn++)
			{
				if (mTop[aColumn] != NULL)
				{
					mTop[aColumn]->mKind = sKinds[aColumn];
					mTop[aColumn]->mRotation = sRotations[aColumn];
				}
			}
		}
	}
	else if (aMode == 1 || aMode == 2)
	{
		mTimedDropColumn = 0;
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = new Piece(mUpdateCnt);
			aPiece->mX = (double) CellX(aColumn);
			aPiece->mY = (double) (CellY(0) - 0x4a);
			aPiece->mKind = mGen->DrawKind();
			aPiece->mType = 1;
			aPiece->mRotation = (int) (mRand->Next() & 3);
			mTop[aColumn] = aPiece;
		}
		DealFirstBoard();
	}
}

// ---------------------------------------------------------------------------------------------------------
// connections

// 0x425410: a connection from the left edge to the right edge is found: its tiles start to burn
void Board::StartBurn(int thePath)
{
	mBurning = true;
	mBurningPath = thePath;
	mBurnCountdown = 0x42;
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType == 1 && aPiece->mPath == thePath)
				aPiece->mType = 4;
		}
	}
}

// 0x426030: a connected left edge and a connected right edge exist
bool Board::BothEnds()
{
	for (int i = 0; i < mRows; i++)
	{
		if (mLeftTouched[i])
		{
			for (int j = 0; j < mRows; j++)
			{
				if (mRightTouched[j])
					return true;
			}
			return false;
		}
	}
	return false;
}

// 0x426070: for each row, floods from the tile at the edge column that can connect to the edge (theMask) and
// notes how many tiles it reached (mLeftReach for the left edge, mRightReach for the right); both edges together start a burn
void Board::Trace(int theColumn, unsigned int theMask, int theEnd, int theOtherEnd, int* thePath, bool theStart)
{
	unsigned int aBoth = (unsigned int) theEnd | (unsigned int) theOtherEnd;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		Piece* aPiece = mGrid[aRow][theColumn];
		for (int j = 0; j < mRows; j++)
		{
			mLeftTouched[j] = false;
			mRightTouched[j] = false;
		}
		if (aPiece != NULL && aPiece->mType == 1 && aPiece->mPath == 0 &&
			(gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & theMask) != 0)
		{
			int aCount = 0;
			unsigned int aEnds = 0;
			Flood(theColumn, aRow, *thePath, &aCount, &aEnds, aRow);
			if ((aEnds & aBoth) == aBoth)
			{
				if (theStart && BothEnds() && !mBurning)
				{
					StartBurn(*thePath);
					mApp->PlaySample(2);
					if (mJustRefilled || mRefilling)
						mChainBurn = true;
				}
			}
			else if (theColumn == 0)
				mLeftReach[aRow] = aCount;
			else
				mRightReach[aRow] = aCount;
		}
		(*thePath)++;
	}
}

// 0x4261e0: the same from the bottom row up, stopping at the first connection (returns whether one was found)
bool Board::TraceUp(int theColumn, unsigned int theMask, int theEnd, int theOtherEnd, int* thePath, bool theStart)
{
	unsigned int aBoth = (unsigned int) theEnd | (unsigned int) theOtherEnd;
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		bool aFound = false;
		Piece* aPiece = mGrid[aRow][theColumn];
		for (int j = 0; j < mRows; j++)
		{
			mLeftTouched[j] = false;
			mRightTouched[j] = false;
		}
		if (aPiece != NULL && aPiece->mType == 1 && aPiece->mPath == 0 &&
			(gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & theMask) != 0)
		{
			int aCount = 0;
			unsigned int aEnds = 0;
			Flood(theColumn, aRow, *thePath, &aCount, &aEnds, aRow);
			if ((aEnds & aBoth) == aBoth)
			{
				if (theStart && BothEnds() && !mBurning)
				{
					StartBurn(*thePath);
					mApp->PlaySample(2);
					if (mJustRefilled || mRefilling)
						mChainBurn = true;
				}
				aFound = true;
			}
			else if (theColumn == 0)
				mLeftReach[aRow] = aCount;
			else
				mRightReach[aRow] = aCount;
		}
		(*thePath)++;
		if (aFound)
			return true;
	}
	return false;
}

// 0x426370: every update the paths are cleared and found again; the shake of the tiles that were turned dies down
void Board::Connections()
{
	for (int aColumn = 0; aColumn < mCols; aColumn++)
	{
		if (mGrid[0][aColumn] != NULL && mGrid[0][aColumn]->mType == 1)
		{
			MarkDirty();
			break;
		}
	}
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece == NULL)
				continue;
			if (aPiece->mType == 1)
			{
				aPiece->mPath = 0;
				aPiece->mPathFrom = -1;
			}
			if (0 < aPiece->mTrailLife)
				aPiece->mTrailLife = aPiece->mTrailLife - 2;
			if (aPiece->mTilt <= 0.0)
			{
				if (aPiece->mTilt < 0.0)
				{
					aPiece->mTilt = ((double) (mUpdateCnt - aPiece->mTurnedAt) + 1.0) * 0.025 + aPiece->mTilt;
					if (0.0 < aPiece->mTilt || aPiece->mTilt < -1.57079632675)
						aPiece->mTilt = 0;
				}
			}
			else
			{
				aPiece->mTilt = aPiece->mTilt - ((double) (mUpdateCnt - aPiece->mTurnedAt) + 1.0) * 0.025;
				if (aPiece->mTilt < 0.0 || 1.57079632675 < aPiece->mTilt)
					aPiece->mTilt = 0;
			}
		}
	}
	for (int i = 0; i < mRows; i++)
	{
		mLeftReach[i] = 0;
		mRightReach[i] = 0;
	}
	for (int i = 0; i < 30; i++)
	{
		mPathEdges[i] = 0;
		mPathLeftLive[i] = false;
		mPathRightLive[i] = false;
	}
	int aPath = 1;
	Trace(0, 8, 1, 2, &aPath, true);
	Trace(mCols - 1, 2, 2, 1, &aPath, true);
}

// 0x425ca0: a flame (type 6, kind 4) starts from one cell towards a neighbour; blocked, it only goes part of the way
void Board::Ignite(int theRow, int theColumn, int theToRow, int theToColumn, bool theBlocked)
{
	Piece* aFlame = MakeSpecial(4, NULL);
	aFlame->mX = (double) CellX(theColumn);
	aFlame->mY = (double) CellY(theRow);
	aFlame->mFromRow = theRow;
	aFlame->mFromColumn = theColumn;
	aFlame->mRow = theToRow;
	aFlame->mColumn = theToColumn;
	aFlame->mSlideStartX = Ftol(aFlame->mX);
	aFlame->mSlideStartY = Ftol(aFlame->mY);
	aFlame->mSlideProgress = 0;
	aFlame->mMoving = true;
	if (!theBlocked)
	{
		aFlame->mSlideEndX = CellX(theToColumn);
		aFlame->mSlideEndY = CellY(theToRow);
		aFlame->mSlideStep = 0.166666;      // (the original stores the decimal literals 0.166666 and 0.333332)
		Piece* aTarget = NULL;
		if (theToRow < mRows && -1 < theToRow && theToColumn < mCols && -1 < theToColumn)
			aTarget = mGrid[theToRow][theToColumn];
		if (aTarget != NULL)
		{
			aTarget->mUsed = true;
			AddLaunchFlame();
			if (aTarget->mSpecial != NULL)
			{
				mGen->CountPiece(aTarget->mSpecial);
				aTarget->mSpecial->mHost = NULL;
				DestroySpecial(aTarget->mSpecial, false);
				aTarget->mSpecial = NULL;
			}
		}
	}
	else
	{
		double aX = (double) CellX(theToColumn);
		aFlame->mSlideEndX = Ftol((aX - aFlame->mX) * 0.5 + aFlame->mX);
		double aY = (double) CellY(theToRow);
		aFlame->mSlideEndY = Ftol((aY - aFlame->mY) * 0.5 + aFlame->mY);
		aFlame->mSlideStep = 0.333332;
		aFlame->mBlocked = true;
	}
	mFlameCount++;
}

// 0x425e30: the burning connection reached the right edge at some rows: score, rocket and new right piece for each
int Board::ResolveLaunch(int thePath)
{
	int aCount = 0;
	for (int i = 0; i < 9; i++)
		mLaunchedRockets[i] = -1;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		Piece* aPiece = mGrid[aRow][mCols - 1];
		if (aPiece != NULL && aPiece->mType == 4 && aPiece->mPath == thePath && mRight[aRow] != NULL &&
			(gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & 2) != 0)
		{
			Piece* aRocket = mRight[aRow];
			int aFactor = aRocket->mIndex + 1;
			if (mApp->mMode == 2)
				aFactor = 1;
			int aScore = AddScore(aFactor * 100);
			AddScoreText(aScore, Ftol(aRocket->mX), Ftol(aRocket->mY));
			mLaunchedRockets[aCount] = aRocket->mIndex;
			MakeRocket(aRow);
			if (mApp->mMode != 2)
				MakeFlame0();
			aCount++;
			mGen->CountPiece(aRocket);
			RemovePiece(aRocket, false);
			mLaunchesDone = mLaunchesDone + 1;
			mLaunchesCounted = mLaunchesCounted + 1;
			AddRight(aRow);
		}
	}
	double aFraction = (double) mLaunchesDone / (double) mLaunchesNeeded;
	if (1.0 < aFraction)
		aFraction = 1.0;
	mLevelFraction = aFraction;
	if (0 < aCount)
	{
		int aSound = aCount + 0x1d;
		if (0x22 < aSound)
			aSound = 0x22;
		mApp->PlaySample(aSound);
		int aTimes = aCount;
		if (aCount < 4)
			aTimes = 3;
		while (0 < aTimes)
		{
			mApp->PlaySample(7);
			aTimes--;
		}
	}
	return aCount;
}

// 0x425490: the burn is over: the connection's tiles go, the launches score and the effects fly
void Board::Launch()
{
	if (!mChainBurn)
	{
		if (mApp->mMode == 2)
			HitBombs();
	}
	else
	{
		mChainBurn = false;
		AddScore(0 < mChainBonusTime ? 4000 : 2000);
		mChainBonusTime = 0xfa;
		mApp->PlaySample(0x1d);
		if (mApp->mMode == 2)
			AttachSpecial10();
	}
	mMoved = true;
	mRefilling = true;
	int aLaunches = ResolveLaunch(mBurningPath);
	switch (aLaunches)
	{
	case 3: mShakeAmount = 1; mShakeTime = 0x14; break;
	case 4: mShakeAmount = 1; mShakeTime = 0x1e; break;
	case 5: mShakeAmount = 2; mShakeTime = 0x2d; break;
	case 6: mShakeAmount = 3; mShakeTime = 0x3c; break;
	case 7: mShakeAmount = 4; mShakeTime = 0x4b; break;
	case 8: mShakeAmount = 4; mShakeTime = 0x5a; break;
	case 9: mShakeAmount = 5; mShakeTime = 100; break;
	}
	int aThisPath = mBurningPath;
	int aMinX = 99999, aMaxX = -1, aMinY = 99999, aMaxY = -1;
	int aRemoved = 0;
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType == 4 && aPiece->mPath == aThisPath)
			{
				if (aPiece->mX < (double) aMinX)
					aMinX = Ftol(aPiece->mX);
				if ((double) aMaxX < aPiece->mX)
					aMaxX = Ftol(aPiece->mX);
				if (aPiece->mY < (double) aMinY)
					aMinY = Ftol(aPiece->mY);
				if ((double) aMaxY < aPiece->mY)
					aMaxY = Ftol(aPiece->mY);
				mGen->CountPiece(aPiece);
				RemovePiece(aPiece, false);
				aRemoved++;
			}
		}
	}
	mTilesRemoved = mTilesRemoved + aRemoved;
	AddScore(aRemoved * 5);
	if (mLevelMostTiles < aRemoved)
		mLevelMostTiles = aRemoved;
	mGen->ShowBonus();
	if (0 < aLaunches)
	{
		static const int kEffectKinds[3][4] = { {0x10, 0x1b, 0xc, 0xf}, {0x10, 0xf, 0xc, 0xd}, {0xe, 0xf, 0xc, 0xe} };
		int aMiddle = aLaunches / 2;
		int aX = 0x96;
		for (int i = 0; i < aLaunches; i++)
		{
			int aKind;
			if (i == aMiddle)
			{
				int aRow = aMiddle;
				if (1 < aMiddle)
					aRow = 2;
				aKind = kEffectKinds[aRow][mRand->Next() & 3];
			}
			else if (i == aLaunches - 1 && 1 < aLaunches)
			{
				aKind = aMiddle + 0x1a;
				if (0x1c < aKind)
					aKind = 0x1d;
			}
			else
				aKind = -1;
			if (aLaunches == 1)
				aKind = -1;
			int aEffect = mLaunchedRockets[i];
			if (mApp->mMode == 2)
				aEffect = (int) (mRand->Next() % 9);
			LevelEffect(1, aEffect, aX, aKind);
			aX += 0x32;
		}
	}
	if (1 < aLaunches)
	{
		int aSound = aLaunches / 2 + 0x1a;
		if (0x1c < aSound)
			aSound = 0x1d;
		mApp->PlaySample(aSound);
	}
	SettleAll(false);
	mBurningPath = -1;
	mBurning = false;
	int aMode = mApp->mMode;
	if (aMode != 2 && 1.0 <= mLevelFraction && mState != 4 && mState != 5)
	{
		bool aEnd = true;
		if (mState == 1)
		{
			if (aMode != 1)
				aEnd = false;
			else
				mJustInTime = true;
		}
		if (aEnd)
			EndLevelA();
	}
	if (mApp->mMode == 0)
	{
		mQuickDrops = 5;
		mFallTimer = mFallPeriod - 100;
	}
	mComboCount = 0;
	mLaunchStep = 0;
	FinishLaunchFlames();
}

// 0x425b50: the burn ends at the left edge (no tile at the right): fuses on the first column light
void Board::FinishBurn()
{
	mComboCount = 0;
	mLaunchStep = 0;
	bool aLit[9];
	int aLitCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		Piece* aPiece = mGrid[aRow][0];
		if (aPiece == NULL || aPiece->mType != 4 || mLeft[aRow] == NULL ||
			(gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & 8) == 0)
			aLit[aRow] = false;
		else
		{
			Ignite(aRow, -1, aRow, 0, false);
			aLit[aRow] = true;
			aLitCount++;
		}
	}
	mApp->PlaySample(0x18);
	// the launch's points show beside the fuse where half of the lit fuses are reached (the decompiler named the count of the
	// lit fuses by a lost local)
	int aCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		if (aLit[aRow] && (aCount = aCount + 1, aLitCount / 2 <= aCount))
		{
			StartLaunchFlames(aRow);
			break;
		}
	}
	if (mApp->mMode == 2 && 0 < aLitCount && !mJustRefilled && !mRefilling && !mChainBurn && 0 < mMatchesLeft)
	{
		MakeFlame1();
		mMatchesLeft = mMatchesLeft - 1;
		if (mMatchesLeft == 2)
			mMatchesWarning = 600;
	}
}

// 0x426580
void Board::SmoothFraction()
{
	if (mLevelFraction < mLevelFractionShown)
	{
		mLevelFractionShown = mLevelFractionShown - 0.01;
		if (mLevelFractionShown < mLevelFraction)
			mLevelFractionShown = mLevelFraction;
	}
	if (mLevelFractionShown < mLevelFraction)
	{
		mLevelFractionShown = mLevelFractionShown + 0.0035;
		if (mLevelFraction < mLevelFractionShown)
			mLevelFractionShown = mLevelFraction;
	}
}

// 0x426620: the night runs out (classic mode only: the original tests app+0x530 == 1 first) and its warnings
void Board::NightTimer()
{
	if (mApp->mMode != 1)
		return;
	if (mState != 0)
		return;
	if (mFrozenTime < 1)
	{
		if (mMoved)
			mNightLeft = mNightLeft - mNightStep;
		if (mNightLeft <= 10.0 && !mNightWarned)
		{
			mNightWarned = true;
			mApp->PlaySample(6);
		}
		if (6 < mLevel && !mClockMade && !mClockDue && (mNightLeft <= mClockNightLeft || mNightLeft <= 10.0))
			mClockDue = true;
		if (mNightLeft <= 0.0 && 0.0 < mNightStep)
		{
			mNightLeft = 0;
			GameOver(0);
		}
		if (mNightLeft < mNightLeftShown)
		{
			mNightLeftShown = mNightLeftShown - 0.01;
			if (mNightLeftShown < mNightLeft)
				mNightLeftShown = mNightLeft;
		}
		if (mNightLeftShown < mNightLeft)
		{
			mNightLeftShown = mNightLeftShown + 0.0035;
			if (mNightLeft < mNightLeftShown)
				mNightLeftShown = mNightLeft;
		}
		double aLeft = 1.0 - mNightLeft / mNightLength;
		if (0.9 <= aLeft && aLeft < 1.0 && mUpdateCnt % 100 == 0)
			mApp->PlaySample(5);
	}
	else
	{
		mFrozenTime = mFrozenTime - 1;
		if (mFrozenTime == 0)
		{
			Effect* anEffect = mEffects->Add(0x17, true);
			anEffect->mLife = 5;
			anEffect->mDelay = 0;
			anEffect->mX = 365.0;
			anEffect->mY = 50.0;
			mApp->PlaySample(0x35);
		}
	}
}

// 0x426860: the timers of the three modes: the next tile falls, the hints appear
void Board::Timers()
{
	if (0 < mShakeTime)
		mShakeTime--;
	if (mState != 0)
	{
		if (mState == 1 && 0 < mGameOverDelay && --mGameOverDelay == 0)
		{
			mState = 2;
			ShowLevelUp(LevelUpDialog::KIND_GAME);
			mApp->mHead->Arrive();
			mApp->mHead->SetSlide(false);
			mWidgetManager->BringToFront(mApp->mHead);
		}
		return;
	}
	if (0 < mChainBonusTime)
		mChainBonusTime--;
	if (0 < mMatchesWarning)
		mMatchesWarning--;
	if (!mBurning)
	{
		int aMode = mApp->mMode;
		if (aMode == 0)
		{
			if (mOverflowTimer < 500)
			{
				if (mGrid[0][mDropColumn] == NULL)
					mOverflowTimer = 500;
				else if (--mOverflowTimer == 0)
					GameOver(1);
			}
			if (mOverflowTimer == 500)
			{
				if (mMoved || mTilesDropped < mGen->Rows() * mCols)
					mFallTimer = mFallTimer + 1;
				if (mFallPeriod <= mFallTimer)
				{
					mFallTimer = 0;
					mFallDisplay = 0;
					DropTop();
				}
			}
		}
		else if ((aMode == 1 || aMode == 2) && (mFallTimer = mFallTimer + 1, mTimedFallPeriod <= mFallTimer))
		{
			mFallTimer = 0;
			mFallDisplay = 0;
			DropTopTimed();
		}
		if (mFallDisplay < mFallTimer && (mFallDisplay = mFallDisplay + 2, mFallTimer < mFallDisplay))
			mFallDisplay = mFallTimer;
	}
	for (int i = 0; i < 6; i++)
	{
		if (mHintDone[i] || mHintCountdown[i] <= 0 || --mHintCountdown[i] != 0 || mHintsQuiet)
			continue;
		switch (i)
		{
		case 0:
			if (mApp->mMode == 1)       // 0x42f6f0
			{
				mHintDone[i] = true;
			}
			break;
		case 1:
			if (0 < mLevel)
			{
				mHintDone[i] = true;
			}
			break;
		case 2:
			if (mApp->mMode == 0)
			{
				mHintDone[i] = true;
			}
			break;
		case 3:
			if (2 < mLevel)
			{
				mHintDone[i] = true;
			}
			break;
		case 4:
			if (3 < mLevel)
			{
				mHintDone[i] = true;
			}
			break;
		case 5:
			if (mApp->mMode == 0)
			{
				Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO14_BOLD], GT(0x004e66d0),
					CellX(3) + mX, mY + 0x1e, 600, 0);
				aPoints->mStatic = true;
				mTexts.push_back(aPoints);
				mWidgetManager->AddWidget(aPoints);
				mHintDone[i] = true;
				mHintsQuiet = true;
			}
			break;
		}
	}
	if (0 < mBannerTime)
		mBannerTime--;
	NightTimer();
}

// 0x424730: the pieces that move about: slides, falls, flames, the specials that fly to the bar
void Board::UpdatePieces()
{
	if (mObjects.empty())
		return;
	std::list<Piece*>::iterator anItr = mObjects.begin();
	do
	{
		bool aDestroy = false;
		bool aLeaveList = false;
		bool aSkipToEnd = false;
		Piece* aPiece = *anItr;
		bool aMoving = aPiece->mMoving;
		if (aMoving)
		{
			int aX = aPiece->mSlideEndX;
			if (aPiece->mSlideStartX != aPiece->mSlideEndX)
				aX = Ftol((1.0 - aPiece->mSlideProgress) * (double) aPiece->mSlideStartX + (double) aPiece->mSlideEndX * aPiece->mSlideProgress);
			aPiece->mX = (double) aX;
			int aY = aPiece->mSlideEndY;
			if (aPiece->mSlideStartY != aPiece->mSlideEndY)
				aY = Ftol((1.0 - aPiece->mSlideProgress) * (double) aPiece->mSlideStartY + (double) aPiece->mSlideEndY * aPiece->mSlideProgress);
			aPiece->mY = (double) aY;
		}
		bool aRemoveFromList = false;
		switch (aPiece->mType)
		{
		case 0:
			if (aMoving)
			{
				aPiece->mSlideProgress = aPiece->mSlideProgress + aPiece->mSlideStep;
				if (1.0 <= aPiece->mSlideProgress)
				{
					if (8 < aPiece->mRow + 1 || mGrid[aPiece->mRow + 1][aPiece->mColumn] != NULL)
					{
						aPiece->mType = 1;
						aPiece->mMoving = false;
						aPiece->mX = (double) aPiece->mSlideEndX;
						aPiece->mY = (double) aPiece->mSlideEndY;
						Ping();
						aRemoveFromList = true;
						break;
					}
					mGrid[aPiece->mRow][aPiece->mColumn] = NULL;
					aPiece->mRow = aPiece->mRow + 1;
					mGrid[aPiece->mRow][aPiece->mColumn] = aPiece;
					aPiece->mSlideStartX = aPiece->mSlideEndX;
					aPiece->mSlideStartY = aPiece->mSlideEndY;
					aPiece->mSlideProgress = aPiece->mSlideProgress - 1.0;
					aPiece->mSlideEndX = CellX(aPiece->mColumn);
					aPiece->mSlideEndY = CellY(aPiece->mRow);
				}
			}
			break;
		case 2:
			if (aMoving)
			{
				aPiece->mSlideProgress = aPiece->mSlideProgress + aPiece->mSlideStep;
				if (1.0 <= aPiece->mSlideProgress)
				{
					aPiece->mType = 1;
					aPiece->mMoving = false;
					aPiece->mX = (double) aPiece->mSlideEndX;
					aPiece->mY = (double) aPiece->mSlideEndY;
					if (mApp->mMode != 1 && mApp->mMode != 2)
						Ping();
					aRemoveFromList = true;
				}
			}
			break;
		case 3:
			if (aPiece->mKind != 2)
			{
				if (mUpdateCnt % 7 == 0 && (aPiece->mFrame = aPiece->mFrame + 1, 13 < aPiece->mFrame))
				{
					aDestroy = true;
					aRemoveFromList = true;
				}
			}
			else if (mUpdateCnt % 0xb == 0 && (aPiece->mFrame = aPiece->mFrame + 1, 3 <= aPiece->mFrame))
			{
				aDestroy = true;
				aRemoveFromList = true;
			}
			break;
		case 6:
			{
				// a special piece
				bool aArrived = false;
				if (!aMoving)
				{
					Piece* aHost = aPiece->mHost;
					if (aHost != NULL)
					{
						aPiece->mX = aHost->mX;
						aPiece->mY = aHost->mY;
						aPiece->mRow = aHost->mRow;
						aPiece->mColumn = aHost->mColumn;
						if (aHost->mY < (double) CellY(0))
							aPiece->mBorn = aPiece->mBorn + 1;
					}
				}
				else
				{
					double aOld = aPiece->mSlideProgress;
					aPiece->mSlideProgress = aPiece->mSlideStep + aOld;
					int aKind = aPiece->mKind;
					if (aPiece->mSlideStep + aOld < 1.0)
					{
						if (aKind == 5 && aPiece->mFlash != NULL)
						{
							aPiece->mFlash->mX = (aPiece->mX - 18.0) + 8.0;
							aPiece->mFlash->mY = (aPiece->mY + 53.0) - 8.0;
						}
					}
					else if (aKind == 4)
					{
						if (mUpdateCnt % 7 == 0)
							aPiece->mFrame = (aPiece->mFrame + 1) % 10;
						if (!aPiece->mBlocked)
						{
							Piece* aHere = mGrid[aPiece->mRow][aPiece->mColumn];
							for (int i = 0; i < 4; i++)
							{
								if (aHere != NULL && (gPipeSteps[i].mOut & gPipeMask[aHere->mRotation + aHere->mKind * 4]) != 0)
								{
									int aRow = gPipeSteps[i].mRow + aPiece->mRow;
									int aColumn = gPipeSteps[i].mCol + aPiece->mColumn;
									if (aRow != aPiece->mFromRow || aColumn != aPiece->mFromColumn)
									{
										Piece* aNext = NULL;
										if (!(aColumn < 0 || mCols <= aColumn || aRow < 0 || mRows <= aRow))
											aNext = mGrid[aRow][aColumn];
										if (aNext == NULL || aNext->mUsed ||
											(gPipeSteps[i].mIn & gPipeMask[aNext->mRotation + aNext->mKind * 4]) == 0)
											Ignite(aPiece->mRow, aPiece->mColumn, aRow, aColumn, true);
										else
										{
											Ignite(aPiece->mRow, aPiece->mColumn, aRow, aColumn, false);
											if (aNext->mType != 4)
											{
												aNext->mType = 4;
												aNext->mPath = aHere->mPath;
												aNext->mPathFrom = aHere->mPathFrom;
											}
										}
									}
								}
							}
						}
						aPiece->mMoving = false;
						aLeaveList = true;
						aDestroy = true;
						aPiece->mX = (double) aPiece->mSlideEndX;
						aPiece->mY = (double) aPiece->mSlideEndY;
					}
					else
					{
						aPiece->mMoving = false;
						aPiece->mX = (double) aPiece->mSlideEndX;
						aPiece->mY = (double) aPiece->mSlideEndY;
						aArrived = true;
						if (aKind == 7)
						{
							int anIndex = aPiece->mIndex;
							if (anIndex == 0)
							{
								mApp->PlaySample(9);
								mSpecialCounts[aKind] = mSpecialCounts[aKind] + 1;
							}
							else if (anIndex == 1)
							{
								mApp->PlaySample(10);
								mSpecialCounts[aKind] = mSpecialCounts[aKind] + 2;
							}
							else if (anIndex == 2)
							{
								mApp->PlaySample(0xb);
								mSpecialCounts[aKind] = mSpecialCounts[aKind] + 5;
							}
							else
							{
								mApp->PlaySample(10);
								mApp->PlaySample(0xb);
								mSpecialCounts[aKind] = mSpecialCounts[aKind] + 10;
							}
							aLeaveList = true;
							aDestroy = true;
							mLevelCoins = mSpecialCounts[aKind];
						}
						else if (aKind == 5)
						{
							if (aPiece->mFlash != NULL)
							{
								mEffects->Remove(aPiece->mFlash);
								aPiece->mFlash = NULL;
							}
							aLeaveList = true;
							aDestroy = true;
						}
						else if (aKind == 3)
						{
							mSpecialCounts[3] = mSpecialCounts[3] + 1;
							mLevelGems = mSpecialCounts[aKind];
							AddScore(BonusFor(aPiece->mIndex));
							aLeaveList = true;
							aDestroy = true;
						}
						else if (aKind == 0xd)
						{
							mSpecialCounts[13] = mSpecialCounts[13] + 1;
							mLevelClocks = mSpecialCounts[aKind];
							if (400 < 1000 - mFrozenTime)
							{
								Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO22_BOLD], GT(0x004e66c0),
									CellX(3) - 0x61 + mX, mY + 0x1e, 400, 0);
								aPoints->mStatic = false;
								mTexts.push_back(aPoints);
								mWidgetManager->AddWidget(aPoints);
							}
							mFrozenTime = mFrozenTime + 1000;
							mApp->PlaySample(0x34);
							aLeaveList = true;
							aDestroy = true;
						}
						else
						{
							if (aKind == 0xb)
							{
								mSpecialCounts[11] = mSpecialCounts[11] + 1;
								mLevelMatches = mSpecialCounts[aKind];
								if (mMatchesLeft < 0x14)
								{
									mMatchesLeft = mMatchesLeft + 1;
									if (2 < mMatchesLeft)
										mMatchesWarning = 0;      // the warning that the matches run out stops
								}
								else
									AddScore(500);
							}
							aLeaveList = true;
							aDestroy = true;
						}
					}
				}
				// the part that follows the specials' arrival (LAB_00424fca)
				int aKind = aPiece->mKind;
				bool aEnd = false;
				if (aKind == 1)
				{
					if (mUpdateCnt % 2 == 0 && aPiece->mY < (double) CellY(8))
						aPiece->mY = aPiece->mY + 1.0;
					if (mUpdateCnt - aPiece->mBorn >= 0x55)
					{
						if (mState == 0)
						{
							Effect* anEffect = mEffects->Add(0x14, true);
							anEffect->mLife = 2;
							anEffect->mDelay = 0;
							anEffect->mX = aPiece->mX;
							anEffect->mY = aPiece->mY;
							mApp->PlaySample(0x32);
							mApp->PlaySample(0x32);
							mShakeAmount = 2;
							mShakeTime = 0x19;
							AddFlash(aPiece, 1);
							Shock(aPiece->mRow, aPiece->mColumn, 0xc);
							SettleAll(false);
							mLevelBombs = mLevelBombs + 1;
						}
						aDestroy = true;
						aEnd = true;
					}
				}
				else if (aKind == 8)
				{
					if (mUpdateCnt - aPiece->mBorn >= 0x101)
					{
						aDestroy = true;
						aEnd = true;
					}
				}
				else if (aKind == 9)
				{
					if (mUpdateCnt - aPiece->mBorn >= 0x101)
					{
						aDestroy = true;
						aEnd = true;
					}
				}
				else if (aKind == 2 && !(aPiece->mBorn != mUpdateCnt && -1 < aPiece->mBorn - mUpdateCnt))
				{
					aDestroy = true;
					aEnd = true;
					if (mState == 0)
					{
						Effect* anEffect = mEffects->Add(5, true);
						anEffect->mAccelY = 0;
						anEffect->mDirY = 0;
						anEffect->mDrag = 0.86;
						anEffect->mLife = 1;
						anEffect->mDelay = 0;
						anEffect->mX = aPiece->mX + 10.0;
						anEffect->mY = aPiece->mY + 10.0;
					}
				}
				if (aEnd || aLeaveList)
					aRemoveFromList = true;
				(void) aArrived;
			}
			break;
		default:
			break;
		}
		(void) aSkipToEnd;
		if (aRemoveFromList)
		{
			aPiece->mInMoverList = false;
			anItr = mObjects.erase(anItr);
		}
		else
			++anItr;
		if (aDestroy)
			RemovePiece(aPiece, false);
	}
	while (anItr != mObjects.end());
	MarkDirty();
}
