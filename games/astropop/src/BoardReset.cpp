// Board: the reset for a new game (the numbers every game starts from, the backdrop's pictures)

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "ResourceManager.h"
#include "Image.h"
#include "Res.h"
#include <vector>

using namespace Sexy;

// 0x450250: everything a new game (or a new try of a level) starts from. It draws random numbers: three different
// pictures of the backdrop (IMAGE_BG_OBJECT1 .. 7) and where the pictures are across the screen.
void Board::ResetGame()
{
	mHeldBricksNotLanded = 0;
	mGameWasReset = true;
	mDangerZoneAlpha = 0;
	mWarpActive = false;
	mWarpCharging = false;
	mWarpFlight = false;
	mNewRankAnim.Reset();
	mNewRankAnim.SetNumFrames(0x3f);
	mNewRankAnim.SetDelay(1);
	mNewRankAnim.mPingPong = true;
	mNewRankAnim.mMaxLoops = 6;
	mNewRankAnim.mLoopCount = 0;
	mNewRankAnim.SetStopped(true);
	mStartTextAlpha = 0;
	mChainCount = 0;
	mChainPoints = 0;
	mLevelUpTextShown = false;
	mUpgradeDialogSeen = false;
	mNewRankDelayOver = true;
	mNewRankDelay = 0;
	mHighScoreSoundDelay = 200;
	mHighScoreSoundDone = false;
	mFrameDimming = 0;
	mStartBannerDoneInBonus = false;
	mPsychicUsed = false;
	mLaserUsed = false;
	gStateSaved = false;
	mSprocketOverlayCel = 0;
	mBonusWarpIn = false;
	mLastChainCount = 0;
	mComboPayDelay = 0;
	mPowerBrickRowState = 0;
	mDangerPulseAlpha = 0;
	mDangerPulseStep = 0;
	mScoreAtStageStart = mScore;
	mClearWaitStarted = true;
	mExtraRowsDue = 0;
	mUnusedIntClearedInReset = 0;
	mStreaksFadingOut = false;
	mUnusedIntClearedInLevelUp = 0;
	mLevelUpPhase = false;
	mGameOver = false;
	mComboTextVisible = true;
	mTurbotWindUpCounter = 0;
	mShipFlameFrame = 0;
	mPerfectBannerHold = 0;
	mPerfectBannerState = 0;
	mPerfectBannerAlpha = 0;
	mShipTilt = 2;
	mTakenBrickCount = 0;
	mAtomicWeaponState = 0;
	mTopRowsAllowed = true;
	mMutatorCooldown = 0;
	mNewRowBouncing = false;
	mUnusedIntClearedOnClick = 0;
	mBoosterDue = false;
	mUnusedIntFromReset = 0;
	mUnusedFlagFromReset = false;
	mPickUpButtonDown = false;
	mBackdropTintChanging = false;
	mBackdropTint = Color(100, 100, 100);
	mGameOverDangerY = -500000000;
	mGameOverDangerHold = -500000000;
	mShipBounceY = -1.0f;
	mShipBounceStep = 1.0f;
	mChainCount = 0;
	mExtraRowsDue = 0;
	mExtraRowDelay = 0;
	mTimeBonusUpdates = 0;
	mUpgradeDialogDone = false;
	mTimeBonusRunning = false;
	mShipX = 0xa6;
	mAtWarpGate = false;
	mFieldCleared = false;
	mShipInWarp = false;
	mStackWarningActive = false;
	mStackWarningTimer = 0;
	mWarpGatePicked = false;
	mWarpEngaged = false;
	mTurbotWaveY = -65536;
	mPointsTextY = -1;
	mPointsTextX = -1;
	mSupportedBrickCount = 0;
	mTimeBonusSets = 0;
	mSetMadeSinceTopRow = false;
	mGameOverAnimDone = false;
	mTurbotFlashAlpha = -1;
	mUnusedClearedInReset = false;
	mPlayStarted = false;
	mWarpLightningImage = NULL;
	mFreezeUpdatesLeft = 0;
	mAtomicFlashAlpha = -1;
	mUnusedIntClearedWithBrickCount = 0;
	mAtomicSymbolTimer = -1;
	mFiftyLeftSoundPlayed = false;
	mHundredLeftSoundPlayed = false;
	mStageBricksCleared = 0;
	mAtomicBlastOver = false;
	mFieldsCleared = 0;
	mBestChain = 0;
	mBonusRoundPoints = 0;
	mPlayPoints = 0;
	mBricksGoneCount = 0;
	mPointerOverFastGate = false;
	mPointerOverSlowGate = false;
	mPointerOverEasyGate = false;
	mSuperchargeMarks.clear();
	mRowIntervalReduction = 0;
	mRemovalPoints = 0;
	if (mHardStage < mStageNumber && 8 < mRowIntervalReduction)
		mRowReductionPeriod = 8;
	mTimeBonusStepsPaid = 0;

	mSpecialCooldowns.clear();
	for (int i = 5; i != 0; i--)
	{
		int aCount = 2000;
		if (0x12 < mStageNumber || mGameType == 2)
			aCount = 0;
		mSpecialCooldowns.push_back(aCount);
	}
	mPauseDarkening = 0;
	mPausedUpdates = 0;
	mRampTime = 0;
	mUnusedIntSetTo500InReset = 500;
	mStackDanger = 0;
	mStackDangerPeak = 0;
	mFuelCellDue = false;
	mRowTimer = -1;
	mPauseCount = 0;

	// the pictures of the backdrop that are in use are dropped (so that they are loaded again)
	for (unsigned int i = 0; i < mBackdropStars.size(); i++)
	{
		int anId = mBackdropStars[i].mImageId;
		if (anId != -1)
			mApp->mResourceManager->DeleteImage(GetStringIdById(anId));
	}

	// three different pictures out of seven (the first draw is thrown away)
	std::vector<Image*> aPictures;
	AstroRand();
	int aFirst = ((int) AstroRand() % 500000000) % 7 + 0x18e;
	int aSecond;
	do
	{
		aSecond = ((int) AstroRand() % 500000000) % 7 + 0x18e;
	}
	while (aFirst == aSecond);
	int aThird = aSecond;
	while (aThird == aSecond || aThird == aFirst)
		aThird = ((int) AstroRand() % 500000000) % 7 + 0x18e;
	aPictures.push_back(LoadImageById(mApp->mResourceManager, aSecond));
	aPictures.push_back(LoadImageById(mApp->mResourceManager, aFirst));
	aPictures.push_back(LoadImageById(mApp->mResourceManager, aThird));

	for (unsigned int i = 0; i < mBackdropStars.size(); i++)
	{
		StarRec* aStar = &mBackdropStars[i];
		aStar->mImage = aPictures[i];
		if (i == 0)
			aStar->mImageId = aSecond;
		else if (i == 1)
			aStar->mImageId = aFirst;
		else
			aStar->mImageId = aThird;
		aStar->mX = (float) (((int) AstroRand() % 500000000) % (0x1b8 - aStar->mImage->mWidth) + 0xa6);
		aStar->mY = (float) ((int) i * -900 + 100);
		aStar->m60 = 0;
		aStar->mSpeed = 0.1f;
	}
	mScreenShakeX = 0;
}
