// The board's matches: finding them (0x6cd468), the special gems they make and what follows a fall (0x6cf3fd)
#include "Board.h"

#include "EffectsManager.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Piece.h"
#include "Points.h"
#include "Shockwave.h"
#include "SpeedText.h"
#include "Traveller.h"
#include "Profile.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstdlib>
#include <set>

using namespace Sexy;

namespace {

// 0x84e8dc: the points of a match by its length; 0x84e900: those of a cascade by its depth (to 6)
const int kMatchPoints[9] = {0, 0, 0, 25, 50, 100, 100, 100, 100};
const int kCascadePoints[7] = {0, 25, 50, 75, 100, 150, 200};
// 0x84e9e0: the speed bonus for each sixth quick match, and 0x84e9f0 the highest of them
const int kSpeedBonusPoints[4] = {0, 500, 1000, 2000};
const int kSpeedBonusMax = 3;

} // namespace

int Sexy::gSparkDelay = 0;

void Board::StartSparks(Piece* thePiece)
{
	thePiece->mUnknownF0 = 0;
	thePiece->mUnknownEC = true;
	thePiece->mUnknownF4 = gSparkDelay;
}

void Board::ResetFruitCount()
{
	if (mUnknown2664 != mUnknown2660 && (mUnknown2660 != 0 || mUnknown2664 == -1))
		return;
	// 0x6b35b1: the fruit's count starts again; a fruit dance that has begun (3) moves on to its end
	mUnknown2660 = 0;
	mUnknown2664 = 0;
	if (mUnknown265C == 3) {
		mUnknown265C = 5;
		mGoodTube->mUnknown5F0.SetCurve(GTS(0x7dc558));
	}
}

void Board::MakePowerGem(Piece* thePiece, float theDelay)
{
	if (thePiece == nullptr)
		return;
	thePiece->MakePower(theDelay);
	if (!mUnknown37F8) {
		if (!IsBusy())
			AddStat(0x15, 1);
		if (mApp->mUnknownC74[0x26] == 0)
			SetStat(0x26, mUnknown158);
	}
}

void Board::MakeElectroGem(Piece* thePiece, float theDelay)
{
	if (thePiece == nullptr)
		return;
	thePiece->mColor = thePiece->mColor % 7;
	thePiece->MakeElectro(theDelay);
	if (!mUnknown37F8) {
		AddStat(0x14, 1);
		if (mApp->mUnknownC74[0x27] == 0)
			SetStat(0x27, mUnknown158);
	}
}

void Board::Make6Gem(Piece* thePiece, float theDelay)
{
	thePiece->Make6(theDelay);
	if (!IsBusy())
		AddStat(0x16, 1);
	if (mApp->mUnknownC74[0x25] == 0)
		SetStat(0x25, mUnknown158);
}

void Board::MakeFruitGem(Piece* thePiece)
{
	if (thePiece == nullptr)
		return;
	if (thePiece->IsFlagSet(9) || thePiece->IsFlagSet(2))
		BreakSpecial(thePiece, false);
	thePiece->ClearSpecial();
	thePiece->SetFlag(0, true);
	thePiece->mUnknown29C = 0;
	thePiece->mBombCount = 0;
	thePiece->mBlasted = false;
	if (6 < thePiece->mColor || thePiece->mColor < 0)
		thePiece->mColor = Rand85B970() % 7;
	if (!mUnknown37F8) {
		thePiece->mScale288 = 0;
		thePiece->mUnknownE0 = 120;
		if (!IsBusy()) {
			AddStat(0x18, 1);
			thePiece->mDevilGlow = 1.0f;  // 0x6a4f56
			thePiece->mJustMade = 5;
			mApp->PlaySample(SOUND_FRUIT_APPEARS);
		}
	}
	mUnknown21DA = true;
}

void Board::ClearHitCounts()
{
	int aLeft = mCols * mRows;
	Piece** aCell = mGrid;
	while (aLeft != 0) {
		Piece* aPiece = *aCell;
		aLeft--;
		aCell++;
		if (aPiece != nullptr)
			aPiece->mUnknown10C = 0;
	}
}

void Board::ClearSparkStarts()
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				aPiece->mUnknownF4 = 0;
				aPiece->mUnknownF0 = 0;
				aPiece->mUnknownEC = false;
				aPiece->mUnknown2F4 = false;
			}
		}
	}
}

Piece* Board::AddPoints(int theX, int theY, int thePoints, const Color& theColor, int theSerial, bool theUnknown1,
	bool theUnknown2, bool theFirst, bool theUnknown3)
{
	if (thePoints < 1)
		return nullptr;
	if (mGameMode == MODE_CHALLENGE) {
		// port: a challenge counts points only when its flags (+0x80) have 8; it comes with the challenges
		if (mUnknown2630 == 0)
			return nullptr;
		NotPorted("Board::AddPoints: the challenge's flags");
		return nullptr;
	}
	int aMult = mUnknown21D0;
	if (!theUnknown3)
		aMult = 1;
	int aPoints = aMult * thePoints;
	if (1 < mUnknown2580 && theUnknown2)
		aPoints *= mUnknown2580;
	int aTotal = mModeMultiplier * aPoints;
	if (mUnknown2041 && -1 < theX && -1 < theY) {
		// A serial of -0x68 to -0x65 (the kinds of special gem) takes a new one, which remembers the kind
		if ((unsigned int)theSerial >= (unsigned int)-0x68 && (unsigned int)theSerial <= (unsigned int)-0x65) {
			int aNew = gSettleCount;
			gSettleCount++;
			mUnknown37B8[aNew] = theSerial;
			theSerial = aNew;
		}
		// The points float up: a floater of the same serial (the same move's chain) takes them, else a new one
		int aSum = mUnknown2580 * thePoints;
		int aScaled = Ftol(std::pow((double)aMult, 0.45) * (double)aSum);
		Points* aFloater = nullptr;
		int aShown = aTotal;
		if (theSerial == -1) {
			theFirst = false;
		} else {
			for (unsigned int i = 0; i < mPointsManager->mPoints.size(); i++) {
				Points* aPoints = mPointsManager->mPoints[i];
				if (aPoints->mSerial == theSerial) {
					double aGrow = (double)aPoints->mTargetScale + 0.05000000074505806;
					aPoints->mPoints += aTotal;
					aPoints->mTargetScale = (float)(1.2000000476837158 >= aGrow ? aGrow : 1.2000000476837158);
					aScaled += aPoints->mUnknown1C;
					aPoints->mLife = 1.0f;
					aSum += aPoints->mUnknown18;
					aFloater = aPoints;
					aShown = aPoints->mPoints;
					break;
				}
			}
		}
		SexyString aText = StrFormat(_S("%d"), aShown);
		// How big the points are, 0 to 1: from 25 to 425 of them
		double aBig = (double)(float)((double)aSum - 25.0) / 400.0;
		float aSize = 0.0f;
		if (!(aBig < 0.0))
			aSize = (float)(1.0 >= aBig ? aBig : 1.0);
		double aBigScaled = (double)(float)((double)aScaled - 25.0) / 400.0;
		float aSizeScaled = 0.0f;
		if (!(aBigScaled < 0.0))
			aSizeScaled = (float)(1.0 >= aBigScaled ? aBigScaled : 1.0);
		double aDouble = (double)aSize + (double)aSize;
		if (!(aDouble <= 1.0))
			aDouble = 1.0;
		float aTargetScale = (float)(aDouble * 0.6500000357627869 + 0.550000011920929);
		if (theY <= 120)
			theY = 120;
		if (aFloater == nullptr) {
			if (FONT_FLOATERS != nullptr) {
				aFloater = new Points(mApp, FONT_FLOATERS, aText, theX, theY, 1.0f, 0, theColor, -1);
				aFloater->mTargetScale = aTargetScale;
				aFloater->mSerial = theSerial;
				aFloater->mSpring = 0.15f;
				float aDamping = (float)((double)aSum * 0.0015 + 0.46);
				aFloater->mDamping = aDamping;
				if (!((double)aDamping <= 0.9620000123977661))
					aFloater->mDamping = 0.962f;
				aFloater->mPoints = aTotal;
				mPointsManager->AddPoints(aFloater);
			}
		} else {
			aFloater->mText = aText;
			aFloater->Flash();
			if (!aFloater->mDrawn) {
				aFloater->mX = (float)(((double)theX + (double)aFloater->mX) * 0.5);
				aFloater->mY = (float)(((double)theY + (double)aFloater->mY) * 0.5);
			}
		}
		if (aFloater != nullptr) {
			// Its layers run through the rainbow mixed with its colour, the more so the bigger the points
			static const Color kRainbow[6] = {Color(255, 0, 0), Color(255, 255, 0), Color(0, 255, 0), Color(0, 255, 255),
				Color(0, 0, 255), Color(255, 0, 255)};  // 0x85c498
			aFloater->mColor = theColor;
			aFloater->mCycles[0].mColors.clear();
			aFloater->mCycles[1].mColors.clear();
			float aRed = (float)theColor.mRed;
			float aGreen = (float)theColor.mGreen;
			float aBlue = (float)theColor.mBlue;
			for (int k = 0; k < 6; k++) {
				for (int aLayer = 0; aLayer < 3; aLayer++) {
					float aMix = 0;
					float aBright = 0;
					float anAlpha = 0;
					if (aLayer == 0 || aLayer == 1) {
						double aRise = ((double)aSizeScaled - 0.30000001192092896) * 2.0;
						aMix = (float)(0.0 > aRise ? 0.0 : (1.0 >= aRise ? aRise : 1.0));
					}
					if (aLayer == 0) {
						double anOver = (double)aSizeScaled - 0.5;
						float anExtra = (float)(0.0 <= anOver ? anOver : 0.0);
						aBright = (k % 2 != 0) ? (float)(0.5 + (double)anExtra) : 0.5f;
						anAlpha = 1.0f;
					} else if (aLayer == 1) {
						double anOver = (double)aSizeScaled - 0.10000000149011612;
						float anExtra = (float)(anOver >= 0.0 ? anOver : 0.0);
						aBright = (k % 2 == 0) ? 0.5f : (float)((double)anExtra + 0.5);
						double aStrong = ((double)aSizeScaled - 0.5) * 3.0;
						anAlpha = (float)(aStrong >= 0.0 ? aStrong : 0.0);
					} else {
						double anOver = (double)aSizeScaled - 1.0;
						aMix = (float)(0.0 > anOver ? 0.0 : (anOver <= 1.0 ? anOver : 1.0));
						aBright = 1.0f;
						anAlpha = 0.7f;
					}
					double aKeep = 1.0 - (double)aMix;
					double aR = (double)aRed * (double)aBright * aKeep;
					double aG = (double)aGreen * (double)aBright * aKeep;
					double aB = (double)aBlue * (double)aBright * aKeep;
					Color aColor;
					aColor.mRed = Ftol((255.0 >= aR ? aR : 255.0) + (double)kRainbow[k].mRed * (double)aMix);
					aColor.mGreen = Ftol((aG <= 255.0 ? aG : 255.0) + (double)kRainbow[k].mGreen * (double)aMix);
					aColor.mBlue = Ftol((255.0 >= aB ? aB : 255.0) + (double)kRainbow[k].mBlue * (double)aMix);
					aColor.mAlpha = Ftol(255.0 * (double)anAlpha);
					aFloater->mCycles[aLayer].mColors.push_back(aColor);
				}
			}
			aFloater->mCycles[0].SetPosition(0.25f);
			aFloater->mCycles[1].SetPosition(0.75f);
			aFloater->mUnknown1C = aScaled;
			aFloater->mFlash = (float)((double)aSize * 0.699999988079071);
			aFloater->mUnknown18 = aSum;
			aFloater->mUnknown188 = true;
			double anOpacity = (double)aSizeScaled * 0.0 + 0.75;
			if (!(1.0 >= anOpacity))
				anOpacity = 1.0;
			aFloater->mColor.mAlpha = Ftol(anOpacity * 255.0);
			aFloater->mLife = (float)((double)aSize * 1.600000023841858 + 0.6000000238418579);
			aFloater->mScale = aFloater->mTargetScale + aFloater->mTargetScale;
		}
	}
	// Classic's progress: the points count less as the level's gauge fills, by the chains' multiplier
	float aMultF = (float)aMult;
	int aChains = mUnknown2234[0x21];
	float anAvg = aMultF;
	if (0 < aChains)
		anAvg = (float)((double)mUnknown2234[0x39] / (double)aChains);
	float aHigh = anAvg <= aMultF ? aMultF : anAvg;
	float aLow = anAvg < aMultF ? anAvg : aMultF;
	float aScale = (float)std::pow((double)aLow * 0.4 + (double)aHigh * 0.6, 0.7);
	if (!theUnknown3)
		aScale = 1.0f;
	int aLeft = aTotal;
	if (0 < aTotal) {
		double aRatio = (double)(aScale / aMultF);
		do {
			int aStep = 100;
			if (aLeft < 100)
				aStep = aLeft;
			int aTo = Ftol(std::pow((double)(mUnknown1544 + aStep) * aRatio, 0.75));
			int aFrom = Ftol(std::pow((double)mUnknown1544 * aRatio, 0.75));
			int aGain = (aTo - aFrom) * 3;
			if (theUnknown1 && mUnknown2041) {
				mUnknown200 += aGain;
				mUnknown14FC += aGain;
			}
			aLeft -= aStep;
			// (the original adds the whole points each step, not the step's)
			mUnknown1544 += aTotal;
		} while (0 < aLeft);
	}
	if (!mUnknown2629 && !mUnknown37C4 && 0 < mUnknown158)
		mApp->mProfile->mTotalPoints += aTotal;
	if (mGameMode == MODE_CHALLENGE)
		theFirst = false;
	if (!mGoodTube->mUnknown14D || theX < 1 || theY < 1 || !theFirst || !mUnknown2041) {
		if (mUnknown2041)
			SetScore(mUnknown168 + aTotal, false);
		if (theFirst)
			AddChainStep();
	} else {
		// The points go to the tube as smoke, to the next light of its level; they reach the score on arrival
		int aSteps = mGoodTube->mUnknown274;
		int aDiv = GetLevelSteps(mGoodTube->mUnknown27C);
		int aPoint[2];
		mGoodTube->GetStepPoint(aPoint, ((int)mUnknown2170.size() + aSteps) % aDiv, true);
		SmokeTraveller* aSmoke = new SmokeTraveller(theColor, theX, theY, mGoodTube->mX + aPoint[0],
			mGoodTube->mY + aPoint[1], aTotal, false);
		aSmoke->mBurst = true;
		mUnknown2170.push_back(aSmoke);
	}
	std::map<int, int>::iterator anIt = mUnknown37B8.find(theSerial);
	int aKind = anIt != mUnknown37B8.end() ? anIt->second : 0;
	if (aKind == -0x68)
		AddStat(0x3d, aTotal);
	else if (aKind == -0x67)
		AddStat(0x3c, aTotal);
	else if (aKind == -0x66)
		AddStat(0x3b, aTotal);
	else if (aKind == -0x65)
		AddStat(0x3a, aTotal);
	mUnknown2234[0x49] += aTotal;
	UpdateProgress(false);
	return nullptr;
}

void Board::ShowCountPopup()
{
	if (IsBusy())
		return;
	if (mUnknown37B0 && mUnknown37B4 <= mUnknown1F8) {
		// port: 0x6b3ffc's popup of the count (FONT_POPUP_COUNT, the curve 0x... at +0x37a4) comes with the popups
		NotPorted("Board::ShowCountPopup (0x6b3ffc)");
	}
}

int Board::FindMatches(unsigned int theFlags)
{
	bool aReal = (theFlags & 2) != 0;
	mUnknown1FF0 = 0;
	mUnknown1FEC = 0;
	mUnknown1FE8 = 0;
	if (aReal) {
		ClearSparkStarts();
		mUnknown2140.clear();
	}
	ClearHitCounts();
	if (mUnknown1382)
		return 0;
	std::vector<Piece*> aReset;    // the gems whose state is cleared once the specials are chosen
	bool aFirst = mUnknown1508 == 0;
	std::vector<MatchInfo> aMatches;
	std::set<int> aTipGems;
	int anAvgX = 0;
	// Across: runs of one colour; a gap does not break a run
	for (int aRow = 0; aRow < mRows; aRow++) {
		int aStart = -1;
		int aColor = -1;
		int aCount = 0;
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				continue;
			bool anEnd = aCol == mCols - 1;
			if (aPiece->mColor == aColor && aPiece->CanMove())
				aCount++;
			else
				anEnd = true;
			if (anEnd) {
				if (mUnknown1C68 <= aCount) {
					MatchInfo anInfo;
					anInfo.mRow = aRow;
					anInfo.mCount = aCount;
					int anEndCol = aCount + aStart;
					anInfo.mKind = 1;
					anInfo.mColor = aColor;
					anInfo.mCol = aStart;
					for (int i = aStart; i < anEndCol; i++) {
						Piece* aGem = GetPieceAt(aRow, i);
						if (aGem != nullptr)
							anInfo.mIds.push_back(aGem->mId);
					}
					aMatches.push_back(anInfo);
				}
				aColor = aPiece->CanMove() ? aPiece->mColor : -1;
				aCount = 1;
				aStart = aCol;
			}
		}
	}
	// Down
	for (int aCol = 0; aCol < mCols; aCol++) {
		int aStart = -1;
		int aColor = -1;
		int aCount = 0;
		for (int aRow = 0; aRow < mRows; aRow++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				continue;
			bool anEnd = aRow == mRows - 1;
			if (aPiece->mColor == aColor && aPiece->CanMove())
				aCount++;
			else
				anEnd = true;
			if (anEnd) {
				if (mUnknown1C68 <= aCount) {
					MatchInfo anInfo;
					anInfo.mCol = aCol;
					anInfo.mCount = aCount;
					int anEndRow = aCount + aStart;
					anInfo.mKind = 2;
					anInfo.mColor = aColor;
					anInfo.mRow = aStart;
					for (int i = aStart; i < anEndRow; i++) {
						Piece* aGem = GetPieceAt(i, aCol);
						if (aGem != nullptr)
							anInfo.mIds.push_back(aGem->mId);
					}
					aMatches.push_back(anInfo);
				}
				aColor = aPiece->CanMove() ? aPiece->mColor : -1;
				aCount = 1;
				aStart = aRow;
			}
		}
	}
	// A challenge's squares: a gem and its four corners of one colour
	if (mGameMode == MODE_CHALLENGE && (mUnknown3918 >> 6 & 1) != 0 && mUnknown1C98 && 1 < mRows - 1) {
		int aRow = 1;
		do {
			int aCol = 1;
			if (1 < mCols - 1) {
				do {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr && aPiece->mUnknown88 == 0) {
						std::vector<Piece*> aSquare;
						aSquare.push_back(aPiece);
						aSquare.push_back(GetPieceAt(aRow + 1, aCol + 1));
						aSquare.push_back(GetPieceAt(aRow - 1, aCol - 1));
						aSquare.push_back(GetPieceAt(aRow + 1, aCol - 1));
						aSquare.push_back(GetPieceAt(aRow - 1, aCol + 1));
						std::vector<Piece*> aSame;
						for (unsigned int i = 0; i < aSquare.size(); i++) {
							Piece* aGem = aSquare[i];
							if (aGem != nullptr && aGem->CanMove() && aGem->mUnknown88 == 0 &&
								aGem->mColor == aPiece->mColor)
								aSame.push_back(aGem);
						}
						if (aSame.size() == aSquare.size()) {
							MatchInfo anInfo;
							anInfo.mRow = aPiece->mRow;
							anInfo.mCol = aPiece->mCol;
							anInfo.mKind = 4;
							anInfo.mColor = aPiece->mColor;
							for (unsigned int i = 0; i < aSquare.size(); i++)
								anInfo.mIds.push_back(aSquare[i]->mId);
							aMatches.push_back(anInfo);
						}
					}
					aCol++;
				} while (aCol < mCols - 1);
			}
			aRow++;
		} while (aRow < mRows - 1);
	}
	int aMatchCount = (int)aMatches.size();
	int aSets;
	if (aMatchCount < 1)
		aSets = 1;
	else if (aMatchCount < 4)
		aSets = aMatchCount;
	else
		aSets = 3;
	mUnknown2580 = aSets;
	unsigned int aMatchIndex = 0;
	if (aMatchCount != 0) {
		do {
			MatchInfo& aMatch = aMatches[aMatchIndex];
			if (mUnknown21F4 == -1)
				mUnknown21F4 = aMatch.mColor;
			if (mApp->mUnknownC74[0x1c] < aMatch.mCount) {
				SetStat(0x1c, aMatch.mCount);
				SetStat(0x1d, aMatch.mColor);
			}
			if (mUnknown2234[0x1c] < aMatch.mCount) {
				mUnknown2234[0x1c] = aMatch.mCount;
				mUnknown2234[0x1d] = aMatch.mColor;
			}
			int aFruits = 0;
			int anElectros = 0;
			int aPowers = 0;
			int aSixes = 0;
			int aRow = aMatch.mRow;
			int aCol = aMatch.mCol;
			Piece* aCentre = GetPieceAt(aRow, aCol);
			if (aCentre == nullptr)
				goto next_match;
			if (aReal) {
				// The match's shockwave, as strong as the cascade, pushes the gems round the match away from it
				MatchShockwave* aWave = new MatchShockwave(Ftol((double)(gGemWidth / 2) + (double)aCentre->mX),
					Ftol((double)(gGemHeight / 2) + (double)aCentre->mY));
				aWave->SetStrength((float)mUnknown1508);
				mUnknown1660.push_back(aWave);
				if (aMatch.mKind == 1) {
					if (GetPieceAt(aRow, aCol - 1) != nullptr)
						aWave->AddPush(aRow, aCol - 1, -1, 0);
					int anEnd = aMatch.mCount + aCol;
					if (GetPieceAt(aRow, anEnd) != nullptr)
						aWave->AddPush(aRow, anEnd, 1, 0);
					int aCell = aCol;
					if (aCell < aMatch.mCount + aCol) {
						do {
							if (GetPieceAt(aRow - 1, aCell) != nullptr)
								aWave->AddPush(aRow - 1, aCell, 0, -1);
							if (GetPieceAt(aRow + 1, aCell) != nullptr)
								aWave->AddPush(aRow + 1, aCell, 0, 1);
							aCell++;
						} while (aCell < aMatch.mCount + aCol);
					}
				} else if (aMatch.mKind == 2) {
					if (GetPieceAt(aRow - 1, aCol) != nullptr)
						aWave->AddPush(aRow - 1, aCol, 0, -1);
					int anEnd = aMatch.mCount + aRow;
					if (GetPieceAt(anEnd, aCol) != nullptr)
						aWave->AddPush(anEnd, aCol, 0, 1);
					int aCell = aRow;
					if (aCell < aMatch.mCount + aRow) {
						do {
							if (GetPieceAt(aCell, aCol - 1) != nullptr)
								aWave->AddPush(aCell, aCol - 1, -1, 0);
							if (GetPieceAt(aCell, aCol + 1) != nullptr)
								aWave->AddPush(aCell, aCol + 1, 1, 0);
							aCell++;
						} while (aCell < aMatch.mCount + aRow);
					}
				}
			}
			if (aMatch.mKind == 4 && aReal) {
				// The square: its corners fly into the middle gem, which turns
				if (aCentre->mPendingKind == 0)
					BreakPiece(aCentre);
				Make6Gem(aCentre, 0.25f);
				aCentre->mPendingDY = 0;
				aCentre->mPendingDX = 0;
				for (unsigned int i = 0; i < aMatch.mIds.size(); i++) {
					Piece* aGem = GetPieceById(aMatch.mIds[i]);
					if (aGem != nullptr && !aGem->mUnknown51D) {
						BreakPiece(aGem);
						if (aGem->mPendingKind == 0) {
							if (aGem->mUnknown538 == 0) {
								aGem->mPendingDX = (float)(((double)aCentre->mX - (double)aGem->mX) / 25.0);
								aGem->mPendingDY = (float)(((double)aCentre->mY - (double)aGem->mY) / 25.0);
								aGem->mPendingTime = 0.25f;
								aGem->mPendingKind = 0x40;
								aGem->mUnknown2EC = aMatch.mSerial;
								aGem->mUnknown4FC = 0;
							}
						} else {
							aReset.push_back(aGem);
						}
					}
				}
				aReset.push_back(aCentre);
				aCentre->mUnknownEC = false;
				aCentre->mBombCount = 0;
				aCentre->ClearFlag(Piece::FLAG_BOMB);
			}
			{
				// Each gem of the match
				int aSumX = 0;
				for (unsigned int i = 0; i < aMatch.mIds.size(); i++) {
					Piece* aGem = GetPieceById(aMatch.mIds[i]);
					if (aGem == nullptr)
						continue;
					if (theFlags != 0)
						StartSparks(aGem);
					if (aReal) {
						if (!aGem->mBreaking)
							aGem->mUnknown110 = aMatchIndex * 15;
						if (aGem->mUnknown10C == 0) {
							mUnknown1F0++;
							if (!aGem->mUnknown537) {
								if (!mUnknown2629 && 0 < mUnknown158)
									mApp->mProfile->mGemsCleared++;
								mUnknown1FC++;
								mUnknown1F8++;
								aGem->mUnknown537 = true;
								AddStat(0x3f, 1);
								bool anElectro = aGem->IsFlagSet(Piece::FLAG_ELECTRO);
								if (anElectro)
									AddStat(0x41, 1);
								if (aGem->IsFlagSet(Piece::FLAG_POWER)) {
									AddStat(0x40, 1);
									if (anElectro)
										AddStat(0x53, 1);
								}
								ShowCountPopup();
							}
						}
					}
					aGem->mUnknown10C++;
					if (aGem->IsFlagSet(Piece::FLAG_ELECTRO))
						anElectros++;
					else if (aGem->IsFlagSet(Piece::FLAG_POWER))
						aPowers++;
					else if (aGem->IsFlagSet(Piece::FLAG_6))
						aSixes++;
					else if (aGem->IsFlagSet(Piece::FLAG_FRUIT))
						aFruits++;
					aSumX = Ftol((double)(gGemWidth / 2) + (double)aGem->mX + (double)aSumX);
				}
				int aLength = aMatch.mCount;
				bool aPower = false;
				bool anElectro = false;
				anAvgX += aSumX / aLength;
				// A match of 4 makes a power gem, of 5 or more an electro gem
				if (aFruits < 3 && mUnknown1C68 < aLength && (aMatch.mKind == 1 || aMatch.mKind == 2)) {
					if (mUnknown1C68 + 1 < aLength) {
						if ((mUnknown3914 >> 1 & 1) == 0 && 0 < mUnknown158)
							aTipGems.insert(aMatch.mIds[0]);
						anElectro = true;
					} else {
						if ((mUnknown3914 & 1) == 0 && 0 < mUnknown158)
							aTipGems.insert(aMatch.mIds[0]);
						aPower = true;
					}
				}
				// Before the profile has them, an electro gem's match makes a power gem, and none makes a power gem
				if (((mUnknown3918 >> 3 & 1) == 0 || mUnknown158 == 0) && anElectro) {
					anElectro = false;
					aPower = true;
				}
				if ((mUnknown3918 >> 4 & 1) == 0 || mUnknown158 == 0)
					aPower = false;
				gSparkDelay = 0;
				if (aReal) {
					int aPoints = 0;
					int aCascade = mUnknown1508;
					if (aCascade == 0 || mUnknown150C) {
						if (6 < aCascade)
							aCascade = 6;
						aPoints = kCascadePoints[aCascade] + kMatchPoints[aMatch.mCount];
					}
					int aSerial = aMatch.mSerial;
					for (unsigned int i = 0; i < aMatch.mIds.size(); i++) {
						Piece* aGem = GetPieceById(aMatch.mIds[i]);
						if (aGem != nullptr) {
							aGem->mUnknown2EC = aSerial;
							if (aGem->IsFlagSet(Piece::FLAG_FRUIT) && !aGem->mUnknown116)
								aGem->mUnknown116 = true;
						}
					}
					if (anElectros == 0 || aPowers == 0 || aSixes == 0) {
						if (aPower || anElectro)
							MakeMatchSpecial(aMatch, aReset, aPower, anElectro);
					} else {
						// A power, an electro and a type 6 gem in one match: every gem on the board loses its
						// special and blows, from the middle rows out
						Piece** aCell = mGrid;
						for (int aLeft = mCols * mRows; 0 < aLeft; aLeft--) {
							Piece* aGem = *aCell;
							if (aGem != nullptr && !aGem->mBreaking) {
								if (aGem->IsFlagSet(Piece::FLAG_POWER) || aGem->IsFlagSet(Piece::FLAG_6) ||
									aGem->IsFlagSet(Piece::FLAG_ELECTRO))
									aGem->ClearSpecial();
								aGem->mUnknown88 = std::abs(aGem->mRow - mRows / 2) * 20 + 10;
								aGem->mUnknown2EC = aLeft;
							}
							aCell++;
						}
						AddStat(0x1e, 1);
					}
					aMatch.mUnknown2C = aPoints;
				}
				// The tips for power and electro gems, in casual and classic
				if ((theFlags & 1) != 0 && (mGameMode == MODE_ZEN || mGameMode == MODE_CLASSIC)) {
					if (!aPower || (mUnknown3914 & 1) != 0) {
						if (!anElectro || (mUnknown3914 >> 1 & 1) != 0)
							goto next_match;
						mUnknown814 = 0x50;
					} else {
						mUnknown814 = 0x28;
					}
					mUnknown23EC = 1.0f;
					mUnknown23F0 = 0.015f;
				}
			}
next_match:
			aMatchIndex++;
		} while (aMatchIndex < aMatches.size());
	}
	if (aMatches.size() != 0)
		anAvgX = anAvgX / (int)aMatches.size();
	if (!aReal) {
		// Only the tips: a gem in two matches
		bool aPowerTips = (mUnknown3918 >> 4 & 1) != 0 && 0 < mUnknown158;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr)
					continue;
				if (aPowerTips && 1 < aPiece->mUnknown10C && (theFlags & 1) != 0 &&
					(mGameMode == MODE_ZEN || mGameMode == MODE_CLASSIC) && (mUnknown3914 & 1) == 0) {
					aTipGems.insert(aPiece->mId);
					mUnknown23EC = 1.0f;
					mUnknown814 = 0x28;
					mUnknown23F0 = 0.015f;
				}
				aPiece->mUnknown10C = 0;
			}
		}
		return FinishMatches(aMatches, aTipGems);
	}
	for (unsigned int i = 0; i < aReset.size(); i++) {
		Piece* aGem = aReset[i];
		aGem->mUnknownF0 = 0;
		aGem->mGlow = 0;
		aGem->mBreaking = false;
		aGem->mUnknown29C = 0;
		aGem->mBlasted = false;
		aGem->mUnknown10C = 0;
		aGem->mUnknownEC = false;
		aGem->mUnknown88 = 0;
		aGem->mUnknown38 = false;
		aGem->mUnknown537 = false;
	}
	{
		// A gem in two matches (an L or a T) becomes a power gem
		bool aPowerGems = (mUnknown3918 >> 4 & 1) != 0 && 0 < mUnknown158;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr || !aPowerGems || aPiece->mUnknown10C < 2)
					continue;
				aPiece->mBreaking = false;
				if (!aPiece->IsSpecial() || aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->mUnknown51D ||
					aPiece->mPendingKind != 0) {
					MakePowerGem(aPiece, 0.0f);
				} else {
					BreakPiece(aPiece);
					if (aPiece->IsFlagSet(Piece::FLAG_FRUIT))
						continue;
					aPiece->mBreaking = false;
					aPiece->mBlasted = false;
					aPiece->mUnknown538 = 0x10;
				}
				int aSerial = -1;
				aPiece->mUnknown29C = 0;
				aPiece->mUnknown10C = 0;
				aPiece->mUnknown88 = 0;
				aPiece->mUnknown51D = true;
				for (unsigned int m = 0; m < aMatches.size(); m++) {
					MatchInfo& aMatch = aMatches[m];
					for (unsigned int i = 0; i < aMatch.mIds.size(); i++) {
						Piece* aGem = GetPieceById(aMatch.mIds[i]);
						if (aGem == nullptr)
							continue;
						if (aSerial == -1) {
							int aColor = aGem->mColor;
							AddPoints(Ftol((double)aGem->mX), Ftol((double)aGem->mY), 25, gGemColors[aColor],
								aMatch.mSerial, true, true, aFirst, true);
							aSerial = aMatch.mSerial;
							aFirst = false;
						} else {
							aMatch.mSerial = aSerial;
						}
						aGem->mUnknown2EC = aPiece->mUnknown2EC;
						if (aGem == aPiece) {
							// The match's other gems fly into it
							for (unsigned int j = 0; j < aMatch.mIds.size(); j++) {
								Piece* anOther = GetPieceById(aMatch.mIds[j]);
								if (anOther != nullptr && !anOther->mUnknown51D && anOther != aPiece &&
									aPiece->mUnknown538 == 0) {
									anOther->mPendingDX = (float)(((double)aPiece->mX - (double)anOther->mX) / 25.0);
									float aY = aPiece->mY;
									anOther->mUnknown110 = 0;
									anOther->mPendingKind = 0x10;
									anOther->mPendingDY = (float)(((double)aY - (double)anOther->mY) / 25.0);
									anOther->mPendingTime = 0.25f;
									aPiece->mUnknown4FC = 0;
									BreakPiece(anOther);
								}
							}
							break;
						}
					}
				}
			}
		}
	}
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr && aPiece->mUnknown10C != 0) {
				aPiece->mUnknown51D = false;
				BreakPiece(aPiece);
			}
		}
	}
	if (mUnknown149C.size() != 0) {
		// (the original reads every cell without a check for a gap)
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr && aPiece->mBreaking && aPiece->mUnknown110 == 0 &&
					!aPiece->IsFlagSet(Piece::FLAG_COAL))
					aPiece->mUnknown110 = 1;
			}
		}
	}
	if (0 < aMatchCount) {
		// A cascade
		mUnknown1508++;
		mUnknown14F4 = 0;
		AddStat(0x52, 1);
		if ((unsigned int)mApp->mUnknownC74[0x2c] <= (unsigned int)mUnknown1508)
			mApp->mUnknownC74[0x2c] = mUnknown1508;
		if (mUnknown37B1 && 2 < mUnknown1508) {
			mUnknown36C0.SetCurve(GTS(0x7dca30));
			mUnknown37A0 = 1.0f;
			mUnknown37A8 = 1.0f;
			mUnknown36B8 = mUnknown1508;
		}
		int aSixSound = -1;
		float aPitch = 0;
		for (unsigned int i = 0; i < aMatches.size(); i++) {
			if (5 < aMatches[i].mCount) {
				aPitch = (float)aMatches[i].mCount - 6.0f;
				aSixSound = SOUND_SIXMATCH;
			}
		}
		if (aSixSound == -1) {
			int aCascade = mUnknown1508;
			if (6 < aCascade)
				aCascade = 6;
			mApp->PlaySample(aCascade + SOUND_COMBO_1);
		} else {
			mApp->PlaySoundEx(aSixSound, 0, aPitch, 0.0f, true, true);
		}
		for (int i = 0; i < 24; i++)
			mUnknown2058[i] = 1.5f;
	}
	mUnknown2018 += aMatchCount;
	AddStat(0x29, aMatchCount);
	mUnknown20F4.clear();
	if (DropPieces())
		FillInBlanks(false);
	if (aMatches.size() != 0) {
		// The match of the fruit's next colour goes first
		unsigned int aNext = (unsigned int)mUnknown2660;
		if (aNext < mBonusColors.size()) {
			int aColor = mBonusColors[aNext];
			for (unsigned int i = 1; i < aMatches.size(); i++) {
				if (aColor == aMatches[i].mColor) {
					std::swap(aMatches[0], aMatches[i]);
					break;
				}
			}
		}
	}
	bool aFruitMatched = false;
	if (aMatches.size() != 0) {
		int aDelay = 0;
		for (unsigned int m = 0; m < aMatches.size(); m++) {
			MatchInfo aMatch = aMatches[m];
			mUnknown2614.push_back(aMatch);
			unsigned int aNext = (unsigned int)mUnknown2660;
			if (aNext < mBonusColors.size()) {
				if ((unsigned int)mBonusColors[aNext] == (unsigned int)aMatch.mColor) {
					if (mUnknown265C == 3)
						mApp->PlaySoundEx(SOUND_FRUITBONUSCOUNT, 0, (float)(unsigned int)mUnknown2660, 0.0f, true,
							true);
					mUnknown2660++;
					if (mUnknown2660 == 4 && mUnknown265C == 2)
						mUnknown265C = 1;
				}
				if ((unsigned int)mBonusColors[0] == (unsigned int)aMatch.mColor)
					aFruitMatched = true;
			}
			if (aMatch.mUnknown2C != 0) {
				// The match's points at its middle
				Piece* aGem = GetPieceById(aMatch.mIds[0]);
				int aSumX = 0;
				int aSumY = 0;
				for (int i = 0; i < (int)aMatch.mIds.size(); i++) {
					Piece* anOther = GetPieceById(aMatch.mIds[i]);
					if (anOther != nullptr) {
						aSumX = Ftol((double)(gGemWidth / 2) + (double)anOther->mX + (double)aSumX);
						aSumY = Ftol((double)(gGemHeight / 2) + (double)anOther->mY + (double)aSumY);
					}
				}
				if (aGem != nullptr) {
					int aCount = (int)aMatch.mIds.size();
					int aLift = Ftol((double)gApp->mScale * 24.0);
					AddPoints(aSumX / aCount, aSumY / aCount - aLift, aMatch.mUnknown2C, gGemColors[aGem->mColor],
						aMatch.mSerial, true, true, aFirst, true);
					// port: with the first points of a cascade and more than one object at +0x2170, the last one's
					// +0x50 and +0x70 change; the points' +0x6c takes aDelay. Both come with the points
					aFirst = false;
				}
			}
			aDelay += 15;
			(void)aDelay;
		}
	}
	if (mUnknown1508 == 1) {
		if (mUnknown265C == 0 && mUnknown2660 == mUnknown2664 && aFruitMatched) {
			mUnknown2664 = 0;
			mUnknown2660 = 1;
		} else {
			ResetFruitCount();
		}
	}
	if (1 < mUnknown2580)
		mApp->PlaySoundAt(SOUND_DOUBLESET, anAvgX, (float)(mUnknown2580 - 2), 0.0f);
	return FinishMatches(aMatches, aTipGems);
}

int Board::FinishMatches(std::vector<MatchInfo>& theMatches, std::set<int>& theTipGems)
{
	if (mUnknown1FF0 != 0) {
		mUnknown1FE8 = mUnknown1FE8 / mUnknown1FF0;
		mUnknown1FEC = mUnknown1FEC / mUnknown1FF0;
	}
	if (0.0f < mUnknown23EC) {
		// The tip's gems glow: those of every match with a gem the tip is about (every match, with none)
		for (unsigned int m = 0; m < theMatches.size(); m++) {
			bool aGlow = theTipGems.empty();
			MatchInfo& aMatch = theMatches[m];
			for (int i = 0; i < (int)aMatch.mIds.size(); i++) {
				if (theTipGems.find(aMatch.mIds[i]) != theTipGems.end())
					aGlow = true;
			}
			if (aGlow) {
				for (int i = 0; i < (int)aMatch.mIds.size(); i++)
					GetPieceById(aMatch.mIds[i])->mUnknown2F4 = true;
			}
		}
	}
	return (int)theMatches.size();
}

void Board::MakeMatchSpecial(MatchInfo& theMatch, std::vector<Piece*>& theReset, bool thePower, bool theElectro)
{
	// The gems that moved into the match, unless special
	std::vector<Piece*> aCandidates;
	int aSumX = 0;
	int aSumY = 0;
	for (unsigned int i = 0; i < theMatch.mIds.size(); i++) {
		Piece* aGem = GetPieceById(theMatch.mIds[i]);
		if (aGem == nullptr)
			continue;
		aSumX = Ftol((double)aSumX + (double)aGem->mX);
		aSumY = Ftol((double)aSumY + (double)aGem->mY);
		if (aGem->mUnknown10C != 0 && !aGem->IsFlagSet(Piece::FLAG_ELECTRO) && !aGem->IsFlagSet(Piece::FLAG_POWER) &&
			!aGem->IsFlagSet(Piece::FLAG_FRUIT) && (aGem->mUnknown2C != aGem->mRow || aGem->mUnknown24 != aGem->mCol))
			aCandidates.push_back(aGem);
	}
	aSumX = aSumX / theMatch.mCount;
	aSumY = aSumY / theMatch.mCount;
	if (aCandidates.empty()) {
		for (unsigned int i = 0; i < theMatch.mIds.size(); i++) {
			Piece* aGem = GetPieceById(theMatch.mIds[i]);
			if (aGem != nullptr && aGem->mFlags == 0)
				aCandidates.push_back(aGem);
		}
	}
	if (aCandidates.empty()) {
		Piece* aGem = GetPieceById(theMatch.mIds[theMatch.mCount / 2]);
		if (aGem != nullptr)
			aCandidates.push_back(aGem);
	}
	// The one nearest the match's middle
	Piece* aChosen = nullptr;
	int aBest = 100000;
	for (unsigned int i = 0; i < aCandidates.size(); i++) {
		Piece* aGem = aCandidates[i];
		if (!aGem->IsFlagSet(Piece::FLAG_BOMB) && !aGem->IsSpecial()) {
			float aDX = fabsf((float)aSumX - aGem->mX);
			float aDY = fabsf((float)aSumY - aGem->mY);
			int aDist = Ftol((double)aDY + (double)aDX);
			if (aDist < aBest) {
				aChosen = aGem;
				aBest = aDist;
			}
		}
	}
	if (aChosen == nullptr) {
		if (aCandidates.empty()) {
			unsigned int aCount = (unsigned int)theMatch.mIds.size();
			unsigned int aRoll = (unsigned int)Rand85B970();
			aChosen = GetPieceById(theMatch.mIds[aRoll % aCount]);
		} else {
			unsigned int aRoll = (unsigned int)Rand85B970();
			aChosen = aCandidates[aRoll % aCandidates.size()];
		}
	}
	Piece* aPiece = aChosen;
	if (!aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
		if (!aPiece->IsSpecial()) {
			if (!thePower) {
				if (!theElectro) {
					aPiece = nullptr;
				} else {
					MakeElectroGem(aPiece, 0.25f);
					aPiece->mPendingDY = 0;
					aPiece->mPendingDX = 0;
					aPiece->mUnknown51D = true;
					if (mUnknown1C68 + 2 < theMatch.mCount) {
						aPiece->mPendingKind += 0x10;
						AddStat(0x15, 1);
					}
					for (unsigned int i = 0; i < theMatch.mIds.size(); i++) {
						Piece* aGem = GetPieceById(theMatch.mIds[i]);
						if (aGem == nullptr || aGem->mUnknown51D)
							continue;
						BreakPiece(aGem);
						if (!aGem->IsSpecial()) {
							if (aGem == aPiece || aGem->mPendingKind != 0) {
								theReset.push_back(aGem);
							} else if (aGem->mUnknown538 == 0) {
								aGem->mPendingDX = (float)(((double)aPiece->mX - (double)aGem->mX) / 25.0);
								aGem->mPendingDY = (float)(((double)aPiece->mY - (double)aGem->mY) / 25.0);
								aGem->mPendingKind = 8;
								aGem->mPendingTime = 0.25f;
								aGem->mUnknown2EC = theMatch.mSerial;
								aGem->mUnknown110 = 0;
								aGem->mUnknown4FC = 0;
							}
						} else {
							aGem->mUnknown536 = false;
						}
					}
				}
			} else {
				MakePowerGem(aPiece, 0.25f);
				aPiece->mPendingDY = 0;
				aPiece->mPendingDX = 0;
				aPiece->mUnknown51D = true;
				for (unsigned int i = 0; i < theMatch.mIds.size(); i++) {
					Piece* aGem = GetPieceById(theMatch.mIds[i]);
					if (aGem == nullptr || aGem->mUnknown51D)
						continue;
					BreakPiece(aGem);
					if (!aGem->IsSpecial()) {
						if (aGem == aPiece || aGem->mPendingKind != 0) {
							theReset.push_back(aGem);
						} else if (aGem->mUnknown538 == 0) {
							aGem->mPendingDX = (float)(((double)aPiece->mX - (double)aGem->mX) / 25.0);
							aGem->mPendingDY = (float)(((double)aPiece->mY - (double)aGem->mY) / 25.0);
							aGem->mPendingKind = 0x10;
							aGem->mPendingTime = 0.25f;
							aGem->mUnknown536 = false;
							aGem->mUnknown2EC = theMatch.mSerial;
							aGem->mUnknown110 = 0;
							aGem->mUnknown4FC = 0;
						}
					} else {
						aGem->mUnknown536 = false;
					}
				}
			}
		} else {
			aPiece->mUnknown538 = theElectro ? 8 : 16;
		}
	} else {
		BreakPiece(aPiece);
	}
	if (aPiece != nullptr) {
		theReset.push_back(aChosen);
		aPiece->mUnknownEC = false;
		aPiece->mBombCount = 0;
		aPiece->ClearFlag(Piece::FLAG_BOMB);
	}
}

void Board::PlayChainSound(int theChain)
{
	int aNote = Ftol((double)(theChain % 12) + 6.0);
	float aLow = (float)((double)(float)(theChain % 12) / 12.0 + 0.1);
	float aHigh = (float)((1.0 - (double)aLow) + 0.1);
	if (aLow < 0.145f)
		aLow = 0.145f;
	if (aHigh < 0.145f)
		aHigh = 0.145f;
	if (1.0f < aLow)
		aLow = 1.0f;
	if (1.0f < aHigh)
		aHigh = 1.0f;
	float aVolume = (float)(0.8 - (double)(float)theChain * 0.02);
	if (aVolume < 0.5f)
		aVolume = 0.5f;
	mApp->PlaySoundEx(SOUND_CHAINX1, 0, (float)(aNote - 12), aVolume * aLow, true, true);
	mApp->PlaySoundEx(SOUND_CHAINX1, 0, (float)aNote, aVolume * aHigh, true, true);
}

int Board::GetLevelSteps(int theLevel)
{
	// The steps of each multiplier level (0x84f074 in the exe)
	static const int kSteps[11] = {0, 4, 8, 12, 16, 16, 16, 16, 16, 16, 16};
	if (9 < theLevel)
		theLevel = 10;
	return kSteps[theLevel];
}

int Board::GetLevelFor(int* theLeft)
{
	int aLevel = 1;
	int aStart = 0;
	while (true) {
		int anEnd = aStart + GetLevelSteps(aLevel);
		if (mUnknown224 < anEnd)
			break;
		aLevel++;
		aStart = anEnd;
		if (aLevel == 10) {
			if (theLeft != nullptr)
				*theLeft = mUnknown224 - anEnd;
			return 10;
		}
	}
	if (theLeft != nullptr)
		*theLeft = mUnknown224 - aStart;
	if (aLevel < 11)
		return aLevel;
	return 10;
}

void Board::SetLevelSteps(int theSteps)
{
	int aLeft = 0;
	int aShown = mGoodTube->mUnknown27C;
	mUnknown224 = theSteps;
	int aLevel = GetLevelFor(&aLeft);
	mUnknown21D0 = aLevel;
	if (aLevel == 10 && GetLevelSteps(10) * 2 <= aLeft) {
		if (!mUnknown214)
			mUnknown344C++;
		mUnknown214 = true;
	}
	if (mUnknown2041) {
		mGoodTube->SetLevelSteps(aLeft);
		mGoodTube->SetLevelShown(aLevel);
		if (aLevel < aShown) {
			mGoodTube->MarkLevelChange();
			mGoodTube->mUnknown29C = mGoodTube->mUnknown29C * 0.5f;
		} else if (aShown < aLevel && aLeft == 0) {
			mGoodTube->MarkLevelChange();
			if (mApp->mUnknownC74[0x36] == 0 && aLevel == 2)
				mApp->mUnknownC74[0x36] = mUnknown158;
			else if (mApp->mUnknownC74[0x37] == 0 && aLevel == 2)
				mApp->mUnknownC74[0x37] = mUnknown158;
		}
	}
}

void Board::AddChainStep()
{
	bool aCounted;
	if (!mUnknown2041 || !mGoodTube->mUnknown14D) {
		aCounted = false;
	} else {
		mUnknown224++;
		aCounted = true;
	}
	// The step that starts a level counts twice
	int aLeft = 0;
	GetLevelFor(&aLeft);
	if (aLeft == 0 && aCounted)
		mUnknown224++;
	AddStat(0x2a, 1);
	if (mGameMode != MODE_CHALLENGE)
		SetLevelSteps(mUnknown224);
}

void Board::ShowTubeLevel()
{
	if (mGoodTube != nullptr && mUnknown2041)
		mGoodTube->SetLevelShown(mUnknown21D0);
}

void Board::MatchAfterMove()
{
	int aMatches;
	if (mGameMode == MODE_PUZZLE) {
		aMatches = 0;
	} else {
		unsigned int aFruitCount = (unsigned int)mUnknown2660;
		mUnknown2614.clear();
		int aCleared = mUnknown1F0;
		if (HasChallengeFlag(0x10)) {
			// port: 0x6b3432 comes with the challenges
		}
		aMatches = FindMatches(2);
		if (aCleared < mUnknown1F0)
			ShowCountPopup();
		if (mGameMode == MODE_CHALLENGE) {
			if (mUnknown2630 != 0)
				NotPorted("Board::MatchAfterMove: the challenge's goal (0x6b3432)");
		} else {
			// port: 0x6b3432's result in the other modes comes with the challenges; it is false there
			if (mUnknown1508 < 1 && (unsigned int)mUnknown2660 == aFruitCount) {
				if (aFruitCount < mBonusColors.size() && aFruitCount == (unsigned int)mUnknown2664)
					mUnknown2660 = 0;
			}
		}
		if (mUnknown2130 && mUnknown2041) {
			mUnknown2130 = false;
			if (aMatches == 0) {
				if (!IsSparkBlocked()) {
					// The chain is over: the steps fall back to the level's start, and a chain that added none drops a level
					int aStat = mUnknown224 % 10;
					if (4 < aStat)
						AddStat(mUnknown224 == 10 ? 0x13 : aStat + 9, 1);
					int aSteps = mUnknown224;
					int aLevel = 1;
					int aStart = 0;
					mUnknown344C = 0;
					if (1 < mUnknown21D0) {
						do {
							aStart += GetLevelSteps(aLevel);
							aLevel++;
						} while (aLevel < mUnknown21D0);
					}
					if (aStart == aSteps) {
						int aDown = mUnknown21D0 - 1;
						int i = 1;
						if (aDown < 1)
							aDown = i;
						aStart = 0;
						mUnknown21D0 = aDown;
						if (1 < aDown) {
							do {
								aStart += GetLevelSteps(i);
								i++;
							} while (i < mUnknown21D0);
						}
					}
					mUnknown21F8 = 100;  // 0x6a3ff4
					SetLevelSteps(aStart);
					mUnknown818 = 0;
					SetStat(0x2a, aStart);
					mUnknown2234[0x2a] = 0;  // (the app's copy keeps the steps)
					mUnknown2201 = false;
					mUnknown21A1 = true;
				}
			} else {
				mUnknown818++;
				int aChain = mUnknown818;
				mUnknown2228 = 0.5f;
				int aBest = mUnknown228[0];
				if (aBest <= aChain)
					aBest = aChain;
				mUnknown228[0] = aBest;
				if (mGameMode == MODE_CHALLENGE) {
					AddStat(0x2a, 1);
				} else if (1 < aChain && 49 < aChain && aChain % 10 == 0) {
					// port: "%d CHAIN!" (0x7dd... read by GetString) on the tube (0x70c94a) comes with the tube's texts
					NotPorted("Board::MatchAfterMove: the chain's text");
				}
			}
			unsigned int& aBestChain = (unsigned int&)mApp->mUnknownC74[0x2b];
			if (aBestChain <= (unsigned int)mUnknown818)
				aBestChain = (unsigned int)mUnknown818;
			int aChain = mUnknown818;
			int aBest = mUnknown2234[0x2b];
			if (aBest <= aChain)
				aBest = aChain;
			mUnknown2234[0x2b] = aBest;
			if (0 < mUnknown158) {
				int& aProfileBest = mApp->mProfile->mBestChain;
				if (aChain <= aProfileBest)
					aChain = aProfileBest;
				aProfileBest = aChain;
			}
		}
	}
	if (mUnknown1FC0 != mUnknown1FB8) {
		if (aMatches == 0) {
			mUnknown215C = -1;
			mUnknown2160 = 1;
			mUnknown25B4 = 0;
			mUnknown25AC = mUnknown25A8;
		} else {
			mUnknown244++;
			mUnknown2164 = 1.0f;
			mUnknown216C = true;
			if (mUnknown1508 == 1)
				PlayChainSound(mUnknown818 + 1);
			if (mGameMode != MODE_PUZZLE && mUnknown2041 && mGoodTube->mUnknown14D && !mUnknown1390 && !mUnknown21A2)
				mUnknown1FC0 = mUnknown1FB8;
			mUnknown25B4++;
			if (!mUnknown2629 && mGameMode != MODE_CHALLENGE) {
				SpeedBonus();
			}
		}
	}
	if (mUnknown15C < 1)
		mUnknown21DA = true;
	if (mUnknown1438) {
		mUnknown1438 = false;
		mApp->KillDialog(40);
		mApp->mUnknown8D8 = false;
	}
	if (mUnknown1FBC != mUnknown1FB8) {
		AddStat(0x39, mUnknown21D0);
		int aCascade = mUnknown1508;
		if (4 < aCascade)
			AddStat(aCascade < 10 ? aCascade + 3 : 0xd, 1);
		mUnknown1FBC = mUnknown1FB8;
	}
}

void Board::Refill()
{
	// Not while a gem is still to go off, or a bomb at 1 is still counting
	int aRow = mRows - 1;
	bool aDone = true;
	if (-1 < aRow) {
		do {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr &&
					(0 < aPiece->mUnknown88 || (aPiece->mBombCount == 1 && aPiece->mUnknownD0 != 0.0f)))
					aDone = false;
			}
			aRow--;
		} while (-1 < aRow);
		if (!aDone)
			return;
	}
	bool aDropped = DropPieces();
	int aNew = 0;
	if (aDropped)
		aNew = FillInBlanks(false);
	MarkDirty();
	mUnknown21DA = false;
	EvaluateCursor(nullptr);
	if (!aDropped && aNew == 0 && mUnknown149C.size() == 0 && mUnknown25A4)
		SettleTurn();
	else if (mUnknown15C < 1)
		mUnknown15C = 0;
	CheckCombos();
}

void Board::BreakPiece(Piece* thePiece)
{
	thePiece->mUnknown10C = 0;
	mUnknown2228 = 0.5f <= mUnknown2228 ? mUnknown2228 : 0.5f;
	if (!mUnknown218C && thePiece->mColor < 7)
		mUnknown218C = true;
	if (thePiece->mUnknown2F5) {
		thePiece->mUnknown2F5 = false;
		return;
	}
	if (0 < mUnknown1508) {
		// The cell glows in the gem's colour, brighter down a cascade
		int aColor = thePiece->mColor;
		int anAlpha = mUnknown1508 * 0x55;
		// (the original's table has black and two whites after the seven colours)
		Color aTint = aColor < 7 ? gGemColors[aColor] : (aColor == 7 ? Color(0, 0, 0) : Color(255, 255, 255));
		aTint.mAlpha = anAlpha < 0x100 ? anAlpha : 0xff;
		mCells[mCols * thePiece->mRow + thePiece->mCol].mColor = aTint;
	}
	if (thePiece->mBreaking)
		return;
	thePiece->mBreaking = true;
	ShowCountPopup();
	if (thePiece->IsFlagSet(Piece::FLAG_BOMB)) {
		// A bomb in a match goes off harmlessly
		if (!thePiece->mUnknown116)
			AddStat(0x47, 1);
		BreakSpecial(thePiece, false);
		thePiece->mBombCount = 0;
		thePiece->ClearFlag(Piece::FLAG_BOMB);
	}
	if (thePiece->mUnknown51D) {
		thePiece->mBreaking = false;
		return;
	}
	int aColor = thePiece->mColor;
	if (-1 < aColor && aColor < 7)
		AddStat(aColor + 0x4b, 1);
	if (mUnknown15C < 1 && !IsBusy()) {
		mUnknown21DA = true;
		mUnknown15C = 0;
	}
	if (thePiece->IsFlagSet(Piece::FLAG_1)) {
		AddStat(0x46, 1);
		if (thePiece->mPendingKind == 0)
			thePiece->mScale288 = 0.0f;
		if (thePiece->mBreaking)
			thePiece->mScale288 = 0.75f;
	} else if (thePiece->IsFlagSet(Piece::FLAG_6) && !thePiece->mUnknown51D) {
		// port: a hypercube's blast (stat 0x42, 50 sparks each of two kinds, 0x6baa5a, 75 points) comes with the
		// special gems; it draws from Rand
		NotPorted("Board::BreakPiece: a hypercube");
	} else if (thePiece->IsFlagSet(Piece::FLAG_COAL) && !thePiece->mUnknown51D) {
		// port: coal breaking (stat 0x4a, 0x6c03a4 with 1) comes with the coal
		NotPorted("Board::BreakPiece: coal");
	} else if (thePiece->IsFlagSet(Piece::FLAG_ELECTRO) && !thePiece->mUnknown51D) {
		// port: an electro gem going off (its strike at +0x149c, 150 points, SOUND_WILDCARD_DESTROYED and
		// SOUND_FLAMELIGHTNING) comes with the electro gems
		NotPorted("Board::BreakPiece: an electro gem");
		return;
	} else if (thePiece->IsFlagSet(Piece::FLAG_POWER)) {
		if (!thePiece->mUnknown537) {
			AddStat(0x40, 1);
			AddPoints(Ftol((double)(gGemWidth / 2) + (double)thePiece->mX),
				Ftol((double)(gGemHeight / 2) + (double)thePiece->mY), 50, gGemColors[thePiece->mColor],
				thePiece->mUnknown2EC, true, true, false, true);
			thePiece->mUnknown537 = true;
			mUnknown1FC++;
			mUnknown1F8++;
		}
		mUnknown37B8[thePiece->mUnknown2EC] = -0x66;  // 0x6b8c49
		thePiece->Blast();
	} else if (thePiece->IsFlagSet(Piece::FLAG_LOCKED)) {
		// port: a gem lock breaking (0x72952f) comes with the gem locks
		NotPorted("Board::BreakPiece: a gem lock");
	} else if (thePiece->IsFlagSet(Piece::FLAG_FRUIT)) {
		// port: the fruit gem (its sound, the dance's curves and +0x265c) comes with the fruit gem
		NotPorted("Board::BreakPiece: a fruit gem");
		thePiece->mBreaking = false;
		return;
	} else if (thePiece->IsFlagSet(Piece::FLAG_DEVIL)) {
		// port: a doom gem breaking (stat 0x44, 0x6c03a4) comes with the doom gems
		NotPorted("Board::BreakPiece: a doom gem");
		thePiece->mBlasted = true;
	}
	if (!thePiece->mUnknown116)
		mUnknown1E8++;
	else
		thePiece->mBlasted = true;
}

void Board::BlowUpPiece(Piece* thePiece, float theX, float theY, bool theBreak, bool theBlast)
{
	// 0x6c1857: the gem bursts into smoke, shards and flashes, flung away from theX, theY; theBreak also breaks it
	// with a small wave, theBlast flings the smoke from the blast's middle (a power gem's)
	const double kPi = 3.1415927410125732;
	const float kTwoPiF = 6.2831855f;  // 0x84e91c
	int aCenterX = Ftol((double)(gGemWidth / 2) + (double)thePiece->mX);
	int aCenterY = Ftol((double)(gGemHeight / 2) + (double)thePiece->mY);
	float aX = (float)aCenterX;
	float aPushX = (float)(((double)aX - (double)theX) * 0.009999999776482582);
	float aY = (float)aCenterY;
	float aPushY = (float)(((double)aY - (double)theY) * 0.009999999776482582);
	if (aPushX == 0.0f)
		aPushY = (float)((double)aPushY * 2.0);
	if (aPushY == 0.0f)
		aPushX = (float)((double)aPushX * 2.0);
	mUnknown1370[GetColAt(Ftol((double)thePiece->mX))] = mUpdateCnt;
	Light aLight;
	aLight.mX = aX;
	aLight.mY = aY;
	aLight.mUnknown8 = 15.0f;
	aLight.mUnknownC = 20.0f;
	aLight.mUnknown10 = 0;
	aLight.mUnknown14 = 1.0f;
	aLight.mUnknown18 = true;
	aLight.mUnknown1C = 0.1f;
	aLight.mUnknown20 = 0.05f;
	mUnknown1650.push_back(aLight);

	// smoke
	for (int aLeft = 14; aLeft != 0; aLeft--) {
		Effect* anEffect = mFXManager->AllocEffect(26);
		float anAngle = (float)((double)RandSigned() * kPi);
		float aMin = (float)((double)gApp->mScale * 0.0);
		float aRange = gApp->mScale + gApp->mScale;
		float aRand = RandSigned();
		float aSpeed = (float)((double)fabsf(aRand) * (double)aRange + (double)aMin);
		anEffect->mDX = (float)((double)cosf(anAngle) * (double)aSpeed + (double)aPushX);
		anEffect->mDY = (float)((double)sinf(anAngle) * (double)aSpeed + (double)aPushY);
		if (theBlast) {
			float aScale = (float)((double)gApp->mScale * 2.5);
			aRand = fabsf(RandSigned());
			anEffect->mDX = (float)((double)aRand * (double)aScale * (double)anEffect->mDX);
			aScale = (float)((double)gApp->mScale * 2.5);
			aRand = fabsf(RandSigned());
			anEffect->mDY = (float)((double)aRand * (double)aScale * (double)anEffect->mDY);
			// from somewhere between the blast's middle and the gem
			float aMix = fabsf(RandSigned());
			aMix = aMix * aMix;
			float aCos = cosf(anAngle);
			aRand = RandSigned();
			double aRest = 1.0 - (double)aMix;
			anEffect->mX = (float)(((double)aMix * (double)theX + (double)aX * aRest) +
				(double)aCos * 64.0 * (double)fabsf(aRand));
			float aSin = sinf(anAngle);
			aRand = RandSigned();
			anEffect->mY = (float)(((double)aMix * (double)theY + (double)aY * aRest) +
				(double)aSin * 64.0 * (double)fabsf(aRand));
			anEffect->mColor = Color(255, 96, 32, 64);
			anEffect->mIsAdditive = true;
			anEffect->mDScale = 0.015f;
		} else {
			float aCos = cosf(anAngle);
			aRand = RandSigned();
			anEffect->mX = (float)((double)aCos * 24.0 * (double)fabsf(aRand) + (double)aX);
			float aSin = sinf(anAngle);
			aRand = RandSigned();
			anEffect->mY = (float)((double)aSin * 24.0 * (double)fabsf(aRand) + (double)aY);
			anEffect->mColor = gGemColors[thePiece->mColor];
			anEffect->mIsAdditive = false;
			anEffect->mDScale = 0.02f;
		}
		anEffect->mScale = (float)((double)fabsf(RandSigned()) + 0.10000000149011612);
		mFXManager->AddEffect(anEffect);
	}

	// shards, in a spiral (or thrown out from the blast)
	for (int i = 0; i < 25; i++) {
		Effect* anEffect = mFXManager->AllocEffect(16);
		anEffect->mColor = gGemColors[thePiece->mColor];
		int aTurns = Ftol((double)(i + 120) / 120.0);
		int aDist = Ftol((double)(aTurns * i) * (double)gApp->mScale);
		int aJitter = (int)Rand() % 100;
		double anAngle = (double)aJitter / 800.0 + (double)i * 0.503;
		float aBase = (float)((double)gApp->mScale * 1.2000000476837158);
		float aRand = fabsf(RandSigned());
		float aSpeed = (float)((double)gApp->mScale * 1.2000000476837158);
		aSpeed = (float)((double)aSpeed * (double)aRand + (double)aBase);
		anEffect->mDX = (float)(cos(anAngle) * (double)aSpeed + (double)aPushX);
		float aLift = (float)((double)gApp->mScale * -2.0);
		float aDY = (float)(sin(anAngle) * (double)aSpeed + (double)aLift + (double)aPushY);
		anEffect->mDY = aDY;
		if (aPushX == 0.0f && aPushY == 0.0f) {
			double aBack = (double)aDist * -1.2000000476837158;
			int anOffset = Ftol((double)anEffect->mDX * aBack);
			anEffect->mX = (float)((double)(aCenterX - anOffset) + 14.0);
			anOffset = Ftol(aBack * (double)aDY);
			anEffect->mY = (float)((double)(aCenterY - anOffset) + 10.0);
		} else {
			anAngle = (double)RandSigned() * 3.14159;
			int aMaxDist = Ftol((double)gApp->mScale * 48.0);
			int aRandDist = Ftol((double)RandSigned() * (double)aMaxDist);
			float aDistF = (float)aRandDist;
			anEffect->mX = (float)(Ftol(cos(anAngle) * (double)aDistF) + aCenterX);
			anEffect->mY = (float)(Ftol(sin(anAngle) * (double)aDistF) + aCenterY);
			float anAwayX = anEffect->mX - theX;
			float anAwayY = anEffect->mY - theY;
			double aDir = (double)(float)std::atan2((double)anAwayY, (double)anAwayX);  // 0x4c1b70
			anAngle = (double)RandSigned() * 0.3 + aDir;
			float aBase2 = (float)((double)gApp->mScale * 3.5);
			aRand = fabsf(RandSigned());
			float aSpeed2 = (float)((double)gApp->mScale * (double)aRand + (double)aBase2);
			anEffect->mDX = (float)(cos(anAngle) * (double)aSpeed2);
			float aLift2 = (float)((double)gApp->mScale * -2.0);
			anEffect->mDY = (float)(sin(anAngle) * (double)aSpeed2 + (double)aLift2);
			anEffect->mUnknown38 = (float)((double)RandSigned() * 0.005 + 0.98);
		}
		anEffect->mAngle = (float)anAngle;
		anEffect->mDAngle = (float)((double)RandSigned() * 0.05000000074505806);
		anEffect->mGravity = (float)((double)gApp->mScale * 0.05999999865889549);
		anEffect->mUnknown9C = (float)((double)RandSigned() * (double)kTwoPiF);
		anEffect->mUnknownA0 = (float)((double)RandSigned() * (double)kTwoPiF);
		anEffect->mUnknownA4 = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
		anEffect->mUnknownA8 = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
		float aFade = fabsf(RandSigned());
		anEffect->mDAlpha = (float)(((double)aFade + (double)aFade + 4.0) * -0.0025);
		mFXManager->AddEffect(anEffect);
	}

	// a ring of grey smoke
	for (int i = 0; i < 14; i++) {
		Effect* anEffect = gBoard->mFXManager->AllocEffect(26);
		float anAngle = (float)((double)i * (double)kTwoPiF / 20.0);
		float aRand = RandSigned();
		float aSpeed = (float)((double)fabsf(aRand) * 5.75 + 0.5);
		anEffect->mDX = (float)((double)cosf(anAngle) * (double)aSpeed + (double)aPushX);
		anEffect->mDY = (float)((double)sinf(anAngle) * (double)aSpeed + (double)aPushY);
		float aCos = cosf(anAngle);
		aRand = RandSigned();
		anEffect->mX = (float)((double)aCos * 25.0 * (double)fabsf(aRand) + (double)aX);
		float aSin = sinf(anAngle);
		aRand = RandSigned();
		anEffect->mIsAdditive = false;
		anEffect->mY = (float)((double)aSin * 25.0 * (double)fabsf(aRand) + (double)aY);
		anEffect->mScale = 0.5f;
		anEffect->mDScale = 0.005f;
		aRand = (float)((double)RandSigned() * 0.5);
		aRand = fabsf(aRand);
		anEffect->mUnknownA0 = (float)((1.0 - (double)aRand) * (double)anEffect->mUnknownA0);
		anEffect->mColor = Color(128, 128, 128);
		gBoard->mFXManager->AddEffect(anEffect);
	}

	if (0.0f < thePiece->mGlow) {
		// a glowing gem throws sparks too
		EffectsManager* aFXManager = gBoard->mFXManager;
		for (int aLeft = 15; aLeft != 0; aLeft--) {
			Effect* anEffect = aFXManager->AllocEffect(12);
			float aMin = gApp->mScale;
			float aRand = fabsf(RandSigned());
			float aRange = gApp->mScale + gApp->mScale;
			float aSpeed = (float)((double)aRange * (double)aRand + (double)aMin);
			float anAngle = (float)((double)RandSigned() * kPi);
			anEffect->mScale = (float)((double)RandSigned() * 0.5 + 1.0);
			anEffect->mDX = (float)((double)cosf(anAngle) * (double)aSpeed);
			anEffect->mDY = (float)((double)sinf(anAngle) * (double)aSpeed);
			float aCos = cosf(anAngle);
			anEffect->mX = (float)((double)gGemWidth * (double)aCos * 0.5 +
				((double)(gGemWidth / 2) + (double)thePiece->mX));
			float aSin = sinf(anAngle);
			anEffect->mIsAdditive = true;
			anEffect->mY = (float)((double)gGemHeight * (double)aSin * 0.5 +
				((double)(gGemHeight / 2) + (double)thePiece->mY));
			anEffect->mAlpha = 1.0f;
			anEffect->mDAlpha = -0.005f;
			anEffect->mGravity = 0;
			aFXManager->AddEffect(anEffect);
		}
		// (unless the board is +0x37c4's, the original adds a heat wave at the gem, 0x6f9efe: Direct3D only)
	}

	if (mUnknown1430 == thePiece)
		mUnknown1430 = nullptr;
	if (theBreak) {
		MatchShockwave* aWave = new MatchShockwave(aCenterX, aCenterY);
		aWave->SetStrength(2.0f);
		mUnknown1660.push_back(aWave);
		BreakPiece(thePiece);
		if (!thePiece->mUnknown537 && !IsBusy()) {
			AddStat(0x3e, 1);
			thePiece->mUnknown537 = true;
			if (!mUnknown2629 && 0 < mUnknown158)
				mApp->mProfile->mGemsCleared++;
			mUnknown1FC++;
			mUnknown1F8++;
			ShowCountPopup();
		}
		if (thePiece->IsFlagSet(Piece::FLAG_COAL))
			thePiece->mBreaking = true;
		else if (thePiece->mUnknown88 == 0)
			thePiece->mBlasted = true;
	}
}

void Board::LiftColumn(Piece* thePiece, float theStrength)
{
	// 0x6a4a1b: each gem above jumps (its fall's speed goes up), less the further above it is; the column's +0x133c
	// keeps the strongest
	static float sReach = (float)(gGemHeight + gGemHeight);  // 0x85c418, set on the first call
	float aFirst = 0;
	for (int aRow = mRows - 1; 0 <= aRow; aRow--) {
		int aCol = thePiece->mCol;
		float aKick = 0;
		do {
			Piece* aPiece = gBoard->GetPieceAt(aRow, aCol);
			if (aPiece != nullptr && aPiece->mY < thePiece->mY) {
				float aFar = (float)((double)fabsf(aPiece->mY - thePiece->mY) / (double)sReach);
				if (1.0f < aFar)
					aFar = 1.0f;
				float aScale = (float)((double)gApp->mScale * -5.0);
				float aNear = 1.0f - aFar;
				aKick = (float)((double)aScale * (double)theStrength * (double)aNear);
				if (aFirst == 0.0f)
					aFirst = aKick;
				aPiece->mUnknown34 = aFirst <= aPiece->mUnknown34 ? aFirst : aPiece->mUnknown34;
			}
			mUnknown133C[aCol] = aKick < aFirst ? aFirst : aKick;
			aCol++;
		} while (aCol <= thePiece->mCol);
	}
}

void Board::LiftAround(Piece* thePiece, float theStrength)
{
	// 0x6a4b7c: in every column the gems above thePiece jump, harder the nearer and the more nearly straight above
	// they are; the column's +0x133c keeps the strongest
	for (int aCol = 0; aCol < mCols; aCol++) {
		float aFirst = 0;
		float aKick = 0;
		for (int aRow = mRows - 1; -1 <= aRow; aRow--) {
			Piece* aPiece = gBoard->GetPieceAt(aRow, aCol);
			float aDX;
			float aDY;
			if (aPiece != nullptr && aPiece->mY < thePiece->mY) {
				aDX = aPiece->mX - thePiece->mX;
				aDY = aPiece->mY - thePiece->mY;
				if (aFirst == 0.0f)
					aFirst = aKick;
				aKick = aFirst <= aPiece->mUnknown34 ? aFirst : aPiece->mUnknown34;
				aPiece->mUnknown34 = aKick;
			} else if (aRow == -1) {
				// and above the top row
				aDX = (float)GetColX(aCol) - thePiece->mX;
				aDY = (float)GetRowY(-1) - thePiece->mY;
			} else {
				continue;
			}
			float anAngle = (float)std::atan2((double)aDY, (double)aDX);  // 0x4c1b70
			float aDist = sqrtf(aDX * aDX + aDY * aDY);
			aDist = (float)((double)aDist / (double)gGemWidth);
			double aGems = (double)aDist - 1.0;
			if (aGems < 0.0)
				aGems = 0.0;
			float anUp = fabsf(sinf(anAngle));
			float aPush = (float)((double)theStrength / (aGems + 1.0) * (double)anUp);
			float aScale = (float)((double)gApp->mScale * -7.0);
			aKick = (float)((double)aScale * (double)aPush);
		}
		aFirst = aKick <= aFirst ? aKick : aFirst;
		mUnknown133C[aCol] = aFirst;
	}
}

void Board::SetOffPowerGem(Piece* thePiece)
{
	// 0x6c6d68: a gem whose countdown has run out goes off: a power gem blasts, anything else (or a power gem while
	// the board is busy) bursts the gems round it
	if (thePiece->IsFlagSet(Piece::FLAG_POWER) && !thePiece->mBlasted && !IsBusy()) {
		thePiece->Blast();
	} else {
		// (the original asks IsBusy and the electro flag again here and keeps neither answer)
		BurstAt(GetColX(thePiece->mUnknown24) + gGemWidth / 2, GetRowY(thePiece->mUnknown30) + gGemHeight / 2, 1);
	}
	if (!thePiece->mUnknown537) {
		AddStat(0x3e, 1);
		if (!mUnknown2629 && 0 < mUnknown158)
			mApp->mProfile->mGemsCleared++;
		mUnknown1FC++;
		mUnknown1F8++;
		thePiece->mUnknown537 = true;
		ShowCountPopup();
	}
	if (DropPieces())
		FillInBlanks(false);
}

// The burst's centres (0x85acf0) and the gems it reaches (0x85a9e8 colours, 0x85a640 centres, 0x85aaf0 the centre that
// reached them); the counts are 0x85aae8 and 0x85aefc
static int sBurstCount = 0;
static int sBurstCentres[64][2];
static int sBurstHitCount = 0;
static int sBurstHitColors[64];
static int sBurstHitCentres[64][2];
static int sBurstHitFrom[64][2];

void Board::BurstGems(int theX, int theY, int theRange)
{
	// 0x6bac72: lights the cell at (theX, theY), then looks for the gems round it. It looks them up with GetPieceAt,
	// which takes a row and a column, at pixel positions, so it never finds one, and the original's burst reaches no
	// other gem (the power gems' chain bonus in it never runs either); the port keeps the lookup as it is.
	mUnknown1370[GetColAt(theX)] = mUpdateCnt;
	int aCentreX = theX;
	int aCentreY = theY;
	Piece* aPiece = GetPieceAtXY(theX, theY);
	if (aPiece != nullptr) {
		aCentreX = Ftol((double)(gGemWidth / 2) + (double)aPiece->mX);
		aCentreY = Ftol((double)(gGemHeight / 2) + (double)aPiece->mY);
	}
	Light aLight;
	aLight.mX = (float)theX;
	aLight.mY = (float)theY;
	aLight.mUnknown8 = 2.0f;
	aLight.mUnknownC = 1.0f;
	aLight.mUnknown10 = 0;
	aLight.mUnknown14 = 1.0f;
	aLight.mUnknown18 = true;
	aLight.mUnknown1C = 0.1f;
	aLight.mUnknown20 = 0.05f;
	mUnknown1650.push_back(aLight);
	sBurstCentres[sBurstCount][0] = aCentreX;
	sBurstCentres[sBurstCount][1] = aCentreY;
	sBurstCount++;
	for (int aDY = -theRange; aDY <= theRange; aDY++) {
		for (int aDX = -theRange; aDX <= theRange; aDX++) {
			Piece* aGem = GetPieceAt(gGemWidth * aDX + theX, gGemHeight * aDY + theY);
			if (aGem == nullptr || aGem->mUnknown88 != 0)
				continue;
			if (aDX != 0 || aDY != 0) {
				int aHit = sBurstHitCount;
				sBurstHitColors[aHit] = aGem->mColor;
				sBurstHitCentres[aHit][0] = Ftol((double)aGem->mX) + gGemWidth / 2;
				sBurstHitCentres[aHit][1] = Ftol((double)aGem->mY) + gGemHeight / 2;
				sBurstHitFrom[aHit][0] = aCentreX;
				sBurstHitFrom[aHit][1] = aCentreY;
				sBurstHitCount = aHit + 1;
				if (aGem->IsFlagSet(Piece::FLAG_POWER)) {
					// port: the stats "ExplosionChainBonus" (10) and "ExplosionChainBonusAdd" (40), out of reach
					NotPorted("Board::BurstGems: a power gem's chain bonus");
					mUnknown1528++;
					aGem->mUnknown88 = 10;
					continue;
				}
			}
			aGem->mBlasted = true;
		}
	}
}

void Board::BurstAt(int theX, int theY, int theRange)
{
	// 0x6baed1: the burst, and a ring of sparkles (effect 17) round each centre it lit, fewer the more gems it reached
	mUnknown152C = true;
	sBurstCount = 0;
	sBurstHitCount = 0;
	BurstGems(theX, theY, theRange);
	static const float sDensity[16] = {1.0f, 0.98f, 0.95f, 0.93f, 0.9f, 0.88f, 0.84f, 0.8f, 0.75f, 0.7f, 0.65f, 0.6f,
		0.55f, 0.5f, 0.4f, 0.35f};  // 0x7dcdf8
	int aHits = sBurstHitCount;
	if (15 <= aHits)
		aHits = 15;
	float aDensity = sDensity[aHits];
	float aDue = 0;
	for (int aCentre = 0; aCentre < sBurstCount; aCentre++) {
		for (int i = 0; i < 18; i++) {
			aDue = aDue + aDensity;
			if (!(1.0f <= aDue))
				continue;
			aDue = (float)((double)aDue - 1.0);
			int anExtra = Ftol((double)i * 0.2);
			int aSpeed = Ftol((double)gApp->mScale * 5.4 + (double)anExtra);
			int aRand = Rand() % 100;
			float anAngle = (float)((double)aRand / 400.0 + (double)i * 0.906000018119812);
			float aDist = (float)aSpeed;
			int aX = Ftol((double)(aDist * cosf(anAngle))) + sBurstCentres[aCentre][0];
			int aY = Ftol((double)(aDist * sinf(anAngle))) + sBurstCentres[aCentre][1];
			// (the original works out the angle from the gem it reached, atan2 at 0x77ea70, and keeps nothing of it)
			Effect* anEffect = mFXManager->AllocEffect(17);
			anEffect->mX = (float)aX;
			anEffect->mY = (float)aY;
			anEffect->mDX = (float)((double)cosf(anAngle) * (double)aDist * 0.25099998712539673);
			anEffect->mDY = (float)((double)sinf(anAngle) * (double)aDist * 0.25099998712539673);
			mFXManager->AddEffect(anEffect);
		}
	}
	if (0 < mUnknown1528) {
		float aCount = (float)mUnknown1528;
		float aShake = (float)((double)aCount * 0.2 + 1.2);
		if (2.0f <= aShake)
			aShake = 2.0f;
		mUnknown151C = aShake;
		float aSize = (float)((double)aCount * 1.2 + 1.25);
		if (6.0f <= aSize)
			aSize = 6.0f;
		mUnknown1518 = aSize;
	}
}

void Board::BreakSpecial(Piece* thePiece, bool theFlag)
{
	// 0x6c03a4: a doom gem, coal, a gem lock or a bomb breaking
	if (thePiece->IsFlagSet(Piece::FLAG_DEVIL)) {
		NotPorted("Board::BreakSpecial: a doom gem");
		return;
	}
	if (thePiece->IsFlagSet(Piece::FLAG_COAL)) {
		NotPorted("Board::BreakSpecial: coal");
		return;
	}
	if (thePiece->IsFlagSet(Piece::FLAG_LOCKED)) {
		NotPorted("Board::BreakSpecial: a gem lock");
		return;
	}
	if (!thePiece->IsFlagSet(Piece::FLAG_BOMB))
		return;
	int aX = Ftol((double)(gGemWidth / 2) + (double)thePiece->mX);
	int aY = Ftol((double)(gGemHeight / 2) + (double)thePiece->mY);
	float aFX = (float)aX;
	float aFY = (float)aY;
	Light aLight;
	aLight.mX = aFX;
	aLight.mY = aFY;
	aLight.mUnknown8 = 2.0f;
	aLight.mUnknownC = 1.0f;
	aLight.mUnknown10 = 0;
	aLight.mUnknown14 = 1.0f;
	aLight.mUnknown18 = true;
	aLight.mUnknown1C = 0.1f;
	aLight.mUnknown20 = 0.05f;
	mUnknown1650.push_back(aLight);
	mApp->PlaySample(SOUND_GEM_COUNTDOWN_DESTROYED);
	if (!mUnknown37C4) {
		mFXManager->AddBombFlash((float)(gGemWidth / 2) + thePiece->mX, (float)(gGemHeight / 2) + thePiece->mY,
			gGemColors[thePiece->mColor]);
	}
	LiftColumn(thePiece, 2.0f);
	int aCount = 20;
	do {
		Effect* anEffect = mFXManager->AllocEffect(39);
		anEffect->mColor = gGemColors[thePiece->mColor];
		anEffect->mX = aFX;
		anEffect->mY = aFY;
		mFXManager->AddEffect(anEffect);
		aCount--;
	} while (aCount != 0);
	if ((unsigned int)thePiece->mUnknown2EC != 0 && !IsBusy()) {
		float aRise = (float)((double)gApp->mScale * 8.0);
		AddPoints(aX, Ftol((double)(aFY - aRise)), 50, gGemColors[thePiece->mColor], thePiece->mUnknown2EC, true, true,
			true, true);
		if (!mUnknown2170.empty()) {
			// the points' smoke AddPoints just sent off
			SmokeTraveller* aSmoke = static_cast<SmokeTraveller*>(mUnknown2170.back());
			aSmoke->mHurry = (float)((double)aSmoke->mHurry * 0.3499999940395355);
		}
	}
	thePiece->mBlasted = true;
}

void Board::SpeedBonus()
{
	if (mUnknown25B4 == 0)
		return;
	unsigned int aGap = (unsigned int)(mUnknown25A8 - mUnknown25B0);
	if (100 < aGap || 150 < (unsigned int)mUnknown25AC + aGap) {
		mUnknown25B4 = 1;
		aGap = 0;
	}
	mUnknown25AC = (int)aGap;
	mUnknown25B0 = mUnknown25A8;
	int aLevel = mUnknown25B4 / 6;
	if (mUnknown25B4 % 6 != 0)
		return;
	if (kSpeedBonusMax < aLevel)
		aLevel = kSpeedBonusMax;
	int aScore = mUnknown168;
	AddPoints(-1, -1, kSpeedBonusPoints[aLevel], Color::White, -1, true, true, false, true);
	mUnknown16C += mUnknown168 - aScore;
	ShowSpeedText(aLevel - 1);
}

void Board::ShowSpeedText(int theLevel)
{
	if (mUnknown158 == 0)
		return;
	SpeedText* aText = new SpeedText(theLevel);
	aText->mMouseVisible = false;
	mUnknown23DC.push_back(aText);
	mApp->PlaySoundEx(SOUND_SPEEDBONUS, 0, (float)theLevel + (float)theLevel, 0.0f, true, true);
}
