// The Board's input and the first board (RocketManiaGame 0x427a90, 0x4281a0-0x428520, 0x4285b0-0x4285d0, 0x428780-0x428ae0,
// 0x428e80-0x428ef0, 0x421f30). See BoardLogic.cpp for the rules.

#include "Board.h"
#include "RocketApp.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "MTRand.h"
#include "WidgetManager.h"

#include <SDL.h>

using namespace Sexy;

static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x428780: the first board of the timed modes: every cell gets a new tile, again and again until no tiles connect the
// left edge to the right one. (When the app's registered flag at app+0x549 is clear the original writes fixed tiles into
// the first rows; the port is registered, so that part is left out.)
void Board::F428780()
{
	int aLoops = 0;
	do
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aColumn = 0; aColumn < mCols; aColumn++)
			{
				Piece* aPiece = NewTop(aColumn);
				aPiece->mRow = aRow;
				aPiece->mColumn = aColumn;
				Piece* anOld = mGrid[aRow][aColumn];
				if (anOld != NULL)
				{
					if (anOld->mSpecial != NULL)
						RemovePiece(anOld->mSpecial, false);
					if (anOld->m94)
						EraseMover(anOld);
					delete anOld;
				}
				mGrid[aRow][aColumn] = aPiece;
				aPiece->mX = (double) CellX(aColumn);
				aPiece->mY = (double) CellY(aRow);
				aPiece->m40 = aPiece->m48 = CellX(aColumn);
				aPiece->m44 = aPiece->m4c = CellY(aRow);
				aPiece->mType = 1;
				aPiece->mMoving = false;
			}
		}
		aLoops++;
	}
	while (F428ae0());
	SDL_Log("Looped %d times to fill board.", aLoops);
}

// 0x428ae0: finds the connections of the board as it is; returns whether the left edge reaches the right one
bool Board::F428ae0()
{
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType == 1)
			{
				aPiece->mPath = 0;
				aPiece->mPathFrom = -1;
			}
		}
	}
	for (int i = 0; i < 30; i++)
	{
		m384[i] = 0;
		m3fc[i] = false;
		m41a[i] = false;
	}
	int aPath = 1;
	if (TraceUp(0, 8, 1, 2, &aPath, false))
		return true;
	return TraceUp(mCols - 1, 2, 2, 1, &aPath, false);
}

// 0x428340: a click turns a tile a quarter (the right button the other way) and it tilts for a moment
void Board::RotatePiece(Piece* thePiece, bool theCounter)
{
	if (!theCounter)
	{
		thePiece->mRotation = (thePiece->mRotation + 1) & 3;
		thePiece->m80 = 1.5707963267948966;
	}
	else
	{
		thePiece->mRotation = (thePiece->mRotation + 3) & 3;
		thePiece->m80 = -1.5707963267948966;
	}
	thePiece->m88 = mUpdateCnt;
	MarkDirty();
	mApp->PlaySample(3);
	m498 = true;
	if (thePiece->mType == 4)
		F4282a0(thePiece->mPath);
}

// 0x428e80: the tile at a cell has an open end on the edge of the board (left at the first column, right at the last)
bool Board::EdgeOpen(int theRow, int theColumn)
{
	Piece* aPiece = mGrid[theRow][theColumn];
	if (theColumn == 0 && (gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & 8) != 0)
		return true;
	if (theColumn == mCols - 1 && (gPipeMask[aPiece->mRotation + aPiece->mKind * 4] & 2) != 0)
		return true;
	return false;
}

// 0x428ef0: state 7, the new board is dealt row by row, from the bottom up, while a sweep effect climbs; each row's
// tiles are drawn again until none of them is open to the edge of the board
void Board::F428ef0()
{
	int aTime = mUpdateCnt - m348;
	if (m4fc != NULL && mEffects->Has(m4fc))
		m4fc->mY = (double) CellY(mRows - 1) + 14.0 -
			(double) aTime * 0.011111111111111112 * (double) (CellY(mRows - 1) - CellY(0));
	if (aTime % 10 != 0)
		return;
	int aRow = m34c;
	for (int aColumn = 0; aColumn < mCols; aColumn++)
	{
		Piece* aPiece = mGrid[aRow][aColumn];
		if (aPiece == NULL)
			continue;
		do
		{
			aPiece->mRotation = (int) (mRand->Next() & 3);
			aPiece->mKind = mGen->DrawKind();
		}
		while (EdgeOpen(aRow, aColumn));
	}
	if (m34c == 0)
	{
		mEffects->Remove(m4fc);
		m4fc = NULL;
		mState = 0;
	}
	else
		m34c--;
}

// 0x421f30: pauses (theOn) or resumes. Left out: the original's test of a flag in the app's object at app+0x598 for a
// game that is not running, the music fades (app+0x2f4), the talking head (0x416e50) and the app's call 0x42d270 that
// tells it the pause state.
void Board::F421f30(bool theOn)
{
	if (mState != 0 && mApp->mBoard->m100 == 0)
		return;
	int anOld = m350;
	if (anOld == 0 && !theOn)
		return;
	m350 = theOn ? anOld + 1 : anOld - 1;
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); ++anItr)
		(*anItr)->SetVisible(m350 == 0);
	if (m350 < 1)
	{
		if (0 < anOld)
			m368 = (int) SDL_GetTicks();
	}
	else if (anOld <= 0)
		m360 = m360 + ((int) SDL_GetTicks() - m368) / 1000;
	MarkDirty();
}

// 0x427a90: the space bar pauses and resumes
void Board::KeyChar(SexyChar theChar)
{
	if (theChar == ' ')
		F421f30(m350 == 0);
}

// 0x4285b0
void Board::MouseLeave()
{
	Widget::MouseLeave();
	m4f4 = -1;
	m4f8 = -1;
}

// 0x4285d0
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	m4f4 = x;
	m4f8 = y;
}

// 0x4283e0. Arcade mode takes the tile under the pointer (a click at the top strip drops more tiles; the turn happens on
// release); the other modes turn the tile at once. Left out: a click on the talking head (0x416a80).
void Board::MouseDown(int x, int y, int theClickCount)
{
	if (theClickCount < 0)
		m439 = true;
	Widget::MouseDown(x, y, theClickCount);
	if (0 < m350)
	{
		F421f30(false);
		return;
	}
	if (mState != 0)
		return;
	m50c = x;
	m510 = y;
	int aMode = mApp->mMode;
	if (aMode == 0)
	{
		if (x < 0x102 || 0x217 < x || y < 5 || 0x38 < y)
		{
			Piece* aPiece = PieceAtPoint(x, y);
			if (aPiece == NULL || (aPiece->mType != 1 && aPiece->mType != 4))
				m514 = NULL;
			else
				m514 = aPiece;
		}
		else
		{
			m37c = 5;
			m2dc = m444 - 100;
		}
	}
	else if (aMode == 1 || aMode == 2)
	{
		Piece* aPiece = PieceAtPoint(x, y);
		if (aPiece != NULL && (aPiece->mType == 1 || (aPiece->mType == 4 && m2f4 == 0)))
			RotatePiece(aPiece, 0 < theClickCount);
	}
}

// 0x428520: arcade mode turns the tile that the pointer was pressed on when it is released on the same tile (unless it
// was dragged to another column)
void Board::MouseUp(int x, int y, int theClickCount)
{
	Widget::MouseUp(x, y, theClickCount);
	if (m518)
	{
		m518 = false;
		return;
	}
	if (mState == 0 && mApp->mMode == 0)
	{
		Piece* aPiece = PieceAtPoint(x, y);
		if (aPiece != NULL && (aPiece->mType == 1 || (aPiece->mType == 4 && m2f4 == 0)) && aPiece == m514)
			RotatePiece(aPiece, 0 < theClickCount);
	}
}

// 0x4281a0: dragging the pressed tile more than half a cell sideways moves it to the next column when that column
// has room at its height
void Board::MouseDrag(int x, int y)
{
	(void) y;
	if (mApp->mMode != 0 || m514 == NULL)
		return;
	Piece* aPiece = m514;
	if (aPiece->mType != 1 && (aPiece->mType != 4 || m2f4 != 0))
	{
		m514 = NULL;
		return;
	}
	double aCentre = aPiece->mX + 21.0;
	int aDistance = Ftol((double) x - aCentre);
	if (aDistance < 0)
		aDistance = -aDistance;
	if (21 < aDistance)
	{
		int aColumn = aPiece->mColumn;
		if ((double) x <= aCentre)
		{
			if (0 < aColumn)
			{
				int aRow = LowestEmpty(aColumn - 1);
				if (aRow != -1 && aPiece->mRow <= aRow)
				{
					MovePiece(aPiece, aRow, aColumn - 1);
					SettleAll(false);
				}
			}
		}
		else if (aColumn < 5)
		{
			int aRow = LowestEmpty(aColumn + 1);
			if (aRow != -1 && aPiece->mRow <= aRow)
			{
				MovePiece(aPiece, aRow, aColumn + 1);
				SettleAll(false);
			}
		}
	}
}
