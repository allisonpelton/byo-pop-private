// The board's turn processing, second half: the mummies and the scorpion move, Update, mouse and buttons:
// WinMM.exe 0x4067f0, 0x407cc0, 0x4083e0, 0x410090, 0x4103d0, 0x410df0, 0x4112b0. See Board.h.

#include "Board.h"
#include "GameText.h"
#include "ImageFont.h"
#include "Screens.h"
#include "MummyApp.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

#include "GameLevels.h"
#include "Tables.inc"

// 0x410090: the mummies choose their step: a white mummy closes the horizontal distance to the explorer first, a red one
// the vertical; then it faces the way it goes (or at the explorer if it cannot move)
void Board::SetMummiesInMotion()
{
	bool aSounded = false;
	for (int i = 0; i < 2; i++)
	{
		Mummy* aMummy = mMummies[i];
		if (aMummy == NULL || !aMummy->mAlive)
			continue;
		aMummy->mIndex = i;
		aMummy->mProgress = 0.0;
		aMummy->mTurn = 0;
		int x = aMummy->mX;
		int y = aMummy->mY;
		if (!mRed)
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				aMummy->mAlive = false;
			else
			{
				int ex = mExplorer->mX;
				if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
				{
					aMummy->mTargetX = x + 1;
					aMummy->mWantDir = 1;
				}
				else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
				{
					aMummy->mTargetX = x - 1;
					aMummy->mWantDir = 3;
				}
				else
				{
					int ey = mExplorer->mY;
					if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0 && (!mGateClosed || x != mGateX || y != mGateY))
					{
						aMummy->mTargetY = y + 1;
						aMummy->mWantDir = 2;
					}
					else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0 && (!mGateClosed || x != mGateX || y - 1 != mGateY))
					{
						aMummy->mTargetY = y - 1;
						aMummy->mWantDir = 0;
					}
				}
			}
		}
		else
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				aMummy->mAlive = false;
			else
			{
				int ey = mExplorer->mY;
				if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0)
				{
					aMummy->mTargetY = y + 1;
					aMummy->mWantDir = 2;
				}
				else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0)
				{
					aMummy->mTargetY = y - 1;
					aMummy->mWantDir = 0;
				}
				else
				{
					int ex = mExplorer->mX;
					if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
					{
						aMummy->mTargetX = x + 1;
						aMummy->mWantDir = 1;
					}
					else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
					{
						aMummy->mTargetX = x - 1;
						aMummy->mWantDir = 3;
					}
				}
			}
		}
		if (aMummy->mTargetX == aMummy->mX && aMummy->mTargetY == aMummy->mY)
		{
			int ex = mExplorer->mX;
			if (aMummy->mX < ex)
				aMummy->mWantDir = 1;
			else if (ex < aMummy->mX)
				aMummy->mWantDir = 3;
			else if (aMummy->mY < mExplorer->mY)
				aMummy->mWantDir = 2;
			else
				aMummy->mWantDir = 0;
		}
		int aSpeed = mApp->mSpeed;
		int anExtra = 0;
		if (aSpeed > 1)
			anExtra = aSpeed;
		if (!aSounded)
		{
			if (aMummy->mX != aMummy->mTargetX || aMummy->mY != aMummy->mTargetY)
			{
				mApp->PlaySample(abs(CrtRand() % 2) + 1 + anExtra, GetPanAt(aMummy->mTargetX));
				aSounded = true;
			}
		}
	}
	mState = (mState == STATE_MUMMY1) ? STATE_MUMMY2 : STATE_MUMMY1;
}

// 0x4103d0: the scorpion chooses its step (the same rules as a mummy); then the scorpion's turn begins
void Board::SetScorpInMotion()
{
	if (mScorpion->mAlive)
	{
		Scorp* aScorpion = mScorpion;
		aScorpion->mProgress = 0;
		int x = aScorpion->mX;
		int y = aScorpion->mY;
		if (!mRed)
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				aScorpion->mAlive = false;
			else
			{
				int ex = mExplorer->mX;
				if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
				{
					aScorpion->mToX = x + 1;
					aScorpion->mDir = 1;
				}
				else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
				{
					aScorpion->mToX = x - 1;
					aScorpion->mDir = 3;
				}
				else
				{
					int ey = mExplorer->mY;
					if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0 && (!mGateClosed || x != mGateX || y != mGateY))
					{
						aScorpion->mToY = y + 1;
						aScorpion->mDir = 2;
					}
					else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0 && (!mGateClosed || x != mGateX || y - 1 != mGateY))
					{
						aScorpion->mToY = y - 1;
						aScorpion->mDir = 0;
					}
				}
			}
		}
		else
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				aScorpion->mAlive = false;
			else
			{
				int ey = mExplorer->mY;
				if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0)
				{
					aScorpion->mToY = y + 1;
					aScorpion->mDir = 2;
				}
				else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0)
				{
					aScorpion->mToY = y - 1;
					aScorpion->mDir = 0;
				}
				else
				{
					int ex = mExplorer->mX;
					if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
					{
						aScorpion->mToX = x + 1;
						aScorpion->mDir = 1;
					}
					else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
					{
						aScorpion->mToX = x - 1;
						aScorpion->mDir = 3;
					}
				}
			}
		}
		aScorpion = mScorpion;
		if (aScorpion->mToX == aScorpion->mX && aScorpion->mToY == aScorpion->mY)
		{
			int ex = mExplorer->mX;
			if (aScorpion->mX < ex)
			{
				aScorpion->mDir = 1;
				aScorpion->mProgress = 0;
				mState = STATE_SCORPION;
				return;
			}
			if (ex < aScorpion->mX)
			{
				aScorpion->mDir = 3;
				aScorpion->mProgress = 0;
				mState = STATE_SCORPION;
				return;
			}
			if (aScorpion->mY < mExplorer->mY)
			{
				aScorpion->mDir = 2;
				aScorpion->mProgress = 0;
				mState = STATE_SCORPION;
				return;
			}
			aScorpion->mDir = 0;
			aScorpion->mProgress = 0;
		}
	}
	mState = STATE_SCORPION;
}

// 0x407cc0: the fights, then the mummies walk their step; when they have arrived, the collisions are settled
void Board::UpdateMummies()
{
	bool aBusy = false;
	for (int i = 0; i < 2; i++)
	{
		Mummy* aMummy = mMummies[i];
		if (aMummy != NULL && aMummy->mFighting)
		{
			aMummy->mFightFrame++;
			if (aMummy->mFightFrame == 0x80)
			{
				aMummy->mFighting = false;
				if (mState == STATE_CAUGHT)
				{
					DoGameOver();
					aMummy->mFighting = true;
				}
				else
					aMummy->CheckGate();
			}
			else
				aBusy = true;
			MarkDirty();
		}
	}
	if (mState != STATE_MUMMY1 && mState != STATE_MUMMY2)
		return;
	if (aBusy)
		return;

	bool aAllArrived = true;
	for (int i = 0; i < 2; i++)
	{
		Mummy* aMummy = mMummies[i];
		if (aMummy == NULL || !aMummy->mAlive)
			continue;
		int aDir = aMummy->mDir;
		int aWant = aMummy->mWantDir;
		if (aDir == aWant)
		{
			if (aMummy->mProgress < (double) mCellSize && (aMummy->mTargetX != aMummy->mX || aMummy->mTargetY != aMummy->mY))
			{
				aAllArrived = false;
				aMummy->mProgress = (double) mCellSize / (60.0 / (double) mApp->mSpeed) + aMummy->mProgress;
				MarkDirty();
			}
		}
		else if (mUpdateCnt % (mApp->mSpeed - 3) == 0)
		{
			// turn a quarter at a time, the short way round
			if (((aWant < aDir) && (aDir - aWant < 3)) || ((aDir < aWant) && (2 < aWant - aDir)))
			{
				aMummy->mTurn--;
				if (aMummy->mTurn < -3)
				{
					aMummy->mTurn = 0;
					aMummy->mDir--;
				}
				if (aMummy->mDir < 0)
					aMummy->mDir = 3;
			}
			else
			{
				aMummy->mTurn++;
				if (aMummy->mTurn == 4)
				{
					aMummy->mTurn = 0;
					aMummy->mDir++;
				}
				if (aMummy->mDir < 0)
					aMummy->mDir = 3;
			}
			if (aMummy->mDir > 3)
				aMummy->mDir = 0;
			MarkDirty();
			aAllArrived = false;
		}
		else
			aAllArrived = false;
	}
	if (!aAllArrived)
		return;

	for (int i = 0; i < 2; i++)
	{
		Mummy* aMummy = mMummies[i];
		if (aMummy == NULL || !aMummy->mAlive)
			continue;
		bool aMoved = true;
		if (aMummy->mX == aMummy->mTargetX && aMummy->mY == aMummy->mTargetY)
			aMoved = false;
		aMummy->mX = aMummy->mTargetX;
		aMummy->mY = aMummy->mTargetY;
		aMummy->mProgress = 0.0;
		bool aKilled = false;
		for (int j = 0; j < 2; j++)
		{
			Mummy* o = mMummies[j];
			if (o != NULL && j != aMummy->mIndex && o->mAlive && aMummy->mX == o->mTargetX && aMummy->mY == o->mTargetY)
			{
				o->mAlive = false;
				aKilled = true;
			}
		}
		Scorp* s = mScorpion;
		bool aFight = false;
		if (aMummy->mX == s->mX && aMummy->mY == s->mY && s->mAlive)
		{
			s->mAlive = false;
			aFight = true;
		}
		else if (aKilled)
			aFight = true;
		else if (aMoved)
			aMummy->CheckGate();
		if (aFight)
		{
			aMummy->mFighting = true;
			aMummy->mFightFrame = 0;
			mApp->PlaySample(SOUND_PUMMEL, GetPanAt(aMummy->mTargetX));
		}
		if (aMummy->mX == mExplorer->mX && aMummy->mY == mExplorer->mY)
		{
			aMummy->mFighting = true;
			aMummy->mFightFrame = 0;
			mApp->PlaySample(SOUND_PUMMEL, GetPanAt(aMummy->mTargetX));
			mState = STATE_CAUGHT;
			mUnsolvableTick = mUpdateCnt;
			if (mApp->mGameMode != MODE_ADVENTURE)
				return;
			mApp->AdventureCaught();
			return;
		}
	}
	if (mState == STATE_MUMMY1)
	{
		SetMummiesInMotion();
		return;
	}
	if (mApp->mGameMode >= MODE_TUTORIAL)
	{
		mTutorialStep++;
		int v = kTutorialMove[mTutorialStep];
		if (v == 5)
			mTutorialStepButton->SetVisible(true);
		else if (v == 6)
		{
			mTutorialStepButton->mLabel = GT(0x00491dfc);
			mTutorialStepButton->SetVisible(true);
			mPlayNowButton->SetVisible(true);
		}
		else if (v == 7)
		{
			mTutorialStepButton->mLabel = GT(0x00491dd0);
			mTutorialStepButton->SetVisible(true);
		}
	}
	mState = STATE_READY;
	if (mSolvable)
		CheckSolvable(true);
	DoProcessQueue();
	DoResetIdles();
	if (mExplorer != NULL)
		UpdateExplorer();
}

// 0x4083e0: the scorpion's sting, then its walk, then what it met
void Board::UpdateScorp()
{
	Scorp* aScorpion = mScorpion;
	if (aScorpion == NULL)
		return;
	if (aScorpion->mStinging)
	{
		aScorpion->mStingFrame++;
		if (aScorpion->mStingFrame == 0x80)
		{
			aScorpion->mStinging = false;
			mExplorer->mDir = 2;
			mState = STATE_SCORPION_LAST;
			aScorpion->mToX = aScorpion->mFromX;
			aScorpion->mToY = aScorpion->mFromY;
		}
		MarkDirty();
		return;
	}
	if (mState != STATE_SCORPION && mState != STATE_SCORPION_LAST)
		return;
	if (aScorpion->mAlive)
	{
		if (aScorpion->mProgress == 0 && (aScorpion->mToX != aScorpion->mX || aScorpion->mToY != aScorpion->mY))
			mApp->PlaySample(abs(CrtRand() % 2) + 0xd, GetPanAt(aScorpion->mToX));
		aScorpion = mScorpion;
		if (aScorpion->mProgress < mCellSize && (aScorpion->mToX != aScorpion->mX || aScorpion->mToY != aScorpion->mY))
		{
			aScorpion->mProgress += mApp->mSpeed;
			MarkDirty();
			return;
		}
		if (aScorpion->mAlive)
		{
			bool aMoved = true;
			if (aScorpion->mX == aScorpion->mToX && aScorpion->mY == aScorpion->mToY)
				aMoved = false;
			aScorpion->mFromX = aScorpion->mX;
			aScorpion->mFromY = aScorpion->mY;
			aScorpion->mX = aScorpion->mToX;
			aScorpion->mY = aScorpion->mToY;
			aScorpion->mProgress = 0;	// 0x408567
			for (int i = 0; i < 2; i++)
			{
				Mummy* m = mMummies[i];
				if (m != NULL && aScorpion->mX == m->mX && aScorpion->mY == m->mY && m->mAlive)
				{
					aScorpion->mAlive = false;
					m->mFighting = true;
					m->mFightFrame = 0;
					mApp->PlaySample(SOUND_PUMMEL, GetPanAt(aScorpion->mToX));
					return;
				}
			}
			if (aScorpion->mX == mExplorer->mX && aScorpion->mY == mExplorer->mY)
			{
				aScorpion->mStinging = true;
				aScorpion->mStingFrame = 0;
				mState = STATE_CAUGHT;
				mUnsolvableTick = mUpdateCnt;
				mApp->PlaySample(SOUND_PUMMEL, GetPanAt(aScorpion->mToX));
				mExplorer->mDir = 2;
				if (mApp->mGameMode != MODE_ADVENTURE)
					return;
				mApp->AdventureCaught();
				return;
			}
			if (mState == STATE_SCORPION_LAST)
			{
				mStungTimer = 0;
				mApp->PlaySample(SOUND_POISON, GetPanAt(mExplorer->mX));
			}
			if (aMoved)
				aScorpion->CheckGate();
		}
	}
	if (mState != STATE_SCORPION_LAST)
	{
		SetMummiesInMotion();
		return;
	}
	mState = STATE_READY;
	if (mSolvable)
		CheckSolvable(true);
	DoProcessQueue();
	DoResetIdles();
	if (mExplorer != NULL)
		UpdateExplorer();
}

// port(mods): the fixes mod's keyTaps: a press that was let go before an update ran is kept for the next update. The original
// reads the key table at an update, so a tap shorter than the time between two updates is lost in it (and shorter than a
// frame on the web page, where a program's single key press, which lasts a millisecond, never moves the explorer)
void Board::KeyDown(KeyCode theKey)
{
	Widget::KeyDown(theKey);
	if (!mApp->mFixes.mKeyTaps)
		return;
	switch (theKey)
	{
	case KEYCODE_SPACE: mTapKeys |= TAP_WAIT; break;
	case KEYCODE_LEFT: mTapKeys |= TAP_WEST; break;
	case KEYCODE_RIGHT: mTapKeys |= TAP_EAST; break;
	case KEYCODE_UP: mTapKeys |= TAP_NORTH; break;
	case KEYCODE_DOWN: mTapKeys |= TAP_SOUTH; break;
	default: break;
	}
}

// 0x4067f0
// 0x406f00
void Board::Update()
{
	if (mApp->mWorldMap != NULL)
		return;
	mTapKeysNow = mTapKeys;
	mTapKeys = 0;
	Widget::Update();
	if (mState == STATE_READY)
	{
		int aMode = mApp->mGameMode;
		if (aMode == MODE_ADVENTURE_DONE || aMode == MODE_CLASSIC_DONE)
		{
			mEndFade = 255.0;
			mState = STATE_ENDING_FADE;
			mApp->PlaySample(SOUND_OPENTREASURE);
		}
	}
	if (mState == STATE_ENDING_FADE)
	{
		mEndFade = mEndFade - 1.25;
		MarkDirty();
		if (mState == STATE_ENDING_FADE && mEndFade <= -100.0)
		{
			mState = STATE_ENDING;
			if (abs(CrtRand() % 2) == 0)
			{
				mApp->DoMusic(0x2c, false);
				mApp->DoMusic(0x2c, false);
			}
			else
			{
				mApp->DoMusic(0x2e, false);
				mApp->DoMusic(0x2e, false);
			}
			mContinueLink->SetVisible(true);
		}
	}
	if (mApp->GetDialogCount() > 0)
		return;
	if (mState == STATE_LOST)
		return;
	if (mUpdateCnt % 10 == 0)
	{
		mTorchFrame = (mTorchFrame + 1) % 10;
		int aValue = (int) ((double) mUpdateCnt * 1.5);
		aValue = aValue % 512;
		aValue = abs(aValue);
		if (aValue < 0x101)
			mEndAlpha = aValue;
		else
			mEndAlpha = 0x200 - aValue;
		MarkDirty();
	}
	if (mUpdateCnt % 6 == 0)
	{
		mKeyFrame = (mKeyFrame + 1) % 0x24;
		MarkDirty();
	}
	if (mUpdateCnt - mSolveTick == 0x32 && mState == STATE_SOLVING)
		DoSolution();
	if (mUpdateCnt - mTimeBase == 0x4b)
	{
		int aMode = mApp->mGameMode;
		if (aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
			mApp->PlaySample(SOUND_TOMBSLIDE);
	}
	if (mUpdateCnt - mTimeBase == 0xa5)
	{
		int aMode = mApp->mGameMode;
		if (aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
			mApp->PlaySample(SOUND_MUMMYHOWL);
	}
	if (mUpdateCnt - mTimeBase < 0xa5)
		mIntroPlaying = true;
	if (mIntroPlaying && 0xa4 < mUpdateCnt - mTimeBase)
	{
		int aMode = mApp->mGameMode;
		if (aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
		{
			if (aMode >= MODE_TUTORIAL)
				mTutorialStepButton->SetVisible(true);
			mIntroPlaying = false;
		}
	}
	if (mState == STATE_LOST || mState == STATE_DONE)
		return;
	if (!mStarted)
		return;
	if (mExplorer != NULL)
	{
		mExplorer->Update();
		if (mExplorer->mDirty)
		{
			MarkDirty();
			mExplorer->mDirty = false;
		}
	}
	for (int i = 0; i < 2; i++)
	{
		if (mMummies[i] != NULL)
		{
			mMummies[i]->Update();
			if (mMummies[i]->mDirty)
			{
				MarkDirty();
				mMummies[i]->mDirty = false;
			}
		}
	}
	if (mUpdateCnt % 10 == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			mDust[0][i] = CrtRand() % (mCellSize - 0xb);
			mDust[1][i] = CrtRand() % (mCellSize - 0xb);
		}
	}
	if ((mFade >= 1.0 || mState == STATE_ENTER) && mExplorer != NULL)
		UpdateExplorer();
	UpdateMummies();
	UpdateScorp();
	UpdateArrows();
	if (mUpdateCnt % 10 == 0)
	{
		bool aSettled = false;
		if (mGateClosed)
		{
			if (0 < mGateAnim)
			{
				mGateAnim--;
				MarkDirty();
			}
			if (mGateClosed)
				aSettled = true;
		}
		if (!aSettled && mGateAnim < 7)
		{
			mGateAnim++;
			MarkDirty();
		}
	}
	if (mTrapDeathTimer > -1 && mTrapDeathTimer < 0x118)
	{
		mTrapDeathTimer++;
		MarkDirty();
		mApp->mSidePanel->MarkDirty();
	}
	if (mTrapDeathTimer == 0x23 && mExitStyle == 0)
		mApp->PlaySample(SOUND_BLOCK, GetPanAt(mExplorer->mTargetX));
	else if (mTrapDeathTimer == 3 && mExitStyle == 1)
		mApp->PlaySample(SOUND_PIT, GetPanAt(mExplorer->mTargetX));
	if (mTrapDeathTimer == 0x118 && mState != STATE_SOLVING)
		DoGameOver();
	if (mStungTimer > -1 && mStungTimer < 100)
	{
		mStungTimer++;
		MarkDirty();
	}
	if (mStungTimer == 100 && mState != STATE_SOLVING)
		DoGameOver();
	if (mState != STATE_ENTER)
	{
		int aMode = mApp->mGameMode;
		if (aMode != MODE_ADVENTURE_DONE && aMode != MODE_CLASSIC_DONE)
		{
			double aFade = (double) (mUpdateCnt - mTimeBase) * 0.01;
			if (aFade < 0.0)
				aFade = 0.0;
			else if (aFade > 1.0)
				aFade = 1.0;
			if (aFade != mFade)
			{
				mFade = aFade;
				MarkDirty();
			}
		}
		if (mState != STATE_ENTER && mFade >= 1.0)
		{
			double aFade2 = (double) (mUpdateCnt - mTimeBase) * 0.01 - 1.0;
			if (aFade2 < 0.0)
				aFade2 = 0.0;
			else if (aFade2 > 1.0)
				aFade2 = 1.0;
			if (aFade2 != mRiseFade)
			{
				mRiseFade = aFade2;
				if (aFade2 == 1.0)
				{
					for (int i = 0; i < 2; i++)
						if (mMummies[i] != NULL)
							mMummies[i]->DoIdle();
				}
				MarkDirty();
			}
		}
	}
	if (mUpdateCnt % 5 == 0)
	{
		for (int i = 0; i < 2; i++)
		{
			if (mTrapAnim[i] < 0)
			{
				if (CrtRand() % 0xaa == 0)
				{
					mTrapAnim[i] = 0;
					MarkDirty();
				}
			}
			else
			{
				mTrapAnim[i]++;
				MarkDirty();
			}
			if (0xd < mTrapAnim[i])
				mTrapAnim[i] = -1;
		}
	}
	if (!mSolvable && mApp->mShowAnkh)
	{
		int aTime = mUpdateCnt - mUnsolvableTick;
		if (299 < aTime && aTime < 0x36c)
		{
			if ((mUpdateCnt / 0x46) % 2 == 0)
			{
				mUnsolvable = true;
				return;
			}
		}
	}
	mUnsolvable = false;
}

// 0x410df0: a click: the explorer's own cell waits, a neighbour moves; during the walk it is kept for the next turn
void Board::MouseDown(int x, int y, int theClickCount)
{
	(void) theClickCount;
	if (mRiseFade < 1.0)
		return;
	int aMode = mApp->mGameMode;
	if (aMode == MODE_ADVENTURE_DONE || aMode == MODE_CLASSIC_DONE)
		return;
	for (int i = 0; i < 2; i++)
		if (mMummies[i] != NULL && mMummies[i]->mFighting)
			return;
	Player* anExplorer = mExplorer;
	int aPixelX = x - 10;
	if (mState != STATE_READY)
	{
		if (mState != STATE_EXPLORER && mState != STATE_SCORPION && mState != STATE_MUMMY1 && mState != STATE_MUMMY2)
			return;
		if (mShowingSolution)
			return;
		int aCX = GetColAt(aPixelX);
		int aCY = GetRowAt(y);
		if (aCX == anExplorer->mTargetX && aCY == anExplorer->mTargetY)
		{
			mClickX = anExplorer->mTargetX;
			mClickY = anExplorer->mTargetY;
			return;
		}
		if (aCX == anExplorer->mTargetX - 1 && aCY == anExplorer->mTargetY)
		{
			mClickX = anExplorer->mTargetX - 1;
			mClickY = anExplorer->mTargetY;
			return;
		}
		if (aCX == anExplorer->mTargetX + 1 && aCY == anExplorer->mTargetY)
		{
			mClickX = anExplorer->mTargetX + 1;
			mClickY = anExplorer->mTargetY;
			return;
		}
		if (aCX == anExplorer->mTargetX && aCY == anExplorer->mTargetY - 1)
		{
			mClickX = anExplorer->mTargetX;
			mClickY = anExplorer->mTargetY - 1;
			return;
		}
		if (aCX == anExplorer->mTargetX && aCY == anExplorer->mTargetY + 1)
		{
			mClickX = anExplorer->mTargetX;
			mClickY = anExplorer->mTargetY + 1;
		}
		return;
	}

	mState = STATE_EXPLORER;
	if (mShowingSolution)
		return;
	bool aMoved = false;
	if (GetColAt(aPixelX) == anExplorer->mX && GetRowAt(y) == anExplorer->mY)
	{
		if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 4)
		{
			DoNewTurn();
			SetScorpInMotion();
			aMoved = true;
		}
	}
	else
	{
		if (GetColAt(aPixelX) == anExplorer->mX - 1 && GetRowAt(y) == anExplorer->mY && CanMove(anExplorer->mX - 1, anExplorer->mY, true))
		{
			if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 1)
			{
				DoNewTurn();
				mExplorer->mDir = 3;
				mExplorer->mTargetX = mExplorer->mX - 1;
				aMoved = true;
			}
		}
		else if (GetColAt(aPixelX) == anExplorer->mX + 1 && GetRowAt(y) == anExplorer->mY && CanMove(anExplorer->mX + 1, anExplorer->mY, true))
		{
			if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 2)
			{
				DoNewTurn();
				mExplorer->mDir = 1;
				mExplorer->mTargetX = mExplorer->mX + 1;
				aMoved = true;
			}
		}
		else if (GetColAt(aPixelX) == anExplorer->mX && GetRowAt(y) == anExplorer->mY - 1 && CanMove(anExplorer->mX, anExplorer->mY - 1, true))
		{
			if (aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 0)
			{
				DoNewTurn();
				mExplorer->mDir = 0;
				mExplorer->mTargetY = mExplorer->mY - 1;
				aMoved = true;
			}
		}
		else if (GetColAt(aPixelX) == anExplorer->mX && GetRowAt(y) == anExplorer->mY + 1 && CanMove(anExplorer->mX, anExplorer->mY + 1, true) &&
			(aMode < MODE_TUTORIAL || kTutorialMove[mTutorialStep] == 3))
		{
			DoNewTurn();
			mExplorer->mDir = 2;
			mExplorer->mTargetY = mExplorer->mY + 1;
			aMoved = true;
		}
	}
	if (!aMoved)
		mState = STATE_READY;
	anExplorer = mExplorer;
	if (anExplorer->mTargetX == anExplorer->mX && anExplorer->mTargetY == anExplorer->mY)
		return;
	MoveSound(anExplorer->mTargetX);
}

// 0x4112a0
void Board::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(0);
}

// 0x4112b0: the tutorial's buttons and the link after a level
// 0x411300
void Board::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mTutorialStepButton->SetVisible(false);
		mTutorialStep++;
		{
			int v = kTutorialMove[mTutorialStep];
			if (v == 5)
			{
				mTutorialStepButton->SetVisible(true);
				return;
			}
			if (v != 6)
			{
				if (v != 7)
					return;
				mTutorialStepButton->mLabel = GT(0x00491dd0);
				mTutorialStepButton->mId = 2;
				mTutorialStepButton->SetVisible(true);
				return;
			}
			mTutorialStepButton->mLabel = GT(0x00491dfc);
			mTutorialStepButton->mId = 1;
			mTutorialStepButton->SetVisible(true);
			mPlayNowButton->SetVisible(true);
		}
		return;
	case 1:
		mPlayNowButton->SetVisible(false);
		mTutorialStepButton->SetVisible(false);
		mTutorialStepButton->mLabel = GT(0x00491f20);
		mTutorialStepButton->mId = 0;
		mApp->mGameMode = 7;
		mTutorialStep++;
		NewGame();
		return;
	case 2:
		mTutorialStepButton->SetVisible(false);
		mApp->mGameMode = MODE_CLASSIC;
		mApp->NewGame();
		return;
	case 3:
		mApp->OnContinueLink();
		return;
	default:
		return;
	}
}
