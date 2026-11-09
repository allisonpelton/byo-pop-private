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

// 0x4544b0: the fields are set in the original's order. The Widget part comes first (Widget(), then the vftable), then the
// zeroed BgLayers and lists, the nine WSTimers are made one after another among the other fields, and the two random
// numbers of mField0ec0 are drawn from gGameRand in the middle of it.
Board::Board(WinWSApp* theApp)
{
	// the two BgLayers (0x170 to 0x1b0) are zeroed first
	int i = 2;
	char* aLayerBytes = (char*)mBgLayers;
	while (i != 0)
	{
		memset(aLayerBytes, 0, sizeof(BgLayer));
		aLayerBytes += sizeof(BgLayer);
		i--;
	}
	// mRecordsByType (0x1b0, an empty std::map), the lists, the strings mTyped, mBannerText and mString0f10 and mDepthMarkers
	// are empty (the std containers make them)
	mApp = theApp;

	mTimer58 = new WSTimer();
	mDrawCount = 0;
	mUpdateCount = 0;
	mField060 = 0;
	mFlag0d28 = false;
	mField0d2c = -1;
	mFastCPU = mApp->mFastCPU;
	mDivePosMarked = 0.0;
	mFirstUpdate = true;
	mScore = 0;
	mField1c0 = 0;
	mWavePhase1 = 0;
	mWavePhase2 = 0;
	mRating = new RatingPopup();		// port: zero-initialised; the original leaves mBonus (+0x2c) and mField34 (+0x34) unset
	mDivePos = 1.0;
	mPausedTime = 0;
	mPaused = false;
	mField0d64 = 0;
	mOpenClam = NULL;
	mPairsFound = 0;
	mFlag0ebc = false;
	mPauseTimer = new WSTimer();
	if (mApp->mLessonManager == NULL || !mApp->mLessonManager->mAbyssMode)
		mEmpInterval = 2500;
	else
		mEmpInterval = 1250;
	mFlag0f80 = true;
	mRating->mPercent = 0;
	mRating->mSmoothY = 0.0;		// the original stores two zero dwords at +0x08 and +0x0c
	mFlag0d60 = false;
	mPlayer = new Player(mApp);
	mFadeAlpha = 255;
	mField0ee4 = 0;
	mImage0ee8 = mApp->mImages[IMAGE_SONAR_SWEEP_0];
	mImage0eec = mApp->mImages[IMAGE_SONAR_SWEEP2_0];
	mImage0ef0 = mApp->mSonarSweepImages[0];
	mImage0ef4 = mApp->mSonarSweep2Images[0];
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
	mField0f64 = 0;
	mField0f60 = 0;
	mField0f6c = 0;
	mField0f68 = 0;
	mRating->mAlpha = 0;
	mRating->mX = (double)(mWidth / 2);
	mRating->mY = (double)mHeight;
	mRating->mStreak = 0;
	mRating->mImage = NULL;
	mRating->mPoints = 0;
	mTypeTimer = new WSTimer();
	mFlashAlpha = 0;
	mFlag0efc = false;
	mTypeTime = 0;
	mTimer0e50 = new WSTimer();
	mTimer0e50->SetDisabled(true);
	mField0e54 = 0;
	if (!mApp->mUser->mFlag1dc)		// the original reads the current user here and stores 0 again
		mField0e54 = 0;
	mField0ef8 = 255;
	mBannerTimer = new WSTimer();
	mBannerLife = 0;
	mField0e94 = 0;
	mPiranhaSpeed = 1.0;
	mPiranhaCount = 0;
	mField0ea4 = 255;
	mFlag0ea8 = false;
	mFlag0eb4 = false;
	mFlag0eb5 = false;
	mPauseCount = 0;
	mFlag0d38 = false;
	mBoss = NULL;
	mField0ec0[1] = 0;
	mField0ec0[0] = 0;
	mField0ec0[2] = 0;
	mField0ec0[4] = 0;
	mField0ec0[3] = 0;
	mField0ec0[5] = (int)(gGameRand->NextNoAssert() & 3) + 4;
	mField0ec0[7] = 0;
	mField0ec0[6] = 0;
	mField0ec0[8] = (int)(gGameRand->NextNoAssert() & 3) + 8;
	mField0f70 = 0;
	mField0f74 = 0;
	mField0f2c = 0;
	mField0f30 = 0;
	mFlag0f40 = false;
	mField0f0c = 0;
	mField0f44 = 255;
	mTimer0f00 = new WSTimer();
	mTimer0f00->SetDisabled(true);
	mField0f38 = 0.0;
	mField0f04 = 255;
	mFlag0f08 = false;
	mField0f48 = 0;
	mGameTimer = new WSTimer();
	mGameTimer->SetDisabled(true);
	mField0e90 = mPlayer->mScore;
	mField0e8c = 0;
	mEmpTimer = new WSTimer();
	mField0e10 = (double)mHeight;
	mField0d20 = mField0e10;
	mField0d18 = mField0e10;
	mField0d10 = mField0e10;
	mField0f84 = mApp->mImages[IMAGE_WATER1]->mHeight;
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
	if (mTimer0e50 != NULL)
		delete mTimer0e50;
	if (mBannerTimer != NULL)
		delete mBannerTimer;
	if (mTimer0f00 != NULL)
		delete mTimer0f00;
	if (mGameTimer != NULL)
		delete mGameTimer;
	if (mEmpTimer != NULL)
		delete mEmpTimer;
	if (mTimer58 != NULL)		// port: the original never deletes this timer (a leak of a WSTimer, 0x64 bytes)
		delete mTimer58;

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
	theWidgetManager->AddWidget(mTimer0e50);
	theWidgetManager->AddWidget(mBannerTimer);
	theWidgetManager->AddWidget(mTimer0f00);
	theWidgetManager->AddWidget(mGameTimer);
	theWidgetManager->AddWidget(mEmpTimer);
	theWidgetManager->AddWidget(mTimer58);
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
	theWidgetManager->RemoveWidget(mTimer0e50);
	theWidgetManager->RemoveWidget(mBannerTimer);
	theWidgetManager->RemoveWidget(mTimer0f00);
	theWidgetManager->RemoveWidget(mGameTimer);
	theWidgetManager->RemoveWidget(mEmpTimer);
	theWidgetManager->RemoveWidget(mTimer58);
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
	if (mPlayer->mState == 7 && mField0ee4 > 0x10)
		return false;
	return true;
}

// 0x440cb0
bool Board::IsEmpTimerPaused()
{
	return mEmpTimer->mPaused;
}
