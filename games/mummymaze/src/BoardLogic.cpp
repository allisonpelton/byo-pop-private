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

#include "Tables.inc"

// the keys the board reads from the widget manager's key table (the original's +0x84, +0x89, +0x8a, +0x8b, +0x8c)
#define KEY_WAIT(m)		((m)->mKeyDown[KEYCODE_SPACE])
#define KEY_WEST(m)		((m)->mKeyDown[KEYCODE_LEFT])
#define KEY_EAST(m)		((m)->mKeyDown[KEYCODE_RIGHT])
#define KEY_NORTH(m)	((m)->mKeyDown[KEYCODE_UP])
#define KEY_SOUTH(m)	((m)->mKeyDown[KEYCODE_DOWN])

// 0x4064b0, 0x4064f0, 0x406530: the three buttons come and go with the board
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayNowButton0);
	theWidgetManager->AddWidget(mPlayNowButton);
	theWidgetManager->AddWidget(mContinueLink);
}

void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayNowButton0);
	theWidgetManager->RemoveWidget(mPlayNowButton);
	theWidgetManager->RemoveWidget(mContinueLink);
}

void Board::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mPlayNowButton0->Resize(500, 0x1b2, 0x82, 0x28);
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
bool Board::CanExplorerMove(int x, int y, bool theOccupied)
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
void Board::ResetIdle()
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
void Mummy::OnArrived()
{
	Board* aBoard = mApp->mBoard;
	if (mX == aBoard->mKeyX && mY == aBoard->mKeyY && mAlive)
	{
		for (int i = 0; i < 2; i++)
		{
			Mummy* anOther = aBoard->mMummies[i];
			if (anOther != NULL && i != mIndex && (!anOther->mAlive || anOther->mX != mX || anOther->mY != mY))
				aBoard->ToggleGate();
		}
	}
}

// 0x429170: a scorpion or the explorer has arrived on the key
void Scorpion::OnArrived()
{
	mApp->mBoard->KeyStep(mX, mY);
}

void Board::KeyStep(int theX, int theY)
{
	if (theX == mKeyX && theY == mKeyY)
		ToggleGate();
}

// the sound of a step: the footsteps of the speed setting, panned by the column
void Board::MoveSound(int theCell)
{
	int aSpeed = mApp->mSpeed;
	int anExtra = 0;
	if (aSpeed > 1)
		anExtra = aSpeed;
	mApp->PlaySample(abs(CrtRand() % 2) + 7 + anExtra, Pan(theCell));
}

// 0x410b10
int Board::Pan(int theCell)
{
	int aX = PixelX(theCell);
	int aSum = mCellSize - PixelX(0) + PixelX(mSize);
	int aHalf = aSum / 2;
	return (int) ((double) (aHalf - aX) / (double) (aSum * 2) * 800.0);
}

// 0x410a80, 0x410ab0
void Board::AgeHover()
{
	for (int x = 0; x < NUM_HOVER; x++)
		for (int y = 0; y < NUM_HOVER; y++)
			if (mHover[x][y] > 0)
				mHover[x][y]++;
}

void Board::ClearHover()
{
	memset(mHover, 0, sizeof(mHover));
}

// 0x40e150
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mMouseX = x;
	mMouseY = y;
	int aCellX = CellX(x - 10);
	int aCellY = CellY(y);
	if (aCellX < 0 || aCellX >= NUM_HOVER || aCellY < 0 || aCellY >= NUM_HOVER)
		return;
	if (mHover[aCellX][aCellY] == 0)
	{
		ClearHover();
		mHover[aCellX][aCellY] = 1;
		MarkDirty();
	}
}

// 0x4065b0: a click made while the explorer was moving is carried out now
void Board::ProcessClick()
{
	Explorer* e = mExplorer;
	if ((mCells[e->mX][e->mY] & EXIT_ANY) == 0)
	{
		if (!mShowingSolution && mApp->mGameMode < 4)
		{
			mState = STATE_EXPLORER;
			if (mClickX == e->mX && mClickY == e->mY)
			{
				PushUndo();
				PlanScorpion();
				mMoveTick = mUpdateCnt;
			}
			else
			{
				int aX = e->mX - 1;
				if (mClickX == aX && mClickY == e->mY && CanExplorerMove(aX, e->mY, true))
				{
					PushUndo();
					mExplorer->mDir = 3;
					mExplorer->mTargetX = mExplorer->mX - 1;
					mMoveTick = mUpdateCnt;
				}
				else
				{
					aX = mExplorer->mX + 1;
					if (mClickX == aX && mClickY == mExplorer->mY && CanExplorerMove(aX, mExplorer->mY, true))
					{
						PushUndo();
						mExplorer->mDir = 1;
						mExplorer->mTargetX = mExplorer->mX + 1;
						mMoveTick = mUpdateCnt;
					}
					else
					{
						int aY = mExplorer->mY - 1;
						if (mClickX == mExplorer->mX && mClickY == aY && CanExplorerMove(mExplorer->mX, aY, true))
						{
							PushUndo();
							mExplorer->mDir = 0;
							mExplorer->mTargetY = mExplorer->mY - 1;
							mMoveTick = mUpdateCnt;
						}
						else
						{
							aY = mExplorer->mY + 1;
							if (mClickX == mExplorer->mX && mClickY == aY && CanExplorerMove(mExplorer->mX, aY, true))
							{
								PushUndo();
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
			e = mExplorer;
			if (e->mTargetX != e->mX || e->mTargetY != e->mY)
				MoveSound(e->mTargetX);
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
	Explorer* e = mExplorer;
	WidgetManager* aMgr = mWidgetManager;

	if (mState == STATE_READY && mDeathTimer2 < 0)
	{
		if ((mCells[e->mX][e->mY] & EXIT_ANY) == 0)
		{
			if (!aFighting && mFade2 >= 1.0)
			{
				if (aMode != 2 && aMode != 3)
					mState = STATE_EXPLORER;
				if (!mShowingSolution && aMode != 2 && aMode != 3)
				{
					bool aMoved = false;
					bool aChosen = true;
					if (!KEY_WAIT(aMgr))
					{
						bool aDone = false;
						if (KEY_WEST(aMgr))
						{
							if (CanExplorerMove(e->mX - 1, e->mY, true))
							{
								if (aMode < 4 || kTutorialMove[mTutorialStep] == 1)
								{
									PushUndo();
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
								if (CanExplorerMove(mExplorer->mX + 1, mExplorer->mY, true))
								{
									if (aMode < 4 || kTutorialMove[mTutorialStep] == 2)
									{
										PushUndo();
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
								if (CanExplorerMove(mExplorer->mX, mExplorer->mY - 1, true))
								{
									if (aMode < 4 || kTutorialMove[mTutorialStep] == 0)
									{
										PushUndo();
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
								if (CanExplorerMove(mExplorer->mX, mExplorer->mY + 1, true) &&
									(aMode < 4 || kTutorialMove[mTutorialStep] == 3))
								{
									PushUndo();
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
					else if (aMode < 4 || kTutorialMove[mTutorialStep] == 4)
					{
						PushUndo();
						PlanScorpion();
						mMoveTick = mUpdateCnt;
						aMoved = true;
					}
					else
						mState = STATE_READY;

					if (aMoved || mState != STATE_READY)
					{
						e = mExplorer;
						if (e->mTargetX != e->mX || e->mTargetY != e->mY)
							MoveSound(e->mTargetX);
					}
				}
				else if (!mSolution.empty())
				{
					// the solution plays: the next position of the explorer
					const ChaseState& s = mSolution.front();
					e = mExplorer;
					if (s.mExplorerX < e->mX)
						e->mDir = 3;
					else if (e->mX < s.mExplorerX)
						e->mDir = 1;
					else if (s.mExplorerY < e->mY)
						e->mDir = 0;
					else if (e->mY < s.mExplorerY)
						e->mDir = 2;
					else
						PlanScorpion();
					mExplorer->mTargetX = s.mExplorerX;
					mExplorer->mTargetY = s.mExplorerY;
					e = mExplorer;
					if (e->mTargetX != e->mX || e->mTargetY != e->mY)
						MoveSound(e->mTargetX);
					mSolution.erase(mSolution.begin());
				}
			}
		}
		else
		{
			// the explorer is on the exit: he walks out
			mState = STATE_EXPLORER;
			if (!mShowingSolution && aMode < 4)
				mApp->PlaySample(SOUND_FINISHEDLEVEL);
			int aFlags = mCells[mExplorer->mX][mExplorer->mY];
			if (aFlags & EXIT_NORTH)
			{
				mExplorer->mDir = 0;
				mExplorer->mTargetY = mExplorer->mTargetY - 1;
				if (mTutorialStep < 0x22 || mTutorialStep > 0x24)
					mPlayNowButton0->SetVisible(false);
			}
			else if (aFlags & EXIT_SOUTH)
			{
				mExplorer->mDir = 2;
				mExplorer->mTargetY = mExplorer->mTargetY + 1;
				mPlayNowButton0->SetVisible(false);
			}
			else if (aFlags & EXIT_WEST)
			{
				mExplorer->mDir = 3;
				mExplorer->mTargetX = mExplorer->mTargetX - 1;
				mPlayNowButton0->SetVisible(false);
			}
			else if (aFlags & EXIT_EAST)
			{
				mExplorer->mDir = 1;
				mExplorer->mTargetX = mExplorer->mTargetX + 1;
				mPlayNowButton0->SetVisible(false);
			}
		}
	}
	else if (mState == STATE_EXPLORER || mState == STATE_SCORPION || mState == STATE_MUMMY1 || mState == STATE_MUMMY2)
	{
		// a key pressed while the pieces move is kept for the next turn
		if (!mShowingSolution && aMode != 2 && aMode != 3)
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
		if (aMode == 2 || aMode == 3)
			mExplorer->mProgress = mExplorer->mProgress + 1.0;
		else
			mExplorer->mProgress = (double) mCellSize / (60.0 / (double) mApp->mSpeed) + mExplorer->mProgress;
		e = mExplorer;
		if ((double) mCellSize <= e->mProgress)
		{
			e->mX = e->mTargetX;
			mExplorer->mY = mExplorer->mTargetY;
			e = mExplorer;
			e->mProgress = 0.0;
			e->mIdleTimer = 0;
			KeyStep(mExplorer->mX, mExplorer->mY);
			e = mExplorer;
			if ((e->mX == mTrapX[0] && e->mY == mTrapY[0]) || (e->mX == mTrapX[1] && e->mY == mTrapY[1]))
			{
				// a skull trap
				mState = STATE_LEAVE;
				e->mDir = 2;
				mExitStyle = abs(CrtRand() % 2);
				if (mExitStyle == 0)
					mDeathTimer = 0x1e;
				else
					mDeathTimer = 0;
				if (aMode == MODE_ADVENTURE)
				{
					mApp->OnAdventureDeath();
					MarkDirty();
					goto L_finish;
				}
			}
			else
				PlanScorpion();
		}
		MarkDirty();
		goto L_finish;
	}
	if (mState != STATE_ENTER)
		goto L_finish;

	// STATE_ENTER: the explorer walks in from outside
	{
		int aExtra = mApp->mSpeed > 1 ? mApp->mSpeed : 0;
		e = mExplorer;
		if (e->mProgress == 0.0 && (e->mTargetX != e->mX || e->mTargetY != e->mY))
		{
			mApp->PlaySample(abs(CrtRand() % 2) + 7 + aExtra, Pan(e->mTargetX));
		}
		mExplorer->mProgress = (double) mCellSize / (60.0 / (double) mApp->mSpeed) + mExplorer->mProgress;
		e = mExplorer;
		if (e->mX < e->mTargetX)
			e->mDir = 1;
		else if (e->mTargetX < e->mX)
			e->mDir = 3;
		e = mExplorer;
		if (e->mY < e->mTargetY)
			e->mDir = 2;
		else if (e->mTargetY < e->mY)
			e->mDir = 0;
		e = mExplorer;
		if ((double) mCellSize <= e->mProgress)
		{
			e->mProgress = 0.0;
			e->mIdleTimer = 0;
			e = mExplorer;
			if (e->mX < e->mTargetX)
				e->mX = e->mX + 1;
			else if (e->mTargetX < e->mX)
				e->mX = e->mX - 1;
			e = mExplorer;
			if (e->mY < e->mTargetY)
				e->mY = e->mY + 1;
			else if (e->mTargetY < e->mY)
				e->mY = e->mY - 1;
		}
		e = mExplorer;
		if (e->mX == e->mTargetX && e->mY == e->mTargetY)
		{
			mState = STATE_READY;
			mTimeBase = mUpdateCnt;
			mClickX = 99;
			mClickY = 99;
		}
		MarkDirty();
	}

L_finish:
	e = mExplorer;
	if (((e->mX < 0 || mSize <= e->mX || e->mY < 0) || (mSize <= e->mY && mApp->mLevelComplete == NULL)) &&
		(mState != STATE_ENTER && mState != STATE_SCORPION_LAST && mDeathTimer2 < 0))
	{
		// the explorer has left the maze
		mState = STATE_READY;
		if (mShowingSolution)
		{
			Lose();
			return;
		}
		if (aMode < 4)
		{
			mApp->OnLevelComplete();
			return;
		}
		if (aMode != 6 && aMode != 8)
		{
			mApp->mGameMode = aMode + 1;
			StartLevel();
		}
	}
}
