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
		int aPhase = mBonusWarpPhase;
		if (aPhase == 1)
		{
			// the warp ends: the streaks run, the screen shakes and fades in
			UpdateStreaks();
			mWarpShakeX = ((int) AstroRand() % 500000000) % 0xb - 5;
			if (mBonusWarpWhiteAlpha == 0x3b && mUpdateCnt % 2 == 0)
				mApp->PlaySample(SOUND_WARP);
			if (mUpdateCnt % 2 == 0)
			{
				mBonusWarpWhiteAlpha++;
				if (mBonusWarpWhiteAlpha > 0x4f)
				{
					mBonusWarpPhase = 2;
					mBonusWarpHold = 0;
					mBonusWarpWhiteAlpha = 0xff;
				}
			}
			mOverlay.SetFill(255, 255, 255, mBonusWarpWhiteAlpha);
		}
		else if (aPhase == 2)
		{
			// the white screen holds; then the texts go and the streaks take the colour of their kind
			UpdateStreaks();
			mWarpShakeX = ((int) AstroRand() % 500000000) % 0xb - 5;
			mOverlay.SetFill(255, 255, 255, mBonusWarpWhiteAlpha);
			mBonusWarpHold++;
			if (mBonusWarpHold > 0x31)
			{
				for (unsigned int i = 0; i < mFloatTexts.size(); i++)
					delete mFloatTexts[i];
				std::vector<FloatText*>().swap(mFloatTexts);
				mBonusWarpHold = 0;
				mBonusWarpPhase = 3;
				mWarpShakeX = 0;
				for (unsigned int i = 0; i < 100 && i < mStreaks.size(); i++)
				{
					StreakRec* aStreak = &mStreaks[i];
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
			if (mBonusWarpWhiteAlpha > 0)
			{
				mBonusWarpWhiteAlpha -= 2;
				if (mBonusWarpWhiteAlpha < 1)
				{
					mBonusWarpWhiteAlpha = 0;
					SetOverlayHidden(true);
				}
				mOverlay.SetFill(255, 255, 255, mBonusWarpWhiteAlpha);
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
			float aTime = mBonusWarpSlowdown + 0.09f;
			mBonusWarpSlowdown = aTime;
			if (aTime >= 9.0f && aStreaksDone == 100)
			{
				if (aStarsDone == (int) mBackdropStars.size() && mBonusWarpWhiteAlpha < 1)
				{
					mBonusWarpPhase = 0;
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
			mBonusFadeAlpha -= 2;
			if (mBonusFadeAlpha < 1)
			{
				mBonusFadeAlpha = 0;
				mBonusFadingOut = false;
			}
		}
		if (mSupaBannerStep == 0 && mBannerTimeLeft > 0)
		{
			int aTime = mBannerTimeLeft;
			mBannerTimeShown = aTime;
			mBannerTimeLeft = aTime - 1;
			mBannerWavePhase = (float) ((double) mBannerWavePhase + 0.07);
			if (aTime - 1 < 1)
				mBannerTimeLeft = 0;
		}
	}

	if (!mBonusWarpIn && mSupaBannerStep == 0 && mBonusRoundStarted && !mClearCountRunning)
	{
		// the time runs out: when everything has settled the clear count starts
		if (aTimeUp && !mStartBannerDoneInBonus && !mBricksRevealing)
		{
			if (mBonusTimeLeft > 0)
				mBonusTimeLeft--;
			if (mBonusTimeLeft % 100 == 0 && mBonusTimeLeft > 0)
				gStateSaved = false;
			if (mBonusTimeLeft < 1 && CountMovingBricks(false) == 0 && mKillAllTypes.empty() && mTakenBrickCount == 0 && mFallingGroups.empty() && mFiredGroups.empty())
			{
				mBonusTimeLeft = 0;
				mClicksBlocked = true;
				mBannerTimeLeft = 400;
				BuildBanner(GT(0x566054), mBannerTimeLeft, FONT_NEURO30);
				StartClearCount();
			}
		}
		return;
	}
	if (!mClearCountRunning)
		return;

	// the tally
	if (mSupaBannerStep == 0 && mBannerTimeLeft < 1)
	{
		int aState = mTallyState;
		if (aState == 0)
		{
			mTallyBarWidth += 8;
			if (mTallyBarWidth > 0x13f)
			{
				mTallyBarWidth = 0x140;
				mTallyState = 1;
				if (mTallyBricksLeft == 0)
					mApp->PlaySample(SOUND_PERFECT);
			}
		}
		else if (aState == 1)
		{
			mTallyDelay--;
			mTallyTitleAlpha += 8;
			if (mTallyTitleAlpha > 0xff)
				mTallyTitleAlpha = 0xff;
			if (mTallyDelay < 1)
			{
				mTallyState = 2;
				mTallyDelay = 0x19;
			}
		}
		else if (aState == 2)
		{
			mTallyDelay--;
			if (mTallyDelay < 1)
			{
				mTallyDelay = 0x19;
				mTallyState = 3;
			}
		}
		else if (aState == 3)
		{
			mTallyDelay--;
			if (mTallyDelay < 1)
			{
				mTallyPenaltyShown += mTallyAmountStep;
				mTallyBricksCounted++;
				if (mTallyPenalty <= mTallyPenaltyShown)
					mTallyPenaltyShown = mTallyPenalty;
				if (mTallySoundCounter == 0)
					mApp->PlaySample(SOUND_COUNT);
				mTallySoundCounter++;
				if (mTallySoundCounter > 1)
					mTallySoundCounter = 0;
				if (mTallyBricksCounted < mTallyBricksLeft)
					mTallyDelay = 3;
				else
				{
					mTallyBricksCounted = mTallyBricksLeft;
					mTallyState = 4;
					mTallyDelay = 10;
					mTallyPenaltyShown = mTallyPenalty;
					if (mTallyBricksLeft > 0)
					{
						mTallyAmountStep = 0;
						mTallyCleanBonusShown = 0;
					}
				}
			}
		}
		else if (aState == 4)
		{
			mTallyDelay--;
			if (mTallyDelay < 1)
			{
				mTallyDelay = (mTallyBricksLeft != 0 ? 0x7d : 0) + 0x19;
				mTallyState = 5;
			}
		}
		else if (aState == 5)
		{
			mTallyDelay--;
			if (mTallyDelay < 1)
			{
				int aTotal;
				if (mTallyBricksLeft == 0)
				{
					mTallyCleanBonusShown += mTallyAmountStep;
					if (mTallySoundCounter == 0)
						mApp->PlaySample(SOUND_COUNT);
					mTallySoundCounter++;
					if (mTallySoundCounter > 3)
						mTallySoundCounter = 0;
					if (mTallyCleanBonusShown < mTallyCleanBonus)
					{
						mTallyDelay = 1;
						return;
					}
					aTotal = (mClearBonusWorth - mTallyPenalty) + mTallyCleanBonus;
					mTallyCleanBonusShown = mTallyCleanBonus;
				}
				else
					aTotal = (mTallyCleanBonus - mTallyPenalty) + mClearBonusWorth;
				mTallyState = 6;
				mTallyDelay = 0x14;
				mTallyPaid = 0;
				// the tally counts up faster for a bigger total
				if (aTotal < 50000)
					mTallyAmountStep = 500;
				else if (aTotal > 99999)
					mTallyAmountStep = (aTotal > 199999) ? 4000 : 2000;
				else
					mTallyAmountStep = 1000;
			}
		}
		else if (aState == 6)
		{
			mTallyDelay--;
			if (mTallyDelay < 1)
			{
				mTallyDelay = 1;
				int aTotal = (mTallyCleanBonus - mTallyPenalty) + mClearBonusWorth;
				int aPaid = mTallyPaid;
				if (aPaid < aTotal)
				{
					int aStep = mTallyAmountStep;
					if (aTotal < aPaid + aStep)
						aStep = aTotal - aPaid;
					mTallyPaid = aPaid + aStep;
					AddScore(aStep, false);
				}
				if (mTallySoundCounter == 0)
					mApp->PlaySample(SOUND_COUNT);
				mTallySoundCounter++;
				if (mTallySoundCounter > 7)
					mTallySoundCounter = 0;
				if (aTotal <= mTallyPaid)
				{
					mTallyPaid = aTotal;
					mTallyState = 8;
					mTallyDelay = 400;
				}
			}
		}
		else if (aState == 8)
		{
			mTallyDelay--;
			if (mTallyDelay < 1)
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
