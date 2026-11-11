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
		Mummy* m = mMummies[i];
		if (m == NULL || !m->mAlive)
			continue;
		m->mIndex = i;
		m->mProgress = 0.0;
		m->mTurn = 0;
		int x = m->mX;
		int y = m->mY;
		if (!mRed)
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				m->mAlive = false;
			else
			{
				int ex = mExplorer->mX;
				if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
				{
					m->mTargetX = x + 1;
					m->mWantDir = 1;
				}
				else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
				{
					m->mTargetX = x - 1;
					m->mWantDir = 3;
				}
				else
				{
					int ey = mExplorer->mY;
					if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0 && (!mGateClosed || x != mGateX || y != mGateY))
					{
						m->mTargetY = y + 1;
						m->mWantDir = 2;
					}
					else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0 && (!mGateClosed || x != mGateX || y - 1 != mGateY))
					{
						m->mTargetY = y - 1;
						m->mWantDir = 0;
					}
				}
			}
		}
		else
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				m->mAlive = false;
			else
			{
				int ey = mExplorer->mY;
				if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0)
				{
					m->mTargetY = y + 1;
					m->mWantDir = 2;
				}
				else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0)
				{
					m->mTargetY = y - 1;
					m->mWantDir = 0;
				}
				else
				{
					int ex = mExplorer->mX;
					if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
					{
						m->mTargetX = x + 1;
						m->mWantDir = 1;
					}
					else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
					{
						m->mTargetX = x - 1;
						m->mWantDir = 3;
					}
				}
			}
		}
		if (m->mTargetX == m->mX && m->mTargetY == m->mY)
		{
			int ex = mExplorer->mX;
			if (m->mX < ex)
				m->mWantDir = 1;
			else if (ex < m->mX)
				m->mWantDir = 3;
			else if (m->mY < mExplorer->mY)
				m->mWantDir = 2;
			else
				m->mWantDir = 0;
		}
		int aSpeed = mApp->mSpeed;
		int anExtra = 0;
		if (aSpeed > 1)
			anExtra = aSpeed;
		if (!aSounded)
		{
			if (m->mX != m->mTargetX || m->mY != m->mTargetY)
			{
				mApp->PlaySample(abs(CrtRand() % 2) + 1 + anExtra, GetPanAt(m->mTargetX));
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
		Scorp* s = mScorpion;
		s->mProgress = 0;
		int x = s->mX;
		int y = s->mY;
		if (!mRed)
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				s->mAlive = false;
			else
			{
				int ex = mExplorer->mX;
				if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
				{
					s->mToX = x + 1;
					s->mDir = 1;
				}
				else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
				{
					s->mToX = x - 1;
					s->mDir = 3;
				}
				else
				{
					int ey = mExplorer->mY;
					if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0 && (!mGateClosed || x != mGateX || y != mGateY))
					{
						s->mToY = y + 1;
						s->mDir = 2;
					}
					else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0 && (!mGateClosed || x != mGateX || y - 1 != mGateY))
					{
						s->mToY = y - 1;
						s->mDir = 0;
					}
				}
			}
		}
		else
		{
			if (x < 0 || mSize <= x || y < 0 || mSize <= y)
				s->mAlive = false;
			else
			{
				int ey = mExplorer->mY;
				if (y < ey && (mCells[x][y] & WALL_SOUTH) == 0)
				{
					s->mToY = y + 1;
					s->mDir = 2;
				}
				else if (ey < y && (mCells[x][y] & WALL_NORTH) == 0)
				{
					s->mToY = y - 1;
					s->mDir = 0;
				}
				else
				{
					int ex = mExplorer->mX;
					if (x < ex && (mCells[x][y] & WALL_EAST) == 0)
					{
						s->mToX = x + 1;
						s->mDir = 1;
					}
					else if (ex < x && (mCells[x][y] & WALL_WEST) == 0)
					{
						s->mToX = x - 1;
						s->mDir = 3;
					}
				}
			}
		}
		s = mScorpion;
		if (s->mToX == s->mX && s->mToY == s->mY)
		{
			int ex = mExplorer->mX;
			if (s->mX < ex)
			{
				s->mDir = 1;
				s->mProgress = 0;
				mState = STATE_SCORPION;
				return;
			}
			if (ex < s->mX)
			{
				s->mDir = 3;
				s->mProgress = 0;
				mState = STATE_SCORPION;
				return;
			}
			if (s->mY < mExplorer->mY)
			{
				s->mDir = 2;
				s->mProgress = 0;
				mState = STATE_SCORPION;
				return;
			}
			s->mDir = 0;
			s->mProgress = 0;
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
		Mummy* m = mMummies[i];
		if (m != NULL && m->mFighting)
		{
			m->mFightFrame++;
			if (m->mFightFrame == 0x80)
			{
				m->mFighting = false;
				if (mState == STATE_CAUGHT)
				{
					DoGameOver();
					m->mFighting = true;
				}
				else
					m->CheckGate();
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
		Mummy* m = mMummies[i];
		if (m == NULL || !m->mAlive)
			continue;
		int aDir = m->mDir;
		int aWant = m->mWantDir;
		if (aDir == aWant)
		{
			if (m->mProgress < (double) mCellSize && (m->mTargetX != m->mX || m->mTargetY != m->mY))
			{
				aAllArrived = false;
				m->mProgress = (double) mCellSize / (60.0 / (double) mApp->mSpeed) + m->mProgress;
				MarkDirty();
			}
		}
		else if (mUpdateCnt % (mApp->mSpeed - 3) == 0)
		{
			// turn a quarter at a time, the short way round
			if (((aWant < aDir) && (aDir - aWant < 3)) || ((aDir < aWant) && (2 < aWant - aDir)))
			{
				m->mTurn--;
				if (m->mTurn < -3)
				{
					m->mTurn = 0;
					m->mDir--;
				}
				if (m->mDir < 0)
					m->mDir = 3;
			}
			else
			{
				m->mTurn++;
				if (m->mTurn == 4)
				{
					m->mTurn = 0;
					m->mDir++;
				}
				if (m->mDir < 0)
					m->mDir = 3;
			}
			if (m->mDir > 3)
				m->mDir = 0;
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
		Mummy* m = mMummies[i];
		if (m == NULL || !m->mAlive)
			continue;
		bool aMoved = true;
		if (m->mX == m->mTargetX && m->mY == m->mTargetY)
			aMoved = false;
		m->mX = m->mTargetX;
		m->mY = m->mTargetY;
		m->mProgress = 0.0;
		bool aKilled = false;
		for (int j = 0; j < 2; j++)
		{
			Mummy* o = mMummies[j];
			if (o != NULL && j != m->mIndex && o->mAlive && m->mX == o->mTargetX && m->mY == o->mTargetY)
			{
				o->mAlive = false;
				aKilled = true;
			}
		}
		Scorp* s = mScorpion;
		bool aFight = false;
		if (m->mX == s->mX && m->mY == s->mY && s->mAlive)
		{
			s->mAlive = false;
			aFight = true;
		}
		else if (aKilled)
			aFight = true;
		else if (aMoved)
			m->CheckGate();
		if (aFight)
		{
			m->mFighting = true;
			m->mFightFrame = 0;
			mApp->PlaySample(SOUND_PUMMEL, GetPanAt(m->mTargetX));
		}
		if (m->mX == mExplorer->mX && m->mY == mExplorer->mY)
		{
			m->mFighting = true;
			m->mFightFrame = 0;
			mApp->PlaySample(SOUND_PUMMEL, GetPanAt(m->mTargetX));
			mState = STATE_CAUGHT;
			m274 = mUpdateCnt;
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
	if (mApp->mGameMode >= 4)
	{
		mTutorialStep++;
		int v = kTutorialMove[mTutorialStep];
		if (v == 5)
			mPlayNowButton0->SetVisible(true);
		else if (v == 6)
		{
			mPlayNowButton0->mLabel = GT(0x00491dfc);
			mPlayNowButton0->SetVisible(true);
			mPlayNowButton->SetVisible(true);
		}
		else if (v == 7)
		{
			mPlayNowButton0->mLabel = GT(0x00491dd0);
			mPlayNowButton0->SetVisible(true);
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
	Scorp* s = mScorpion;
	if (s == NULL)
		return;
	if (s->mStinging)
	{
		s->mStingFrame++;
		if (s->mStingFrame == 0x80)
		{
			s->mStinging = false;
			mExplorer->mDir = 2;
			mState = STATE_SCORPION_LAST;
			s->mToX = s->mFromX;
			s->mToY = s->mFromY;
		}
		MarkDirty();
		return;
	}
	if (mState != STATE_SCORPION && mState != STATE_SCORPION_LAST)
		return;
	if (s->mAlive)
	{
		if (s->mProgress == 0 && (s->mToX != s->mX || s->mToY != s->mY))
			mApp->PlaySample(abs(CrtRand() % 2) + 0xd, GetPanAt(s->mToX));
		s = mScorpion;
		if (s->mProgress < mCellSize && (s->mToX != s->mX || s->mToY != s->mY))
		{
			s->mProgress += mApp->mSpeed;
			MarkDirty();
			return;
		}
		if (s->mAlive)
		{
			bool aMoved = true;
			if (s->mX == s->mToX && s->mY == s->mToY)
				aMoved = false;
			s->mFromX = s->mX;
			s->mFromY = s->mY;
			s->mX = s->mToX;
			s->mY = s->mToY;
			s->mProgress = 0;	// 0x408567
			for (int i = 0; i < 2; i++)
			{
				Mummy* m = mMummies[i];
				if (m != NULL && s->mX == m->mX && s->mY == m->mY && m->mAlive)
				{
					s->mAlive = false;
					m->mFighting = true;
					m->mFightFrame = 0;
					mApp->PlaySample(SOUND_PUMMEL, GetPanAt(s->mToX));
					return;
				}
			}
			if (s->mX == mExplorer->mX && s->mY == mExplorer->mY)
			{
				s->mStinging = true;
				s->mStingFrame = 0;
				mState = STATE_CAUGHT;
				m274 = mUpdateCnt;
				mApp->PlaySample(SOUND_PUMMEL, GetPanAt(s->mToX));
				mExplorer->mDir = 2;
				if (mApp->mGameMode != MODE_ADVENTURE)
					return;
				mApp->AdventureCaught();
				return;
			}
			if (mState == STATE_SCORPION_LAST)
			{
				mDeathTimer2 = 0;
				mApp->PlaySample(SOUND_POISON, GetPanAt(mExplorer->mX));
			}
			if (aMoved)
				s->CheckGate();
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

// 0x4067f0
// 0x406f00
void Board::Update()
{
	if (mApp->mWorldMap != NULL)
		return;
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
	if (mUpdateCnt - m270 == 0x32 && mState == STATE_SOLVING)
		DoSolution();
	if (mUpdateCnt - mTimeBase == 0x4b)
	{
		int aMode = mApp->mGameMode;
		if (aMode != 2 && aMode != 3)
			mApp->PlaySample(SOUND_TOMBSLIDE);
	}
	if (mUpdateCnt - mTimeBase == 0xa5)
	{
		int aMode = mApp->mGameMode;
		if (aMode != 2 && aMode != 3)
			mApp->PlaySample(SOUND_MUMMYHOWL);
	}
	if (mUpdateCnt - mTimeBase < 0xa5)
		mFlagEc1 = true;
	if (mFlagEc1 && 0xa4 < mUpdateCnt - mTimeBase)
	{
		int aMode = mApp->mGameMode;
		if (aMode != 2 && aMode != 3)
		{
			if (aMode > 3)
				mPlayNowButton0->SetVisible(true);
			mFlagEc1 = false;
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
	if (mDeathTimer > -1 && mDeathTimer < 0x118)
	{
		mDeathTimer++;
		MarkDirty();
		mApp->mSidePanel->MarkDirty();
	}
	if (mDeathTimer == 0x23 && mExitStyle == 0)
		mApp->PlaySample(SOUND_BLOCK, GetPanAt(mExplorer->mTargetX));
	else if (mDeathTimer == 3 && mExitStyle == 1)
		mApp->PlaySample(SOUND_PIT, GetPanAt(mExplorer->mTargetX));
	if (mDeathTimer == 0x118 && mState != STATE_SOLVING)
		DoGameOver();
	if (mDeathTimer2 > -1 && mDeathTimer2 < 100)
	{
		mDeathTimer2++;
		MarkDirty();
	}
	if (mDeathTimer2 == 100 && mState != STATE_SOLVING)
		DoGameOver();
	if (mState != STATE_ENTER)
	{
		int aMode = mApp->mGameMode;
		if (aMode != 2 && aMode != 3)
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
			if (aFade2 != mFade2)
			{
				mFade2 = aFade2;
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
		int aTime = mUpdateCnt - m274;
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
	if (mFade2 < 1.0)
		return;
	int aMode = mApp->mGameMode;
	if (aMode == 2 || aMode == 3)
		return;
	for (int i = 0; i < 2; i++)
		if (mMummies[i] != NULL && mMummies[i]->mFighting)
			return;
	Player* e = mExplorer;
	int aPixelX = x - 10;
	if (mState != STATE_READY)
	{
		if (mState != STATE_EXPLORER && mState != STATE_SCORPION && mState != STATE_MUMMY1 && mState != STATE_MUMMY2)
			return;
		if (mShowingSolution)
			return;
		int aCX = GetColAt(aPixelX);
		int aCY = GetRowAt(y);
		if (aCX == e->mTargetX && aCY == e->mTargetY)
		{
			mClickX = e->mTargetX;
			mClickY = e->mTargetY;
			return;
		}
		if (aCX == e->mTargetX - 1 && aCY == e->mTargetY)
		{
			mClickX = e->mTargetX - 1;
			mClickY = e->mTargetY;
			return;
		}
		if (aCX == e->mTargetX + 1 && aCY == e->mTargetY)
		{
			mClickX = e->mTargetX + 1;
			mClickY = e->mTargetY;
			return;
		}
		if (aCX == e->mTargetX && aCY == e->mTargetY - 1)
		{
			mClickX = e->mTargetX;
			mClickY = e->mTargetY - 1;
			return;
		}
		if (aCX == e->mTargetX && aCY == e->mTargetY + 1)
		{
			mClickX = e->mTargetX;
			mClickY = e->mTargetY + 1;
		}
		return;
	}

	mState = STATE_EXPLORER;
	if (mShowingSolution)
		return;
	bool aMoved = false;
	if (GetColAt(aPixelX) == e->mX && GetRowAt(y) == e->mY)
	{
		if (aMode < 4 || kTutorialMove[mTutorialStep] == 4)
		{
			DoNewTurn();
			SetScorpInMotion();
			aMoved = true;
		}
	}
	else
	{
		if (GetColAt(aPixelX) == e->mX - 1 && GetRowAt(y) == e->mY && CanMove(e->mX - 1, e->mY, true))
		{
			if (aMode < 4 || kTutorialMove[mTutorialStep] == 1)
			{
				DoNewTurn();
				mExplorer->mDir = 3;
				mExplorer->mTargetX = mExplorer->mX - 1;
				aMoved = true;
			}
		}
		else if (GetColAt(aPixelX) == e->mX + 1 && GetRowAt(y) == e->mY && CanMove(e->mX + 1, e->mY, true))
		{
			if (aMode < 4 || kTutorialMove[mTutorialStep] == 2)
			{
				DoNewTurn();
				mExplorer->mDir = 1;
				mExplorer->mTargetX = mExplorer->mX + 1;
				aMoved = true;
			}
		}
		else if (GetColAt(aPixelX) == e->mX && GetRowAt(y) == e->mY - 1 && CanMove(e->mX, e->mY - 1, true))
		{
			if (aMode < 4 || kTutorialMove[mTutorialStep] == 0)
			{
				DoNewTurn();
				mExplorer->mDir = 0;
				mExplorer->mTargetY = mExplorer->mY - 1;
				aMoved = true;
			}
		}
		else if (GetColAt(aPixelX) == e->mX && GetRowAt(y) == e->mY + 1 && CanMove(e->mX, e->mY + 1, true) &&
			(aMode < 4 || kTutorialMove[mTutorialStep] == 3))
		{
			DoNewTurn();
			mExplorer->mDir = 2;
			mExplorer->mTargetY = mExplorer->mY + 1;
			aMoved = true;
		}
	}
	if (!aMoved)
		mState = STATE_READY;
	e = mExplorer;
	if (e->mTargetX == e->mX && e->mTargetY == e->mY)
		return;
	MoveSound(e->mTargetX);
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
		mPlayNowButton0->SetVisible(false);
		mTutorialStep++;
		{
			int v = kTutorialMove[mTutorialStep];
			if (v == 5)
			{
				mPlayNowButton0->SetVisible(true);
				return;
			}
			if (v != 6)
			{
				if (v != 7)
					return;
				mPlayNowButton0->mLabel = GT(0x00491dd0);
				mPlayNowButton0->mId = 2;
				mPlayNowButton0->SetVisible(true);
				return;
			}
			mPlayNowButton0->mLabel = GT(0x00491dfc);
			mPlayNowButton0->mId = 1;
			mPlayNowButton0->SetVisible(true);
			mPlayNowButton->SetVisible(true);
		}
		return;
	case 1:
		mPlayNowButton->SetVisible(false);
		mPlayNowButton0->SetVisible(false);
		mPlayNowButton0->mLabel = GT(0x00491f20);
		mPlayNowButton0->mId = 0;
		mApp->mGameMode = 7;
		mTutorialStep++;
		NewGame();
		return;
	case 2:
		mPlayNowButton0->SetVisible(false);
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
