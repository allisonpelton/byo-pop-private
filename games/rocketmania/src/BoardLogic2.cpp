// The rules of the Board, second half (RocketManiaGame 0x422460-0x4285b0, 0x424730 and Update 0x426f40): the first
// board, the moving pieces, the launch of a connection, the timers of the three modes. See BoardLogic.cpp.

#include "Board.h"
#include "RocketApp.h"
#include "Compat2004.h"
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

static int Ftol(double theValue)
{
	return (int) theValue;
}

// ---------------------------------------------------------------------------------------------------------
// the lists of moving pieces

// 0x427f90
void Board::AddMover(Piece* thePiece)
{
	thePiece->m94 = true;
	mObjects.push_back(thePiece);
}

// 0x427ff0 and 0x428040: takes a piece out of the list (it stays alive)
void Board::EraseMover(Piece* thePiece)
{
	thePiece->m94 = false;
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
	aFlash->m7c = 0;
	AddMover(aFlash);
}

// 0x427d00: takes a piece off the board and deletes it. theFlag is passed to DestroySpecial for its special piece
void Board::RemovePiece(Piece* thePiece, bool theFlag)
{
	if (m514 == thePiece)
		m514 = NULL;
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
			m2f4--;
		if (thePiece->mFlash != NULL)
			mEffects->Remove(thePiece->mFlash);
		if ((mState == 4 || mState == 5) &&
			(thePiece->mKind == 0 || thePiece->mKind == 1 || thePiece->mKind == 0xc || thePiece->mKind == 6 || thePiece->mKind == 2))
		{
			Effect* anEffect = mEffects->Add(0xb, true);
			anEffect->mLife = 1;
			anEffect->mc0 = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
			anEffect->mf8 = 5.5;
			anEffect->m110 = 0x32;
			anEffect->m118 = 0x46;
			anEffect->m108 = 0.9;
		}
		if (thePiece->mKind == 0xd)
		{
			Effect* anEffect = mEffects->Add(5, true);
			anEffect->ma8 = 0;
			anEffect->mac = 0;
			anEffect->md0 = 0;
			anEffect->md4 = 0;
			anEffect->m108 = 0.93;
			anEffect->mLife = 1;
			anEffect->mc0 = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
		}
		break;
	}
	if (thePiece->mType != 3 && thePiece->mType != 6 && thePiece->mType != 5 && mState == 0)
		AddFlash(thePiece, 2);
	if (thePiece->m94)
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
	thePiece->mb0 = thePiece->mRow;
	thePiece->mb4 = thePiece->mColumn;
	thePiece->mac = 100;
	mGrid[thePiece->mRow][thePiece->mColumn] = NULL;
	mGrid[theRow][theColumn] = thePiece;
	thePiece->mRow = theRow;
	thePiece->mColumn = theColumn;
	thePiece->mX = (double) CellX(theColumn);
	thePiece->mY = (double) CellY(theRow);
	if (thePiece->mType == 4)
		F4282a0(thePiece->mPath);
	MarkDirty();
	m518 = true;
	m519 = true;
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
			thePiece->m38 = 0.2 / (double) (aTarget - aOldRow);
			thePiece->m40 = Ftol(thePiece->mX);
			thePiece->m44 = Ftol(thePiece->mY);
			thePiece->m48 = CellX(thePiece->mColumn);
			thePiece->m4c = CellY(aTarget);
			thePiece->m30 = 0;
		}
	}
}

// 0x422bc0: every tile with an empty cell under it falls
void Board::SettleAll(bool theFlag)
{
	m45c = theFlag;
	m458 = mUpdateCnt;
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
	double aSpeed = thePiece->m20 + 0.13;
	thePiece->m20 = aSpeed;
	thePiece->mX = thePiece->m18 + thePiece->mX;
	double aY = aSpeed + thePiece->mY;
	thePiece->mY = aY;
	if (432.0 <= aY)
	{
		thePiece->mY = 432.0 - (aY - 432.0);
		thePiece->m20 = aSpeed * -0.5;
		thePiece->m18 = thePiece->m18 * 0.93;
	}
	if (thePiece->mX <= aLeft)
	{
		thePiece->mX = (aLeft - thePiece->mX) + aLeft;
		thePiece->m18 = thePiece->m18 * -0.8;
	}
	if (aRight <= thePiece->mX)
	{
		thePiece->mX = aRight - (thePiece->mX - aRight);
		thePiece->m18 = thePiece->m18 * -0.8;
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
				aBomb->ma8--;
				if (aBomb->ma8 == 0)
				{
					Effect* anEffect = mEffects->Add(0x15, true);
					anEffect->mLife = 1;
					anEffect->mc0 = 0;
					anEffect->mX = aBomb->mX;
					anEffect->mY = aBomb->mY;
					mApp->PlaySample(0x32);
					RemovePiece(aBomb, false);
				}
				else
				{
					Effect* anEffect = mEffects->Add(0x16, true);
					anEffect->mLife = 1;
					anEffect->mc0 = 0;
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
	if (mApp->mMode != 0 || theForget || true)
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
		m2d1[aRow] = false;   // board+0x2d1.. and +0x2c8..: the flags that say an edge is connected
		m2c8[aRow] = false;
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
		aPiece->m94 = false;
		anItr = mObjects.erase(anItr);
		delete aPiece;
	}
}

// 0x422620: the first board of a game (not yet translated into a loaded game's pieces)
void Board::SetupBoard(bool theKeep, bool theLoaded)
{
	m4a8 = false;
	m4a9 = false;
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
		m384[i] = 0;
		m3fc[i] = false;
		m41a[i] = false;
	}
	if (theLoaded)
		return;
	for (int i = 0; i < m7c; i++)
	{
		Piece* aPiece = new Piece(mUpdateCnt);
		aPiece->mColumn = -1;
		unsigned int aRow = (unsigned int) mRand->Next() % (unsigned int) mRows;
		while (mLeft[aRow] != NULL)
			aRow = (aRow + 1) % mRows;
		aPiece->mRow = (int) aRow;
		aPiece->mX = (double) CellX(aPiece->mColumn);
		aPiece->mY = (double) CellY(aPiece->mRow);
		aPiece->mIndex2 = 0;
		aPiece->mType = 5;
		mLeft[aRow] = aPiece;
	}
	for (int i = 0; i < m80; i++)
	{
		unsigned int aRow = (unsigned int) mRand->Next() % (unsigned int) mRows;
		while (mRight[aRow] != NULL)
			aRow = (aRow + 1) % mRows;
		AddRight(aRow);
	}
	int aMode = mApp->mMode;
	if (aMode == 0)
	{
		m448 = 0;
		mSpare = NewTop(0);
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = new Piece(mUpdateCnt);
			aPiece->mColumn = aColumn;
			aPiece->mRow = 0;   // (the original stores the column at +0x54 and 0 at +0x50: a tile above the first row)
			aPiece->mRow = -1;
			aPiece->mX = (double) CellX(aColumn);
			aPiece->mY = (double) (CellY(0) - 0x4a);
			aPiece->mKind = mGen->DrawKind();
			aPiece->mType = 1;
			aPiece->mRotation = (int) (mRand->Next() & 3);
			mTop[aColumn] = aPiece;
		}
		// the first top line of an unregistered game has fixed tiles (the app's flag at 0x549 is clear)
	}
	else if (aMode == 1 || aMode == 2)
	{
		m454 = 0;
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = new Piece(mUpdateCnt);
			aPiece->mColumn = aColumn;
			aPiece->mRow = -1;
			aPiece->mX = (double) CellX(aColumn);
			aPiece->mY = (double) (CellY(0) - 0x4a);
			aPiece->mKind = mGen->DrawKind();
			aPiece->mType = 1;
			aPiece->mRotation = (int) (mRand->Next() & 3);
			mTop[aColumn] = aPiece;
		}
		F428780();
	}
}

// ---------------------------------------------------------------------------------------------------------
// connections

// 0x425410: a connection from the left edge to the right edge is found: its tiles start to burn
void Board::StartBurn(int thePath)
{
	m2e8 = true;
	m2f0 = thePath;
	m2ec = 0x42;
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
		if (m2c8[i])
		{
			for (int j = 0; j < mRows; j++)
			{
				if (m2d1[j])
					return true;
			}
			return false;
		}
	}
	return false;
}

// 0x426070: for each row, floods from the tile at the edge column that can connect to the edge (theMask) and
// notes how many tiles it reached (m280 for the left edge, m2a4 for the right); both edges together start a burn
void Board::Trace(int theColumn, unsigned int theMask, int theEnd, int theOtherEnd, int* thePath, bool theStart)
{
	unsigned int aBoth = (unsigned int) theEnd | (unsigned int) theOtherEnd;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		Piece* aPiece = mGrid[aRow][theColumn];
		for (int j = 0; j < mRows; j++)
		{
			m2c8[j] = false;
			m2d1[j] = false;
		}
		if (aPiece != NULL && aPiece->mType == 1 && aPiece->mPath == 0 &&
			(gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & theMask) != 0)
		{
			int aCount = 0;
			unsigned int aEnds = 0;
			Flood(theColumn, aRow, *thePath, &aCount, &aEnds, aRow);
			if ((aEnds & aBoth) == aBoth)
			{
				if (theStart && BothEnds() && !m2e8)
				{
					StartBurn(*thePath);
					mApp->PlaySample(2);
					if (m4a9 || m4a8)
						m4da = true;
				}
			}
			else if (theColumn == 0)
				m280[aRow] = aCount;
			else
				m2a4[aRow] = aCount;
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
			m2c8[j] = false;
			m2d1[j] = false;
		}
		if (aPiece != NULL && aPiece->mType == 1 && aPiece->mPath == 0 &&
			(gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & theMask) != 0)
		{
			int aCount = 0;
			unsigned int aEnds = 0;
			Flood(theColumn, aRow, *thePath, &aCount, &aEnds, aRow);
			if ((aEnds & aBoth) == aBoth)
			{
				if (theStart && BothEnds() && !m2e8)
				{
					StartBurn(*thePath);
					mApp->PlaySample(2);
					if (m4a9 || m4a8)
						m4da = true;
				}
				aFound = true;
			}
			else if (theColumn == 0)
				m280[aRow] = aCount;
			else
				m2a4[aRow] = aCount;
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
			if (0 < aPiece->mac)
				aPiece->mac = aPiece->mac - 2;
			if (aPiece->m80 <= 0.0)
			{
				if (aPiece->m80 < 0.0)
				{
					aPiece->m80 = ((double) (mUpdateCnt - aPiece->m88) + 1.0) * 0.025 + aPiece->m80;
					if (0.0 < aPiece->m80 || aPiece->m80 < -1.57079632675)
						aPiece->m80 = 0;
				}
			}
			else
			{
				aPiece->m80 = aPiece->m80 - ((double) (mUpdateCnt - aPiece->m88) + 1.0) * 0.025;
				if (aPiece->m80 < 0.0 || 1.57079632675 < aPiece->m80)
					aPiece->m80 = 0;
			}
		}
	}
	for (int i = 0; i < mRows; i++)
	{
		m280[i] = 0;
		m2a4[i] = 0;
	}
	for (int i = 0; i < 30; i++)
	{
		m384[i] = 0;
		m3fc[i] = false;
		m41a[i] = false;
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
	aFlame->m40 = Ftol(aFlame->mX);
	aFlame->m44 = Ftol(aFlame->mY);
	aFlame->m30 = 0;
	aFlame->mMoving = true;
	if (!theBlocked)
	{
		aFlame->m48 = CellX(theToColumn);
		aFlame->m4c = CellY(theToRow);
		aFlame->m38 = 0.16666666666666666;
		Piece* aTarget = NULL;
		if (theToRow < mRows && -1 < theToRow && theToColumn < mCols && -1 < theToColumn)
			aTarget = mGrid[theToRow][theToColumn];
		if (aTarget != NULL)
		{
			aTarget->m60 = true;
			F429050();
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
		aFlame->m48 = Ftol((aX - aFlame->mX) * 0.5 + aFlame->mX);
		double aY = (double) CellY(theToRow);
		aFlame->m4c = Ftol((aY - aFlame->mY) * 0.5 + aFlame->mY);
		aFlame->m38 = 0.3333333333333333;
		aFlame->m61 = true;
	}
	m2f4++;
}

// 0x425e30: the burning connection reached the right edge at some rows: score, rocket and new right piece for each
int Board::ResolveLaunch(int thePath)
{
	int aCount = 0;
	for (int i = 0; i < 9; i++)
		m250[i] = -1;
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
			m250[aCount] = aRocket->mIndex;
			MakeRocket(aRow);
			if (mApp->mMode != 2)
				MakeFlame0();
			aCount++;
			mGen->CountPiece(aRocket);
			RemovePiece(aRocket, false);
			m374 = m374 + 1;
			m378 = m378 + 1;
			AddRight(aRow);
		}
	}
	double aFraction = (double) m374 / (double) m74;
	if (1.0 < aFraction)
		aFraction = 1.0;
	m300 = aFraction;
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
	if (!m4da)
	{
		if (mApp->mMode == 2)
			HitBombs();
	}
	else
	{
		m4da = false;
		AddScore(500);
		m4b0 = 0xfa;
		mApp->PlaySample(0x1d);
		if (mApp->mMode == 2)
			AttachSpecial10();
	}
	m498 = true;
	m4a8 = true;
	int aLaunches = ResolveLaunch(m2f0);
	switch (aLaunches)
	{
	case 3: m4b8 = 1; m4b4 = 0x14; break;
	case 4: m4b8 = 1; m4b4 = 0x1e; break;
	case 5: m4b8 = 2; m4b4 = 0x2d; break;
	case 6: m4b8 = 3; m4b4 = 0x3c; break;
	case 7: m4b8 = 4; m4b4 = 0x4b; break;
	case 8: m4b8 = 4; m4b4 = 0x5a; break;
	case 9: m4b8 = 5; m4b4 = 100; break;
	}
	int aThisPath = m2f0;
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
	m370 = m370 + aRemoved;
	AddScore(aRemoved * 5);
	if (mC4 < aRemoved)
		mC4 = aRemoved;
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
			int aEffect = m250[i];
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
	m2f0 = -1;
	m2e8 = false;
	int aMode = mApp->mMode;
	if (aMode != 2 && 1.0 <= m300 && mState != 4 && mState != 5)
	{
		bool aEnd = true;
		if (mState == 1)
		{
			if (aMode != 1)
				aEnd = false;
			else
				m4d9 = true;
		}
		if (aEnd)
			EndLevelA();
	}
	if (mApp->mMode == 0)
	{
		m37c = 5;
		m2dc = m444 - 100;
	}
	m490 = 0;
	m494 = 0;
	F429080();
}

// 0x425b50: the burn ends at the left edge (no tile at the right): fuses on the first column light
void Board::FinishBurn()
{
	m490 = 0;
	m494 = 0;
	bool aLit[9];
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
		}
	}
	mApp->PlaySample(0x18);
	int aCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		if (aLit[aRow] && (aCount = aCount + 1, mRows / 2 <= aCount))
		{
			F429060(aRow);
			break;
		}
	}
	if (mApp->mMode == 2 && 0 < mRows && !m4a9 && !m4a8 && !m4da && 0 < m108)
	{
		MakeFlame1();
		m108 = m108 - 1;
		if (m108 == 2)
			m10c = 600;
	}
}

// 0x426580
void Board::SmoothFraction()
{
	if (m300 < m308)
	{
		m308 = m308 - 0.01;
		if (m308 < m300)
			m308 = m300;
	}
	if (m308 < m300)
	{
		m308 = m308 + 0.0035;
		if (m300 < m308)
			m308 = m300;
	}
}

// 0x426620: the night runs out (classic mode) and its warnings
void Board::NightTimer()
{
	if (mState != 0)
		return;
	if (m4bc < 1)
	{
		if (m498)
			m310 = m310 - m320;
		if (m310 <= 10.0 && !m499)
		{
			m499 = true;
			mApp->PlaySample(6);
		}
		if (6 < mLevel && !m4c8 && !m4c9 && (m310 <= m4d0 || m310 <= 10.0))
			m4c9 = true;
		if (m310 <= 0.0 && 0.0 < m320)
		{
			m310 = 0;
			F421de0(0);
		}
		if (m310 < m318)
		{
			m318 = m318 - 0.01;
			if (m318 < m310)
				m318 = m310;
		}
		if (m318 < m310)
		{
			m318 = m318 + 0.0035;
			if (m310 < m318)
				m318 = m310;
		}
		double aLeft = 1.0 - m310 / m68;
		if (0.9 <= aLeft && aLeft < 1.0 && mUpdateCnt % 100 == 0)
			mApp->PlaySample(5);
	}
	else
	{
		m4bc = m4bc - 1;
		if (m4bc == 0)
		{
			Effect* anEffect = mEffects->Add(0x17, true);
			anEffect->mLife = 5;
			anEffect->mc0 = 0;
			anEffect->mX = 365.0;
			anEffect->mY = 50.0;
			mApp->PlaySample(0x35);
		}
	}
}

// 0x426860: the timers of the three modes: the next tile falls, the hints appear
void Board::Timers()
{
	if (0 < m4b4)
		m4b4--;
	if (mState != 0)
	{
		if (mState == 1 && 0 < m440 && --m440 == 0)
		{
			mState = 2;
			// not translated: the original opens the level-up dialog (0x40c2c0, id 0xd) with 5 and the board's
			// random generator, the talking head is told (0x416590, 0x416e80) and made a child of the board
		}
		return;
	}
	if (0 < m4b0)
		m4b0--;
	if (0 < m10c)
		m10c--;
	if (!m2e8)
	{
		int aMode = mApp->mMode;
		if (aMode == 0)
		{
			if (m44c < 500)
			{
				if (mGrid[0][m448] == NULL)
					m44c = 500;
				else if (--m44c == 0)
					F421de0(1);
			}
			if (m44c == 500)
			{
				if (m498 || m2f8 < mGen->Rows() * mCols)
					m2dc = m2dc + 1;
				if (m444 <= m2dc)
				{
					m2dc = 0;
					m2e0 = 0;
					DropTop();
				}
			}
		}
		else if ((aMode == 1 || aMode == 2) && (m2dc = m2dc + 1, m450 <= m2dc))
		{
			m2dc = 0;
			m2e0 = 0;
			DropTopTimed();
		}
		if (m2e0 < m2dc && (m2e0 = m2e0 + 2, m2dc < m2e0))
			m2e0 = m2dc;
	}
	for (int i = 0; i < 6; i++)
	{
		if (m340[i] || m328[i] <= 0 || --m328[i] != 0 || m4c0)
			continue;
		switch (i)
		{
		case 0:
			// the original asks the app whether it is active (0x42f6f0) and has the head speak
			m340[i] = true;
			break;
		case 1:
			if (0 < mLevel)
				m340[i] = true;
			break;
		case 2:
			if (mApp->mMode == 0)
				m340[i] = true;
			break;
		case 3:
			if (2 < mLevel)
				m340[i] = true;
			break;
		case 4:
			if (3 < mLevel)
				m340[i] = true;
			break;
		case 5:
			if (mApp->mMode == 0)
			{
				Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO14_BOLD], "Click here to drop more tiles",
					CellX(3) + mX, mY + 0x1e, 600, 0);
				aPoints->mStatic = true;
				mTexts.push_back(aPoints);
				mWidgetManager->AddWidget(aPoints);
				m340[i] = true;
				m4c0 = true;
			}
			break;
		}
	}
	if (0 < m43c)
		m43c--;
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
			int aX = aPiece->m48;
			if (aPiece->m40 != aPiece->m48)
				aX = Ftol((1.0 - aPiece->m30) * (double) aPiece->m40 + (double) aPiece->m48 * aPiece->m30);
			aPiece->mX = (double) aX;
			int aY = aPiece->m4c;
			if (aPiece->m44 != aPiece->m4c)
				aY = Ftol((1.0 - aPiece->m30) * (double) aPiece->m44 + (double) aPiece->m4c * aPiece->m30);
			aPiece->mY = (double) aY;
		}
		bool aRemoveFromList = false;
		switch (aPiece->mType)
		{
		case 0:
			if (aMoving)
			{
				aPiece->m30 = aPiece->m30 + aPiece->m38;
				if (1.0 <= aPiece->m30)
				{
					if (8 < aPiece->mRow + 1 || mGrid[aPiece->mRow + 1][aPiece->mColumn] != NULL)
					{
						aPiece->mType = 1;
						aPiece->mMoving = false;
						aPiece->mX = (double) aPiece->m48;
						aPiece->mY = (double) aPiece->m4c;
						Ping();
						aRemoveFromList = true;
						break;
					}
					mGrid[aPiece->mRow][aPiece->mColumn] = NULL;
					aPiece->mRow = aPiece->mRow + 1;
					mGrid[aPiece->mRow][aPiece->mColumn] = aPiece;
					aPiece->m40 = aPiece->m48;
					aPiece->m44 = aPiece->m4c;
					aPiece->m30 = aPiece->m30 - 1.0;
					aPiece->m48 = CellX(aPiece->mColumn);
					aPiece->m4c = CellY(aPiece->mRow);
				}
			}
			break;
		case 2:
			if (aMoving)
			{
				aPiece->m30 = aPiece->m30 + aPiece->m38;
				if (1.0 <= aPiece->m30)
				{
					aPiece->mType = 1;
					aPiece->mMoving = false;
					aPiece->mX = (double) aPiece->m48;
					aPiece->mY = (double) aPiece->m4c;
					if (mApp->mMode != 1 && mApp->mMode != 2)
						Ping();
					aRemoveFromList = true;
				}
			}
			break;
		case 3:
			if (aPiece->mKind != 2)
			{
				if (mUpdateCnt % 7 == 0 && (aPiece->m7c = aPiece->m7c + 1, 13 < aPiece->m7c))
				{
					aDestroy = true;
					aRemoveFromList = true;
				}
			}
			else if (mUpdateCnt % 0xb == 0 && (aPiece->m7c = aPiece->m7c + 1, 3 <= aPiece->m7c))
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
					double aOld = aPiece->m30;
					aPiece->m30 = aPiece->m38 + aOld;
					int aKind = aPiece->mKind;
					if (aPiece->m38 + aOld < 1.0)
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
							aPiece->m7c = (aPiece->m7c + 1) % 10;
						if (!aPiece->m61)
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
										if (aNext == NULL || aNext->m60 ||
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
						aPiece->mX = (double) aPiece->m48;
						aPiece->mY = (double) aPiece->m4c;
					}
					else
					{
						aPiece->mMoving = false;
						aPiece->mX = (double) aPiece->m48;
						aPiece->mY = (double) aPiece->m4c;
						aArrived = true;
						if (aKind == 7)
						{
							int anIndex = aPiece->mIndex;
							if (anIndex == 0)
							{
								mApp->PlaySample(9);
								m8c[aKind] = m8c[aKind] + 1;
							}
							else if (anIndex == 1)
							{
								mApp->PlaySample(10);
								m8c[aKind] = m8c[aKind] + 2;
							}
							else if (anIndex == 2)
							{
								mApp->PlaySample(0xb);
								m8c[aKind] = m8c[aKind] + 5;
							}
							else
							{
								mApp->PlaySample(10);
								mApp->PlaySample(0xb);
								m8c[aKind] = m8c[aKind] + 10;
							}
							aLeaveList = true;
							aDestroy = true;
							mCc = m8c[aKind];
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
							m8c[3] = m8c[3] + 1;
							mD0 = m8c[aKind];
							AddScore(BonusFor(aPiece->mIndex));
							aLeaveList = true;
							aDestroy = true;
						}
						else if (aKind == 0xd)
						{
							m8c[13] = m8c[13] + 1;
							mD8 = m8c[aKind];
							if (400 < 1000 - m4bc)
							{
								Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO22_BOLD], GT(0x004e66c0),
									CellX(3) - 0x61 + mX, mY + 0x1e, 400, 0);
								aPoints->mStatic = false;
								mTexts.push_back(aPoints);
								mWidgetManager->AddWidget(aPoints);
							}
							m4bc = m4bc + 1000;
							mApp->PlaySample(0x34);
							aLeaveList = true;
							aDestroy = true;
						}
						else
						{
							if (aKind == 0xb)
							{
								m8c[11] = m8c[11] + 1;
								mDc = m8c[aKind];
								if (m108 < 0x14)
									m108 = m108 + 1;
								else
									AddScore(500);
								if (m108 > 2 && m108 <= 0x14 && false)
									m10c = 0;
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
							anEffect->mc0 = 0;
							anEffect->mX = aPiece->mX;
							anEffect->mY = aPiece->mY;
							mApp->PlaySample(0x32);
							mApp->PlaySample(0x32);
							m4b8 = 2;
							m4b4 = 0x19;
							AddFlash(aPiece, 1);
							Shock(aPiece->mRow, aPiece->mColumn, 0xc);
							SettleAll(false);
							mD4 = mD4 + 1;
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
						anEffect->ma8 = 0;
						anEffect->mac = 0;
						anEffect->md0 = 0;
						anEffect->md4 = 0;
						anEffect->m108 = 0.92;
						anEffect->mLife = 1;
						anEffect->mc0 = 0;
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
			aPiece->m94 = false;
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
