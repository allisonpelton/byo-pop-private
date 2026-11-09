// The board at rest: the next move, the tips for new kinds of gem and the turn's end
#include "Board.h"

#include "CustomBassMusicInterface.h"
#include "FlyingText.h"
#include "FruitAwardScreen.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Piece.h"
#include "Profile.h"
#include "Res.h"
#include "SoundInstance.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

void Board::SettleTurn()
{
	mCursorTurnBase = mCursorTurnNow;
	if (IsBusy() || mElectroStrikes.size() != 0 || mHintShowing)
		return;
	CheckTips();
	ShowNextTip();
	CheckLevelDone();
	mInputLock = 0;
	// The original writes "Move FOUND at" the move, or "NO MOVE FOUND", to the debugger
	FindMove(&mBestMoveRow, &mBestMoveCol, false);
	if (mGameMode == MODE_CHALLENGE) {
		if (mChallenge != 0)
			CheckGoal(false);
	} else if (mBonusColors.size() == 0) {
		// The tube's text once a bonus challenge is over
		SexyString aText;
		if (!IsBusy())
			aText = gApp->GetString(GTS(0x7dd398), GT(0x7dd370));
		else
			aText = gApp->GetString("", _S(""));
		mGoodTube->mText = aText;
	}
	// The gems that cannot move, and the special ones among them
	mMatchesInTurn = 0;
	mFrozenGemCount = 0;
	mFrozenSpecialCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				continue;
			if (!aPiece->IsFlagSet(Piece::FLAG_BOMB) && !aPiece->IsFlagSet(Piece::FLAG_DEVIL) &&
				!aPiece->IsFlagSet(Piece::FLAG_DROPPER)) {
				if (CanMove(aPiece))
					continue;
			} else {
				mFrozenSpecialCount++;
			}
			mFrozenGemCount++;
		}
	}
	if (mHintShowing || mSettling || mTwisting || !mHintsReady || mFruitBlast != 0.0f ||
		mElectroStrikes.size() != 0)
		return;
	if (!mFruitDue) {
		if (mBonusStage != 4) {
			bool aFirst = mBeforeFirstTwist;
			if (mTutorial != nullptr)
				mTutorial->MoveDone();
			EndTurn();
			if (!aFirst && !mGameLoaded) {
				mTurnCount++;
				AddStat(0x21, 1);
				if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr && !mGameLoaded) {
					mApp->mChallengeInfo->EndMove(mChallenge);
					CheckGoal(true);
				}
			}
		}
	} else {
		mFruitDue = false;
		// The multiplier went past its top (SetLevelSteps) or a fruit dance ended: a fruit (AwardFruit), and steps
		// past 151 (at most 14) go on top of the steps of the levels below the board's
		AwardFruit(-1, -1);
		if (152 <= mLevelStepCount) {
			int aSteps = mLevelStepCount - 151;
			if (14 < aSteps)
				aSteps = 14;
			mLevelStepCount = aSteps;
			int aLevel = 1;
			if (1 < mStepLevel) {
				do {
					mLevelStepCount += GetLevelSteps(aLevel);
					aLevel++;
				} while (aLevel < mStepLevel);
			}
			SetLevelSteps(mLevelStepCount);
		}
	}
}

void Board::AwardFruit(int theRow, int theCol)
{
	// The fruits' colours go round the seven
	int aColor = mFruitsAwarded % 7;
	Piece* aPiece;
	if (theRow < 0) {
		// A bomb's or a locked gem's place if there is one (whatever its colour), else the first plain gem of the
		// colour from a cell picked at random, round the grid
		int aCount = mCols * mRows;
		aPiece = FindHintPiece();
		if (aPiece == nullptr) {
			int anIndex = BoardRand() % aCount;
			int aTried = 0;
			if (0 < aCount) {
				do {
					Piece* aCell = mGrid[anIndex];
					if (aCell != nullptr && aCell->mColor == aColor && aCell->mFlags == 0 && !aCell->mBlasted &&
						!aCell->mBreaking) {
						aPiece = aCell;
						break;
					}
					anIndex++;
					if (!(anIndex < aCount))
						anIndex -= aCount;
					aTried++;
				} while (aTried < aCount);
			}
		}
	} else {
		aPiece = GetPieceAt(theRow, theCol);
	}
	// Else up to 100 cells at random, any plain gem
	int aTry = 0;
	while (aPiece == nullptr) {
		int aCol = BoardRand() % mCols;
		int aRow = BoardRand() % mRows;
		aPiece = GetPieceAt(aRow, aCol);
		if (aPiece != nullptr && (aPiece->mBlasted || aPiece->mBreaking || aPiece->mFlags != 0))
			aPiece = nullptr;
		aTry++;
		if (aTry == 100)
			return;
	}
	FruitAwardScreen* aScreen = new FruitAwardScreen(this);
	if (mBonusStage == 4) {
		// The fruit dance is on: the fruits leave from the tube's bottom
		aScreen->mFromY = Ftol((double)mGoodTube->mHeight * 0.8500000238418579 + (double)mGoodTube->mY);
	}
	mGoodTube->mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
	aScreen->SetPiece(aPiece->mId);
	mPendingDialogs.push_back(aScreen);
	mFruitsAwarded++;
}

void Board::QueueTip(int theKind, Piece* thePiece)
{
	if (mIsReplay)
		return;
	// The first gem of each kind; a kind already waiting keeps its gem
	mWaitingTips.insert(std::make_pair(theKind, thePiece == nullptr ? 0 : thePiece->mId));
}

void Board::ShowNextTip()
{
	if (mBonusStage == 4 || mFruitBlast != 0.0f || mWaitingTips.empty() || mTipShowing)
		return;
	// The first waiting tip, by its kind, and the gem it points at
	std::map<int, int>::iterator aTip = mWaitingTips.begin();
	int aKind = aTip->first;
	int anId = aTip->second;
	mWaitingTips.erase(aTip);
	if (0 <= mApp->mAutoPlayMode) {
		// -autoplay sees every tip at once
		mApp->mProfile->SetTipSeen(aKind, true);
		return;
	}
	Piece* aPiece = GetPieceById(anId);

	// The script that teaches the kind (the names are the labels in properties/tutorial.txt)
	std::string aName;
	switch (aKind) {
	case 0: aName = "flamegem"; break;
	case 1: aName = "lightninggem"; break;
	case 3: aName = "skullintro"; break;
	case 4: aName = "lockintro"; break;
	case 5: aName = "devilintro"; break;
	case 6: aName = "fruitintro"; break;
	case 7: aName = "coal"; break;
	case 8: aName = "timefreeze"; break;
	case 9: aName = "bombgem"; break;
	case 0xb: aName = "wheel"; break;
	case 0xc: aName = "chains2x"; break;
	case 0xd: aName = "chains3x"; break;
	case 0xe: aName = "basics"; break;
	case 0x12: aName = "supernova"; break;
	default: break;
	}
	if (aName.empty() || (aPiece == nullptr && anId != 0))
		return;

	mTipKind = aKind;
	mTipShowing = true;
	// (the original prints "[RUNNING SCRIPT" and the name with OutputDebugString)
	if (mTipScript.GetDataLen() == 0) {
		// The script's lines, a line each (blank lines skipped, and a last line without an end of line)
		Buffer aFile;
		if (gApp->ReadBufferFromFile("properties/tutorial.txt", &aFile, true)) {
			SexyString aLine;
			for (unsigned char aChar : aFile.mData) {
				if (aChar == '\n' || aChar == '\r') {
					aLine = Trim(aLine);  // 0x45d880
					if (!aLine.empty())
						mTipScript.WriteString(aLine);
					aLine.clear();
				} else {
					aLine += (SexyChar)aChar;
				}
			}
		}
	}
	mTipScript.SeekFront();
	mTutorial = new Tutorial(this, aPiece, aName.c_str(), &mTipScript);
	mTutorial->Resize(0, 0, mWidth, mHeight);
	mTutorial->mKind = aKind;
	mPendingDialogs.push_back(mTutorial);
}

void Board::ShowTube(bool theShow, bool theInstant)
{
	if (theShow) {
		mGoodTube->UpdateImages();
		if (!mLevelStarted) {
			mShownBeforeStart = true;
			if (mTwisting) {
				mTwisting = false;
				LoadGame(mSaveBuffer);
			}
		}
		int aSpace = gBoard->mWidth - GetBoardWidth();
		int aHeight = mGoodTube->GetTubeHeight();
		int aWidth = mGoodTube->GetTubeWidth();
		aSpace -= gBoard->mGoodTube->GetTubeWidth();
		mGoodTube->Resize(Ftol((double)(aSpace / 3) - (double)mTubeOffsetX), 0, aWidth, aHeight);
	}
	mLevelStarted = theShow;
	if (theShow) {
		mGoodTube->mVisible = true;
		if (theInstant)
			mGoodTube->mX = 0;
		else
			StartTubeSlide();
	} else {
		mGoodTube->mVisible = false;
		if (theInstant)
			mGoodTube->mX = mApp->mWidth / 2 - GetImageOffsetX(0x3d6);
	}
}

void Board::CloseTip()
{
	Tutorial* aTip = mTutorial;
	if (aTip != nullptr) {
		if (aTip->mMarkSeen)
			mApp->mProfile->SetTipSeen(aTip->mKind, true);
		if (mTutorial->mKind == 0xe) {
			mReplayPodDue = false;
			if (gApp->mStartClassicAtOnce) {
				mApp->EraseFile(mApp->GetSaveName(mGameMode, mChallengeCategory));
				mGameOver = true;
			} else {
				mGoodTube->SetShown(true);
				mGoodTube->SetLevelSoundOn(true);
				mShownScore = 0;
				mScore = 0;
				mChainLinks = 0;
				mLevelStepCount = 0;
				SetLevelSteps(0);
			}
			mUnlockFlags = mApp->mProfile->mGemTypesUnlocked;
			mSeenFlags = mApp->mProfile->mTipsSeen;
			mApp->mProfile->mRank = 0;
			mApp->mProfile->Save();
		}
		// (the board's dialog queue deletes it)
		mTutorial = nullptr;
	}
	mTwistSpeedScale = 1.0f;
	mClicksIgnored = false;
	mSpeedTarget = 1.0f;
	mFallHeld = false;
	mSpeedNow = 1.0f;
	mApp->mWidgetManager->SetFocus(this);
	if (mTipShowing && ((mSeenFlags >> 0xe) & 1) == 0) {
		mApp->mProfile->SetTipSeen(0xe, true);
		// 0x75fc8a
		if (mApp->mBoard == nullptr || !mApp->mBoard->mIsReplay)
			std::memset(mApp->mStats, 0, sizeof(mApp->mStats));
		std::memset(mGameStats, 0, sizeof(mGameStats));  // 0x6a69f0
		ReadConfig(true);
		gApp->mProfile->Save();
	}
	mSeenFlags = mApp->mProfile->mTipsSeen;
	mTipShowing = false;
}

bool Board::CheckTips()
{
	if (mTipShowing)
		return true;
	if (IsBusy() || mGameMode == MODE_BLITZ)
		return false;
	Piece* aKind1 = nullptr;
	Piece* aDevil = nullptr;
	Piece* aLocked = nullptr;
	Piece* aCoal = nullptr;
	Piece* aBomb = nullptr;
	Piece* aFruit = nullptr;
	Piece* anElectro = nullptr;
	Piece* aPower = nullptr;
	Piece* anElectroPower = nullptr;
	bool aBombAtZero = false;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr || aPiece->mFlags == 0)
				continue;
			if (aPiece->IsFlagSet(Piece::FLAG_BOMB)) {
				aBomb = aPiece;
				if (aPiece->mBombCount == 0)
					aBombAtZero = true;
			} else if (aPiece->IsFlagSet(Piece::FLAG_DROPPER)) {
				aKind1 = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_LOCKED)) {
				if (aPiece->IsFlagSet(Piece::FLAG_DOOM))
					aDevil = aPiece;
				else
					aLocked = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
				aDevil = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_FRUIT)) {
				aFruit = aPiece;
			} else if (!aPiece->IsFlagSet(Piece::FLAG_ELECTRO)) {
				if (aPiece->IsFlagSet(Piece::FLAG_POWER))
					aPower = aPiece;
				else if (aPiece->IsFlagSet(Piece::FLAG_COAL))
					aCoal = aPiece;
			} else if (aPiece->IsFlagSet(Piece::FLAG_POWER)) {
				anElectroPower = aPiece;
			} else {
				anElectro = aPiece;
			}
		}
	}
	if (aBombAtZero)
		return false;
	// Each tip once, unless the profile has seen it (its bit in +0x3914)
	bool aShown = false;
	if (aBomb != nullptr && (mSeenFlags >> 9 & 1) == 0) {
		QueueTip(9, aBomb);
		aShown = true;
	}
	if (aFruit != nullptr && (mSeenFlags >> 6 & 1) == 0) {
		QueueTip(6, aFruit);
		aShown = true;
	}
	if (aKind1 != nullptr && (mSeenFlags >> 3 & 1) == 0) {
		QueueTip(3, nullptr);
		aShown = true;
	}
	if (aDevil != nullptr && (mSeenFlags >> 5 & 1) == 0)
		QueueTip(5, aDevil);
	if (aLocked != nullptr && (mSeenFlags >> 4 & 1) == 0)
		QueueTip(4, aLocked);
	if (anElectro != nullptr && (mSeenFlags >> 1 & 1) == 0) {
		QueueTip(1, anElectro);
		aShown = true;
	}
	if (aPower != nullptr && (mSeenFlags & 1) == 0) {
		QueueTip(0, aPower);
		aShown = true;
	}
	if (aCoal != nullptr && (mSeenFlags >> 7 & 1) == 0) {
		QueueTip(7, aCoal);
		aShown = true;
	}
	if (anElectroPower != nullptr && (mSeenFlags >> 0x12 & 1) == 0) {
		QueueTip(0x12, anElectroPower);
		aShown = true;
	}
	return aShown;
}

bool Board::DropFlaggedGems()
{
	if (IsSparkBlocked())
		return false;
	int aCol = 0;
	bool aDropped = false;
	if (0 < mCols) {
		do {
			int aRow = mRows;
			while (aRow--, -1 < aRow) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr)
					continue;
				if (aPiece->IsFlagSet(Piece::FLAG_DROPPER) && !aPiece->mIsKept && !aPiece->mBreaking) {
					Piece* aBelow = GetPieceAtXY(Ftol(aPiece->mX), Ftol((double)gGemHeight + (double)aPiece->mY));
					if (aBelow != nullptr && aBelow->mPendingDX == 0.0f && aBelow->mPendingDY == 0.0f &&
						!aBelow->mBreaking) {
						// The gem drops onto the one below (a 0x34-byte object, 0x6f4985, into +0x21e4, updated by
						// 0x6f465f). Never reached: no gem ever has FLAG_DROPPER (SetFlag(FLAG_DROPPER) is called only by 0x6c2490, which
						// only that update calls; otherwise the flag is only kept, copied by a soul or loaded)
						NotPorted("Board::DropFlaggedGems: the drop (0x6f4985)");
						aDropped = true;
					} else {
						if ((double)GetRowY(mRows - 1) > (double)aPiece->mY)
							break;
						// At the bottom row the game ends and the board blows up
						EndGame(true);
					}
					aPiece->mBlasted = true;
					aPiece->mFruitCounted = true;
					break;
				}
				aPiece->mIsKept = false;
			}
			aCol++;
		} while (aCol < mCols);
	}
	return aDropped;
}

void Board::EndTurn()
{
	if (mIsReplay || mReplayPlaying)
		mReplayMoveReady = true;
	mUnlockFlags = mApp->mProfile->mGemTypesUnlocked;
	mSeenFlags = mApp->mProfile->mTipsSeen;
	if (!mMatchAtOnce) {
		int aMoves = mGemsBroken - mGemsBrokenAtTurnStart;
		if (aMoves < mMostMovesInLevel)
			aMoves = mMostMovesInLevel;
		mMostMovesInLevel = aMoves;
		mMatchCheckDue = false;
	}
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				int aLeft = aPiece->mLockState;
				if (1 < aLeft) {
					aPiece->mLockFlash = 1.0f;
					aPiece->mLockState = aLeft - 1;
				}
				aPiece->mStrikeDelay = 0;
				aPiece->mHasBurst = false;
			}
		}
	}
	if (mCoalMax != 0) {
		if (mGameStats[74] == 0) {
			int aCoal = mCoalLevel;
			mCoalDecayCount++;
			if ((aCoal + 2) * 50 < mCoalDecayCount) {
				mCoalDecayCount = 0;
				if (0 < aCoal)
					mCoalLevel = aCoal - 1;
			}
		} else {
			int aCoal = mCoalLevel;
			if (aCoal < mGameStats[74])
				aCoal = mGameStats[74];
			mCoalDecayCount = 0;
			mCoalLevel = aCoal;
		}
	}
	if (CountDownCounters())
		return;
	if (!CheckTips()) {
		if (mTurnCount != mTurnSettled)
			mTurnSettled = mTurnCount;
	}
	ResetFruitCount();
	if (mGoalCheckDue) {
		CheckGoal(true);
		mGoalCheckDue = false;
	}
	if (mPlayState != 1) {
		int aMoves = mGemsBroken - mGemsBrokenAtTurnStart;
		UpdateRecords();
		if (20 < aMoves || 3 < mChainLength || mFruitBrokeThisTurn)
			mReplayPodDue = true;
	}
	if (mReplayPlaying) {
		if (mTutorial == nullptr && !mIsReplay)
			mReplayPodDue = true;
		if (mReplayPlaying) {
			mWidgetFlagsMod.mRemoveFlags &= ~WIDGETFLAGS_ALLOW_MOUSE;
			mReplayPlaying = false;
		}
	}
	if (mSparkBlock < 1) {
		if (mTurnSoundsOn && mTurnOfLastSound != mTurnCount) {
			mTurnOfLastSound = mTurnCount;
			DropFlaggedGems();
		}
	} else {
		mSparkBlock--;
		if (mSparkBlock == 0) {
			for (int i = 0; i < 2; i++) {
				if (mGlowSounds[i] != nullptr) {
					mGlowSounds[i]->Stop();
					mGlowSounds[i]->Release();
					mGlowSounds[i] = nullptr;
				}
			}
		}
	}
	if (!IsGameOverPending()) {
		if (-1 < mModeToUnlock) {
			UnlockMode(mModeToUnlock);  // 0x6b40b5
			mModeToUnlock = -1;
		}
		CheckCombos();
		mBeforeFirstTwist = false;
		mGameLoaded = false;
		if (mGameMode != MODE_CHALLENGE && (mGameMode == MODE_BLITZ || 1 < mLevel) && mBonusColors.size() == 0)
			StartBonusChallenge(-1);
	}
}

// 0x6a42ad: the getter of mStepLevel (+0x21d0), inline here
void Board::UpdateRecords()
{
	// 0x6bcd96: the session's (the app's) and the player's (the profile's) best gems in a move, move's points and
	// cascade, each with the game as the level started for its replay; a challenge's count only for the session
	if (mLevel == 0 || mIsReplay || mReplayPlaying)
		return;
	bool aForProfile = mGameMode != MODE_CHALLENGE;
	Profile* aProfile = mApp->mProfile;
	int aGems = mLevelGemCount;
	if (mApp->mBestMoves[0].mBest < aGems) {
		mApp->mBestMoves[0].mBuffer = mSaveBuffer;
		mApp->mBestMoves[0].mReplayStart = mReplayStartFrame;
		mApp->mBestMoves[0].mReplayEnd = mReplayEndFrame;
		mApp->mBestMoves[0].mBest = aGems;
		mBestMoveGems = aGems;
	}
	if (aForProfile && aProfile->mSavedGames[0].mBest < aGems) {
		aProfile->mSavedGames[0].mData = mSaveBuffer;
		aProfile->mSavedGames[0].mReplayStart = mReplayStartFrame;
		aProfile->mSavedGames[0].mReplayEnd = mReplayEndFrame;
		aProfile->mSavedGames[0].mBest = aGems;
	}
	// the move's points, with the geodes still in flight
	int aPoints = 0;
	for (unsigned int i = 0; i < mGeodes.size(); i++)  // 0x6bcd96
		aPoints += mGeodes[i]->mPoints * mStepLevel * mMatchSetCount * mModeMultiplier;
	aPoints = mScore - mScoreAtTurnStart + aPoints;
	if (mApp->mBestMoves[2].mBest < aPoints) {
		mBestMovePoints = aPoints;
		mApp->mBestMoves[2].mBuffer = mSaveBuffer;
		mApp->mBestMoves[2].mReplayStart = mReplayStartFrame;
		mApp->mBestMoves[2].mReplayEnd = mReplayEndFrame;
		mApp->mBestMoves[2].mBest = aPoints;
	}
	if (aForProfile && aProfile->mSavedGames[2].mBest < aPoints) {
		aProfile->mSavedGames[2].mData = mSaveBuffer;
		aProfile->mSavedGames[2].mReplayStart = mReplayStartFrame;
		aProfile->mSavedGames[2].mReplayEnd = mReplayEndFrame;
		aProfile->mSavedGames[2].mBest = aPoints;
	}
	if (mApp->mBestMoves[1].mBest < mChainLength) {
		mBestMoveChain = mChainLength;
		mApp->mBestMoves[1].mBuffer = mSaveBuffer;
		mApp->mBestMoves[1].mReplayStart = mReplayStartFrame;
		mApp->mBestMoves[1].mReplayEnd = mReplayEndFrame;
		mApp->mBestMoves[1].mBest = mChainLength;
	}
	if (aForProfile && aProfile->mSavedGames[1].mBest < mChainLength) {
		mGameStats[(0x22e4 - 0x2234) / 4] = mChainLength;
		aProfile->mSavedGames[1].mData = mSaveBuffer;
		aProfile->mSavedGames[1].mReplayStart = mReplayStartFrame;
		aProfile->mSavedGames[1].mReplayEnd = mReplayEndFrame;
		aProfile->mSavedGames[1].mBest = mChainLength;
	}
	if (mLongestChain < mChainLength)
		mLongestChain = mChainLength;
}

bool Board::IsGameOverPending()
{
	if (mGameOver)
		return true;
	if (mGameMode < MODE_CHALLENGE || MODE_BLITZ < mGameMode) {
		// The music is in danger while +0x262c is below 6
		CustomBassMusicInterface* aMusic = mApp->mTwistMusic;
		if (aMusic != nullptr)
			aMusic->SetState(mLowestBombCount < 6 ? 1 : 0);
	}
	return false;
}

namespace {

const int gPraiseRatings[6] = {3, 6, 12, 20, 30, 45};          // 0x84e960: the least rating for each praise
const int gPraiseSounds[6] = {951, 947, 940, 953, 957, 961};   // 0x7dbf00
const int gPraiseImages[6] = {721, 676, 580, 725, 759, 763};   // 0x7dbf18

}  // namespace

void Board::CheckCombos()
{
	bool aDemo = mIsReplay;
	mChainShine = 0;
	bool aShow = !CheckLevelDone() && !aDemo;
	if (mGameMode == MODE_BLITZ && mBlitzStartTime <= (1000 / mApp->mFrameTime) * 10)
		aShow = false;
	int aRating = GetTurnRating();
	int aLevel = 5;
	if (mLastPraiseLevel < 5) {
		do {
			if (gPraiseRatings[aLevel] <= aRating) {
				if (0 < aLevel && !mIsReplay)
					mReplayPodDue = true;
				if (aShow)
					ShowPraise(aLevel);
				return;
			}
			aLevel--;
		} while (mLastPraiseLevel < aLevel);
	}
}

int Board::GetTurnRating()
{
	mTurnRating = 0;
	int aPart = mGameStats[74] * 2 - 1;
	if (aPart < 0)
		aPart = 0;
	mTurnRating = aPart;
	float aCascades;
	if (0.0f <= (float)std::pow((double)(float)(mChainLength - 1), (double)1.5f))  // 0x670a49
		aCascades = (float)std::pow((double)(float)(mChainLength - 1), (double)1.5f);
	else
		aCascades = 0;
	mTurnRating += Ftol((double)aCascades);
	aPart = mGameStats[64] * 2 - 1;
	if (aPart < 0)
		aPart = 0;
	int aSum = mTurnRating + aPart;
	int aPart2 = mGameStats[65] * 2 - 1;
	mTurnRating = aSum;
	if (aPart2 < 0)
		aPart2 = 0;
	int aPart3 = mGameStats[20] * 2 - 1;
	mTurnRating = mGameStats[69] * mGameStats[69] + mGameStats[72] + mGameStats[71] + mGameStats[21] + aSum +
		aPart2;
	if (aPart3 < 0)
		aPart3 = 0;
	mTurnRating += aPart3;
	aPart = mGameStats[28] * 8 - 40;
	if (aPart < 0)
		aPart = 0;
	mTurnRating += aPart;
	mTurnRating += Ftol(std::pow((double)mLevelGemCount / 15.0, 1.5));
	return mTurnRating;
}

void Board::ShowPraise(int theLevel)
{
	if (mGameMode == MODE_CHALLENGE || mLevelDone || IsBusy())
		return;
	if (mApp->mUnmanagedSound != nullptr) {
		// another voice is still speaking: UpdateGame shows it once that has faded
		mPendingPraise = theLevel;
		return;
	}
	int aSound = gPraiseSounds[theLevel];
	mPraiseUpdate = mUpdateCnt;
	mLastPraiseLevel = theLevel;
	int anUp = Ftol((double)gApp->mScale * 250.0);
	int aY = GetRowY(0) + (mRows * gGemHeight / 2 - anUp);
	int aX = GetColX(0) + mCols * gGemWidth / 2;
	FlyingText* aText =
		new FlyingText(0, GetImageById(gPraiseImages[theLevel]), aX, aY, GetSoundById(aSound), 0.0f);
	aText->mQueued = false;
	mFlyingTexts.push_back(aText);
	// (the original also notes the update count for each praise, 0x85a9d0, which nothing reads)
}

void Board::ShowMenu()
{
	// 0x6a6983
	if (mRewindSound != nullptr) {
		mRewindSound->Release();
		mRewindSound = nullptr;
	}
	mApp->ShowOptionsDialog();
}

void Board::ShowZenJump(int theMode)
{
	// 0x6a615b: once for each player (the profile's flag 0x11); the mode select it puts up is the board's dialog
	mApp->PlaySample(SOUND_MENU_SLIDE);
	mApp->mProfile->SetTipSeen(0x11, true);
	mApp->mProfile->Save();
	mCursorShown = false;
	mApp->ShowModeSelect(theMode);
	mShownDialog = static_cast<TwistWidget*>(mApp->mMandalaWidget);
}

void Board::AskQuit()
{
	// 0x6b38f3: the quit question, by the game's mode; the board hears the answer
	SexyString aHeader;
	SexyString aLines;
	if (mGameMode == MODE_CHALLENGE) {
		if (!IsTimed()) {
			aHeader = mApp->GetString(GTS(0x7dc6a0), GT(0x7dc680));
			aLines = mApp->GetString(GTS(0x7dc638), GT(0x7dc5a8));
		} else {
			aHeader = mApp->GetString(GTS(0x7dc794), GT(0x7dc788));
			aLines = mApp->GetString(GTS(0x7dc740), GT(0x7dc6b0));
		}
	} else if (mGameMode == MODE_BLITZ) {
		aHeader = mApp->GetString(GTS(0x7da148), GT(0x7da12c));
		aLines = mApp->GetString(GTS(0x7dc820), GT(0x7dc7a0));
	} else if (mGameMode == MODE_ZEN) {
		aHeader = mApp->GetString(GTS(0x7dc93c), GT(0x7dc920));
		aLines = mApp->GetString(GTS(0x7dc8e0), GT(0x7dc860));
	} else {
		aHeader = mApp->GetString(GTS(0x7da148), GT(0x7da12c));
		aLines = mApp->GetString(GTS(0x7dc9c8), GT(0x7dc950));
	}
	TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, 1, true, aHeader, aLines,
		GT(0x80d9a4), 3);
	aDialog->mTwistListener = this;
	double aMinWidth = (double)gGemWidth * 6.75;
	double aWidth = (double)aDialog->GetPreferredWidth() <= aMinWidth ? aMinWidth : (double)aDialog->GetPreferredWidth();
	aDialog->mWidth = Ftol(aWidth);
	aDialog->mHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
	aDialog->mZOrder = 200;
	mApp->AddDialog(aDialog);
	aDialog->mX = mWidth / 2 - aDialog->mWidth / 2;
	aDialog->mY = mHeight / 2 - aDialog->mHeight / 2;
	aDialog->mYesButton->mLabel = mApp->GetString(GTS(0x812e14), GT(0x7db674));
	aDialog->mNoButton->mLabel = mApp->GetString(GTS(0x812e10), GT(0x7db66c));
}

void Board::DialogButtonDepress(int theDialogId, int theButtonId)
{
	// 0x6c8632
	if (theDialogId == 1) {
		mApp->KillDialog(1);
		if (!mApp->mLoadingThreadCompleted) {
			if (mTutorial != nullptr)  // (the original does not look)
				mTutorial->mState = 0x10;
		} else {
			Tutorial* aTip = mTutorial;
			if (aTip != nullptr) {
				if (mShownDialog == aTip)
					mShownDialog = nullptr;
				if (aTip->mKind == 0xe) {
					mApp->mProfile->MarkTutorialDone();
					mUnlockFlags = mApp->mProfile->mGemTypesUnlocked;
					mSeenFlags = mApp->mProfile->mTipsSeen;
				}
				aTip->mWidgetManager->RemoveWidget(aTip);
				mTipSlide.SetInVal(0.0);
				mApp->SafeDeleteWidget(aTip);
				mTutorial = nullptr;
			}
			if (theButtonId == 1000) {
				if (mApp->mOptionsDialog != nullptr)
					mApp->FinishOptionsDialog(false);
				if (mGameMode == MODE_ZEN)
					UpdateStats();
				mEndPending = 1;
				if (mApp->mMusicInterface != nullptr) {
					// port: the original keeps BASS's position in the music playing (0x6ee282) here (+0x37dc); nothing
					// reads it back, so the port keeps 0 (as in RemovedFromManager)
				}
			}
		}
	} else if (theDialogId == 0x28) {
		if (theButtonId == 4) {
			ShowMenu();
		} else if (theButtonId == 0x3e9) {
			if (!mTipShowing) {
				mApp->mProfile->mBeforeFirstMove = false;
				mApp->mProfile->Save();
			} else {
				CloseTip();
			}
		}
	}
}
