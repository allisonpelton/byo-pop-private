// BoardCtor.cpp: making and unmaking the board, and its buttons (Sexy::Board::Board 0x5ad581, ~Board 0x5a5abe,
// ButtonDepress 0x5ad35c).

#include "Board.h"
#include <cstring>
#include "WinDMApp.h"
#include "Profile.h"
#include "Res.h"
#include "GalaxyParser.h"
#include "PuzzleStateOverlay.h"
#include "EffectOverlay.h"
#include "GalaxyMap.h"
#include "Points.h"
#include "StarField.h"
#include "Hyperspace.h"
#include "PodDialog.h"
#include "MainMenu.h"
#include "Compat2004.h"
#include "Common.h"
#include "MemoryImage.h"
#include "SoundInstance.h"
#include <math.h>

using namespace Sexy;

extern Color* const gGemColors;
extern Color* const gOrigGemColors;

// the tables of the whirlpool: the sine and the cosine of the 4096 steps of a full turn (0x6713c8 and 0x6755c8); the
// constructor makes them once
float gSinTable[4096];
float gCosTable[4096];
static bool gTablesMade = false;        // 0x6797c8

// port: the board's memory is zeroed first (Board.h)
void* Board::operator new(size_t theSize)
{
	void* aPointer = ::operator new(theSize);
	memset(aPointer, 0, theSize);
	return aPointer;
}

// port: pairs with the operator new above
void Board::operator delete(void* thePointer)
{
	::operator delete(thePointer);
}

// 0x5ad581
Board::Board(WinDMApp* theApp)
{
	mApp = theApp;

	// the colours of the gems start as the original's
	for (int i = 0; i < 8; i++)
		gGemColors[i] = gOrigGemColors[i];

	mLevelValue = 0.0;
	mLevel = 0;
	mBombChance = mApp->GetDouble("BombChance");
	mBombCountdown = mApp->GetInteger("BombCountdown");
	mRockChance = mApp->GetDouble("RockChance");
	mPowerGemChance = mApp->GetDouble("PowerGemChance");
	mTimedGoal = (double)mApp->mTimedBonus;
	mTimedGoalStep = mApp->mTimedBonusMult;

	// (the original also seeds its recorder, GameStateEngine, with this number; the port has no recorder)
	int aSeed = Rand();
	mRand.SRand(aSeed);

	mBackdropNum = -1;
	mShootingStarPos = 0.0f;
	mEndInfoProcessed = false;
	mBackdropImage = NULL;
	mNextBackdropImage = NULL;
	mShootingStar = false;
	for (int aRow = 0; aRow < 8; aRow++)
		for (int aCol = 0; aCol < 8; aCol++)
			mBoard[aRow][aCol] = NULL;

	if (!gTablesMade)
	{
		for (int i = 0; i < 0x1000; i++)
		{
			double anAngle = (double)i * 3.14159;
			anAngle = (anAngle + anAngle) * 0.000244140625;
			gSinTable[i] = (float)sin(anAngle);
			gCosTable[i] = (float)cos(anAngle);
		}
		gTablesMade = true;
	}

	// the 7 by 7 points of the water mesh: the position on the grid, the position in the texture, the angle and the
	// distance of each from the middle
	for (int aGridY = 0; aGridY < 0x24c; aGridY += 0x54)
	{
		float aY = (float)(aGridY / 6);
		float aV = (float)((double)aY / 84.0);
		float aDY = (float)((double)aY - 42.0);
		double aDY2 = (double)(aDY * aDY);
		WaterPoint* aPoint = &mWaterMesh[aGridY / 0x54][0];
		for (int aGridX = 0; aGridX < 0x24c; aGridX += 0x54, aPoint++)
		{
			float aX = (float)(aGridX / 6);
			aPoint->mX = aX;
			aPoint->mY = aY;
			aPoint->mU = (float)((double)aX / 84.0 / 40.0);
			aPoint->mV = aV;
			float aDX = (float)((double)aX - 42.0);
			aPoint->mAngle = (float)((double)(float)atan2((double)aDY, (double)aDX) + 6.28318);
			float aDist = (float)((double)(aDX * aDX) + aDY2);
			aPoint->mRadius = (float)sqrt(aDist);
			aPoint->mScale = 1.0f;
		}
	}

	mInterfaceRestore = 0.0f;
	mWhirlStrength = 0.0f;
	mBarBlink = false;
	mHyperspace = NULL;
	mHyperspaceDelay = 0;
	mPlayWhirlSound = false;
	mInterfaceFlyRamp.SetOutRange(0.0, 1.0);
	mInterfaceFlyRamp.SetMode(0);
	mInterfaceFlyRamp.SetRamp(1);
	mPodArrowPhase = 0.0;
	mHintButtonArrowPhase = 0.0;
	mUndoIndex[0] = -1;
	mUndoIndex[1] = -1;
	mFadeIn = 1.0f;
	mSolutionNode = -1;
	mSolutionFirstNode = -1;
	mPuzzleFade = 1.0f;
	mPauseFade = 1.0f;
	mStartCountdown = 300;
	mLevelTextCountdown = 0;
	mPodArrowShown = false;
	mPauseTextOn = false;
	mSolutionMove = 0;
	mSolutionHash = 0;
	mSolutionValid = false;
	mSolutionMoves = 0;
	mSolutionHintIndex = 0;
	mIsPlanetIntro = false;
	mTutorialButton1 = NULL;
	mTutorialButton2 = NULL;
	mTutorialWidget = NULL;
	mBackLink = NULL;
	mPowerGemTipShown = false;
	mFlameFromCross = false;
	mHyperCubeTipShown = false;
	mHintPiece = 0;
	mNoMoveYet = true;

	// the mode says which rules play
	int aMode = mApp->mGameMode;
	mIsClassic = (aMode == 0 || aMode == 8);
	mIsAction = (aMode == 1 || aMode == 5 || aMode == 7);
	mIsFinity = (aMode == 7);
	mIsPuzzle = (aMode == 2 || aMode == 6);
	mIsEndless = (aMode == 3);
	mIsSecret = (aMode == 8 || aMode == 5 || aMode == 6 || aMode == 7);
	mIsTwilight = (aMode == 4);
	mIsHyper = (aMode == 5);
	mFlipped = mIsTwilight;
	if (mIsPuzzle)
	{
		// the last galaxy is the one that is reached at the end: a progress marker past it starts over
		PuzzleProgress* aProgress = mApp->mProfile->GetPuzzleProgress();
		if (mApp->mGalaxyParser->GetPlanetCount() - 1 <= aProgress->mPuzzleNum / 5)
			aProgress->mPuzzleNum = 0;
	}

	mShakeSize = 0.0f;
	mFinale = false;
	mShakeTime = 0.0f;
	mFreeMode = false;
	mInUndoPreview = false;
	mShowPlanetCountAtStart = 0;
	mSelectedPiece = NULL;
	mLastLandSoundTick = 0;
	mGameOverDelay = 0;
	mShakeX = 0;
	mShakeY = 0;
	mGameSeconds = 0;
	mSecondsCredited = 0;
	mPlayedSeconds = 0;
	mGemsCleared = 0;
	mGemsCredited = 0;
	mBiggestCascade = 0;
	mBiggestCombo = 0;
	mPowerGemsMade = 0;
	mPowerGemsCredited = 0;
	mHyperCubesMade = 0;
	mHyperCubesCredited = 0;
	mScore = 0;
	mShownScore = 0;
	mTimerFrame = 0x17;
	mLastWarningTick = 0;
	mMoveScore = 0;
	mHoverPod = -1;
	mPendingPod = -1;
	mDropDelay = mIsPuzzle ? 0 : 0xb4;
	mBigMoveTimer = 0;
	mBigMoveGlow = 0.0;
	mBoardInOverlay = false;
	mMoreHintsDelay = 0;
	mMovedSinceHint = true;
	mHintFromCol = -1;
	mHintFromRow = -1;
	mHintToCol = -1;
	mHintToRow = -1;
	mHintAccepted = false;
	mUnpauseDelay = 0;
	mMouseHeld = false;
	mPlayStartTick = 0;
	mPauseCount = 0;
	mInterfacePauseCount = 0;
	mSkipBestRecords = false;
	mLevelStartScore = 0;
	mNeverSetScoreMark = 0;
	mDrain = 0.0;
	mDrainSpeed = 0.0;
	mCheerDelay = 0;
	mCheerGiven = 0;
	mCheerTier = 0;
	mLastGoodTick = 0;
	mSwapPiece[0] = NULL;
	mSwapPiece[1] = NULL;
	mSwapFade = 1.0f;
	mScoreFont = FONT_SCORE;
	mNextLevel = 1;
	// the picture that the level text is drawn into for the glow (+0x15f54)
	// the picture of the level number that the endless mode keeps
	mFontGlowImage = new MemoryImage(mApp);
	mFontGlowImage->Create(mApp->ScaleNumUp(0x2a, 0), mApp->ScaleNumUp(0x2a, 0));
	mFontGlowImage->SetImageMode(true, true);

	SetupLevelData();
	mLevelGoalScore = mLevelGoalPoints;
	mPlayUpdates = 0;
	mBarShown = 0;
	mDrainGrace = 0xfa;
	mWarpFade = 0.0;
	mBlackHoleAngle = 0.0;
	mBlackHoleSpin = 0.0;
	mFadingSound = NULL;
	mSetsInMove = 0;
	mCascadeCount = 0;
	mStarRot = 0.0;
	mStarDrift = 0.0;
	mStarPhase = 0.0;
	for (int i = 0; i < 8; i++)
		mColumnExplodeTick[i] = 0;
	mExplodeHappened = false;
	mCrazy = false;

	if (!mIsPuzzle)
		NewBoard();
	else if (mIsSecret)
		LoadNextSecretPuzzle();
	mClearBacklog = false;

	// the overlays: the pods of the puzzle packs, the map of the galaxies, and the big texts
	if (!mIsPuzzle || mIsSecret)
		mPuzzleOverlay = NULL;
	else
		mPuzzleOverlay = new PuzzleStateOverlay(mApp);
	if (!mIsPuzzle)
		mGalaxyMap = NULL;
	else
	{
		mGalaxyMap = new GalaxyMap(mApp);
		mGalaxyMap->Resize(0, 0, mApp->ScaleNum(0x400, 0), mApp->ScaleNum(0x300, 0));
	}
	mEffectOverlay = new EffectOverlay(mApp);
	mEffectOverlay->Resize(0, 0, mApp->ScaleNum(0x400, 0), mApp->ScaleNum(0x300, 0));
	if (mIsPuzzle && !mIsSecret)
		SelectPuzzlePod(mPuzzleOverlay->mSelectedPod);

	// the buttons: menu, hint, and undo (puzzles) or pause (the timed games)
	mMenuButton = new PodButton(0, this);
	mMenuButton->mGlowColor = Color(0xff, 0xff, 0xff);
	mMenuButton->mGlowImage = IMAGE_UI_SMALL_ROLLOVER;
	mMenuButton->mButtonImage = IMAGE_UI_BUTTON_MENU;
	mHintButton = new PodButton(1, this);
	mHintButton->mGlowColor = Color(0xff, 0xff, 0xff);
	mHintButton->mGlowImage = IMAGE_UI_BIG_ROLLOVER;
	mHintButton->mButtonImage = IMAGE_UI_BUTTON_HINT;
	if (!mIsPuzzle)
	{
		if (!mIsAction)
			mThirdButton = NULL;
		else
		{
			mThirdButton = new PodButton(3, this);
			mThirdButton->mGlowColor = Color(0xff, 0xff, 0xff);
			mThirdButton->mGlowImage = IMAGE_UI_SMALL_ROLLOVER;
			mThirdButton->mButtonImage = IMAGE_UI_BUTTON_PAUSE;
		}
	}
	else
	{
		mThirdButton = new PodButton(2, this);
		mThirdButton->mGlowColor = Color(0xff, 0xff, 0xff);
		mThirdButton->mGlowImage = IMAGE_UI_SMALL_ROLLOVER;
		mThirdButton->mButtonImage = IMAGE_UI_BUTTON_UNDO;
	}

	mCoversScreenFlag = false;
	mScreenState = SCREEN_HIDDEN;
	mStaticImage = NULL;
	mGemImage = NULL;
	mRedrawBackground = true;
	mHyperGlow = false;
	mHyperGlowDelay = 0;
	mSweep0 = 0;
	mSweep1 = 0;
	mSweep2 = 0;
	mSweepA = 0;
	mSweepB = 0;
	mSweepTimer = 0;
	mHintCountdown = 0;
	mCountBombs = false;
	mLastExplodeSoundTick = 0;
	mStartGameMusic = true;
	mSecretRun = 0;
	mSecretSolved = 0;
	if (mApp->mGameMode < 4)
		mSecretModeUnlocked = mApp->mProfile->IsModeUnlocked(mApp->mGameMode + 4);
	else
		mSecretModeUnlocked = false;
	ResizeButtons();
	CreateFontGlowImages();
	mImagesChanged = false;
	mOldSkool = false;
	mXmas = false;
	mCheated = false;
	mSlowMotion = false;
	mNausea = false;
	mCheatStars = NULL;
	mNoFrame = false;
}

// 0x5a5abe, 0x59b55a (the loop that deletes the score texts of mPoints, inlined; the original deletes them from the back)
Board::~Board()
{
	// (the original waits here for its worker thread; the port runs the worker's tasks in line)

	// the best level of the mode is kept
	if (!mCheated)
	{
		int aMode = mApp->mGameMode;
		if (aMode < 4)
		{
			int& aBest = mApp->mProfile->mModeLevel[aMode];
			if (aBest <= mLevel)
				aBest = mLevel;
		}
	}
	// a game in progress is saved
	if (CanPlay() && (!mIsPuzzle || mIsSecret))
	{
		SaveGame();
		mApp->mProfile->WriteProfile();
	}
	ProcessEndGameInfo();

	delete mPuzzleOverlay;
	delete mGemImage;
	delete mStaticImage;
	delete mFontGlowImage;
	delete mBackdropImage;
	delete mNextBackdropImage;
	delete mGalaxyMap;
	delete mMenuButton;
	delete mHintButton;
	delete mThirdButton;
	delete mCheatStars;
	for (size_t i = 0; i < mPoints.size(); i++)
		delete mPoints[i];
	mPoints.clear();
	KillTutorialButtons();
	if (mFadingSound != NULL)
		mFadingSound->Release();
	mApp->KillDialog(DIALOG_MESSAGE);
	// (the trial's counter of games played, at the application's +0x734, is not ported)
	delete mHyperspace;
	ClearLightning();
	// the effect overlay (the original never deletes it: its text picture and its tables leaked with every board)
	delete mEffectOverlay;
	// (the lists, vectors and strings of the board go with the object)
}

// 0x596bc2: a button of the board (the menu and hint buttons) goes down with the click sound
void Board::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_CLICK);
}

// the ButtonListener hook (0x5ad347 is its thunk; 0x5ad35c the body)
void Board::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mApp->DoOptions();
		break;
	case 1:
		if (!mIsPuzzle || !mIsSecret || mHintsUsed != 0)
			TryShowHint();
		else if (mState != STATE_SECRET_FINISHED)
			mApp->DoDialog(DIALOG_HINT_WARNING, true, "Warning!",
				"You will not score any points for this level if you get any hints.\n\nAre you sure you want a hint?",
				"", 1);
		break;
	case 2:
		Undo();
		break;
	case 3:
		if (mPauseCount < 1)
			Pause(true);
		else
			Unpause(true);
		break;
	case 400:
		if (mState == STATE_SECRET_FINISHED)
		{
			// the secret puzzles' "Play Again"
			KillTutorialButtons();
			mApp->DoNewGame();
			mApp->mBoard->mStartCountdown = 0x32;
		}
		else if (!mIsSecret)
			SelectPuzzlePod(-1);
		else
		{
			UndoAll();
			KillTutorialButtons();
		}
		break;
	case 0x191:
		if (mState == STATE_SECRET_FINISHED)
		{
			mApp->DoMainMenu();
			mApp->mMainMenu->Show();
			KillTutorialButtons();
			Hide();
		}
		else
		{
			DeferredSelectPuzzlePod(0);
			mStartCountdown = 0x4b;
		}
		break;
	case 500:
		mState = STATE_FADE;
		mApp->DoMainMenu();
		mApp->mMainMenu->Show();
		KillTutorialButtons();
		HideNow();
		UpdateSecretUnlocked();
		break;
	}
}
