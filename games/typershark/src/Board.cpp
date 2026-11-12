// Board.cpp: the Board's constructor and destructor and the Widget overrides (job B2): what it makes, what it adds to the
// widget manager, and the two small queries the status bar asks.
//
// Left out of the constructor (0x4544b0): the registry read at 0x454681 to 0x4546a1 that set Board +0x150 (the abyss depth
// limit of an unregistered copy). The link widget that AddedToManager makes (id 300, mContinueLink) is the "press space bar
// to continue" button of the treasure screen and only returns to the map.
//
// What the original leaves unset is set here (marked `port:`): mGameOverDialog, mDiverSpeed, mLessonManager, mStats,
// mTreasureImage, mOceanDepth and mContinueLink, which other functions set before they read them, and the two ints of the
// RatingPopup that the constructor does not write. The destructor frees what the original leaks: the timer at +0x58 (the
// ninth WSTimer; the original deletes the other eight) and the bonus creatures still in their list.

#include "Board.h"
#include "Enemy.h"
#include "Bubble.h"
#include "BonusText.h"
#include "BackgroundFish.h"
#include "HudBar.h"
#include "StatusBar.h"
#include "WSTimer.h"
#include "LessonManager.h"
#include "UserProfile.h"
#include "GameText.h"
#include "MTRand.h"
#include "Res.h"
#include "Image.h"
#include "MemoryImage.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include "HyperlinkWidget.h"
#include "ImageFont.h"
#include <math.h>
#include <string.h>

using namespace Sexy;

// port: the number the next board takes (Board::mBoardSerial, for the autosplit observer)
static unsigned int sBoardSerials = 0;

// 0x4544b0: the fields are set in the original's order. The Widget part comes first (Widget(), then the vftable), then the
// zeroed BgLayers and lists, the nine WSTimers are made one after another among the other fields, and the two random
// numbers of mSparkles are drawn from gGameRand in the middle of it.
Board::Board(WinWSApp* theApp)
{
	mBoardSerial = ++sBoardSerials;
	// the two BgLayers (0x170 to 0x1b0) are zeroed first
	int i = 2;
	char* aLayerBytes = (char*)mBgLayers;
	while (i != 0)
	{
		memset(aLayerBytes, 0, sizeof(BgLayer));
		aLayerBytes += sizeof(BgLayer);
		i--;
	}
	// mRecordsByType (0x1b0, an empty std::map), the lists, the strings mTyped, mBannerText and mBonusWord and mDepthMarkers
	// are empty (the std containers make them)
	mApp = theApp;

	mUnreadTimer = new WSTimer();
	mDrawCount = 0;
	mUpdateCount = 0;
	mAlwaysZero = 0;
	mAbyssBossDue = false;
	mAbyssBossIndex = -1;
	mFastCPU = mApp->mFastCPU;
	mDivePosMarked = 0.0;
	mFirstUpdate = true;
	mScore = 0;
	mLessonStepDepth = 0;
	mWavePhase1 = 0;
	mWavePhase2 = 0;
	mRating = new RatingPopup();		// port: zero-initialised; the original leaves mBonus (+0x2c) and mNeverWritten (+0x34) unset
	mDivePos = 1.0;
	mPausedTime = 0;
	mPaused = false;
	mBonusCreaturesKilled = 0;
	mOpenClam = NULL;
	mPairsFound = 0;
	mBossBeaten = false;
	mPauseTimer = new WSTimer();
	if (mApp->mLessonManager == NULL || !mApp->mLessonManager->mAbyssMode)
		mEmpInterval = 2500;
	else
		mEmpInterval = 1250;
	mScreenClear = true;
	mRating->mPercent = 0;
	mRating->mSmoothY = 0.0;		// the original stores two zero dwords at +0x08 and +0x0c
	mAlwaysFalse = false;
	mPlayer = new Player(mApp);
	mFadeAlpha = 255;
	mSonarStep = 0;
	mSonarTopRightImage = mApp->mImages[IMAGE_SONAR_SWEEP_0];
	mSonarBottomRightImage = mApp->mImages[IMAGE_SONAR_SWEEP2_0];
	mSonarBottomLeftImage = mApp->mSonarSweepImages[0];
	mSonarTopLeftImage = mApp->mSonarSweep2Images[0];
	// the table of sines (360 entries of sin(i * 1/pi)), made with fsin
	int aSinIndex = 0;
	do
	{
		mSinTable[aSinIndex] = sin((double)aSinIndex * 0.3183098861837907);
		aSinIndex++;
	} while (aSinIndex < 0x168);
	mWaveAmp = 4.0;
	mWaveGrowing = true;
	mRating->mActive = false;
	mTarget = NULL;
	mFloorY = mApp->mHeight - mApp->mImages[IMAGE_STATUSBARX]->mHeight;
	mRatingTimer = new WSTimer();
	mWaveLettersWrong = 0;
	mWaveLettersRight = 0;
	mLettersWrong = 0;
	mLettersRight = 0;
	mRating->mAlpha = 0;
	mRating->mX = (double)(mWidth / 2);
	mRating->mY = (double)mHeight;
	mRating->mStreak = 0;
	mRating->mImage = NULL;
	mRating->mPoints = 0;
	mTypeTimer = new WSTimer();
	mFlashAlpha = 0;
	mCreaturePassDone = false;
	mTypeTime = 0;
	mLevelTitleTimer = new WSTimer();
	mLevelTitleTimer->SetDisabled(true);
	mLevelTitleAlpha = 0;
	if (!mApp->mUser->mSeenTutorial)		// the original reads the current user here and stores 0 again
		mLevelTitleAlpha = 0;
	mSonarAlpha = 255;
	mBannerTimer = new WSTimer();
	mBannerLife = 0;
	mPenaltyTypoCount = 0;
	mPiranhaSpeed = 1.0;
	mPiranhaCount = 0;
	mHintAlpha = 255;
	mHintFading = false;
	mEmpDraining = false;
	mEmpUsedInWave = false;
	mPauseCount = 0;
	mLetterTyped = false;
	mBoss = NULL;
	mSparkles[1] = 0;
	mSparkles[0] = 0;
	mSparkles[2] = 0;
	mSparkles[4] = 0;
	mSparkles[3] = 0;
	mSparkles[5] = (int)(gGameRand->NextNoAssert() & 3) + 4;
	mSparkles[7] = 0;
	mSparkles[6] = 0;
	mSparkles[8] = (int)(gGameRand->NextNoAssert() & 3) + 8;
	mBonusLettersRight = 0;
	mBonusLettersWrong = 0;
	mBonusWordPos = 0;
	mBonusWordCount = 0;
	mAirFillRising = false;
	mBonusWordPoints = 0;
	mAirFillAlpha = 255;
	mTreasureTimer = new WSTimer();
	mTreasureTimer->SetDisabled(true);
	mAirGauge = 0.0;
	mTreasureAlpha = 255;
	mTreasureFading = false;
	mTreasureFrame = 0;
	mGameTimer = new WSTimer();
	mGameTimer->SetDisabled(true);
	mStartScore = mPlayer->mScore;
	mAccurateLevels = 0;
	mEmpTimer = new WSTimer();
	mOceanFloorY = (double)mHeight;
	mFarMountainsY = mOceanFloorY;
	mMidMountainsY = mOceanFloorY;
	mNearMountainsY = mOceanFloorY;
	mWaterImageHeight = mApp->mImages[IMAGE_WATER1]->mHeight;
	mSurfaceHeight = mApp->mImages[IMAGE_TOPBACKGROUND]->mHeight;
	((MemoryImage*)mApp->mImages[IMAGE_WATER1])->mD3DFlags = 1;		// the Direct3D image flag 1: the texture is made in the fewest pieces
	mHudBar = new HudBar(mPlayer, mApp);
	i = 0;
	do
	{
		mBgLayers[i].mY = (double)(mApp->mImages[IMAGE_WATER1]->GetHeight() * i);
		mBgLayers[i].mX = 0.0;
		i++;
	} while (i < 2);
	// the two Smoothers and the FrameSmoother (0x68 to 0x150) are zeroed last
	memset(mSmoothers, 0, sizeof(mSmoothers));
	memset(&mFrameSmoother, 0, sizeof(mFrameSmoother));

	// port: the original leaves these for later functions to set
	mGameOverDialog = NULL;
	mDiverSpeed = 0.0;
	mLessonManager = NULL;
	memset(&mStats, 0, sizeof(mStats));
	mTreasureImage = NULL;
	mOceanDepth = 0;
	mContinueLink = NULL;
}

// 0x451700 (the body of the destructor; the deleting destructor 0x454e10 calls it and then Widget's)
Board::~Board()
{
	if (mApp->mSoundInstance38 != NULL)		// port: the original has no test
		mApp->mSoundInstance38->Stop();
	if (mPlayer != NULL)
		delete mPlayer;

	for (unsigned int i = 0; i < mSharks.size(); i++)
	{
		if (mSharks[i] != NULL)
			delete mSharks[i];
	}
	mSharks.clear();

	for (unsigned int i = 0; i < mPiranhas.size(); i++)
	{
		if (mPiranhas[i] != NULL)
			delete mPiranhas[i];
	}
	mPiranhas.clear();

	for (unsigned int i = 0; i < mBubbles.size(); i++)
	{
		if (mBubbles[i] != NULL)
			delete mBubbles[i];
	}

	for (unsigned int i = 0; i < mClams.size(); i++)
	{
		if (mClams[i] != NULL)
			delete mClams[i];
	}
	mClams.clear();
	mBubbles.clear();

	// the labels of the depth ruler: the string is freed and then the marker
	for (unsigned int i = 0; i < mDepthMarkers.size(); i++)
	{
		if (mDepthMarkers[i] != NULL)
			delete mDepthMarkers[i];
	}
	mDepthMarkers.clear();

	delete mHudBar;
	mRating->~RatingPopup();
	delete mRating;

	for (unsigned int i = 0; i < mBackgroundFish.size(); i++)
	{
		if (mBackgroundFish[i] != NULL)
			delete mBackgroundFish[i];
	}

	for (unsigned int i = 0; i < mBonusTexts.size(); i++)
	{
		if (mBonusTexts[i] != NULL)
			delete mBonusTexts[i];
	}
	mBonusTexts.clear();

	// port: the original frees the list's memory but not the bonus creatures still in it, which nothing uses afterwards
	for (unsigned int i = 0; i < mBonusCreatures.size(); i++)
	{
		if (mBonusCreatures[i] != NULL)
			delete mBonusCreatures[i];
	}
	mBonusCreatures.clear();

	if (mContinueLink != NULL)
		delete mContinueLink;
	mContinueLink = NULL;
	if (mGameOverDialog != NULL)
		mApp->KillDialog(DIALOG_GAME_OVER);
	mGameOverDialog = NULL;
	if (mBoss != NULL)
		delete mBoss;

	// leak fix (the original leaks these: 0x45c140 and 0x40b360 take a creature off its list without deleting it): the retired
	// creatures are freed after the boss, the last that could use them, and after mTarget, which may point to one
	for (unsigned int i = 0; i < mRetired.size(); i++)
		delete mRetired[i];
	mRetired.clear();

	if (mPauseTimer != NULL)
		delete mPauseTimer;
	if (mRatingTimer != NULL)
		delete mRatingTimer;
	if (mTypeTimer != NULL)
		delete mTypeTimer;
	if (mLevelTitleTimer != NULL)
		delete mLevelTitleTimer;
	if (mBannerTimer != NULL)
		delete mBannerTimer;
	if (mTreasureTimer != NULL)
		delete mTreasureTimer;
	if (mGameTimer != NULL)
		delete mGameTimer;
	if (mEmpTimer != NULL)
		delete mEmpTimer;
	if (mUnreadTimer != NULL)		// port: the original never deletes this timer (a leak of a WSTimer, 0x64 bytes)
		delete mUnreadTimer;

	// the strings, the other lists and mRecordsByType (the enemy records belong to the lesson) go with the members
}

// 0x445eb0 slot 1: the link widget of the treasure screen is made hidden and disabled, then the nine timers join the manager
// (the manager updates them, which is what makes them count)
void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);

	mContinueLink = new HyperlinkWidget(300, mApp);
	mContinueLink->SetVisible(false);
	mContinueLink->mUnderlineSize = 1;
	mContinueLink->SetFont(mApp->mFontRockwell13);
	mContinueLink->mOverColor = Color(255, 255, 255);
	mContinueLink->mColor = Color(0, 255, 0);
	mContinueLink->mLabel = GT(0x0052f8d8);
	mContinueLink->SetDisabled(true);
	int aWidth = mContinueLink->mFont->StringWidth(mContinueLink->mLabel) + 10;
	int aHeight = mContinueLink->mFont->GetHeight() + 10;
	mContinueLink->Resize(mApp->mWidth / 2 - aWidth / 2, (mFloorY - mContinueLink->mFont->GetHeight()) - 15, aWidth, aHeight);
	theWidgetManager->AddWidget(mContinueLink);
	mContinueLink->mDoFinger = true;
	mContinueLink->SetDisabled(true);

	theWidgetManager->AddWidget(mPauseTimer);
	theWidgetManager->AddWidget(mRatingTimer);
	theWidgetManager->AddWidget(mTypeTimer);
	theWidgetManager->AddWidget(mLevelTitleTimer);
	theWidgetManager->AddWidget(mBannerTimer);
	theWidgetManager->AddWidget(mTreasureTimer);
	theWidgetManager->AddWidget(mGameTimer);
	theWidgetManager->AddWidget(mEmpTimer);
	theWidgetManager->AddWidget(mUnreadTimer);
}

// 0x43fbf0 slot 2
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mContinueLink != NULL)
		theWidgetManager->RemoveWidget(mContinueLink);
	theWidgetManager->RemoveWidget(mPauseTimer);
	theWidgetManager->RemoveWidget(mRatingTimer);
	theWidgetManager->RemoveWidget(mTypeTimer);
	theWidgetManager->RemoveWidget(mLevelTitleTimer);
	theWidgetManager->RemoveWidget(mBannerTimer);
	theWidgetManager->RemoveWidget(mTreasureTimer);
	theWidgetManager->RemoveWidget(mGameTimer);
	theWidgetManager->RemoveWidget(mEmpTimer);
	theWidgetManager->RemoveWidget(mUnreadTimer);
}

// 0x4401b0 slot 4: the link follows while the diver is in state 7; showing the board restarts a paused game timer, gives
// it the keyboard focus and starts the sound that plays while it is up (looping, kept by the app); hiding it stops that sound
void Board::SetVisible(bool isVisible)
{
	Widget::SetVisible(isVisible);
	if (mPlayer->mState == 7)
		mContinueLink->SetVisible(isVisible);
	if (isVisible)
	{
		if (mGameTimer->mPaused)
		{
			mGameTimer->SetDisabled(false);
			mGameTimer->Reset();
		}
		mApp->mWidgetManager->SetFocus(this);
		mApp->mSoundInstance38->Play(true, false);
		return;
	}
	mApp->mSoundInstance38->Stop();
}

// 0x440250 slot 10
void Board::SetDisabled(bool isDisabled)
{
	Widget::SetDisabled(isDisabled);
	if (mPlayer->mState == 7)
		mContinueLink->SetDisabled(isDisabled);
}

// 0x442520 slot 21 (the original's override does nothing)
void Board::KeyDown(KeyCode theKey)
{
}

// 0x440c20 slot 25: the pointer is the plain arrow; the status bar (the strip below the play field) sees the pointer enter
// while it is in the left part of that strip and the game is running, and see it leave otherwise
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->SetCursor(0);
	StatusBar* aStatusBar = mApp->mStatusBar;
	if (aStatusBar != NULL)
	{
		if (mFloorY < y && x <= 0x172 && !mPaused)
		{
			int aState = mPlayer->mState;
			if (aState != 4 && aState != 5 && aState != 7 && aState != 6 && aState != 8 && aState != 0xc)
			{
				aStatusBar->MouseEnter();
				return;
			}
		}
		aStatusBar->MouseLeave();
	}
}

// 0x440b70: false while the sonar sweep of diver state 7 has gone past its last picture
bool Board::IsHudActive()
{
	if (mPlayer->mState == 7 && mSonarStep > 0x10)
		return false;
	return true;
}

// 0x440cb0
bool Board::IsEmpTimerPaused()
{
	return mEmpTimer->mPaused;
}
