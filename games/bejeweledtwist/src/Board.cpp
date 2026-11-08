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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace Sexy;

Board* Sexy::gBoard = nullptr;
int Sexy::gSettleCount = 0;
MTRand Sexy::gUnknown85B970;

int Sexy::Rand85B970()
{
	return (int)gUnknown85B970.Next();
}
// The static initialiser 0x7d5096 makes all ten Color() (opaque black); the first board's constructor copies the
// seven colours over the first seven, and the last three stay black (the credits' sparks are black until a game starts)
Color Sexy::gGemColors[10];

namespace {

// 0x85c378: the colours the constructor copies into gGemColors
const Color kGemColorTable[7] = {Color(255, 32, 32), Color(255, 255, 255), Color(0, 255, 0), Color(255, 255, 0),
	Color(255, 0, 255), Color(255, 128, 0), Color(0, 128, 255)};

// 0x85a9d0 (0x18 bytes) and 0x85aef0: globals the constructor clears (their use not yet known)
int gUnknown85A9D0[6] = {};
int gUnknown85AEF0 = 0;

}  // namespace

// The plain fields come from their declarations' defaults; this keeps the order of everything else
Board::Board(WinTwistApp* theApp, int theMode, int theFlags, int theUnknown)
{
	mApp = theApp;
	// port: the original starts a CSV header in a stats buffer at app+0x1024 when it is empty (0x6b2880); the
	// buffer is never written out, so it is left out
	mName = "BOARD";
	if (theFlags != 8) {
		mApp->mUnknownF68 = -1;
		mApp->mUnknownF6C = 0;
		mApp->mUnknownF70 = 0;
	}
	mUnknown2674 = theUnknown;
	Profile* aProfile = mApp->mProfile;
	if ((theMode == 2 || theMode == 3) && (!aProfile->GetFlag1E4(14) || aProfile->mRank == -1))
		aProfile->MarkTutorialDone();
	for (int anId = -101; anId >= -104; anId--)
		mUnknown37B8[anId] = anId;  // 0x6b8c49
	gBoard = this;
	// port: -seed=N replaces the time, so that test runs repeat
	gUnknown85B970.SRand(mApp->mHasSeedParam ? (unsigned int)mApp->mRandSeed : (unsigned int)time(nullptr));
	gUnknown85AEF0 = 0;
	mUnknown3924 = mApp->mUnknownF85 ? 0.0f : 1.0f;
	mUnknown1F88 = Color(0, 0, 0, 0);
	// 0x6a634f
	mUnknown25B4 = 0;
	mUnknown25AC = mUnknown25A8;
	std::memset(gUnknown85A9D0, 0, sizeof(gUnknown85A9D0));
	// 0x6ab455
	int aCount = 0;
	for (size_t i = 0; i < aProfile->mBools80.size(); i++) {
		if (aProfile->mBools80[i])
			aCount++;
	}
	mUnknown1328 = aCount;
	static const int kRolls[25] = {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 5};
	mUnknown21E0 = kRolls[Rand85B970() % 25];  // 0x6a3447: the board's own generator
	if (gApp->mProfile->mGamesPlayed == 0)
		mUnknown21E0 = 3;
	mZOrder = -5;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mShineImage = IMAGE_GEM_LIGHTING;
	mGridBarTop = IMAGE_INGAMEUI_GRIDBAR_TOP;
	mGridBarBot = IMAGE_INGAMEUI_GRIDBAR_BOT;
	mUnknown211C = new BoardStepper(mApp);  // 0x74c8c8
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mUnknown37C4)
		std::memset(mApp->mUnknownC74, 0, sizeof(mApp->mUnknownC74));
	std::memset(mUnknown2234, 0, sizeof(mUnknown2234));  // 0x6a69f0
	std::memset(mUnknown3560, 0, sizeof(mUnknown3560));
	mGameMode = theMode;
	mModeFlags = theFlags;
	LoadConfig();
	if ((theFlags & 8) == 0)
		mApp->mLastGameMode = mGameMode;
	mUnknown1FBC = mUnknown1FB8;
	mUnknown1FC0 = mUnknown1FB8;
	mUnknown2190 = mUnknown1FBC;
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
			aPoint->mUnknown18 = 1.0f;
			aPoint++;
		} while (aCol < 588);
		aRow += 84;
	} while (aRow < 588);

	mUnknown1C2C = (float)(mWidth / 2);
	mUnknown1C30 = (float)(mHeight / 2);
	mUnknown1578 = FONT_SCORE;
	mUnknown157C = FONT_SCORE;
	mUnknown15F0 = new MemoryImage(mApp);
	mUnknown15F0->Create(42, 42);
	mUnknown15F0->SetImageMode(true, true);
	mUnknown1584 = mUnknown1588;
	mUnknown20E0 = (float)(mApp->mWidth / 2 - GetImageOffsetX(0x3d6));
	if (mModeFlags == 8)
		mUnknown20E0 = (float)((double)mUnknown20E0 * 0.75);
	if (mApp->mTwistMusic != nullptr)
		mApp->mTwistMusic->SetState(0);

	mGoodTube = new GoodTube(this);
	mGoodTube->SetShown(true);
	mGoodTube->SetUnknown14D(true);
	mGoodTube->SetUnknown158(0);
	mGoodTube->SetUnknown15C(0);
	mGoodTube->SetUnknown160(1);
	if (theMode == 2) {
		mUnknown158 = 1;
		mGoodTube->SetUnknown14D(false);
		mGoodTube->SetUnknown158(4);
		mGoodTube->SetUnknown160(2);
		mGoodTube->SetChallenge(true);
	} else if (theMode == 3) {
		mGoodTube->SetColor(Color(0, 255, 0), true);
		mGoodTube->SetUnknown15C(1);
	}

	mFXManager = new EffectsManager();
	// 0x6f5ea6 makes the first manager's height image when there is a device and none yet
	mFXManager->MakeHeightImage();
	mFXManager->mUnknownB6 = true;  // +0xb6
	mFXManager->mUnknownA1 = true;  // +0xa1: the depth test goes off first (0x6f6451)
	mFXManager2 = new EffectsManager();
	mFXManager3 = new EffectsManager();
	mPointsManager = new PointsManager();
	mPointsManager->mMouseVisible = false;
	mReplayPod = nullptr;
	mApp->mWidgetManager->SetFocus(this);
	mUnknown21B0 = 0.0f;
	mUnknown21BC = 0.0f;
	mUnknown21C0 = 0.0f;
	mUnknown37B1 = true;
	mUnknown37B0 = true;
	mUnknown37B4 = 15;
	switch (mGameMode) {
	case 2:
		mUnknown154 = 1;
		mUnknown37B0 = false;
		mUnknown37B1 = false;
		break;
	case 3:
		mUnknown154 = 0;
		mUnknown3458 = 30000;  // 0x6a697d
		mUnknown137D = true;
		break;
	case 5:
		if (mUnknown158 == 0)
			mUnknown158 = 1;
		break;
	default:
		mUnknown154 = 0;
		break;
	}
	mUnknown2224 = 0.0f;
	mUnknown23D4 = 0;
	mUnknown23D8 = 0.0f;
	// 0x6a3fff
	mUnknown214 = false;
	mUnknown220 = 1.0f;
	mUnknown2644 = mBonusChallengeMinGems;
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
	mUnknown150C = true;
	mUnknown1559 = true;
	std::memset(mUnknown133C, 0, sizeof(mUnknown133C));
}

void Board::ReadConfig(bool theResize)
{
	int anOldRows = mRows;
	int anOldCols = mCols;
	int aRows = mApp->GetInteger("GameRows", 8);
	int aCols = mApp->GetInteger("GameColumns", 8);
	mUnknown1C68 = 3;
	mCoalMax = mApp->GetInteger("CoalMax", 0);
	mFruitFirstLevel = mApp->GetInteger("FruitFirstLevel", 8);
	int aGap = 1;
	if (0 < mApp->GetInteger("BonusRoundLevelGap", 3))
		aGap = mApp->GetInteger("BonusRoundLevelGap", 3);
	mBonusRoundLevelGap = aGap;
	mMaxLocksOnScreen = mApp->GetInteger("MaxLocksOnScreen", 8);
	mUnknown1BF8 = true;
	mUnknown1C6C = true;
	mUnknown1C6E = true;
	mUnknown1BFC = 7;
	mUnknown1FB4 = false;
	mUnknown1C6D = false;
	mUnknown1C6F = false;
	mUnknown1C70 = false;
	mUnknown1C71 = false;
	mCounter.Read("Counter");
	mBadGemsOnBoardMax.Read("BadGemsOnBoardMax");
	mLockGem.Read("LockGem");
	mCoalGem.Read("CoalGem");
	mSkulls.Read("Skulls");
	mLevelUpPoints.Read("LevelUpPoints");
	mDevil.Read("Devil");
	mUnknown1C98 = mApp->GetBoolean("IceGemsAllowed", false);
	mUnknown1C79 = true;
	mUnknown1C78 = true;
	mUnknown1C7A = false;
	mUnknown2138 = false;
	mUnknown1C7B = false;
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
	delete[] mUnknown1370;
	mUnknown1370 = new int[mCols];
	for (int i = 0; i < mCols; i++)
		mUnknown1370[i] = 0;
}

void Board::ResizeBoard(int theRows, int theCols)
{
	if (theRows == 0)
		theRows = mRows;
	if (theCols == 0)
		theCols = mCols;
	mUnknown2204 = 0;
	mUnknown2180 = 0;
	mUnknown2184 = 0;
	mUnknown16C = 0;
	mUnknown14FC = 0;
	ClearBoard();
	AllocGrid(theRows, theCols);
	MakeCells();
	NewLevel();
}

void Board::ClearBoard()
{
	for (size_t i = 0; i < mUnknown149C.size(); i++)
		delete static_cast<ElectroStrike*>(mUnknown149C[i]);  // 0x71eb9e
	mUnknown149C.clear();  // 0x6af6b6
	for (size_t i = 0; i < mUnknown14AC.size(); i++)
		delete mUnknown14AC[i];
	mUnknown14AC.clear();  // 0x6af6f1
	mUnknown1430 = nullptr;
	if (mUnknown21D9) {
		mUnknown21D9 = false;
		mUnknown15C = 0;
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
	delete[] mUnknown135C;
	mRows = theRows;
	mCols = theCols;
	mGrid = new Piece*[theRows * theCols];
	std::memset(mGrid, 0, mCols * mRows * sizeof(Piece*));
	mUnknown135C = new Piece*[mCols * mRows];
	std::memset(mUnknown135C, 0, mCols * mRows * sizeof(Piece*));
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
	if (thePiece == mUnknown1430)
		mUnknown1430 = nullptr;
	mGrid[mCols * thePiece->mRow + thePiece->mCol] = nullptr;
	if (theDelete)
		delete thePiece;  // 0x6a54ac
}

void Board::NewLevel()
{
	mUnknown3904 = false;
	mUnknown1EC = mUnknown1E8;
	mUnknown1508 = 0;
	mUnknown262A = false;
	mUnknown1C90 = false;
	mUnknown1F70 = mUnknown158 % 7;
	// 0x6a634f
	mUnknown25B4 = 0;
	mUnknown25AC = mUnknown25A8;
	mUnknown21A3 = false;
	delete mBackdrop;
	mBackdrop = nullptr;
	// 0x6f3869: images\<art size>\backdrops\twist-back<n>.png; none of the twelve is in the game's files, so the
	// original's load always fails and the backdrop's settings (+0x15fc, the image's +0x92, +0x44 and its slots 20
	// and 32) are never made
	char aPath[64];
	std::snprintf(aPath, sizeof(aPath), "images/%d/backdrops/twist-back%d.png", mApp->mArtRes, mUnknown158 % 12 + 1);
	if (mApp->FileExists(aPath))
		NotPorted("Board::NewLevel's backdrop (0x6bf659)");
	mUnknown2134 = 1.0f;
	mUnknown1C44 = 0;
	mUnknown20F0 = 0;
	mShineAmount = 1.0f;
	if (0 < mUnknown158 && mGameMode != 5 && !GetUnknown23F5())
		mUnknown2041 = true;
	std::memset(mUnknown2234, 0, sizeof(mUnknown2234));
	SetStat(0x24, mUnknown158);
	mUnknown2234[0x24] = mUnknown158;
	// (not while the basics tutorial is up)
	if (mUnknown2584 == nullptr || mUnknown2584->mKind != 0xe) {
		if (mLevelUpPoints.GetCount() < 500)
			mUnknown204 = 500;
		else
			mUnknown204 = mLevelUpPoints.GetCount();
	}
	int aMode = mGameMode;
	if (aMode == 4 && 100000 <= mUnknown204)
		mUnknown204 = 100000;
	if (!mUnknown21A2) {
		mUnknown14F4 = 0;
		mUnknown1F6C = false;
		mUnknown2201 = true;
		mUnknown208 = mUnknown168;
		mUnknown200 = 0;
		if (aMode != 2 && aMode != 3)
			mGoodTube->SetProgress(0.0f, true);
		mUnknown214 = false;
		mUnknown164 = false;
		mUnknown230 = 0;
		mUnknown238 = 0;
		mUnknown234 = 0;
		mUnknown228[1] = 0;
		mUnknown228[0] = 0;
		mUnknown244 = 0;
		mUnknown23C = 0;
		mUnknown240 = 0;
		mUnknown1EC = 0;
		mUnknown1E8 = 0;
		mUnknown21F8 = 100;  // 0x6a3ff4
		ShowTubeLevel();
		ResetLevelValues();
		mLockGem.Restart();
		mCounter.Restart();
		mSkulls.Restart();  // 0x6a4043
		// 0x6a49a3
		mUnknown2590 = Ftol((double)gGemWidth * (double)0.24f + (double)(mCols * gGemWidth));
	}
	if (mGameMode != 2)
		UpdateProgress(false);
	mUnknown2598 = 1.0f;
	mUnknown2594 = 1.0f;
	mUnknown259C = 1.0f;
	if (1 < mUnknown158)
		mUnknown21A1 = true;
	if (mUnknown23F5) {
		// 0x75fc8a
		if (mApp->mBoard == nullptr || !mApp->mBoard->mUnknown37C4)
			std::memset(mApp->mUnknownC74, 0, sizeof(mApp->mUnknownC74));
	}
	mUnknown2644 = mBonusChallengeMinGems;
	mUnknown2668 = true;
	if (mGameMode != 2 && (mGameMode == 3 || 1 < mUnknown158) && mBonusColors.empty())
		StartBonusChallenge(-1);
}

void Board::SetStat(int theStat, int theValue)
{
	mUnknown2234[theStat] = theValue;
	if (!mUnknown37C4)
		mApp->mUnknownC74[theStat] = theValue;
}

void Board::ResetLevelValues()
{
	if (mGameMode == 4 || mGameMode == 5)
		mCounter.mFirstChangeLevel = mUnknown158 + 10;
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
	if (mGoodTube->mUnknown15C == 0)
		mGoodTube->SetProgress((float)mUnknown200 / (float)mUnknown204, theAtOnce);
}

void Board::UpdateProgress(bool theNoCheck)
{
	UpdateTubeProgress(theNoCheck);
	CheckLevelDone();
	if (mGameMode == 1) {
		float aFrac = (float)mUnknown200 / (float)mUnknown204;
		if (!mUnknown178.IsInitialized())
			mUnknown178.SetCurve(GTS(0x7dcf78));
		mUnknown178.SetInVal((double)aFrac);
		mLockGem.mUnknown48 = mUnknown178.GetOutVal();
		mCounter.mUnknown48 = mUnknown178.GetOutVal();
	}
	if (!theNoCheck)
		CheckCombos();
}

bool Board::CheckLevelDone()
{
	if (mUnknown37C4 || mGameMode == 2 || mGameMode == 3)
		return false;
	if (!mUnknown21A3) {
		if (mUnknown200 < mUnknown204)
			return false;
		if (mReplayPod != nullptr && mReplayPod->mState != ReplayWidget::STATE_GOING && mReplayPod->mVisible) {
			// the pod goes down for the level's end, to come back after the rank bar
			mUnknown2628 = true;
			mReplayPod->mState = ReplayWidget::STATE_GOING;
			mReplayPod->mPopped = false;
		}
		mUnknown21A3 = true;
		bool aBonus = !(mGameMode == 0 || mGameMode == 5);
		if (mUnknown23F5)
			aBonus = false;
		if (aBonus && 0 < mUnknown158 && !mUnknown21A2)
			mUnknown269C = GetBonusLevel(mUnknown158);
	}
	return true;
}

void Board::StartBonusChallenge(int theExcludeColor)
{
	int aMin = mBonusChallengeMinGems;
	int aMax = mUnknown158 / 3 + aMin;
	if (mBonusChallengeMaxGems < aMax)
		aMax = mBonusChallengeMaxGems;
	int aRoll = Rand85B970();  // 0x6a3447, as are the colours below
	int aPoints = mUnknown2640 * 100;
	mUnknown2644 = aRoll % (aMax - aMin + 1) + aMin;
	if (mUnknown2644 == mBonusChallengeMinGems)
		aPoints += 1000;
	else if (mUnknown2644 - mBonusChallengeMinGems == 1)
		aPoints += 2500;
	else
		aPoints += 5000;
	mUnknown2660 = 0;
	mBonusColors.clear();
	mUnknown2658 = aPoints;
	unsigned int i = 0;
	if (mUnknown2644 != 0) {
		do {
			int aColor;
			for (;;) {
				aColor = Rand85B970() % 7;
				if (i < 2 && aColor == theExcludeColor)
					continue;
				if (!mBonusColors.empty() && aColor == mBonusColors.back())
					continue;
				break;
			}
			mBonusColors.push_back(aColor);
			i++;
		} while (i < (unsigned int)mUnknown2644);
	}
	if (mUnknown265C == 1) {
		mGoodTube->mUnknown3A8 = 0;
		mUnknown2664 = -1;
		mUnknown265C = 3;
		mGoodTube->mUnknown5F0.SetCurve(GTS(0x7dcf14));
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

Board::~Board()
{
	// port: the rest of the destructor comes with the board's update
	// The original frees the list of smoke without deleting the smoke; the port deletes it, without its points or sparks
	// The original never deletes blitz's final score
	delete mUnknown3464;
	delete mUnknown3468;
	delete mUnknown211C;  // 0x6bf0bb
	delete mUnknown38F0;
	delete mUnknown38F4;
	for (Traveller* aTraveller : mUnknown2170) {
		if (SmokeTraveller* aSmoke = dynamic_cast<SmokeTraveller*>(aTraveller)) {
			aSmoke->mNoPoints = true;
			aSmoke->mBurst = false;
		}
		delete aTraveller;
	}
	mUnknown2170.clear();
	RemoveWidget(mPointsManager);
	delete mPointsManager;
	// The same holds for the shockwaves
	for (Shockwave* aWave : mUnknown1660)
		delete aWave;
	mUnknown1660.clear();
	for (GemSoul* aSoul : mUnknown26B0)
		delete aSoul;
	mUnknown26B0.clear();
	for (FlyingText* aText : mFlyingTexts)
		delete aText;
	mFlyingTexts.clear();
	// The dialogs still waiting, then the one up, are taken off the screen and deleted (0x6befa1)
	for (TwistWidget* aDialog : mUnknown23DC) {
		mApp->mWidgetManager->RemoveWidget(aDialog);
		delete aDialog;
	}
	mUnknown23DC.clear();
	if (mUnknown2208 != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mUnknown2208);
		delete mUnknown2208;
		mUnknown2208 = nullptr;
	}
	if (mReplayPod != nullptr) {
		RemoveWidget(mReplayPod);
		delete mReplayPod;
	}
	if (mRewindSound != nullptr)
		mRewindSound->Release();
	delete[] mUnknown1370;
	delete[] mUnknown135C;
	delete[] mCells;
	delete[] mGrid;
	if (mUnknown267C != nullptr) {
		// a fruit dance still playing, and the background back at its speed
		RemoveWidget(mUnknown267C);
		delete mUnknown267C;
		mUnknown267C = nullptr;
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

void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	if (mApp->mMusicInterface != nullptr) {
		if (mUnknown3928 == 0 && mUnknown392C == 0) {
			// port: the original keeps BASS's position in the music playing (0x6ee282) here; nothing reads it back
			// (PlayMusic drops it), so the port keeps 0
		}
		mApp->mMusicInterface->ResumeAllMusic();
	}
	if (mUnknown2584 != nullptr && mUnknown2584->mKind == 0xe) {
		mApp->mProfile->MarkTutorialDone();
		mUnknown37C4 = true;
	}
	if (mUnknown37C4) {
		// (the exe clears its sound manager's flag (+0x300a0) that the menus' replays set, which sends new sounds
		// through DirectSound's parametric equaliser; the port's sound manager has none)
	}
	// a challenge met (or its goal checked as met) is over as well
	if (mGameMode == MODE_CHALLENGE && (mApp->mChallengeInfo->mResult == 1 || mUnknown266A))
		mUnknown36B0 = true;
	if (!mUnknown37C4 && !mUnknown1390) {
		if (mUnknown36B0) {
			// a night challenge's new best goes into the profile and onto its planet, with the planet's tip
			if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr &&
				(mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0 &&
				mApp->mProfile->mInts204[mChallenge->mCategory] < mUnknown2634) {
				mApp->mProfile->mInts204[mChallenge->mCategory] = mUnknown2634;
				if (mApp->mMainMenu != nullptr && mApp->mMainMenu->mChallengeMenu != nullptr && mChallenge != nullptr &&
					(mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) != 0) {
					ChallengeButton* aButton = mApp->mMainMenu->mChallengeMenu->mButtons[mChallenge->mCategory];
					aButton->mProgress = mUnknown2634;
					aButton->MakeTip();
				}
			}
			// the game is over: its save goes
			if ((unsigned int)mGameMode < 4)
				mApp->EraseFile(mApp->GetSaveName(mGameMode, mUnknown2674));  // 0x763a85
		} else {
			Buffer aBuffer;
			// a game left in the middle of a move goes back to the level's start (or the replay's)
			if (mUnknown2629)
				LoadGame(mUnknown23B4);
			else if (((!mUnknown25A5 || mUnknown160 != 0) && mSaveBuffer.GetDataLen() != 0) || IsBusy())
				LoadGame(mSaveBuffer);
			SaveGame(&aBuffer, true);
			if (0 < mUnknown158 && !IsTimed())
				mApp->WriteBufferToFile(mApp->GetSaveName(mGameMode, mUnknown2674), &aBuffer);
		}
	}
	if (mGrid != nullptr) {
		ClearBoard();
		delete[] mGrid;
		mGrid = nullptr;
	}
	if (mUnknown1488 != 0)  // never reached: only the constructor (0x6c582b) writes +0x1488, a 0
		NotPorted("Board::RemovedFromManager: +0x1488");  // 0x6a404e: its slot 8, and +0x1488 = 0
	mApp->SetCursor(CURSOR_POINTER);
	TwistWidget::RemovedFromManager(theWidgetManager);
	if (mUnknown139C != nullptr)
		theWidgetManager->RemoveWidget(mUnknown139C);
	if (mHyperspace != nullptr)
		mApp->mWidgetManager->RemoveWidget(mHyperspace);
	mApp->mWidgetManager->RemoveWidget(mFXManager);
	RemoveWidget(mGoodTube);
	RemoveWidget(mPointsManager);
	if (mUnknown26A0 != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mUnknown26A0);
		mUnknown26A0 = nullptr;
	}
	mApp->mWidgetManager->RemoveWidget(mUnknown2208);
	// (and the puzzle's panel, +0x23d4, which no game has: no menu starts a puzzle)
}

void Board::EndGame(bool theNuke)
{
	if (mUnknown36B0)
		return;
	if (!theNuke)
		mUnknown26CC = nullptr;
	mUnknown36B0 = true;
	mUnknown36B1 = theNuke;
	mApp->mUnknownF68 = mApp->CheckForHighScore(mGameMode, mApp->mProfile->mName, mUnknown158, mUnknown168);
	mApp->mUnknownF6C = mUnknown168;
	mApp->mUnknownF70 = mUnknown158;
	mApp->mProfile->mGamesPlayed++;
	mApp->mProfile->mUnknown140[mGameMode]++;
	if (mUnknown1488 != 0)  // never reached: only the constructor (0x6c582b) writes +0x1488, a 0
		NotPorted("Board::EndGame: +0x1488");  // 0x6a404e: its slot 8, and +0x1488 = 0
	mApp->PlayVoice(SOUND_VOICE_GAMEOVER, false);
	mUnknown1430 = nullptr;
	mUnknown1438 = false;
	mApp->KillDialog(40);
	if (mApp->mTwistMusic != nullptr)
		mApp->mTwistMusic->SetState(0);
	mUnknown160 = 7;
	mUnknown26C8 = 1;
	mUnknown14EC = (int)((unsigned int)(mUpdateCnt - mUnknown14DC) / 100) + mUnknown14E8;
	mApp->mUnknown75C++;
	mUnknown20D0 = false;
	mUnknown1FFC = 0x41;
	if (mGameMode != MODE_BLITZ || mUnknown36B1) {
		if (mApp->mMusicInterface != nullptr) {
			mApp->mMusicInterface->StopAllMusic();
			mApp->PlayMusic(200 + (mUnknown36B1 ? 1 : 0), false, false, 0, 0);
		}
		if (mUnknown36B1) {
			// The board blows up
			mApp->PlaySample(mUnknown3930 ? SOUND_DOOMGEMGAMEOVER : SOUND_ENDFINAL6B);
			mUnknown27B8.SetCurve(GTS(0x7dc490));
			mUnknown2828.SetCurve(GTS(0x7dc464), &mUnknown27B8);
			mUnknown26D8.SetCurve(GTS(0x7dc448), &mUnknown27B8);
			mUnknown2748.SetCurve(GTS(0x7dc42c), &mUnknown27B8);
			mUnknown2908.SetCurve(GTS(0x7dc40c), &mUnknown27B8);
			mUnknown2978.SetCurve(GTS(0x7dc3ec), &mUnknown27B8);
			if (mUnknown26CC != nullptr)
				mUnknown26CC->mUnknown2F6 = false;
			mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
			return;
		}
	}
	mUnknown26D0 = new GameOverShow(this);
	AddWidget(mUnknown26D0);
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
}

void Board::StartGame()
{
	if (!mApp->mPlayPressed) {
		Buffer aBuffer;
		mApp->ReadBufferFromFile(mApp->GetSaveName(mGameMode, mUnknown2674), &aBuffer, true);
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
					mApp->mProfile->SetFlag1E4(aFlag, true);
				}
			}
		}
		SetScore(mUnknown168, true);
		if (mGameMode == MODE_CHALLENGE)
			UpdateChallengeTube(true);
		mUnknown16C = mUnknown168;
		mUnknown1EC = mUnknown1E8;
		mUnknown1F4 = mUnknown1F0;
		mGoodTube->SetLevel(mUnknown158);
		mShineAmount = 0;
		if (mBonusColors.size() != 0) {
			mGoodTube->mUnknown330 = 1.0f;
			mGoodTube->mUnknown3F0 = 1.0f;
			// The tube's Update (slot 22, 0x702736); only a loaded game gets here
			mGoodTube->Update();
		}
		mApp->mPlaySamples = true;
		if (aLoaded)
			mUnknown158--;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					aPiece->mUnknown535 = false;
			}
		}
		if (mGameMode == MODE_BLITZ) {
			mGoodTube->SetUnknown15C(1);
			mGoodTube->SetUnknown4BC(30000);  // 0x6a697d
			float aValue;
			if (!mApp->mPlayPressed) {
				mGoodTube->SetTime(mUnknown3458, true);
				mGoodTube->StartUnknown4AC(true, false);
				SetBlitzColor();
				UpdateTubeProgress(true);
				aValue = 1.5f;
				mUnknown160 = 5;
			} else {
				aValue = 3.5f;
			}
			mUnknown345C = aValue;
			mGoodTube->SetUnknown4A8(mApp->mPlayPressed);
		}
		// (an unregistered copy also empties the tube from zen's level 14 on: registration code, left out)
		if (!aLoaded)
			mApp->mUnknownC74[mGameMode]++;
	}
	mUnknown1559 = true;
	if (gApp->mProfile->mGamesPlayed == 0 && (mGameMode == MODE_ZEN || !mApp->SaveExists(MODE_ZEN, 0))) {
		if (mGameMode == MODE_CLASSIC)
			return;
		if (!mApp->SaveExists(MODE_CLASSIC, 0))
			return;
	}
	mApp->mProfile->SetFlag1E4(0x11, true);
	mUnknown1559 = false;
}

void Board::TakeHyperspace(Hyperspace* theHyperspace)
{
	mUnknown1618 = 1.0;
	mUnknown20DC = 2;
	if (mUnknown160 < 1)
		mUnknown20EC = 4.0f;
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
		mUnknown21A2 = false;
		// port: the tube's effects manager (+0x28c) is cleared, once it is ported
	} else {
		mApp->mVizManager->SetVisible(false);
		mHyperspace = theHyperspace;
		mApp->mWidgetManager->AddWidget(theHyperspace);
		mHyperspace->mFlag15A = true;
		int aTransition;
		if (mGameMode == MODE_CHALLENGE) {
			mUnknown2041 = true;
			mGoodTube->SetShown(true);
			aTransition = 9;
		} else if (!gApp->mPlayPressed) {
			aTransition = 0xc;
		} else if (gApp->mProfile->GetFlag1E4(0xe) || mGameMode == MODE_BLITZ) {
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
	mUnknown1378 = 1;
	mUnknown13B0.SetOutRange(0, 0);
	mUnknown13B0.SetInVal(0);
	ReadConfig(true);
	if (!gApp->mProfile->GetFlag1E4(0xe) && mGameMode != MODE_BLITZ && mGameMode != MODE_CHALLENGE) {
		QueueTip(0xe, nullptr);
		ShowNextTip();
		mUnknown37B0 = false;
		mUnknown37B1 = false;
		if (mApp->mUnknownF85)
			mUnknown2584->Preload();
	} else if (mGameMode != MODE_ZEN) {
		if (mGameMode == MODE_PUZZLE) {
			mUnknown2042 = false;
			mUnknown2054 = true;
			// 0x6c3515(mUnknown2034, true), the puzzle's board, which draws from Rand. Never reached: no menu starts a
			// puzzle (mode 5)
			NotPorted("Board::StartLevel: the puzzle's board");
		} else {
			mUnknown2041 = true;
			mUnknown2042 = true;
		}
	}
	// (onto whatever mSaveBuffer holds: StartLevel does not clear it)
	SaveGame(&mSaveBuffer, false);
	LayoutTube();
}

bool Board::ReadSaveHeader(Buffer& theBuffer, int* theVersion, int* theMode, int* theFlags, int* theUnknown154,
	int* theLevel)
{
	theBuffer.SeekFront();
	int aVersion = theBuffer.ReadLong();
	*theVersion = aVersion;
	if (0x3b < aVersion) {
		*theMode = theBuffer.ReadLong();
		*theFlags = theBuffer.ReadLong();
		*theUnknown154 = theBuffer.ReadByte();
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
	aBytes[4] = (char)theValue.mUnknown50;
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
	theValue->mUnknown51 = true;
	if (theBytes.size() < 5)
		return;  // (a damaged save: the original copies whatever lies past the string)
	std::memcpy(&theValue->mValue, theBytes.data(), 4);
	theValue->mUnknown50 = theBytes[4] != 0;
}

// 0x6ace0a: four bytes of the board from a string
void ReadFourBytes(void* theData, const std::string& theBytes)
{
	if (theBytes.size() < 4)
		return;  // (likewise)
	std::memcpy(theData, theBytes.data(), 4);
}

}  // namespace

bool Board::LoadGame(Buffer& theBuffer)
{
	int anOldLevel = mUnknown158;
	int aVersion = 0;
	if (!ReadSaveHeader(theBuffer, &aVersion, &mGameMode, &mModeFlags, &mUnknown154, &mUnknown158))
		return false;
	ClearBoard();
	LoadConfig();
	mUnknown1C40 = false;
	mUnknown1C42 = false;
	mUnknown15C = theBuffer.ReadLong();
	mUnknown160 = theBuffer.ReadLong();
	mUnknown164 = theBuffer.ReadBoolean();
	mUnknown168 = theBuffer.ReadLong();
	mUnknown14F8 = mUnknown168;
	mUnknown1E8 = theBuffer.ReadLong();
	mUnknown1EC = theBuffer.ReadLong();
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mUnknown37C4)
		std::memset(mApp->mUnknownC74, 0, sizeof(mApp->mUnknownC74));
	std::memset(mUnknown2234, 0, sizeof(mUnknown2234));  // 0x6a69f0
	int aStatCount;
	if (aVersion == 0x3a)
		aStatCount = 0x66;
	else
		aStatCount = theBuffer.ReadShort();
	// (only version 0x3a, which ReadSaveHeader refuses, has more than 84; the original writes those past the app's)
	for (int i = 0; i < aStatCount; i++) {
		int aStat = theBuffer.ReadLong();
		if (i < 0x54)
			mApp->mUnknownC74[i] = aStat;
	}
	for (int i = 0; i < aStatCount; i++) {
		int aStat = theBuffer.ReadLong();
		if (i < 0x54)
			mUnknown2234[i] = aStat;
	}
	mUnknown208 = theBuffer.ReadLong();
	mUnknown200 = theBuffer.ReadLong();
	mUnknown204 = theBuffer.ReadLong();
	mUnknown214 = theBuffer.ReadBoolean();
	mUnknown218 = theBuffer.ReadShort();
	mUnknown224 = theBuffer.ReadLong();
	// 0x6aefc8: the level's record
	mUnknown228[0] = theBuffer.ReadLong();
	mUnknown228[1] = theBuffer.ReadLong();
	mUnknown230 = (float)std::atof(theBuffer.ReadString().c_str());
	mUnknown234 = theBuffer.ReadLong();
	mUnknown238 = theBuffer.ReadLong();
	mUnknown23C = theBuffer.ReadLong();
	mUnknown244 = theBuffer.ReadLong();
	mUnknown240 = theBuffer.ReadLong();
	mUpdateCnt = theBuffer.ReadLong();
	mUnknown1FB8 = theBuffer.ReadLong();
	mUnknown1FC4 = theBuffer.ReadLong();
	mUnknown1454 = (float)((double)theBuffer.ReadLong() / 10000.0);
	mUnknown1458 = (float)((double)theBuffer.ReadLong() / 10000.0);
	mUnknown1464 = theBuffer.ReadLong();
	mUnknown1468 = theBuffer.ReadLong();
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
			mCells[mCols * aRow + aCol].mUnknown0 = theBuffer.ReadBoolean();  // 0x6a38ee
		}
	}
	// 0x456330: a state of any other size leaves the generator as it was (the original asserts)
	std::string aRandState = theBuffer.ReadString();
	if (aRandState.size() == 4 + 4 * MTRAND_N) {
		uint32_t aState[MTRAND_N];
		int anIndex;
		std::memcpy(&anIndex, &aRandState[0], 4);
		std::memcpy(aState, &aRandState[4], 4 * MTRAND_N);
		gUnknown85B970.SetState(aState, anIndex);
	}
	ReadLevelValueBytes(&mLockGem, theBuffer.ReadString());
	ReadLevelValueBytes(&mSkulls, theBuffer.ReadString());
	ReadLevelValueBytes(&mCounter, theBuffer.ReadString());
	ReadLevelValueBytes(&mBadGemsOnBoardMax, theBuffer.ReadString());
	ReadLevelValueBytes(&mUnknown1E00, theBuffer.ReadString());
	ReadLevelValueBytes(&mLevelUpPoints, theBuffer.ReadString());
	ReadLevelValueBytes(&mCoalGem, theBuffer.ReadString());
	mUnknown21D0 = theBuffer.ReadShort();
	mUnknown21F8 = theBuffer.ReadLong();
	mUnknown1F6C = theBuffer.ReadBoolean();
	mUnknown1F70 = theBuffer.ReadShort();
	gNextPieceId = theBuffer.ReadLong();
	mUnknown21DA = theBuffer.ReadBoolean();
	mUnknown2041 = theBuffer.ReadBoolean();
	mUnknown14F4 = theBuffer.ReadLong();
	mUnknown14FC = theBuffer.ReadLong();
	mUnknown21C = theBuffer.ReadLong();
	mUnknown21DC = theBuffer.ReadLong();
	mUnknown21E0 = theBuffer.ReadLong();
	mBonusColors.clear();
	int aColorCount = theBuffer.ReadShort();
	for (int i = aColorCount; i != 0; i--)
		mBonusColors.push_back(theBuffer.ReadShort());
	if (aColorCount == 0)
		mGoodTube->mUnknown330 = 0;
	mGoodTube->ResetBonus();
	mGoodTube->mUnknown3FC = 0;
	mGoodTube->mUnknown3F8 = 0;
	mUnknown2658 = theBuffer.ReadLong();
	mUnknown2660 = theBuffer.ReadShort();
	mUnknown2644 = theBuffer.ReadLong();
	mUnknown2640 = theBuffer.ReadLong();
	mGoodTube->mUnknown3A8 = mUnknown2660;
	if (aVersion < 0x39)
		mUnknown265C = 0;
	else
		mUnknown265C = theBuffer.ReadByte();
	mGoodTube->mUnknown5F0.SetConstant(mUnknown265C == 3 ? 1.0 : 0.0);
	mUnknown220 = (float)((double)theBuffer.ReadLong() / 10000.0);
	mUnknown3458 = theBuffer.ReadLong();
	mUnknown818 = theBuffer.ReadLong();
	if (theBuffer.ReadBoolean()) {
		for (WinTwistApp::UnknownFA0& aSlot : mApp->mUnknownFA0) {  // 0x6a3156
			aSlot.mUnknown20 = theBuffer.ReadShort();
			aSlot.mUnknown24 = theBuffer.ReadShort();
			aSlot.mUnknown28 = theBuffer.ReadLong();
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
			mUnknown2638 = theBuffer.ReadLong();
			mUnknown2634 = theBuffer.ReadLong();
		}
	}
	if (0x34 < aVersion) {
		mUnknown1F64 = theBuffer.ReadLong();
		mUnknown1F68 = theBuffer.ReadLong();
	}
	if (0x35 < aVersion) {
		mUnknown3908 = theBuffer.ReadLong();
		mUnknown390C = theBuffer.ReadLong();
		mUnknown3910 = theBuffer.ReadLong();
	}
	if (0x37 < aVersion)
		mUnknown1F0 = theBuffer.ReadLong();
	if (0x39 < aVersion) {
		ReadFourBytes(&mUnknown3918, theBuffer.ReadString());  // 0x6ace0a
		ReadFourBytes(&mUnknown3914, theBuffer.ReadString());
	}
	if (0x3c < aVersion) {
		theBuffer.ReadBytes((uchar*)&mUnknown3928, 4);
		theBuffer.ReadBytes((uchar*)&mUnknown392C, 4);
	}
	if (aVersion < 0x3e)
		mUnknown1FC = mApp->mUnknownC74[(0xd70 - 0xc74) / 4] + mApp->mUnknownC74[(0xd6c - 0xc74) / 4];
	else
		mUnknown1FC = theBuffer.ReadLong();
	mUnknown1FC0 = mUnknown1FB8 - 1;
	mUnknown1FBC = mUnknown1FB8 - 1;
	if (mGoodTube != nullptr)
		SetLevelSteps(mUnknown224);
	SetScore(mUnknown168, true);
	mGoodTube->mUnknown29C = 0;
	mGoodTube->mUnknown1D8 = 0;
	mGoodTube->mUnknown1E0 = 0;
	mGoodTube->mUnknown278 = (float)mGoodTube->mUnknown274;
	mGoodTube->SetLevel(mUnknown158);
	mUnknown20B8 = 0;
	mUnknown25B4 = 0;
	mUnknown21A2 = true;
	mUnknown814 = 0;
	mUnknown2598 = 1.0f;
	mUnknown2594 = 1.0f;
	mUnknown2590 = GetGridRight();
	mUnknown259C = 1.0f;
	// (the original logs "Load game ENDS" to the debugger)
	// (0x7581e5 after each branch takes the VizManager's visualizer made for the jump's ChooseViz; only with Direct3D's
	// visualizers is one made, so in 2D it does nothing)
	if (mHyperspace != nullptr) {
		// A hyperspace still running ends at once: the VizManager's background for the level (0x6c967c)
		mUnknown158--;
		mHyperspace->ChooseViz(false);
		mUnknown158++;
		mHyperspace->mParent->RemoveWidget(mHyperspace);
		mApp->SafeDeleteWidget(mHyperspace);
		mUnknown3924 = 1.0f;
		mHyperspace = nullptr;
		mUnknown26A8 = 0;
		mApp->mVizManager->TakePendingViz();  // 0x7581e5
		if (mUnknown26AC != nullptr) {
			mUnknown26AC->mParent->RemoveWidget(mUnknown26AC);
			mApp->SafeDeleteWidget(mUnknown26AC);
			mUnknown26AC = nullptr;
		}
		if (mUnknown26A0 != nullptr) {
			mUnknown26A0->mParent->RemoveWidget(mUnknown26A0);
			mApp->SafeDeleteWidget(mUnknown26A0);
			mUnknown26A0 = nullptr;
		}
		mUnknown2108 = 0;
		mUnknown20DC = 0;
		mUnknown2220 = 0;
		mMouseVisible = true;
		mUnknown37F4 = 0;
		mUnknown1C90 = false;
		mApp->mVizManager->SetVisible(true);
		mApp->mVizManager->mSpeed = 1.0;
	} else if (mUnknown158 < anOldLevel) {
		// A game loaded at a lower level than the board's: the VizManager's background through a hyperspace made for
		// it, with the level one down (0x6c9789)
		Hyperspace* aHyperspace = new Hyperspace();
		mHyperspace = aHyperspace;
		mUnknown158--;
		aHyperspace->ChooseViz(false);
		mUnknown158++;
		mGoodTube->SetLevel(mUnknown158);
		delete mHyperspace;
		mHyperspace = nullptr;
		mApp->mVizManager->TakePendingViz();  // 0x7581e5
	}
	UpdateProgress(true);
	for (GemSoul* aSoul : mUnknown26B0)
		delete aSoul;
	mUnknown26B0.clear();
	mUnknown34F0.SetConstant(0);
	mUnknown37FC = 0;
	mUnknown26A8 = 0;
	// 0x6b02ea
	for (Shockwave* aWave : mUnknown1660)
		delete aWave;
	mUnknown1660.clear();
	return true;
}

void Board::SetScore(int theScore, bool theUnknown14F8)
{
	mUnknown168 = theScore;
	if (theUnknown14F8)
		mUnknown14F8 = theScore;
	SetStat(0x49, theScore);
	UpdateProgress(theUnknown14F8);
}

void Board::SetChallenge(Challenge* theChallenge)
{
	mChallenge = theChallenge;
	mUnknown2634 = 0;
	mApp->mChallengeInfo->mResult = 0;
	if (mChallenge == nullptr) {
		mUnknown2674 = -1;
		mUnknown2678 = 0;
		return;
	}
	mUnknown2674 = theChallenge->mCategory;
	mUnknown2678 = theChallenge->mNumber;
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
		mGoodTube->mUnknown150 = mApp->mProfile->mInts204[mUnknown2674];
		if (mGoodTube->mUnknown150 > 0)
			mGoodTube->mUnknown768 = true;
	}
	if ((mChallenge->mFlags & Challenge::FLAG_TIME_LIMIT) != 0) {
		mGoodTube->SetUnknown15C(1);
		mGoodTube->StartUnknown4AC(true, false);
		mGoodTube->SetUnknown4A8(true);
		mGoodTube->SetUnknown4BC(mChallenge->mTimeGoal * 100);
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
	mGoodTube->SetUnknown158(aKind);
	if (aNightMode)
		mGoodTube->SetUnknown158(5);
	// the picture behind: the category's visualizer (Viz2d in 2D whatever it says) and backdrop
	std::string aViz = mApp->mChallengeInfo->GetViz(mUnknown2674);
	int aBackground = mApp->mChallengeInfo->GetBackground(mUnknown2674);
	if (aViz.empty())
		aViz = "RedBars";
	if (gD3DInterface == nullptr)
		aViz = "Viz2d";
	mApp->mVizManager->SetVizByName(aViz.c_str(), true);
	mApp->mVizManager->SetBackdrop(aBackground - 1, true);
	mGoodTube->SetUnknown160(2);
	UpdateChallengeTube(false);
}

void Board::RestartChallenge()
{
	SetChallenge(mChallenge);
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mUnknown37C4)
		std::memset(mApp->mUnknownC74, 0, sizeof(mApp->mUnknownC74));
	std::memset(mUnknown2234, 0, sizeof(mUnknown2234));  // 0x6a69f0
	mUnknown2614.clear();  // 0x6c4d02
	std::memset(mUnknown3560, 0, sizeof(mUnknown3560));
	mUnknown1390 = true;
	mChallenge->mProgress = 0;
	mGoodTube->mUnknown538 = 0;
	mUnknown2638 = 0;
	mUnknown2634 = 0;
	mSaveBuffer.Clear();
	mUnknown16C = 0;
	mUnknown168 = 0;
	mUnknown1F8 = 0;
	mUnknown1FC = 0;
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
	while (mUnknown2170.size() != 0) {
		delete mUnknown2170.back();
		if (mUnknown2170.size() != 0)
			mUnknown2170.pop_back();
	}
	mUnknown3928 = 0;
	mUnknown248 = 0.0f;
	mUnknown392C = 0;
	mUnknown265C = 0;
	int i = 0;
	do {
		WinTwistApp::UnknownFA0& aBest = mApp->mUnknownFA0[i];
		aBest.mUnknown20 = -1;
		aBest.mUnknown24 = -1;
		aBest.mUnknown28 = 0;
		aBest.mBuffer.Clear();
		i++;
	} while (i < 3);
	mSaveBuffer.Clear();
	// 0x75fc8a
	if (mApp->mBoard == nullptr || !mApp->mBoard->mUnknown37C4)
		std::memset(mApp->mUnknownC74, 0, sizeof(mApp->mUnknownC74));
	std::memset(mUnknown2234, 0, sizeof(mUnknown2234));  // 0x6a69f0
	mUnknown2614.clear();  // 0x6c4d02
	mGoodTube->ResetBonus();
	mGoodTube->mUnknown3FC = 0;
	mGoodTube->mUnknown3F8 = 0.0f;
	mUnknown2660 = 0;
	mUnknown2664 = 0;
	mGoodTube->mUnknown3A8 = 0;
	StartBonusChallenge(-1);
	mGoodTube->SetColor(Color(0, 255, 0), true);  // 0x401720
	std::memset(mUnknown3560, 0, sizeof(mUnknown3560));
	mUnknown1390 = true;
	mUnknown1F8 = 0;
	mUnknown1FC = 0;
	mUnknown2640 = 0;
	mUnknown16C = 0;
	mUnknown14F8 = 0;
	mUnknown168 = 0;
	mUnknown818 = 0;
	mUnknown1F4 = 0;
	mUnknown1EC = 0;
	mUnknown1F0 = 0;
	mUnknown1E8 = 0;
	SetLevelSteps(0);
	mUnknown21D0 = 1;
	mApp->FadeOutMusic();
	mUnknown3458 = 30000;  // 0x6a697d
	mGoodTube->SetTime(mUnknown3458, true);
	mUnknown160 = 3;
	NewLevel();
	ClearBoard();
	mUnknown20DC = 2;
	FillInBlanks(false);
	mUnknown20DC = 0;
	if (mApp->mVizManager != nullptr)
		mApp->mVizManager->Slot7();
}

void Board::UpdateChallengeTimer()
{
	if (mChallenge == nullptr || (mChallenge->mFlags & Challenge::FLAG_TIME_LIMIT) == 0)
		return;
	// the time left in hundredths: the limit less the seconds played (the app's +0xd00) and this second's part
	unsigned int aPlayed = (unsigned int)mApp->mUnknownC74[35];
	int aLeft = mChallenge->mTimeGoal * 100 - (int)(aPlayed * 100 + mUnknown14E4);
	if (aLeft < 0)
		aLeft = 0;
	if (1 < aPlayed) {
		int aSeconds = (aLeft + 99) / 100;
		if (mUnknown263C != aSeconds) {
			mUnknown263C = aSeconds;
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
				aProgress = (float)(1.0 - (double)(unsigned int)mApp->mUnknownC74[0x23] / (double)mChallenge->mTimeGoal);
		} else if ((mChallenge->mFlags & Challenge::FLAG_MOVES) != 0) {
			if (mChallenge->mMoveGoal == 0)
				aProgress = 0;
			else
				aProgress = (float)(1.0 - (double)(unsigned int)mApp->mUnknownC74[0x21] / (double)mChallenge->mMoveGoal);
		}
	}
	mGoodTube->SetProgress(aProgress, theAtOnce);
}

void Board::SetBlitzColor()
{
	// 0x85c420: set on the first call
	static const Color aColors[6] = {Color(255, 0, 0), Color(255, 0, 0), Color(255, 255, 0), Color(0, 255, 0),
		Color(0, 255, 0), Color(0, 255, 0)};
	int aStep = mUnknown3458 / 6000;
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
		delete mUnknown38F0;
		mUnknown38F0 = new TextImage(aFont, aMinuteTexts[aMinutes], 0, 0, -1, 0, nullptr);
		SetFontLayerColor(aFont, 1, Color::White);
		SetFontLayerColor(aFont, 2, Color(0, 0, 0, 0));
		delete mUnknown38F4;
		mUnknown38F4 = new TextImage(aFont, aMinuteTexts[aMinutes], 0, 0, -1, 0, nullptr);
		SetFontLayerColor(aFont, 0, Color::White);
		SetFontLayerColor(aFont, 1, Color::White);
		SetFontLayerColor(aFont, 2, Color::White);
		mUnknown3900 = 5.0f;
		mUnknown38F8 = 0.5f;
		mUnknown38FC = 1.0f;
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
	mUnknown40 = 1.0;
	mUnknown48 = 1.0;
	if (!mUnknown51)
		Restart();
}

void LevelValue::RestartAtLevel()
{
	if (mFirstLevel == gBoard->mUnknown158)
		Restart();
}

int LevelValue::GetCount()
{
	int aLevel = gBoard->mUnknown158;
	int aValue;
	if (aLevel < mFirstLevel || mFirstLevel < 0) {
		aValue = -1;
	} else {
		aValue = mBase;
		if (mFirstChangeLevel <= aLevel)
			aValue = aValue + ((aLevel - mFirstChangeLevel) + 1) / mLevelsBetweenChange * mChange;
		if (0 < mTurnsBetweenChange)
			aValue = aValue + (int)((unsigned int)gApp->mUnknownC74[0x21] / (unsigned int)mTurnsBetweenChange) * mChange;
		if (-1 < mMin && aValue < mMin)
			aValue = mMin;
		if (-1 < mMax && mMax < aValue)
			aValue = mMax;
	}
	return aValue;
}

int LevelValue::GetValue()
{
	int aLevel = gBoard->mUnknown158;
	int aValue;
	if (aLevel < mFirstLevel || mFirstLevel < 0) {
		aValue = -1;
	} else {
		aValue = mValueBase;
		if (mFirstChangeLevel <= aLevel)
			aValue = aValue + ((aLevel - mFirstChangeLevel) + 1) / mLevelsBetweenChange * mValueChange;
		if (0 < mTurnsBetweenChange)
			aValue = aValue + (int)((unsigned int)gApp->mUnknownC74[0x21] / (unsigned int)mTurnsBetweenChange) * mValueChange;
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
	if (!mUnknown37F8)
		mApp->PlaySoundEx(theSoundId, GetPan(Ftol(thePiece->mX)), thePitch, theVolume, theTrack, true);
}

void Board::SaveGame(Buffer* theBuffer, bool theReplays)
{
	// (the original logs "SaveGame starts" to the debugger)
	theBuffer->WriteLong(0x3e);
	theBuffer->WriteLong(mGameMode);
	theBuffer->WriteLong(mModeFlags);
	theBuffer->WriteByte((uchar)mUnknown154);
	theBuffer->WriteLong(mUnknown158);
	theBuffer->WriteLong(mUnknown15C);
	theBuffer->WriteLong(mUnknown160);
	theBuffer->WriteBoolean(mUnknown164);
	theBuffer->WriteLong(mUnknown168);
	theBuffer->WriteLong(mUnknown1E8);
	theBuffer->WriteLong(mUnknown1EC);
	theBuffer->WriteShort(0x54);
	for (int i = 0; i < 0x54; i++)
		theBuffer->WriteLong(mApp->mUnknownC74[i]);
	for (int i = 0; i < 0x54; i++)
		theBuffer->WriteLong(mUnknown2234[i]);
	theBuffer->WriteLong(mUnknown208);
	theBuffer->WriteLong(mUnknown200);
	theBuffer->WriteLong(mUnknown204);
	theBuffer->WriteBoolean(mUnknown214);
	theBuffer->WriteShort((short)mUnknown218);
	theBuffer->WriteLong(mUnknown224);
	// 0x6af04f: the level's record
	theBuffer->WriteLong(mUnknown228[0]);
	theBuffer->WriteLong(mUnknown228[1]);
	theBuffer->WriteString(StrFormat(GTC(0x7dc1e0), (double)mUnknown230));
	theBuffer->WriteLong(mUnknown234);
	theBuffer->WriteLong(mUnknown238);
	theBuffer->WriteLong(mUnknown23C);
	theBuffer->WriteLong(mUnknown244);
	theBuffer->WriteLong(mUnknown240);
	theBuffer->WriteLong(mUpdateCnt);
	theBuffer->WriteLong(mUnknown1FB8);
	theBuffer->WriteLong(mUnknown1FC4);
	theBuffer->WriteLong(Ftol((double)mUnknown1454 * 10000.0));
	theBuffer->WriteLong(Ftol((double)mUnknown1458 * 10000.0));
	theBuffer->WriteLong(mUnknown1464);
	theBuffer->WriteLong(mUnknown1468);
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
			theBuffer->WriteBoolean(mCells[mCols * aRow + aCol].mUnknown0);
		}
	}
	theBuffer->WriteString(RandStateBytes(gUnknown85B970));
	theBuffer->WriteString(LevelValueBytes(mLockGem));
	theBuffer->WriteString(LevelValueBytes(mSkulls));
	theBuffer->WriteString(LevelValueBytes(mCounter));
	theBuffer->WriteString(LevelValueBytes(mBadGemsOnBoardMax));
	theBuffer->WriteString(LevelValueBytes(mUnknown1E00));
	theBuffer->WriteString(LevelValueBytes(mLevelUpPoints));
	theBuffer->WriteString(LevelValueBytes(mCoalGem));  // (mDevil is not saved)
	theBuffer->WriteShort((short)mUnknown21D0);
	theBuffer->WriteLong(mUnknown21F8);
	theBuffer->WriteBoolean(mUnknown1F6C);
	theBuffer->WriteShort((short)mUnknown1F70);
	theBuffer->WriteLong(gNextPieceId);
	theBuffer->WriteBoolean(mUnknown21DA);
	theBuffer->WriteBoolean(mUnknown2041);
	theBuffer->WriteLong(mUnknown14F4);
	theBuffer->WriteLong(mUnknown14FC);
	theBuffer->WriteLong(mUnknown21C);
	theBuffer->WriteLong(mUnknown21DC);
	theBuffer->WriteLong(mUnknown21E0);
	int aCount = (int)mBonusColors.size();
	theBuffer->WriteShort((short)aCount);
	for (int i = 0; i < aCount; i++)
		theBuffer->WriteShort((short)mBonusColors[i]);
	theBuffer->WriteLong(mUnknown2658);
	theBuffer->WriteShort((short)mUnknown2660);
	theBuffer->WriteLong(mUnknown2644);
	theBuffer->WriteLong(mUnknown2640);
	theBuffer->WriteByte((uchar)mUnknown265C);
	theBuffer->WriteLong(Ftol((double)mUnknown220 * 10000.0));
	theBuffer->WriteLong(mUnknown3458);
	theBuffer->WriteLong(mUnknown818);
	if (!theReplays) {
		theBuffer->WriteBoolean(false);
	} else {
		theBuffer->WriteBoolean(true);
		for (WinTwistApp::UnknownFA0& aSlot : mApp->mUnknownFA0) {  // 0x6a3192
			theBuffer->WriteShort((short)aSlot.mUnknown20);
			theBuffer->WriteShort((short)aSlot.mUnknown24);
			theBuffer->WriteLong(aSlot.mUnknown28);
			theBuffer->WriteBuffer(aSlot.mBuffer.mData);
		}
	}
	if (mChallenge == nullptr) {
		theBuffer->WriteBoolean(false);
	} else {
		// The challenge: its category, its number, how far its pattern is; then the stat it counts and its best
		theBuffer->WriteBoolean(true);
		theBuffer->WriteShort((short)mChallenge->mCategory);
		theBuffer->WriteShort((short)mChallenge->mNumber);
		theBuffer->WriteShort((short)mChallenge->mProgress);
		theBuffer->WriteLong(mUnknown2638);
		theBuffer->WriteLong(mUnknown2634);
	}
	theBuffer->WriteLong(mUnknown1F64);
	theBuffer->WriteLong(mUnknown1F68);
	theBuffer->WriteLong(mUnknown3908);
	theBuffer->WriteLong(mUnknown390C);
	theBuffer->WriteLong(mUnknown3910);
	theBuffer->WriteLong(mUnknown1F0);
	theBuffer->WriteString(FourBytes(&mUnknown3918));
	theBuffer->WriteString(FourBytes(&mUnknown3914));
	theBuffer->WriteBytes((const uchar*)&mUnknown3928, 4);
	theBuffer->WriteBytes((const uchar*)&mUnknown392C, 4);
	theBuffer->WriteLong(mUnknown1FC);
	// (and "SaveGame ENDS")
}

void Board::StartReplay(Buffer& theBuffer, int theRow, int theCol, bool thePlayback)
{
	if (mUnknown1570 != 0)
		Unpause(true);
	mGoodTube->ResetBonus();
	mFXManager2->Clear();
	mFXManager->Clear();
	// (with Direct3D a menu's replay board also takes the slide's place)
	if (!mUnknown37C4 || gD3DInterface != nullptr)
		mBoardX = GetSlideX();
	LoadGame(theBuffer);
	if (mUnknown37C4) {
		LoadConfig();
		ReadConfig(false);
		UpdateProgress(true);
	}
	mUnknown21A2 = false;
	MoveCursor(theRow, theCol, false, nullptr);
	mUnknown1430 = GetPieceAt(theRow, theCol);
	mUnknown20D0 = true;
	mGoodTube->mUnknown3B0 = 0;
	mGoodTube->mUnknown3F8 = 0.0f;
	if (mGameMode == MODE_BLITZ) {
		mGoodTube->SetUnknown15C(1);
		mGoodTube->SetUnknown4BC(30000);  // 0x6a697d
		mGoodTube->SetTime(mUnknown3458, true);
		mGoodTube->StartUnknown4AC(true, false);
		SetBlitzColor();
		UpdateTubeProgress(true);
		mGoodTube->SetUnknown4A8(mApp->mPlayPressed);
	}
	if (!thePlayback) {
		mUnknown160 = 0;
		mUnknown3444 = 0;
		StartTwist(false, -1, -1);
	} else {
		// The recorded frames of the move run backwards first (UpdateGame's state 1), to the rewind's sound
		if (mRewindSound == nullptr)
			mRewindSound = mApp->mSoundManager->GetSoundInstance(SOUND_REWIND);
		if (mRewindSound != nullptr)
			mRewindSound->Play(true, false);
		mUnknown3448 = mUnknown3444;
		mUnknown160 = 1;
	}
	if (!mApp->Is3DAccelerated())
		mUnknown3880.SetConstant(0.0);
	// The player cannot click the board until the replay's move is over
	mWidgetFlagsMod.mRemoveFlags |= WIDGETFLAGS_ALLOW_MOUSE;
	mUnknown2629 = true;
}

void Board::ShowRankBonus(int theBonus)
{
	if (mUnknown26A0 != nullptr) {
		mApp->mWidgetManager->RemoveWidget(mUnknown26A0);
		delete mUnknown26A0;
	}
	AddStat(0x38, theBonus);
	// The ranks the points already reach (the rank goes up only here and in the bar)
	Profile* aProfile = mApp->mProfile;
	while (aProfile->mRank < (int)mApp->mRanks->mRanks.size() - 1 &&
		(unsigned int)mApp->mRanks->GetPoints(aProfile->mRank + 1) <= (unsigned int)aProfile->mUnknown160)
		aProfile->mRank++;
	if ((unsigned int)aProfile->mUnknown160 < (unsigned int)mApp->mRanks->GetPoints(aProfile->mRank) ||
		(unsigned int)mApp->mRanks->GetPoints(aProfile->mRank + 1) <= (unsigned int)aProfile->mUnknown160)
		aProfile->mUnknown160 = mApp->mRanks->GetPoints(aProfile->mRank);
	int aX = Ftol((double)mApp->mWidth * 0.5);
	if (gApp->mLastGameMode == 1)
		aX += Ftol((double)gApp->mScale * -90.0);
	float aYFrac = mGameMode == 3 ? 0.75f : 0.5f;
	int aY = Ftol((double)mApp->mHeight * (double)aYFrac);
	mUnknown26A0 = new RankBonusWidget(aProfile->mUnknown160, theBonus, aX, aY);
	mApp->mWidgetManager->AddWidget(mUnknown26A0);
}
