// The player's twist: the mouse over the board, the click, and the four gems turning about the cursor
#include "Board.h"

#include "Buffer.h"
#include "Challenge.h"
#include "CustomBassMusicInterface.h"
#include "GoodTube.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "GameText.h"
#include "GraphicsRecorder.h"
#include "Piece.h"
#include "Profile.h"
#include "ReplayWidget.h"
#include "Res.h"
#include "TwistButton.h"
#include "TwistUtil.h"
#include "WheelSpin.h"
#include "WidgetManager.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

void Board::MouseMove(int x, int y)
{
	// Under -autoplay the mouse leaves the cursor alone
	if (mApp->mAutoPlayMode < 0)
		UpdateCursor(x, y);
}

void Board::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

void Board::MouseUp(int x, int y, int theClickCount)
{
	mClickedThisUpdate = false;
}

void Board::MouseWheel(int theDelta)
{
}

// Whenever the board loses the focus (to a dialog or a panel over it, or the app's), Blitz's music (tracks 300 and
// 308) pauses, and it resumes when the board has it back unless the board is paused more than once. The skip flag set
// here is only ever cleared again (MouseDown).
void Board::LostFocus()
{
	Widget::LostFocus();
	// The original says "BOARD:  LOST FOCUS()" with OutputDebugString
	mSkipMouseClick = true;
	mWantsFocus = true;
	if (mApp->mTwistMusic != nullptr && (mApp->mTwistMusic->mCurTrack == 300 || mApp->mTwistMusic->mCurTrack == 308))
		mApp->mTwistMusic->PauseAllMusic();
}

void Board::GotFocus()
{
	if (mPauseCount < 2 && mApp->mTwistMusic != nullptr &&
		(mApp->mTwistMusic->mCurTrack == 300 || mApp->mTwistMusic->mCurTrack == 308))
		mApp->mTwistMusic->ResumeAllMusic();
	// The original says "BOARD:  GOT FOCUS()" with OutputDebugString
	Widget::GotFocus();
}

// The last 24 letters typed are kept while the mouse is off the grid (any key typed over it clears them). SNACKERS
// switches coal to its snackers look and back; NORMAL stops the rain effect and sets the game's speed back to 1. Either
// word clears the letters. (With +0x1bf1 set, never in the original, NORMAL would ask the app for a change of screen
// mode instead; that branch is left out.)
void Board::KeyChar(SexyChar theChar)
{
	mTypedKeys.append(1, (SexyChar)(toupper((unsigned char)theChar) & 0xff));
	if (0x18 < mTypedKeys.size())
		mTypedKeys.erase(mTypedKeys.begin());
	if (GetRowAt(mBoardMouseY) < 0 || GetColAt(mBoardMouseX) < 0) {
		if (mTypedKeys.find("NORMAL") != SexyString::npos) {
			mRainEffect = false;
			mSpeedTarget = 1.0f;
			mSpeedNow = 1.0f;
		} else if (mTypedKeys.find("SNACKERS") != SexyString::npos) {
			mCoalAsSnackers = !mCoalAsSnackers;
		} else {
			return;
		}
	}
	mTypedKeys.clear();
}

void Board::WarpToCursor()
{
	// SetCursorPos: the original's ClientToScreen of the cursor's middle (+0x1c00 and its size +0x1c08)
	if (mCursorPiece != nullptr)
		mApp->WarpMouse(mCursorWidth / 2 + mCursorX, mCursorHeight / 2 + mCursorY);
}

void Board::KeyDown(KeyCode theKey)
{
	if (mApp->mDialogMap.size() != 0)
		return;
	if (theKey == KEYCODE_PAUSE || theKey == KEYCODE_ESCAPE) {
		if (!IsBusy() && !mIsReplay && !mReplayPlaying && mPlayState == 0 && mShownDialog == nullptr) {
			if (mPauseCount == 0)
				Pause(true);
			else
				Unpause(true);
		}
	} else {
		bool aPlaying = !mIsReplay && !mReplayPlaying;
		if (-1 < mApp->mAutoPlayMode)
			aPlaying = false;
		// Enter or space stops the wheel
		if (aPlaying && (theKey == KEYCODE_RETURN || theKey == KEYCODE_SPACE) && gWheelSpin != nullptr &&
			!gWheelSpin->mButton->mDisabled)
			static_cast<ButtonListener*>(gWheelSpin)->ButtonDepress(0);
		if (aPlaying && mCursorShown) {
			int aY = mCursorY;
			int aRow = GetRowAt(aY);
			int aCol = GetColAt(mCursorX);
			if (theKey == KEYCODE_RETURN || theKey == KEYCODE_SPACE) {
				// A click on the cursor's gem
				MouseDown(gGemWidth / 2 + mCursorX, gGemHeight / 2 + aY, 0, 1);
			} else {
				// The arrows move the cursor, unless the opposite one is down too
				aY = aRow;
				if (theKey == KEYCODE_LEFT) {
					if (aCol < 1 || mTwisting || mWidgetManager->mKeyDown[KEYCODE_RIGHT])
						goto done;
					aCol--;
				} else if (theKey == KEYCODE_UP) {
					if (aRow < 1 || mTwisting || mWidgetManager->mKeyDown[KEYCODE_DOWN])
						goto done;
					aY = aRow - 1;
				} else if (theKey == KEYCODE_RIGHT) {
					if (mCols - 2 <= aCol || mTwisting || mWidgetManager->mKeyDown[KEYCODE_LEFT])
						goto done;
					aCol++;
				} else {
					if (theKey != KEYCODE_DOWN)
						goto debug;
					if (mRows - 2 <= aRow || mTwisting || mWidgetManager->mKeyDown[KEYCODE_UP])
						goto done;
					aY = aRow + 1;
				}
				MoveCursor(aY, aCol, false, nullptr);
				WarpToCursor();
				goto done;
			}
		}
	}
debug:
	// Ctrl and a digit loads debugN.sav (the original's test saves)
	if ((unsigned int)(theKey - KEYCODE_ASCIIBEGIN) < 10 && mWidgetManager->mKeyDown[KEYCODE_CONTROL]) {
		mApp->mScoresLoaded = false;
		Buffer aBuffer;
		bool aRead = mApp->ReadBufferFromFile(StrFormat("debug%d.sav", theKey - KEYCODE_ASCIIBEGIN), &aBuffer, true);
		if (aRead) {
			LoadGame(aBuffer);
			mGameLoaded = false;
			SetScore(mScore, false);
		}
		mTurnOfLastSound = mTurnCount - 1;
		mApp->mScoresLoaded = true;
	}
done:
	Widget::KeyDown(theKey);
}

void Board::UpdateCursor(int x, int y)
{
	if (0 < mPauseCount || IsBusy() || mReplayPlaying || mPlayState == 1 || mIsReplay)
		return;
	if (mShownDialog != nullptr && (Widget*)mShownDialog != mTutorial && mShownDialog->mMouseVisible)
		return;
	for (int i = 0; i < (int)mPendingDialogs.size(); i++) {
		if (mPendingDialogs[i]->mMouseVisible)
			return;
	}
	mBoardMouseX = x;
	mBoardMouseY = y;
	int anOldCol = GetColAt(mCursorX);
	int anOldRow = GetRowAt(mCursorY);
	int aRow = GetRowAt(y);
	int aCol = GetColAt(x);
	bool aTwisting = mTwisting;
	if (!aTwisting && (mMatchAtOnce || mMatchDelay == 0)) {
		int aDX = x - mCursorWidth / 2 - mCursorX;
		int aDY = y - mCursorHeight / 2 - mCursorY;
		int aFar = std::abs(std::abs(aDY) < std::abs(aDX) ? aDX : aDY);
		int aFar2 = std::abs(std::abs(aDY) < std::abs(aDX) ? aDX : aDY);
		int aGemHeight = gGemHeight;
		// The cursor follows the mouse once it is far enough from the cursor's centre
		float aDist = (float)((double)(aDY * aDY + aDX * aDX) * 0.30000001192092896 +
			(double)aFar * 0.699999988079071 * (double)aFar2);
		float aNear = (float)((double)(aGemHeight * aGemHeight) * 0.699999988079071 * 0.699999988079071);
		if (aNear < aDist) {
			aRow = GetRowAt(y - gGemWidth / 2);
			aCol = GetColAt(x - aGemHeight / 2);
			if (aCol < 0)
				aCol++;
			if (aRow < 0)
				aRow++;
			if (-1 < aCol && aCol < mCols && -1 < aRow && aRow < mRows) {
				if (mCols - 2 < aCol)
					aCol = mCols - 2;
				if (mRows - 2 < aRow)
					aRow = mRows - 2;
				MoveCursor(aRow, aCol, false, nullptr);
			}
		}
	} else if (aTwisting) {
		mTwistMatches = false;
	}
	if (mInputLock == 0 && !aTwisting && (anOldCol != aCol || anOldRow != aRow) && !mClicksIgnored) {
		ClearSparkStarts();
		mCursorMoveGlow = 1.0f;
	}
}

void Board::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	UpdateCursor(x, y);
	if (mGameMode == MODE_PUZZLE && (mPuzzleSolved || mEditorMode))
		return;
	if (mGeodesPending && !mGeodes.empty())
		return;
	if (mShownDialog != nullptr && mShownDialog->mMouseVisible && (Widget*)mShownDialog != mTutorial)
		return;
	for (int i = 0; i < (int)mPendingDialogs.size(); i++) {
		if (mPendingDialogs[i]->mMouseVisible)
			return;
	}
	if (mTutorial != nullptr) {
		// (not while the tutorial waits on its dialog or its seconds, or on a forced move with no square to make it)
		if (mTutorial->mWaitsForPlayer)
			return;
		int aState = mTutorial->mState;
		if (aState == Tutorial::STATE_SECONDS || (aState == Tutorial::STATE_MOVE && mTipCol == -1))
			return;
	}
	if (mApp->mOptionsDialog != nullptr)
		return;
	if (mPauseCount != 0) {
		// a click on the paused board resumes the game
		Unpause(true);
		return;
	}
	if (mIsReplay || mReplayPlaying || mShownBeforeStart || !mElectroStrikes.empty() || mLevelDone || !mCursorShown)
		return;
	bool aQueue;
	if (!mHintShowing) {
		if (mSkipMouseClick) {
			// The original says "BOARD:  mSkipMouseClick cleared" with OutputDebugString
			mSkipMouseClick = false;
		}
		if (0 < mClickBlockUpdates)
			return;
		mClickedThisUpdate = true;
		if (mClicksIgnored)
			return;
		if (mHintsReady) {
			ClearSparkStarts();
			mClickCursorX = mCursorX;
			mClickCursorY = mCursorY;
			mClickCursorWidth = mCursorWidth;
			mClickCursorHeight = mCursorHeight;
			if (mCursorAlpha < 0.5f)
				return;
			if (0 < mInputLock)
				return;
			mLastClickX = x;
			mLastClickY = y;
			if (mTwisting)
				return;
			if (mInputLock != 0)
				return;
			mTwistClockwise = false;
			if (!mHintsReady || (mTwistBlocked && (!mPuzzleBoardMade || !mTwistOverEmpty))) {
				// A twist the locked gems block: they shake, and the cursor turns a little and back
				if (mTipCol == -1) {
					bool aBlocked = false;
					if (mUnturnableCells.size() != 0) {
						aBlocked = true;
						for (unsigned int i = 0; i < mUnturnableCells.size(); i++) {
							Piece* aPiece = GetPieceAt(mUnturnableCells[i].first, mUnturnableCells[i].second);
							if (aPiece != nullptr)
								aPiece->mLockShake = 1;
						}
					}
					SetupTwist(-1, -1);
					mTwistStep = 3.0f;
					mTwisting = true;
					if (!mLevelStarted || aBlocked)
						mApp->PlaySample(SOUND_BADMOVE);
				}
			} else {
				StartTwist(false, -1, -1);
			}
			if (mApp->GetCurrentMusic() != 305)
				return;
			UpdateTurnBasedMusic();
			return;
		}
		aQueue = !mTwisting;
	} else {
		aQueue = theBtnNum == 0;
	}
	// A click while the board is not ready waits (UpdateGame takes it within 20 updates)
	if (aQueue) {
		mAutoClickX = x;
		mAutoClickY = y;
		mAutoClickTime = mUpdateCnt;
	}
}

void Board::StartTwist(bool theInstant, int theX, int theY)
{
	if (mShownBeforeStart && !mIsReplay)
		return;
	if (mChallenge != 0 && (mChallengeOver || mChallengeMet))
		return;
	if (mTutorial != nullptr && !mTutorial->mMovesAllowed)
		return;
	mBeforeFirstTwist = false;
	mReplayPodDue = false;
	if (mTipSlide.GetOutVal() != 0.0) {
		float anIn = (float)mTipSlide.GetInVal();
		mTipSlide.SetCurve(GTS(0x7dd5e8));
		mTipSlide.SetInVal(1.0 - (double)anIn);
	}
	mCountersAllowed = true;
	mBonusMatchedMark = mBonusMatched;
	std::memset(mGameStats, 0, sizeof(mGameStats));  // 0x6a69f0
	mTurnRating = 0;
	mTurnMatches.clear();
	if (!mReplayPlaying) {
		mSaveBuffer.Clear();
		mReplaySave.Clear();
	}
	mGeodeBonusCount = 0;
	if (!mIsReplay && !mReplayPlaying) {
		// The game before the twist, for the undo of a turn-based challenge and the tutorials
		SaveGame(&mSaveBuffer, false);
		mReplayStartFrame = GetRowAt(mCursorY);
		mReplayEndFrame = GetColAt(mCursorX);
		mSavedRank = mApp->mProfile->mRank;
		mSavedRankPoints = mApp->mProfile->mRankPoints;
	}
	if (mReplayPod != nullptr) {
		mReplayPod->mState = ReplayWidget::STATE_GOING;
		mReplayPod->mPopped = false;
	}
	mTwistsMade++;
	mHintWait = 0.0f;
	mTurnStartedWithMatch = mTwistMatches;
	mGemsBrokenAtTurnStart = mGemsBroken;
	mGemsClearedAtTurnStart = mGemsCleared;
	mScoreAtTurnStart = mScore;
	mHintState = 0;
	mHintsShown = 0;
	mBurstSize = 25;
	mBlastChainCount = 0;
	mTurnPoints = 0;
	mLastPraiseLevel = -1;
	mUpdatesSinceCascade = 0;
	mColorGemBroke = false;
	mFirstMatchColor = -1;
	mChainLength = 0;
	mFruitBrokeThisTurn = false;
	mBombsMayCountDown = true;
	mGoalCheckDue = true;
	mLevelGemCount = 0;
	mProgressGainAtTurnStart = mProgressGain;
	mTipRow = -1;
	mTipCol = -1;
	mMatchCheckDue = true;
	mChainAdded = false;
	if (mGameMode == MODE_CHALLENGE && mChallenge != 0) {
		CheckGoal(false);
	}
	if (!theInstant) {
		double aX;
		if (mCursorPiece != nullptr) {
			aX = (double)mCursorPiece->mX + (double)gGemWidth;
		} else {
			Board* aBoard = gBoard;
			aX = (double)(aBoard->GetColX(0) + (aBoard->mCols * gGemWidth) / 2);
		}
		// Lower when the twist makes no match
		float aPitch = (float)(mTwistMatches ? 0 : -5);
		mApp->PlaySoundEx(SOUND_START_ROTATE, GetPan(Ftol(aX)), aPitch, 0.0f, true, true);
	}
	SetupTwist(theX, theY);
	if (theInstant) {
		FinishTwist(true);
	} else {
		mTwisting = true;
		float aSpeed;
		if (0.0f <= mSpeedNow)
			aSpeed = (float)(((double)mTwistSpeedGrade * 0.03 + 0.20999999344348907) * (double)mStepLevel +
				3.0999999046325684);
		else
			aSpeed = 1.0f;
		mTwistStep = mTwistSpeedScale * aSpeed;
	}
	CountDownBombs(theInstant);
	if (mSparkBlock == 0) {
		int aCount = mCols * mRows;
		Piece** aCell = mGrid;
		while (aCount != 0) {
			Piece* aPiece = *aCell;
			aCell++;
			aCount--;
			if (aPiece != nullptr && 0 < aPiece->mLockState && !aPiece->mIsTwisting) {
				aPiece->mLockState--;
				if (aPiece->mLockState < 1)
					LockPiece(aPiece);
			}
		}
	}
	NextTurnGems();  // 0x6c7225: the lock and doom gems, in every mode
}

namespace {

// 0x694387: theValue to the power thePower, by squaring, as a float
float PowInt(float theValue, int thePower)
{
	unsigned int aPower = (unsigned int)thePower;
	if (thePower < 0)
		aPower = (unsigned int)-thePower;
	float aResult = 1.0f;
	while (true) {
		if ((aPower & 1) != 0)
			aResult = aResult * theValue;
		aPower = aPower >> 1;
		if (aPower == 0)
			break;
		theValue = theValue * theValue;
	}
	if (thePower < 0)
		aResult = 1.0f / aResult;
	return aResult;
}

}  // namespace

float Board::RandUnit()
{
	int aRoll = BoardRand() % 10000 - 5000;
	return (float)((double)aRoll / 5000.0);
}

void Board::PreLockPiece(Piece* thePiece)
{
	thePiece->mLockTick = 0;
	thePiece->mLockState = 2;
	PlayPieceSound(thePiece, SOUND_GEM_PRELOCK, 0.0f, 0.0f, true);
}

// 0x6afdee: a piece record reset, inline here
// 0x6b97b6: the piece record constructor taking the next serial, inline here
void Board::NextTurnGems()
{
	// the gems in locks now
	int aLocked = 0;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr && aPiece->IsFlagSet(Piece::FLAG_LOCKED))
				aLocked++;
		}
	}
	if (mGameMode == MODE_PUZZLE || mSparkBlock != 0)
		return;
	// (the original writes "mLockGemTimingInfo.NextTurn();" to the debugger)
	mLockGem.NextTurn();
	if (!CanAddCounter())
		return;
	// (and "Freq: %d Timer: %d" with mLockGem's count and value)
	if (aLocked < mMaxLocksOnScreen && mLockGem.mIsDue) {
		int aTotal = mCols * mRows;
		Piece* aLock = nullptr;
		if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr &&
			(mChallenge->mFlags & Challenge::FLAG_CONSTANTCOUNTERS) != 0 && 0 < mRows) {
			// Constant counters: a gem next to the bomb or devil gem with the lowest count takes the lock
			Piece* aBomb = nullptr;
			int aLowest = 1000;
			for (int aRow = 0; aRow < mRows; aRow++) {
				for (int aCol = 0; aCol < mCols; aCol++) {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr && (aPiece->IsFlagSet(Piece::FLAG_BOMB) || aPiece->IsFlagSet(Piece::FLAG_DEVIL)) &&
						aPiece->mBombCount < aLowest) {
						aBomb = aPiece;
						aLowest = aPiece->mBombCount;
					}
				}
			}
			if (aBomb != nullptr) {
				std::vector<int> aDirs;
				int aDir = 0;
				do {
					aDirs.push_back(aDir);
					aDir++;
				} while (aDir < 8);
				// Each try takes its direction out of the list and then steps on, so only the directions 0, 2, 4
				// and 6 (right, down, left, up) are ever tried, in that order
				unsigned int i = 0;
				while (i < aDirs.size()) {
					int aTry = aDirs[i];
					aDirs.erase(aDirs.begin() + i);
					int aDRow = GameInt(GTB(0x84e978, 64) + aTry * 8);
					int aDCol = GameInt(GTB(0x84e978, 64) + aTry * 8 + 4);
					Piece* aNext = GetPieceAt(aDRow + aBomb->mRow, aDCol + aBomb->mCol);
					if (aNext != nullptr && aNext->mFlags == 0 && !aNext->mBreaking) {
						aLock = aNext;
						break;
					}
					i++;
				}
			}
		}
		if (aLock == nullptr) {
			// A gem from a random place on, or (on an odd roll) inside the border, the first that can take a lock
			int aIndex;
			if (BoardRand() % 2 == 0) {
				aIndex = BoardRand() % aTotal;
			} else {
				int aRowPick = BoardRand() % (mRows - 2);
				int aColPick = BoardRand() % (mCols - 2);
				aIndex = aColPick + (aRowPick + 1) * mCols + 1;
			}
			int aLeft = aTotal;
			if (aTotal != 0) {
				Piece** aCell = &mGrid[aIndex];
				do {
					Piece* aPiece = *aCell;
					aLeft--;
					if (aPiece != nullptr && !aPiece->mBlasted && !aPiece->mBreaking && aPiece->mColor <= 6 &&
						aPiece->mFlags == 0) {
						aLock = aPiece;
						break;
					}
					aIndex++;
					aCell++;
					if (aTotal <= aIndex) {
						aIndex -= aTotal;
						aCell -= aTotal;
					}
				} while (aLeft != 0);
			}
		}
		if (aLock != nullptr) {
			PreLockPiece(aLock);
			mLockGem.Restart();
		}
	}
	// A doom gem: while there is none, when the tube is full enough by chance (not in Zen, nor early on a level whose
	// matches are not yet three times its moves); a constant-doom challenge always has one
	bool anAppear;
	if (!mDevilChecked && 0 <= mDevil.GetCount() && !IsBusy() && mGameMode != MODE_ZEN) {
		anAppear = true;
		if (0 < mGameStats[0x21] && (float)((double)mGameStats[0x39] / (double)mGameStats[0x21]) < 3.0f)
			anAppear = false;
		float aProgress = mGoodTube->mProgress;
		float aRand = RandUnit();
		float aScaled = (float)((double)aProgress * 1.2000000476837158);
		double aPow = (double)PowInt(aScaled, 4);
		float aChance = (float)((double)fabsf(aRand) * aPow);
		if ((double)aChance < 0.05000000074505806)
			anAppear = false;
	} else {
		anAppear = false;
	}
	if (mGameMode == MODE_CHALLENGE && mChallenge != nullptr &&
		(mChallenge->mFlags & Challenge::FLAG_CONSTANTDOOM) != 0) {
		mDevilChecked = false;
		anAppear = true;
		int i = 0;
		Piece** aCell = mGrid;
		if (0 < mCols * mRows) {
			do {
				Piece* aPiece = *aCell;
				if (aPiece != nullptr && (aPiece->IsFlagSet(Piece::FLAG_DEVIL) || aPiece->IsFlagSet(Piece::FLAG_DOOM)))
					anAppear = false;
				i++;
				aCell++;
			} while (i < mCols * mRows);
		}
	}
	if (!anAppear)
		return;
	// A gem inside the border that no move on the board would match becomes the doom gem (the original tries until
	// one does, then counts its hundred tries down)
	int aCursorX = mCursorX;
	int aCursorY = mCursorY;
	int aTries = 100;
	// port: when no gem inside the border can take it (none plain, or every plain one in some move's line) the
	// original's search never ends and the game freezes (NOTES.md, "Deviations"). The gems some move would match are
	// the same for every pick, so the first sweep tells whether any pick can succeed; if none can, no doom gem comes
	// this turn. Where one can, the picks, the random draws and the sweeps are the original's.
	auto aCanTake = [](Piece* thePiece) {
		return thePiece != nullptr && !thePiece->mBlasted && !thePiece->mBreaking && thePiece->mColor <= 6 &&
			thePiece->mFlags == 0;
	};
	bool anyPlain = false;
	for (int aRow = 1; aRow < mRows - 1; aRow++)
		for (int aCol = 1; aCol < mCols - 1; aCol++)
			anyPlain = anyPlain || aCanTake(mGrid[aCol + aRow * mCols]);
	if (!anyPlain)
		return;
	bool aSwept = false;
	do {
		while (!mDevilChecked) {
			int aRowPick = BoardRand() % (mRows - 2);
			int aColPick = BoardRand() % (mCols - 2);
			Piece* aPiece = mGrid[aColPick + (aRowPick + 1) * mCols + 1];
			if (aPiece == nullptr || aPiece->mBlasted || aPiece->mBreaking || 6 < aPiece->mColor || aPiece->mFlags != 0)
				continue;
			Piece* aSaved = mCursorPiece;
			bool aMatched = false;
			std::vector<int> aLineIds;  // port: every gem some move matches (see above)
			MatchInfo anInfo;  // 0x6b97b6
			int aRow = 0;
			do {
				int aCol = 0;
				do {
					anInfo.Reset();  // 0x6afdee
					MoveCursor(aRow, aCol, true, &anInfo);
					if (!mTwistBlocked && mTwistMatches) {
						for (unsigned int j = 0; !anInfo.mIds.empty() && j < anInfo.mIds.size(); j++) {
							if (anInfo.mIds[j] == aPiece->mId)
								aMatched = true;
							if (!aSwept)
								aLineIds.push_back(anInfo.mIds[j]);
						}
					}
					aCol++;
				} while (aCol < 6);
				aRow++;
			} while (aRow < 8);
			if (!aMatched) {
				mCursorPiece = aSaved;
				PreLockPiece(aPiece);
				aPiece->SetFlag(Piece::FLAG_DOOM, true);
				mApp->PlaySample(SOUND_DOOMGEM_APPEARS);
				mDevilChecked = true;
			} else if (!aSwept) {
				aSwept = true;
				bool aFree = false;
				for (int aRow = 1; aRow < mRows - 1 && !aFree; aRow++) {
					for (int aCol = 1; aCol < mCols - 1 && !aFree; aCol++) {
						Piece* aCell = mGrid[aCol + aRow * mCols];
						aFree = aCanTake(aCell) &&
							std::find(aLineIds.begin(), aLineIds.end(), aCell->mId) == aLineIds.end();
					}
				}
				if (!aFree) {
					aTries = 1;
					break;
				}
			}
		}
		aTries--;
	} while (aTries != 0);
	MoveCursor(GetRowAt(aCursorY), GetColAt(aCursorX), true, nullptr);
}

void Board::SetupTwist(int theX, int theY)
{
	mTwistPieces.clear();
	if (theX < 0)
		theX = mCursorX;
	if (theY < 0)
		theY = mCursorY;
	// The twist's centre
	int aCenterX = gGemWidth + theX;
	int aCenterY = gGemHeight + theY;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				continue;
			aPiece->mRingAngle = -1.0f;
			aPiece->mRingDelay = 0.0f;
			aPiece->mLastCol = aPiece->mCol;
			aPiece->mLastRow = aPiece->mRow;
			aPiece->mBurstRow = aPiece->mRow;
			aPiece->mHasBroken = false;
			aPiece->mPendingKind = 0;
			// The four gems half a gem from the centre turn
			int aDX = std::abs(-(gGemWidth / 2) - Ftol(aPiece->mX) + aCenterX);
			int aDY = std::abs(-(gGemHeight / 2) - Ftol(aPiece->mY) + aCenterY);
			if ((aDX == gGemWidth / 2 && aDY <= gGemHeight / 2) || (aDY == gGemHeight / 2 && aDX <= gGemWidth / 2)) {
				mTwistPieces.push_back(aPiece);
				aPiece->mIsTwistCorner = true;
			} else {
				aPiece->mIsTwistCorner = false;
			}
		}
	}
	// Sorted by their quarter about the centre
	float aQuarter = (float)((double)6.2831854820251465f * 0.25);
	std::vector<Piece*> aByQuarter;
	for (int i = 4; i != 0; i--)
		aByQuarter.push_back(nullptr);
	for (unsigned int i = 0; i < mTwistPieces.size(); i++) {
		Piece* aPiece = mTwistPieces[i];
		int aDX = Ftol(aPiece->mX) + gGemWidth / 2 - aCenterX;
		int aDY = Ftol(aPiece->mY) + gGemHeight / 2 - aCenterY;
		float anAngle = (float)std::atan2((double)aDY, (double)aDX);
		if (anAngle < 0.0f)
			anAngle = 6.2831854820251465f + anAngle;
		aByQuarter[Ftol((double)anAngle / (double)aQuarter)] = aPiece;
	}
	int aCount = 0;
	for (unsigned int i = 0; i < aByQuarter.size(); i++) {
		if (aByQuarter[i] != nullptr) {
			mTwistPieces[aCount] = aByQuarter[i];
			aCount++;
		}
	}
	// Each gem moves to the next one's cell: clockwise, or the other way with +0x2015
	int aSize = (int)mTwistPieces.size();
	for (int i = 0; i < aSize; i++) {
		int aNext = i + 1;
		if (mTwistClockwise)
			aNext -= 2;
		if (aNext < 0)
			aNext += (int)mTwistPieces.size();
		else if (aNext == (int)mTwistPieces.size())
			aNext = 0;
		Piece* aFrom = mTwistPieces[i];
		Piece* aTo = mTwistPieces[aNext];
		StartPieceMove(mTwistPieces[i], aTo->mCol - aFrom->mCol, aTo->mRow - aFrom->mRow);
	}
}

bool Board::StartPieceMove(Piece* thePiece, int theDCol, int theDRow)
{
	ClearCellHints();
	mCounterHintDue = false;
	mRecordedFrames = 0;
	// The move's frames are recorded afresh
	gGraphicsRecorder->PopFramesFrom(0);
	if ((theDCol == 0 && theDRow == 0) || thePiece == nullptr)
		return false;
	int aCol = thePiece->mCol + theDCol;
	int aRow = thePiece->mRow + theDRow;
	if (aCol < 0 || mCols <= aCol || aRow < 0 || mRows <= aRow)
		return false;
	mTwistTried = true;
	thePiece->mFruitTimer = 0;
	thePiece->mIsTwisting = true;
	int aToX = GetColX(thePiece->mCol + theDCol);
	int aToY = GetRowY(thePiece->mRow + theDRow);
	int aCenterX = mCursorX + gGemWidth / 2;
	int aCenterY = mCursorY + gGemHeight / 2;
	int aX = Ftol(thePiece->mX);
	thePiece->mTwistStartX = aX;
	int aY = Ftol(thePiece->mY);
	thePiece->mTwistStartY = aY;
	thePiece->mTwistOffsetX = Ftol((double)aX - (double)aCenterX);
	thePiece->mTwistOffsetY = Ftol((double)aY - (double)aCenterY);
	thePiece->mTwistDCol = theDCol;
	thePiece->mTwistDRow = theDRow;
	thePiece->mTwistMoveX = Ftol((double)aToX - (double)thePiece->mX);
	thePiece->mTwistMoveY = Ftol((double)aToY - (double)thePiece->mY);
	return true;
}

void Board::FinishTwist(bool theStop)
{
	if (mTutorial != nullptr)
		mTutorial->PointAtNothing();
	std::vector<Piece*> aTurned;
	Piece** aCell = mGrid;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && aPiece->mIsTwisting) {
				if (theStop) {
					aPiece->mIsTwisting = false;
					aPiece->mTurnAmount = 0.0f;
				}
				aTurned.push_back(aPiece);
				*aCell = nullptr;
			}
			aCell++;
		}
	}
	if (theStop)
		mTwisting = false;
	// The turned gems in their new cells
	for (unsigned int i = 0; i < aTurned.size(); i++) {
		Piece* aPiece = aTurned[i];
		aPiece->mCol += aPiece->mTwistDCol;
		aPiece->mRow += aPiece->mTwistDRow;
		int aRow = aPiece->mRow;
		aPiece->mBurstRow = aRow;
		aPiece->mX = (float)GetColX(aPiece->mCol);
		aPiece->mY = (float)GetRowY(aRow);
		mGrid[mCols * aRow + aPiece->mCol] = aPiece;
	}
	if (mGameMode == MODE_PUZZLE)
		mTwistGlow = 1.0f;
}

void Board::CountDownBombs(bool theInstant)
{
	if (!mBombsMayCountDown || IsSparkBlocked() || (mGameMode == MODE_PUZZLE && mPuzzleSolved) || IsBusy())
		return;
	Piece** aCell = mGrid;
	mLowestCounter = mCounter.GetValue();
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && 0 < aPiece->mBombCount) {
				int aLeft;
				if (theInstant) {
					aLeft = aPiece->mBombCount - 1;
					aPiece->mBombCount = aLeft;
				} else if (mTwistMatches && aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
					// A devil gem that this twist's match may clear keeps its count
					aCell++;
					continue;
				} else {
					// The counter pulses; the count itself goes down as the twist ends
					aPiece->mCounterFlash = 1.0f;
					aPiece->mCounterSparks = 0.33f;
					aLeft = aPiece->mBombCount - 1;
				}
				// The lowest count left on the board
				int aLow = aLeft < 0 ? 0 : aLeft;
				if (aLow < mLowestCounter)
					mLowestCounter = aLeft < 0 ? 0 : aLeft;
			}
			aCell++;
		}
	}
	mCounterFlash = 255;
	mBombsMayCountDown = false;
}

void Board::LockPiece(Piece* thePiece)
{
	bool aKept = thePiece->IsFlagSet(Piece::FLAG_DOOM);
	thePiece->ClearSpecial();
	thePiece->SetFlag(Piece::FLAG_LOCKED, true);
	if (aKept)
		thePiece->SetFlag(Piece::FLAG_DOOM, true);
	thePiece->mLockState = 0;
	thePiece->mLockTick = 0;
	thePiece->mLockCount = 1;
	thePiece->mColor = thePiece->mColor % 7;
	mApp->PlaySample(SOUND_GEM_LOCK_START);
	AddStat(0x1b, 1);
}

void Board::UnturnPieces()
{
	Piece** aCell = mGrid;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && aPiece->mIsTwisting) {
				aPiece->mX = (float)GetColX(aCol);
				aPiece->mIsTwisting = false;
				aPiece->mY = (float)GetRowY(aRow);
			}
			aCell++;
		}
	}
}

void Board::ClearTurning()
{
	Piece** aCell = mGrid;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = *aCell;
			aCell++;
			if (aPiece != nullptr && aPiece->mIsTwisting)
				aPiece->mIsTwisting = false;
		}
	}
}

void Board::UpdateTwist()
{
	if (mShownBeforeStart)
		return;
	// A gem that cannot move sends the twist back
	bool aStuck = false;
	Piece** aCell = mGrid;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			if (*aCell != nullptr && (*aCell)->mIsTwisting && !CanMove(*aCell))
				aStuck = true;
			aCell++;
		}
	}
	MarkDirty();
	float aFade = (float)((double)mCursorTwistFade + 0.05000000074505806);
	mHintsReady = false;
	if (1.0f < aFade)
		aFade = 1.0f;
	mCursorTwistFade = aFade;
	float anOld = mTwistAngle;
	float aStep = mInstantTwist ? 90.0f : mTwistStep;
	float anAngle = mTwistAngle + aStep;
	mTwistAngle = anAngle;
	if (90.0f <= anAngle)
		mTwistAngle = 90.0f;
	else if (anAngle < 0.0f)
		mTwistAngle = 0.0f;
	// Eased: slow out, fast through the middle, slow in
	float aT = (float)((double)mTwistAngle / 90.0);
	float anEase;
	if (0.5f > aT) {
		double aPow = (double)(float)std::pow((double)aT, (double)2.25f);
		anEase = (float)((double)(float)std::pow((double)2.0f, (double)2.25f) * 0.5 * aPow);
	} else {
		float aRest = 1.0f - aT;
		double aPow = (double)(float)std::pow((double)aRest, (double)2.25f);
		anEase = (float)(1.0 - (double)(float)std::pow((double)2.0f, (double)2.25f) * 0.5 * aPow);
	}
	float aTurn = mTwistClockwise ? 1.5707963705062866f : -1.5707963705062866f;
	mCursorTurnNow = (float)((double)aTurn * (double)anEase + (double)mCursorTurnBase);
	double aDegrees = (double)anEase * 90.0;
	if (90.0 < aDegrees)
		aDegrees = 90.0;
	double aRadians = aDegrees * 3.14159 / 180.0;
	double aSin = std::sin(aRadians);
	if (mTwistAngle != anOld) {
		aCell = mGrid;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = *aCell;
				if (aPiece != nullptr && aPiece->mIsTwisting && CanMove(aPiece)) {
					int aDRow = aPiece->mRow - GetRowAt(mCursorY);
					int aDCol = aPiece->mCol - GetColAt(mCursorX);
					if ((aDRow == 0 || aDRow == 1) && (aDCol == 0 || aDCol == 1)) {
						// A gem of the cursor's square circles its centre
						float aCenterX = (float)(gGemWidth / 2 + mCursorX);
						float aCenterY = (float)(gGemHeight / 2 + mCursorY);
						float aX = (float)aPiece->mTwistOffsetX;
						float aY = (float)aPiece->mTwistOffsetY;
						float aRadius = sqrtf((float)((double)aX * (double)aX + (double)aY * (double)aY));
						float aStart = (float)std::atan2((double)aY, (double)aX);
						float aNow = (float)(mTwistClockwise ? (double)aStart - aRadians : (double)aStart + aRadians);
						aPiece->mX = (float)((double)cosf(aNow) * (double)aRadius + (double)aCenterX);
						aPiece->mY = (float)((double)sinf(aNow) * (double)aRadius + (double)aCenterY);
					} else {
						// Any other gem slides straight to its cell
						aPiece->mX = (float)((double)aPiece->mTwistMoveX * aSin + (double)aPiece->mTwistStartX);
						aPiece->mY = (float)((double)aPiece->mTwistMoveY * aSin + (double)aPiece->mTwistStartY);
					}
				}
				aCell++;
			}
		}
	}
	mFinishingTwist = true;
	if (30.0f <= mTwistAngle && aStuck) {
		// Back from a third of the way
		mTwistAngle = 30.0f;
		mTwistStep = -0.75f;
		aCell = mGrid;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				if (*aCell != nullptr && (*aCell)->mIsTwisting)
					CanMove(*aCell);
				aCell++;
			}
		}
	}
	if (90.0f <= mTwistAngle) {
		if (anOld < 90.0f) {
			// The gems arrive: they take their new cells and the board looks for matches
			if (0.0f < mMatchFlash)
				mApp->PlaySample(SOUND_TUTORIAL_MATCH);
			FinishTwist(false);
			FindMatches(1);
		}
		if (0.0f < mMatchFlash) {
			// The tutorial's match pulses before the board goes on
			mMatchFlash = mMatchFlash - mMatchFlashDecay;
			for (int aRow = 0; aRow < mRows; aRow++) {
				for (int aCol = 0; aCol < mCols; aCol++) {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr && aPiece->mIsInMatch) {
						if (0.0f < mMatchFlash) {
							double aSize = (double)(mMatchFlash + mMatchFlash);
							if (1.0 < aSize)
								aSize = 1.0;
							double aWave = std::sin(((double)mMatchFlash + 0.55) * (double)6.2831854820251465f);
							aPiece->mDrawScale = (float)(aSize * 0.5 * aWave + 1.149999976158142);
						} else {
							aPiece->mDrawScale = 1.0f;
						}
					}
				}
			}
			return;
		}
		int aCount = mCols * mRows;
		aCell = mGrid;
		while (aCount != 0) {
			Piece* aPiece = *aCell;
			aCount--;
			aCell++;
			if (aPiece != nullptr && 0 < aPiece->mLockState && aPiece->mIsTwisting) {
				aPiece->mLockState--;
				if (aPiece->mLockState < 1)
					LockPiece(aPiece);
			}
		}
		ClearSparkStarts();
		if (!mInstantTwist && !mTwistMatches && !mBadTwistsAllowed)
			return;
		ClearTurning();
		mTurnEndSoundDue = true;
		mTwisting = false;
		if (!mTurnStartedWithMatch && mLevelStarted) {
			// A twist that made no match
			if (mGameMode != MODE_CHALLENGE)
				mApp->PlaySoundEx(SOUND_BADMOVE, 0, -2.0f, 0.0f, true, true);
			AddStat(0x22, 1);
			mBadTwists++;
		}
		if (!mMatchAtOnce) {
			if (mDelayMatchAfterMove)
				mMatchDelay = 10;
			else
				MatchAfterMove();
			if (IsTipShowing())
				mSpeedNow = 1.0f;
		} else {
			MatchAfterMove();
		}
		mFinishingTwist = false;
	} else {
		if (0.0f < mTwistAngle)
			return;
		// Back where it started
		UnturnPieces();
		mInputLock = 0;
		mTwisting = false;
	}
	mTwistAngle = 0.0f;
}
