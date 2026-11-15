// Board: the state that is reset for each game, the per-update tick of the effects, and the score.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Image.h"
#include "Res.h"

using namespace Sexy;

// a test's weapon charge for a new board and for ResetState (a test command, `set chargestart`; below 0: none, the charge starts at 0)
float Sexy::gTestChargeAtReset = -1.0f;

// 0x4346c0: the numbers a new game (or a new level) starts from
void Board::ResetState()
{
	mWelcomePlayed = false;
	mSupaButtonX = -IMAGE_SUPA_BUTTON_ALERT->mWidth;
	mWeaponCharge = gTestChargeAtReset < 0.0f ? 0.0f : gTestChargeAtReset;
	mBrickometerDrain = 0;
	mBrickometerFill = 0;
	mFlurryTimer = 0;
	mPhase = 4;
	mPhaseDelay = PickSpawnDelay();
	mResumeUpdate = 0;
	mPauseStartUpdate = 0;
	mClickRepeating = false;
	mPlayTime = 0;
	mComboChain = 0;
	mComboTimeLeft = 0;
	mButtonsHeld = 0;
	mScore = 0;
	mScoreShown = 0;
	mHandKindBrick = NULL;
	mShipX = 0xa6;
	mSupaButtonLit = false;
	mShipFrame = 2;
	mStreaksFadingOut = false;
	mPowerupActive = 0;
	mPlayStarted = false;
	mSirenOffDelay = -1;
	mSirenAnim.SetNumFrames(IMAGE_SIREN->mNumCols);
	mSirenAnim.SetDelay(8);
	mSirenAnim.mHoldLast = false;
	mSirenAnim.SetStopped(true);
}

// 0x439c20: one update's worth of ticking for the fades: the alpha at +0x1b38 climbs, the overlay's colour fades out
// (and the overlay goes when the colour is gone and nothing stops it), the streaks slow down, and so on
void Board::UpdateEffects()
{
	mShipFadeAlpha += 2;
	if (mShipFadeAlpha > 0xff)
		mShipFadeAlpha = 0xff;

	mOverlayFillColor.mAlpha -= 2;
	if (mOverlayFillColor.mAlpha < 0)
	{
		if (mHintSpots.empty() || mHintSpots[0]->mActive == 0 || -mLevelUpPanelHeight < mLevelUpPanelY)
			SetOverlayHidden(true);
		mOverlayFillColor.mAlpha = 0;
	}
	mOverlay.SetFill(mOverlayFillColor.mRed, mOverlayFillColor.mGreen, mOverlayFillColor.mBlue, mOverlayFillColor.mAlpha);

	int anOld = mSceneryLayer.mPeriod;
	mSceneryLayer.mPeriod = anOld + 1;
	if (1 < anOld)
		mSceneryLayer.mPeriod = 1;

	int aNumDone = 0;
	for (int i = 0; i < 100; i++)
	{
		StreakRec* aStreak = &mStreaks[i];
		aStreak->mMargin -= 3;
		if (aStreak->mMargin < 0)
		{
			aStreak->mMargin = 0;
			aNumDone++;
		}
		aStreak->mSpeed = aStreak->mSpeed - 0.04f;
		if (aStreak->mSpeed < aStreak->mMinSpeed)
			aStreak->mSpeed = aStreak->mMinSpeed;
	}
	if (aNumDone == 100)
		mStreaksFadingOut = false;

	int aPhase = mShipFrame;
	mEffectsUpdateCount++;
	if (aPhase != 2)
	{
		if (mUpdateCnt % 8 == 0 && -1 < mEffectsUpdateCount)
		{
			if (aPhase < 3)
				mShipFrame = 2;
			else
				mShipFrame = aPhase - 1;
		}
	}

	if (0.0f < mBrickometerDrain)
	{
		float aValue = mBrickometerDrain - 0.0076f;
		mBrickometerDrain = aValue;
		if (aValue < 0.0f)
			mBrickometerDrain = 0;
		mBrickometerFill = mBrickometerDrain;
	}
}

// 0x4344d0: the score the next level needs, by the level
void Board::SetNextLevelScore()
{
	int aLevel = mLevelNumber;
	if (aLevel < 4)
		mClearBonusWorth = 10000;
	else if (aLevel < 8)
		mClearBonusWorth = 25000;
	else if (aLevel < 0xc)
		mClearBonusWorth = 50000;
	else if (aLevel < 0x10)
		mClearBonusWorth = 75000;
	else if (aLevel < 0x14)
		mClearBonusWorth = 100000;
	else if (aLevel < 0x47)
		mClearBonusWorth = (((aLevel - 0x13) / 4) + 2) * 50000;
	else
		mClearBonusWorth = 500000;
	if (500000 < mClearBonusWorth)
		mClearBonusWorth = 500000;
}

// 0x433b90: points are added to the score; the best score of the mode is kept in the profile, and the points are
// counted for the stats (mPlayPoints while mBonusRoundOn is off, else mBonusRoundPoints unless mClearCountRunning holds them back)
void Board::AddScore(int thePoints, bool theForce)
{
	mScore += thePoints;
	int aBest = 0;
	mApp->mUser->GetInt(mMode, &aBest);
	if (aBest < mScore)
		mApp->mUser->SetInt(mMode, mScore);
	if (!mBonusRoundOn)
	{
		mPlayPoints += thePoints;
		return;
	}
	if (!mClearCountRunning || theForce)
		mBonusRoundPoints += thePoints;
}

// the best of a stat is kept in the profile (setting 0x5d)
// port: split out of Board::SaveGameState (0x43a9a0)
static void SaveBestStat(UserProfile* theUser, int theValue)
{
	int aBest = 0;
	theUser->GetInt(0x5d, &aBest);
	if (aBest < theValue)
		theUser->SetInt(0x5d, theValue);
}

// 0x43a9a0: when nothing moves, the field and the counters are written to the profile (so that a game can be
// continued): the type of every cell (and a number for steel and jammed bricks) in the profile's table of scores, the
// rest in its settings 0x56 .. 0x65. Which settings are written depends on the mode the game is in.
void Board::SaveGameState()
{
	if (mPowerupActive != 0)
		return;
	if (mHeldBricksNotLanded != 0)
		return;
	if (!mHeldBricks.empty())
		return;
	if (mNewRowBouncing)
		return;
	if (!mFallingGroups.empty())
		return;
	if (!mFiredGroups.empty())
		return;
	if (CountMovingBricks(false) != 0)
		return;
	if (!mAtomicMarkedBricks.empty())
		return;
	if (!mTakenBricks.empty())
		return;
	if (!mTurbotMarkedCells.empty())
		return;
	if (!mKillAllTypes.empty())
		return;

	UserProfile* aUser = mApp->mUser;
	gStateSaved = true;
	bool aIdle = !mWarpActive && !mWarpCharging && !mWarpFlight && !mAtWarpGate && !mWarpGatePicked;
	if (!mClearCountRunning && aIdle && !mBonusWarpIn && !mBricksRevealing)
	{
		if (!mFieldBricks.empty())
		{
			for (unsigned int aRow = 0; aRow < mFieldBricks.size(); aRow++)
			{
				for (int aCol = 0; aCol < 10; aCol++)
				{
					Brick* aBrick = mFieldBricks[aRow][aCol];
					int aType;
					if (aBrick == NULL)
						aType = -1;
					else
						aType = aBrick->GetEffectiveType();
					int aValue = -1;
					if (aBrick == NULL || !aBrick->IsJammed())
					{
						if (aType == 9)
							aValue = static_cast<SpecialBrick*>(aBrick)->mStage;
					}
					else
						aValue = 500000000;
					aUser->mScores[aRow][aCol].mValue = aType;
					aUser->mScores[aRow][aCol].mAux = aValue;
				}
			}
		}
	}

	// which of the four sets of settings is written: the classic game (A: nothing running, no 912/917), the timed
	// mode (Y, mBonusRoundOn on), the others (W), or the game's end (Z, mBonusWarpIn or mBricksRevealing). The original tests the same flags in a
	// tangle of jumps (and a return that no combination of them reaches); this is the same decision.
	if (!mBonusWarpIn && !mBricksRevealing)
	{
		if (!mBonusRoundOn && !mClearCountRunning && aIdle)
		{
			StoreProgress();
			aUser->SetInt(0x56, mStageBricksCleared);
			aUser->SetInt(0x57, 0);
			aUser->SetInt(0x61, mScoreAtStageStart);
			aUser->SetInt(0x58, 0);
			aUser->SetInt(0x62, 0);
			aUser->SetInt(0x5b, 0);
			aUser->SetInt(0x60, mRampTime);
			aUser->SetInt(0x5c, mPlayPoints);
			SaveBestStat(aUser, mBestChain);
			if (mSuperchargeMarks.empty())
				aUser->SetInt(0x59, -1);
			else
			{
				aUser->SetInt(0x59, mSuperchargeMarks[0].mType);
				aUser->SetInt(0x5a, mSuperchargeMarks[0].mCount);
			}
			aUser->SetInt(0x65, mFreezeUpdatesLeft);
			return;
		}
		if (mBonusRoundOn && !mClearCountRunning && aIdle)
		{
			StoreProgress();
			aUser->SetInt(0x57, mBonusTimeLeft);
			aUser->SetInt(0x58, 0);
			aUser->SetInt(0x5b, 0);
			aUser->SetInt(0x62, mBonusRoundPoints);
			aUser->SetInt(0x61, mScoreAtStageStart);
			aUser->SetInt(0x5c, mPlayPoints);
			aUser->SetInt(0x60, mRampTime);
			SaveBestStat(aUser, mBestChain);
			if (mSuperchargeMarks.empty())
			{
				aUser->SetInt(0x59, -1);
				aUser->SetInt(0x5a, 0);
			}
			else
			{
				aUser->SetInt(0x59, mSuperchargeMarks[0].mType);
				aUser->SetInt(0x5a, mSuperchargeMarks[0].mCount);
			}
			aUser->SetInt(0x65, mFreezeUpdatesLeft);
			return;
		}
		StoreProgress();
		aUser->SetInt(0x58, 1);
		aUser->SetInt(0x5b, 0);
		aUser->SetInt(0x57, 0);
		aUser->SetInt(0x62, mBonusRoundPoints);
		aUser->SetInt(0x61, mScoreAtStageStart);
		aUser->SetInt(0x5c, mPlayPoints);
		aUser->SetInt(0x60, mRampTime);
		SaveBestStat(aUser, mBestChain);
		if (mLevelEndScreenKind != 1)
			return;
		aUser->SetInt(0x5f, (mTallyCleanBonus - mTallyPenalty) + mClearBonusWorth);
		return;
	}

	StoreProgress();
	aUser->SetInt(0x5b, 1);
	aUser->SetInt(0x57, 0);
	aUser->SetInt(0x62, 0);
	aUser->SetInt(0x61, mScoreAtStageStart);
	aUser->SetInt(0x5c, mPlayPoints);
	aUser->SetInt(0x60, mRampTime);
	SaveBestStat(aUser, mBestChain);
}

bool Sexy::gStateSaved = false;   // 0x5a5100: set by SaveGameState once it passes its quiet-field checks; the callback at +0x60 and the code that moves bricks clear it
