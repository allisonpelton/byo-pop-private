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
static bool gTablesMade = false;        // DAT_006797c8

void* Board::operator new(size_t theSize)
{
	void* aPointer = ::operator new(theSize);
	memset(aPointer, 0, theSize);
	return aPointer;
}

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
	m15e30 = (double)mApp->mTimedBonus;
	m15e38 = mApp->mTimedBonusMult;

	// (the original also seeds its recorder, GameStateEngine, with this number; the port has no recorder)
	int aSeed = Rand();
	mRand.SRand(aSeed);

	mBackdropNum = -1;
	m184c = 0.0f;
	mEndInfoProcessed = false;
	mBackdropImage = NULL;
	mNextBackdropImage = NULL;
	m15f6a = false;
	m1848 = false;
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
			aPoint->mRadius = (float)sqrt(aDist);   // (Ghidra calls the exe's sqrtf wrapper "logf")
			aPoint->mScale = 1.0f;
		}
	}

	m15e04 = 0.0f;
	m15d9c = 0.0f;
	m15f18 = false;
	mHyperspace = NULL;
	m15df0 = 0;
	m1750 = true;
	m15de8 = false;
	m15db0.SetOutRange(0.0, 1.0);
	m15db0.SetMode(0);
	m15db0.SetRamp(1);
	m1858 = 0.0;
	m1860 = 0.0;
	mUndoIndex[0] = -1;
	mUndoIndex[1] = -1;
	mFadeIn = 1.0f;
	mSolutionNode = -1;
	mSolutionFirstNode = -1;
	m15fac = 1.0f;
	m15fb4 = 1.0f;
	m1764 = 300;
	m1768 = 0;
	m1850 = false;
	m15fb8 = false;
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
	m194d = false;
	m194c = false;
	m194e = false;
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
	m191c = false;
	mShakeTime = 0.0f;
	m1724 = false;
	m1723 = false;
	mInUndoPreview = false;
	m190c = 0;
	m195c = 0;
	mSelectedPiece = NULL;
	m15e54 = 0;
	m15e58 = 0;
	mShakeX = 0;
	mShakeY = 0;
	m15ebc = false;
	m1960 = false;
	m15e14 = 0;
	m15e18 = 0;
	mPlayedSeconds = 0;
	m15e44 = 0;
	m15e48 = 0;
	m15e6c = 0;
	m15e70 = 0;
	m15e7c = 0;
	m15e80 = 0;
	m15e84 = 0;
	m15e88 = 0;
	mScore = 0;
	m15e40 = 0;
	m15f1c = 0x17;
	m15f20 = 0;
	m15e94 = 0;
	m15e90 = 0;
	mHoverPod = -1;
	mPendingPod = -1;
	mDropDelay = mIsPuzzle ? 0 : 0xb4;
	m15e98 = 0;
	m15ea0 = 0.0;
	m15f24 = false;
	m1910 = 0;
	m15ea8 = true;
	m15eac = -1;
	m15eb0 = -1;
	m15eb4 = -1;
	m15eb8 = -1;
	m15ea9 = false;
	mUnpauseDelay = 0;
	m15f38 = false;
	mPlayStartTick = 0;
	mPauseCount = 0;
	mInterfacePauseCount = 0;
	m15ee0 = false;
	m15ee8 = 0;
	m15ee4 = 0;
	m15ef8 = 0.0;
	m15f00 = 0.0;
	m1602c = 0;
	m16024 = 0;
	m16028 = 0;
	m16030 = 0;
	mSwapPiece[0] = NULL;
	mSwapPiece[1] = NULL;
	mSwapFade = 1.0f;
	m15ed8 = FONT_DIGIT;
	m15edc = FONT_SCORE;
	mNextLevel = 1;
	m15f40 = NULL;
	// the picture that the level text is drawn into for the glow (+0x15f54)
	// the picture of the level number that the endless mode keeps
	mFontGlowImage = new MemoryImage(mApp);
	mFontGlowImage->Create(mApp->ScaleNumUp(0x2a, 0), mApp->ScaleNumUp(0x2a, 0));
	mFontGlowImage->SetImageMode(true, true);

	SetupLevelData();
	m15eec = m15ef0;
	m1998 = 0;
	m15f50 = false;
	m1994 = 0;
	m15f88 = 0xfa;
	m15f80 = 0;
	m15f90 = 0.0;
	m15f98 = 0.0;
	m15fa0 = 0.0;
	m1728 = NULL;
	m15e4c = 0;
	m15e50 = 0;
	m16020 = false;
	m1964 = 0;
	m1968 = 1;
	m196c = -1;
	m1970 = -1;
	m1974 = -1;
	mStarRot = 0.0;
	mStarDrift = 0.0;
	mStarPhase = 0.0;
	m15fd8 = 0.0;
	for (int i = 0; i < 8; i++)
		mColumnExplodeTick[i] = 0;
	mExplodeHappened = false;
	mCrazy = false;

	if (!mIsPuzzle)
		NewBoard();
	else if (mIsSecret)
		LoadNextSecretPuzzle();
	m1751 = false;

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
	m15f70 = 0;
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
	m15f60 = NULL;
	m15f64 = NULL;
	m15f69 = true;
	m17d0 = false;
	m17d4 = 0;
	m176c = 0;
	mSweep0 = 0;
	mSweep1 = 0;
	mSweep2 = 0;
	mSweepA = 0;
	mSweepB = 0;
	mSweepTimer = 0;
	mHintCountdown = 0;
	m15f68 = false;
	mLastExplodeSoundTick = 0;
	m1770 = true;
	mSecretRun = 0;
	mSecretSolved = 0;
	if (mApp->mGameMode < 4)
		mSecretModeUnlocked = mApp->mProfile->IsModeUnlocked(mApp->mGameMode + 4);
	else
		mSecretModeUnlocked = false;
	ResizeButtons();
	CreateFontGlowImages();
	m165ae = false;
	m165ac = false;
	m165ad = false;
	mCheated = false;
	m165af = false;
	m165b4 = 0;
	m165b8 = false;
	m165bc = NULL;
	m165c0 = false;
}

// 0x5a5abe
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
	delete m15f64;
	delete m15f60;
	delete m15f40;
	delete mFontGlowImage;
	delete mBackdropImage;
	delete mNextBackdropImage;
	delete mGalaxyMap;
	delete mMenuButton;
	delete mHintButton;
	delete mThirdButton;
	delete m165bc;
	for (size_t i = 0; i < mPoints.size(); i++)
		delete mPoints[i];
	mPoints.clear();
	KillTutorialButtons();
	if (m1728 != NULL)
		m1728->Release();
	mApp->KillDialog(0x25);
	// (the trial's counter of games played, at the application's +0x734, is not ported)
	delete mHyperspace;
	ClearLightning();
	// (the lists, vectors and strings of the board go with the object)
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
		if (!mIsPuzzle || !mIsSecret || m1918 != 0)
			TryShowHint();
		else if (mState != 0x14)
			mApp->DoDialog(0x17, true, "Warning!",
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
		if (mState == 0x14)
		{
			// the secret puzzles' "Play Again"
			KillTutorialButtons();
			mApp->DoNewGame();
			mApp->mBoard->m1764 = 0x32;
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
		if (mState == 0x14)
		{
			mApp->DoMainMenu();
			mApp->mMainMenu->Show();
			KillTutorialButtons();
			Hide();
		}
		else
		{
			DeferredSelectPuzzlePod(0);
			m1764 = 0x4b;
		}
		break;
	case 500:
		mState = 0x13;
		mApp->DoMainMenu();
		mApp->mMainMenu->Show();
		KillTutorialButtons();
		HideNow();
		UpdateSecretUnlocked();
		break;
	}
}
