// Board: the introduction of a stage (the warp streaks slow down and the field comes up) and the tally at the end of
// a stage or a game (UpdateIntroAndTally, 0x466e80).

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "Font.h"
#include "GameText.h"
#include "Particle.h"
#include "Res.h"

using namespace Sexy;

// 0x466e80 (3168 bytes)
void Board::UpdateIntroAndTally()
{
	if (mBonusWarpIn)
	{
		int aPhase = m1bc0;
		if (aPhase == 1)
		{
			// the warp ends: the streaks run, the screen shakes and fades in
			UpdateStreaks();
			mWarpShakeX = ((int) AstroRand() % 500000000) % 0xb - 5;
			if (m1bc8 == 0x3b && mUpdateCnt % 2 == 0)
				mApp->PlaySample(SOUND_WARP);
			if (mUpdateCnt % 2 == 0)
			{
				m1bc8++;
				if (m1bc8 > 0x4f)
				{
					m1bc0 = 2;
					m1bc4 = 0;
					m1bc8 = 0xff;
					m1bd0 = 0x182 - GetFontById(0xb1)->StringWidth(GT(0x566074)) / 2;
				}
			}
			mOverlay.SetFill(255, 255, 255, m1bc8);
		}
		else if (aPhase == 2)
		{
			// the white screen holds; then the texts go and the streaks take the colour of their kind
			UpdateStreaks();
			mWarpShakeX = ((int) AstroRand() % 500000000) % 0xb - 5;
			mOverlay.SetFill(255, 255, 255, m1bc8);
			m1bc4++;
			if (m1bc4 > 0x31)
			{
				for (unsigned int i = 0; i < mFloatTexts.size(); i++)
					delete mFloatTexts[i];
				std::vector<FloatText*>().swap(mFloatTexts);
				m1bc4 = 0;
				m1bc0 = 3;
				mWarpShakeX = 0;
				for (unsigned int i = 0; i < 100 && i < mStreaks.size(); i++)
				{
					StreakRec* aStreak = &mStreaks[i];
					aStreak->mFlag28 = false;
					if (aStreak->mKind == 0)
						aStreak->mColor = Color(0, 0, 250);
					else if (aStreak->mKind == 1)
						aStreak->mColor = Color(250, 0, 0);
					else if (aStreak->mKind == 2)
						aStreak->mColor = Color(250, 250, 0);
				}
				mBricksRevealing = true;
				StartBonusRound(-1);
			}
		}
		else if (aPhase == 3)
		{
			// the streaks and the stars slow down and the white fades away
			int aStreaksDone = 0;
			if (m1bc8 > 0)
			{
				m1bc8 -= 2;
				if (m1bc8 < 1)
				{
					m1bc8 = 0;
					SetOverlayHidden(true);
				}
				mOverlay.SetFill(255, 255, 255, m1bc8);
			}
			for (unsigned int i = 0; i < 100 && i < mStreaks.size(); i++)
			{
				StreakRec* aStreak = &mStreaks[i];
				int aClamped = 0;
				aStreak->mMargin -= 3;
				if (aStreak->mMargin < 0)
				{
					aStreak->mMargin = 0;
					aClamped = 1;
				}
				aStreak->mSpeed -= 0.08f;
				if (aStreak->mSpeed <= aStreak->mMinSpeed)
				{
					aStreak->mSpeed = aStreak->mMinSpeed;
					aClamped++;
				}
				if (aClamped == 2)
					aStreaksDone++;
			}
			int aStarsDone = 0;
			for (unsigned int i = 0; i < mBackdropStars.size(); i++)
			{
				mBackdropStars[i].mSpeed -= 0.4f;
				if (mBackdropStars[i].mSpeed <= 0.1f)
				{
					mBackdropStars[i].mSpeed = 0.1f;
					aStarsDone++;
				}
			}
			// (x87 at 24 bits: the sum is a float when it is compared)
			float aTime = m1c00 + 0.09f;
			m1c00 = aTime;
			if (aTime >= 9.0f && aStreaksDone == 100)
			{
				if (aStarsDone == (int) mBackdropStars.size() && m1bc8 < 1)
				{
					m1bc0 = 0;
					mBonusWarpIn = false;
				}
			}
		}
	}

	bool aTimeUp = mBonusRoundOn;
	if (aTimeUp)
	{
		if (mSupaBannerStep == 0 && mBonusFadingOut)
		{
			m1b74 -= 2;
			if (m1b74 < 1)
			{
				m1b74 = 0;
				mBonusFadingOut = false;
			}
		}
		if (mSupaBannerStep == 0 && m1b7c > 0)
		{
			int aTime = m1b7c;
			m1b84 = aTime;
			m1b7c = aTime - 1;
			mBannerWavePhase = (float) ((double) mBannerWavePhase + 0.07);
			if (aTime - 1 < 1)
				m1b7c = 0;
		}
	}

	if (!mBonusWarpIn && mSupaBannerStep == 0 && mBonusRoundStarted && !mClearCountRunning)
	{
		// the time runs out: when everything has settled the clear count starts
		if (aTimeUp && !mStartBannerDoneInBonus && !mBricksRevealing)
		{
			if (m1b70 > 0)
				m1b70--;
			if (m1b70 % 100 == 0 && m1b70 > 0)
				gStateSaved = false;
			if (m1b70 < 1 && CountMovingBricks(false) == 0 && mKillAllTypes.empty() && mTakenBrickCount == 0 && mFallingGroups.empty() && mFiredGroups.empty())
			{
				m1b70 = 0;
				mClicksBlocked = true;
				m1b7c = 400;
				BuildBanner(GT(0x566054), m1b7c, FONT_NEURO30);
				StartClearCount();
			}
		}
		return;
	}
	if (!mClearCountRunning)
		return;

	// the tally
	if (mSupaBannerStep == 0 && m1b7c < 1)
	{
		int aState = m1bb0;
		if (aState == 0)
		{
			m1b88 += 8;
			if (m1b88 > 0x13f)
			{
				m1b88 = 0x140;
				m1bb0 = 1;
				if (m1b94 == 0)
					mApp->PlaySample(SOUND_PERFECT);
			}
		}
		else if (aState == 1)
		{
			m1b90--;
			m1b8c += 8;
			if (m1b8c > 0xff)
				m1b8c = 0xff;
			if (m1b90 < 1)
			{
				m1bb0 = 2;
				m1b90 = 0x19;
			}
		}
		else if (aState == 2)
		{
			m1b90--;
			if (m1b90 < 1)
			{
				m1b90 = 0x19;
				m1bb0 = 3;
			}
		}
		else if (aState == 3)
		{
			m1b90--;
			if (m1b90 < 1)
			{
				m1ba0 += m1bb4;
				m1b98++;
				if (m1b9c <= m1ba0)
					m1ba0 = m1b9c;
				if (m1bb8 == 0)
					mApp->PlaySample(SOUND_COUNT);
				m1bb8++;
				if (m1bb8 > 1)
					m1bb8 = 0;
				if (m1b98 < m1b94)
					m1b90 = 3;
				else
				{
					m1b98 = m1b94;
					m1bb0 = 4;
					m1b90 = 10;
					m1ba0 = m1b9c;
					if (m1b94 > 0)
					{
						m1bb4 = 0;
						m1ba8 = 0;
					}
				}
			}
		}
		else if (aState == 4)
		{
			m1b90--;
			if (m1b90 < 1)
			{
				m1b90 = (m1b94 != 0 ? 0x7d : 0) + 0x19;
				m1bb0 = 5;
			}
		}
		else if (aState == 5)
		{
			m1b90--;
			if (m1b90 < 1)
			{
				int aTotal;
				if (m1b94 == 0)
				{
					m1ba8 += m1bb4;
					if (m1bb8 == 0)
						mApp->PlaySample(SOUND_COUNT);
					m1bb8++;
					if (m1bb8 > 3)
						m1bb8 = 0;
					if (m1ba8 < m1ba4)
					{
						m1b90 = 1;
						return;
					}
					aTotal = (mClearBonusWorth - m1b9c) + m1ba4;
					m1ba8 = m1ba4;
				}
				else
					aTotal = (m1ba4 - m1b9c) + mClearBonusWorth;
				m1bb0 = 6;
				m1b90 = 0x14;
				m1bac = 0;
				// the tally counts up faster for a bigger total
				if (aTotal < 50000)
					m1bb4 = 500;
				else if (aTotal > 99999)
					m1bb4 = (aTotal > 199999) ? 4000 : 2000;
				else
					m1bb4 = 1000;
			}
		}
		else if (aState == 6)
		{
			m1b90--;
			if (m1b90 < 1)
			{
				m1b90 = 1;
				int aTotal = (m1ba4 - m1b9c) + mClearBonusWorth;
				int aPaid = m1bac;
				if (aPaid < aTotal)
				{
					int aStep = m1bb4;
					if (aTotal < aPaid + aStep)
						aStep = aTotal - aPaid;
					m1bac = aPaid + aStep;
					AddScore(aStep, false);
				}
				if (m1bb8 == 0)
					mApp->PlaySample(SOUND_COUNT);
				m1bb8++;
				if (m1bb8 > 7)
					m1bb8 = 0;
				if (aTotal <= m1bac)
				{
					m1bac = aTotal;
					m1bb0 = 8;
					m1b90 = 400;
				}
			}
		}
		else if (aState == 8)
		{
			m1b90--;
			if (m1b90 < 1)
			{
				// the tally is over: the bricks left on the field are given up and the next stage begins
				mClearCountRunning = false;
				mBonusRoundOn = false;
				mClicksBlocked = false;
				for (std::list<Brick*>::iterator anItr = mHeldBricks.begin(); anItr != mHeldBricks.end(); ++anItr)
				{
					mDoomedBricks.insert(*anItr);
					(*anItr)->mGone = true;
				}
				mHeldBricks.clear();
				StartLevelUp();
			}
		}
	}
}
