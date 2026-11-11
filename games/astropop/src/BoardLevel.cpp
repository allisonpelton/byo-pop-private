// Board: the end of a level. The bonus for a run of combos is paid, the pieces on the field are handed to the
// deleting, and the next phase starts (the bonus round, or the level-up screen).

#include "Board.h"
#include "Brick.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "SmallObjects.h"
#include "SoundInstance.h"
#include "UserProfile.h"
#include "Common.h"
#include "Particle.h"
#include "Image.h"
#include "MemoryImage.h"
#include "Res.h"

using namespace Sexy;

// 0x452d30: a run of more than one combo pays a bonus of its points times its length; it floats up as a text and a
// sample (one for each length, up to 8) plays. (The original takes a parameter that it never reads.)
void Board::PayComboBonus()
{
	if (mChainCount > 1 && (!mBonusRoundOn || mBonusWarpIn))
	{
		int aPoints = mChainPoints * mChainCount;
		AddScore(aPoints, true);

		FloatText* aText = new FloatText(StrFormat(GTC(0x5652d0), aPoints), FONT_POINTS, (float) mPointsTextX, (float) (mPointsTextY + 0x19), false);
		mFloatTexts.push_back(aText);

		if (mStageBricksCleared < mBricksToPass && !mClearCountRunning)
		{
			int aSound = mChainCount - 2 + SOUND_VCOMBO1;
			if (aSound >= SOUND_VCOMBO1 + 8)
				aSound = SOUND_VCOMBO1 + 7;
			mApp->PlaySample(aSound);
		}
	}
	mChainPoints = 0;
	mChainCount = 0;
	mLastChainCount = 0;
}

// 0x465290: the level is complete (the Board's Update calls it when the goal is reached). The best level of the mode is
// kept in the profile every fourth level. Everything the player had in hand or on the field goes to the bricks to
// delete. In the survival mode's bonus rounds (mLevelEndScreenKind is 1) the bonus round starts at once; otherwise the level-up text
// comes and the level-up phase is set up.
void Board::LevelComplete()
{
	if (mLevelCompleteBlocked)
		return;

	SoundInstance* aKlaxon = mApp->mLoopSounds[7];
	if (aKlaxon->IsPlaying())
		aKlaxon->Stop();

	if (mLevelNumber % 4 == 0 && mLevelNumber > 1)
	{
		int aLevelStart = 0;
		mApp->mUser->GetInt(mMode + 8, &aLevelStart);
		if ((aLevelStart / 4) * 4 <= mLevelNumber)
		{
			int aBest = 0;
			mApp->mUser->GetInt(mMode + 0x6b, &aBest);
			if (aBest < mScore)
				mApp->mUser->SetInt(mMode + 0x6b, mScore);
		}
	}

	mPerfectBannerState = 0;
	for (std::list<Brick*>::iterator anItr = mHeldBricks.begin(); anItr != mHeldBricks.end(); ++anItr)
		mDoomedBricks.insert(*anItr);
	mHeldBricks.clear();
	mStageTextEnabled = true;
	std::vector<MarkRec>().swap(mSuperchargeMarks);
	for (unsigned int i = 0; i < mFadedBricks.size(); i++)
	{
		mFadedBricks[i]->mGone = true;
		mDoomedBricks.insert(mFadedBricks[i]);
	}
	std::vector<Brick*>().swap(mFadedBricks);
	std::vector<DrawnText>().swap(mSpecialBannerTexts);
	if (mHandKindBrick != NULL)
	{
		mDoomedBricks.insert(mHandKindBrick);
		mHandKindBrick = NULL;
	}
	mBackdropTint = Color(100, 100, 100);
	mHeldBricksNotLanded = 0;
	mBrickometerDrain = 0.0f;
	mPickUpButtonDown = true;
	if (mSupaBannerStep != 0)
		mSupaBannerStep = 6;
	mLevelsCompleted++;
	gStateSaved = false;
	if (mChainCount > 1)
		PayComboBonus();

	if (mGameType == 1 && mLevelEndScreenKind == 1)
	{
		mApp->PlaySample(SOUND_BONUS_ROUND);
		mClicksBlocked = true;
		mBonusRoundOn = true;
		SetNextLevelScore();
		Fire(0x28, 1);
		mLevelUpPhase = false;
		mFreezeUpdatesLeft = 0;
		mDangerPulseAlpha = 0;
		mDangerPulseStep = 0;
		mPhase = 10;
		mPhaseDelay = PickSpawnDelay();
		mFlurryTimer = 400;
		mSirenAnim.SetStopped(true);
		mApp->PlaySong(0x29);
		mTallyBarWidth = 0;
		mTallyTitleAlpha = 0;
		mTallyBricksLeft = 0;
		mTallyBricksCounted = 0;
		mTallyPenalty = 0;
		mTallyPenaltyShown = 0;
		mTallyCleanBonus = 0;
		mTallyCleanBonusShown = 0;
		mTallyState = 0;
		mTallyPaid = 0;
		mClearCountRunning = false;
		mBricksRevealing = false;
		mBonusRoundLoaded = false;
		mBonusFadingOut = false;
		mBonusFadeAlpha = 0;
		mBannerTimeLeft = 0;
		mBonusRevealDelay = 200;
		mBonusTimeLeft = -1;
		mBonusRevealDone = false;
		mBonusWarpIn = true;
		mBonusWarpPhase = 1;
		mBonusWarpHold = 0;
		// (the original also reads the volume of the hyperspace loop and drops the number)
		mApp->mHyperspaceLoop->Play(true, false);
		mApp->mEngineLoop->Stop();
		SetOverlayHidden(false);
		mOverlay.SetFill(0xff, 0xff, 0xff, 0);
		for (unsigned int aRow = 0; aRow < mFieldBricks.size(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
			{
				if (mFieldBricks[aRow][aCol] != NULL)
					mFieldBricks[aRow][aCol]->StartShake();
			}
		}
		aKlaxon = mApp->mLoopSounds[7];
		if (aKlaxon->IsPlaying())
			aKlaxon->Stop();
		SaveGameState();
		return;
	}

	mLevelUpTextShown = true;
	delete mLevelUpText;
	mLevelUpText = new LevelUpText(IMAGE_LEVELUP_TEXT, 0);
	mLevelUpPhase = true;
	mUnusedIntClearedInLevelUp = 0;
	mApp->PlaySample(SOUND_LEVEL_COMPLETE);
	mApp->PlaySample(SOUND_WORDSWOOSH);
	mApp->PlaySong(0x29);
	StartLevelUp();
}

// A row of the table of upgrades (one table for each mode, rows by the group of four levels): the profile setting that
// counts the upgrade, its step and the text that announces it (the address of the original's literal).
struct UpgradeRec
{
	int				mSetting;
	int				mValue;
	unsigned int	mTextVA;
};

static const UpgradeRec kUpgrades[4][9] =
{
	{	// mode 0 (the original's table at 0x5a53e4)
		{61, 4, 0x56a098}, {51, 1, 0x569ce0}, {59, 1, 0x569c98}, {55, 1, 0x569c40}, {51, 2, 0x569bf0},
		{55, 2, 0x569c40}, {59, 2, 0x569c98}, {55, 3, 0x569c40}, {51, 3, 0x569ba0}
	},
	{	// mode 1 (0x5a55b4; row 0 is not used)
		{0, 0, 0}, {52, 1, 0x569f80}, {52, 2, 0x569f18}, {60, 1, 0x569ed0}, {52, 3, 0x569e68}, {52, 4, 0x569e00},
		{60, 2, 0x569ed0}, {52, 5, 0x569d98}, {52, 6, 0x569d30}
	},
	{	// mode 2 (0x5a52c4)
		{58, 3, 0x56a218}, {53, 1, 0x56a148}, {53, 2, 0x56a0e0}, {61, 1, 0x56a098}, {53, 3, 0x56a050},
		{61, 2, 0x56a098}, {53, 4, 0x569fe8}, {61, 3, 0x56a098}, {61, 4, 0x56a098}
	},
	{	// mode 3 (0x5a51a4; row 0 is not used)
		{0, 0, 0}, {54, 1, 0x56a2b8}, {62, 1, 0x56a270}, {58, 1, 0x56a218}, {62, 2, 0x56a270}, {58, 2, 0x56a218},
		{54, 2, 0x56a1c8}, {62, 3, 0x56a270}, {58, 3, 0x56a218}
	}
};

// 0x433130: what the upgrade screen of the next level offers (the argument is the level that is about to begin). 3: the
// level opens a mode (level 8 of mode 0 unlocks mode 3, level 16 of mode 1 unlocks mode 2) that is still locked;
// 4 or 2: every fourth level, while the profile's count of the upgrade is at its step (4 with the mode to unlock); 1: the
// levels before a multiple of four, unless the screen shown now was kind 1; else 0.
int Board::UpgradeScreenKind(int theLevel)
{
	int aUnlock = 0;
	if ((mMode == 0 && theLevel == 8 && !mApp->mUser->HasModeUnlocked(3)) ||
		(mMode == 1 && theLevel == 0x10 && !mApp->mUser->HasModeUnlocked(2)))
		aUnlock = 3;

	if (theLevel % 4 == 0 && theLevel >= 0 && theLevel <= 0x20)
	{
		const UpgradeRec& aRow = kUpgrades[(mMode >= 0 && mMode <= 2) ? mMode : 3][theLevel / 4];
		int aCount = 0;
		mApp->mUser->GetInt(aRow.mSetting, &aCount);
		if (aCount <= aRow.mValue)
			return aUnlock == 3 ? 4 : 2;
	}
	if (aUnlock == 3)
		return 3;
	if ((theLevel - 3) % 4 == 0 || theLevel == 3)
		return mLevelEndScreenKind == 1 ? 0 : 1;
	return 0;
}

// 0x4330a0: the profile setting that the upgrade of the level counts (the row of the level's group of four in the mode's
// table), -1 for a mode that has no table
int Board::GetUpgradeSetting()
{
	if (mMode < 0 || mMode > 3)
		return -1;
	return kUpgrades[mMode][mLevelNumber / 4].mSetting;
}

// 0x441850: the words of that row that name the upgrade (empty with no table)
std::string Board::GetUpgradeText()
{
	if (mMode < 0 || mMode > 3 || kUpgrades[mMode][mLevelNumber / 4].mTextVA == 0)
		return std::string();
	return GT(kUpgrades[mMode][mLevelNumber / 4].mTextVA);
}

// 0x43e220: a number with a comma after each three digits (counted from the right)
std::string Board::FormatNumber(int theNumber)
{
	std::string aString = StrFormat("%d", theNumber);
	int aCount = 1;
	int anIndex = (int) aString.length();
	while (--anIndex >= 0)
	{
		if (aCount % 3 == 0 && anIndex > 0)
			aString.insert(anIndex, ",");
		aCount++;
	}
	return aString;
}

// 0x434200: the picture of the upgrade screen is set up: the head and the arm, each with a frame animation
void Board::SetupUpgradeAnims()
{
	mRightArmX = 0x294;
	mRightArmY = 0x1a0;
	mRightArmStopX = 0x1cf;
	mRightArmStopY = 0x1a0;
	mRightHeadImage = IMAGE_UPGRADE_HEAD;
	mRightArmImage = IMAGE_UPGRADE_ARM;
	mRightHeadAnim.Reset();
	mRightHeadAnim.SetNumFrames(IMAGE_UPGRADE_HEAD->mNumCols);
	mRightHeadAnim.SetDelay(6);
	mRightHeadAnim.mHoldLast = false;
	mRightHeadAnim.SetRange(0x10, 0x15);

	mLeftArmX = -IMAGE_UPGRADE_ARM->mWidth;
	mLeftArmY = 0x1a0;
	mLeftArmStopX = 0x82;
	mLeftArmStopY = 0x1a0;
	mLeftArmImagesOwned = true;
	mLeftHeadAnim.Reset();
	mLeftHeadAnim.SetNumFrames(IMAGE_UPGRADE_HEAD->mNumCols);
	mLeftHeadAnim.SetDelay(6);
	mLeftHeadAnim.mHoldLast = false;
	mLeftHeadAnim.SetDirection(-1);
	mLeftHeadAnim.SetRange(0, 5);
}

// 0x432b50: the weapon levels of the player (profile settings 0x33 + mode, 0x3b + mode, and 0x37 or 0x3a) set how fast
// the weapon charges (mWeaponChargeStep, a factor) and how long it lasts (mWeaponDrainStep); the laser of mode 0 gets its animation. (The
// original computes the factor in x87 arithmetic.)
void Board::ApplyWeaponLevels()
{
	mApp->mUser->GetInt(mMode + 0x33, &mWeaponPower);
	mApp->mUser->GetInt(mMode + 0x3b, &mWeaponChargeLevel);
	if (mMode == 0)
		mApp->mUser->GetInt(0x37, &mWeaponDurationLevel);
	else if (mMode == 3)
		mApp->mUser->GetInt(0x3a, &mWeaponDurationLevel);
	else
		mWeaponDurationLevel = -1;

	mWeaponChargeMax = 18000.0f;
	if (mMode == 1)
	{
		if (mWeaponChargeLevel == 1)
			mWeaponChargeStep = 1.2f;
		else if (mWeaponChargeLevel == 2)
			mWeaponChargeStep = 1.7142857f;
		else if (mWeaponChargeLevel == 3)
			mWeaponChargeStep = 3.0f;
		mWeaponDrainStep = (float) ((double) 18000.0f * (double) 0.002f);
		return;
	}
	if (mMode == 2)
	{
		switch (mWeaponChargeLevel)
		{
		case 1:		mWeaponChargeStep = 1.2f;		break;
		case 2:		mWeaponChargeStep = 1.3333334f;	break;
		case 3:		mWeaponChargeStep = 1.5f;		break;
		case 4:		mWeaponChargeStep = 1.7142857f;	break;
		case 5:		mWeaponChargeStep = 2.0f;		break;
		}
		mWeaponDrainStep = (float) ((double) 18000.0f * (double) 0.002f);
		return;
	}
	if (mMode != 0 && mMode != 3)
		return;

	mWeaponChargeStep = (float) (18000.0 / (18000.0 - ((double) mWeaponChargeLevel - 1.0) * 3000.0));
	if (mMode == 0)
	{
		mLaserAnim.SetNumFrames(IMAGE_PLAYER1_LASER->mNumCols);
		mLaserAnim.SetDelay(2);
		mLaserAnim.mHoldLast = true;
		mLaserAnim.SetStopped(true);
	}
	switch (mWeaponDurationLevel)
	{
	case 1:		mWeaponDrainStep = 22.5f;			break;
	case 2:		mWeaponDrainStep = 20.0f;			break;
	case 3:		mWeaponDrainStep = 18.0f;			break;
	case 4:		mWeaponDrainStep = 16.363636f;		break;
	}
}

// 0x43b1b0: the five layers of the backdrop (the warp's layers: one picture, then a left and a right one for each of the
// next two) start at the top of their pictures
void Board::SetupWarpLayers()
{
	struct WarpLayer
	{
		Image*	mImage;
		float	mSpeed;
		float	mRangeStart;
	};
	WarpLayer aLayers[5] =
	{
		{IMAGE_WARP_LAYER_1, 0.01f, 229.0f},
		{IMAGE_WARP_LAYER_2_LEFT, 0.25f, 133.0f},
		{IMAGE_WARP_LAYER_2_RIGHT, 0.25f, 436.0f},
		{IMAGE_WARP_LAYER_3_LEFT, 1.0f, 0.0f},
		{IMAGE_WARP_LAYER_3_RIGHT, 1.0f, 517.0f}
	};
	for (int i = 0; i < 5; i++)
	{
		PlanetRec& aLayer = mPlanets[i];
		Image* anImage = aLayers[i].mImage;
		aLayer.mImage = anImage;
		aLayer.mPeriod = 1;
		aLayer.mSpeed = aLayers[i].mSpeed;
		aLayer.mF24 = 0.0f;
		aLayer.mF28 = -1.0f;
		aLayer.mRangeStart = aLayers[i].mRangeStart;
		aLayer.mCur[0] = 0.0f;
		aLayer.mCur[1] = (float) (anImage->mHeight - 0x1e0);
		aLayer.mCur[2] = (float) anImage->mWidth;
		aLayer.mCur[3] = 480.0f;
		aLayer.mNext[0] = 0.0f;
		aLayer.mNext[1] = 0.0f;
		aLayer.mNext[2] = (float) anImage->mWidth;
		aLayer.mNext[3] = 0.0f;
	}
}

// 0x45dd70: the level-up screen. The counters of the screen and of the warp start, the best level of the mode is kept,
// the upgrade of the level (every fourth, for the modes that have them) is counted in the profile, the weapon levels
// are read, and the panel of text (points, biggest combo, time, bonus, next rank) is made.
void Board::StartLevelUp()
{
	mDangerPulseAlpha = 0;
	mDangerPulseStep = 0;
	mWarpCharging = true;
	mWarpFlight = false;
	mWarpChargeAlpha = 0;
	mShipFrame = 0;
	mAtWarpGate = false;
	mOverlayFillColor = Color(0xff, 0xff, 0xff, 0);
	mOverlay.SetFill(0xff, 0xff, 0xff, 0);
	SetOverlayHidden(false);
	mWarpFrameInterval = 6;
	mWarpGlowStep = 0xc;
	mWarpGlowAlpha = 0;
	mWarpShakeX = 0;
	mWarpLightningAlpha = 0;
	unsigned int aRand = AstroRand();
	mWarpPictureLift = 0;
	mWarpShipY = -52.0f;
	mWarpLightningPeriod = (((int) aRand % 500000000) % 200) + 100;
	mBackdropTint = Color(100, 100, 100);
	mFrameDimming = 0;

	int aBestLevel = 0;
	mApp->mUser->GetInt(mMode + 8, &aBestLevel);
	if (aBestLevel < mLevelNumber)
		mApp->mUser->SetInt(mMode + 8, mLevelNumber);
	SetupUpgradeAnims();

	if (mLevelEndScreenKind == 2 || mLevelEndScreenKind == 4)
	{
		int aCount = 0;
		int aSetting = aBestLevel;
		if (mMode >= 0 && mMode <= 3)
			aSetting = kUpgrades[mMode][mLevelNumber / 4].mSetting;
		mApp->mUser->GetInt(aSetting, &aCount);
		aCount++;
		mApp->mUser->SetInt(aSetting, aCount);
		ApplyWeaponLevels();
	}

	if (mLevelEndScreenKind == 3 || mLevelEndScreenKind == 4)
	{
		int aValue = 0x13;
		bool aKnown = true;
		if (mMode == 0)
		{
			mApp->mUser->GetInt(0x30, &aValue);
			aValue = (((aValue & 4) != 0) ? 3 : 0) + 0x41;
			mApp->mUser->SetInt(0x30, aValue);
		}
		else if (mMode == 1)
		{
			mApp->mUser->GetInt(0x30, &aValue);
			aValue = (((aValue & 0x40) != 0) ? 0x2e : 0) + 0x16;
			mApp->mUser->SetInt(0x30, aValue);
		}
		else
			aKnown = false;
		if (aKnown)
		{
			aValue += 0x193;
			mApp->mUser->SetInt(0x32, aValue);
		}
	}

	mFreezeUpdatesLeft = 0;
	mSirenAnim.SetStopped(true);
	mGameOverDangerY = -500000000;
	mGameOverDangerHold = -500000000;
	mPhase = 10;
	mPhaseDelay = PickSpawnDelay();
	mFlurryTimer = 700;
	mWarpGateAnim.SetStopped(false);

	// the lines of the panel
	mLevelUpLetterIndex = 0;
	mLevelUpLineIndex = 0;
	std::vector<std::string>().swap(mLevelUpLines);
	mLevelUpLines.push_back(GT(0x565728) + FormatNumber(mPlayPoints));
	mLevelUpLines.push_back(StrFormat(GTC(0x565714), mBestChain));
	int aSeconds = (mRampTime / 100) % 0x3c;
	int aMinutes = mRampTime / 6000;
	if (aSeconds < 10)
		mLevelUpLines.push_back(StrFormat(GTC(0x5656f8), aMinutes, aSeconds));
	else
		mLevelUpLines.push_back(StrFormat(GTC(0x5656dc), aMinutes, aSeconds));

	mApp->GetRankName(mScore, &mRankNumber);
	delete mRankImage;
	mRankImage = mApp->MakeRankImage(mRankNumber, 0x50);

	if (mLevelEndScreenKind == 1)
		mLevelUpLines.push_back(StrFormat(GTC(0x5656c8), FormatNumber(mTallyCleanBonus - mTallyPenalty + mClearBonusWorth).c_str()));

	int aPointsToNext = mApp->PointsToNextRank(mScore);
	if (aPointsToNext == -1)
		mLevelUpLines.push_back(GT(0x56569c));
	else
		mLevelUpLines.push_back(StrFormat(GTC(0x5656b4), FormatNumber(aPointsToNext + mScore).c_str()));

	// the streaks of the backdrop take the colour of their kind
	for (unsigned int i = 0; i < mStreaks.size() && i < 100; i++)
	{
		StreakRec& aStreak = mStreaks[i];
		aStreak.mFlag28 = false;
		if (aStreak.mKind == 0)
			aStreak.mColor = Color(0x50, 0x50, 0x50);
		else if (aStreak.mKind == 1)
			aStreak.mColor = Color(0x96, 0x96, 0x96);
		else if (aStreak.mKind == 2)
			aStreak.mColor = Color(0xfa, 0xfa, 0xfa);
	}

	mLevelUpTypeInterval = 5;
	mLevelUpCursorInterval = 0x19;
	mLevelUpCursorShown = true;
	mLevelUpScanHeight = 0;
	mLevelUpScanInterval = 2;
	mLevelUpPanelHeight = 0xce;
	mLevelUpScanY = -206;
	mLevelUpScanMaxHeight = 10;
	mLevelUpPanelY = -206;
	SetupWarpLayers();
}
