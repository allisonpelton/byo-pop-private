// The board's rules and turn processing: WinMM.exe 0x4064b0-0x4083e0, 0x410090-0x410df0, 0x4112a0-0x4112b0.
// See Board.h and docs/GAME_NOTES.md.

#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "WidgetManager.h"
#include "KeyCodes.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

#include "GameLevels.h"
#include "Tables.inc"

// the keys the board reads from the widget manager's key table (the original's +0x84, +0x89, +0x8a, +0x8b, +0x8c). With the
// fixes mod's keyTaps a key that was pressed and let go since the last update counts as well (mTapKeysNow, Board::KeyDown);
// without it mTapKeysNow stays 0 and the table alone decides, as in the original
#define KEY_WAIT(m)		((m)->mKeyDown[KEYCODE_SPACE] || (mTapKeysNow & TAP_WAIT) != 0)
#define KEY_WEST(m)		((m)->mKeyDown[KEYCODE_LEFT] || (mTapKeysNow & TAP_WEST) != 0)
#define KEY_EAST(m)		((m)->mKeyDown[KEYCODE_RIGHT] || (mTapKeysNow & TAP_EAST) != 0)
#define KEY_NORTH(m)	((m)->mKeyDown[KEYCODE_UP] || (mTapKeysNow & TAP_NORTH) != 0)
#define KEY_SOUTH(m)	((m)->mKeyDown[KEYCODE_DOWN] || (mTapKeysNow & TAP_SOUTH) != 0)

// 0x4064b0: the three buttons come and go with the board
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mTutorialStepButton);
	theWidgetManager->AddWidget(mPlayNowButton);
	theWidgetManager->AddWidget(mContinueLink);
}

// 0x4064f0
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mTutorialStepButton);
	theWidgetManager->RemoveWidget(mPlayNowButton);
	theWidgetManager->RemoveWidget(mContinueLink);
}

// 0x406530
void Board::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mTutorialStepButton->Resize(500, 0x1b2, 0x82, 0x28);
	mPlayNowButton->Resize(0xaa, 0x1b2, 0x82, 0x28);
	mContinueLink->Resize(mWidth / 2 - 0xe1 + mX, 0x1b1, 0x1c2, 0x1e);
}

// 0x4079c0: can something at (x1, y1) get to (x2, y2): the same cell, or a neighbour with no wall in between.
// theOccupied also refuses a cell with a living mummy or scorpion; the gate is the barrier below its cell
bool Board::CanMove(int x1, int y1, int x2, int y2, bool theOccupied, bool theGateClosed)
{
	if (x2 < 0 || x2 >= mSize || y2 < 0 || y2 >= mSize)
		return false;
	if (!theOccupied)
	{
		if (x2 == x1 && y2 == y1)
			return true;
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			Mummy* aMummy = mMummies[i];
			if (aMummy != NULL && aMummy->mAlive && aMummy->mX == x2 && aMummy->mY == y2)
				return false;
		}
		if (mScorpion->mAlive && mScorpion->mX == x2 && mScorpion->mY == y2)
			return false;
		if (x2 == x1 && y2 == y1)
			return true;
	}
	if (y2 == y1)
	{
		if (x2 == x1 - 1 && (mCells[x1][y1] & WALL_WEST) == 0)
			return true;
		if (x2 == x1 + 1 && (mCells[x1][y1] & WALL_EAST) == 0)
			return true;
		return false;
	}
	if (x2 == x1)
	{
		if (y2 == y1 - 1 && (mCells[x1][y1] & WALL_NORTH) == 0 && (!theGateClosed || x1 != mGateX || y2 != mGateY))
			return true;
		if (y2 == y1 + 1 && (mCells[x1][y1] & WALL_SOUTH) == 0 && (!theGateClosed || x1 != mGateX || y1 != mGateY))
			return true;
	}
	return false;
}

// 0x407b50: can the explorer go to (x, y) from where he is going to be
bool Board::CanMove(int x, int y, bool theOccupied)
{
	if (x < 0 || x >= mSize || y < 0 || y >= mSize)
		return false;
	if (theOccupied)
	{
		for (int i = 0; i < 2; i++)
		{
			Mummy* aMummy = mMummies[i];
			if (aMummy != NULL && aMummy->mAlive && aMummy->mX == x && aMummy->mY == y)
				return false;
		}
		if (mScorpion->mAlive && mScorpion->mX == x && mScorpion->mY == y)
			return false;
		if (x == mExplorer->mX && y == mExplorer->mY)
			return true;
	}
	int aX = mExplorer->mX;
	int aY = mExplorer->mY;
	if (x == aX && y == aY)
		return true;
	if (y == aY)
	{
		if (x == aX - 1 && (mCells[aX][aY] & WALL_WEST) == 0)
			return true;
		if (x == aX + 1 && (mCells[aX][aY] & WALL_EAST) == 0)
			return true;
	}
	else if (x == aX)
	{
		if (y == aY - 1 && (mCells[aX][aY] & WALL_NORTH) == 0 && (!mGateClosed || aX != mGateX || aY - 1 != mGateY))
			return true;
		if (y == aY + 1 && (mCells[aX][aY] & WALL_SOUTH) == 0 && (!mGateClosed || aX != mGateX || aY != mGateY))
			return true;
	}
	return false;
}

// 0x407960: the idle animations start over
void Board::DoResetIdles()
{
	if (mExplorer != NULL && mStarted)
	{
		mExplorer->mIdleTimer = 0;
		mExplorer->mIdle = false;
		mExplorer->mIdleFrame = 0;
	}
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] != NULL && mStarted)
			mMummies[i]->ResetIdle();
	}
}

// 0x421d00: a mummy has arrived on the key: the gate flips (once per other mummy that is not on the key with it)
void Mummy::CheckGate()
{
	Board* aBoard = mApp->mBoard;
	if (mX == aBoard->mKeyX && mY == aBoard->mKeyY && mAlive)
	{
		for (int i = 0; i < 2; i++)
		{
			Mummy* anOther = aBoard->mMummies[i];
			if (anOther != NULL && i != mIndex && (!anOther->mAlive || anOther->mX != mX || anOther->mY != mY))
				aBoard->FlipGate();
		}
	}
}

// 0x429170: a scorpion or the explorer has arrived on the key
void Scorp::CheckGate()
{
	mApp->mBoard->KeyStep(mX, mY);
}

void Board::KeyStep(int theX, int theY)
{
	if (theX == mKeyX && theY == mKeyY)
		FlipGate();
}

// port: the sound of a step, which the original plays in place (0x4065b0, 0x406f10): the footsteps of the speed
// setting, panned by the column
void Board::MoveSound(int theCell)
{
	int aSpeed = mApp->mSpeed;
	int anExtra = 0;
	if (aSpeed > 1)
		anExtra = aSpeed;
	mApp->PlaySample(abs(CrtRand() % 2) + 7 + anExtra, GetPanAt(theCell));
}

// 0x410b10
int Board::GetPanAt(int theCell)
{
	int aX = GetXAt(theCell);
	int aSum = mCellSize - GetXAt(0) + GetXAt(mSize);
	int aHalf = aSum / 2;
	return (int) ((double) (aHalf - aX) / (double) (aSum * 2) * 800.0);
}

// 0x410a80, 0x410ab0
void Board::UpdateArrows()
{
	for (int x = 0; x < NUM_HOVER; x++)
		for (int y = 0; y < NUM_HOVER; y++)
			if (mHover[x][y] > 0)
				mHover[x][y]++;
}

void Board::ClearArrows()
{
	memset(mHover, 0, sizeof(mHover));
}

// 0x40e150 (the original also stores the point, at +0x27c and +0x280, and never reads it)
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	int aCellX = GetColAt(x - 10);
	int aCellY = GetRowAt(y);
	if (aCellX < 0 || aCellX >= NUM_HOVER || aCellY < 0 || aCellY >= NUM_HOVER)
		return;
	if (mHover[aCellX][aCellY] == 0)
	{
		ClearArrows();
		mHover[aCellX][aCellY] = 1;
		MarkDirty();
	}
}

// 0x4065b0: a click made while the explorer was moving is carried out now
void Board::DoProcessQueue()
{
	Player* anExplorer = mExplorer;
	if ((mCells[anExplorer->mX][anExplorer->mY] & EXIT_ANY) == 0)
	{
		if (!mShowingSolution && mApp->mGameMode < MODE_TUTORIAL)
		{
			mState = STATE_EXPLORER;
			if (mClickX == anExplorer->mX && mClickY == anExplorer->mY)
			{
				DoNewTurn();
				SetScorpInMotion();
				mMoveTick = mUpdateCnt;
			}
			else
			{
				int aX = anExplorer->mX - 1;
				if (mClickX == aX && mClickY == anExplorer->mY && CanMove(aX, anExplorer->mY, true))
				{
					DoNewTurn();
					mExplorer->mDir = 3;
					mExplorer->mTargetX = mExplorer->mX - 1;
					mMoveTick = mUpdateCnt;
				}
				else
				{
					aX = mExplorer->mX + 1;
					if (mClickX == aX && mClickY == mExplorer->mY && CanMove(aX, mExplorer->mY, true))
					{
						DoNewTurn();
						mExplorer->mDir = 1;
						mExplorer->mTargetX = mExplorer->mX + 1;
						mMoveTick = mUpdateCnt;
					}
					else
					{
						int aY = mExplorer->mY - 1;
						if (mClickX == mExplorer->mX && mClickY == aY && CanMove(mExplorer->mX, aY, true))
						{
							DoNewTurn();
							mExplorer->mDir = 0;
							mExplorer->mTargetY = mExplorer->mY - 1;
							mMoveTick = mUpdateCnt;
						}
						else
						{
							aY = mExplorer->mY + 1;
							if (mClickX == mExplorer->mX && mClickY == aY && CanMove(mExplorer->mX, aY, true))
							{
								DoNewTurn();
								mExplorer->mDir = 2;
								mExplorer->mTargetY = mExplorer->mY + 1;
								mMoveTick = mUpdateCnt;
							}
							else
								mState = STATE_READY;
						}
					}
				}
			}
			anExplorer = mExplorer;
			if (anExplorer->mTargetX != anExplorer->mX || anExplorer->mTargetY != anExplorer->mY)
				MoveSound(anExplorer->mTargetX);
		}
		mClickX = 99;
		mClickY = 99;
	}
}

// 0x406f10: the explorer: take a move (keys, a queued click, the solution), walk it, and arrive
void Board::UpdateExplorer()
{
	bool aFighting = false;
	for (int i = 0; i < 2; i++)
		if (mMummies[i] != NULL && mMummies[i]->mFighting)
			aFighting = true;
	int aMode = mApp->mGameMode;
	Player* anExplorer = mExplorer;
	WidgetManager* aMgr = mWidgetManager;

	if (mState == STATE_READY && mStungTimer < 0)
	{
		if ((mCells[anExplorer->mX][anExplorer->mY] & EXIT_ANY) == 0)
		{
			if (!aFighting && mRiseFade >= 1.0)
			{
				if (aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
					mState = STATE_EXPLORER;
				if (!mShowingSolution && aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
				{
					bool aMoved = false;
					bool aChosen = true;
					if (!KEY_WAIT(aMgr))
					{
						bool aDone = false;
						if (KEY_WEST(aMgr))
						{
							if (CanMove(anExplorer->mX - 1, anExplorer->mY, true))
							{
								if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 1)
								{
									DoNewTurn();
									mExplorer->mDir = 3;
									mExplorer->mTargetX = mExplorer->mX - 1;
									mMoveTick = mUpdateCnt;
									aMoved = true;
								}
								else
									aChosen = false;
								aDone = true;
							}
						}
						if (!aDone)
						{
							if (KEY_EAST(aMgr))
							{
								if (CanMove(mExplorer->mX + 1, mExplorer->mY, true))
								{
									if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 2)
									{
										DoNewTurn();
										mExplorer->mDir = 1;
										mExplorer->mTargetX = mExplorer->mX + 1;
										mMoveTick = mUpdateCnt;
										aMoved = true;
									}
									else
										aChosen = false;
									aDone = true;
								}
							}
						}
						if (!aDone)
						{
							if (KEY_NORTH(aMgr))
							{
								if (CanMove(mExplorer->mX, mExplorer->mY - 1, true))
								{
									if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 0)
									{
										DoNewTurn();
										mExplorer->mDir = 0;
										mExplorer->mTargetY = mExplorer->mY - 1;
										mMoveTick = mUpdateCnt;
										aMoved = true;
									}
									else
										aChosen = false;
									aDone = true;
								}
							}
						}
						if (!aDone)
						{
							if (KEY_SOUTH(aMgr))
							{
								if (CanMove(mExplorer->mX, mExplorer->mY + 1, true) &&
									(aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 3))
								{
									DoNewTurn();
									mExplorer->mDir = 2;
									mExplorer->mTargetY = mExplorer->mY + 1;
									mMoveTick = mUpdateCnt;
									aMoved = true;
								}
							}
						}
						if (!aMoved)
							mState = STATE_READY;
						(void) aChosen;
					}
					else if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 4)
					{
						DoNewTurn();
						SetScorpInMotion();
						mMoveTick = mUpdateCnt;
						aMoved = true;
					}
					else
						mState = STATE_READY;

					if (aMoved || mState != STATE_READY)
					{
						anExplorer = mExplorer;
						if (anExplorer->mTargetX != anExplorer->mX || anExplorer->mTargetY != anExplorer->mY)
							MoveSound(anExplorer->mTargetX);
					}
				}
				else if (!mSolution.empty())
				{
					// the solution plays: the next position of the explorer
					const ChaseState& aState = mSolution.front();
					anExplorer = mExplorer;
					if (aState.mExplorerX < anExplorer->mX)
						anExplorer->mDir = 3;
					else if (anExplorer->mX < aState.mExplorerX)
						anExplorer->mDir = 1;
					else if (aState.mExplorerY < anExplorer->mY)
						anExplorer->mDir = 0;
					else if (anExplorer->mY < aState.mExplorerY)
						anExplorer->mDir = 2;
					else
						SetScorpInMotion();
					mExplorer->mTargetX = aState.mExplorerX;
					mExplorer->mTargetY = aState.mExplorerY;
					anExplorer = mExplorer;
					if (anExplorer->mTargetX != anExplorer->mX || anExplorer->mTargetY != anExplorer->mY)
						MoveSound(anExplorer->mTargetX);
					mSolution.erase(mSolution.begin());
				}
			}
		}
		else
		{
			// the explorer is on the exit: he walks out
			mState = STATE_EXPLORER;
			if (!mShowingSolution && aMode < MODE_TUTORIAL)
				mApp->PlaySample(SOUND_FINISHEDLEVEL);
			int aFlags = mCells[mExplorer->mX][mExplorer->mY];
			if (aFlags & EXIT_NORTH)
			{
				mExplorer->mDir = 0;
				mExplorer->mTargetY = mExplorer->mTargetY - 1;
				if (mTutorialStep < 0x22 || mTutorialStep > 0x24)
					mTutorialStepButton->SetVisible(false);
			}
			else if (aFlags & EXIT_SOUTH)
			{
				mExplorer->mDir = 2;
				mExplorer->mTargetY = mExplorer->mTargetY + 1;
				mTutorialStepButton->SetVisible(false);
			}
			else if (aFlags & EXIT_WEST)
			{
				mExplorer->mDir = 3;
				mExplorer->mTargetX = mExplorer->mTargetX - 1;
				mTutorialStepButton->SetVisible(false);
			}
			else if (aFlags & EXIT_EAST)
			{
				mExplorer->mDir = 1;
				mExplorer->mTargetX = mExplorer->mTargetX + 1;
				mTutorialStepButton->SetVisible(false);
			}
		}
	}
	else if (mState == STATE_EXPLORER || mState == STATE_SCORPION || mState == STATE_MUMMY1 || mState == STATE_MUMMY2)
	{
		// a key pressed while the pieces move is kept for the next turn
		if (!mShowingSolution && aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
		{
			if (KEY_WAIT(aMgr))
			{
				if (mUpdateCnt - mMoveTick < 2)
					mMoveTick = mUpdateCnt;
				else
				{
					mClickX = mExplorer->mTargetX;
					mClickY = mExplorer->mTargetY;
				}
			}
			else if (KEY_WEST(aMgr))
			{
				if (mUpdateCnt - mMoveTick < 2 && mExplorer->mDir == 3)
					mMoveTick = mUpdateCnt;
				else
				{
					mClickX = mExplorer->mTargetX - 1;
					mClickY = mExplorer->mTargetY;
				}
			}
			else if (KEY_EAST(aMgr))
			{
				if (mUpdateCnt - mMoveTick < 2 && mExplorer->mDir == 1)
					mMoveTick = mUpdateCnt;
				else
				{
					mClickX = mExplorer->mTargetX + 1;
					mClickY = mExplorer->mTargetY;
				}
			}
			else if (KEY_NORTH(aMgr))
			{
				if (mUpdateCnt - mMoveTick < 2 && mExplorer->mDir == 0)
					mMoveTick = mUpdateCnt;
				else
				{
					mClickX = mExplorer->mTargetX;
					mClickY = mExplorer->mTargetY - 1;
				}
			}
			else if (KEY_SOUTH(aMgr))
			{
				if (mUpdateCnt - mMoveTick < 2 && mExplorer->mDir == 2)
					mMoveTick = mUpdateCnt;
				else
				{
					mClickX = mExplorer->mTargetX;
					mClickY = mExplorer->mTargetY + 1;
				}
			}
		}
		MarkDirty();
	}

	if (mState == STATE_EXPLORER)
	{
		// the walk: the progress counts up to a cell (a quicker walk through the ending board)
		if (aMode == MODE_ADVENTURE_DONE || aMode == MODE_CLASSIC_DONE)
			mExplorer->mProgress = mExplorer->mProgress + 1.0;
		else
			mExplorer->mProgress = (double) mCellSize / (60.0 / (double) mApp->mSpeed) + mExplorer->mProgress;
		anExplorer = mExplorer;
		if ((double) mCellSize <= anExplorer->mProgress)
		{
			anExplorer->mX = anExplorer->mTargetX;
			mExplorer->mY = mExplorer->mTargetY;
			anExplorer = mExplorer;
			anExplorer->mProgress = 0.0;
			KeyStep(mExplorer->mX, mExplorer->mY);
			anExplorer = mExplorer;
			if ((anExplorer->mX == mTrapX[0] && anExplorer->mY == mTrapY[0]) || (anExplorer->mX == mTrapX[1] && anExplorer->mY == mTrapY[1]))
			{
				// a skull trap
				mState = STATE_LEAVE;
				anExplorer->mDir = 2;
				mExitStyle = abs(CrtRand() % 2);
				if (mExitStyle == 0)
					mTrapDeathTimer = 0x1e;
				else
					mTrapDeathTimer = 0;
				if (aMode == MODE_ADVENTURE)
				{
					mApp->AdventureCaught();
					MarkDirty();
					goto L_finish;
				}
			}
			else
				SetScorpInMotion();
		}
		MarkDirty();
		goto L_finish;
	}
	if (mState != STATE_ENTER)
		goto L_finish;

	// STATE_ENTER: the explorer walks in from outside
	{
		int aExtra = mApp->mSpeed > 1 ? mApp->mSpeed : 0;
		anExplorer = mExplorer;
		if (anExplorer->mProgress == 0.0 && (anExplorer->mTargetX != anExplorer->mX || anExplorer->mTargetY != anExplorer->mY))
		{
			mApp->PlaySample(abs(CrtRand() % 2) + 7 + aExtra, GetPanAt(anExplorer->mTargetX));
		}
		mExplorer->mProgress = (double) mCellSize / (60.0 / (double) mApp->mSpeed) + mExplorer->mProgress;
		anExplorer = mExplorer;
		if (anExplorer->mX < anExplorer->mTargetX)
			anExplorer->mDir = 1;
		else if (anExplorer->mTargetX < anExplorer->mX)
			anExplorer->mDir = 3;
		anExplorer = mExplorer;
		if (anExplorer->mY < anExplorer->mTargetY)
			anExplorer->mDir = 2;
		else if (anExplorer->mTargetY < anExplorer->mY)
			anExplorer->mDir = 0;
		anExplorer = mExplorer;
		if ((double) mCellSize <= anExplorer->mProgress)
		{
			anExplorer->mProgress = 0.0;
			anExplorer = mExplorer;
			if (anExplorer->mX < anExplorer->mTargetX)
				anExplorer->mX = anExplorer->mX + 1;
			else if (anExplorer->mTargetX < anExplorer->mX)
				anExplorer->mX = anExplorer->mX - 1;
			anExplorer = mExplorer;
			if (anExplorer->mY < anExplorer->mTargetY)
				anExplorer->mY = anExplorer->mY + 1;
			else if (anExplorer->mTargetY < anExplorer->mY)
				anExplorer->mY = anExplorer->mY - 1;
		}
		anExplorer = mExplorer;
		if (anExplorer->mX == anExplorer->mTargetX && anExplorer->mY == anExplorer->mTargetY)
		{
			mState = STATE_READY;
			mTimeBase = mUpdateCnt;
			mClickX = 99;
			mClickY = 99;
		}
		MarkDirty();
	}

L_finish:
	anExplorer = mExplorer;
	if (((anExplorer->mX < 0 || mSize <= anExplorer->mX || anExplorer->mY < 0) || (mSize <= anExplorer->mY && mApp->mLevelComplete == NULL)) &&
		(mState != STATE_ENTER && mState != STATE_SCORPION_LAST && mStungTimer < 0))
	{
		// the explorer has left the maze
		mState = STATE_READY;
		if (mShowingSolution)
		{
			DoGameOver();
			return;
		}
		if (aMode < MODE_TUTORIAL)
		{
			mApp->DoNextLevel();
			return;
		}
		if (aMode != MODE_TUTORIAL + 2 && aMode != MODE_TUTORIAL + 4)
		{
			mApp->mGameMode = aMode + 1;
			NewGame();
		}
	}
}
