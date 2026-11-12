// The Board's input and the first board (RocketManiaGame 0x427a90, 0x4281a0-0x428520, 0x4285b0-0x4285d0, 0x428780-0x428ae0,
// 0x421f30). See BoardLogic.cpp for the rules.

#include "Board.h"
#include "RocketApp.h"
#include "TalkingHead.h"
#include "MusicInterface.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "MTRand.h"
#include "WidgetManager.h"

#include <SDL.h>

using namespace Sexy;

// port: the original's float to int conversion (__ftol): truncation
static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x428780: the first board of the timed modes: every cell gets a new tile, again and again until no tiles connect the
// left edge to the right one. With the hints on (app+0x549, the Options hints checkbox, is clear) fixed tiles are written
// over the generated ones in the last rows (two in the classic mode, three in the strategy mode) before each check, so the
// first board is the same every time.
void Board::DealFirstBoard()
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
					if (anOld->mInMoverList)
						EraseMover(anOld);
					delete anOld;
				}
				mGrid[aRow][aColumn] = aPiece;
				aPiece->mX = (double) CellX(aColumn);
				aPiece->mY = (double) CellY(aRow);
				aPiece->mSlideStartX = aPiece->mSlideEndX = CellX(aColumn);
				aPiece->mSlideStartY = aPiece->mSlideEndY = CellY(aRow);
				aPiece->mType = 1;
				aPiece->mMoving = false;
			}
		}
		if (!mApp->mNoHints)
		{
			static const int sClassicKinds[12] = { 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0 };
			static const int sClassicRotations[12] = { 1, 1, 3, 0, 2, 2, 1, 2, 2, 1, 1, 1 };
			static const int sStrategyKinds[18] = { 1, 1, 1, 2, 0, 1, 0, 0, 1, 1, 1, 3, 1, 1, 2, 0, 0, 2 };
			static const int sStrategyRotations[18] = { 3, 0, 3, 0, 2, 1, 1, 1, 3, 0, 2, 2, 1, 2, 2, 1, 1, 2 };
			bool aClassic = mApp->mMode == 1;
			const int* aKinds = aClassic ? sClassicKinds : sStrategyKinds;
			const int* aRotations = aClassic ? sClassicRotations : sStrategyRotations;
			int aFirstRow = aClassic ? 5 : 4;
			for (int i = 0; i < (aClassic ? 2 : 3); i++)
			{
				for (int aColumn = 0; aColumn < mCols; aColumn++)
				{
					Piece* aPiece = mGrid[aFirstRow + i][aColumn];
					if (aPiece != NULL)
					{
						aPiece->mKind = aKinds[i * 6 + aColumn];
						aPiece->mRotation = aRotations[i * 6 + aColumn];
					}
				}
			}
		}
		aLoops++;
	}
	while (LeftReachesRight());
	SDL_Log("Looped %d times to fill board.", aLoops);
}

// 0x428ae0: finds the connections of the board as it is; returns whether the left edge reaches the right one
bool Board::LeftReachesRight()
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
		mPathEdges[i] = 0;
		mPathLeftLive[i] = false;
		mPathRightLive[i] = false;
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
		thePiece->mTilt = 1.5707963267948966;
	}
	else
	{
		thePiece->mRotation = (thePiece->mRotation + 3) & 3;
		thePiece->mTilt = -1.5707963267948966;
	}
	thePiece->mTurnedAt = mUpdateCnt;
	MarkDirty();
	mApp->PlaySample(3);
	mMoved = true;
	if (thePiece->mType == 4)
		CancelBurn(thePiece->mPath);
}

// 0x421f30: pauses (theOn) or resumes; the head goes down into its hole for the pause and up again after it, and the app's
// call 0x42d270 tells the buttons the pause state.
void Board::SetPause(bool theOn)
{
	if (mState != 0 && mApp->mBoard->mBuyRocketsState == 0)
		return;
	int anOld = mPause;
	if (anOld == 0 && !theOn)
		return;
	mPause = theOn ? anOld + 1 : anOld - 1;
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); ++anItr)
		(*anItr)->SetVisible(mPause == 0);
	if (mPause < 1)
	{
		if (0 < anOld)
		{
			mStartTicks = (int) SDL_GetTicks();
			mApp->mMusicInterface->FadeIn(SONG_GAME, -1, 0.666, false);
			mApp->mHead->StartSlide(true, 0);
			mApp->DisableBoardButtons(false);
		}
	}
	else if (anOld <= 0)
	{
		mSecondsPlayed = mSecondsPlayed + ((int) SDL_GetTicks() - mStartTicks) / 1000;
		mApp->mMusicInterface->FadeOut(SONG_GAME, false, 0.008);
		mApp->mHead->StartSlide(false, 0);
		mApp->DisableBoardButtons(true);
	}
	MarkDirty();
}

// 0x427a90: the space bar pauses and resumes
void Board::KeyChar(SexyChar theChar)
{
	if (theChar == ' ')
		SetPause(mPause == 0);
}

// 0x4285b0
void Board::MouseLeave()
{
	Widget::MouseLeave();
	mPointerX = -1;
	mPointerY = -1;
}

// 0x4285d0
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mPointerX = x;
	mPointerY = y;
}

// 0x4283e0. Arcade mode takes the tile under the pointer (a click at the top strip drops more tiles; the turn happens on
// release); the other modes turn the tile at once.
void Board::MouseDown(int x, int y, int theClickCount)
{
	if (theClickCount < 0)
		mRightClicked = true;
	Widget::MouseDown(x, y, theClickCount);
	if (0 < mPause)
	{
		SetPause(false);
		return;
	}
	if (mState != 0)
		return;
	int aMode = mApp->mMode;
	if (aMode == 0)
	{
		if (x < 0x102 || 0x217 < x || y < 5 || 0x38 < y)
		{
			Piece* aPiece = PieceAtPoint(x, y);
			if (aPiece == NULL || (aPiece->mType != 1 && aPiece->mType != 4))
				mPressedPiece = NULL;
			else
				mPressedPiece = aPiece;
		}
		else
		{
			mQuickDrops = 5;
			mFallTimer = mFallPeriod;
		}
	}
	else if (aMode == 1 || aMode == 2)
	{
		Piece* aPiece = PieceAtPoint(x, y);
		if (aPiece != NULL && (aPiece->mType == 1 || (aPiece->mType == 4 && mFlameCount == 0)))
			RotatePiece(aPiece, 0 < theClickCount);
	}
}

// 0x428520: arcade mode turns the tile that the pointer was pressed on when it is released on the same tile (unless it
// was dragged to another column)
void Board::MouseUp(int x, int y, int theClickCount)
{
	Widget::MouseUp(x, y, theClickCount);
	if (mMovedSinceDown)
	{
		mMovedSinceDown = false;
		return;
	}
	if (mState == 0 && mApp->mMode == 0)
	{
		Piece* aPiece = PieceAtPoint(x, y);
		if (aPiece != NULL && (aPiece->mType == 1 || (aPiece->mType == 4 && mFlameCount == 0)) && aPiece == mPressedPiece)
			RotatePiece(aPiece, 0 < theClickCount);
	}
}

// 0x4281a0: dragging the pressed tile more than half a cell sideways moves it to the next column when that column
// has room at its height
void Board::MouseDrag(int x, int y)
{
	(void) y;
	if (mApp->mMode != 0 || mPressedPiece == NULL)
		return;
	Piece* aPiece = mPressedPiece;
	if (aPiece->mType != 1 && (aPiece->mType != 4 || mFlameCount != 0))
	{
		mPressedPiece = NULL;
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
