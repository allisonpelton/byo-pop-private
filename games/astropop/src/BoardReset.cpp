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
	m1aac = 0;
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
	m1b44 = 0;
	mHighScoreSoundDelay = 200;
	mHighScoreSoundDone = false;
	m1b3c = 0;
	mStartBannerDoneInBonus = false;
	mPsychicUsed = false;
	mLaserUsed = false;
	gStateSaved = false;
	m1b24 = 0;
	mBonusWarpIn = false;
	mLastChainCount = 0;
	m1b18 = 0;
	m1b10 = 0;
	m1aa8 = 0;
	m1aa4 = 0;
	mScoreAtStageStart = mScore;
	mClearWaitStarted = true;
	mExtraRowsDue = 0;
	mUnusedIntClearedInReset = 0;
	mStreaksFadingOut = false;
	mUnusedIntClearedInLevelUp = 0;
	mLevelUpPhase = false;
	mGameOver = false;
	mComboTextVisible = true;
	m1b54 = 0;
	mShipFlameFrame = 0;
	m1b68 = 0;
	m1b64 = 0;
	m1b60 = 0;
	mShipTilt = 2;
	mTakenBrickCount = 0;
	m1a3c = 0;
	mTopRowsAllowed = true;
	m1b5c = 0;
	mNewRowBouncing = false;
	mUnusedIntClearedOnClick = 0;
	mBoosterDue = false;
	mUnusedIntFromReset = 0;
	mUnusedFlagFromReset = false;
	mPickUpButtonDown = false;
	mBackdropTintChanging = false;
	mBackdropTint = Color(100, 100, 100);
	m1a98 = -500000000;
	m1a9c = -500000000;
	m1bf8 = -1.0f;
	m1bfc = 1.0f;
	mChainCount = 0;
	mExtraRowsDue = 0;
	m1ab4 = 0;
	m1a2c = 0;
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
	m1a48 = -65536;
	mPointsTextY = -1;
	mPointsTextX = -1;
	m1a68 = 0;
	m1a50 = 0;
	mSetMadeSinceTopRow = false;
	mGameOverAnimDone = false;
	m1a4c = -1;
	mUnusedClearedInReset = false;
	mPlayStarted = false;
	mWarpLightningImage = NULL;
	m19fc = 0;
	m1a44 = -1;
	m1a10 = 0;
	m1a40 = -1;
	mFiftyLeftSoundPlayed = false;
	mHundredLeftSoundPlayed = false;
	m1a14 = 0;
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
	if (m1bbc < mStageNumber && 8 < mRowIntervalReduction)
		mRowReductionPeriod = 8;
	m1a30 = 0;

	mSpecialCooldowns.clear();
	for (int i = 5; i != 0; i--)
	{
		int aCount = 2000;
		if (0x12 < mStageNumber || m1a60 == 2)
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
	m1a80 = 0;
}
