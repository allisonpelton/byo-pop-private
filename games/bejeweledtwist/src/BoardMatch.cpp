// The board's matches: finding them (0x6cd468), the special gems they make and what follows a fall (0x6cf3fd)
#include "Board.h"

#include "GameText.h"
#include "GoodTube.h"
#include "Piece.h"
#include "Shockwave.h"
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
		if (theSerial == -1)
			theFirst = false;
		// port: the points float up from theX, theY in theColor (the PointsManager at +0x21fc: a floater of the same
		// serial takes them, else a new one in FONT_FLOATERS, sized by the points); it comes with the PointsManager
		NotPorted("Board::AddPoints: the floating points");
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
				// port: 0x6c3456, the bonus for quick moves (its points through 0x6bf983), comes with the points
				NotPorted("Board::MatchAfterMove: the quick moves' bonus (0x6c3456)");
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
		// port: a bomb in a match is defused (stat 0x47 unless +0x116, 0x6c03a4, its count cleared, the flag off); it
		// comes with the bombs
		NotPorted("Board::BreakPiece: a bomb");
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
		// port: a power gem's blast (stat 0x40, 50 points, 0x6b8c49, 0x729363) comes with the special gems
		NotPorted("Board::BreakPiece: a power gem");
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
