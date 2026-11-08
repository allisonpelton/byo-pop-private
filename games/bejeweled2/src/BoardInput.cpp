// BoardInput.cpp: the mouse on the board (Sexy::Board::MovePiece, MouseDown, MouseDrag; 0x5aa2f1-0x5aaac2).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "GalaxyParser.h"
#include "PuzzleStateOverlay.h"
#include <stdlib.h>

using namespace Sexy;

// 0x5aa2f1: the gem moves one square in a direction (a swap with the neighbour, or a slide into an empty square)
bool Board::MovePiece(Piece* thePiece, int theDX, int theDY)
{
	if (theDX == 0 && theDY == 0)
		return false;

	int aCol = thePiece->mCol;
	int aRow = thePiece->mRow;
	unsigned int aNewCol = aCol + theDX;
	unsigned int aNewRow = aRow + theDY;
	// the same two squares as the move before (in either direction): the next hint is not free
	bool aSameMove = (aCol == m15eac && aRow == m15eb0 && (int)aNewCol == m15eb4 && (int)aNewRow == m15eb8) ||
		(aCol == m15eb4 && aRow == m15eb8 && (int)aNewCol == m15eac && (int)aNewRow == m15eb0);
	m15eac = -1;
	m15ea9 = aSameMove;
	if (aNewCol >= 8 || aNewRow >= 8)
		return false;

	// the move as a byte for the solutions: the square and the direction (0 left, 0x40 up, 0x80 right, 0xc0 down)
	unsigned char aMove = (unsigned char)(aRow << 3 | aCol);
	if (theDX >= 0)
	{
		if (theDY < 0)
			aMove |= 0x40;
		else if (theDX < 1)
			aMove |= 0xc0;
		else
			aMove |= 0x80;
	}
	mSolutionMove = TranslateMoveByte(aMove);
	MarkForUndo();
	Piece* aOther = mBoard[aNewRow][aNewCol];
	m15ea8 = true;
	if (aOther == NULL)
	{
		// a slide into an empty square
		mHyperSwap = false;
		thePiece->mHintTime = 0;
		mSwapPiece[1] = NULL;
		mSwapTime = 0;
		mSwapPiece[0] = thePiece;
		mSwapCenterX = (int)(GetColX(thePiece->mCol + theDX) + thePiece->mX) / 2;
		mSwapCenterY = (int)(GetRowY(thePiece->mRow + theDY) + thePiece->mY) / 2;
		thePiece->mSwapDX = theDX;
		mSwapPiece[0]->mSwapDY = theDY;
		mSwapPiece[0]->m7C = (int)mSwapPiece[0]->mX - mSwapCenterX;
		mSwapPiece[0]->m80 = (int)mSwapPiece[0]->mY - mSwapCenterY;
		mSwapPiece[0]->m5C = mUpdateCnt;
		mState = STATE_SWAPPING;
		if (!WillBeValidSwap())
			mState = STATE_SWAP_BACK;
	}
	else
		SwapPieces(thePiece, aOther);
	return true;
}

// a message box with the board's standard footer and button (the "Illegal Move" and "Wrong Piece" boxes)
void Board::DoNoticeDialog(const char* theHeader, const char* theLines)
{
	mApp->DoDialog(0x25, true, theHeader, theLines, "OK", 3);
}

// 0x5aa4e8
void Board::MouseDown(int theX, int theY, int theClickCount)
{
	if (mPauseCount != 0)
	{
		Unpause(true);
		return;
	}
	if (mHyperspace != NULL || mPendingPod != -1 || mUnpauseDelay > 0)
		return;

	m15f38 = true;
	if (mPuzzleOverlay != NULL && mState != 10 && mState != 0x12 && mState != 0xd && mState != 0xe &&
		mState != STATE_SWAPPING && mScreenState == SCREEN_SHOWN)
	{
		if (!mPuzzleOverlay->MouseOverDome(theX, theY))
		{
			if (mHoverPod != -1)
			{
				mState = 0xf;
				DeferredSelectPuzzlePod(mHoverPod);
				return;
			}
		}
		else
		{
			mApp->SetCursor(0);
			DoWhirlpool();
			PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
			aProgress->mPuzzleNum = (aProgress->mPuzzleNum / 5 + 1) * 5;
			m15f6c = aProgress->mPuzzleNum / 5;
		}
	}
	if (mState == 0x10 || mState == 0x11 || mState == 0x12)
		return;
	m15f30 = theX;
	m15f34 = theY;
	if (mState == 6 || mState == 7 || mState == 10 || mState == 0xb)
		return;

	if (theClickCount < 1)
	{
		// a right click lets go of the gem
		if (mIsPlanetIntro)
			return;
		mSelectedPiece = NULL;
	}
	else
	{
		Piece* aPiece = GetPieceAt(theX, theY);
		if (mSelectedPiece == NULL)
		{
			mSwapFade = 1.0f;
			if (aPiece == NULL)
				return;
			if (mIsPlanetIntro)
			{
				// the tutorial puzzle only lets the player pick the gem it points at
				if ((int)mTutorialMoves.size() <= mTutorialStep)
					return;
				if (mTutorialMoves[mTutorialStep].mCol == aPiece->mCol && mTutorialMoves[mTutorialStep].mRow == aPiece->mRow)
					AdvanceTutorial();
				else
				{
					DoNoticeDialog("Wrong Piece", "Please select the piece indicated under the CLICK HERE arrow.");
					return;
				}
			}
			m195c = 0;
			mSelectedPiece = aPiece;
			MarkDirtyFull();
			mApp->PlaySample(SOUND_SELECT, GetPanPosition((int)mSelectedPiece->mX + 0x2a));
			return;
		}

		if (mState != STATE_IDLE)
			return;
		// the second click: where it is, relative to the gem that is selected, says which neighbour to swap with
		float aSelX = (float)(mSelectedPiece->mX + 42.0);
		theX = theX - (int)mApp->ScaleNum(aSelX, 0.0f);
		float aSelY = (float)(mSelectedPiece->mY + 42.0);
		int aDY = theY - (int)mApp->ScaleNum(aSelY, 0.0f);
		if (mFlipped)
			aDY = -aDY;
		int aTargetCol = mSelectedPiece->mCol;
		int aTargetRow = mSelectedPiece->mRow;
		int aMax = abs(aDY);
		if (abs(theX) > aMax)
			aMax = abs(theX);
		int aMin = abs(aDY);
		if (abs(theX) < aMin)
			aMin = abs(theX);

		if (aMax > (int)mApp->ScaleNum(151.2, 0.0))
		{
			// too far from the gem
			if (!mIsPlanetIntro && mApp->mProfile->WantsHint(0))
				DoNoticeDialog("Illegal Move", "You must select two gems that are adjacent to each other.");
		}
		else if (aMax >= mApp->ScaleNum(0x2a, 0) - 2 && aMin <= mApp->ScaleNum(0x2a, 0) + 2)
		{
			if (abs(theX) > abs(aDY))
			{
				if (theX < 0)
					aTargetCol--;
				else
					aTargetCol++;
			}
			else if (aDY < 0)
				aTargetRow--;
			else
				aTargetRow++;
		}

		if (mIsPlanetIntro)
		{
			if ((int)mTutorialMoves.size() <= mTutorialStep)
				return;
			if (mTutorialMoves[mTutorialStep].mCol != aTargetCol || mTutorialMoves[mTutorialStep].mRow != aTargetRow)
			{
				DoNoticeDialog("Wrong Spot", "Please click the location indicated by the CLICK HERE arrow.");
				return;
			}
			AdvanceTutorial();
		}
		MovePiece(mSelectedPiece, aTargetCol - mSelectedPiece->mCol, aTargetRow - mSelectedPiece->mRow);
		mSelectedPiece = NULL;
	}
	MarkDirtyFull();
}

// 0x5aaac3: dragging the mouse far enough from where it went down moves the gem under it that way
void Board::MouseDrag(int theX, int theY)
{
	if (m15f38 && mSelectedPiece != NULL && mState == STATE_IDLE && !mIsPlanetIntro)
	{
		int aDX = theX - m15f30;
		int aDY = theY - m15f34;
		if (mFlipped)
			aDY = -aDY;
		if (abs(aDX) > 0x17 || abs(aDY) > 0x17)
		{
			int aMoveX, aMoveY;
			if (abs(aDY) < abs(aDX))
			{
				aMoveY = 0;
				aMoveX = aDX < 1 ? -1 : 1;
			}
			else
			{
				aMoveY = aDY < 1 ? -1 : 1;
				aMoveX = 0;
			}
			MovePiece(mSelectedPiece, aMoveX, aMoveY);
			mSelectedPiece = NULL;
			MarkDirtyFull();
		}
	}
}
