// The rules of the Board (RocketManiaGame 0x422120-0x428100): how tiles are made and fall, how a connection from the left
// edge to a rocket is found and burnt, the timers of the three modes and the board's Update (0x426f40). The functions
// keep the original's order and shape; the field offsets are in Board.h. The talking head, the dialogs and the particle effects
// are in their own files (TalkingHead.cpp, Dialogs.cpp, Effects.cpp).

#include "Board.h"
#include "RocketApp.h"
#include "TalkingHead.h"
#include "Compat2004.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "MTRand.h"
#include "WidgetManager.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

const unsigned int Sexy::gPipeMask[20] = {
	0x5, 0xa, 0x5, 0xa,
	0x3, 0x6, 0xc, 0x9,
	0xe, 0xd, 0xb, 0x7,
	0xf, 0xf, 0xf, 0xf,
	0x1, 0x2, 0x4, 0x8
};

const PipeStep Sexy::gPipeSteps[4] = {
	{-1, 0, 1, 4},
	{1, 0, 4, 1},
	{0, -1, 8, 2},
	{0, 1, 2, 8}
};

// port: the original's float to int conversion (__ftol): truncation
static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x416ef0: rounds half away from zero (on top of the truncation)
static int RoundNearest(double theValue)
{
	int aInt = (int) theValue;
	if (theValue <= 0.0)
	{
		if (theValue - (double) aInt < -0.5)
			return aInt - 1;
	}
	else if (0.5 < theValue - (double) aInt)
		return aInt + 1;
	return aInt;
}

// ---------------------------------------------------------------------------------------------------------
// Piece, generator, effects

// 0x41ad90
Piece::Piece(int theBorn) :
	mId(0), mReferencesResolved(false), mX(0), mY(0),
	mSpeedX(((double) GlobalRand() * 4.656612875245797e-10) * 10.0 - 5.0),   // the global generator, 0x45b9ae
	mSpeedY(0), mMoving(false), mSlideProgress(0), mSlideStep(0),
	mSlideStartX(0), mSlideStartY(0), mSlideEndX(0), mSlideEndY(0), mRow(0), mColumn(0), mFromRow(0), mFromColumn(0),
	mUsed(false), mBlocked(false), mType(7), mKind(-1), mRotation(-1), mBorn(theBorn),
	mPath(0), mPathFrom(-1), mFrame(0), mTilt(0), mTurnedAt(theBorn),
	mSpecial(NULL), mHost(NULL), mInMoverList(false), mAlwaysZero(0), mFlash(NULL),
	mIndex(0), mIndex2(0), mHitsLeft(3), mTrailLife(0), mMovedFromRow(0), mMovedFromColumn(0), mLoadedSpecial(0), mLoadedHost(0)
{
}

// 0x42a510
PieceGenerator::PieceGenerator(RocketApp* theApp, Board* theBoard, MTRand* theRand) :
	mApp(theApp), mBoard(theBoard), mRand(theRand)
{
	ResetCounts();
	mCoinCount = 0;
	mLaunchBonus = 0;
	for (int i = 0; i < 5; i++)
		mThreshold[i] = 0;
	mTableIndex = -1;
	for (int i = 0; i < 9; i++)
		mMeter[i] = 0;
}

// 0x42aa00, 0x42aa20
void PieceGenerator::ResetCounts()
{
	for (int i = 0; i < 10; i++)
		mLaunched[i] = 0;
	for (int i = 0; i < 4; i++)
		mCoins[i] = 0;
	mTotal = 0;
}

// the level limits of 0x4e6800 (three per difficulty) and the lower and upper rows of 0x4e6828 (a block of six rows
// of five per difficulty: the rows 0 to 2 are the lower values for the three modes, 3 to 5 the upper)
static const int kLevelLimit[9] = { 40, 40, 40, 30, 30, 30, 20, 20, 20 };
static const int kThresholds[3][6][5] = {
	{ {160, 120, 40, 20, 0}, {160, 120, 40, 20, 0}, {160, 120, 40, 20, 0},
	  {130, 150, 20, 10, 4}, {130, 150, 20, 10, 4}, {174, 130, 20, 8, 8} },
	{ {160, 120, 40, 20, 0}, {160, 120, 40, 20, 0}, {160, 120, 40, 20, 0},
	  {130, 150, 20, 10, 8}, {130, 150, 20, 10, 8}, {174, 130, 20, 8, 8} },
	{ {160, 120, 40, 20, 0}, {160, 120, 40, 20, 0}, {160, 120, 40, 20, 0},
	  {130, 150, 20, 10, 8}, {130, 150, 20, 10, 8}, {174, 130, 20, 8, 8} }
};

// 0x42a460: the cumulative weights by which a kind of tile 0 to 4 is drawn, between the lower row at level 0 and the
// upper row at the level limit
void PieceGenerator::BuildThresholds(int theDifficultyIndex, int theLevel, int theMode)
{
	double aFraction;
	int aLimit = kLevelLimit[theMode + theDifficultyIndex * 3];
	if (theLevel < aLimit)
		aFraction = (double) theLevel / (double) aLimit;
	else
		aFraction = 1.0;

	for (int i = 0; i < 5; i++)
	{
		int aLow = kThresholds[theDifficultyIndex][theMode][i];
		int aHigh = kThresholds[theDifficultyIndex][theMode + 3][i];
		int aStep = RoundNearest((double) (aHigh - aLow) * aFraction);
		mThreshold[i] = aStep + aLow;
		if (i > 0)
			mThreshold[i] = mThreshold[i - 1] + aStep + aLow;
	}
	mTableIndex = theMode;
}

// 0x42a590
int PieceGenerator::DrawKind()
{
	unsigned int aValue = (unsigned int) mRand->Next();
	int aKind = 0;
	do
	{
		if ((int) (aValue % (unsigned int) mThreshold[4]) < mThreshold[aKind])
			return aKind;
		aKind++;
	}
	while (aKind < 5);
	return 0;
}

// 0x42a570: the chance of a special piece is one in this many tiles (0 at level 0: never)
int PieceGenerator::SpecialInterval()
{
	int aLevel = mBoard->mLevel;
	if (aLevel == 0)
		return 0;
	if (aLevel > 0x14)
		return 0x1e;
	return 0x38 - aLevel;
}

// 0x42a5c0
unsigned char PieceGenerator::SpecialWeight(int theKind)
{
	if (theKind == 2)
		return 0xf;
	if (theKind == 0)
		return 0xf;
	return (unsigned char) ((((unsigned int) (theKind != 0xc)) - 1U) & 10);
}

// 0x42a600: which special piece: 2, 0 or 0xc by their weights (the second and third need a higher level), -1 none
int PieceGenerator::DrawSpecial()
{
	int aLevel = mBoard->mLevel;
	int aWeight2 = (aLevel < 3) ? 0 : SpecialWeight(2);
	int aWeight0 = (aLevel < 5) ? 0 : SpecialWeight(0);
	int aWeight12 = (aLevel < 7 || mApp->mMode != 1) ? 0 : SpecialWeight(0xc);

	unsigned int aTotal = aWeight12 + aWeight0 + aWeight2;
	if (aTotal == 0)
		return -1;
	unsigned int aValue = (unsigned int) mRand->Next() % aTotal;
	if ((int) aValue < aWeight2)
		return 2;
	return ((int) aValue < aWeight0 + aWeight2) ? 0 : 0xc;
}

// 0x42a6b0: which kind of bomb (the kind of its pipe)
int PieceGenerator::DrawBombKind(int theLevel)
{
	if (theLevel < 3)
		return 0;
	if (theLevel < 10)
		return (int) (mRand->Next() & 1);
	if (theLevel < 0xf)
		return (int) (mRand->Next() % 3);
	if (theLevel < 0x14)
		return (int) (mRand->Next() & 3);
	return (int) (mRand->Next() % 5);
}

// 0x42a720
bool PieceGenerator::SpecialNow()
{
	unsigned int anInterval = SpecialInterval();
	if (anInterval == 0)
		return false;
	return (unsigned int) mRand->Next() % anInterval == 0;
}

// 0x42a750
int PieceGenerator::RocketsForLevel(int theLevel)
{
	int aCount = theLevel + 9;
	if (aCount > 0x31)
		aCount = 0x31;
	return aCount;
}

// 0x42a770
void PieceGenerator::ResetForLevel()
{
	ResetCounts();
	Board* aBoard = mBoard;
	int aLevel = aBoard->mLevel;
	switch (mApp->mDifficulty)
	{
	case 1: aBoard->mNightLength = 130.0; aBoard->mMatchesLeft = 10; break;
	case 2: aBoard->mNightLength = 100.0; aBoard->mMatchesLeft = 8; break;
	case 3: aBoard->mNightLength = 80.0; aBoard->mMatchesLeft = 6; break;
	}
	if (mApp->mMode != 0 && mApp->mMode != 1)
	{
		aBoard->mLeftCount = aBoard->mRows;
		aBoard->mRightCount = aBoard->mRows;
		aBoard->mRightEdgeLoaded = false;
		aBoard->mLaunchesNeeded = 1;
		aBoard->mLaunchesNeededCopy = aBoard->mLaunchesNeeded;
		return;
	}
	aBoard->mLaunchesNeeded = RocketsForLevel(aLevel + 1);
	aBoard->mLaunchesNeededCopy = aBoard->mLaunchesNeeded;
	aBoard->mLeftCount = aBoard->mRows;
	aBoard->mRightCount = aBoard->mRows;
	aBoard->mRightEdgeLoaded = false;   // (the original stores DL, which 0x42a77f has set to 0)
}

// 0x42a890: the updates a tile takes to fall in arcade mode, at a level (shorter as the level grows)
int PieceGenerator::FallDelay(int theLevel)
{
	static const int aLow[3] = { 0x50, 0x3c, 0x28 };
	static const int aHigh[3] = { 0xa0, 0x78, 0x50 };
	int aDifficulty = mApp->mDifficulty - 1;
	double aLevel = (double) theLevel;
	if (aLevel >= 10.0)
	{
		double aValue = (double) aLow[aDifficulty] - (aLevel - 10.0) * 5.0;
		if (aValue < 20.0)
			aValue = 20.0;
		return Ftol(aValue);
	}
	return Ftol((double) aHigh[aDifficulty] - (double) (aHigh[aDifficulty] - aLow[aDifficulty]) * aLevel * 0.1);
}

// 0x42a940
int PieceGenerator::Rows()
{
	int aDifficulty = mApp->mDifficulty;
	if (aDifficulty == 1)
		return 3;
	if (aDifficulty == 2)
		return 4;
	return (aDifficulty == 3) * 4 + 1;
}

// 0x42a980
int PieceGenerator::Rows20()
{
	return 0x14 / Rows();
}

// 0x42a990
void PieceGenerator::CountSpecial(Piece* thePiece)
{
	if (thePiece->mKind == 6)
	{
		mCoins[thePiece->mIndex]++;
		mCoinCount++;
	}
}

// 0x42a9c0
void PieceGenerator::CountPiece(Piece* thePiece)
{
	if (thePiece != NULL)
	{
		if (thePiece->mType == 5)
		{
			mLaunched[thePiece->mIndex]++;
			return;
		}
		if (thePiece->mType == 6 || (thePiece = thePiece->mSpecial) != NULL)
			CountSpecial(thePiece);
	}
}

// 0x42aa40: the launches of a round are over: the bonus for chained launches
void PieceGenerator::ShowBonus()
{
	int aTotal = 0;
	for (int i = 0; i < 10; i++)
		aTotal = aTotal + mLaunched[i];
	mTotal = mTotal + aTotal;
	if (mBoard->mLevelMostRockets < aTotal)   // board+0xc8: the largest launch
		mBoard->mLevelMostRockets = aTotal;

	Board* aBoard = mBoard;
	// the texts' addresses in the original (GameText.h), by the number of rockets launched together
	static const unsigned int sMessages[10] = {
		0, 0, 0, 0x004e6a70, 0x004e6a5c, 0x004e6a34, 0x004e6a14, 0x004e69f4, 0x004e69d4, 0x004e69b4
	};
	static const int sGreen[10] = { 0, 0, 0, 0, 0, 0xff, 0xc0, 0x80, 0x40, 0 };
	switch (aTotal)
	{
	case 3:
	case 4:
		aBoard->AddText(GT(sMessages[aTotal]), Board::CellX(3), 0x1e);
		break;
	case 5: case 6: case 7: case 8: case 9:
		{
			// the title text is the string at 0x4e6a54; it is drawn on the "multi" picture in a colour
			Points* aPoints = aBoard->AddText("", Board::CellX(3), 0x1e);
			aPoints->SetBack(mApp->mImages[IMAGE_MULTI], 0xff, sGreen[aTotal], 0xff);
			aBoard->AddText(GT(sMessages[aTotal]), 0x181 + (aTotal == 5 ? 0 : 1), 0x23);
			static const int sBonusPoints[5] = { 1000, 2500, 10000, 25000, 100000 };
			aBoard->AddScore(sBonusPoints[aTotal - 5]);
		}
		break;
	default:
		break;
	}

	if (mApp->mMode == 2)
	{
		if (2 < aTotal)
			mLaunchBonus = aTotal - 2;
	}
	else
	{
		if (aTotal < 2)
			mLaunchBonus = mLaunchBonus + mTotal - 1;
		else
			mLaunchBonus = mLaunchBonus + (mTotal - 1) * aTotal;
		int aLevel = aBoard->mLevel < 0x32 ? aBoard->mLevel : 0x32;
		mLaunchBonus = Ftol(((double) aLevel * 0.02 + 1.0) * (double) mLaunchBonus);
		if (mApp->mMode == 0)
			mLaunchBonus = Ftol((double) mLaunchBonus * 1.5);
	}
	mTotal = 0;
	ResetCounts();
}

// 0x42ad50
void PieceGenerator::Seed(unsigned int* theKind, int* theB, Board* theBoard, int theColumn)
{
	*theKind = Fuse(theColumn);
	*theB = 0;
}

// 0x42ad70
unsigned int PieceGenerator::Fuse(int theRow)
{
	if (mTableIndex == 2)
		return (unsigned int) (mRand->Next() & 3);
	return (unsigned int) mMeter[theRow];
}

// 0x42ada0
int PieceGenerator::FullestMeter()
{
	int aValue = 9;
	while (true)
	{
		int aCount = 0;
		for (int i = 0; i < 9; i++)
		{
			if (mMeter[i] == aValue)
				aCount++;
		}
		if (2 < aCount)
			break;
		aValue--;
		if (aValue < 0)
			return 9;
	}
	return aValue;
}

// 0x42add0
void PieceGenerator::RaiseMeter(int theRow)
{
	if (mMeter[theRow] < 9)
		mMeter[theRow]++;
}

// 0x42adf0
bool PieceGenerator::AllMetersFull()
{
	for (int i = 0; i < 9; i++)
	{
		if (mMeter[i] < 9)
			return false;
	}
	return true;
}

// 0x42ae10: the launchers that coins upgrade, five coins each, in the order of the table at 0x4e6990 and only those whose
// meter is not full and not above the fullest meter's level (FullestMeter). Mode 0 counts them; mode 1 gives the launcher of
// the theIndex-th.
static const int sUpgradeOrder[9] = {4, 5, 3, 6, 2, 7, 1, 8, 0};

int PieceGenerator::Upgrades(int theCoins, int theMode, int theIndex)
{
	int aCount = 0;
	int aLevel = FullestMeter();
	for (int i = 0; i < 9; i++)
	{
		if (5 <= theCoins)
		{
			int aMeter = mMeter[sUpgradeOrder[i]];
			if ((aMeter < 9) && (aMeter <= aLevel))
			{
				theCoins -= 5;
				if ((theMode == 1) && (theIndex == aCount))
					return sUpgradeOrder[i];
				aCount++;
			}
		}
	}
	if (theMode != 0)
		return 1;
	return aCount;
}

// ---------------------------------------------------------------------------------------------------------
// Points

// 0x429ed0
Points::Points(RocketApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife, int theMode) :
	mApp(theApp), mText(theText), mFont(theFont), mBackImage(NULL), mBackColor(255, 255, 255), mLife(theLife), mPhase(0),
	mStatic(false), mTextWidth(theFont->StringWidth(theText)), mLeft(theX), mTop(theY), mMode(theMode), mBaseline(0)
{
	// the text is only to be looked at: a click goes through it to the board (the original's constructor clears the mouse flag)
	mMouseVisible = false;
	Position();
}

// 0x42a230
void Points::SetBack(Image* theImage, int r, int g, int b)
{
	mBackImage = theImage;
	mBackColor = Color(r, g, b);
	Position();
}

// 0x42a150
void Points::Position()
{
	int aX = mLeft;
	if (mMode == 0)
		aX = mLeft - mTextWidth / 2;
	else if (mMode == 1)
		aX = mLeft - mTextWidth;
	if (mBackImage == NULL)
	{
		Resize(aX, mTop - mFont->GetAscent(), mTextWidth, mFont->GetHeight());
		mBaseline = mFont->GetAscent();
		return;
	}
	Resize(aX, mTop - mBackImage->GetHeight() / 2, mTextWidth, mBackImage->GetHeight());
	mBaseline = mFont->GetAscent() / 4 + mBackImage->GetHeight() / 2;
}

// 0x42a2d0: the text with a black outline, in a colour that runs through the hues, fading out over its last 18 updates
void Points::Draw(Graphics* g)
{
	double aFade = (double) mLife * 0.05555555555555555;
	if (aFade > 1.0)
		aFade = 1.0;
	if (mBackImage != NULL)
	{
		g->SetColorizeImages(true);
		g->SetColor(mBackColor);
		g->DrawImage(mBackImage, mWidth / 2 - mBackImage->GetWidth() / 2, 0);
		g->SetColorizeImages(false);
	}
	unsigned long aRGB = mApp->HSLToRGB(mPhase, 0xff, 0x80);
	Color aColor((int) ((aRGB >> 16) & 0xff), (int) ((aRGB >> 8) & 0xff), (int) (aRGB & 0xff), Ftol(aFade * 255.0));
	g->SetFont(mFont);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mText, -1, mBaseline - 1);
	g->DrawString(mText, 1, mBaseline - 1);
	g->DrawString(mText, -1, mBaseline + 1);
	g->DrawString(mText, 1, mBaseline + 1);
	g->SetColor(aColor);
	g->DrawString(mText, 0, mBaseline);
}

// ---------------------------------------------------------------------------------------------------------
// Board

// 0x41b660
Piece* Board::FindLoadedPiece(int theId)
{
	if (theId != 0)
	{
		for (std::list<Piece*>::iterator anItr = mLoadedPieces.begin(); anItr != mLoadedPieces.end(); ++anItr)
		{
			if ((*anItr)->mId == theId)
				return *anItr;
		}
	}
	return NULL;
}

// 0x422120
void Board::ResetState(bool theContinuing)
{
	mChainBurn = false;
	mMatchesWarning = 0;
	mHintsQuiet = false;
	mTilesDropped = 0;
	mOverflowTimer = 500;
	if (!theContinuing)
		mApp->mHead->GoToRest(true);
	for (int i = 0; i < 14; i++)
	{
		if (mLevel < 1 || i != 7)
			mSpecialCounts[i] = 0;
	}
	mGen->ResetForLevel();
	mNightLeft = mNightLength;
	mNightLeftShown = mNightLength;
	mNightStep = 0.01;
	mLastLanternOut = -1;
	for (int i = 0; i < 10; i++)
		mLanternPuff[i] = 0;
	mLevelFraction = 0;
	// the head says "get ready" at every level but the first one, where (unless the hints are off) it has its own lines
	bool aQuiet = false;
	if (mLevel == 0)
	{
		mLevelFractionShown = 0;
		aQuiet = !mApp->mNoHints;
	}
	if (!aQuiet)
	{
		mApp->mHead->Say(SOUND_VOICE_GET_READY_02);
		mApp->mHead->SetMood(0xe);
	}
	mTilesRemoved = 0;
	mLaunchesDone = 0;
	mLaunchesCounted = 0;
	mEndCountdown = 0xfa;
	mBannerTime = 0x82;
	mGameOverDelay = 0;
	int aTable = 0;
	if (mApp->mMode != 0)
		aTable = (mApp->mMode != 1) + 1;
	mGen->BuildThresholds(mApp->mDifficulty - 1, mLevel, aTable);
	mHintCountdown[0] = Ftol(mNightLength * 50.0);
	mHintCountdown[2] = 1000;
	mHintCountdown[1] = 2000;
	mHintCountdown[3] = 3000;
	mHintCountdown[4] = 4000;
	mHintCountdown[5] = 2000;
	mNightWarned = false;
	FinishLaunchFlames();
	RemoveSetOffSpecial();
	mLevelMostTiles = 0;
	mLevelMostRockets = 0;
	mLevelCoins = 0;
	mLevelGems = 0;
	mLevelBombs = 0;
	mLevelClocks = 0;
	mLevelMatches = 0;
	mAlwaysZero = 0;
	mChainBonusTime = 0;
	mShakeAmount = 0;
	mFrozenTime = 0;
	mShakeTime = 0;
	mClockMade = false;
	mClockDue = false;
	unsigned int aValue = (unsigned int) mRand->Next();
	if ((aValue & 1) == 0)
	{
		int aGlobal = GlobalRand();
		mJustInTime = false;
		mClockNightLeft = ((double) aGlobal * 4.656612875245797e-10 * 16.0 - 8.0) + 25.0;
		return;
	}
	mClockNightLeft = -1.0;
	mJustInTime = false;
}

// 0x422390: a new rocket on the right edge
Piece* Board::AddRight(int theRow)
{
	Piece* aPiece = new Piece(mUpdateCnt);
	aPiece->mColumn = mCols;
	aPiece->mRow = theRow;
	aPiece->mX = (double) CellX(aPiece->mColumn);
	aPiece->mY = (double) CellY(aPiece->mRow);
	aPiece->mKind = 1;
	aPiece->mType = 5;
	unsigned int aKind;
	int aB;
	mGen->Seed(&aKind, &aB, this, aPiece->mRow);   // the row (the fourth argument is the caller's edx, theRow, still)
	aPiece->mIndex = (int) aKind;
	aPiece->mIndex2 = aB;
	mRight[aPiece->mRow] = aPiece;
	return aPiece;
}

// 0x422c50 and 0x423f30 (the same code twice): a new tile above the first row, with a special piece on it sometimes
Piece* Board::NewTop(int theColumn)
{
	Piece* aPiece = new Piece(mUpdateCnt);
	aPiece->mX = (double) CellX(theColumn);
	aPiece->mRow = -1;
	aPiece->mColumn = theColumn;
	aPiece->mY = (double) (CellY(0) - 0x4a);
	aPiece->mKind = mGen->DrawKind();
	aPiece->mRotation = (int) (mRand->Next() & 3);
	aPiece->mType = 1;
	aPiece->mSpecial = MaybeSpecial(aPiece);
	aPiece->mAlwaysZero = 0;
	return aPiece;
}

// 0x422d30
void Board::LevelEffect(int theAction, int theKind, int theX, int theY)
{
	if (theAction == 0)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aColumn = 0; aColumn < mCols; aColumn++)
			{
				Piece* aPiece = mGrid[aRow][aColumn];
				if (aPiece != NULL && aPiece->mType == 1)
					RemovePiece(aPiece, false);
			}
		}
		Effect* anEffect = mEffects->Add(0, true);
		anEffect->mLife = 300;
	}
	else if (theAction == 1)
		BurstSome(theKind, theX, theY);
}

// 0x422df0: the fireworks of the end of a level
void Board::Burst(int theKind, int theX, int theY)
{
	Effect* anEffect = NULL;
	switch (theKind)
	{
	case 0: anEffect = mEffects->Add(0xf, true); anEffect->mLife = 2; break;
	case 1: anEffect = mEffects->Add(7, true); anEffect->mLife = 1; break;
	case 2: anEffect = mEffects->Add(9, true); anEffect->mLife = 1; break;
	case 3: anEffect = mEffects->Add(0x10, true); anEffect->mLife = 0xb4; break;
	case 4: anEffect = mEffects->Add(1, true); anEffect->mLife = 1; break;
	case 5: anEffect = mEffects->Add(10, true); anEffect->mLife = 100; break;
	case 6: anEffect = mEffects->Add(0x11, true); anEffect->mLife = 100; break;
	case 7:
		// four sparklers of the same kind: the first stays where the kind puts it, the others are moved and set going
		// (the original stores these as doubles that Ghidra shows as their halves: mMoveX and mMoveY are -5.0 or 5.0, mY 0.0)
		anEffect = mEffects->Add(0x12, true);
		anEffect->mLayer = true; anEffect->mLife = 0x3c; anEffect->mDelay = theX; anEffect->mSound = theY;
		anEffect = mEffects->Add(0x12, true);
		anEffect->mLife = 0x3c;
		anEffect->mMoveX = -5.0;
		anEffect->mLayer = true; anEffect->mDelay = theX; anEffect->mSound = theY;
		anEffect->mX = (double) (0x17c - GlobalRand() % 0x79);
		anEffect = mEffects->Add(0x12, true);
		anEffect->mMoveX = 5.0; anEffect->mMoveY = 5.0;
		anEffect->mLife = 0x3c;
		anEffect->mY = 0.0;
		anEffect->mLayer = true; anEffect->mDelay = theX; anEffect->mSound = theY;
		anEffect = mEffects->Add(0x12, true);
		anEffect->mLife = 0x3c;
		anEffect->mY = 0.0;
		anEffect->mMoveX = -5.0; anEffect->mMoveY = 5.0;
		anEffect->mX = (double) (0x17c - GlobalRand() % 0x79);
		break;
	case 8:
		anEffect = mEffects->Add(8, true);
		anEffect->mLayer = true; anEffect->mLife = 100; anEffect->mDelay = theX; anEffect->mSound = theY;
		anEffect = mEffects->Add(8, true);
		anEffect->mLife = 100; anEffect->mX = 490.0;
		break;
	}
	if (anEffect != NULL)
	{
		anEffect->mLayer = true;
		anEffect->mDelay = theX;
		anEffect->mSound = theY;
	}
}

// 0x4230b0: kind 9 sets off two of the others at random
void Board::BurstSome(int theKind, int theX, int theY)
{
	if (theKind == 9)
	{
		Burst((int) (mRand->Next() % 9), theX, theY);
		Burst((int) (mRand->Next() % 9), theX, theY);
		return;
	}
	Burst(theKind, theX, theY);
}

// 0x423120: the sound of the multiplier of a chain of tiles and the text "Score x<n>" where the piece was
void Board::ComboSound(double theX, double theY)
{
	int aSound = mComboCount++;
	aSound += 0x29;
	if (0x2c < aSound)
		aSound = 0x2c;
	mApp->PlaySample(aSound);
	if (1 < mComboCount)
	{
		char aBuf[256];
		snprintf(aBuf, sizeof(aBuf), "Score x%d", mComboCount);
		Points* aPoints = AddText(aBuf, Ftol(theX), Ftol(theY));
		aPoints->mStatic = false;
	}
}

// 0x4231a0
int Board::BonusFor(int theIndex)
{
	switch (theIndex)
	{
	case 0: return 500;
	case 1: return 600;
	case 2: return 700;
	case 3: return 800;
	case 4: return 1000;
	default: return 0;
	}
}

// 0x423200
void Board::SmokePuff(Piece* thePiece)
{
	Effect* anEffect = mEffects->Add(5, true);
	anEffect->mDrag = 0.94;
	anEffect->mAccelY = 0;
	anEffect->mDirY = 0;
	anEffect->mDelay = 0;
	anEffect->mLife = 1;
	anEffect->mX = thePiece->mX + 10.0;
	anEffect->mY = thePiece->mY + 10.0;
}

// 0x423270: a special piece is taken off the board. theFlag set: it is destroyed (smoke); clear: it is picked up and
// flies to its place in the bar
void Board::DestroySpecial(Piece* thePiece, bool theFlag)
{
	switch (thePiece->mKind)
	{
	case 0:
		if (mState == 0)
		{
			Effect* anEffect = mEffects->Add(0x13, false);
			anEffect->mLife = 300;
			anEffect->mDelay = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
			thePiece->mFlash = anEffect;
			mApp->PlaySample(0x33);
			if (!theFlag)
			{
				thePiece->mHost = NULL;
				thePiece->mKind = 1;
				thePiece->mBorn = mUpdateCnt;
				return;
			}
		}
		break;
	case 2:
		if (mState != 0)
			break;
		if (!theFlag)
		{
			thePiece->mHost = NULL;
			thePiece->mSlideStartX = Ftol(thePiece->mX);
			thePiece->mSlideStartY = Ftol(thePiece->mY);
			thePiece->mSlideEndX = 0x28;
			thePiece->mSlideEndY = 0x46;
			thePiece->mSlideProgress = 0;
			thePiece->mSlideStep = 0.02;
			thePiece->mMoving = true;
			thePiece->mKind = 3;
			Effect* anEffect = mEffects->Add(4, true);
			anEffect->mLife = 1;
			anEffect->mDelay = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
			if (mState != 3)
			{
				int aSound = mLaunchStep++ + 0x2e;
				if (0x31 < aSound)
					aSound = 0x31;
				mApp->PlaySample(aSound);
			}
			char aBuf[64];
			snprintf(aBuf, sizeof(aBuf), "+%d", BonusFor(thePiece->mIndex));
			Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO14_BOLD], aBuf, Ftol((double) mX + thePiece->mX),
				Ftol((double) mY + thePiece->mY), 100, 0);
			mTexts.push_back(aPoints);
			mWidgetManager->AddWidget(aPoints);
			return;
		}
		SmokePuff(thePiece);
		break;
	case 6:
		if (mState == 0)
		{
			if (!theFlag)
			{
				thePiece->mHost = NULL;
				thePiece->mSlideStartX = Ftol(thePiece->mX);
				thePiece->mSlideStartY = Ftol(thePiece->mY);
				thePiece->mSlideEndX = 0x87;
				thePiece->mSlideEndY = 0x1a4;
				thePiece->mSlideProgress = 0;
				thePiece->mSlideStep = 0.02;
				thePiece->mMoving = true;
				thePiece->mKind = 7;
				Effect* anEffect = mEffects->Add(4, true);
				anEffect->mLife = 2;
				anEffect->mDelay = 0;
				anEffect->mX = thePiece->mX;
				anEffect->mY = thePiece->mY;
				if (mState == 3)
					return;
				ComboSound(thePiece->mX, thePiece->mY);
				return;
			}
			SmokePuff(thePiece);
		}
		break;
	case 10:
		if (mState != 0)
			break;
		if (theFlag)
		{
			SmokePuff(thePiece);
			break;
		}
		if (CountBurning() + mMatchesLeft < 0x14)
		{
			thePiece->mSlideEndY = 0xaa;
			thePiece->mUsed = false;
		}
		else
		{
			int anAdded = AddScore(300);
			char aBuf[64];
			snprintf(aBuf, sizeof(aBuf), "+%d", anAdded);
			Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO14_BOLD], aBuf, Ftol((double) mX + thePiece->mX),
				Ftol((double) mY + thePiece->mY), 100, 0);
			mTexts.push_back(aPoints);
			mWidgetManager->AddWidget(aPoints);
			thePiece->mSlideEndY = 0x46;
			thePiece->mUsed = true;
		}
		thePiece->mSlideEndX = 0x28;
		thePiece->mHost = NULL;
		thePiece->mSlideStartX = Ftol(thePiece->mX);
		thePiece->mSlideStartY = Ftol(thePiece->mY);
		thePiece->mSlideProgress = 0;
		thePiece->mSlideStep = 0.02;
		thePiece->mMoving = true;
		thePiece->mKind = 0xb;
		{
			Effect* anEffect = mEffects->Add(5, true);
			anEffect->mLife = 1;
			anEffect->mDelay = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
		}
		if (mState == 3)
			return;
		{
			int aSound = mLaunchStep + 0x2d;
			if (0x31 < aSound)
				aSound = 0x31;
			mApp->PlaySample(aSound);
		}
		return;
	case 0xc:
		if (mState != 0)
			break;
		if (!theFlag)
		{
			thePiece->mHost = NULL;
			thePiece->mSlideStartX = Ftol(thePiece->mX);
			thePiece->mSlideStartY = Ftol(thePiece->mY);
			thePiece->mSlideEndY = 0x28;
			thePiece->mSlideEndX = CellX(3) - mApp->mImages[IMAGE_CLOCK]->GetWidth() / 2;
			thePiece->mSlideProgress = 0;
			thePiece->mSlideStep = 0.02;
			thePiece->mMoving = true;
			thePiece->mKind = 0xd;
			Effect* anEffect = mEffects->Add(4, true);
			anEffect->mLife = 1;
			anEffect->mDelay = 0;
			anEffect->mX = thePiece->mX;
			anEffect->mY = thePiece->mY;
			if (mState == 3)
				return;
			int aSound = mLaunchStep + 0x2d;
			if (0x31 < aSound)
				aSound = 0x31;
			mApp->PlaySample(aSound);
			return;
		}
		SmokePuff(thePiece);
		break;
	}
	RemovePiece(thePiece, theFlag);
}

// 0x423b00: when a bomb goes off: the current update count plus 2500 to 3500 (a random time)
int Board::RandomTick()
{
	int aRand = GlobalRand();
	int aBase = mUpdateCnt + 3000;
	return Ftol((double) aRand * 4.656612875245797e-10 * 1000.0 - 500.0 + (double) aBase);
}

// 0x423b40: a special piece (type 6) of a kind on a host tile
Piece* Board::MakeSpecial(int theKind, Piece* theHost)
{
	Piece* aPiece = new Piece(mUpdateCnt);
	aPiece->mType = 6;
	aPiece->mKind = theKind;
	switch (theKind)
	{
	case 0:
	case 2:
	case 6:
	case 10:
	case 0xc:
		aPiece->mX = theHost->mX;
		aPiece->mY = theHost->mY;
		aPiece->mRow = theHost->mRow;
		aPiece->mColumn = theHost->mColumn;
		aPiece->mHost = theHost;
		if (theKind == 2)
		{
			aPiece->mIndex = mGen->DrawBombKind(mLevel);
			aPiece->mBorn = RandomTick();
		}
		else if (theKind == 10)
		{
			aPiece->mSlideStartX = Ftol(theHost->mX);
			aPiece->mSlideStartY = 0;
		}
		else if (theKind == 0xc)
			mClockMade = true;
		break;
	case 5:
		aPiece->mX = theHost->mX;
		aPiece->mY = theHost->mY;
		aPiece->mRow = theHost->mRow;
		aPiece->mColumn = theHost->mColumn;
		aPiece->mIndex = theHost->mIndex;
		aPiece->mHost = NULL;
		break;
	case 4:
	case 8:
	case 9:
		aPiece->mHost = NULL;
		break;
	}
	AddMover(aPiece);
	return aPiece;
}

// 0x423ca0
Piece* Board::MaybeSpecial(Piece* theHost)
{
	if (mClockDue && !mClockMade)
	{
		mClockDue = false;
		return MakeSpecial(0xc, theHost);
	}
	if (mGen->SpecialNow())
	{
		int aKind = mGen->DrawSpecial();
		if (aKind != -1)
			return MakeSpecial(aKind, theHost);
	}
	return NULL;
}

// 0x423d10 and 0x423e20 find a tile that has no special piece, starting at a random place, and put one on it
Piece* Board::FindFreeTile()
{
	bool aFound = false;
	unsigned int aColumn = mRand->Next() % (unsigned int) mCols;
	unsigned int aRow = mRand->Next() % (unsigned int) (mRows / 2);
	Piece* aPiece = NULL;
	for (int i = 0; i < mCols; i++)
	{
		if (aFound)
			break;
		for (int j = 0; j < mRows; j++)
		{
			aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType != 4 && aPiece->mSpecial == NULL)
			{
				aFound = true;
				break;
			}
			aRow = (aRow + 1) % mRows;
		}
		aColumn = (aRow + 1) % mCols;
	}
	return aFound ? aPiece : NULL;
}

void Board::AttachBomb(int theIndex)
{
	Piece* aTile = FindFreeTile();
	if (aTile != NULL)
	{
		Piece* aSpecial = MakeSpecial(6, aTile);
		aTile->mSpecial = aSpecial;
		aSpecial->mIndex = theIndex;
		mApp->PlaySample(0x19);
	}
}

void Board::AttachSpecial10()
{
	Piece* aTile = FindFreeTile();
	if (aTile != NULL)
	{
		Piece* aSpecial = MakeSpecial(10, aTile);
		aTile->mSpecial = aSpecial;
		mApp->PlaySample(0x19);
		mMatchAttached = true;
	}
}

// 0x424010: arcade mode: the next tile falls into the first row at the pointer's column
void Board::DropTop()
{
	if (mGrid[0][mDropColumn] == NULL)
	{
		mOverflowTimer = 500;
		Piece* aPiece = mTop[mDropColumn];
		if (aPiece != NULL)
		{
			mTilesDropped = mTilesDropped + 1;
			if (mGen->Rows() * mCols - 1 < mTilesDropped)
				mFallPeriod = mGen->FallDelay(mLevel);
			if (mQuickDrops != 0)
			{
				mFallPeriod = 2;
				mQuickDrops = mQuickDrops - 1;
			}
			AddMover(aPiece);
			aPiece->mType = 0;
			mTop[mDropColumn] = NULL;
			mGrid[0][mDropColumn] = aPiece;
			aPiece->mRow = 0;
			aPiece->mMoving = true;
			aPiece->mColumn = mDropColumn;
			aPiece->mSlideStartX = Ftol(aPiece->mX);
			aPiece->mSlideStartY = Ftol(aPiece->mY + 42.0);
			aPiece->mSlideEndX = CellX(mDropColumn);
			aPiece->mSlideEndY = CellY(0);
			aPiece->mSlideProgress = 0;
			aPiece->mSlideStep = 0.2;
			mTop[mDropColumn] = mSpare;
			mDropColumn = (mDropColumn + 1) % mCols;
			mSpare = NewTop(mDropColumn);
		}
	}
	else
	{
		mOverflowTimer = mOverflowTimer - 1;
		mFallDisplay = mFallPeriod;
	}
}

// 0x424180: classic and strategy modes: the tile of the next column falls
int Board::DropTopTimed()
{
	int aColumn = mTimedDropColumn;
	if (mGrid[0][aColumn] == NULL)
	{
		Piece* aPiece = mTop[aColumn];
		if (aPiece == NULL)
			return aColumn;
		AddMover(aPiece);
		aPiece->mType = 0;
		mTop[mTimedDropColumn] = NULL;
		mGrid[0][mTimedDropColumn] = aPiece;
		aPiece->mRow = 0;
		aPiece->mMoving = true;
		aPiece->mColumn = mTimedDropColumn;
		aPiece->mSlideStartX = Ftol(aPiece->mX);
		aPiece->mSlideStartY = CellY(0) - 0x4a;
		aPiece->mSlideEndX = CellX(mTimedDropColumn);
		aPiece->mSlideEndY = CellY(0);
		aPiece->mSlideProgress = 0;
		aPiece->mSlideStep = 0.08;
		mTop[mTimedDropColumn] = NewTop(mTimedDropColumn);
		aColumn = mTimedDropColumn;
	}
	mTimedDropColumn = (aColumn + 1) % mCols;
	return (aColumn + 1) / mCols;
}

// 0x424370: a floating text in the colours of the board's flashing font, placed relative to the board
Points* Board::AddText(const std::string& theText, int theX, int theY)
{
	Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO14_BOLD], theText, mX + theX, mY + theY, 200, 0);
	aPoints->mStatic = true;
	mTexts.push_back(aPoints);
	mWidgetManager->AddWidget(aPoints);
	return aPoints;
}

// 0x4244f0: "+<score>" where a launch ended; it lasts longer for a higher score
void Board::AddScoreText(int theScore, int theX, int theY)
{
	int aLife = Ftol((double) theScore * 0.6666666666666666);
	if (aLife < 100)
		aLife = 100;
	else if (aLife > 600)
		aLife = 600;
	char aBuf[64];
	snprintf(aBuf, sizeof(aBuf), "+%d", theScore);
	Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO14_BOLD], aBuf, mX + theX, mY + theY, aLife, 0);
	mTexts.push_back(aPoints);
	mWidgetManager->AddWidget(aPoints);
}

// 0x4246c0
int Board::CountBurning()
{
	int aCount = 0;
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece->mType == 6 && aPiece->mKind == 0xb && !aPiece->mUsed)
			aCount++;
	}
	return aCount;
}

// 0x424700
void Board::Ping()
{
	if (mPingCooldown == 0)
	{
		mApp->PlaySample(1);
		mPingCooldown = 0xf;
	}
}

// 0x4259c0 (its argument is a stack argument that the decompiler dropped): adds points, scaled by the difficulty
// (0x425990: 1, 1.5 or 2), times the number of tiles in the chain once there are several
int Board::AddScore(int thePoints)
{
	double aFactor;
	if (mApp->mDifficulty == 1)
		aFactor = 1.0;
	else if (mApp->mDifficulty == 2)
		aFactor = 1.5;
	else
		aFactor = 2.0;
	int aPoints = Ftol(aFactor * (double) thePoints);
	if (1 < mComboCount)
		aPoints = mComboCount * aPoints;
	mScore = mScore + aPoints;
	if (mScore < 0)
		mScore = 0;
	return aPoints;
}

// 0x425940
void Board::EndLevelA()
{
	mOverflowTimer = 500;
	mApp->mHead->StartSlide(false, 0);
	mState = 4;
	mLevelEndCountdown = 400;
}

// 0x425980
void Board::EndLevelB()
{
	mState = 5;
}

// 0x425a00, 0x425a30: the two flames of the bar (a marker piece of kind 8 that fades out). The original stores the places as
// doubles, which the decompiler shows as halves: 0x403d0000 and 0x4064c000 are 29.0 and 166.0, 0x40460000 is 44.0
void Board::MakeFlame0()
{
	Piece* aPiece = MakeSpecial(8, NULL);
	aPiece->mX = 29.0;
	aPiece->mY = 166.0;
	aPiece->mIndex = IMAGE_ROCKET_GLOW;     // (the original's 0x6e, its table's entry of "rocket-glow")
}

// 0x425a30: the second flame marker (MakeFlame0 is 0x425a00)
void Board::MakeFlame1()
{
	Piece* aPiece = MakeSpecial(8, NULL);
	aPiece->mX = 44.0;
	aPiece->mY = 166.0;
	aPiece->mIndex = IMAGE_MATCH_GLOW;      // (the original's 0x6f)
}

// 0x425a60
void Board::MakeMarker(int theRow, int theKind)
{
	Piece* aPiece = MakeSpecial(9, NULL);
	aPiece->mIndex = theKind;
	aPiece->mRow = theRow;
}

// 0x425a80: the rocket that leaves the right edge at a row after a launch: flies up and to the right
void Board::MakeRocket(int theRow)
{
	Piece* aPiece = MakeSpecial(5, mRight[theRow]);
	aPiece->mSlideStartX = Ftol(aPiece->mX + 1.0);
	aPiece->mSlideStartY = Ftol(aPiece->mY - 77.0);
	aPiece->mSlideEndY = aPiece->mSlideStartY - 200;
	aPiece->mSlideEndX = aPiece->mSlideStartX + 200;
	aPiece->mMoving = true;
	aPiece->mSlideStep = 0.015;
	aPiece->mSlideProgress = 0;
	Effect* anEffect = mEffects->Add(3, false);
	aPiece->mFlash = anEffect;
	anEffect->mX = (aPiece->mX - 18.0) + 8.0;
	anEffect->mY = (aPiece->mY + 53.0) - 8.0;
	anEffect->mLife = 100;
}

// 0x4251b0 floods the connected tiles from a tile of the first column (theFrom = 0) or the last: marks each tile
// with the number of its path (mPath) and, in mPathEdges / mPathLeftLive / mPathRightLive, which edges the path touches
void Board::Flood(int theColumn, int theRow, int thePath, int* theCount, unsigned int* theEnds, int theFrom)
{
	Piece* aPiece = mGrid[theRow][theColumn];
	if (aPiece->mPath == thePath)
		return;
	*theCount = *theCount + 1;
	aPiece->mPathFrom = theFrom;
	aPiece->mPath = thePath;
	unsigned int aMask = gPipeMask[aPiece->mRotation + aPiece->mKind * 4];
	if (theColumn == 0 && (aMask & 8) != 0)
	{
		unsigned int aEnds = *theEnds;
		if ((aEnds & 1) == 0)
			aEnds = aEnds | 1;
		else
			aEnds = aEnds | 0x10;
		*theEnds = aEnds;
		if (mLeft[theRow] != NULL)
			mLeftTouched[theRow] = true;
		mPathEdges[thePath] = mPathEdges[thePath] | 1;
		bool aValue = true;
		if (!mPathLeftLive[thePath] && !mLeftTouched[theRow])
			aValue = false;
		mPathLeftLive[thePath] = aValue;
	}
	else if (theColumn == mCols - 1 && (aMask & 2) != 0)
	{
		unsigned int aEnds = *theEnds;
		if ((aEnds & 2) == 0)
			aEnds = aEnds | 2;
		else
			aEnds = aEnds | 0x20;
		*theEnds = aEnds;
		if (mRight[theRow] != NULL || mRightEdgeLoaded)
			mRightTouched[theRow] = true;
		mPathEdges[thePath] = mPathEdges[thePath] | 2;
		bool aValue = true;
		if (!mPathRightLive[thePath] && mRight[theRow] == NULL)
			aValue = false;
		mPathRightLive[thePath] = aValue;
	}
	unsigned int aBits;
	if (theRow == 0 && (aMask & 1) != 0)
	{
		unsigned int aEnds = *theEnds;
		if ((aEnds & 4) == 0)
			*theEnds = aEnds | 4;
		else
			*theEnds = aEnds | 0x40;
		aBits = mPathEdges[thePath] | 4;
		mPathEdges[thePath] = aBits;
	}
	else if (theRow == mRows - 1 && (aMask & 4) != 0)
	{
		unsigned int aEnds = *theEnds;
		if ((aEnds & 8) == 0)
			*theEnds = aEnds | 8;
		else
			*theEnds = aEnds | 0x80;
		aBits = mPathEdges[thePath] | 8;
		mPathEdges[thePath] = aBits;
	}
	for (int i = 0; i < 4; i++)
	{
		if ((gPipeSteps[i].mOut & aMask) == 0)
			continue;
		int aNeighborRow = gPipeSteps[i].mRow + theRow;
		int aNeighborColumn = gPipeSteps[i].mCol + theColumn;
		if (aNeighborColumn >= 0 && aNeighborColumn < mCols && aNeighborRow >= 0 && aNeighborRow < mRows)
		{
			Piece* aNeighbor = mGrid[aNeighborRow][aNeighborColumn];
			if (aNeighbor != NULL && aNeighbor->mType == 1 && aNeighbor->mPath == 0 &&
				(gPipeSteps[i].mIn & gPipeMask[aNeighbor->mRotation + aNeighbor->mKind * 4]) != 0)
				Flood(aNeighborColumn, aNeighborRow, thePath, theCount, theEnds, theFrom);
		}
	}
}
