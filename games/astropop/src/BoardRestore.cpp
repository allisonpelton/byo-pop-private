// Board: the saved game (0x467af0). The Board is made as for a new game, then the levels the player had passed are played
// through (AdvanceLevel), the field is rebuilt from the table the profile keeps (mScores: the kind of each brick and a
// value for it) and the counters are read back from the profile's settings 0x56 .. 0x66. A game saved between two levels
// (setting 0x5b or 0x58 above nothing) comes back at the warp screen instead.

#include "Board.h"
#include "GameText.h"
#include "AstroPopApp.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "Font.h"
#include "Image.h"
#include "Res.h"
#include "SoundInstance.h"
#include "UserProfile.h"

using namespace Sexy;

// 0x467af0
void Board::RestoreGame(int theScore, int theSetting54, int theSetting55, float theSetting52, float theSetting50, int theSavedLevel)
{
	mWeaponChargeMax = 25600.0f;
	mScoreMultiplier = theSetting52;
	mStageTextEnabled = false;
	ApplyWeaponLevels();
	mApp->mEngineLoop->SetVolume(1.0);
	mApp->mHyperspaceLoop->SetVolume(1.0);

	// the levels the player had passed
	if (theSavedLevel > 1)
	{
		int aLeft = theSavedLevel - 1;
		do
		{
			mStageNumber++;
			AdvanceLevel(false);
			aLeft--;
		}
		while (aLeft != 0);
	}
	mLevelNumber = theSavedLevel / 2 + 1;

	// the picture of the stage and the hint on the screen start without a fade
	unsigned int aStageCount = mStageHintPanels.empty() ? 0 : (unsigned int) mStageHintPanels.size();
	if ((unsigned int) mStageNumber < aStageCount)
	{
		StageBg* aBg = mStageHintPanels[mStageNumber];
		aBg->mPanelAlphaStep = 0;
		aBg->mPanelAlpha = 0xff;
		aBg->mOverPicAlphaStep = 0;
		aBg->mPicAlphaStep = 0;
		aBg->mPicAlpha = 0xff;
		aBg->mOverPicAlpha = 0;
	}
	if (!mHintSpots.empty() && mHintSpots[0]->mActive)
	{
		StageBg* aBg = mHintSpots[0]->mBg;
		aBg->mPicAlpha = 0xff;
		aBg->mPanelAlpha = 0xff;
		aBg->mCrossfadeTimer = 0;
		aBg->mPanelAlphaStep = 0;
		aBg->mPicAlphaStep = 0;
		aBg->mOverPicAlphaStep = 0;
		aBg->mOverPicAlpha = 0;
		aBg->mFadedOut = false;
	}
	if (mWeaponCharge == mWeaponChargeMax)
	{
		mMeterFullAnim.Reset();
		mMeterFullAnim.SetStopped(false);
	}

	mLevelsCompleted = theSetting55;
	ClearField();
	LoadLevel(false);
	mScore = theScore;
	mScoreShown = theScore;
	mWeaponCharge = theSetting50;
	mPlayTime = theSetting54;
	UserProfile* aUser = mApp->mUser;
	aUser->GetInt(0x5c, &mPlayPoints);
	aUser->GetInt(0x60, &mRampTime);
	aUser->GetInt(0x61, &mScoreAtStageStart);
	aUser->GetInt(0x62, &mBonusRoundPoints);
	int aBonusLevel = 0;
	aUser->GetInt(0x5b, &aBonusLevel);
	int aBetweenLevels = 0;
	aUser->GetInt(0x58, &aBetweenLevels);

	if (aBonusLevel == 0)
	{
		if (aBetweenLevels == 0)
		{
			// a game saved in the middle of a level
			mStageBricksCleared = 0;
			aUser->GetInt(0x56, &mStageBricksCleared);
			int aBrokenBricks = 0;
			mBrickometerFill = (float) mStageBricksCleared / (float) mBricksToPass;
			aUser->GetInt(0x57, &aBrokenBricks);
			if (aBrokenBricks < 1)
			{
				mLevelEndScreenKind = 0;
				aUser->GetInt(99, &mLevelEndScreenKind);
				mNextLevelScreenKind = UpgradeScreenKind(mLevelNumber + 1);
			}
			else
			{
				// ... or in a bonus round, with the bricks it has left
				mLevelEndScreenKind = 0;
				aUser->GetInt(99, &mLevelEndScreenKind);
				mNextLevelScreenKind = UpgradeScreenKind(mLevelNumber + 1);
				mBonusTimeLeft = aBrokenBricks;
				mBonusRoundOn = true;
				mLevelUpPhase = false;
				aUser->GetInt(0x5d, &mBestChain);
				SetNextLevelScore();
				mTallyBarWidth = 0;
				mTallyTitleAlpha = 0;
				mTallyBricksLeft = 0;
				mTallyBricksCounted = 0;
				mTallyPenalty = 0;
				mTallyPenaltyShown = 0;
				mTallyCleanBonusShown = 0;
				mTallyCleanBonus = 0;
				mTallyState = 0;
				mTallyPaid = 0;
				mClearCountRunning = false;
				mBonusFadingOut = false;
				mBonusRoundLoaded = true;
				mBonusRevealDone = false;
				mBonusFadeAlpha = 0;
				mBannerTimeLeft = 0;
				mBonusRevealDelay = 200;
				std::string aBanner = GT(0x005647cc);
				mBonusWarpTextX = 0x182 - GetFontById(FONT_DIALOG_BOX_BTN_ID)->StringWidth(aBanner) / 2;
				mPlayStarted = true;
				mStartedFromCheckpoint = false;
				mBonusWarpIn = false;
				mBonusWarpPhase = 0;
				mBonusWarpHold = 0;
				mApp->mHyperspaceLoop->Play(true, false);
				mApp->mEngineLoop->Stop();
				mWarpShakeX = 0;
				int aStreakIndex = 0;
				do
				{
					StreakRec* aStreak = &mStreaks[aStreakIndex];
					aStreak->mFlag28 = false;
					if (aStreak->mKind == 0)
						aStreak->mColor = Color(0, 0, 0xfa);
					else if (aStreak->mKind == 1)
						aStreak->mColor = Color(0xfa, 0, 0);
					else if (aStreak->mKind == 2)
						aStreak->mColor = Color(0xfa, 0xfa, 0);
					aStreakIndex++;
				}
				while (aStreakIndex < 100);
			}

			// the bricks of the field: the kind and the value of each are in the profile
			int aRow = 0;
			int aY = 4;
			while (true)
			{
				if (mFieldBricks.empty() || (unsigned int) mFieldBricks.size() <= (unsigned int) aRow)
					break;
				int aCol = 0;
				int aX = 0xa6;
				do
				{
					int aKind = aUser->mScores[aRow][aCol].mValue;
					int aValue = aUser->mScores[aRow][aCol].mAux;
					Brick* aBrick = NULL;
					switch (aKind)
					{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
						if (aValue == 500000000)
							aBrick = CreateKillAll(aKind);
						else
							aBrick = CreateBrick(aKind);
						break;
					case 6:
					case 7:
						aBrick = CreateSpecialBrick(aKind);
						ShowHint(aKind, aCol, aRow, true);
						break;
					case 8:
						aBrick = CreateFuelCell(500, 5);
						ShowHint(aKind, aCol, aRow, true);
						break;
					case 9:
						aBrick = CreateSpecialBrick(9);
						ShowHint(aKind, aCol, aRow, true);
						if (aUser->mScores[aRow][aCol].mAux == 1)
							static_cast<SpecialBrick*>(aBrick)->Crack(false);
						break;
					case 10:
						aBrick = CreatePowerBrick(10);
						ShowHint(aKind, aCol, aRow, true);
						break;
					case 0xb:
						aBrick = CreateSpecialBrick(0xb);
						ShowHint(aKind, aCol, aRow, true);
						break;
					case 0xc:
						aBrick = CreateMutator();
						FireTo(aBrick, 0x28, 1);
						ShowHint(aKind, aCol, aRow, true);
						break;
					case 0xd:
					case 0xe:
						aBrick = CreatePowerBrick(aKind);
						ShowHint(aKind, aCol, aRow, true);
						break;
					default:
						break;
					}
					if (aBrick != NULL)
					{
						aBrick->SetParticleX((float) aX);
						int aBrickY = aY;
						if (4 < aY)
							aBrickY = aY + 5;
						aBrick->SetParticleY((float) aBrickY);
					}
					aX += 0x2c;
					mFieldBricks[aRow][aCol] = aBrick;
					aCol++;
				}
				while (aX < 0x25e);
				aRow++;
				aY += 0x19;
			}

			// the marks (the kind of brick the level asks for)
			MarkRec aMark;
			aMark.mType = -1;
			aMark.mCount = -1;
			aMark.mActive = false;
			if (aUser->GetInt(0x59, &aMark.mType) && aMark.mType > -1)
			{
				aUser->GetInt(0x5a, &aMark.mCount);
				mSuperchargeMarks.push_back(aMark);
			}
			if (!mSuperchargeMarks.empty())
			{
				Fire(0x20, mSuperchargeMarks[0].mType);
				int aMarkType = mSuperchargeMarks[0].mType;
				if (GetMarkCount(aMarkType) < 0xff)
					Fire(0x25, GetMarkCount(aMarkType));
			}
			int aWaiting = 0;
			if (aUser->GetInt(0x65, &aWaiting))
			{
				BeginClearWait();
				mFreezeUpdatesLeft = aWaiting;
			}
			int aValue198c = mShipX;
			aUser->GetInt(0x66, &aValue198c);
			mShipX = aValue198c;
			mSirenOffDelay = -1;
			CheckStackHeight();
			CheckShipColumn();
		}
		else if (0 < aBetweenLevels)
		{
			// a game saved at the warp screen
			mStreaksFadingOut = false;
			mWarpActive = true;
			mAtWarpGate = true;
			mWarpLightningAlpha = 0;
			unsigned int aDraw = AstroRand();
			mWarpShipY = 423.0f; // 0x43d38000 at 0x4687f3
			mWarpLightningPeriod = (((int) aDraw % 500000000) % 200) + 100;
			mBackdropTint = Color(100, 100, 100);
			mPhase = 10;
			mPhaseDelay = PickSpawnDelay();
			mFlurryTimer = 700;
			mWarpGateAnim.SetStopped(false);
			mFastGateGlow = 0;
			mSlowGateGlow = 0;
			mEasyGateGlow = 0;
			mLevelEndScreenKind = 0;
			aUser->GetInt(99, &mLevelEndScreenKind);
			mNextLevelScreenKind = UpgradeScreenKind(mLevelNumber + 1);
			if (mBestLevelReached < 4)
				mShipX = IMAGE_SLOW_WARP_GATE->mWidth / 2 - 0x1a + mSlowGateX;
			else
				mShipX = 0x159;
			aUser->GetInt(0x5c, &mPlayPoints);
			aUser->GetInt(0x5d, &mBestChain);
			int aBonus = 0;
			aUser->GetInt(0x5f, &aBonus);

			// the panel of what the player has got so far
			std::vector<std::string>().swap(mLevelUpLines);
			mLevelUpLines.push_back(GT(0x00565728) + FormatNumber(mPlayPoints));
			mLevelUpLines.push_back(StrFormat(GTC(0x00565714), mBestChain));
			int aSeconds = (mRampTime / 100) % 0x3c;
			if (aSeconds < 10)
				mLevelUpLines.push_back(StrFormat(GTC(0x005656f8), mRampTime / 6000, aSeconds));
			else
				mLevelUpLines.push_back(StrFormat(GTC(0x005656dc), mRampTime / 6000, aSeconds));
			std::string aRankName = mApp->GetRankName(mScore, &mRankNumber);
			if (mRankImage != NULL)
				delete mRankImage;
			mRankImage = mApp->MakeRankImage(mRankNumber, 0x50);
			if (mLevelEndScreenKind == 1)
			{
				mScore = mPlayPoints + mBonusRoundPoints + mScoreAtStageStart + aBonus;
				mScoreShown = mScore;
				mLevelUpLines.push_back(StrFormat(GTC(0x005660ac), FormatNumber(mScore).c_str()));
			}
			int aToNext = mApp->PointsToNextRank(mScore);
			if (aToNext == -1)
				mLevelUpLines.push_back(GT(0x0056569c));
			else
				mLevelUpLines.push_back(StrFormat(GTC(0x005656b4), FormatNumber(aToNext + mScore).c_str()));
			mShipInWarp = true;
			mLevelUpTypeInterval = 5;
			mLevelUpCursorInterval = 0x19;
			mLevelUpCursorShown = true;
			mLevelUpScanHeight = 0;
			mLevelUpScanInterval = 2;
			mLevelUpPanelHeight = 0xce;
			mLevelUpScanY = -0xce;
			mLevelUpScanMaxHeight = 10;
			mLevelUpPanelY = 0;
			mLevelUpLetterIndex = 0;
			mLevelUpLineIndex = (int) mLevelUpLines.size();
			SetupWarpLayers();
			int aStreakIndex = 0;
			do
			{
				StreakRec* aStreak = &mStreaks[aStreakIndex];
				aStreak->mMargin = 0x1e0;
				aStreak->mSpeed = aStreak->mSpeed + 4.7999997f;
				aStreakIndex++;
			}
			while (aStreakIndex < 100);
			mBrickometerFill = 1.0f;
		}
	}
	else if (0 < aBonusLevel && aBetweenLevels == 0)
	{
		// a game saved in the bonus round of a level
		mBonusRoundOn = true;
		mLevelUpPhase = false;
		mTallyBarWidth = 0;
		mTallyTitleAlpha = 0;
		mTallyBricksLeft = 0;
		mTallyBricksCounted = 0;
		mTallyPenalty = 0;
		mTallyPenaltyShown = 0;
		mTallyCleanBonusShown = 0;
		mTallyCleanBonus = 0;
		mTallyState = 0;
		mTallyPaid = 0;
		mClicksBlocked = true;
		mClearCountRunning = false;
		mBonusRoundLoaded = false;
		mBonusFadingOut = false;
		mBonusFadeAlpha = 0;
		mBannerTimeLeft = 0;
		mBonusRevealDelay = 200;
		SetNextLevelScore();
		mBonusTimeLeft = -1;
		mBonusRevealDone = false;
		std::string aBanner = GT(0x005647cc);
		mBonusWarpTextX = 0x182 - GetFontById(FONT_DIALOG_BOX_BTN_ID)->StringWidth(aBanner) / 2;
		mPlayStarted = true;
		mStartedFromCheckpoint = false;
		mBonusWarpIn = false;
		mBonusWarpPhase = 0;
		mBonusWarpHold = 0;
		mApp->mHyperspaceLoop->Play(true, false);
		mApp->mEngineLoop->Stop();
		mWarpShakeX = 0;
		int aStreakIndex = 0;
		do
		{
			StreakRec* aStreak = &mStreaks[aStreakIndex];
			aStreak->mFlag28 = false;
			if (aStreak->mKind == 0)
				aStreak->mColor = Color(0, 0, 0xfa);
			else if (aStreak->mKind == 1)
				aStreak->mColor = Color(0xfa, 0, 0);
			else if (aStreak->mKind == 2)
				aStreak->mColor = Color(0xfa, 0xfa, 0);
			aStreakIndex++;
		}
		while (aStreakIndex < 100);
		mBricksRevealing = true;
		int aStage = -1;
		aUser->GetInt(0x3f, &aStage);
		if (aStage < 1)
			aStage = -1;
		StartBonusRound(aStage);
		mLevelEndScreenKind = 0;
		aUser->GetInt(99, &mLevelEndScreenKind);
		mNextLevelScreenKind = UpgradeScreenKind(mLevelNumber + 1);
		MarkRec aMark;
		aMark.mType = -1;
		aMark.mCount = -1;
		aMark.mActive = false;
		if (aUser->GetInt(0x59, &aMark.mType) && aMark.mType > -1)
		{
			aUser->GetInt(0x5a, &aMark.mCount);
			mSuperchargeMarks.push_back(aMark);
		}
		if (!mSuperchargeMarks.empty())
		{
			Fire(0x20, mSuperchargeMarks[0].mType);
			int aMarkType = mSuperchargeMarks[0].mType;
			if (GetMarkCount(aMarkType) < 0xff)
				Fire(0x25, GetMarkCount(aMarkType));
		}
		int aWaiting = 0;
		if (aUser->GetInt(0x65, &aWaiting))
		{
			BeginClearWait();
			mFreezeUpdatesLeft = aWaiting;
		}
		int aValue198c = mShipX;
		aUser->GetInt(0x66, &aValue198c);
		mShipX = aValue198c;
	}
	mTallyBricksCounted = 0;
	aUser->GetInt(100, &mTallyBricksCounted);
	mUpgradeDialogDone = true;
	if (mWeaponChargeMax <= mWeaponCharge)
		mSupaButtonX = 0;
}
