// Sexy::Board (0x6a3d7b-0x6d6899): being ported, see Board.h.
#include "Board.h"

#include "Buffer.h"
#include "CustomBassMusicInterface.h"
#include "EffectsManager.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Hyperspace.h"
#include "MemoryImage.h"
#include "Piece.h"
#include "Profile.h"
#include "PropertiesParser.h"
#include "Res.h"
#include "TwistUtil.h"
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
// port: the constructor's copy comes with it; until then the table is set here
Color Sexy::gGemColors[7] = {Color(255, 32, 32), Color(255, 255, 255), Color(0, 255, 0), Color(255, 255, 0),
	Color(255, 0, 255), Color(255, 128, 0), Color(0, 128, 255)};

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
	SRand((unsigned int)time(nullptr));
	gUnknown85AEF0 = 0;
	mUnknown3924 = mApp->mUnknownF85 ? 0.0f : 1.0f;
	mUnknown1F88 = Rect(0, 0, 0, 0);
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
	mUnknown21E0 = kRolls[Rand() % 25];
	if (gApp->mProfile->mGamesPlayed == 0)
		mUnknown21E0 = 3;
	mZOrder = -5;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mShineImage = IMAGE_GEM_LIGHTING;
	mGridBarTop = IMAGE_INGAMEUI_GRIDBAR_TOP;
	mGridBarBot = IMAGE_INGAMEUI_GRIDBAR_BOT;
	// port: the 0x10-byte object (0x74c8c8) comes with its first use
	NotPorted("Board's +0x211c object (0x74c8c8)");
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
		// port: 0x70c8b6(Color(0, 255, 0), 1), the tube's fill colour, comes with the tube's drawing
		mGoodTube->SetUnknown15C(1);
	}

	// port: the first manager's Direct3D height image (0x6f5ea6, +0xb6 and +0xa1) is left out
	mFXManager = new EffectsManager();
	mFXManager2 = new EffectsManager();
	mFXManager3 = new EffectsManager();
	// port: the points manager (0x7329b7) comes with its first use
	NotPorted("Board's PointsManager (0x7329b7)");
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
	// 0x6a3e81: the tube shows the level (0x703431(+0x21d0)) once it is ported
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
	if (!mUnknown149C.empty()) {
		// port: these objects come with the matching code (their destructor is 0x71eb9e)
		NotPorted("Board::ClearBoard's +0x149c objects (0x71eb9e)");
	}
	mUnknown149C.clear();  // 0x6af6b6
	for (size_t i = 0; i < mUnknown14AC.size(); i++)
		std::free(mUnknown14AC[i]);
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
	mUnknown135C = new int[mCols * mRows];
	std::memset(mUnknown135C, 0, mCols * mRows * sizeof(int));
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
	// port: unless the dialog at +0x2584 is up with +0x214 at 14 (it comes with its dialog)
	if (mUnknown2584 == nullptr) {
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
		std::memset(mUnknown228, 0, sizeof(mUnknown228));
		mUnknown1EC = 0;
		mUnknown1E8 = 0;
		mUnknown21F8 = 100;  // 0x6a3ff4
		// 0x6a3e81: the tube's level (0x703431(+0x21d0)) once it is ported
		if (mGoodTube != nullptr && mUnknown2041)
			NotPorted("GoodTube 0x703431 (the level shown)");
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
		NotPorted("Board 0x6bbbc2 (after the progress)");
}

bool Board::CheckLevelDone()
{
	if (mUnknown37C4 || mGameMode == 2 || mGameMode == 3)
		return false;
	if (!mUnknown21A3) {
		if (mUnknown200 < mUnknown204)
			return false;
		if (mUnknown2624 != nullptr) {
			// port: sets the widget's +0x160 to 3 and +0x14c off, and +0x2628, once it is known
			NotPorted("Board::CheckLevelDone's +0x2624 widget");
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
	int aRoll = Rand();
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
				aColor = Rand() % 7;
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
	delete[] mUnknown1370;
	delete[] mUnknown135C;
	delete[] mCells;
	delete[] mGrid;
	if (gBoard == this)
		gBoard = nullptr;
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
		SetLevelNum(mUnknown168, true);
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
			// port: the tube's Update (slot 22, 0x702736) once it is ported; only a loaded game gets here
			NotPorted("Board::StartGame: GoodTube::Update");
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
		} else if (mGameMode == MODE_ZEN && 13 < mUnknown158 && !gApp->mRegistered) {
			mUnknown200 = 0;
			UpdateTubeProgress(true);
		}
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
		// port: its z order is the VizManager's (app +0xc6c) + 1 once the VizManager is ported
		mApp->mWidgetManager->AddWidget(mHyperspace);
		mHyperspace->mTransition = 8;
		mHyperspace->mState = 1;
		mUnknown21A2 = false;
		// port: the tube's effects manager (+0x28c) is cleared, once it is ported
	} else {
		// port: the VizManager (app +0xc6c) is hidden here (slot 31 with false) once it is ported
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
		// port: the hint request 0x6b98f8(0xe, 0) and the tutorial's dialog 0x6bbc58 ("basics") come with the
		// tutorial; neither draws from Rand
		NotPorted("Board::StartLevel: the basics tutorial");
		mUnknown37B0 = false;
		mUnknown37B1 = false;
		if (mApp->mUnknownF85) {
			// port: 0x7540ad, the tutorial's board, which draws from Rand
			NotPorted("Board::StartLevel: the tutorial's board");
		}
	} else if (mGameMode != MODE_ZEN) {
		if (mGameMode == MODE_PUZZLE) {
			mUnknown2042 = false;
			mUnknown2054 = true;
			// port: 0x6c3515(mUnknown2034, true), the puzzle's board, which draws from Rand
			NotPorted("Board::StartLevel: the puzzle's board");
		} else {
			mUnknown2041 = true;
			mUnknown2042 = true;
		}
	}
	// port: SaveGame (0x6b0d91) into mSaveBuffer and the tube's layout (0x6b3da8) come next; neither draws from Rand
	NotPorted("Board::StartLevel: SaveGame and the tube's layout");
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

bool Board::LoadGame(Buffer& theBuffer)
{
	int aVersion = 0;
	if (!ReadSaveHeader(theBuffer, &aVersion, &mGameMode, &mModeFlags, &mUnknown154, &mUnknown158))
		return false;
	// port: the rest of 0x6c8e99 (the board, its pieces and the level values) comes with the saving of a game
	NotPorted("Board::LoadGame");
	return false;
}

void Board::SetLevelNum(int theLevel, bool theUnknown14F8)
{
	mUnknown168 = theLevel;
	if (theUnknown14F8)
		mUnknown14F8 = theLevel;
	SetStat(0x49, theLevel);
	UpdateProgress(theUnknown14F8);
}

void Board::UpdateChallengeTube(bool theAtOnce)
{
	if (mUnknown2630 == 0)
		return;
	// port: the challenge at +0x2630 (its name, flags at +0x80 and goals at +0x84 and +0x88) comes with the challenges
	NotPorted("Board::UpdateChallengeTube");
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
			aValue = aValue + (int)(gApp->mUnknownCF8 / (unsigned int)mTurnsBetweenChange) * mChange;
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
			aValue = aValue + (int)(gApp->mUnknownCF8 / (unsigned int)mTurnsBetweenChange) * mValueChange;
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

void Board::PieceArrived(Piece* thePiece)
{
	(void)thePiece;
	NotPorted("Board::PieceArrived (0x6c0ccf)");
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
