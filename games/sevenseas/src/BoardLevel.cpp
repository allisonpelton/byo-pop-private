// Board.cpp, part 1: a new game, a new level and the small list helpers (WinSS.exe 0x401000-0x403580, 0x402540-0x402690).

#include "Board.h"
#include "SevenSeasApp.h"
#include "Screens.h"
#include "Compat2004.h"
#include "WidgetManager.h"

#include <cstdlib>

using namespace Sexy;

static const int kCornerMin = 0;
static const int kCornerMax = 10;

// 0x45dcc1 + the inlined constructor of the 0x48-byte ship (rand() draws the animation phase)
static Ship* NewShip()
{
	Ship* aShip = new Ship();
	aShip->mMoveProgress = 0;
	aShip->mHeading = 8;
	aShip->mWantHeading = 8;
	aShip->mAnim = CrtRand() % 8;
	return aShip;
}

// the caller's part of a ship's initialisation, which follows the inlined constructor (and draws the phase again)
static void InitShip(Ship* theShip, int theType, int theX, int theY)
{
	theShip->mX = theX;
	theShip->mNextX = theX;
	theShip->mY = theY;
	theShip->mNextY = theY;
	theShip->mHeading = 4;
	theShip->mWantHeading = 4;
	theShip->mMoveProgress = 0;
	theShip->mAnim = CrtRand() % 8;
	theShip->mType = theType;
	for (int i = 0; i < 9; i++)
		theShip->mTrail[i] = -1;
}

// 0x401000
Board::Board(SevenSeasApp* theApp, bool theLoad) :
	mApp(theApp)
{
	mHeading = 8;
	mWantHeading = 8;
	mMoveProgress = 0;
	mShipAnim = CrtRand() % 8;
	mBlink = false;
	mStartLink = NULL;
	mScore = 0;
	mSavedLevels = 0;
	mLifeFlash = 0;
	mTutorialStep = 0;
	mBonusState = 0;
	ClearCellFlash();
	CrtRand();
	CrtRand();
	// the score check (the +0x6c object): not ported
	NewGame(theLoad);
}

// 0x401280
Board::~Board()
{
	ClearPorts();
	ClearAnims();
	ClearShots();
	ClearShips();
	for (std::list<ScoreText*>::iterator anItr = mScoreTexts.begin(); anItr != mScoreTexts.end(); ++anItr)
		delete *anItr;
	mScoreTexts.clear();
}

// 0x401470
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	if (mStartLink != NULL)
		theWidgetManager->RemoveWidget(mStartLink);
}

// 0x4014a0
void Board::NewGame(bool theLoad)
{
	mApp->mGamesPlayed++;
	mTutorialShown = true;
	if ((mApp->mDifficulty != 1) || mApp->mTutorial)
		mTutorialShown = false;
	if (mStartLink != NULL)
	{
		mWidgetManager->RemoveWidget(mStartLink);
		mApp->SafeDeleteWidget(mStartLink);
		mStartLink = NULL;
	}
	mLevel = 0;
	mScore = 0;
	mSavedLevels = 1;
	mLives = 3;
	mLevelOver = false;
	mApp->mFlag4c4 = false;
	mSeconds = 0;
	mStartTick = GameTickCount();
	mKills = 0;
	mKillsTotal = 0;
	mUnused_a0 = 0;
	mGameStartCnt = 0;
	NewLevel();
	if (theLoad)
		LoadGame();
}

// 0x401580: a new level (or the tutorial, the bonus screen, the game over)
void Board::NewLevel()
{
	mLevelStartCnt = mUpdateCnt;
	mLevelComplete = false;
	mApp->mTalkWidget->mMode = -1;
	mShovelX = -1;
	mShovelY = -1;
	mBonusReady = false;
	for (int i = 0; i < GRID * GRID; i++)
		(&mPoints[0][0])[i] = 0;
	mDirty = true;
	mUnused_95008 = 0;

	if (!AllPortsDone())
	{
		if (!mLevelOver)
			mLevelComplete = true;
		return;
	}
	if (mLives < 1)
	{
		GameOver();
		return;
	}
	mLifeFlash = 0;
	mLevel++;
	ClearPorts();
	ClearAnims();
	ClearShots();
	ClearShips();
	ClearCellFlash();

	if (!mApp->mTutorial)
	{
		int aLevel = mLevel;
		if (aLevel % 10 == 0)
		{
			mApp->SetMusicOrder((aLevel / 10) % 4 + 0x1f, true);
			mBonus = true;
		}
		else
		{
			mApp->SetMusicOrder(abs((aLevel - 1) % 0xb), true);
			mBonus = false;
		}
	}
	else
	{
		mApp->SetMusicOrder(0x14, true);
		mBonus = false;
	}

	if (mApp->mTutorial)
	{
		SetupTutorial();
		return;
	}
	if (mBonus)
	{
		mState = STATE_IDLE;
		for (int i = 0; i < 81; i++)
			(&mBonusGrid[0][0])[i] = -1;
		mBonusState = 0;
		mApp->ShowBonusNag();
		return;
	}

	mMoveProgress = 0;
	mPlayerX = 5;
	mPlayerY = 5;
	mHeading = 4;
	mWantHeading = 4;
	mShipAnim = CrtRand() % 8;
	mBlink = false;
	for (int i = 0; i < 9; i++)
		mGunFlash[i] = -1;
	mLevelOver = false;
	mKills = 0;

	int aRank = mLevel + (mApp->mDifficulty - 2) * 10;
	int aTens = aRank / 10;
	int aOnes = aRank % 10;
	int aNumShips = aTens + 4;     // pirates (type 0)
	if (aOnes == 2)
		aNumShips = aTens + 5;
	if (aOnes == 3)
		aNumShips = aNumShips + 2;
	int aNumRed = 0;               // type 1
	if ((aOnes == 4) || (aOnes == 6))
		aNumRed = aTens + 1;
	if (aOnes == 8)
		aNumShips = aNumShips + 4;
	int aNumBlue = 0;              // type 3
	if (aOnes == 7)
		aNumBlue = aTens + 1;
	int aNumGhost = 0;             // type 4
	if (aOnes == 9)
		aNumGhost = aTens + 1;
	int aNumIslands = aNumShips - 2;
	bool aNormal = true;
	if (aRank < 0)
	{
		aRank = aRank + 10;
		aNormal = false;
		aNumRed = 0;
		aNumBlue = 0;
		aNumGhost = 0;
		aNumIslands = 4;
		if (aRank % 2 == 0)
			aNumIslands = 3;
		aOnes = aRank % 10;
		if ((aOnes == 1) || (aOnes == 2) || (aOnes == 7))
			aNumShips = 3;
		else if ((aOnes == 3) || (aOnes == 4) || (aOnes == 8))
			aNumShips = 4;
		else if ((aOnes == 5) || (aOnes == 6) || (aOnes == 9))
			aNumShips = 5;
		if ((aOnes == 7) || (aOnes == 8) || (aOnes == 9))
			aNumRed = 1;
		aNumIslands = (aNumShips - 2) * aNumIslands;
	}
	if (0x1e < aRank)
		aNumRed = aNumRed - 2 + aRank / 10;

	int aTaken[GRID * GRID];   // local_1f0: cells that a ship stands on
	for (int i = 0; i < GRID * GRID; i++)
		aTaken[i] = 0;
	for (int i = 0; i < GRID; i++)
		for (int j = 0; j < GRID; j++)
		{
			mVisited[i][j] = false;
			mTerrain[i][j] = 0;
		}

	// the islands
	while (aNumIslands > 0)
	{
		int aX = 0;
		int aY = 0;
		bool aFree = false;
		bool aNear = true;
		bool aCorner = true;
		for (;;)
		{
			if (aFree && !aNear && !aCorner)
				break;
			aX = CrtRand() % 0xb;
			aY = CrtRand() % 0xb;
			if (aX < 0)
				aX = -aX;
			if (aY < 0)
				aY = -aY;
			bool aNearX = (aX >= mPlayerX - 2) && (aX <= mPlayerX + 2);
			bool aNearY = (aY >= mPlayerY - 2) && (aY <= mPlayerY + 2);
			aFree = mTerrain[aX][aY] == 0;
			aNear = aNearX && aNearY;
			aCorner = ((aX == kCornerMin) || (aX == kCornerMax)) && ((aY == kCornerMin) || (aY == kCornerMax));
		}
		if (CrtRand() % 2 == 0)
			mTerrain[aX][aY] = 2;
		else
			mTerrain[aX][aY] = 1;
		aNumIslands--;
	}

	// the four ports in the corners
	for (int aPortX = 0; aPortX < GRID; aPortX += 10)
	{
		for (int aPortY = 0; aPortY < GRID; aPortY += 10)
		{
			Port* aPort = new Port();
			aPort->mX = aPortX;
			aPort->mY = aPortY;
			aPort->mActive = true;
			aPort->mFrame = 0;
			aPort->mAppear = 0;
			mPorts.push_back(aPort);
		}
	}

	// the ships: ordinary pirates first (not on the tutorial's square), then the other kinds
	for (int aCount = aNumShips; aCount > 0; aCount--)
	{
		bool aFree = false;
		bool aNear = true;
		bool aCorner = true;
		bool aZone = true;
		int aX = 0;
		int aY = 0;
		while (!aFree || aNear || aCorner || aZone)
		{
			aX = CrtRand() % 0xb;
			aY = CrtRand() % 0xb;
			if (aX < 0)
				aX = -aX;
			if (aY < 0)
				aY = -aY;
			bool aNearX = (aX >= mPlayerX - 2) && (aX <= mPlayerX + 2);
			bool aNearY = (aY >= mPlayerY - 2) && (aY <= mPlayerY + 2);
			bool aInX = (aX >= 3) && (aX <= 7);
			bool aInY = (aY >= 2) && (aY <= 3);
			int anIdx = aX * 0xb + aY;
			aFree = (aTaken[anIdx] == 0) && (mTerrain[aX][aY] == 0);
			aNear = aNearX && aNearY;
			aZone = mTutorialShown && aInX && aInY;
			aCorner = ((aX == kCornerMin) || (aX == kCornerMax)) && ((aY == kCornerMin) || (aY == kCornerMax));
		}
		aTaken[aX * 0xb + aY] = 1;
		Ship* aShip = NewShip();
		InitShip(aShip, 0, aX, aY);
		mShips.push_back(aShip);
	}
	struct Kind { int mType; int mCount; };
	Kind aKinds[3] = {{1, aNumRed}, {4, aNumGhost}, {3, aNumBlue}};
	for (int aKind = 0; aKind < 3; aKind++)
	{
		for (int aCount = aKinds[aKind].mCount; aCount > 0; aCount--)
		{
			int aX = 0;
			int aY = 0;
			bool aFree = false;
			bool aNear = true;
			bool aCorner = true;
			while (!aFree || aNear || aCorner)
			{
				aX = CrtRand() % 0xb;
				aY = CrtRand() % 0xb;
				if (aX < 0)
					aX = -aX;
				if (aY < 0)
					aY = -aY;
				bool aNearX = (aX >= mPlayerX - 2) && (aX <= mPlayerX + 2);
				bool aNearY = (aY >= mPlayerY - 2) && (aY <= mPlayerY + 2);
				aFree = (aTaken[aX * 0xb + aY] == 0) && (mTerrain[aX][aY] == 0);
				aNear = aNearX && aNearY;
				aCorner = ((aX == kCornerMin) || (aX == kCornerMax)) && ((aY == kCornerMin) || (aY == kCornerMax));
			}
			aTaken[aX * 0xb + aY] = 1;
			Ship* aShip = NewShip();
			InitShip(aShip, aKinds[aKind].mType, aX, aY);
			mShips.push_back(aShip);
		}
	}
	if (((aOnes == 5) || (aOnes == 6)) && aNormal)
	{
		// one sea monster
		int aX = 0;
		int aY = 0;
		bool aFree = false;
		bool aNear = true;
		bool aCorner = true;
		while (!aFree || aNear || aCorner)
		{
			aX = CrtRand() % 0xb;
			aY = CrtRand() % 0xb;
			if (aX < 0)
				aX = -aX;
			if (aY < 0)
				aY = -aY;
			bool aNearX = (aX >= mPlayerX - 2) && (aX <= mPlayerX + 2);
			bool aNearY = (aY >= mPlayerY - 2) && (aY <= mPlayerY + 2);
			aFree = (aTaken[aX * 0xb + aY] == 0) && (mTerrain[aX][aY] == 0);
			aNear = aNearX && aNearY;
			aCorner = ((aX == kCornerMin) || (aX == kCornerMax)) && ((aY == kCornerMin) || (aY == kCornerMax));
		}
		aTaken[aX * 0xb + aY] = 1;
		Ship* aShip = NewShip();
		InitShip(aShip, 2, aX, aY);
		mShips.push_back(aShip);
	}

	ClearUndo(0);
	mState = STATE_IDLE;
	if (((mLevel - 1) % 10 == 0) && (mLevel != 1))
	{
		// after ten levels the map is shown
		mApp->ShowMap();
	}
	else
	{
		int aWarning;
		if (((mApp->mDifficulty == 2) && (10 < mLevel)) || (mApp->mDifficulty == 3))
			aWarning = 3;
		else if (aNumGhost > 0)
			aWarning = 1;
		else if (aNumBlue > 0)
			aWarning = 0;
		else
		{
			if (((aOnes != 5) && (aOnes != 6)) || !aNormal)
				return;
			aWarning = 2;
		}
		mApp->ShowWarning(aWarning);
	}
}

// 0x402690: the tutorial level (two ports, four ships)
void Board::SetupTutorial()
{
	mPlayerX = 5;
	mPlayerY = 5;
	mHeading = 4;
	mWantHeading = 4;
	mMoveProgress = 0;
	mShipAnim = CrtRand() % 8;
	mBlink = false;
	for (int i = 0; i < 9; i++)
		mGunFlash[i] = -1;
	mLevelOver = false;
	mKills = 0;
	for (int i = 0; i < GRID; i++)
		for (int j = 0; j < GRID; j++)
		{
			mVisited[i][j] = false;
			mTerrain[i][j] = 0;
		}
	mTerrain[6][5] = 2;     // +0x2d8
	mTerrain[4][3] = 1;     // +0x278
	mTerrain[7][8] = 1;     // +0x310
	for (int aPortX = 0; aPortX < GRID; aPortX += 10)
	{
		Port* aPort = new Port();
		aPort->mX = aPortX;
		aPort->mY = 10;
		aPort->mActive = true;
		aPort->mFrame = CrtRand() % 16;
		aPort->mAppear = 0;
		mPorts.push_back(aPort);
	}
	static const int kShips[4][3] = {{0, 2, 5}, {0, 7, 2}, {0, 8, 5}, {1, 3, 2}};
	for (int i = 0; i < 4; i++)
	{
		Ship* aShip = NewShip();
		InitShip(aShip, kShips[i][0], kShips[i][1], kShips[i][2]);
		mShips.push_back(aShip);
	}
	mTutorialStep = 0;
	mState = STATE_IDLE;
}
