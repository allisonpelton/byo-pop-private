// Sexy::Board (0x6a3d7b-0x6d6899): being ported, see Board.h.
#include "Board.h"
#include "ElectroStrike.h"

#include "Buffer.h"
#include "ChallengeMenu.h"
#include "CustomBassMusicInterface.h"
#include "EffectsManager.h"
#include "FlyingText.h"
#include "GameOverShow.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Hyperspace.h"
#include "MemoryImage.h"
#include "Menu.h"
#include "Piece.h"
#include "Points.h"
#include "Shockwave.h"
#include "GemSoul.h"
#include "Traveller.h"
#include "Profile.h"
#include "RankBonusWidget.h"
#include "Ranks.h"
#include "PropertiesParser.h"
#include "ReplayWidget.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TwistUtil.h"
#include "VizManager.h"
#include "TwistD3D.h"
#include "WinTwistApp.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace Sexy;

Board* Sexy::gBoard = nullptr;
int Sexy::gSettleCount = 0;
MTRand Sexy::gBoardRand;

int Sexy::BoardRand()
{
	return (int)gBoardRand.Next();
}
// The static initialiser 0x7d5096 makes all ten Color() (opaque black); the first board's constructor copies the
// seven colours over the first seven, and the last three stay black (the credits' sparks are black until a game starts)
Color Sexy::gGemColors[10];

namespace {

// 0x85c378: the colours the constructor copies into gGemColors
const Color kGemColorTable[7] = {Color(255, 32, 32), Color(255, 255, 255), Color(0, 255, 0), Color(255, 255, 0),
	Color(255, 0, 255), Color(255, 128, 0), Color(0, 128, 255)};

// 0x85a9d0 and 0x85aef0, globals the constructor clears, are left out: the first holds the board's update count at
// each praise level (ShowPraise, 0x6ba73a, writes it) and nothing reads either

}  // namespace

// The plain fields come from their declarations' defaults; this keeps the order of everything else
// 0x6a3fff: resets +0x214 and +0x220, inline here
// 0x6a634f: resets the quick-move count and gap, inline here
// 0x6a697d: returns 30000, inline here
// 0x6a69f0: clears +0x2234, 0x150 bytes, inline here
// 0x6ab455: counts the set entries of a list, inline here
// 0x6b8c49: a lookup in the piece map, inline here
// 0x75fc8a: clears +0xc74, 0x150 bytes, inline here
Board::Board(WinTwistApp* theApp, int theMode, int theFlags, int theChallengeCategory)
{
	mApp = theApp;
	// port: the original starts a CSV header in a stats buffer at app+0x1024 when it is empty (0x6b2880); the
	// buffer is never written out, so it is left out
	mName = "BOARD";
	if (theFlags != 8) {
		mApp->mNewScorePlace = -1;
		mApp->mNewScorePoints = 0;
		mApp->mNewScoreLevel = 0;
	}
	mChallengeCategory = theChallengeCategory;
	Profile* aProfile = mApp->mProfile;
	if ((theMode == 2 || theMode == 3) && (!aProfile->HasSeenTip(14) || aProfile->mRank == -1))
		aProfile->MarkTutorialDone();
	for (int anId = -101; anId >= -104; anId--)
		mSpecialKindBySerial[anId] = anId;  // 0x6b8c49
	gBoard = this;
	// port: -seed=N replaces the time, so that test runs repeat
	gBoardRand.SRand(mApp->mHasSeedParam ? (unsigned int)mApp->mRandSeed : (unsigned int)time(nullptr));
	mBoardFade = mApp->mStartClassicAtOnce ? 0.0f : 1.0f;
	mFillColor = Color(0, 0, 0, 0);
	// 0x6a634f
	mQuickMoveCount = 0;
	mQuickMoveGap = mSettledUpdates;
	// 0x6ab455
	int aCount = 0;
	for (size_t i = 0; i < aProfile->mSavedBools.size(); i++) {
		if (aProfile->mSavedBools[i])
			aCount++;
	}
	mSavedBoolsSet = aCount;
	static const int kRolls[25] = {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5};
	mIntroRoll = kRolls[BoardRand() % 25];  // 0x6a3447: the board's own generator
	if (gApp->mProfile->mGamesPlayed == 0)
		mIntroRoll = 3;
	mZOrder = -5;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mShineImage = IMAGE_GEM_LIGHTING;
	mGridBarTop = IMAGE_INGAMEUI_GRIDBAR_TOP;
	mGridBarBot = IMAGE_INGAMEUI_GRIDBAR_BOT;
	mStepper = new BoardStepper(mApp);  // 0x74c8c8
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mIsReplay)
		std::memset(mApp->mStats, 0, sizeof(mApp->mStats));
	std::memset(mGameStats, 0, sizeof(mGameStats));  // 0x6a69f0
	std::memset(mGemCounts, 0, sizeof(mGemCounts));
	mGameMode = theMode;
	mModeFlags = theFlags;
	LoadConfig();
	if ((theFlags & 8) == 0)
		mApp->mLastGameMode = mGameMode;
	mTurnCountedForStats = mTurnCount;
	mTurnCountedForSpeed = mTurnCount;
	mTurnOfLastSound = mTurnCountedForStats;
	for (int i = 0; i < 7; i++)
		gGemColors[i] = kGemColorTable[i];
	mRand.SRand(Rand());

	// The points of a cell, row by row
	CellPoint* aPoint = mCellPoints;
	int aRow = 0;
	do {
		float aY = (float)(aRow / 6);
		float aV = (float)((double)aY / 84.0);
		float aDY = (float)((double)aY - 42.0);
		double aDY2 = (double)(aDY * aDY);
		int aCol = 0;
		do {
			float aX = (float)(aCol / 6);
			aPoint->mX = aX;
			aPoint->mY = aY;
			aPoint->mU = (float)((double)aX / 84.0 / 40.0);
			aPoint->mV = aV;
			float aDX = (float)((double)aX - 42.0);
			float anAngle = (float)std::atan2((double)aDY, (double)aDX);
			aPoint->mAngle = (float)((double)anAngle + 6.28318);
			float aDist2 = (float)((double)(aDX * aDX) + aDY2);
			aCol += 84;
			aPoint->mDist = std::sqrt(aDist2);
			aPoint->mScale = 1.0f;
			aPoint++;
		} while (aCol < 588);
		aRow += 84;
	} while (aRow < 588);

	mEasedCursorX = (float)(mWidth / 2);
	mEasedCursorY = (float)(mHeight / 2);
	mScoreFont = FONT_SCORE;
	mScoreFontCopy = FONT_SCORE;
	mSquareImage = new MemoryImage(mApp);
	mSquareImage->Create(42, 42);
	mSquareImage->SetImageMode(true, true);
	mTubeOffsetX = (float)(mApp->mWidth / 2 - GetImageOffsetX(0x3d6));
	if (mModeFlags == 8)
		mTubeOffsetX = (float)((double)mTubeOffsetX * 0.75);
	if (mApp->mTwistMusic != nullptr)
		mApp->mTwistMusic->SetState(0);

	mGoodTube = new GoodTube(this);
	mGoodTube->SetShown(true);
	mGoodTube->SetLevelSoundOn(true);
	mGoodTube->SetTubeKind(0);
	mGoodTube->SetBlitzTube(0);
	mGoodTube->SetChallengeStyle(1);
	if (theMode == 2) {
		mLevel = 1;
		mGoodTube->SetLevelSoundOn(false);
		mGoodTube->SetTubeKind(4);
		mGoodTube->SetChallengeStyle(2);
		mGoodTube->SetChallenge(true);
	} else if (theMode == 3) {
		mGoodTube->SetColor(Color(0, 255, 0), true);
		mGoodTube->SetBlitzTube(1);
	}

	mFXManager = new EffectsManager();
	// 0x6f5ea6 makes the first manager's height image when there is a device and none yet
	mFXManager->MakeHeightImage();
	mFXManager->mIsFirstManager = true;  // +0xb6
	mFXManager->mDepthTestOffFirst = true;  // +0xa1: the depth test goes off first (0x6f6451)
	mFXManager2 = new EffectsManager();
	mFXManager3 = new EffectsManager();
	mPointsManager = new PointsManager();
	mPointsManager->mMouseVisible = false;
	mReplayPod = nullptr;
	mApp->mWidgetManager->SetFocus(this);
	mBlueFlash = 0.0f;
	mGlowSoundWait = 0.0f;
	mGlowSoundRepeat = 0.0f;
	mCascadePopupOn = true;
	mCountPopupOn = true;
	mCountPopupMin = 15;
	switch (mGameMode) {
	case 2:
		mTwistSpeedGrade = 1;
		mCountPopupOn = false;
		mCascadePopupOn = false;
		break;
	case 3:
		mTwistSpeedGrade = 0;
		mBlitzStartTime = 30000;  // 0x6a697d
		mIsBlitzGame = true;
		break;
	case 5:
		if (mLevel == 0)
			mLevel = 1;
		break;
	default:
		mTwistSpeedGrade = 0;
		break;
	}
	mAlertGlow = 0.0f;
	mClearedAtStart = 0;
	mMoveBlockWait = 0.0f;
	// 0x6a3fff
	mFruitDue = false;
	mSavedOnlyFraction = 1.0f;
	mBonusGemCount = mBonusChallengeMinGems;
	mBoardX = mApp->mWidth;

	// The tips, a line each (blank lines skipped, and a last line without an end of line)
	Buffer aBuffer;
	if (gApp->ReadBufferFromFile("properties/tips.txt", &aBuffer, true)) {
		SexyString aLine;
		for (unsigned char aChar : aBuffer.mData) {
			if (aChar == '\n' || aChar == '\r') {
				aLine = Trim(aLine);  // 0x45d880
				if (!aLine.empty())
					mTips.push_back(aLine);
				aLine.clear();
			} else {
				aLine += (SexyChar)aChar;
			}
		}
	}
	mCascadePointsOn = true;
	mFirstLevelHints = true;
	std::memset(mColumnLift, 0, sizeof(mColumnLift));
}

void Board::ReadConfig(bool theResize)
{
	int anOldRows = mRows;
	int anOldCols = mCols;
	int aRows = mApp->GetInteger("GameRows", 8);
	int aCols = mApp->GetInteger("GameColumns", 8);
	mMatchLength = 3;
	mCoalMax = mApp->GetInteger("CoalMax", 0);
	mFruitFirstLevel = mApp->GetInteger("FruitFirstLevel", 8);
	int aGap = 1;
	if (0 < mApp->GetInteger("BonusRoundLevelGap", 3))
		aGap = mApp->GetInteger("BonusRoundLevelGap", 3);
	mBonusRoundLevelGap = aGap;
	mMaxLocksOnScreen = mApp->GetInteger("MaxLocksOnScreen", 8);
	mBadTwistsAllowed = true;
	mBombsCanMove = true;
	mCountersOn = true;
	mGemColorCount = 7;
	mMatchAtOnce = false;
	mDelayMatchAfterMove = false;
	mTwistOverEmpty = false;
	mCounter.Read("Counter");
	mBadGemsOnBoardMax.Read("BadGemsOnBoardMax");
	mLockGem.Read("LockGem");
	mCoalGem.Read("CoalGem");
	mSkulls.Read("Skulls");
	mLevelUpPoints.Read("LevelUpPoints");
	mDevil.Read("Devil");
	mIceGemsAllowed = mApp->GetBoolean("IceGemsAllowed", false);
	mTurnSoundsOn = true;
	mDropperGemsMove = true;
	mFruitsBlast = false;
	mLockedGemsBusy = false;
	mLaserGemCascades = mApp->GetInteger("LaserGemCascades", 3);
	mAmnestyGemCount = mApp->GetInteger("AmnestyGemCount", 10);
	mTimeFreezeMatchCount = mApp->GetInteger("TimeFreezeMatchCount", 3);
	mModeMultiplier = mApp->GetInteger("ModeMultiplier", 1);
	mBonusChallengeMinGems = mApp->GetInteger("BonusChallengeMinGems", 3);
	mBonusChallengeMaxGems = mApp->GetInteger("BonusChallengeMaxGems", 5);
	ShowTubeLevel();
	if (theResize && (anOldRows != aRows || anOldCols != aCols))
		ResizeBoard(aRows, aCols);
	mCols = aCols;
	mRows = aRows;
	delete[] mColumnBreakTime;
	mColumnBreakTime = new int[mCols];
	for (int i = 0; i < mCols; i++)
		mColumnBreakTime[i] = 0;
}

void Board::ResizeBoard(int theRows, int theCols)
{
	if (theRows == 0)
		theRows = mRows;
	if (theCols == 0)
		theCols = mCols;
	mLevelsFinished = 0;
	mFrozenGemCount = 0;
	mFrozenSpecialCount = 0;
	mScoreAtTurnStart = 0;
	mProgressGain = 0;
	ClearBoard();
	AllocGrid(theRows, theCols);
	MakeCells();
	NewLevel();
}

void Board::ClearBoard()
{
	for (size_t i = 0; i < mElectroStrikes.size(); i++)
		delete static_cast<ElectroStrike*>(mElectroStrikes[i]);  // 0x71eb9e
	mElectroStrikes.clear();  // 0x6af6b6
	for (size_t i = 0; i < mGeodes.size(); i++)
		delete mGeodes[i];
	mGeodes.clear();  // 0x6af6f1
	mCursorPiece = nullptr;
	if (mTwisting) {
		mTwisting = false;
		mInputLock = 0;
	}
	if (mGrid != nullptr) {
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					RemovePiece(aPiece, true);
			}
		}
		// The original says "Piecemap still has pieces after ClearBoard()" in its debug output when mPieceMap is not
		// empty here
	}
}

void Board::AllocGrid(int theRows, int theCols)
{
	if (mGrid != nullptr) {
		ClearBoard();
		delete[] mGrid;
	}
	delete[] mSavedGrid;
	mRows = theRows;
	mCols = theCols;
	mGrid = new Piece*[theRows * theCols];
	std::memset(mGrid, 0, mCols * mRows * sizeof(Piece*));
	mSavedGrid = new Piece*[mCols * mRows];
	std::memset(mSavedGrid, 0, mCols * mRows * sizeof(Piece*));
	MakeCells();
}

void Board::MakeCells()
{
	delete[] mCells;
	int aCount = mRows * mCols;
	mCells = new Cell[aCount];
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			mCells[mCols * aRow + aCol].mRow = aRow;
			mCells[mCols * aRow + aCol].mCol = aCol;
		}
	}
}

void Board::RemovePiece(Piece* thePiece, bool theDelete)
{
	if (thePiece == nullptr)
		return;
	if (thePiece == mCursorPiece)
		mCursorPiece = nullptr;
	mGrid[mCols * thePiece->mRow + thePiece->mCol] = nullptr;
	if (theDelete)
		delete thePiece;  // 0x6a54ac
}

// 0x6a3ff4: sets the +0x21f8 field to 100, inline here
// 0x6f3869: the backdrop file check (inline here)
void Board::NewLevel()
{
	mReplayPodDue = false;
	mGemsBrokenAtTurnStart = mGemsBroken;
	mChainLength = 0;
	mFruitBrokeThisTurn = false;
	mFallHeld = false;
	mLevelColor = mLevel % 7;
	// 0x6a634f
	mQuickMoveCount = 0;
	mQuickMoveGap = mSettledUpdates;
	mLevelDone = false;
	delete mBackdrop;
	mBackdrop = nullptr;
	// 0x6f3869: images\<art size>\backdrops\twist-back<n>.png; none of the twelve is in the game's files, so the
	// original's load always fails and the backdrop's settings (+0x15fc, the image's +0x92, +0x44 and its slots 20
	// and 32) are never made
	char aPath[64];
	std::snprintf(aPath, sizeof(aPath), "images/%d/backdrops/twist-back%d.png", mApp->mArtRes, mLevel % 12 + 1);
	if (mApp->FileExists(aPath))
		NotPorted("Board::NewLevel's backdrop (0x6bf659)");
	mCounterFlash = 0;
	mScoreRollGlow = 0;
	mShineAmount = 1.0f;
	if (0 < mLevel && mGameMode != 5 && !IsTipShowing())
		mLevelStarted = true;
	std::memset(mGameStats, 0, sizeof(mGameStats));
	SetStat(0x24, mLevel);
	mGameStats[0x24] = mLevel;
	// (not while the basics tutorial is up)
	if (mTutorial == nullptr || mTutorial->mKind != 0xe) {
		if (mLevelUpPoints.GetCount() < 500)
			mLevelProgressGoal = 500;
		else
			mLevelProgressGoal = mLevelUpPoints.GetCount();
	}
	int aMode = mGameMode;
	if (aMode == 4 && 100000 <= mLevelProgressGoal)
		mLevelProgressGoal = 100000;
	if (!mGameLoaded) {
		mUpdatesSinceCascade = 0;
		mDevilChecked = false;
		mLevelCounts = true;
		mLevelStartScore = mScore;
		mLevelProgress = 0;
		if (aMode != 2 && aMode != 3)
			mGoodTube->SetProgress(0.0f, true);
		mFruitDue = false;
		mClearedOnNewLevel = false;
		mPlaySeconds = 0;
		mLongestChain = 0;
		mMostMovesInLevel = 0;
		mLevelRecords[1] = 0;
		mLevelRecords[0] = 0;
		mChainsMade = 0;
		mTwistsMade = 0;
		mBadTwists = 0;
		mGemsBrokenAtTurnStart = 0;
		mGemsBroken = 0;
		mLevelStartMark = 100;  // 0x6a3ff4
		ShowTubeLevel();
		ResetLevelValues();
		mLockGem.Restart();
		mCounter.Restart();
		mSkulls.Restart();  // 0x6a4043
		// 0x6a49a3
		mBarWidth = Ftol((double)gGemWidth * (double)0.24f + (double)(mCols * gGemWidth));
	}
	if (mGameMode != 2)
		UpdateProgress(false);
	mBarTopAlpha = 1.0f;
	mBarEndY = 1.0f;
	mBarBottomAlpha = 1.0f;
	if (1 < mLevel)
		mCountersAllowed = true;
	if (mTipShowing) {
		// 0x75fc8a
		if (mApp->mBoard == nullptr || !mApp->mBoard->mIsReplay)
			std::memset(mApp->mStats, 0, sizeof(mApp->mStats));
	}
	mBonusGemCount = mBonusChallengeMinGems;
	mGoalCheckDue = true;
	if (mGameMode != 2 && (mGameMode == 3 || 1 < mLevel) && mBonusColors.empty())
		StartBonusChallenge(-1);
}

void Board::SetStat(int theStat, int theValue)
{
	mGameStats[theStat] = theValue;
	if (!mIsReplay)
		mApp->mStats[theStat] = theValue;
}

void Board::ResetLevelValues()
{
	if (mGameMode == 4 || mGameMode == 5)
		mCounter.mFirstChangeLevel = mLevel + 10;
	else
		mCounter.RestartAtLevel();
	mLockGem.RestartAtLevel();
	mSkulls.RestartAtLevel();
	mBadGemsOnBoardMax.RestartAtLevel();
	mCoalGem.RestartAtLevel();
	mDevil.RestartAtLevel();
	if (mGameMode != 5)
		mLevelUpPoints.RestartAtLevel();
}

void Board::UpdateTubeProgress(bool theAtOnce)
{
	if (mGoodTube->mIsBlitzTube == 0)
		mGoodTube->SetProgress((float)mLevelProgress / (float)mLevelProgressGoal, theAtOnce);
}

void Board::UpdateProgress(bool theNoCheck)
{
	UpdateTubeProgress(theNoCheck);
	CheckLevelDone();
	if (mGameMode == 1) {
		float aFrac = (float)mLevelProgress / (float)mLevelProgressGoal;
		if (!mProgressCurve.IsInitialized())
			mProgressCurve.SetCurve(GTS(0x7dcf78));
		mProgressCurve.SetInVal((double)aFrac);
		mLockGem.mDistanceScale = mProgressCurve.GetOutVal();
		mCounter.mDistanceScale = mProgressCurve.GetOutVal();
	}
	if (!theNoCheck)
		CheckCombos();
}

bool Board::CheckLevelDone()
{
	if (mIsReplay || mGameMode == 2 || mGameMode == 3)
		return false;
	if (!mLevelDone) {
		if (mLevelProgress < mLevelProgressGoal)
			return false;
		if (mReplayPod != nullptr && mReplayPod->mState != ReplayWidget::STATE_GOING && mReplayPod->mVisible) {
			// the pod goes down for the level's end, to come back after the rank bar
			mReplayPodDown = true;
			mReplayPod->mState = ReplayWidget::STATE_GOING;
			mReplayPod->mPopped = false;
		}
		mLevelDone = true;
		bool aBonus = !(mGameMode == 0 || mGameMode == 5);
		if (mTipShowing)
			aBonus = false;
		if (aBonus && 0 < mLevel && !mGameLoaded)
			mBonusLevel = GetBonusLevel(mLevel);
	}
	return true;
}

void Board::StartBonusChallenge(int theExcludeColor)
{
	int aMin = mBonusChallengeMinGems;
	int aMax = mLevel / 3 + aMin;
	if (mBonusChallengeMaxGems < aMax)
		aMax = mBonusChallengeMaxGems;
	int aRoll = BoardRand();  // 0x6a3447, as are the colours below
	int aPoints = mBonusChallengesMet * 100;
	mBonusGemCount = aRoll % (aMax - aMin + 1) + aMin;
	if (mBonusGemCount == mBonusChallengeMinGems)
		aPoints += 1000;
	else if (mBonusGemCount - mBonusChallengeMinGems == 1)
		aPoints += 2500;
	else
		aPoints += 5000;
	mBonusMatched = 0;
	mBonusColors.clear();
	mBonusPoints = aPoints;
	unsigned int i = 0;
	if (mBonusGemCount != 0) {
		do {
			int aColor;
			for (;;) {
				aColor = BoardRand() % 7;
				if (i < 2 && aColor == theExcludeColor)
					continue;
				if (!mBonusColors.empty() && aColor == mBonusColors.back())
					continue;
				break;
			}
			mBonusColors.push_back(aColor);
			i++;
		} while (i < (unsigned int)mBonusGemCount);
	}
	if (mBonusStage == 1) {
		mGoodTube->mBonusShownCount = 0;
		mBonusMatchedMark = -1;
		mBonusStage = 3;
		mGoodTube->mBonusGlowCurve.SetCurve(GTS(0x7dcf14));
	}
}

// 0x6b2b77: properties\config.xml, then the mode's own file over it (casual 0, normal 1, blitz 3, zen 4)
void Board::LoadConfig()
{
	std::string aName = "properties/config";
	if (mGameMode == 0)
		aName += "_casual";
	else if (mGameMode == 1)
		aName += "_normal";
	else if (mGameMode == 3)
		aName += "_blitz";
	else if (mGameMode == 4)
		aName += "_zen";
	aName += ".xml";
	{
		PropertiesParser aParser(mApp);
		aParser.ParsePropertiesFile("properties/config.xml");
	}
	PropertiesParser aParser(mApp);
	aParser.ParsePropertiesFile(aName);
}

// 0x6befa1
Board::~Board()
{
	// port: the rest of the destructor comes with the board's update
	// The original frees the list of smoke without deleting the smoke; the port deletes it, without its points or sparks
	// The original never deletes blitz's final score
	delete mFinalScoreLabel;
	delete mFinalScoreText;
	delete mStepper;  // 0x6bf0bb
	delete mMinuteWarningFront;
	delete mMinuteWarningGlow;
	for (Traveller* aTraveller : mSmokeTravellers) {
		if (SmokeTraveller* aSmoke = dynamic_cast<SmokeTraveller*>(aTraveller)) {
			aSmoke->mNoPoints = true;
			aSmoke->mBurst = false;
		}
		delete aTraveller;
	}
	mSmokeTravellers.clear();
	RemoveWidget(mPointsManager);
	delete mPointsManager;
	// The same holds for the shockwaves
	for (Shockwave* aWave : mShockwaves)
		delete aWave;
	mShockwaves.clear();
	for (GemSoul* aSoul : mLevelSouls)
		delete aSoul;
	mLevelSouls.clear();
	for (FlyingText* aText : mFlyingTexts)
		delete aText;
	mFlyingTexts.clear();
	// The dialogs still waiting, then the one up, are taken off the screen and deleted (0x6befa1)
	for (TwistWidget* aDialog : mPendingDialogs) {
		mApp->mWidgetManager->RemoveWidget(aDialog);
		delete aDialog;
	}
	mPendingDialogs.clear();
	if (mShownDialog != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mShownDialog);
		delete mShownDialog;
		mShownDialog = nullptr;
	}
	if (mReplayPod != nullptr) {
		RemoveWidget(mReplayPod);
		delete mReplayPod;
	}
	if (mRewindSound != nullptr)
		mRewindSound->Release();
	delete[] mColumnBreakTime;
	delete[] mSavedGrid;
	delete[] mCells;
	delete[] mGrid;
	if (mFruitDance != nullptr) {
		// a fruit dance still playing, and the background back at its speed
		RemoveWidget(mFruitDance);
		delete mFruitDance;
		mFruitDance = nullptr;
		if (gApp->mVizManager != nullptr)  // port: the original does not check
			gApp->mVizManager->mSpeed = 1.0;
	}
	if (gBoard == this)
		gBoard = nullptr;
}

void Board::AddedToManager(WidgetManager* theWidgetManager)
{
	TwistWidget::AddedToManager(theWidgetManager);
	theWidgetManager->SetFocus(this);
	// The tube, the floating points and the effects lie over the board, each over the last
	mGoodTube->mZOrder = mZOrder + 1;
	AddWidget(mGoodTube);
	mGoodTube->mVisible = true;
	if (mGameMode != MODE_BLITZ)
		LayoutTube();
	mPointsManager->mZOrder = mGoodTube->mZOrder + 1;
	AddWidget(mPointsManager);
	mFXManager->mZOrder = mPointsManager->mZOrder + 1;
	AddWidget(mFXManager);
	if (mGameMode == MODE_PUZZLE) {
		// The puzzle's panel (+0x23d4, a PuzzleUI of 0x168 bytes, 0x735df7), left of the board. Never reached: no menu
		// starts a puzzle (mode 5)
		NotPorted("Board::AddedToManager: the puzzle's panel");
	}
}

// 0x6a404e: releases the +0x1488 object, inline here
// 0x6ee282: a helper of the dialog button handler, inline here
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	if (mApp->mMusicInterface != nullptr) {
		if (mMusicPositionLow == 0 && mMusicPositionHigh == 0) {
			// port: the original keeps BASS's position in the music playing (0x6ee282) here; nothing reads it back
			// (PlayMusic drops it), so the port keeps 0
		}
		mApp->mMusicInterface->ResumeAllMusic();
	}
	if (mTutorial != nullptr && mTutorial->mKind == 0xe) {
		mApp->mProfile->MarkTutorialDone();
		mIsReplay = true;
	}
	if (mIsReplay) {
		// (the exe clears its sound manager's flag (+0x300a0) that the menus' replays set, which sends new sounds
		// through DirectSound's parametric equaliser; the port's sound manager has none)
	}
	// a challenge met (or its goal checked as met) is over as well
	if (mGameMode == MODE_CHALLENGE && (mApp->mChallengeInfo->mResult == 1 || mChallengeOver))
		mGameOver = true;
	if (!mIsReplay && !mBeforeFirstTwist) {
		if (mGameOver) {
			// a night challenge's new best goes into the profile and onto its planet, with the planet's tip
			if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr &&
				(mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0 &&
				mApp->mProfile->mChallengeBests[mChallenge->mCategory] < mChallengeBest) {
				mApp->mProfile->mChallengeBests[mChallenge->mCategory] = mChallengeBest;
				if (mApp->mMainMenu != nullptr && mApp->mMainMenu->mChallengeMenu != nullptr && mChallenge != nullptr &&
					(mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0) {
					ChallengeButton* aButton = mApp->mMainMenu->mChallengeMenu->mButtons[mChallenge->mCategory];
					aButton->mProgress = mChallengeBest;
					aButton->MakeTip();
				}
			}
			// the game is over: its save goes
			if ((unsigned int)mGameMode < 4)
				mApp->EraseFile(mApp->GetSaveName(mGameMode, mChallengeCategory));  // 0x763a85
		} else {
			Buffer aBuffer;
			// a game left in the middle of a move goes back to the level's start (or the replay's)
			if (mReplayPlaying)
				LoadGame(mReplaySave);
			else if (((!mBoardSettled || mPlayState != 0) && mSaveBuffer.GetDataLen() != 0) || IsBusy())
				LoadGame(mSaveBuffer);
			SaveGame(&aBuffer, true);
			if (0 < mLevel && !IsTimed())
				mApp->WriteBufferToFile(mApp->GetSaveName(mGameMode, mChallengeCategory), &aBuffer);
		}
	}
	if (mGrid != nullptr) {
		ClearBoard();
		delete[] mGrid;
		mGrid = nullptr;
	}
	if (mReplacedGems != 0)  // never reached: only the constructor (0x6c582b) writes +0x1488, a 0
		NotPorted("Board::RemovedFromManager: +0x1488");  // 0x6a404e: its slot 8, and +0x1488 = 0
	mApp->SetCursor(CURSOR_POINTER);
	TwistWidget::RemovedFromManager(theWidgetManager);
	if (mCornerWidget != nullptr)
		theWidgetManager->RemoveWidget(mCornerWidget);
	if (mHyperspace != nullptr)
		mApp->mWidgetManager->RemoveWidget(mHyperspace);
	mApp->mWidgetManager->RemoveWidget(mFXManager);
	RemoveWidget(mGoodTube);
	RemoveWidget(mPointsManager);
	if (mRankBonusWidget != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mRankBonusWidget);
		mRankBonusWidget = nullptr;
	}
	mApp->mWidgetManager->RemoveWidget(mShownDialog);
	// (and the puzzle's panel, +0x23d4, which no game has: no menu starts a puzzle)
}

void Board::EndGame(bool theNuke)
{
	if (mGameOver)
		return;
	if (!theNuke)
		mEndingPiece = nullptr;
	mGameOver = true;
	mEndedByNuke = theNuke;
	mApp->mNewScorePlace = mApp->CheckForHighScore(mGameMode, mApp->mProfile->mName, mLevel, mScore);
	mApp->mNewScorePoints = mScore;
	mApp->mNewScoreLevel = mLevel;
	mApp->mProfile->mGamesPlayed++;
	mApp->mProfile->mGamesByMode[mGameMode]++;
	if (mReplacedGems != 0)  // never reached: only the constructor (0x6c582b) writes +0x1488, a 0
		NotPorted("Board::EndGame: +0x1488");  // 0x6a404e: its slot 8, and +0x1488 = 0
	mApp->PlayVoice(SOUND_VOICE_GAMEOVER, false);
	mCursorPiece = nullptr;
	mWaitDialogUp = false;
	mApp->KillDialog(40);
	if (mApp->mTwistMusic != nullptr)
		mApp->mTwistMusic->SetState(0);
	mPlayState = 7;
	mEndState = 1;
	mSecondsPlayed = (int)((unsigned int)(mUpdateCnt - mPlayStartCount) / 100) + mSecondsBeforePause;
	mApp->mGamesEnded++;
	mCursorShown = false;
	mGameOverTicks = 0x41;
	if (mGameMode != MODE_BLITZ || mEndedByNuke) {
		if (mApp->mMusicInterface != nullptr) {
			mApp->mMusicInterface->StopAllMusic();
			mApp->PlayMusic(200 + (mEndedByNuke ? 1 : 0), false, false, 0, 0);
		}
		if (mEndedByNuke) {
			// The board blows up
			mApp->PlaySample(mEndedByDevil ? SOUND_DOOMGEMGAMEOVER : SOUND_ENDFINAL6B);
			mNukeProgress.SetCurve(GTS(0x7dc490));
			mNukeCloudAlpha.SetCurve(GTS(0x7dc464), &mNukeProgress);
			mNukeNovaScale.SetCurve(GTS(0x7dc448), &mNukeProgress);
			mNukeNovaAlpha.SetCurve(GTS(0x7dc42c), &mNukeProgress);
			mNukeGemZoom.SetCurve(GTS(0x7dc40c), &mNukeProgress);
			mNukeTubeShake.SetCurve(GTS(0x7dc3ec), &mNukeProgress);
			if (mEndingPiece != nullptr)
				mEndingPiece->mIsDrawn = false;
			mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
			return;
		}
	}
	mGameOverShow = new GameOverShow(this);
	AddWidget(mGameOverShow);
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
}

void Board::StartGame()
{
	if (!mApp->mPlayPressed) {
		Buffer aBuffer;
		mApp->ReadBufferFromFile(mApp->GetSaveName(mGameMode, mChallengeCategory), &aBuffer, true);
		mApp->mPlaySamples = false;
		bool aLoaded = LoadGame(aBuffer);
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr) {
					int aFlag;
					if (aPiece->IsFlagSet(Piece::FLAG_POWER)) {
						if (aPiece->IsFlagSet(Piece::FLAG_ELECTRO))
							aFlag = 0x12;
						else
							aFlag = 0;
					} else {
						if (!aPiece->IsFlagSet(Piece::FLAG_ELECTRO))
							continue;
						aFlag = 1;
					}
					mApp->mProfile->SetTipSeen(aFlag, true);
				}
			}
		}
		SetScore(mScore, true);
		if (mGameMode == MODE_CHALLENGE)
			UpdateChallengeTube(true);
		mScoreAtTurnStart = mScore;
		mGemsBrokenAtTurnStart = mGemsBroken;
		mGemsClearedAtTurnStart = mGemsCleared;
		mGoodTube->SetLevel(mLevel);
		mShineAmount = 0;
		if (mBonusColors.size() != 0) {
			mGoodTube->mBonusShown = 1.0f;
			mGoodTube->mBonusGlowFade = 1.0f;
			// The tube's Update (slot 22, 0x702736); only a loaded game gets here
			mGoodTube->Update();
		}
		mApp->mPlaySamples = true;
		if (aLoaded)
			mLevel--;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					aPiece->mIsLive = false;
			}
		}
		if (mGameMode == MODE_BLITZ) {
			mGoodTube->SetBlitzTube(1);
			mGoodTube->SetTimeFull(30000);  // 0x6a697d
			float aValue;
			if (!mApp->mPlayPressed) {
				mGoodTube->SetTime(mBlitzStartTime, true);
				mGoodTube->StartTimer(true, false);
				SetBlitzColor();
				UpdateTubeProgress(true);
				aValue = 1.5f;
				mPlayState = 5;
			} else {
				aValue = 3.5f;
			}
			mStateWait = aValue;
			mGoodTube->SetShowTenths(mApp->mPlayPressed);
		}
		// (an unregistered copy also empties the tube from zen's level 14 on: registration code, left out)
		if (!aLoaded)
			mApp->mStats[mGameMode]++;
	}
	mFirstLevelHints = true;
	if (gApp->mProfile->mGamesPlayed == 0 && (mGameMode == MODE_ZEN || !mApp->SaveExists(MODE_ZEN, 0))) {
		if (mGameMode == MODE_CLASSIC)
			return;
		if (!mApp->SaveExists(MODE_CLASSIC, 0))
			return;
	}
	mApp->mProfile->SetTipSeen(0x11, true);
	mFirstLevelHints = false;
}

void Board::TakeHyperspace(Hyperspace* theHyperspace)
{
	mJumpActive = 1.0;
	mLevelUpTilePhase = 2;
	if (mPlayState < 1)
		mProgressHold = 4.0f;
	if (mHyperspace != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mHyperspace);
		delete mHyperspace;
		mHyperspace = nullptr;
	}
	if (theHyperspace == nullptr) {
		mHyperspace = new Hyperspace();
		mHyperspace->mZOrder = mApp->mVizManager->mZOrder + 1;
		mApp->mWidgetManager->AddWidget(mHyperspace);
		mHyperspace->mTransition = 8;
		mHyperspace->mState = 1;
		mGameLoaded = false;
		// port: the tube's effects manager (+0x28c) is cleared, once it is ported
	} else {
		mApp->mVizManager->SetVisible(false);
		mHyperspace = theHyperspace;
		mApp->mWidgetManager->AddWidget(theHyperspace);
		mHyperspace->mTakenByBoard = true;
		int aTransition;
		if (mGameMode == MODE_CHALLENGE) {
			mLevelStarted = true;
			mGoodTube->SetShown(true);
			aTransition = 9;
		} else if (!gApp->mPlayPressed) {
			aTransition = 0xc;
		} else if (gApp->mProfile->HasSeenTip(0xe) || mGameMode == MODE_BLITZ) {
			aTransition = 9;
		} else {
			aTransition = 10;
		}
		mHyperspace->SetTransition(aTransition);
	}
	mHyperspace->Resize(0, 0, mWidth, mHeight);
}

void Board::StartLevel()
{
	mStartState = 1;
	mStartFade.SetOutRange(0, 0);
	mStartFade.SetInVal(0);
	ReadConfig(true);
	if (!gApp->mProfile->HasSeenTip(0xe) && mGameMode != MODE_BLITZ && mGameMode != MODE_CHALLENGE) {
		QueueTip(0xe, nullptr);
		ShowNextTip();
		mCountPopupOn = false;
		mCascadePopupOn = false;
		if (mApp->mStartClassicAtOnce)
			mTutorial->Preload();
	} else if (mGameMode != MODE_ZEN) {
		if (mGameMode == MODE_PUZZLE) {
			mLevelPrepared = false;
			mPuzzleBoardMade = true;
			// 0x6c3515(the puzzle's number at +0x2034, true), the puzzle's board, which draws from Rand. Never reached: no menu starts a
			// puzzle (mode 5)
			NotPorted("Board::StartLevel: the puzzle's board");
		} else {
			mLevelStarted = true;
			mLevelPrepared = true;
		}
	}
	// (onto whatever mSaveBuffer holds: StartLevel does not clear it)
	SaveGame(&mSaveBuffer, false);
	LayoutTube();
}

bool Board::ReadSaveHeader(Buffer& theBuffer, int* theVersion, int* theMode, int* theFlags, int* theTwistSpeedGrade,
	int* theLevel)
{
	theBuffer.SeekFront();
	int aVersion = theBuffer.ReadLong();
	*theVersion = aVersion;
	if (0x3b < aVersion) {
		*theMode = theBuffer.ReadLong();
		*theFlags = theBuffer.ReadLong();
		*theTwistSpeedGrade = theBuffer.ReadByte();
		*theLevel = theBuffer.ReadLong();
	}
	return 0x3b < aVersion;
}

namespace {

// 0x74f20a: a LevelValue as a five-byte string, its value and +0x50
std::string LevelValueBytes(const LevelValue& theValue)
{
	std::string aBytes(5, '\0');
	std::memcpy(&aBytes[0], &theValue.mValue, 4);
	aBytes[4] = (char)theValue.mIsDue;
	return aBytes;
}

// 0x6b0785: four bytes of the board as a string
std::string FourBytes(const void* theData)
{
	std::string aBytes(4, '\0');
	std::memcpy(&aBytes[0], theData, 4);
	return aBytes;
}

// 0x456570: the generator's state as a 2500-byte string, the index of its next word and then its 624 words
std::string RandStateBytes(const MTRand& theRand)
{
	uint32_t aState[MTRAND_N];
	int anIndex = 0;
	theRand.GetState(aState, &anIndex);
	std::string aBytes(4 + 4 * MTRAND_N, '\0');
	std::memcpy(&aBytes[0], &anIndex, 4);
	std::memcpy(&aBytes[4], aState, 4 * MTRAND_N);
	return aBytes;
}

// 0x74eaed: a LevelValue from its five-byte string; it keeps that value from then on
void ReadLevelValueBytes(LevelValue* theValue, const std::string& theBytes)
{
	theValue->mKeepSavedValue = true;
	if (theBytes.size() < 5)
		return;  // (a damaged save: the original copies whatever lies past the string)
	std::memcpy(&theValue->mValue, theBytes.data(), 4);
	theValue->mIsDue = theBytes[4] != 0;
}

// 0x6ace0a: four bytes of the board from a string
void ReadFourBytes(void* theData, const std::string& theBytes)
{
	if (theBytes.size() < 4)
		return;  // (likewise)
	std::memcpy(theData, theBytes.data(), 4);
}

}  // namespace

// 0x6aefc8: reads the two saved values, inline here
bool Board::LoadGame(Buffer& theBuffer)
{
	int anOldLevel = mLevel;
	int aVersion = 0;
	if (!ReadSaveHeader(theBuffer, &aVersion, &mGameMode, &mModeFlags, &mTwistSpeedGrade, &mLevel))
		return false;
	ClearBoard();
	LoadConfig();
	mTwistMatches = false;
	mTurnStartedWithMatch = false;
	mInputLock = theBuffer.ReadLong();
	mPlayState = theBuffer.ReadLong();
	mClearedOnNewLevel = theBuffer.ReadBoolean();
	mScore = theBuffer.ReadLong();
	mShownScore = mScore;
	mGemsBroken = theBuffer.ReadLong();
	mGemsBrokenAtTurnStart = theBuffer.ReadLong();
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mIsReplay)
		std::memset(mApp->mStats, 0, sizeof(mApp->mStats));
	std::memset(mGameStats, 0, sizeof(mGameStats));  // 0x6a69f0
	int aStatCount;
	if (aVersion == 0x3a)
		aStatCount = 0x66;
	else
		aStatCount = theBuffer.ReadShort();
	// (only version 0x3a, which ReadSaveHeader refuses, has more than 84; the original writes those past the app's)
	for (int i = 0; i < aStatCount; i++) {
		int aStat = theBuffer.ReadLong();
		if (i < 0x54)
			mApp->mStats[i] = aStat;
	}
	for (int i = 0; i < aStatCount; i++) {
		int aStat = theBuffer.ReadLong();
		if (i < 0x54)
			mGameStats[i] = aStat;
	}
	mLevelStartScore = theBuffer.ReadLong();
	mLevelProgress = theBuffer.ReadLong();
	mLevelProgressGoal = theBuffer.ReadLong();
	mFruitDue = theBuffer.ReadBoolean();
	mFruitsAwarded = theBuffer.ReadShort();
	mLevelStepCount = theBuffer.ReadLong();
	// 0x6aefc8: the level's record
	mLevelRecords[0] = theBuffer.ReadLong();
	mLevelRecords[1] = theBuffer.ReadLong();
	mPlaySeconds = (float)std::atof(theBuffer.ReadString().c_str());
	mMostMovesInLevel = theBuffer.ReadLong();
	mLongestChain = theBuffer.ReadLong();
	mTwistsMade = theBuffer.ReadLong();
	mChainsMade = theBuffer.ReadLong();
	mBadTwists = theBuffer.ReadLong();
	mUpdateCnt = theBuffer.ReadLong();
	mTurnCount = theBuffer.ReadLong();
	mTurnSettled = theBuffer.ReadLong();
	mTwistAngle = (float)((double)theBuffer.ReadLong() / 10000.0);
	mTwistStep = (float)((double)theBuffer.ReadLong() / 10000.0);
	mSavedOnlyFirst = theBuffer.ReadLong();
	mSavedOnlySecond = theBuffer.ReadLong();
	mSparkBlock = theBuffer.ReadShort();
	// 0x6b13b8: the pieces, numbered afresh from 1
	int aRows = theBuffer.ReadLong();
	int aCols = theBuffer.ReadLong();
	AllocGrid(aRows, aCols);
	gNextPieceId = 1;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = new Piece();
			aPiece->Load(&theBuffer);
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mX = (float)GetColX(aCol);
			aPiece->mY = (float)GetRowY(aRow);
			if (aPiece->mColor == -1) {
				delete aPiece;
				aPiece = nullptr;
			}
			mGrid[mCols * aRow + aCol] = aPiece;
			mCells[mCols * aRow + aCol].mSavedState = theBuffer.ReadBoolean();  // 0x6a38ee
		}
	}
	// 0x456330: a state of any other size leaves the generator as it was (the original asserts)
	std::string aRandState = theBuffer.ReadString();
	if (aRandState.size() == 4 + 4 * MTRAND_N) {
		uint32_t aState[MTRAND_N];
		int anIndex;
		std::memcpy(&anIndex, &aRandState[0], 4);
		std::memcpy(aState, &aRandState[4], 4 * MTRAND_N);
		gBoardRand.SetState(aState, anIndex);
	}
	ReadLevelValueBytes(&mLockGem, theBuffer.ReadString());
	ReadLevelValueBytes(&mSkulls, theBuffer.ReadString());
	ReadLevelValueBytes(&mCounter, theBuffer.ReadString());
	ReadLevelValueBytes(&mBadGemsOnBoardMax, theBuffer.ReadString());
	ReadLevelValueBytes(&mSavedOnlyLevelSetting, theBuffer.ReadString());
	ReadLevelValueBytes(&mLevelUpPoints, theBuffer.ReadString());
	ReadLevelValueBytes(&mCoalGem, theBuffer.ReadString());
	mStepLevel = theBuffer.ReadShort();
	mLevelStartMark = theBuffer.ReadLong();
	mDevilChecked = theBuffer.ReadBoolean();
	mLevelColor = theBuffer.ReadShort();
	gNextPieceId = theBuffer.ReadLong();
	mSettling = theBuffer.ReadBoolean();
	mLevelStarted = theBuffer.ReadBoolean();
	mUpdatesSinceCascade = theBuffer.ReadLong();
	mProgressGain = theBuffer.ReadLong();
	mSavedOnlyValue = theBuffer.ReadLong();
	mWheelSpinCount = theBuffer.ReadLong();
	mIntroRoll = theBuffer.ReadLong();
	mBonusColors.clear();
	int aColorCount = theBuffer.ReadShort();
	for (int i = aColorCount; i != 0; i--)
		mBonusColors.push_back(theBuffer.ReadShort());
	if (aColorCount == 0)
		mGoodTube->mBonusShown = 0;
	mGoodTube->ResetBonus();
	mGoodTube->mBonusStepsLeft = 0;
	mGoodTube->mBonusSparkTimer = 0;
	mBonusPoints = theBuffer.ReadLong();
	mBonusMatched = theBuffer.ReadShort();
	mBonusGemCount = theBuffer.ReadLong();
	mBonusChallengesMet = theBuffer.ReadLong();
	mGoodTube->mBonusShownCount = mBonusMatched;
	if (aVersion < 0x39)
		mBonusStage = 0;
	else
		mBonusStage = theBuffer.ReadByte();
	mGoodTube->mBonusGlowCurve.SetConstant(mBonusStage == 3 ? 1.0 : 0.0);
	mSavedOnlyFraction = (float)((double)theBuffer.ReadLong() / 10000.0);
	mBlitzStartTime = theBuffer.ReadLong();
	mChainLinks = theBuffer.ReadLong();
	if (theBuffer.ReadBoolean()) {
		for (WinTwistApp::BestMove& aSlot : mApp->mBestMoves) {  // 0x6a3156
			aSlot.mReplayStart = theBuffer.ReadShort();
			aSlot.mReplayEnd = theBuffer.ReadShort();
			aSlot.mBest = theBuffer.ReadLong();
			aSlot.mBuffer.Clear();
			ByteVector aBytes;
			theBuffer.ReadBuffer(&aBytes);
			aSlot.mBuffer.WriteBytes(aBytes.empty() ? nullptr : &aBytes[0], (int)aBytes.size());
		}
	}
	if (0x33 < aVersion && theBuffer.ReadBoolean()) {
		// the challenge being played: its category, number and pattern progress
		int aCategory = theBuffer.ReadShort();
		int aNumber = theBuffer.ReadShort();
		int aProgress = theBuffer.ReadShort();
		if (mGameMode == MODE_CHALLENGE) {
			SetChallenge(mApp->mChallengeInfo->GetChallenge(aCategory, aNumber));
			// port: the original writes the progress even when there is no such challenge (and crashes)
			if (mChallenge != nullptr)
				mChallenge->mProgress = aProgress;
		}
		if (0x36 < aVersion) {
			mChallengeCount = theBuffer.ReadLong();
			mChallengeBest = theBuffer.ReadLong();
		}
	}
	if (0x34 < aVersion) {
		mCoalLevel = theBuffer.ReadLong();
		mCoalDecayCount = theBuffer.ReadLong();
	}
	if (0x35 < aVersion) {
		mBestMoveChain = theBuffer.ReadLong();
		mBestMoveGems = theBuffer.ReadLong();
		mBestMovePoints = theBuffer.ReadLong();
	}
	if (0x37 < aVersion)
		mGemsCleared = theBuffer.ReadLong();
	if (0x39 < aVersion) {
		ReadFourBytes(&mUnlockFlags, theBuffer.ReadString());  // 0x6ace0a
		ReadFourBytes(&mSeenFlags, theBuffer.ReadString());
	}
	if (0x3c < aVersion) {
		theBuffer.ReadBytes((uchar*)&mMusicPositionLow, 4);
		theBuffer.ReadBytes((uchar*)&mMusicPositionHigh, 4);
	}
	if (aVersion < 0x3e)
		mGameGemCount = mApp->mStats[(0xd70 - 0xc74) / 4] + mApp->mStats[(0xd6c - 0xc74) / 4];
	else
		mGameGemCount = theBuffer.ReadLong();
	mTurnCountedForSpeed = mTurnCount - 1;
	mTurnCountedForStats = mTurnCount - 1;
	if (mGoodTube != nullptr)
		SetLevelSteps(mLevelStepCount);
	SetScore(mScore, true);
	mGoodTube->mLevelFlash = 0;
	mGoodTube->mFlashBrightness = 0;
	mGoodTube->mFlashLevel = 0;
	mGoodTube->mShownSteps = (float)mGoodTube->mChainSteps;
	mGoodTube->SetLevel(mLevel);
	mRingBurst = 0;
	mQuickMoveCount = 0;
	mGameLoaded = true;
	mSlowDownWait = 0;
	mBarTopAlpha = 1.0f;
	mBarEndY = 1.0f;
	mBarWidth = GetGridRight();
	mBarBottomAlpha = 1.0f;
	// (the original logs "Load game ENDS" to the debugger)
	// (0x7581e5 after each branch takes the VizManager's visualizer made for the jump's ChooseViz; only with Direct3D's
	// visualizers is one made, so in 2D it does nothing)
	if (mHyperspace != nullptr) {
		// A hyperspace still running ends at once: the VizManager's background for the level (0x6c967c)
		mLevel--;
		mHyperspace->ChooseViz(false);
		mLevel++;
		mHyperspace->mParent->RemoveWidget(mHyperspace);
		mApp->SafeDeleteWidget(mHyperspace);
		mBoardFade = 1.0f;
		mHyperspace = nullptr;
		mJumpStage = 0;
		mApp->mVizManager->TakePendingViz();  // 0x7581e5
		if (mLevelUpShow != nullptr) {
			mLevelUpShow->mParent->RemoveWidget(mLevelUpShow);
			mApp->SafeDeleteWidget(mLevelUpShow);
			mLevelUpShow = nullptr;
		}
		if (mRankBonusWidget != nullptr) {
			mRankBonusWidget->mParent->RemoveWidget(mRankBonusWidget);
			mApp->SafeDeleteWidget(mRankBonusWidget);
			mRankBonusWidget = nullptr;
		}
		mJumpFillWait = 0;
		mLevelUpTilePhase = 0;
		mJumpLoadFade = 0;
		mMouseVisible = true;
		mSettleWait = 0;
		mFallHeld = false;
		mApp->mVizManager->SetVisible(true);
		mApp->mVizManager->mSpeed = 1.0;
	} else if (mLevel < anOldLevel) {
		// A game loaded at a lower level than the board's: the VizManager's background through a hyperspace made for
		// it, with the level one down (0x6c9789)
		Hyperspace* aHyperspace = new Hyperspace();
		mHyperspace = aHyperspace;
		mLevel--;
		aHyperspace->ChooseViz(false);
		mLevel++;
		mGoodTube->SetLevel(mLevel);
		delete mHyperspace;
		mHyperspace = nullptr;
		mApp->mVizManager->TakePendingViz();  // 0x7581e5
	}
	UpdateProgress(true);
	for (GemSoul* aSoul : mLevelSouls)
		delete aSoul;
	mLevelSouls.clear();
	mGoScale.SetConstant(0);
	mLastSoulLandUpdate = 0;
	mJumpStage = 0;
	// 0x6b02ea
	for (Shockwave* aWave : mShockwaves)
		delete aWave;
	mShockwaves.clear();
	return true;
}

void Board::SetScore(int theScore, bool theAtOnce)
{
	mScore = theScore;
	if (theAtOnce)
		mShownScore = theScore;
	SetStat(0x49, theScore);
	UpdateProgress(theAtOnce);
}

void Board::SetChallenge(Challenge* theChallenge)
{
	mChallenge = theChallenge;
	mChallengeBest = 0;
	mApp->mChallengeInfo->mResult = 0;
	if (mChallenge == nullptr) {
		mChallengeCategory = -1;
		mChallengeNumber = 0;
		return;
	}
	mChallengeCategory = theChallenge->mCategory;
	mChallengeNumber = theChallenge->mNumber;
	if (!mChallenge->mConfig.empty()) {
		// its own properties file, then the config again
		mApp->LoadProperties(std::string(mChallenge->mConfig.begin(), mChallenge->mConfig.end()), false, false);
		ReadConfig(false);
		if (mLockGem.mFirstLevel > -1)
			mLockGem.mFirstLevel = 1;
		if (mCounter.mFirstLevel > -1)
			mCounter.mFirstLevel = 1;
		if (mCoalGem.mFirstLevel > -1)
			mCoalGem.mFirstLevel = 1;
	}
	bool aNightMode = (theChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0;
	if (aNightMode) {
		// the night challenge starts from the stage the player has got to
		mGoodTube->mBestSoFar = mApp->mProfile->mChallengeBests[mChallengeCategory];
		if (mGoodTube->mBestSoFar > 0)
			mGoodTube->mShowBestEver = true;
	}
	if ((mChallenge->mFlags & Challenge::FLAG_TIME_LIMIT) != 0) {
		mGoodTube->SetBlitzTube(1);
		mGoodTube->StartTimer(true, false);
		mGoodTube->SetShowTenths(true);
		mGoodTube->SetTimeFull(mChallenge->mTimeGoal * 100);
		UpdateChallengeTimer();
	}
	unsigned int aFlags = mChallenge->mFlags;
	int aKind;
	if ((aFlags & Challenge::FLAG_MOVES) != 0 && mChallenge->mRules.size() == 1)
		aKind = 3;
	else if ((aFlags & Challenge::FLAG_PATTERN) == 0)
		aKind = 2;
	else
		aKind = 1;
	mGoodTube->SetTubeKind(aKind);
	if (aNightMode)
		mGoodTube->SetTubeKind(5);
	// the picture behind: the category's visualizer (Viz2d in 2D whatever it says) and backdrop
	std::string aViz = mApp->mChallengeInfo->GetViz(mChallengeCategory);
	int aBackground = mApp->mChallengeInfo->GetBackground(mChallengeCategory);
	if (aViz.empty())
		aViz = "RedBars";
	if (gD3DInterface == nullptr)
		aViz = "Viz2d";
	mApp->mVizManager->SetVizByName(aViz.c_str(), true);
	mApp->mVizManager->SetBackdrop(aBackground - 1, true);
	mGoodTube->SetChallengeStyle(2);
	UpdateChallengeTube(false);
}

void Board::RestartChallenge()
{
	SetChallenge(mChallenge);
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mIsReplay)
		std::memset(mApp->mStats, 0, sizeof(mApp->mStats));
	std::memset(mGameStats, 0, sizeof(mGameStats));  // 0x6a69f0
	mTurnMatches.clear();  // 0x6c4d02
	std::memset(mGemCounts, 0, sizeof(mGemCounts));
	mBeforeFirstTwist = true;
	mChallenge->mProgress = 0;
	mGoodTube->mGoalShown = 0;
	mChallengeCount = 0;
	mChallengeBest = 0;
	mSaveBuffer.Clear();
	mScoreAtTurnStart = 0;
	mScore = 0;
	mLevelGemCount = 0;
	mGameGemCount = 0;
	if (mApp->GetCurrentMusic() == 305)
		UpdateTurnBasedMusic();
	mApp->PlaySample(SOUND_RESHUFFLE1);
	NewLevel();
	ClearBoard();
	FillInBlanks(false);
	if ((mChallenge->mFlags & Challenge::FLAG_TIME) != 0)
		PlayChallengeMusic();
	UpdateChallengeTube(false);
}

void Board::RestartBlitz()
{
	// the points' smoke goes
	while (mSmokeTravellers.size() != 0) {
		delete mSmokeTravellers.back();
		if (mSmokeTravellers.size() != 0)
			mSmokeTravellers.pop_back();
	}
	mMusicPositionLow = 0;
	mFruitBlast = 0.0f;
	mMusicPositionHigh = 0;
	mBonusStage = 0;
	int i = 0;
	do {
		WinTwistApp::BestMove& aBest = mApp->mBestMoves[i];
		aBest.mReplayStart = -1;
		aBest.mReplayEnd = -1;
		aBest.mBest = 0;
		aBest.mBuffer.Clear();
		i++;
	} while (i < 3);
	mSaveBuffer.Clear();
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mIsReplay)
		std::memset(mApp->mStats, 0, sizeof(mApp->mStats));
	std::memset(mGameStats, 0, sizeof(mGameStats));  // 0x6a69f0
	mTurnMatches.clear();  // 0x6c4d02
	mGoodTube->ResetBonus();
	mGoodTube->mBonusStepsLeft = 0;
	mGoodTube->mBonusSparkTimer = 0.0f;
	mBonusMatched = 0;
	mBonusMatchedMark = 0;
	mGoodTube->mBonusShownCount = 0;
	StartBonusChallenge(-1);
	mGoodTube->SetColor(Color(0, 255, 0), true);  // 0x401720
	std::memset(mGemCounts, 0, sizeof(mGemCounts));
	mBeforeFirstTwist = true;
	mLevelGemCount = 0;
	mGameGemCount = 0;
	mBonusChallengesMet = 0;
	mScoreAtTurnStart = 0;
	mShownScore = 0;
	mScore = 0;
	mChainLinks = 0;
	mGemsClearedAtTurnStart = 0;
	mGemsBrokenAtTurnStart = 0;
	mGemsCleared = 0;
	mGemsBroken = 0;
	SetLevelSteps(0);
	mStepLevel = 1;
	mApp->FadeOutMusic();
	mBlitzStartTime = 30000;  // 0x6a697d
	mGoodTube->SetTime(mBlitzStartTime, true);
	mPlayState = 3;
	NewLevel();
	ClearBoard();
	mLevelUpTilePhase = 2;
	FillInBlanks(false);
	mLevelUpTilePhase = 0;
	if (mApp->mVizManager != nullptr)
		mApp->mVizManager->Slot7();
}

void Board::UpdateChallengeTimer()
{
	if (mChallenge == nullptr || (mChallenge->mFlags & Challenge::FLAG_TIME_LIMIT) == 0)
		return;
	// the time left in hundredths: the limit less the seconds played (the app's +0xd00) and this second's part
	unsigned int aPlayed = (unsigned int)mApp->mStats[35];
	int aLeft = mChallenge->mTimeGoal * 100 - (int)(aPlayed * 100 + mSecondProgress);
	if (aLeft < 0)
		aLeft = 0;
	if (1 < aPlayed) {
		int aSeconds = (aLeft + 99) / 100;
		if (mBlitzSecondsShown != aSeconds) {
			mBlitzSecondsShown = aSeconds;
			ShowBlitzTime(aSeconds);
		}
	}
	mGoodTube->SetTime(aLeft, true);
}

void Board::UpdateChallengeTube(bool theAtOnce)
{
	if (mChallenge == nullptr)
		return;
	// The tube shows the challenge's text with its count, and how much is done
	float aProgress;
	mGoodTube->mText = mApp->mChallengeInfo->GetText(mChallenge, true, false, &aProgress);  // 0x7070f4
	if ((mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0) {
		// A nightmode challenge runs out: the share of the time (or the moves) left
		if ((mChallenge->mFlags & Challenge::FLAG_TIME) != 0) {
			if (mChallenge->mTimeGoal == 0)
				aProgress = 0;
			else
				aProgress = (float)(1.0 - (double)(unsigned int)mApp->mStats[0x23] / (double)mChallenge->mTimeGoal);
		} else if ((mChallenge->mFlags & Challenge::FLAG_MOVES) != 0) {
			if (mChallenge->mMoveGoal == 0)
				aProgress = 0;
			else
				aProgress = (float)(1.0 - (double)(unsigned int)mApp->mStats[0x21] / (double)mChallenge->mMoveGoal);
		}
	}
	mGoodTube->SetProgress(aProgress, theAtOnce);
}

void Board::SetBlitzColor()
{
	// 0x85c420: set on the first call
	static const Color aColors[6] = {Color(255, 0, 0), Color(255, 0, 0), Color(255, 255, 0), Color(0, 255, 0),
		Color(0, 255, 0), Color(0, 255, 0)};
	int aStep = mBlitzStartTime / 6000;
	mGoodTube->SetColor(aColors[aStep], false);
	if (aStep == 1) {
		ColorCycle& aCycle = mGoodTube->mColorCycle;
		aCycle.mColors.clear();
		aCycle.mPos = 0;
		aCycle.mColors.push_back(Color(255, 0, 0));
		aCycle.mColors.push_back(Color(64, 0, 0));
		mGoodTube->mColorCycle.mLoop = true;
	}
}

void Board::ShowWords(const SexyString& theText, bool theQueued, bool theClear)
{
	if (theClear) {
		size_t i = 0;
		if (mFlyingTexts.size() != 0) {
			do {
				if (mFlyingTexts[i] != nullptr)
					delete mFlyingTexts[i];
				i++;
			} while (i < mFlyingTexts.size());
		}
		mFlyingTexts.clear();  // 0x6af767
	}
	int aX = GetColX(0) + mCols * gGemWidth / 2;
	int aY = GetRowY(0) + mRows * gGemHeight / 2;
	FlyingText* aText = new FlyingText(FONT_INCREDIBLE, theText, aX, aY);  // 0x6fd358
	aText->mQueued = theQueued;
	mFlyingTexts.push_back(aText);  // 0x6b9bcd
}

void Board::ShowBlitzTime(int theSeconds)
{
	// The minute marks' texts, made on the first mark (0x85c484 to 0x85c490, flag 0x85c494)
	static bool aMadeTexts = false;
	static SexyString aMinuteTexts[5];
	if (theSeconds == 30) {
		mApp->PlayVoice(SOUND_VOICE_THIRTYSECONDS, false);  // slot 108
		ShowWords(GT(0x7dd06c), false, true);
	}
	if ((unsigned int)(theSeconds - 1) < 9) {
		mApp->PlaySample(GetSoundById(theSeconds + 0x348));
	} else if (theSeconds % 60 == 0 && (unsigned int)(theSeconds / 60 - 1) < 4) {
		int aMinutes = theSeconds / 60;
		mApp->PlayVoice(GetSoundById(GameInt(GTB(0x84e9b4, 20) + aMinutes * 4)), false);
		if (!aMadeTexts) {
			aMadeTexts = true;
			aMinuteTexts[1] = GT(0x7dd04c);
			aMinuteTexts[2] = GT(0x7dd02c);
			aMinuteTexts[3] = GT(0x7dd00c);
			aMinuteTexts[4] = GT(0x7dcfec);
		}
		// FONT_WARNING's layers: the glow's text with layer 2 clear, then the front's with layer 1 clear, so that
		// the two images come out in different colours
		Font* aFont = FONT_WARNING;
		SetFontLayerColor(aFont, 0, Color(0, 0, 0, 0));
		SetFontLayerColor(aFont, 1, Color(0, 0, 0, 0));
		SetFontLayerColor(aFont, 2, Color::White);
		// (the original leaves the last minute's images; the port frees them, nothing draws them again)
		delete mMinuteWarningFront;
		mMinuteWarningFront = new TextImage(aFont, aMinuteTexts[aMinutes], 0, 0, -1, 0, nullptr);
		SetFontLayerColor(aFont, 1, Color::White);
		SetFontLayerColor(aFont, 2, Color(0, 0, 0, 0));
		delete mMinuteWarningGlow;
		mMinuteWarningGlow = new TextImage(aFont, aMinuteTexts[aMinutes], 0, 0, -1, 0, nullptr);
		SetFontLayerColor(aFont, 0, Color::White);
		SetFontLayerColor(aFont, 1, Color::White);
		SetFontLayerColor(aFont, 2, Color::White);
		mMinuteWarningWait = 5.0f;
		mMinuteWarningAlpha = 0.5f;
		mMinuteWarningRed = 1.0f;
		SetBlitzColor();
	}
}

Piece* Board::GetPieceAt(int theRow, int theCol)
{
	if (theRow < 0 || mRows <= theRow || theCol < 0 || mCols <= theCol)
		return nullptr;
	return mGrid[mCols * theRow + theCol];
}

Piece* Board::GetPieceById(int theId)
{
	std::map<int, Piece*>::iterator anItr = mPieceMap.find(theId);
	if (anItr == mPieceMap.end())
		return nullptr;
	return anItr->second;
}

void Board::AddPieceId(int theId, Piece* thePiece)
{
	mPieceMap.insert(std::make_pair(theId, thePiece));
}

void Board::RemovePieceId(int theId)
{
	std::map<int, Piece*>::iterator anItr = mPieceMap.find(theId);
	if (anItr != mPieceMap.end())
		mPieceMap.erase(anItr);
}

void LevelValue::Read(const char* theName)
{
	std::string aName = theName;
	mFirstLevel = gApp->GetInteger(aName + "FirstLevel", -1);
	mLevelsBetweenChange = gApp->GetInteger(aName + "LevelsBetweenChange", 1);
	mFirstChangeLevel = gApp->GetInteger(aName + "FirstChangeLevel", -1);
	if (mFirstChangeLevel < 0)
		mFirstChangeLevel = mFirstLevel + mLevelsBetweenChange;
	mBase = gApp->GetInteger(aName + "Base", 1);
	mChange = gApp->GetInteger(aName + "Change", 1);
	mMin = gApp->GetInteger(aName + "Min", -1);
	mMax = gApp->GetInteger(aName + "Max", -1);
	mValueBase = gApp->GetInteger(aName + "ValueBase", 1);
	mValueChange = gApp->GetInteger(aName + "ValueChange", 1);
	mValueMin = gApp->GetInteger(aName + "ValueMin", -1);
	mValueMax = gApp->GetInteger(aName + "ValueMax", -1);
	mTurnBased = gApp->GetBoolean(aName + "TurnBased", false);
	mChancePow = gApp->GetDouble(aName + "ChancePow", 0);
	mTurnsBetweenChange = gApp->GetInteger(aName + "TurnsBetweenChange", 0);
	mChanceScale = 1.0;
	mDistanceScale = 1.0;
	if (!mKeepSavedValue)
		Restart();
}

void LevelValue::RestartAtLevel()
{
	if (mFirstLevel == gBoard->mLevel)
		Restart();
}

int LevelValue::GetCount()
{
	int aLevel = gBoard->mLevel;
	int aValue;
	if (aLevel < mFirstLevel || mFirstLevel < 0) {
		aValue = -1;
	} else {
		aValue = mBase;
		if (mFirstChangeLevel <= aLevel)
			aValue = aValue + ((aLevel - mFirstChangeLevel) + 1) / mLevelsBetweenChange * mChange;
		if (0 < mTurnsBetweenChange)
			aValue = aValue + (int)((unsigned int)gApp->mStats[0x21] / (unsigned int)mTurnsBetweenChange) * mChange;
		if (-1 < mMin && aValue < mMin)
			aValue = mMin;
		if (-1 < mMax && mMax < aValue)
			aValue = mMax;
	}
	return aValue;
}

int LevelValue::GetValue()
{
	int aLevel = gBoard->mLevel;
	int aValue;
	if (aLevel < mFirstLevel || mFirstLevel < 0) {
		aValue = -1;
	} else {
		aValue = mValueBase;
		if (mFirstChangeLevel <= aLevel)
			aValue = aValue + ((aLevel - mFirstChangeLevel) + 1) / mLevelsBetweenChange * mValueChange;
		if (0 < mTurnsBetweenChange)
			aValue = aValue + (int)((unsigned int)gApp->mStats[0x21] / (unsigned int)mTurnsBetweenChange) * mValueChange;
		if (-1 < mValueMin && aValue < mValueMin)
			aValue = mValueMin;
		if (-1 < mValueMax && mValueMax < aValue)
			aValue = mValueMax;
	}
	return aValue;
}

Piece* Board::GetPieceAtXY(int theX, int theY)
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				float aX = (float)theX;
				if (aPiece->mX <= aX && aX < (float)gGemWidth + aPiece->mX) {
					float aY = (float)theY;
					if (aPiece->mY <= aY && aY < (float)gGemHeight + aPiece->mY)
						return aPiece;
				}
			}
		}
	}
	return nullptr;
}

int Board::GetPan(int theX)
{
	int aWidth = GetColX(mCols - 1) + (gGemWidth - GetColX(0));
	int aLeft = GetColX(0);
	return Ftol((double)(theX - aWidth / 2 - aLeft) / aWidth * 2 * 800.0);
}

void Board::PlayPieceSound(Piece* thePiece, int theSoundId, float thePitch, float theVolume, bool theTrack)
{
	if (!mQuietFill)
		mApp->PlaySoundEx(theSoundId, GetPan(Ftol(thePiece->mX)), thePitch, theVolume, theTrack, true);
}

// 0x6a38db: writes one byte to the save buffer, inline here
// 0x6a6584: writes the grid cells, inline here
// 0x6af04f: writes the saved values, inline here
void Board::SaveGame(Buffer* theBuffer, bool theReplays)
{
	// (the original logs "SaveGame starts" to the debugger)
	int aStart = theBuffer->mWriteBitPos / 8;  // port (web): see mSaveRanges
	theBuffer->WriteLong(0x3e);
	theBuffer->WriteLong(mGameMode);
	theBuffer->WriteLong(mModeFlags);
	theBuffer->WriteByte((uchar)mTwistSpeedGrade);
	theBuffer->WriteLong(mLevel);
	theBuffer->WriteLong(mInputLock);
	theBuffer->WriteLong(mPlayState);
	theBuffer->WriteBoolean(mClearedOnNewLevel);
	theBuffer->WriteLong(mScore);
	theBuffer->WriteLong(mGemsBroken);
	theBuffer->WriteLong(mGemsBrokenAtTurnStart);
	theBuffer->WriteShort(0x54);
	for (int i = 0; i < 0x54; i++)
		theBuffer->WriteLong(mApp->mStats[i]);
	for (int i = 0; i < 0x54; i++)
		theBuffer->WriteLong(mGameStats[i]);
	theBuffer->WriteLong(mLevelStartScore);
	theBuffer->WriteLong(mLevelProgress);
	theBuffer->WriteLong(mLevelProgressGoal);
	theBuffer->WriteBoolean(mFruitDue);
	theBuffer->WriteShort((short)mFruitsAwarded);
	theBuffer->WriteLong(mLevelStepCount);
	// 0x6af04f: the level's record
	theBuffer->WriteLong(mLevelRecords[0]);
	theBuffer->WriteLong(mLevelRecords[1]);
	theBuffer->WriteString(StrFormat(GTC(0x7dc1e0), (double)mPlaySeconds));
	theBuffer->WriteLong(mMostMovesInLevel);
	theBuffer->WriteLong(mLongestChain);
	theBuffer->WriteLong(mTwistsMade);
	theBuffer->WriteLong(mChainsMade);
	theBuffer->WriteLong(mBadTwists);
	theBuffer->WriteLong(mUpdateCnt);
	theBuffer->WriteLong(mTurnCount);
	theBuffer->WriteLong(mTurnSettled);
	theBuffer->WriteLong(Ftol((double)mTwistAngle * 10000.0));
	theBuffer->WriteLong(Ftol((double)mTwistStep * 10000.0));
	theBuffer->WriteLong(mSavedOnlyFirst);
	theBuffer->WriteLong(mSavedOnlySecond);
	theBuffer->WriteShort((short)mSparkBlock);
	// 0x6a6584: the pieces, each cell's piece (or -1) and its +0x0
	theBuffer->WriteLong(mRows);
	theBuffer->WriteLong(mCols);
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				theBuffer->WriteShort(-1);
			else
				aPiece->Save(theBuffer);
			theBuffer->WriteBoolean(mCells[mCols * aRow + aCol].mSavedState);
		}
	}
	theBuffer->WriteString(RandStateBytes(gBoardRand));
	theBuffer->WriteString(LevelValueBytes(mLockGem));
	theBuffer->WriteString(LevelValueBytes(mSkulls));
	theBuffer->WriteString(LevelValueBytes(mCounter));
	theBuffer->WriteString(LevelValueBytes(mBadGemsOnBoardMax));
	theBuffer->WriteString(LevelValueBytes(mSavedOnlyLevelSetting));
	theBuffer->WriteString(LevelValueBytes(mLevelUpPoints));
	theBuffer->WriteString(LevelValueBytes(mCoalGem));  // (mDevil is not saved)
	theBuffer->WriteShort((short)mStepLevel);
	theBuffer->WriteLong(mLevelStartMark);
	theBuffer->WriteBoolean(mDevilChecked);
	theBuffer->WriteShort((short)mLevelColor);
	theBuffer->WriteLong(gNextPieceId);
	theBuffer->WriteBoolean(mSettling);
	theBuffer->WriteBoolean(mLevelStarted);
	theBuffer->WriteLong(mUpdatesSinceCascade);
	theBuffer->WriteLong(mProgressGain);
	theBuffer->WriteLong(mSavedOnlyValue);
	theBuffer->WriteLong(mWheelSpinCount);
	theBuffer->WriteLong(mIntroRoll);
	int aCount = (int)mBonusColors.size();
	theBuffer->WriteShort((short)aCount);
	for (int i = 0; i < aCount; i++)
		theBuffer->WriteShort((short)mBonusColors[i]);
	theBuffer->WriteLong(mBonusPoints);
	theBuffer->WriteShort((short)mBonusMatched);
	theBuffer->WriteLong(mBonusGemCount);
	theBuffer->WriteLong(mBonusChallengesMet);
	theBuffer->WriteByte((uchar)mBonusStage);
	theBuffer->WriteLong(Ftol((double)mSavedOnlyFraction * 10000.0));
	theBuffer->WriteLong(mBlitzStartTime);
	theBuffer->WriteLong(mChainLinks);
	if (!theReplays)
		theBuffer->WriteBoolean(false);
	else
		WriteReplays(theBuffer);
	if (mChallenge == nullptr) {
		theBuffer->WriteBoolean(false);
	} else {
		// The challenge: its category, its number, how far its pattern is; then the stat it counts and its best
		theBuffer->WriteBoolean(true);
		theBuffer->WriteShort((short)mChallenge->mCategory);
		theBuffer->WriteShort((short)mChallenge->mNumber);
		theBuffer->WriteShort((short)mChallenge->mProgress);
		theBuffer->WriteLong(mChallengeCount);
		theBuffer->WriteLong(mChallengeBest);
	}
	theBuffer->WriteLong(mCoalLevel);
	theBuffer->WriteLong(mCoalDecayCount);
	theBuffer->WriteLong(mBestMoveChain);
	theBuffer->WriteLong(mBestMoveGems);
	theBuffer->WriteLong(mBestMovePoints);
	theBuffer->WriteLong(mGemsCleared);
	theBuffer->WriteString(FourBytes(&mUnlockFlags));
	theBuffer->WriteString(FourBytes(&mSeenFlags));
	theBuffer->WriteBytes((const uchar*)&mMusicPositionLow, 4);
	theBuffer->WriteBytes((const uchar*)&mMusicPositionHigh, 4);
	theBuffer->WriteLong(mGameGemCount);
	// (and "SaveGame ENDS")
	if (!theReplays && (theBuffer == &mSaveBuffer || theBuffer == &mReplaySave)) {
		// port (web): this save replaces any this buffer held from here on
		for (size_t i = mSaveRanges.size(); i-- > 0;)
			if (mSaveRanges[i].mBuffer == theBuffer && mSaveRanges[i].mStart >= aStart)
				mSaveRanges.erase(mSaveRanges.begin() + i);
		mSaveRanges.push_back({theBuffer, aStart, theBuffer->mWriteBitPos / 8});
	}
}

// part of SaveGame (0x6b0d91): with theReplays, its flag and the app's three replays
void Board::WriteReplays(Buffer* theBuffer)
{
	theBuffer->WriteBoolean(true);
	for (WinTwistApp::BestMove& aSlot : mApp->mBestMoves) {  // 0x6a3192
		theBuffer->WriteShort((short)aSlot.mReplayStart);
		theBuffer->WriteShort((short)aSlot.mReplayEnd);
		theBuffer->WriteLong(aSlot.mBest);
		theBuffer->WriteBuffer(aSlot.mBuffer.mData);
	}
}

// port (web): RemovedFromManager's save. A game left in a replay or between a twist and its settle is saved as it was
// before (mReplaySave, mSaveBuffer: LoadGame of it, then SaveGame with the replays), which ResaveAsLoaded gives from
// that save's bytes; LoadGame reads a buffer from its read position, and one it cannot read leaves the board as it is.
// The level and mode that decide are the loaded save's, as they are after LoadGame.
bool Board::GetQuitSave(Buffer* theSave, int* theMode)
{
	bool aTutorialDone = mIsReplay || (mTutorial != nullptr && mTutorial->mKind == 0xe);
	bool anOver = mGameOver ||
				  (mGameMode == MODE_CHALLENGE && (mApp->mChallengeInfo->mResult == 1 || mChallengeOver));
	if (aTutorialDone || mBeforeFirstTwist || anOver)
		return false;
	const Buffer* aSource = nullptr;
	if (mReplayPlaying)
		aSource = &mReplaySave;
	else if (((!mBoardSettled || mPlayState != 0) && mSaveBuffer.GetDataLen() != 0) || IsBusy())
		aSource = &mSaveBuffer;
	const SaveRange* aRange = nullptr;
	if (aSource != nullptr) {
		for (const SaveRange& aSaved : mSaveRanges)
			if (aSaved.mBuffer == aSource && aSaved.mStart * 8 == aSource->mReadBitPos &&
				aSaved.mEnd <= aSource->GetDataLen())
				aRange = &aSaved;
		if (aRange == nullptr && aSource->mReadBitPos < aSource->GetDataLenBits())
			SDL_Log("Board::GetQuitSave: a save not written by SaveGame; the board as it is instead");
	}
	if (aRange != nullptr)
		ResaveAsLoaded(aSource->GetDataPtr() + aRange->mStart, theSave);
	else
		SaveGame(theSave, true);
	theSave->SeekFront();
	theSave->ReadLong();
	int aMode = theSave->ReadLong();
	theSave->ReadLong();
	theSave->ReadByte();
	int aLevel = theSave->ReadLong();
	theSave->SeekFront();
	if (!(0 < aLevel) || aMode == MODE_BLITZ || HasChallengeFlag(2))
		return false;
	*theMode = aMode;
	return true;
}

// port (web): what LoadGame of a save SaveGame wrote without the replays, then SaveGame with them, writes. It walks
// the save by SaveGame's layout and copies it, but for what the two change: SetScore puts the score in stat 0x49 (the
// app's and the board's), the level's time (+0x230) is read into a float and written again, and so are the three
// values kept as ten-thousandths (+0x1454, +0x1458, +0x220); the replays' flag becomes the app's replays. A piece
// (Piece::Save, or the short -1 of none) loads and saves as it was. The suite compares this with a quit's save.
void Board::ResaveAsLoaded(const uchar* theSave, Buffer* theOut)
{
	const uchar* aPos = theSave;
	auto Copy = [&](int theBytes) {
		theOut->WriteBytes(aPos, theBytes);
		aPos += theBytes;
	};
	auto Long = [&]() {
		int aValue;
		std::memcpy(&aValue, aPos, 4);
		return aValue;
	};
	auto Short = [&]() {
		short aValue;
		std::memcpy(&aValue, aPos, 2);
		return (int)aValue;
	};
	auto TenThousandths = [&]() {
		float aValue = (float)((double)Long() / 10000.0);
		aPos += 4;
		theOut->WriteLong(Ftol((double)aValue * 10000.0));
	};
	auto LenString = [&]() { Copy(2 + Short()); };
	int aScore;
	std::memcpy(&aScore, theSave + 26, 4);  // +0x168, after 0x3e, the mode, the flags, +0x154 and four more
	Copy(38);
	int aStats = Short();
	Copy(2);
	for (int anArray = 0; anArray < 2; anArray++)
		for (int i = 0; i < aStats; i++) {
			int aStat = Long();
			aPos += 4;
			theOut->WriteLong(i == 0x49 ? aScore : aStat);
		}
	Copy(27);  // +0x208 to the level's record
	std::string aTime((const char*)aPos + 2, Short());
	aPos += 2 + aTime.size();
	theOut->WriteString(StrFormat(GTC(0x7dc1e0), (double)(float)std::atof(aTime.c_str())));
	Copy(32);  // +0x234 to +0x1fc4
	TenThousandths();
	TenThousandths();
	Copy(10);  // +0x1464, +0x1468, the spark block
	int aRows = Long();
	int aCols;
	std::memcpy(&aCols, aPos + 4, 4);
	Copy(8);
	for (int i = 0; i < aRows * aCols; i++) {
		Copy(Short() == -1 ? 2 : 23);
		Copy(1);
	}
	for (int i = 0; i < 8; i++)  // the generator and the seven level values
		LenString();
	Copy(35);  // +0x21d0 to +0x21e0
	int aColors = Short();
	Copy(2 + 2 * aColors);
	Copy(15);  // +0x2658 to +0x265c
	TenThousandths();
	Copy(8);  // +0x3458, +0x818
	aPos++;
	WriteReplays(theOut);
	if (*aPos != 0)
		Copy(1 + 2 * 3 + 4 * 2);  // the challenge
	else
		Copy(1);
	Copy(24);  // +0x1f64 to +0x1f0
	LenString();  // +0x3918
	LenString();  // +0x3914
	Copy(12);  // +0x3928, +0x392c, +0x1fc
}

void Board::StartReplay(Buffer& theBuffer, int theRow, int theCol, bool thePlayback)
{
	if (mPauseCount != 0)
		Unpause(true);
	mGoodTube->ResetBonus();
	mFXManager2->Clear();
	mFXManager->Clear();
	// (with Direct3D a menu's replay board also takes the slide's place)
	if (!mIsReplay || gD3DInterface != nullptr)
		mBoardX = GetSlideX();
	LoadGame(theBuffer);
	if (mIsReplay) {
		LoadConfig();
		ReadConfig(false);
		UpdateProgress(true);
	}
	mGameLoaded = false;
	MoveCursor(theRow, theCol, false, nullptr);
	mCursorPiece = GetPieceAt(theRow, theCol);
	mCursorShown = true;
	mGoodTube->mBonusState = 0;
	mGoodTube->mBonusSparkTimer = 0.0f;
	if (mGameMode == MODE_BLITZ) {
		mGoodTube->SetBlitzTube(1);
		mGoodTube->SetTimeFull(30000);  // 0x6a697d
		mGoodTube->SetTime(mBlitzStartTime, true);
		mGoodTube->StartTimer(true, false);
		SetBlitzColor();
		UpdateTubeProgress(true);
		mGoodTube->SetShowTenths(mApp->mPlayPressed);
	}
	if (!thePlayback) {
		mPlayState = 0;
		mRecordedFrames = 0;
		StartTwist(false, -1, -1);
	} else {
		// The recorded frames of the move run backwards first (UpdateGame's state 1), to the rewind's sound
		if (mRewindSound == nullptr)
			mRewindSound = mApp->mSoundManager->GetSoundInstance(SOUND_REWIND);
		if (mRewindSound != nullptr)
			mRewindSound->Play(true, false);
		mRewindFrame = mRecordedFrames;
		mPlayState = 1;
	}
	if (!mApp->Is3DAccelerated())
		mTubeSlide.SetConstant(0.0);
	// The player cannot click the board until the replay's move is over
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	mReplayPlaying = true;
}

void Board::ShowRankBonus(int theBonus)
{
	if (mRankBonusWidget != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mRankBonusWidget);
		delete mRankBonusWidget;
	}
	AddStat(0x38, theBonus);
	// The ranks the points already reach (the rank goes up only here and in the bar)
	Profile* aProfile = mApp->mProfile;
	while (aProfile->mRank < (int)mApp->mRanks->mRanks.size() - 1 &&
		(unsigned int)mApp->mRanks->GetPoints(aProfile->mRank + 1) <= (unsigned int)aProfile->mRankPoints)
		aProfile->mRank++;
	if ((unsigned int)aProfile->mRankPoints < (unsigned int)mApp->mRanks->GetPoints(aProfile->mRank) ||
		(unsigned int)mApp->mRanks->GetPoints(aProfile->mRank + 1) <= (unsigned int)aProfile->mRankPoints)
		aProfile->mRankPoints = mApp->mRanks->GetPoints(aProfile->mRank);
	int aX = Ftol((double)mApp->mWidth * 0.5);
	if (gApp->mLastGameMode == 1)
		aX += Ftol((double)gApp->mScale * -90.0);
	float aYFrac = mGameMode == 3 ? 0.75f : 0.5f;
	int aY = Ftol((double)mApp->mHeight * (double)aYFrac);
	mRankBonusWidget = new RankBonusWidget(aProfile->mRankPoints, theBonus, aX, aY);
	mApp->mWidgetManager->AddWidget(mRankBonusWidget);
}
