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
	m1bdc = 25600.0f;
	m940 = theSetting52;
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
			mField193C++;
			AdvanceLevel(false);
			aLeft--;
		}
		while (aLeft != 0);
	}
	m1940 = theSavedLevel / 2 + 1;

	// the picture of the stage and the hint on the screen start without a fade
	unsigned int aStageCount = mStageHintPanels.empty() ? 0 : (unsigned int) mStageHintPanels.size();
	if ((unsigned int) mField193C < aStageCount)
	{
		StageBg* aBg = mStageHintPanels[mField193C];
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
	if (m1bd8 == m1bdc)
	{
		mMeterFullAnim.Reset();
		mMeterFullAnim.SetStopped(false);
	}

	m1b1c = theSetting55;
	ClearField();
	LoadLevel(false);
	mScore = theScore;
	m1970 = theScore;
	m1bd8 = theSetting50;
	m19b8 = theSetting54;
	UserProfile* aUser = mApp->mUser;
	aUser->GetInt(0x5c, &mPlayPoints);
	aUser->GetInt(0x60, &m19b4);
	aUser->GetInt(0x61, &m1978);
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
			m1a14 = 0;
			aUser->GetInt(0x56, &m1a14);
			int aBrokenBricks = 0;
			mBrickometerFill = (float) m1a14 / (float) m1a18;
			aUser->GetInt(0x57, &aBrokenBricks);
			if (aBrokenBricks < 1)
			{
				m1b2c = 0;
				aUser->GetInt(99, &m1b2c);
				m1b28 = UpgradeScreenKind(m1940 + 1);
			}
			else
			{
				// ... or in a bonus round, with the bricks it has left
				m1b2c = 0;
				aUser->GetInt(99, &m1b2c);
				m1b28 = UpgradeScreenKind(m1940 + 1);
				m1b70 = aBrokenBricks;
				mBonusRoundOn = true;
				mLevelUpPhase = false;
				aUser->GetInt(0x5d, &mBestChain);
				SetNextLevelScore();
				m1b88 = 0;
				m1b8c = 0;
				m1b94 = 0;
				m1b98 = 0;
				m1b9c = 0;
				m1ba0 = 0;
				m1ba8 = 0;
				m1ba4 = 0;
				m1bb0 = 0;
				m1bac = 0;
				mClearCountRunning = false;
				mBonusFadingOut = false;
				mBonusRoundLoaded = true;
				mBonusRevealDone = false;
				m1b74 = 0;
				m1b7c = 0;
				m1b78 = 200;
				std::string aBanner = GT(0x005647cc);
				m1bd0 = 0x182 - GetFontById(FONT_DIALOG_BOX_BTN_ID)->StringWidth(aBanner) / 2;
				mPlayStarted = true;
				mStartedFromCheckpoint = false;
				mBonusWarpIn = false;
				m1bc0 = 0;
				m1bc4 = 0;
				mApp->mHyperspaceLoop->Play(true, false);
				mApp->mEngineLoop->Stop();
				m199c = 0;
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
				m19fc = aWaiting;
			}
			int aValue198c = m198c;
			aUser->GetInt(0x66, &aValue198c);
			m198c = aValue198c;
			m1a7c = -1;
			CheckStackHeight();
			CheckShipColumn();
		}
		else if (0 < aBetweenLevels)
		{
			// a game saved at the warp screen
			mStreaksFadingOut = false;
			mWarpActive = true;
			mAtWarpGate = true;
			m19f0 = 0;
			unsigned int aDraw = AstroRand();
			m938 = 423.0f; // 0x43d38000 at 0x4687f3
			m19f4 = (((int) aDraw % 500000000) % 200) + 100;
			mBackdropTint = Color(100, 100, 100);
			m1ab8 = 10;
			m1ac0 = PickSpawnDelay();
			m1abc = 700;
			mWarpGateAnim.SetStopped(false);
			m19ec = 0;
			m19e8 = 0;
			m19e4 = 0;
			m1b2c = 0;
			aUser->GetInt(99, &m1b2c);
			m1b28 = UpgradeScreenKind(m1940 + 1);
			if (m1a0c < 4)
				m198c = IMAGE_SLOW_WARP_GATE->mWidth / 2 - 0x1a + m1a70;
			else
				m198c = 0x159;
			aUser->GetInt(0x5c, &mPlayPoints);
			aUser->GetInt(0x5d, &mBestChain);
			int aBonus = 0;
			aUser->GetInt(0x5f, &aBonus);

			// the panel of what the player has got so far
			std::vector<std::string>().swap(mLevelUpLines);
			mLevelUpLines.push_back(GT(0x00565728) + FormatNumber(mPlayPoints));
			mLevelUpLines.push_back(StrFormat(GTC(0x00565714), mBestChain));
			int aSeconds = (m19b4 / 100) % 0x3c;
			if (aSeconds < 10)
				mLevelUpLines.push_back(StrFormat(GTC(0x005656f8), m19b4 / 6000, aSeconds));
			else
				mLevelUpLines.push_back(StrFormat(GTC(0x005656dc), m19b4 / 6000, aSeconds));
			std::string aRankName = mApp->GetRankName(mScore, &m1bd4);
			if (mRankImage != NULL)
				delete mRankImage;
			mRankImage = mApp->MakeRankImage(m1bd4, 0x50);
			if (m1b2c == 1)
			{
				mScore = mPlayPoints + mBonusRoundPoints + m1978 + aBonus;
				m1970 = mScore;
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
		m1b88 = 0;
		m1b8c = 0;
		m1b94 = 0;
		m1b98 = 0;
		m1b9c = 0;
		m1ba0 = 0;
		m1ba8 = 0;
		m1ba4 = 0;
		m1bb0 = 0;
		m1bac = 0;
		mClicksBlocked = true;
		mClearCountRunning = false;
		mBonusRoundLoaded = false;
		mBonusFadingOut = false;
		m1b74 = 0;
		m1b7c = 0;
		m1b78 = 200;
		SetNextLevelScore();
		m1b70 = -1;
		mBonusRevealDone = false;
		std::string aBanner = GT(0x005647cc);
		m1bd0 = 0x182 - GetFontById(FONT_DIALOG_BOX_BTN_ID)->StringWidth(aBanner) / 2;
		mPlayStarted = true;
		mStartedFromCheckpoint = false;
		mBonusWarpIn = false;
		m1bc0 = 0;
		m1bc4 = 0;
		mApp->mHyperspaceLoop->Play(true, false);
		mApp->mEngineLoop->Stop();
		m199c = 0;
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
		m1b2c = 0;
		aUser->GetInt(99, &m1b2c);
		m1b28 = UpgradeScreenKind(m1940 + 1);
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
			m19fc = aWaiting;
		}
		int aValue198c = m198c;
		aUser->GetInt(0x66, &aValue198c);
		m198c = aValue198c;
	}
	m1b98 = 0;
	aUser->GetInt(100, &m1b98);
	mUpgradeDialogDone = true;
	if (m1bdc <= m1bd8)
		m1b48 = 0;
}
