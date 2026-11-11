// The board's matches: finding them (0x6cd468), the special gems they make and what follows a fall (0x6cf3fd)
#include "Board.h"

#include "EffectsManager.h"
#include "ElectroStrike.h"
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
	thePiece->mSparkleAlpha = 0;
	thePiece->mHasSparkle = true;
	thePiece->mSparkleDelay = gSparkDelay;
}

// 0x6b40b5: the player's profile opens theMode (once) and the banner of it comes up (DrawOverlay). The original also
// writes "UNLOCKED <mode>, LEVEL, RANK, total playtime" to its trace stream.
void Board::UnlockMode(int theMode)
{
	if (mApp->mProfile->IsModeUnlocked(theMode))
		return;
	mApp->mProfile->UnlockMode(theMode);
	mApp->PlaySample(SOUND_GAMEUNLOCKED);
	mUnlockedMode = theMode;
	mBannerStripe.SetCurve(GTS(0x7dcbd4));
	mBannerShow.SetCurve(GTS(0x7dcb90));
	mBannerModePicture.SetCurve(GTS(0x7dcb58));
	mBannerGlow.SetCurve(GTS(0x7dcb10));
	mBannerSheenColor.SetCurve(GTS(0x7dcad8));
	mBannerSheenPos.SetCurve(GTS(0x7dcaa4));
}

// 0x6b3432: true once the tube's bonus challenge is met (the board has the bonus colours' count matched); in a
// challenge, the challenge's goal
bool Board::CheckGoal(bool theFlag)
{
	if (mGoodTube != nullptr && 0.0f < mGoodTube->mBonusTimer)
		return false;
	if (mGameMode == MODE_CHALLENGE) {
		if (mChallenge != nullptr) {
			// theFlag asks for the game to end when the challenge has failed. The check is a final one (the stats
			// are kept) once the challenge is over (+0x266a, +0x2669)
			int aValue = 0;
			bool aFinal = mChallengeOver || mChallengeMet;
			int aResult = mApp->mChallengeInfo->Check(mChallenge, &aValue, aFinal);
			// +0x2638 is the stat the challenge counts, +0x2634 the best of it
			mChallengeCount = (aFinal && aValue < mChallengeCount) ? mChallengeCount : aValue;
			mChallengeBest = mChallengeBest < mChallengeCount ? mChallengeCount : mChallengeBest;
			if (aFinal)
				return true;
			UpdateChallengeTube(false);
			if ((mChallenge->mFlags & Challenge::FLAG_NIGHTMODE) == 0) {
				if (aResult != 1) {
					if (aResult == 2 && theFlag)
						EndGame(false);
					return false;
				}
			} else if (aResult == 0) {
				return false;
			}
			mChallengeMet = true;
		}
	} else {
		if (mBonusColors.size() != 0 && mBonusColors.size() <= (unsigned int)mBonusMatched && mBonusStage != 4) {
			mApp->PlayVoice(SOUND_VOICE_CHALLENGECOMPLETE, false);
			mApp->PlaySample(SOUND_CHALLENGECOMPLETED);
			mBonusChallengesMet++;
			if (mBonusStage == 3)
				mBonusStage = 4;
			else
				mGoodTube->StartBonus();
			return true;
		}
	}
	return false;
}

void Board::ResetFruitCount()
{
	if (mBonusMatchedMark != mBonusMatched && (mBonusMatched != 0 || mBonusMatchedMark == -1))
		return;
	// 0x6b35b1: the fruit's count starts again; a fruit dance that has begun (3) moves on to its end
	mBonusMatched = 0;
	mBonusMatchedMark = 0;
	if (mBonusStage == 3) {
		mBonusStage = 5;
		mGoodTube->mBonusGlowCurve.SetCurve(GTS(0x7dc558));
	}
}

void Board::MakePowerGem(Piece* thePiece, float theDelay)
{
	if (thePiece == nullptr)
		return;
	thePiece->MakePower(theDelay);
	if (!mQuietFill) {
		if (!IsBusy())
			AddStat(0x15, 1);
		if (mApp->mStats[0x26] == 0)
			SetStat(0x26, mLevel);
	}
}

void Board::MakeElectroGem(Piece* thePiece, float theDelay)
{
	if (thePiece == nullptr)
		return;
	thePiece->mColor = thePiece->mColor % 7;
	thePiece->MakeElectro(theDelay);
	if (!mQuietFill) {
		AddStat(0x14, 1);
		if (mApp->mStats[0x27] == 0)
			SetStat(0x27, mLevel);
	}
}

void Board::MakeIceGem(Piece* thePiece, float theDelay)
{
	thePiece->MakeIce(theDelay);
	if (!IsBusy())
		AddStat(0x16, 1);
	if (mApp->mStats[0x25] == 0)
		SetStat(0x25, mLevel);
}

// 0x6a4f56: resets +0x2e4 and +0x2e8, inline here
void Board::MakeFruitGem(Piece* thePiece)
{
	if (thePiece == nullptr)
		return;
	if (thePiece->IsFlagSet(Piece::FLAG_BOMB) || thePiece->IsFlagSet(Piece::FLAG_LOCKED))
		BreakSpecial(thePiece, false);
	thePiece->ClearSpecial();
	thePiece->SetFlag(Piece::FLAG_FRUIT, true);
	thePiece->mAura = 0;
	thePiece->mBombCount = 0;
	thePiece->mBlasted = false;
	if (6 < thePiece->mColor || thePiece->mColor < 0)
		thePiece->mColor = BoardRand() % 7;
	if (!mQuietFill) {
		thePiece->mBirthScale = 0;
		thePiece->mFruitTimer = 120;
		if (!IsBusy()) {
			AddStat(0x18, 1);
			thePiece->mDevilGlow = 1.0f;  // 0x6a4f56
			thePiece->mJustMade = 5;
			mApp->PlaySample(SOUND_FRUIT_APPEARS);
		}
	}
	mSettling = true;
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
			aPiece->mMatchCount = 0;
	}
}

void Board::ClearSparkStarts()
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				aPiece->mSparkleDelay = 0;
				aPiece->mSparkleAlpha = 0;
				aPiece->mHasSparkle = false;
				aPiece->mIsInMatch = false;
			}
		}
	}
}

Points* Board::AddPoints(int theX, int theY, int thePoints, const Color& theColor, int theSerial, bool theAddsProgress,
	bool theMultiplyBySets, bool theFirst, bool theByLevel)
{
	if (thePoints < 1)
		return nullptr;
	if (mGameMode == MODE_CHALLENGE) {
		// a challenge counts points only when its flags have 8
		if (mChallenge == nullptr || (mChallenge->mFlags & Challenge::FLAG_POINTS) == 0)
			return nullptr;
	}
	int aMult = mStepLevel;
	if (!theByLevel)
		aMult = 1;
	int aPoints = aMult * thePoints;
	if (1 < mMatchSetCount && theMultiplyBySets)
		aPoints *= mMatchSetCount;
	int aTotal = mModeMultiplier * aPoints;
	Points* aFloater = nullptr;  // what it returns
	if (mLevelStarted && -1 < theX && -1 < theY) {
		// A serial of -0x68 to -0x65 (the kinds of special gem) takes a new one, which remembers the kind
		if ((unsigned int)theSerial >= (unsigned int)-0x68 && (unsigned int)theSerial <= (unsigned int)-0x65) {
			int aNew = gSettleCount;
			gSettleCount++;
			mSpecialKindBySerial[aNew] = theSerial;
			theSerial = aNew;
		}
		// The points float up: a floater of the same serial (the same move's chain) takes them, else a new one
		int aSum = mMatchSetCount * thePoints;
		int aScaled = Ftol(std::pow((double)aMult, 0.45) * (double)aSum);
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
					aScaled += aPoints->mChainScaled;
					aPoints->mLife = 1.0f;
					aSum += aPoints->mChainSum;
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
				aFloater = new Points(mApp, FONT_FLOATERS, aText, theX, theY, 1.0f, theColor, -1);
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
			aFloater->mChainScaled = aScaled;
			aFloater->mFlash = (float)((double)aSize * 0.699999988079071);
			aFloater->mChainSum = aSum;
			aFloater->mUseLastLayerCycle = true;
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
	int aChains = mGameStats[0x21];
	float anAvg = aMultF;
	if (0 < aChains)
		anAvg = (float)((double)mGameStats[0x39] / (double)aChains);
	float aHigh = anAvg <= aMultF ? aMultF : anAvg;
	float aLow = anAvg < aMultF ? anAvg : aMultF;
	float aScale = (float)std::pow((double)aLow * 0.4 + (double)aHigh * 0.6, 0.7);
	if (!theByLevel)
		aScale = 1.0f;
	int aLeft = aTotal;
	if (0 < aTotal) {
		double aRatio = (double)(aScale / aMultF);
		do {
			int aStep = 100;
			if (aLeft < 100)
				aStep = aLeft;
			int aTo = Ftol(std::pow((double)(mTurnPoints + aStep) * aRatio, 0.75));
			int aFrom = Ftol(std::pow((double)mTurnPoints * aRatio, 0.75));
			int aGain = (aTo - aFrom) * 3;
			if (theAddsProgress && mLevelStarted) {
				mLevelProgress += aGain;
				mProgressGain += aGain;
			}
			aLeft -= aStep;
			// (the original adds the whole points each step, not the step's)
			mTurnPoints += aTotal;
		} while (0 < aLeft);
	}
	if (!mReplayPlaying && !mIsReplay && 0 < mLevel)
		mApp->mProfile->mTotalPoints += aTotal;
	if (mGameMode == MODE_CHALLENGE)
		theFirst = false;
	if (!mGoodTube->mLevelSoundOn || theX < 1 || theY < 1 || !theFirst || !mLevelStarted) {
		if (mLevelStarted)
			SetScore(mScore + aTotal, false);
		if (theFirst)
			AddChainStep();
	} else {
		// The points go to the tube as smoke, to the next light of its level; they reach the score on arrival
		int aSteps = mGoodTube->mChainSteps;
		int aDiv = GetLevelSteps(mGoodTube->mShownLevel);
		int aPoint[2];
		mGoodTube->GetStepPoint(aPoint, ((int)mSmokeTravellers.size() + aSteps) % aDiv, true);
		SmokeTraveller* aSmoke = new SmokeTraveller(theColor, theX, theY, mGoodTube->mX + aPoint[0],
			mGoodTube->mY + aPoint[1], aTotal, false);
		aSmoke->mBurst = true;
		mSmokeTravellers.push_back(aSmoke);
	}
	std::map<int, int>::iterator anIt = mSpecialKindBySerial.find(theSerial);
	int aKind = anIt != mSpecialKindBySerial.end() ? anIt->second : 0;
	if (aKind == -0x68)
		AddStat(0x3d, aTotal);
	else if (aKind == -0x67)
		AddStat(0x3c, aTotal);
	else if (aKind == -0x66)
		AddStat(0x3b, aTotal);
	else if (aKind == -0x65)
		AddStat(0x3a, aTotal);
	mGameStats[0x49] += aTotal;
	UpdateProgress(false);
	return aFloater;
}

// 0x6b3ffc: the big count of the board's gems left of it (drawn by DrawCounters)
void Board::ShowCountPopup()
{
	if (IsBusy())
		return;
	int aCount = mLevelGemCount;
	if (mCountPopupOn && mCountPopupMin <= aCount) {
		SetFontLayerColor(FONT_POPUP_COUNT, 1, Color(0x80, 0x80, 0x80));
		if (mCountPopupCount != aCount)
			mCountPopupCount = aCount;
		mCountPopupScale.SetCurve(GTS(0x7dca30));
		mCountPopupShadow = 1.0f;
		mCountPopupFade = 1.0f;
	}
}

int Board::FindMatches(unsigned int theFlags)
{
	bool aReal = (theFlags & 2) != 0;
	mMatchGemCount = 0;
	mMatchCenterY = 0;
	mMatchCenterX = 0;
	if (aReal) {
		ClearSparkStarts();
		mMatchIds.clear();
	}
	ClearHitCounts();
	if (mEditorNoMatches)
		return 0;
	std::vector<Piece*> aReset;    // the gems whose state is cleared once the specials are chosen
	bool aFirst = mChainLength == 0;
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
				if (mMatchLength <= aCount) {
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
				if (mMatchLength <= aCount) {
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
	if (mGameMode == MODE_CHALLENGE && (mUnlockFlags >> 6 & 1) != 0 && mIceGemsAllowed && 1 < mRows - 1) {
		int aRow = 1;
		do {
			int aCol = 1;
			if (1 < mCols - 1) {
				do {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr && aPiece->mBusyTime == 0) {
						std::vector<Piece*> aSquare;
						aSquare.push_back(aPiece);
						aSquare.push_back(GetPieceAt(aRow + 1, aCol + 1));
						aSquare.push_back(GetPieceAt(aRow - 1, aCol - 1));
						aSquare.push_back(GetPieceAt(aRow + 1, aCol - 1));
						aSquare.push_back(GetPieceAt(aRow - 1, aCol + 1));
						std::vector<Piece*> aSame;
						for (unsigned int i = 0; i < aSquare.size(); i++) {
							Piece* aGem = aSquare[i];
							if (aGem != nullptr && aGem->CanMove() && aGem->mBusyTime == 0 &&
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
	mMatchSetCount = aSets;
	unsigned int aMatchIndex = 0;
	if (aMatchCount != 0) {
		do {
			MatchInfo& aMatch = aMatches[aMatchIndex];
			if (mFirstMatchColor == -1)
				mFirstMatchColor = aMatch.mColor;
			if (mApp->mStats[0x1c] < aMatch.mCount) {
				SetStat(0x1c, aMatch.mCount);
				SetStat(0x1d, aMatch.mColor);
			}
			if (mGameStats[0x1c] < aMatch.mCount) {
				mGameStats[0x1c] = aMatch.mCount;
				mGameStats[0x1d] = aMatch.mColor;
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
				aWave->SetStrength((float)mChainLength);
				mShockwaves.push_back(aWave);
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
				MakeIceGem(aCentre, 0.25f);
				aCentre->mPendingDY = 0;
				aCentre->mPendingDX = 0;
				for (unsigned int i = 0; i < aMatch.mIds.size(); i++) {
					Piece* aGem = GetPieceById(aMatch.mIds[i]);
					if (aGem != nullptr && !aGem->mHasBroken) {
						BreakPiece(aGem);
						if (aGem->mPendingKind == 0) {
							if (aGem->mStrikeDelay == 0) {
								aGem->mPendingDX = (float)(((double)aCentre->mX - (double)aGem->mX) / 25.0);
								aGem->mPendingDY = (float)(((double)aCentre->mY - (double)aGem->mY) / 25.0);
								aGem->mPendingTime = 0.25f;
								aGem->mPendingKind = 0x40;
								aGem->mMatchSerial = aMatch.mSerial;
								aGem->mLockState = 0;
							}
						} else {
							aReset.push_back(aGem);
						}
					}
				}
				aReset.push_back(aCentre);
				aCentre->mHasSparkle = false;
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
							aGem->mBreakDelay = aMatchIndex * 15;
						if (aGem->mMatchCount == 0) {
							mGemsCleared++;
							if (!aGem->mHasBurst) {
								if (!mReplayPlaying && 0 < mLevel)
									mApp->mProfile->mGemsCleared++;
								mGameGemCount++;
								mLevelGemCount++;
								aGem->mHasBurst = true;
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
					aGem->mMatchCount++;
					if (aGem->IsFlagSet(Piece::FLAG_ELECTRO))
						anElectros++;
					else if (aGem->IsFlagSet(Piece::FLAG_POWER))
						aPowers++;
					else if (aGem->IsFlagSet(Piece::FLAG_ICE))
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
				if (aFruits < 3 && mMatchLength < aLength && (aMatch.mKind == 1 || aMatch.mKind == 2)) {
					if (mMatchLength + 1 < aLength) {
						if ((mSeenFlags >> 1 & 1) == 0 && 0 < mLevel)
							aTipGems.insert(aMatch.mIds[0]);
						anElectro = true;
					} else {
						if ((mSeenFlags & 1) == 0 && 0 < mLevel)
							aTipGems.insert(aMatch.mIds[0]);
						aPower = true;
					}
				}
				// Before the profile has them, an electro gem's match makes a power gem, and none makes a power gem
				if (((mUnlockFlags >> 3 & 1) == 0 || mLevel == 0) && anElectro) {
					anElectro = false;
					aPower = true;
				}
				if ((mUnlockFlags >> 4 & 1) == 0 || mLevel == 0)
					aPower = false;
				gSparkDelay = 0;
				if (aReal) {
					int aPoints = 0;
					int aCascade = mChainLength;
					if (aCascade == 0 || mCascadePointsOn) {
						if (6 < aCascade)
							aCascade = 6;
						aPoints = kCascadePoints[aCascade] + kMatchPoints[aMatch.mCount];
					}
					int aSerial = aMatch.mSerial;
					for (unsigned int i = 0; i < aMatch.mIds.size(); i++) {
						Piece* aGem = GetPieceById(aMatch.mIds[i]);
						if (aGem != nullptr) {
							aGem->mMatchSerial = aSerial;
							if (aGem->IsFlagSet(Piece::FLAG_FRUIT) && !aGem->mFruitCounted)
								aGem->mFruitCounted = true;
						}
					}
					if (anElectros == 0 || aPowers == 0 || aSixes == 0) {
						if (aPower || anElectro)
							MakeMatchSpecial(aMatch, aReset, aPower, anElectro);
					} else {
						// A power, an electro and an ice gem in one match: every gem on the board loses its
						// special and blows, from the middle rows out
						Piece** aCell = mGrid;
						for (int aLeft = mCols * mRows; 0 < aLeft; aLeft--) {
							Piece* aGem = *aCell;
							if (aGem != nullptr && !aGem->mBreaking) {
								if (aGem->IsFlagSet(Piece::FLAG_POWER) || aGem->IsFlagSet(Piece::FLAG_ICE) ||
									aGem->IsFlagSet(Piece::FLAG_ELECTRO))
									aGem->ClearSpecial();
								aGem->mBusyTime = std::abs(aGem->mRow - mRows / 2) * 20 + 10;
								aGem->mMatchSerial = aLeft;
							}
							aCell++;
						}
						AddStat(0x1e, 1);
					}
					aMatch.mPoints = aPoints;
				}
				// The tips for power and electro gems, in casual and classic
				if ((theFlags & 1) != 0 && (mGameMode == MODE_ZEN || mGameMode == MODE_CLASSIC)) {
					if (!aPower || (mSeenFlags & 1) != 0) {
						if (!anElectro || (mSeenFlags >> 1 & 1) != 0)
							goto next_match;
						mSlowDownWait = 0x50;
					} else {
						mSlowDownWait = 0x28;
					}
					mMatchFlash = 1.0f;
					mMatchFlashDecay = 0.015f;
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
		bool aPowerTips = (mUnlockFlags >> 4 & 1) != 0 && 0 < mLevel;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr)
					continue;
				if (aPowerTips && 1 < aPiece->mMatchCount && (theFlags & 1) != 0 &&
					(mGameMode == MODE_ZEN || mGameMode == MODE_CLASSIC) && (mSeenFlags & 1) == 0) {
					aTipGems.insert(aPiece->mId);
					mMatchFlash = 1.0f;
					mSlowDownWait = 0x28;
					mMatchFlashDecay = 0.015f;
				}
				aPiece->mMatchCount = 0;
			}
		}
		return FinishMatches(aMatches, aTipGems);
	}
	for (unsigned int i = 0; i < aReset.size(); i++) {
		Piece* aGem = aReset[i];
		aGem->mSparkleAlpha = 0;
		aGem->mGlow = 0;
		aGem->mBreaking = false;
		aGem->mAura = 0;
		aGem->mBlasted = false;
		aGem->mMatchCount = 0;
		aGem->mHasSparkle = false;
		aGem->mBusyTime = 0;
		aGem->mHasPulsingLight = false;
		aGem->mHasBurst = false;
	}
	{
		// A gem in two matches (an L or a T) becomes a power gem
		bool aPowerGems = (mUnlockFlags >> 4 & 1) != 0 && 0 < mLevel;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr || !aPowerGems || aPiece->mMatchCount < 2)
					continue;
				aPiece->mBreaking = false;
				if (!aPiece->IsSpecial() || aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->mHasBroken ||
					aPiece->mPendingKind != 0) {
					MakePowerGem(aPiece, 0.0f);
				} else {
					BreakPiece(aPiece);
					if (aPiece->IsFlagSet(Piece::FLAG_FRUIT))
						continue;
					aPiece->mBreaking = false;
					aPiece->mBlasted = false;
					aPiece->mStrikeDelay = 0x10;
				}
				int aSerial = -1;
				aPiece->mAura = 0;
				aPiece->mMatchCount = 0;
				aPiece->mBusyTime = 0;
				aPiece->mHasBroken = true;
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
						aGem->mMatchSerial = aPiece->mMatchSerial;
						if (aGem == aPiece) {
							// The match's other gems fly into it
							for (unsigned int j = 0; j < aMatch.mIds.size(); j++) {
								Piece* anOther = GetPieceById(aMatch.mIds[j]);
								if (anOther != nullptr && !anOther->mHasBroken && anOther != aPiece &&
									aPiece->mStrikeDelay == 0) {
									anOther->mPendingDX = (float)(((double)aPiece->mX - (double)anOther->mX) / 25.0);
									float aY = aPiece->mY;
									anOther->mBreakDelay = 0;
									anOther->mPendingKind = 0x10;
									anOther->mPendingDY = (float)(((double)aY - (double)anOther->mY) / 25.0);
									anOther->mPendingTime = 0.25f;
									aPiece->mLockState = 0;
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
			if (aPiece != nullptr && aPiece->mMatchCount != 0) {
				aPiece->mHasBroken = false;
				BreakPiece(aPiece);
			}
		}
	}
	if (mElectroStrikes.size() != 0) {
		// (the original reads every cell without a check for a gap)
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr && aPiece->mBreaking && aPiece->mBreakDelay == 0 &&
					!aPiece->IsFlagSet(Piece::FLAG_COAL))
					aPiece->mBreakDelay = 1;
			}
		}
	}
	if (0 < aMatchCount) {
		// A cascade
		mChainLength++;
		mUpdatesSinceCascade = 0;
		AddStat(0x52, 1);
		if ((unsigned int)mApp->mStats[0x2c] <= (unsigned int)mChainLength)
			mApp->mStats[0x2c] = mChainLength;
		if (mCascadePopupOn && 2 < mChainLength) {
			mCascadePopupScale.SetCurve(GTS(0x7dca30));
			mCascadePopupShadow = 1.0f;
			mCascadePopupFade = 1.0f;
			mCascadePopupCount = mChainLength;
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
			int aCascade = mChainLength;
			if (6 < aCascade)
				aCascade = 6;
			mApp->PlaySample(aCascade + SOUND_COMBO_1);
		} else {
			mApp->PlaySoundEx(aSixSound, 0, aPitch, 0.0f, true, true);
		}
		for (int i = 0; i < 24; i++)
			mGlowRing[i] = 1.5f;
	}
	mMatchesInTurn += aMatchCount;
	AddStat(0x29, aMatchCount);
	mTwistPieces.clear();
	if (DropPieces())
		FillInBlanks(false);
	if (aMatches.size() != 0) {
		// The match of the fruit's next colour goes first
		unsigned int aNext = (unsigned int)mBonusMatched;
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
			mTurnMatches.push_back(aMatch);
			unsigned int aNext = (unsigned int)mBonusMatched;
			if (aNext < mBonusColors.size()) {
				if ((unsigned int)mBonusColors[aNext] == (unsigned int)aMatch.mColor) {
					if (mBonusStage == 3)
						mApp->PlaySoundEx(SOUND_FRUITBONUSCOUNT, 0, (float)(unsigned int)mBonusMatched, 0.0f, true,
							true);
					mBonusMatched++;
					if (mBonusMatched == 4 && mBonusStage == 2)
						mBonusStage = 1;
				}
				if ((unsigned int)mBonusColors[0] == (unsigned int)aMatch.mColor)
					aFruitMatched = true;
			}
			if (aMatch.mPoints != 0) {
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
					Points* aPoints = AddPoints(aSumX / aCount, aSumY / aCount - aLift, aMatch.mPoints,
						gGemColors[aGem->mColor], aMatch.mSerial, true, true, aFirst, true);
					if (aFirst && 1 < mSmokeTravellers.size()) {
						// The first points of a cascade, with other smoke already on its way: theirs waits and goes
						// slower
						SmokeTraveller* aSmoke = static_cast<SmokeTraveller*>(mSmokeTravellers.back());
						aSmoke->mDelay = 0.1f;
						aSmoke->mHurry = (float)((double)aSmoke->mHurry * 0.3499999940395355);
					}
					aFirst = false;
					if (aPoints != nullptr)
						aPoints->mDelay = aDelay;
				}
			}
			aDelay += 15;
		}
	}
	if (mChainLength == 1) {
		if (mBonusStage == 0 && mBonusMatched == mBonusMatchedMark && aFruitMatched) {
			mBonusMatchedMark = 0;
			mBonusMatched = 1;
		} else {
			ResetFruitCount();
		}
	}
	if (1 < mMatchSetCount)
		mApp->PlaySoundAt(SOUND_DOUBLESET, anAvgX, (float)(mMatchSetCount - 2), 0.0f);
	return FinishMatches(aMatches, aTipGems);
}

// part of FindMatches (0x6cd468): its end, from the matches' average position to the count it returns
int Board::FinishMatches(std::vector<MatchInfo>& theMatches, std::set<int>& theTipGems)
{
	if (mMatchGemCount != 0) {
		mMatchCenterX = mMatchCenterX / mMatchGemCount;
		mMatchCenterY = mMatchCenterY / mMatchGemCount;
	}
	if (0.0f < mMatchFlash) {
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
					GetPieceById(aMatch.mIds[i])->mIsInMatch = true;
			}
		}
	}
	return (int)theMatches.size();
}

// part of FindMatches (0x6cd468): the special gem a match of four, five or more makes
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
		if (aGem->mMatchCount != 0 && !aGem->IsFlagSet(Piece::FLAG_ELECTRO) && !aGem->IsFlagSet(Piece::FLAG_POWER) &&
			!aGem->IsFlagSet(Piece::FLAG_FRUIT) && (aGem->mLastRow != aGem->mRow || aGem->mLastCol != aGem->mCol))
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
			unsigned int aRoll = (unsigned int)BoardRand();
			aChosen = GetPieceById(theMatch.mIds[aRoll % aCount]);
		} else {
			unsigned int aRoll = (unsigned int)BoardRand();
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
					aPiece->mHasBroken = true;
					if (mMatchLength + 2 < theMatch.mCount) {
						aPiece->mPendingKind += 0x10;
						AddStat(0x15, 1);
					}
					for (unsigned int i = 0; i < theMatch.mIds.size(); i++) {
						Piece* aGem = GetPieceById(theMatch.mIds[i]);
						if (aGem == nullptr || aGem->mHasBroken)
							continue;
						BreakPiece(aGem);
						if (!aGem->IsSpecial()) {
							if (aGem == aPiece || aGem->mPendingKind != 0) {
								theReset.push_back(aGem);
							} else if (aGem->mStrikeDelay == 0) {
								aGem->mPendingDX = (float)(((double)aPiece->mX - (double)aGem->mX) / 25.0);
								aGem->mPendingDY = (float)(((double)aPiece->mY - (double)aGem->mY) / 25.0);
								aGem->mPendingKind = 8;
								aGem->mPendingTime = 0.25f;
								aGem->mMatchSerial = theMatch.mSerial;
								aGem->mBreakDelay = 0;
								aGem->mLockState = 0;
							}
						} else {
							aGem->mIsCounted = false;
						}
					}
				}
			} else {
				MakePowerGem(aPiece, 0.25f);
				aPiece->mPendingDY = 0;
				aPiece->mPendingDX = 0;
				aPiece->mHasBroken = true;
				for (unsigned int i = 0; i < theMatch.mIds.size(); i++) {
					Piece* aGem = GetPieceById(theMatch.mIds[i]);
					if (aGem == nullptr || aGem->mHasBroken)
						continue;
					BreakPiece(aGem);
					if (!aGem->IsSpecial()) {
						if (aGem == aPiece || aGem->mPendingKind != 0) {
							theReset.push_back(aGem);
						} else if (aGem->mStrikeDelay == 0) {
							aGem->mPendingDX = (float)(((double)aPiece->mX - (double)aGem->mX) / 25.0);
							aGem->mPendingDY = (float)(((double)aPiece->mY - (double)aGem->mY) / 25.0);
							aGem->mPendingKind = 0x10;
							aGem->mPendingTime = 0.25f;
							aGem->mIsCounted = false;
							aGem->mMatchSerial = theMatch.mSerial;
							aGem->mBreakDelay = 0;
							aGem->mLockState = 0;
						}
					} else {
						aGem->mIsCounted = false;
					}
				}
			}
		} else {
			aPiece->mStrikeDelay = theElectro ? 8 : 16;
		}
	} else {
		BreakPiece(aPiece);
	}
	if (aPiece != nullptr) {
		theReset.push_back(aChosen);
		aPiece->mHasSparkle = false;
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
		if (mLevelStepCount < anEnd)
			break;
		aLevel++;
		aStart = anEnd;
		if (aLevel == 10) {
			if (theLeft != nullptr)
				*theLeft = mLevelStepCount - anEnd;
			return 10;
		}
	}
	if (theLeft != nullptr)
		*theLeft = mLevelStepCount - aStart;
	if (aLevel < 11)
		return aLevel;
	return 10;
}

void Board::SetLevelSteps(int theSteps)
{
	int aLeft = 0;
	int aShown = mGoodTube->mShownLevel;
	mLevelStepCount = theSteps;
	int aLevel = GetLevelFor(&aLeft);
	mStepLevel = aLevel;
	if (aLevel == 10 && GetLevelSteps(10) * 2 <= aLeft) {
		if (!mFruitDue)
			mOverflowLevelCount++;
		mFruitDue = true;
	}
	if (mLevelStarted) {
		mGoodTube->SetLevelSteps(aLeft);
		mGoodTube->SetLevelShown(aLevel);
		if (aLevel < aShown) {
			mGoodTube->MarkLevelChange();
			mGoodTube->mLevelFlash = mGoodTube->mLevelFlash * 0.5f;
		} else if (aShown < aLevel && aLeft == 0) {
			mGoodTube->MarkLevelChange();
			if (mApp->mStats[0x36] == 0 && aLevel == 2)
				mApp->mStats[0x36] = mLevel;
			else if (mApp->mStats[0x37] == 0 && aLevel == 2)
				mApp->mStats[0x37] = mLevel;
		}
	}
}

void Board::AddChainStep()
{
	bool aCounted;
	if (!mLevelStarted || !mGoodTube->mLevelSoundOn) {
		aCounted = false;
	} else {
		mLevelStepCount++;
		aCounted = true;
	}
	// The step that starts a level counts twice
	int aLeft = 0;
	GetLevelFor(&aLeft);
	if (aLeft == 0 && aCounted)
		mLevelStepCount++;
	AddStat(0x2a, 1);
	if (mGameMode != MODE_CHALLENGE)
		SetLevelSteps(mLevelStepCount);
}

void Board::ShowTubeLevel()
{
	if (mGoodTube != nullptr && mLevelStarted)
		mGoodTube->SetLevelShown(mStepLevel);
}

void Board::MatchAfterMove()
{
	int aMatches;
	if (mGameMode == MODE_PUZZLE) {
		aMatches = 0;
	} else {
		unsigned int aFruitCount = (unsigned int)mBonusMatched;
		mTurnMatches.clear();
		int aCleared = mGemsCleared;
		if (HasChallengeFlag(0x10))
			CheckGoal(false);
		aMatches = FindMatches(2);
		if (aCleared < mGemsCleared)
			ShowCountPopup();
		if (mGameMode == MODE_CHALLENGE) {
			if (mChallenge != 0)
				CheckGoal(false);
		} else {
			if (!CheckGoal(false) && mChainLength < 1 && (unsigned int)mBonusMatched == aFruitCount) {
				if (aFruitCount < mBonusColors.size() && aFruitCount == (unsigned int)mBonusMatchedMark)
					mBonusMatched = 0;
			}
		}
		if (mTurnEndSoundDue && mLevelStarted) {
			mTurnEndSoundDue = false;
			if (aMatches == 0) {
				if (!IsSparkBlocked()) {
					// The chain is over: the steps fall back to the level's start, and a chain that added none drops a level
					int aStat = mLevelStepCount % 10;
					if (4 < aStat)
						AddStat(mLevelStepCount == 10 ? 0x13 : aStat + 9, 1);
					int aSteps = mLevelStepCount;
					int aLevel = 1;
					int aStart = 0;
					mOverflowLevelCount = 0;
					if (1 < mStepLevel) {
						do {
							aStart += GetLevelSteps(aLevel);
							aLevel++;
						} while (aLevel < mStepLevel);
					}
					if (aStart == aSteps) {
						int aDown = mStepLevel - 1;
						int i = 1;
						if (aDown < 1)
							aDown = i;
						aStart = 0;
						mStepLevel = aDown;
						if (1 < aDown) {
							do {
								aStart += GetLevelSteps(i);
								i++;
							} while (i < mStepLevel);
						}
					}
					mLevelStartMark = 100;  // 0x6a3ff4
					SetLevelSteps(aStart);
					mChainLinks = 0;
					SetStat(0x2a, aStart);
					mGameStats[0x2a] = 0;  // (the app's copy keeps the steps)
					mLevelCounts = false;
					mCountersAllowed = true;
				}
			} else {
				mChainLinks++;
				int aChain = mChainLinks;
				mChainShine = 0.5f;
				int aBest = mLevelRecords[0];
				if (aBest <= aChain)
					aBest = aChain;
				mLevelRecords[0] = aBest;
				if (mGameMode == MODE_CHALLENGE) {
					AddStat(0x2a, 1);
				} else if (1 < aChain && 49 < aChain && aChain % 10 == 0) {
					// 0x6cf3fd: "%d CHAIN!" on the tube (Color::White stands for the exe's 0xe14068)
					SexyString aChainText = StrFormat(mApp->GetString(GTS(0x7dd764), GT(0x7dd750)).c_str(), mChainLinks);
					mGoodTube->SetTip(aChainText, Color::White, 0.0f, 10000);
				}
			}
			unsigned int& aBestChain = (unsigned int&)mApp->mStats[0x2b];
			if (aBestChain <= (unsigned int)mChainLinks)
				aBestChain = (unsigned int)mChainLinks;
			int aChain = mChainLinks;
			int aBest = mGameStats[0x2b];
			if (aBest <= aChain)
				aBest = aChain;
			mGameStats[0x2b] = aBest;
			if (0 < mLevel) {
				int& aProfileBest = mApp->mProfile->mBestChain;
				if (aChain <= aProfileBest)
					aChain = aProfileBest;
				aProfileBest = aChain;
			}
		}
	}
	if (mTurnCountedForSpeed != mTurnCount) {
		if (aMatches == 0) {
			mChainFlashSet = -1;
			mChainFlashKind = 1;
			mQuickMoveCount = 0;
			mQuickMoveGap = mSettledUpdates;
		} else {
			mChainsMade++;
			mChainFlash = 1.0f;
			mChainAdded = true;
			if (mChainLength == 1)
				PlayChainSound(mChainLinks + 1);
			if (mGameMode != MODE_PUZZLE && mLevelStarted && mGoodTube->mLevelSoundOn && !mBeforeFirstTwist && !mGameLoaded)
				mTurnCountedForSpeed = mTurnCount;
			mQuickMoveCount++;
			if (!mReplayPlaying && mGameMode != MODE_CHALLENGE) {
				SpeedBonus();
			}
		}
	}
	if (mInputLock < 1)
		mSettling = true;
	if (mWaitDialogUp) {
		mWaitDialogUp = false;
		mApp->KillDialog(40);
		mApp->mWaitDialogActive = false;
	}
	if (mTurnCountedForStats != mTurnCount) {
		AddStat(0x39, mStepLevel);
		int aCascade = mChainLength;
		if (4 < aCascade)
			AddStat(aCascade < 10 ? aCascade + 3 : 0xd, 1);
		mTurnCountedForStats = mTurnCount;
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
					(0 < aPiece->mBusyTime || (aPiece->mBombCount == 1 && aPiece->mCounterFlash != 0.0f)))
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
	mSettling = false;
	EvaluateCursor(nullptr);
	if (!aDropped && aNew == 0 && mElectroStrikes.size() == 0 && mHintsReady)
		SettleTurn();
	else if (mInputLock < 1)
		mInputLock = 0;
	CheckCombos();
}

void Board::BreakPiece(Piece* thePiece)
{
	thePiece->mMatchCount = 0;
	mChainShine = 0.5f <= mChainShine ? mChainShine : 0.5f;
	if (!mColorGemBroke && thePiece->mColor < 7)
		mColorGemBroke = true;
	if (thePiece->mHasPendingCount) {
		thePiece->mHasPendingCount = false;
		return;
	}
	if (0 < mChainLength) {
		// The cell glows in the gem's colour, brighter down a cascade
		int aColor = thePiece->mColor;
		int anAlpha = mChainLength * 0x55;
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
		if (!thePiece->mFruitCounted)
			AddStat(0x47, 1);
		BreakSpecial(thePiece, false);
		thePiece->mBombCount = 0;
		thePiece->ClearFlag(Piece::FLAG_BOMB);
	}
	if (thePiece->mHasBroken) {
		thePiece->mBreaking = false;
		return;
	}
	int aColor = thePiece->mColor;
	if (-1 < aColor && aColor < 7)
		AddStat(aColor + 0x4b, 1);
	if (mInputLock < 1 && !IsBusy()) {
		mSettling = true;
		mInputLock = 0;
	}
	if (thePiece->IsFlagSet(Piece::FLAG_DROPPER)) {
		AddStat(0x46, 1);
		if (thePiece->mPendingKind == 0)
			thePiece->mBirthScale = 0.0f;
		if (thePiece->mBreaking)
			thePiece->mBirthScale = 0.75f;
	} else if (thePiece->IsFlagSet(Piece::FLAG_ICE) && !thePiece->mHasBroken) {
		// Never reached: an ice gem is made only from a challenge's square match (MatchInfo kind 4)
		// or a soul of one, and squares need the profile's bit 6 (only tutorial.txt's "unlock icegem", which no line
		// has) and "IceGemsAllowed" (false in config.xml; only config_action.xml, which no mode loads, says true).
		// Its blast (stat 0x42, 50 sparks each of two kinds, 0x6baa5a, 75 points, Rand draws) is left out
		NotPorted("Board::BreakPiece: a hypercube");
	} else if (thePiece->IsFlagSet(Piece::FLAG_COAL) && !thePiece->mHasBroken) {
		AddStat(0x4a, 1);
		BreakSpecial(thePiece, 1);
	} else if (thePiece->IsFlagSet(Piece::FLAG_ELECTRO) && !thePiece->mHasBroken) {
		// An electro gem goes off, unless a strike of its own runs already: it leaves the other strikes' lit gems,
		// and its strike (+0x149c) starts
		unsigned int i = 0;
		if (mElectroStrikes.size() != 0) {
			do {
				ElectroStrike* aStrike = static_cast<ElectroStrike*>(mElectroStrikes[i]);
				if (aStrike->mPieceId == thePiece->mId)
					return;
				std::vector<int>& anIds = aStrike->mIds;
				unsigned int j = 0;
				if (anIds.size() != 0) {
					do {
						if (anIds[j] == thePiece->mId)
							anIds.erase(anIds.begin() + j);
						j++;
					} while (j < anIds.size());
				}
				i++;
			} while (i < mElectroStrikes.size());
		}
		bool aPower = thePiece->IsFlagSet(Piece::FLAG_POWER);
		if (!thePiece->mHasBurst) {
			if (aPower) {
				AddStat(0x40, 1);
				AddStat(0x53, 1);
			}
			AddStat(0x41, 1);
			AddStat(0x3e, 1);
			thePiece->mHasBurst = true;
			mGameGemCount++;
			mLevelGemCount++;
		}
		mSpecialKindBySerial[thePiece->mMatchSerial] = -0x65;  // 0x6b8c49
		thePiece->mGlow = 0;
		mApp->PlaySample(SOUND_WILDCARD_DESTROYED);
		ElectroStrike* aStrike = new ElectroStrike(thePiece);
		mElectroStrikes.push_back(aStrike);  // 0x6b9a24
		AddPoints(Ftol((double)(gGemWidth / 2) + (double)thePiece->mX),
			Ftol((double)(gGemHeight / 2) + (double)thePiece->mY), 150, gGemColors[thePiece->mColor],
			thePiece->mMatchSerial, true, true, false, true);
		thePiece->mBreaking = false;
		thePiece->mMatchCount = 0;
		thePiece->mBusyTime = 0;
		thePiece->mHasBroken = true;
		mInputLock = 0;
		PlayPieceSound(thePiece, aPower ? SOUND_FLAMELIGHTNING : GetSoundById(SOUND_ELECTRO_START1_ID), 0.0f, 0.0f,
			true);
		return;
	} else if (thePiece->IsFlagSet(Piece::FLAG_POWER)) {
		if (!thePiece->mHasBurst) {
			AddStat(0x40, 1);
			AddPoints(Ftol((double)(gGemWidth / 2) + (double)thePiece->mX),
				Ftol((double)(gGemHeight / 2) + (double)thePiece->mY), 50, gGemColors[thePiece->mColor],
				thePiece->mMatchSerial, true, true, false, true);
			thePiece->mHasBurst = true;
			mGameGemCount++;
			mLevelGemCount++;
		}
		mSpecialKindBySerial[thePiece->mMatchSerial] = -0x66;  // 0x6b8c49
		thePiece->Blast();
	} else if (thePiece->IsFlagSet(Piece::FLAG_LOCKED)) {
		thePiece->BreakLock(false);
	} else if (thePiece->IsFlagSet(Piece::FLAG_FRUIT)) {
		if (mFruitBlast != 0.0f) {
			// A fruit's blast is running: this one waits
			thePiece->mBreaking = false;
			thePiece->mBusyTime = 1;
			thePiece->mAura = 1.0f;
			return;
		}
		// The fruit's blast starts (UpdateFruitBlast), and the fruit dance's count of matches (+0x265c 1 or 2)
		AddStat(0x45, 1);
		PlayPieceSound(thePiece, SOUND_GEM_FRUIT_DESTROYED, 0.0f, 0.0f, true);
		mFruitsBroken++;
		mFruitBrokeFree = true;
		int aRow = 0;
		if (0 < mRows) {
			do {
				int aCol = 0;
				if (0 < mCols) {
					do {
						Piece* aPiece = GetPieceAt(aRow, aCol);
						if (aPiece != nullptr && (aPiece->IsFlagSet(Piece::FLAG_LOCKED) || aPiece->IsFlagSet(Piece::FLAG_BOMB)))
							mFruitBrokeFree = false;
						aCol++;
					} while (aCol < mCols);
				}
				aRow++;
			} while (aRow < mRows);
		}
		mFruitBlast = 0.0001f;
		mFruitBrokeThisTurn = true;
		if (mBonusStage == 0)
			mBonusStage = (mGoodTube->mBonusState == 0) + 1;
		else
			mBonusMatchedMark = -1;
		mHintsReady = false;
		mFruitColor = thePiece->mColor;
		mFruitCol = GetColAt(Ftol((double)thePiece->mX));
		mFruitRow = GetRowAt(Ftol((double)(gGemHeight / 2) + (double)thePiece->mY));
		mFruitId = thePiece->mId;
		mBlastRingSize.SetCurve(GTS(0x7dd2f8));
		mBlastRingAlpha.SetCurve(GTS(0x7dd2dc));
		mFruitGlowAlpha.SetCurve(GTS(0x7dd2b0));
		mFruitBlastScale.SetCurve(GTS(0x7dd28c));
		mFallDelay = 0;
		thePiece->mBreaking = false;
		return;
	} else if (thePiece->IsFlagSet(Piece::FLAG_DEVIL)) {
		AddStat(0x44, 1);
		BreakSpecial(thePiece, false);
		thePiece->mBlasted = true;
	}
	if (!thePiece->mFruitCounted)
		mGemsBroken++;
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
	mColumnBreakTime[GetColAt(Ftol((double)thePiece->mX))] = mUpdateCnt;
	Light aLight;
	aLight.mX = aX;
	aLight.mY = aY;
	aLight.mSize = 15.0f;
	aLight.mFalloff = 20.0f;
	aLight.mLevel = 0;
	aLight.mStrength = 1.0f;
	aLight.mRising = true;
	aLight.mRiseRate = 0.1f;
	aLight.mFadeRate = 0.05f;
	mLights.push_back(aLight);

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
			anEffect->mVelocityDamping = (float)((double)RandSigned() * 0.005 + 0.98);
		}
		anEffect->mAngle = (float)anAngle;
		anEffect->mDAngle = (float)((double)RandSigned() * 0.05000000074505806);
		anEffect->mGravity = (float)((double)gApp->mScale * 0.05999999865889549);
		anEffect->mBlend = (float)((double)RandSigned() * (double)kTwoPiF);
		anEffect->mRateFrom = (float)((double)RandSigned() * (double)kTwoPiF);
		anEffect->mRateTo = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
		anEffect->mTumbleTurn = (float)(((double)fabsf(RandSigned()) * 3.0 + 1.0) * 0.04500000178813934);
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
		anEffect->mRateFrom = (float)((1.0 - (double)aRand) * (double)anEffect->mRateFrom);
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
		// Unless the board is +0x37c4's, a heat wave at the gem (0x6f9efe, no Direct3D test: 2D mode adds it too and
		// only ages it), of the strength 0.8 and the growth 0.02
		if (!mIsReplay) {
			HeatWave* aWave = mFXManager->AddHeatWave((float)(gGemWidth / 2) + thePiece->mX,
				(float)(gGemHeight / 2) + thePiece->mY, 0.8f);
			aWave->mGrowth = 0.02f;
		}
	}

	if (mCursorPiece == thePiece)
		mCursorPiece = nullptr;
	if (theBreak) {
		MatchShockwave* aWave = new MatchShockwave(aCenterX, aCenterY);
		aWave->SetStrength(2.0f);
		mShockwaves.push_back(aWave);
		BreakPiece(thePiece);
		if (!thePiece->mHasBurst && !IsBusy()) {
			AddStat(0x3e, 1);
			thePiece->mHasBurst = true;
			if (!mReplayPlaying && 0 < mLevel)
				mApp->mProfile->mGemsCleared++;
			mGameGemCount++;
			mLevelGemCount++;
			ShowCountPopup();
		}
		if (thePiece->IsFlagSet(Piece::FLAG_COAL))
			thePiece->mBreaking = true;
		else if (thePiece->mBusyTime == 0)
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
				aPiece->mFallSpeed = aFirst <= aPiece->mFallSpeed ? aFirst : aPiece->mFallSpeed;
			}
			mColumnLift[aCol] = aKick < aFirst ? aFirst : aKick;
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
				aKick = aFirst <= aPiece->mFallSpeed ? aFirst : aPiece->mFallSpeed;
				aPiece->mFallSpeed = aKick;
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
		mColumnLift[aCol] = aFirst;
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
		BurstAt(GetColX(thePiece->mLastCol) + gGemWidth / 2, GetRowY(thePiece->mBurstRow) + gGemHeight / 2, 1);
	}
	if (!thePiece->mHasBurst) {
		AddStat(0x3e, 1);
		if (!mReplayPlaying && 0 < mLevel)
			mApp->mProfile->mGemsCleared++;
		mGameGemCount++;
		mLevelGemCount++;
		thePiece->mHasBurst = true;
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
	mColumnBreakTime[GetColAt(theX)] = mUpdateCnt;
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
	aLight.mSize = 2.0f;
	aLight.mFalloff = 1.0f;
	aLight.mLevel = 0;
	aLight.mStrength = 1.0f;
	aLight.mRising = true;
	aLight.mRiseRate = 0.1f;
	aLight.mFadeRate = 0.05f;
	mLights.push_back(aLight);
	sBurstCentres[sBurstCount][0] = aCentreX;
	sBurstCentres[sBurstCount][1] = aCentreY;
	sBurstCount++;
	for (int aDY = -theRange; aDY <= theRange; aDY++) {
		for (int aDX = -theRange; aDX <= theRange; aDX++) {
			Piece* aGem = GetPieceAt(gGemWidth * aDX + theX, gGemHeight * aDY + theY);
			if (aGem == nullptr || aGem->mBusyTime != 0)
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
					// A power gem in reach: the count +0x1528 and its +0x88 = 10. The original reads the app's integers
					// "ExplosionChainBonusAdd" (default 10) and "ExplosionChainBonus" (40) here (0x4a3f30) and drops
					// them, so the port leaves the reads out
					mBurstPowerGems++;
					aGem->mBusyTime = 10;
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
	mBurstSoundDue = true;
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
			// the speed times the sine and cosine at the FPU's precision, not rounded to float before the conversion
			int aX = Ftol((double)aDist * (double)cosf(anAngle)) + sBurstCentres[aCentre][0];
			int aY = Ftol((double)aDist * (double)sinf(anAngle)) + sBurstCentres[aCentre][1];
			// (the original works out the angle from the gem it reached, atan2 at 0x77ea70, and keeps nothing of it)
			Effect* anEffect = mFXManager->AllocEffect(17);
			anEffect->mX = (float)aX;
			anEffect->mY = (float)aY;
			anEffect->mDX = (float)((double)cosf(anAngle) * (double)aDist * 0.25099998712539673);
			anEffect->mDY = (float)((double)sinf(anAngle) * (double)aDist * 0.25099998712539673);
			mFXManager->AddEffect(anEffect);
		}
	}
	if (0 < mBurstPowerGems) {
		float aCount = (float)mBurstPowerGems;
		float aShake = (float)((double)aCount * 0.2 + 1.2);
		if (2.0f <= aShake)
			aShake = 2.0f;
		mShakeAmount = aShake;
		float aSize = (float)((double)aCount * 1.2 + 1.25);
		if (6.0f <= aSize)
			aSize = 6.0f;
		mShakeSize = aSize;
	}
}

// 0x6ba87f: a geode flung from (theX, theY) along theAngle. Its draws from Rand are kept (the factors are 0).
// 0x6a3217: the geode record's three curved values, constructed here
Geode* Board::AddGeode(int theX, int theY, float theAngle)
{
	if (mReplayPlaying)
		mGeodesPending = true;
	Geode* aGeode = new Geode();
	aGeode->mFlight.SetCurve(GTS(0x7dcd88));
	aGeode->mBrightness.SetCurve(GTS(0x7dcd68), &aGeode->mFlight);
	aGeode->mX = (float)theX;
	aGeode->mY = (float)theY;
	float aCos = cosf(theAngle);
	float aSpeed = (float)((double)gApp->mScale * 4.0);
	aGeode->mDX = (float)((double)RandSigned() * 0.0 + (double)(aSpeed * aCos));
	float aSin = sinf(theAngle);
	aSpeed = (float)((double)gApp->mScale * 4.0);
	aGeode->mDY = (float)((double)RandSigned() * 0.0 + (double)(aSpeed * aSin));
	aGeode->mX = (float)((double)aGeode->mDX * 10.0 + (double)aGeode->mX);
	aGeode->mY = (float)((double)aGeode->mDY * 10.0 + (double)aGeode->mY);
	aGeode->mLife = (float)((double)fabsf(RandSigned()) * 0.0 + 0.44999998807907104);
	aGeode->mArmTurn.SetCurve(GTS(0x7dcd50), &aGeode->mFlight);
	aGeode->mLandCount = 0;
	aGeode->mPoints = 100;
	mGeodes.push_back(aGeode);  // 0x6b9a79
	return aGeode;
}

void Board::BreakSpecial(Piece* thePiece, int theType)
{
	// 0x6c03a4: a doom gem, coal, a gem lock or a bomb breaking
	if (thePiece->IsFlagSet(Piece::FLAG_DEVIL)) {
		// A doom gem: 25 bursts, a hundred times three sparklets in purple and pale green, and ten points
		mGeodeFlash = 1.0f;
		mApp->PlaySample(SOUND_DOOMGEM_DESTROYED);
		int aX = Ftol((double)(gGemWidth / 2) + (double)thePiece->mX);
		int aY = Ftol((double)(gGemHeight / 2) + (double)thePiece->mY);
		float aFX = (float)aX;
		float aFY = (float)aY;
		int aCount = 25;
		do {
			Effect* anEffect = mFXManager->AllocEffect(38);
			anEffect->mX = (float)((double)anEffect->mDX * 10.0 + (double)aFX);
			anEffect->mY = (float)(10.0 * (double)anEffect->mDY + (double)aFY);
			mFXManager->AddEffect(anEffect);
			aCount--;
		} while (aCount != 0);
		int aBursts = 100;
		do {
			float aSize = (float)((double)fabsf(RandSigned()) * 0.10000000149011612 + 1.5);
			float aReach = (float)((double)gApp->mScale * 5.0);
			float aFar = fabsf(RandSigned());
			float aTwice = gApp->mScale + gApp->mScale;
			float aSpeed = (float)((double)aFar * (double)aReach * (double)aTwice);
			RandSigned();
			int i = 0;
			do {
				EffectsManager* aFXManager = mFXManager2;
				Effect* anEffect = aFXManager->AllocEffect(0);
				anEffect->mImage = IMAGE_SPARKLET;
				if (i == 0) {
					Rand();
					anEffect->mColor = Color(40, 0, 40);
					anEffect->mScale = (float)((double)aSize * 3.009999990463257);
					anEffect->mAlpha = 1.0f;
					anEffect->mDAlpha = -0.005f;
				} else if (i == 1) {
					Rand();
					anEffect->mColor = Color(100, 0, 100);
					anEffect->mScale = (float)((double)aSize * 1.100000023841858);
					anEffect->mAlpha = 0.8f;
					anEffect->mDAlpha = -0.005f;
				} else if (i == 2) {
					Rand();
					anEffect->mColor = Color(200, 250, 200);
					anEffect->mScale = aSize;
					anEffect->mAlpha = 0.5f;
					anEffect->mDAlpha = -0.004f;
				}
				float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
				anEffect->mIsAdditive = true;
				anEffect->mDScale = -0.008f;
				anEffect->mDXScalar = 0.95f;
				anEffect->mX = aFX;
				anEffect->mDX = cosf(anAngle) * aSpeed;
				anEffect->mY = aFY;
				anEffect->mDY = sinf(anAngle) * aSpeed;
				anEffect->mGravity = (float)((double)gApp->mScale * -0.0);
				anEffect->mDXScalar = 0.99f;
				anEffect->mDYScalar = 0.99f;
				aFXManager->AddEffect(anEffect);
				i++;
			} while (i < 3);
			aBursts--;
		} while (aBursts != 0);
		if (mJumpStage == 0) {
			int i = 0;
			do {
				float aRise = (float)((double)gApp->mScale * 8.0);
				AddPoints(aX, Ftol((double)aFY - (double)aRise), 50, gGemColors[thePiece->mColor], thePiece->mMatchSerial,
					true, true, true, true);
				if (!mSmokeTravellers.empty()) {
					// the points' smoke AddPoints just sent off: dark purple, later and slower each time
					SmokeTraveller* aSmoke = static_cast<SmokeTraveller*>(mSmokeTravellers.back());
					aSmoke->mAdditive = false;
					aSmoke->mColor = Color(100, 24, 64);
					float aStep = (float)i;
					aSmoke->mHurry = (float)((0.02 - (double)aStep * 0.009999999776482582) * (double)aSmoke->mHurry);
					aSmoke->mDelay = (float)((double)aStep * 0.029999999329447746);
				}
				i++;
			} while (i < 10);
			// Two heat waves at the gem follow (0x6f9efe with 2.0, no Direct3D test), their growth 0.01 and 0.012
			HeatWave* aWave = mFXManager->AddHeatWave((float)(gGemWidth / 2) + thePiece->mX,
				(float)(gGemHeight / 2) + thePiece->mY, 2.0f);
			aWave->mGrowth = 0.01f;
			aWave = mFXManager->AddHeatWave((float)(gGemWidth / 2) + thePiece->mX,
				(float)(gGemHeight / 2) + thePiece->mY, 2.0f);
			aWave->mGrowth = 0.012f;
		}
		return;
	}
	if (thePiece->IsFlagSet(Piece::FLAG_COAL)) {
		// Coal: one to five geodes fly out in a fan, with a nova where it was
		int aCount = 1;
		if (BoardRand() % 2 == 0)
			aCount = 2;
		if (BoardRand() % 3 == 0)
			aCount++;
		if (BoardRand() % 4 == 0)
			aCount++;
		if (BoardRand() % 5 == 0)
			aCount++;
		BoardRand();
		int aSteps = aCount - 1;
		float aSpread = (float)((double)(float)aSteps * 0.8);
		PlayPieceSound(thePiece, SOUND_COAL_BLASTED, 0.0f, 0.0f, true);
		int aDelay = 45;
		for (int i = 0; i < aCount; i++) {
			int aDivisor = 1 <= aSteps ? aSteps : 1;
			float aAngle = (float)((double)i / (double)aDivisor * (double)aSpread -
				((double)aSpread * 0.5 + 1.5707963705062866));
			int aY = Ftol((double)(gGemHeight / 2) + (double)thePiece->mY);
			int aX = Ftol((double)(gGemWidth / 2) + (double)thePiece->mX);
			Geode* aGeode = AddGeode(aX, aY, aAngle);
			aGeode->mIndex = i;
			aGeode->mDelay = theType == 2 ? aDelay - 25 : aDelay;
			int aBonus = mGeodeBonusCount * 50;
			if (250 < aBonus)
				aBonus = 250;
			aGeode->mPoints += aBonus;
			mGeodeBonusCount++;
			aDelay += 10;
		}
		mGeodeFlash = 1.0f;
		Effect* aNova = mFXManager2->AllocEffect(43);
		aNova->mX = (float)(gGemWidth / 2) + thePiece->mX;
		aNova->mY = (float)(gGemHeight / 2) + thePiece->mY;
		aNova->mScale = 0;
		aNova->mDScale = 0.035f;
		aNova->mDAlpha = 0;
		mFXManager2->AddEffect(aNova);
		return;
	}
	if (thePiece->IsFlagSet(Piece::FLAG_LOCKED)) {
		// A gem lock: two lines of nine shards each (type 13, or 14 for theType), turned half a turn on the right
		mApp->PlaySample(SOUND_GEM_LOCK_BREAK);
		int aFrame = 0;
		int i = 0;
		int aType = (theType != 0) + 13;
		do {
			int aSide = 0;
			do {
				double aX;
				if (aSide == 0) {
					aX = (double)(i * gGemWidth) / 9.0 + (double)thePiece->mX;
				} else {
					float aWidth = (float)gGemWidth;
					aX = ((double)thePiece->mX + (double)aWidth) - (double)aWidth / 9.0;
				}
				float aY = (float)(((double)gGemHeight + (double)thePiece->mY) - (double)(i * gGemHeight) / 9.0);
				Effect* anEffect = gBoard->mFXManager->AllocEffect(aType);
				anEffect->mX = (float)aX;
				anEffect->mFrame = aFrame;
				anEffect->mY = aY;
				anEffect->mAngle = aSide == 0 ? 0.0f : 3.1415927f;
				gBoard->mFXManager->AddEffect(anEffect);
				aSide++;
			} while (aSide < 2);
			aFrame ^= 1;
			i++;
		} while (i < 9);
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
	aLight.mSize = 2.0f;
	aLight.mFalloff = 1.0f;
	aLight.mLevel = 0;
	aLight.mStrength = 1.0f;
	aLight.mRising = true;
	aLight.mRiseRate = 0.1f;
	aLight.mFadeRate = 0.05f;
	mLights.push_back(aLight);
	mApp->PlaySample(SOUND_GEM_COUNTDOWN_DESTROYED);
	if (!mIsReplay) {
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
	if ((unsigned int)thePiece->mMatchSerial != 0 && !IsBusy()) {
		float aRise = (float)((double)gApp->mScale * 8.0);
		AddPoints(aX, Ftol((double)(aFY - aRise)), 50, gGemColors[thePiece->mColor], thePiece->mMatchSerial, true, true,
			true, true);
		if (!mSmokeTravellers.empty()) {
			// the points' smoke AddPoints just sent off
			SmokeTraveller* aSmoke = static_cast<SmokeTraveller*>(mSmokeTravellers.back());
			aSmoke->mHurry = (float)((double)aSmoke->mHurry * 0.3499999940395355);
		}
	}
	thePiece->mBlasted = true;
}

void Board::SpeedBonus()
{
	if (mQuickMoveCount == 0)
		return;
	unsigned int aGap = (unsigned int)(mSettledUpdates - mQuickMoveTime);
	if (100 < aGap || 150 < (unsigned int)mQuickMoveGap + aGap) {
		mQuickMoveCount = 1;
		aGap = 0;
	}
	mQuickMoveGap = (int)aGap;
	mQuickMoveTime = mSettledUpdates;
	int aLevel = mQuickMoveCount / 6;
	if (mQuickMoveCount % 6 != 0)
		return;
	if (kSpeedBonusMax < aLevel)
		aLevel = kSpeedBonusMax;
	int aScore = mScore;
	AddPoints(-1, -1, kSpeedBonusPoints[aLevel], Color::White, -1, true, true, false, true);
	mScoreAtTurnStart += mScore - aScore;
	ShowSpeedText(aLevel - 1);
}

void Board::ShowSpeedText(int theLevel)
{
	if (mLevel == 0)
		return;
	SpeedText* aText = new SpeedText(theLevel);
	aText->mMouseVisible = false;
	mPendingDialogs.push_back(aText);
	mApp->PlaySoundEx(SOUND_SPEEDBONUS, 0, (float)theLevel + (float)theLevel, 0.0f, true, true);
}
