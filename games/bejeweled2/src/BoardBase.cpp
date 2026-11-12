// BoardBase.cpp: the small parts of the Board (Sexy::Board, 0x59483b-0x5ae34e): the end of game statistics, the board
// geometry, the pause counters, the hint search, the swap bookkeeping, the sliding of the board in and out, and the
// puzzle solution helpers. The larger parts are in BoardLogic.cpp (matching, falling, scoring), BoardUpdate.cpp (DoUpdate),
// BoardInput.cpp (mouse and keys), BoardSave.cpp (the saved game and the undo states) and BoardDraw.cpp.

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "PodDialog.h"
#include "Hyperspace.h"
#include "PuzzleStateOverlay.h"
#include "GalaxyMap.h"
#include "EffectOverlay.h"
#include "WidgetManager.h"
#include "SoundManager.h"
#include "Buffer.h"
#include "MemoryImage.h"
#include <math.h>

using namespace Sexy;

// the number of updates a hint stays lit (the original's data at 0x66b054 never changes)
static const int HINT_TIME = 290;

// 0x59483b: the figures of a finished (or abandoned) game go into the profile and the app's counters
void Board::ProcessEndGameInfo()
{
	if (mEndInfoProcessed)
		return;

	int aTotalSeconds = (unsigned int)(mUpdateCnt - mPlayStartTick) / 100 + mPlayedSeconds;
	m15e14 = aTotalSeconds;
	mApp->mProfile->mStat[1] += aTotalSeconds - m15e18;
	m15e18 = m15e14;

	if (!mIsPuzzle && !mCheated)
	{
		mApp->mProfile->mStat[6] += m15e7c - m15e80;
		if (mApp->mProfile->mStat[6] > 9)
			mApp->mProfile->DisableHint(1);
		m15e80 = m15e7c;

		mApp->mProfile->mStat[7] += m15e84 - m15e88;
		if (mApp->mProfile->mStat[7] > 4)
			mApp->mProfile->DisableHint(2);
		m15e88 = m15e84;

		mApp->mProfile->mStat[3] += m15e44 - m15e48;
		m15e48 = m15e44;
	}

	if (!mCheated)
	{
		if (!mIsEndless)
			mApp->mProfile->mStat[0] = std::max(mApp->mProfile->mStat[0], mScore);
		mApp->mProfile->mStat[4] = std::max(mApp->mProfile->mStat[4], m15e6c);
		mApp->mProfile->mStat[5] = std::max(mApp->mProfile->mStat[5], m15e70);
	}

	int aSeconds = m1998 / 100;
	if (mIsAction)
		mApp->mModeSeconds[1] += aSeconds;
	else if (mIsPuzzle)
		mApp->mModeSeconds[2] += aSeconds;
	else if (mIsEndless)
		mApp->mModeSeconds[3] += aSeconds;
	else
		mApp->mModeSeconds[0] += aSeconds;

	mEndInfoProcessed = true;
}

// 0x594a01
void Board::AddedToManager(WidgetManager* theManager)
{
	Widget::AddedToManager(theManager);

	if (mIsPuzzle)
	{
		if (mPuzzleOverlay != NULL)
			theManager->AddWidget((Widget*)mPuzzleOverlay);
		theManager->AddWidget(mThirdButton);
	}
	else if (mIsAction)
		theManager->AddWidget(mThirdButton);

	theManager->AddWidget(mMenuButton);
	theManager->AddWidget(mHintButton);
	theManager->AddWidget((Widget*)mEffectOverlay);
}

// 0x594a71
void Board::RemovedFromManager(WidgetManager* theManager)
{
	Widget::RemovedFromManager(theManager);

	if (mIsPuzzle)
	{
		if (mPuzzleOverlay != NULL)
			theManager->RemoveWidget((Widget*)mPuzzleOverlay);
		theManager->RemoveWidget(mThirdButton);
	}
	else if (mIsAction)
		theManager->RemoveWidget(mThirdButton);

	if (mGalaxyMap != NULL)
		theManager->RemoveWidget((Widget*)mGalaxyMap);
	theManager->RemoveWidget((Widget*)mEffectOverlay);
	if (mHyperspace != NULL)
		theManager->RemoveWidget(mHyperspace);
	theManager->RemoveWidget(mMenuButton);
	theManager->RemoveWidget(mHintButton);
	if (mTutorialButton1 != NULL)
		theManager->RemoveWidget(mTutorialButton1);
	if (mTutorialButton2 != NULL)
		theManager->RemoveWidget(mTutorialButton2);
	if (mTutorialWidget != NULL)
		theManager->RemoveWidget(mTutorialWidget);
}

// 0x594b3b
void Board::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);
	mMenuButton->SetVisible(isVisible);
	mHintButton->SetVisible(isVisible);
	if (mThirdButton != NULL)
		mThirdButton->SetVisible(isVisible);
}

// 0x594ba4
int Board::GetColX(int theCol)
{
	if (!mIsPuzzle || mIsSecret)
		return theCol * 84 + 0x133;
	return theCol * 84 + 0x13e;
}

// 0x594bd5
int Board::GetRowY(int theRow)
{
	if (!mIsPuzzle || mIsSecret)
	{
		if (!mFlipped)
			return theRow * 84 + 0x20;
		return 0x26c - theRow * 84;
	}
	return theRow * 84 + 0x2b;
}

// 0x5957f3
void Board::GetBoardCenter(int* theX, int* theY)
{
	int aWidth = 84 - GetColX(0) + GetColX(7);
	*theX = aWidth / 2 + GetColX(0);
	int aHeight = 84 - GetRowY(0) + GetRowY(7);
	*theY = aHeight / 2 + GetRowY(0);
}

// 0x5953ef
int Board::GetPanPosition(int theX)
{
	int aWidth = 84 - GetColX(0) + GetColX(7);
	int aOffset = theX - aWidth / 2 - GetColX(0);
	return (int)(((double)aOffset / (double)aWidth + (double)aOffset / (double)aWidth) * 800.0);
}

// 0x594d55: the worker thread (it runs in line in the port) has made the next backdrop; it replaces the old one
void Board::ProcessLevelImages()
{
	if (mNextBackdropImage != NULL)
	{
		delete mBackdropImage;
		mBackdropImage = mNextBackdropImage;
		mNextBackdropImage = NULL;
	}
}

// 0x594d97: the tutorial's sweep over the board moves one square on
void Board::AdvanceSweep()
{
	mSweepB = -1;
	mSweepA = -1;
	mSweep1++;
	int v = mSweep1 + mSweep0;
	mSweepTimer = 120;
	bool aDone = false;
	if (v > 7)
	{
		mSweep1 = 0;
		mSweep2++;
		if (mSweep2 < 8)
		{
			mSweepB = mSweep2 - 1;
			aDone = true;
		}
		else
		{
			mSweep2 = 0;
			v = ++mSweep0;
		}
	}
	if (!aDone)
		mSweepA = v - 1;
	if (mSweep0 > 6)
	{
		mSweep1 = 0;
		mSweep2 = 7;
		mSweep0 = 7;
	}
}

// 0x594eff: the board answers the mouse (not during the game over states)
bool Board::CanPlay()
{
	return !mNoMoveYet && mState != 6 && mState != 7 && mState != 0x14;
}

// 0x594f20: a checksum of the board's gems (type, flag and counter of each square)
unsigned int Board::GetBoardHash()
{
	Buffer aBuffer;
	aBuffer.WriteLong(0);
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			unsigned char aByte;
			if (aPiece == NULL)
				aByte = 0xff;
			else
			{
				aBuffer.WriteByte((unsigned char)aPiece->mType);
				aBuffer.WriteByte(aPiece->m70 ? 1 : 0);
				aByte = (unsigned char)aPiece->m58;
			}
			aBuffer.WriteByte(aByte);
		}
	}
	return aBuffer.GetCRC32(0);
}

// 0x594fd2: the puzzle comes mirrored (left to right) half of the time, and always with its colours shuffled
void Board::MakePuzzleCrazy()
{
	unsigned int aBit = Rand() & 0x80000001;
	if (aBit == 0)
	{
		Piece* aOld[8][8];
		memcpy(aOld, mBoard, sizeof(aOld));
		for (int aRow = 0; aRow < 8; aRow++)
		{
			for (int aK = 0; aK < 8; aK++)
			{
				int aNewCol = 7 - aK;
				Piece* aPiece = aOld[aRow][aK];
				if (aPiece != NULL)
				{
					aPiece->mCol = aNewCol;
					aPiece->mX = (float)GetColX(aNewCol);
				}
				mBoard[aRow][aNewCol] = aPiece;
			}
		}
		mCrazy = true;
	}
	else
		mCrazy = false;

	int aColors[7] = {0, 1, 2, 3, 4, 5, 6};
	for (int aCount = 7; aCount != 0; aCount--)
	{
		int aIdx1 = Rand();
		int aIdx2 = Rand();
		int aTemp = aColors[aIdx1 % 7];
		aColors[aIdx1 % 7] = aColors[aIdx2 % 7];
		aColors[aIdx2 % 7] = aTemp;
	}
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL && aPiece->mType < 7)
				aPiece->mType = aColors[aPiece->mType];
		}
	}
}

// 0x5950f5
void Board::ClearBoard()
{
	mSelectedPiece = 0;
	mSwapPiece[0] = NULL;
	mSwapPiece[1] = NULL;
	if (mState == 2 || mState == 4 || mState == 3)
		mState = 1;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			if (mBoard[aRow][aCol] != NULL)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				mBoard[aRow][aCol] = NULL;
				delete aPiece;
			}
		}
	}
}

// 0x59515c
bool Board::IsBoardEmpty()
{
	for (int aRow = 0; aRow < 8; aRow++)
		for (int aCol = 0; aCol < 8; aCol++)
			if (mBoard[aRow][aCol] != NULL)
				return false;
	return true;
}

// 0x595188
void Board::Pause(bool theInterface)
{
	if (mPauseCount == 0)
	{
		mPlayedSeconds += (unsigned int)(mUpdateCnt - mPlayStartTick) / 100;
		MarkDirtyFull();
	}
	if (theInterface)
	{
		if (mInterfacePauseCount == 0)
			MarkDirtyFull();
		mInterfacePauseCount++;
	}
	mPauseCount++;
}

// 0x5951db
void Board::Unpause(bool theInterface)
{
	mUnpauseDelay = 10;
	if (theInterface)
	{
		if (mInterfacePauseCount > 0)
		{
			mInterfacePauseCount--;
			if (mInterfacePauseCount < 1)
				MarkDirtyFull();
		}
	}
	if (mPauseCount > 0)
	{
		mPauseCount--;
		if (mPauseCount == 0)
			mPlayStartTick = mUpdateCnt;
	}
}

// 0x59522a: the gem under a pixel (in window coordinates)
Piece* Board::GetPieceAt(int theX, int theY)
{
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL &&
				mApp->ScaleNum(aPiece->mX, 0.0f) <= (float)theX && (float)theX < mApp->ScaleNum(aPiece->mX + 84.0f, 0.0f) &&
				mApp->ScaleNum(aPiece->mY, 0.0f) <= (float)theY && (float)theY < mApp->ScaleNum(aPiece->mY + 84.0f, 0.0f))
				return aPiece;
		}
	}
	return NULL;
}

// 0x595346: is there a line of three gems of one colour
bool Board::HasSet()
{
	for (int aRow = 0; aRow < 8; aRow++)
	{
		int aRun = 0;
		int aLast = -1;
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				aLast = -1;
			else
			{
				int aType = aPiece->mType;
				if (aType < 7 && aType == aLast)
				{
					aRun++;
					if (aRun == 3)
						return true;
				}
				else
				{
					aRun = 1;
					aLast = aType;
				}
			}
		}
	}
	for (int aCol = 0; aCol < 8; aCol++)
	{
		int aRun = 0;
		int aLast = -1;
		for (int aRow = 0; aRow < 8; aRow++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				aLast = -1;
			else
			{
				int aType = aPiece->mType;
				if (aType < 7 && aType == aLast)
				{
					aRun++;
					if (aRun == 3)
						return true;
				}
				else
				{
					aRun = 1;
					aLast = aType;
				}
			}
		}
	}
	return false;
}

// 0x59543c: the move number theSkip (counting from 0) that makes a line of three. theMove gets the square of the
// gem that stays (column, row) and the one the other gem comes from (column, row). A hypercube (type 9) may swap with
// any gem. theRows and theCols say which lines count.
bool Board::FindMove(int* theMove, int theSkip, bool theRows, bool theCols)
{
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	int aCount = 0;
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			for (int aDir = 0; aDir < 4; aDir++)
			{
				unsigned int aCol2 = aDirs[aDir][0] + aCol;
				unsigned int aRow2 = aDirs[aDir][1] + aRow;
				if (aCol2 >= 8 || aRow2 >= 8)
					continue;

				bool aFound = false;
				if (aPiece != NULL && aPiece->mType == 9 && mBoard[aRow2][aCol2] != NULL)
					aFound = true;

				Piece* aOther = mBoard[aRow2][aCol2];
				if (aOther != NULL && aOther->mType < 7)
				{
					mBoard[aRow][aCol] = aOther;
					mBoard[aRow2][aCol2] = aPiece;

					int aType = mBoard[aRow][aCol]->mType;
					int aLeft = aCol;
					while (aLeft > 0 && mBoard[aRow][aLeft - 1] != NULL && mBoard[aRow][aLeft - 1]->mType == aType)
						aLeft--;
					int aRight = aCol;
					while (aRight <= 6 && mBoard[aRow][aRight + 1] != NULL && mBoard[aRow][aRight + 1]->mType == aType)
						aRight++;
					int aTop = aRow;
					while (aTop > 0 && mBoard[aTop - 1][aCol] != NULL && mBoard[aTop - 1][aCol]->mType == aType)
						aTop--;
					int aBottom = aRow;
					while (aBottom <= 6 && mBoard[aBottom + 1][aCol] != NULL && mBoard[aBottom + 1][aCol]->mType == aType)
						aBottom++;

					Piece* aTemp = mBoard[aRow][aCol];
					mBoard[aRow][aCol] = mBoard[aRow2][aCol2];
					mBoard[aRow2][aCol2] = aTemp;

					if ((aRight - aLeft > 1 && theRows) || (aBottom - aTop > 1 && theCols))
						aFound = true;
				}

				if (aFound)
				{
					if (aCount == theSkip)
					{
						if (theMove != NULL)
						{
							theMove[0] = aCol;
							theMove[2] = aCol2;
							theMove[1] = aRow;
							theMove[3] = aRow2;
						}
						return true;
					}
					aCount++;
				}
			}
		}
	}
	return false;
}

// 0x59562e
void Board::KillTutorialButtons()
{
	if (mTutorialButton1 != NULL)
	{
		if (mParent != NULL)
			mParent->RemoveWidget(mTutorialButton1);
		mApp->SafeDeleteWidget(mTutorialButton1);
		mTutorialButton1 = NULL;
	}
	if (mTutorialButton2 != NULL)
	{
		if (mParent != NULL)
			mParent->RemoveWidget(mTutorialButton2);
		mApp->SafeDeleteWidget(mTutorialButton2);
		mTutorialButton2 = NULL;
	}
	if (mTutorialWidget != NULL)
	{
		if (mParent != NULL)
			mParent->RemoveWidget(mTutorialWidget);
		mApp->SafeDeleteWidget(mTutorialWidget);
		mTutorialWidget = NULL;
	}
}

// 0x5956c9
void Board::MouseMove(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
	if (mIsPuzzle && !mIsSecret && mState != 10 && mState != 0x12 && mState != 0xd && mScreenState == SCREEN_SHOWN &&
		mHyperspace == NULL)
	{
		mHoverPod = mPuzzleOverlay->GetPodAt(x, y);
		mPuzzleOverlay->SetPodOver(mHoverPod);
		bool aHand = mHoverPod != -1 || mPuzzleOverlay->MouseOverDome(mMouseX, mMouseY);
		mApp->SetCursor(aHand ? CURSOR_HAND : CURSOR_POINTER);
		return;
	}
	mApp->SetCursor(CURSOR_POINTER);
	mHoverPod = -1;
}

// 0x59578c
void Board::MouseLeave()
{
	mMouseX = -1;
	mMouseY = -1;
	mHoverPod = -1;
	if (mPuzzleOverlay != NULL)
		mPuzzleOverlay->SetPodOver(-1);
}

// 0x595857: pieces above a gap in a column that just exploded speed up (to catch up with the pieces below)
void Board::ExplodingFinished()
{
	if (mState == 0xe)
		return;

	for (int aCol = 0; aCol < 8; aCol++)
	{
		bool aGap = false;
		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
			{
				aGap = true;
				continue;
			}
			if (!aGap)
				continue;

			bool aNeighbor = false;
			if (aCol > 0)
				aNeighbor = mColumnExplodeTick[aCol - 1] == mUpdateCnt;
			if (aCol < 7)
				aNeighbor = aNeighbor || mColumnExplodeTick[aCol + 1] == mUpdateCnt;

			float aNewVY = aPiece->mVY;
			float aSign = 1.0f;
			if (mFlipped)
				aSign = -1.0f;
			if (mColumnExplodeTick[aCol] == mUpdateCnt)
				aNewVY = aSign * -4.5f;
			else if (aNeighbor)
			{
				aNewVY = aPiece->mVY * aSign;
				if (-4.1f < aNewVY)
					aNewVY = -4.1f;
				aNewVY = aNewVY * aSign;
			}
			if (aPiece->mVY != aNewVY)
			{
				aPiece->mVY = aNewVY;
				aPiece->mY = aPiece->mY + aNewVY;
			}
		}
	}
}

// 0x595c53: the swap in progress ends: the gems move to their new squares
void Board::CommitSwap()
{
	Piece* aPiece = mSwapPiece[0];
	// (the original tells its recorder, a GameStateEngine nothing reads back: GameStateEngine::Swap)
	mBoard[mSwapPiece[0]->mRow][mSwapPiece[0]->mCol] = NULL;
	if (mSwapPiece[1] != NULL)
		mBoard[mSwapPiece[1]->mRow][mSwapPiece[1]->mCol] = NULL;

	aPiece->mCol += aPiece->mSwapDX;
	aPiece->mRow += aPiece->mSwapDY;
	aPiece->mX = (float)GetColX(aPiece->mCol);
	aPiece->mY = (float)GetRowY(aPiece->mRow);
	mBoard[aPiece->mRow][aPiece->mCol] = aPiece;

	if (mSwapPiece[1] != NULL)
	{
		Piece* aPiece2 = mSwapPiece[1];
		aPiece2->mCol += aPiece2->mSwapDX;
		aPiece2->mRow += aPiece2->mSwapDY;
		aPiece2->mX = (float)GetColX(aPiece2->mCol);
		aPiece2->mY = (float)GetRowY(aPiece2->mRow);
		mBoard[aPiece2->mRow][aPiece2->mCol] = aPiece2;
	}
}

// 0x595da8: would the swap that is going on make a line
bool Board::WillBeValidSwap()
{
	if (mSwapPiece[0] == NULL)
		return false;

	Piece* aPiece1 = mSwapPiece[0];
	Piece* aPiece2 = mSwapPiece[1];
	mBoard[aPiece1->mRow][aPiece1->mCol] = NULL;
	if (aPiece2 != NULL)
		mBoard[aPiece2->mRow][aPiece2->mCol] = NULL;

	int aCol1 = aPiece1->mSwapDX + aPiece1->mCol;
	int aRow1 = aPiece1->mSwapDY + aPiece1->mRow;
	mBoard[aRow1][aCol1] = aPiece1;
	int aCol2 = 0, aRow2 = 0;
	if (aPiece2 != NULL)
	{
		aCol2 = aPiece2->mSwapDX + aPiece2->mCol;
		aRow2 = aPiece2->mSwapDY + aPiece2->mRow;
		mBoard[aRow2][aCol2] = aPiece2;
	}

	bool aResult = HasSet();

	mBoard[aRow1][aCol1] = NULL;
	if (aPiece2 != NULL)
		mBoard[aRow2][aCol2] = NULL;
	mBoard[aPiece1->mRow][aPiece1->mCol] = aPiece1;
	if (aPiece2 != NULL)
		mBoard[aPiece2->mRow][aPiece2->mCol] = aPiece2;
	return aResult;
}

// 0x5961a2: a move byte (direction in the top two bits, row, column) for a mirrored puzzle
unsigned char Board::TranslateMoveByte(unsigned char theMove)
{
	if (mCrazy)
	{
		unsigned char aDir = theMove >> 6;
		if (aDir == 0)
			aDir = 2;
		else if (aDir == 2)
			aDir = 0;
		theMove = (unsigned char)(((aDir << 3 | (theMove >> 3 & 7)) << 3) | (7 - (theMove & 7)));
	}
	return theMove;
}

// 0x5961ec
void Board::ClearPieceHints()
{
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL)
			{
				aPiece->mHintTime = 0;
				aPiece->mHintFrame = 0;
			}
		}
	}
}

// 0x59621c: the hint button (or the idle timer) lights a gem that can move
void Board::ShowHint()
{
	if (m15ea8)
	{
		mScore += (int)(mLevelValue * -50.0);
		if (mScore < 0)
			mScore = 0;
		m15ea8 = false;
	}
	int aMove[4];
	if (FindMove(aMove, 0, true, true))
	{
		Piece* aPiece = mBoard[aMove[3]][aMove[2]];
		aPiece->mHintTime = HINT_TIME;
		aPiece->mHintFrame = 0;
		aPiece->mB0 = 0;
		aPiece->mB4 = 0;
		aPiece->mA4 = 1;
	}
	mHintCountdown = 0;
}

// 0x5962b2: the board slides off the screen
void Board::Hide()
{
	mSlide.SetOutRange(0, 2000.0);
	mSlide.SetInVal(0);
	mSlide.SetMode(CurvedVal::MODE_CLAMP);
	mSlide.SetRamp(CurvedVal::RAMP_SLOW_TO_FAST);
	mScreenState = SCREEN_SLIDING_OUT;
}

// 0x598206: ... and back
void Board::Show()
{
	mSlide.SetOutRange(2000.0, 0);
	mSlide.SetInVal(0);
	mSlide.SetMode(CurvedVal::MODE_CLAMP);
	mSlide.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
	if (mPuzzleOverlay != NULL)
	{
		mPuzzleOverlay->Resize(0, 0, 0, 0);
		mPuzzleOverlay->mDomeWidget->Resize(0, 0, 0, 0);
	}
	ResizeButtons();
	mScreenState = SCREEN_SLIDING_IN;
}

// 0x59828a
void Board::ShowNow()
{
	mScreenState = SCREEN_SHOWN;
	mSlide.SetOutRange(0, 0);
	mSlide.SetInVal(0);
	if (mPuzzleOverlay != NULL)
	{
		mPuzzleOverlay->Resize(0, 0, mApp->mWidth, mApp->mHeight);
		mPuzzleOverlay->mDomeWidget->Resize(mApp->ScaleNum(0x67, 0), mApp->ScaleNum(0x42, 0),
			mApp->ScaleNumUp(0x61, 0), mApp->ScaleNumUp(0x17, 0));
	}
	ResizeButtons();
}

// 0x59834e
void Board::HideNow()
{
	mScreenState = SCREEN_HIDDEN;
	mSlide.SetOutRange(2000.0, 2000.0);
	ResizeButtons();
}

// 0x596302: the map of the galaxies slides in (and the board out)
void Board::ShowGalaxyMap()
{
	if (mState == 1 || mState == 0x10 || mState == 0x11 || mState == 0x12)
	{
		Hide();
		mGalaxySlide.SetOutRange(-3500.0, 0);
		mGalaxySlide.SetInVal(0);
		mGalaxySlide.SetMode(CurvedVal::MODE_CLAMP);
		mGalaxySlide.SetRamp(CurvedVal::RAMP_FAST_TO_SLOW);
		mGalaxyMap->StartSetup();
		mGalaxyMap->Resize(0, mApp->ScaleNum(-768, 0), 0, 0);
		mParent->AddWidget((Widget*)mGalaxyMap);
		mState = 0x15;
	}
}

// 0x59837e
void Board::HideGalaxyMap()
{
	if (mState == 0x15 || mState == 0x16)
	{
		Show();
		mGalaxySlide.SetOutRange(0, -1500.0);
		mGalaxySlide.SetInVal(0);
		mGalaxySlide.SetMode(CurvedVal::MODE_CLAMP);
		mGalaxySlide.SetRamp(CurvedVal::RAMP_SLOW_TO_FAST);
		mState = 0x17;
	}
}

// 0x598095: a swap in progress is settled at once (the gems snap to their squares)
void Board::StabilizeState()
{
	if (mState == 2 || mState == 3)
	{
		mSwapPiece[0]->mX = (float)GetColX(mSwapPiece[0]->mCol);
		mSwapPiece[0]->mY = (float)GetRowY(mSwapPiece[0]->mRow);
		if (mSwapPiece[1] != NULL)
		{
			mSwapPiece[1]->mX = (float)GetColX(mSwapPiece[1]->mCol);
			mSwapPiece[1]->mY = (float)GetRowY(mSwapPiece[1]->mRow);
		}
		mSwapPiece[0] = NULL;
		mSwapPiece[1] = NULL;
		mState = 1;
	}
	if (mState == 4)
	{
		CommitSwap();
		mSwapPiece[0] = NULL;
		mSwapPiece[1] = NULL;
		mState = 1;
	}
}

// 0x596466: the menu, hint and undo/pause buttons move with the board's slide
void Board::ResizeButtons()
{
	int aSlide = (int)mSlide.GetOutVal();
	if (!mIsPuzzle && !mIsAction)
	{
		mMenuButton->Resize(mApp->ScaleNum(0x58 - aSlide, 0), mApp->ScaleNum(0x1a2, 0), mApp->ScaleNumUp(0x6d, 0),
			mApp->ScaleNumUp(0x6d, 0));
		mHintButton->Resize(mApp->ScaleNum(0x3c - aSlide, 0), mApp->ScaleNum(0x217, 0), mApp->ScaleNumUp(0xaa, 0),
			mApp->ScaleNumUp(0xaa, 0));
	}
	else
	{
		mMenuButton->Resize(mApp->ScaleNum(0x60 - aSlide, 0), mApp->ScaleNum(0x155, 0), mApp->ScaleNumUp(0x6d, 0),
			mApp->ScaleNumUp(0x6d, 0));
		mHintButton->Resize(mApp->ScaleNum(0x40 - aSlide, 0), mApp->ScaleNum(0x1cb, 0), mApp->ScaleNumUp(0xaa, 0),
			mApp->ScaleNumUp(0xaa, 0));
		mThirdButton->Resize(mApp->ScaleNum(0x60 - aSlide, 0), mApp->ScaleNum(0x282, 0), mApp->ScaleNumUp(0x6d, 0),
			mApp->ScaleNumUp(0x6d, 0));
	}

	if (!m191c)
	{
		if (mTutorialButton1 != NULL)
		{
			mTutorialButton1->Resize(mApp->ScaleNum(aSlide + 0x1c6, 0), mApp->ScaleNum(0x14a, 0), mApp->ScaleNum(400, 0),
				mTutorialButton1->mComponentImage->mHeight);
		}
		if (mTutorialButton2 != NULL)
		{
			mTutorialButton2->Resize(mApp->ScaleNum(aSlide + 0x1c6, 0), mApp->ScaleNum(400, 0), mApp->ScaleNum(400, 0),
				mTutorialButton1->mComponentImage->mHeight);
		}
	}
}

// 0x596747
void Board::UpdateStarRots()
{
	double aRot = mStarRot + 0.015;
	mStarRot = aRot;
	if (6.28318 < aRot)
		mStarRot = aRot - 6.28318;
	double aPhase = mStarPhase + 0.006;
	mStarPhase = aPhase;
	if (1.0 < aPhase)
		mStarPhase = aPhase - 1.0;
	mStarDrift = mStarDrift - 0.004;
	if (0.0 <= mStarRot)
		return;
	mStarRot = mStarRot + 6.28318;
}

// 0x59814e: the new board's gems fall off the screen (a new board takes over when all of them are gone)
void Board::UpdateBoardDropping()
{
	bool aAllGone = true;
	MarkDirtyFull();
	for (int aCol = 0; aCol < 8; aCol++)
	{
		int aRand = Rand();
		bool aKick = (aRand % 15) == 0;
		for (int aRow = 7; aRow >= 0; aRow--)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL)
			{
				if (aPiece->mVY == 0.0f)
				{
					if (aKick)
					{
						aPiece->mVY = 1.0f;
						aKick = false;
					}
				}
				else
				{
					aPiece->mY = aPiece->mY + aPiece->mVY;
					aPiece->mVY = (float)((double)aPiece->mVY + 0.18);
				}
				if ((float)(mHeight + 0x20) > aPiece->mY)
					aAllGone = false;
			}
		}
	}
	if (aAllGone)
	{
		m15f88 = 200;
		NewBoard();
	}
}

// 0x597efd: eight rows of new gems, none of them in a line. They start above the screen (below it when the board is
// upside down).
void Board::NewBoard()
{
	// (the original tells its recorder, GameStateEngine::NewBoard)
	mState = 0;
	mFadeIn = 1.0f;
	mHintCountdown = 0;

	for (int aRow = 7; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			if (mBoard[aRow][aCol] != NULL)
			{
				delete mBoard[aRow][aCol];
				// the original leaves the pointer, and the HasSet calls below read the freed gems of the squares that are
				// not made yet (GAME_NOTES.md, "Oddities"); the port reads empty squares
				mBoard[aRow][aCol] = NULL;
			}
		}
	}
	for (int aRow = 7; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = new Piece();
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mX = (float)GetColX(aCol);
			int aStart = mFlipped ? 800 : 0;
			if (aRow < 7)
				aStart = (int)mBoard[aRow + 1][aCol]->mY;
			int aY;
			if (!mFlipped)
				aY = (aStart - Rand() % 0xf0) - 100;
			else
				aY = Rand() % 0xf0 + 100 + aStart;
			aPiece->mY = (float)aY;
			mBoard[aRow][aCol] = aPiece;
			do
			{
				aPiece->mType = mRand.Next() % 7;
			}
			while (HasSet());
		}
	}
}
