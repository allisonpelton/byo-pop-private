// The player's twist: the mouse over the board, the click, and the four gems turning about the cursor
#include "Board.h"

#include "Buffer.h"
#include "Challenge.h"
#include "GoodTube.h"

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
	mUnknown15D0 = false;
}

void Board::MouseWheel(int theDelta)
{
}

void Board::WarpToCursor()
{
	// SetCursorPos: the original's ClientToScreen of the cursor's middle (+0x1c00 and its size +0x1c08)
	if (mUnknown1430 != nullptr)
		mApp->WarpMouse(mUnknown1C08 / 2 + mUnknown1C00, mUnknown1C0C / 2 + mUnknown1C04);
}

void Board::KeyDown(KeyCode theKey)
{
	if (mApp->mDialogMap.size() != 0)
		return;
	if (theKey == KEYCODE_PAUSE || theKey == KEYCODE_ESCAPE) {
		if (!IsBusy() && !mUnknown37C4 && !mUnknown2629 && mUnknown160 == 0 && mUnknown2208 == nullptr) {
			if (mUnknown1570 == 0)
				Pause(true);
			else
				Unpause(true);
		}
	} else {
		bool aPlaying = !mUnknown37C4 && !mUnknown2629;
		if (-1 < mApp->mAutoPlayMode)
			aPlaying = false;
		// Enter or space stops the wheel
		if (aPlaying && (theKey == KEYCODE_RETURN || theKey == KEYCODE_SPACE) && gWheelSpin != nullptr &&
			!gWheelSpin->mButton->mDisabled)
			static_cast<ButtonListener*>(gWheelSpin)->ButtonDepress(0);
		if (aPlaying && mUnknown20D0) {
			int aY = mUnknown1C04;
			int aRow = GetRowAt(aY);
			int aCol = GetColAt(mUnknown1C00);
			if (theKey == KEYCODE_RETURN || theKey == KEYCODE_SPACE) {
				// A click on the cursor's gem
				MouseDown(gGemWidth / 2 + mUnknown1C00, gGemHeight / 2 + aY, 0, 1);
			} else {
				// The arrows move the cursor, unless the opposite one is down too
				aY = aRow;
				if (theKey == KEYCODE_LEFT) {
					if (aCol < 1 || mUnknown21D9 || mWidgetManager->mKeyDown[KEYCODE_RIGHT])
						goto done;
					aCol--;
				} else if (theKey == KEYCODE_UP) {
					if (aRow < 1 || mUnknown21D9 || mWidgetManager->mKeyDown[KEYCODE_DOWN])
						goto done;
					aY = aRow - 1;
				} else if (theKey == KEYCODE_RIGHT) {
					if (mCols - 2 <= aCol || mUnknown21D9 || mWidgetManager->mKeyDown[KEYCODE_LEFT])
						goto done;
					aCol++;
				} else {
					if (theKey != KEYCODE_DOWN)
						goto debug;
					if (mRows - 2 <= aRow || mUnknown21D9 || mWidgetManager->mKeyDown[KEYCODE_UP])
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
			mUnknown21A2 = false;
			SetScore(mUnknown168, false);
		}
		mUnknown2190 = mUnknown1FB8 - 1;
		mApp->mScoresLoaded = true;
	}
done:
	Widget::KeyDown(theKey);
}

void Board::UpdateCursor(int x, int y)
{
	if (0 < mUnknown1570 || IsBusy() || mUnknown2629 || mUnknown160 == 1 || mUnknown37C4)
		return;
	if (mUnknown2208 != nullptr && (Widget*)mUnknown2208 != mUnknown2584 && mUnknown2208->mMouseVisible)
		return;
	for (int i = 0; i < (int)mUnknown23DC.size(); i++) {
		if (mUnknown23DC[i]->mMouseVisible)
			return;
	}
	mUnknown15B4 = x;
	mUnknown15B8 = y;
	int anOldCol = GetColAt(mUnknown1C00);
	int anOldRow = GetRowAt(mUnknown1C04);
	int aRow = GetRowAt(y);
	int aCol = GetColAt(x);
	bool aTwisting = mUnknown21D9;
	if (!aTwisting && (mUnknown1C6D || mUnknown2008 == 0)) {
		int aDX = x - mUnknown1C08 / 2 - mUnknown1C00;
		int aDY = y - mUnknown1C0C / 2 - mUnknown1C04;
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
		mUnknown1C40 = false;
	}
	if (mUnknown15C == 0 && !aTwisting && (anOldCol != aCol || anOldRow != aRow) && !mUnknown21A0) {
		ClearSparkStarts();
		mUnknown1450 = 1.0f;
	}
}

void Board::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	UpdateCursor(x, y);
	if (mGameMode == MODE_PUZZLE && (mUnknown1324 || mUnknown2030))
		return;
	if (mUnknown3905 && !mUnknown14AC.empty())
		return;
	if (mUnknown2208 != nullptr && mUnknown2208->mMouseVisible && (Widget*)mUnknown2208 != mUnknown2584)
		return;
	for (int i = 0; i < (int)mUnknown23DC.size(); i++) {
		if (mUnknown23DC[i]->mMouseVisible)
			return;
	}
	if (mUnknown2584 != nullptr) {
		// (not while the tutorial waits on its dialog or its seconds, or on a forced move with no square to make it)
		if (mUnknown2584->mUnknown210)
			return;
		int aState = mUnknown2584->mState;
		if (aState == Tutorial::STATE_SECONDS || (aState == Tutorial::STATE_MOVE && mUnknown2400 == -1))
			return;
	}
	if (mApp->mOptionsDialog != nullptr)
		return;
	if (mUnknown1570 != 0) {
		// a click on the paused board resumes the game
		Unpause(true);
		return;
	}
	if (mUnknown37C4 || mUnknown2629 || mUnknown25A6 || !mUnknown149C.empty() || mUnknown21A3 || !mUnknown20D0)
		return;
	bool aQueue;
	if (!mUnknown21D8) {
		if (mUnknown2200) {
			// The original says "BOARD:  mSkipMouseClick cleared" with OutputDebugString
			mUnknown2200 = false;
		}
		if (0 < mUnknown15D4)
			return;
		mUnknown15D0 = true;
		if (mUnknown21A0)
			return;
		if (mUnknown25A4) {
			ClearSparkStarts();
			mUnknown1C1C = mUnknown1C00;
			mUnknown1C20 = mUnknown1C04;
			mUnknown1C24 = mUnknown1C08;
			mUnknown1C28 = mUnknown1C0C;
			if (mUnknown20CC < 0.5f)
				return;
			if (0 < mUnknown15C)
				return;
			mUnknown15C8 = x;
			mUnknown15CC = y;
			if (mUnknown21D9)
				return;
			if (mUnknown15C != 0)
				return;
			mUnknown2015 = false;
			if (!mUnknown25A4 || (mUnknown1C41 && (!mUnknown2054 || !mUnknown1C71))) {
				// A twist the locked gems block: they shake, and the cursor turns a little and back
				if (mUnknown2400 == -1) {
					bool aBlocked = false;
					if (mUnknown2120.size() != 0) {
						aBlocked = true;
						for (unsigned int i = 0; i < mUnknown2120.size(); i++) {
							Piece* aPiece = GetPieceAt(mUnknown2120[i].first, mUnknown2120[i].second);
							if (aPiece != nullptr)
								aPiece->mLockShake = 1;
						}
					}
					SetupTwist(-1, -1);
					mUnknown1458 = 3.0f;
					mUnknown21D9 = true;
					if (!mUnknown2041 || aBlocked)
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
		aQueue = !mUnknown21D9;
	} else {
		aQueue = theBtnNum == 0;
	}
	// A click while the board is not ready waits (UpdateGame takes it within 20 updates)
	if (aQueue) {
		mUnknown15BC = x;
		mUnknown15C0 = y;
		mUnknown15C4 = mUpdateCnt;
	}
}

void Board::StartTwist(bool theInstant, int theX, int theY)
{
	if (mUnknown25A6 && !mUnknown37C4)
		return;
	if (mChallenge != 0 && (mUnknown266A || mUnknown2669))
		return;
	if (mUnknown2584 != nullptr && !mUnknown2584->mMovesAllowed)
		return;
	mUnknown1390 = false;
	mUnknown3904 = false;
	if (mUnknown830.GetOutVal() != 0.0) {
		float anIn = (float)mUnknown830.GetInVal();
		mUnknown830.SetCurve(GTS(0x7dd5e8));
		mUnknown830.SetInVal(1.0 - (double)anIn);
	}
	mUnknown21A1 = true;
	mUnknown2664 = mUnknown2660;
	std::memset(mUnknown2234, 0, sizeof(mUnknown2234));  // 0x6a69f0
	mUnknown3804 = 0;
	mUnknown2614.clear();
	if (!mUnknown2629) {
		mSaveBuffer.Clear();
		mUnknown23B4.Clear();
	}
	mUnknown3450 = 0;
	if (!mUnknown37C4 && !mUnknown2629) {
		// The game before the twist, for the undo of a turn-based challenge and the tutorials
		SaveGame(&mSaveBuffer, false);
		mUnknown23A4 = GetRowAt(mUnknown1C04);
		mUnknown23A8 = GetColAt(mUnknown1C00);
		mUnknown23B0 = mApp->mProfile->mRank;
		mUnknown23AC = mApp->mProfile->mUnknown160;
	}
	if (mReplayPod != nullptr) {
		mReplayPod->mState = ReplayWidget::STATE_GOING;
		mReplayPod->mPopped = false;
	}
	mUnknown23C++;
	mUnknown25D0 = 0.0f;
	mUnknown1C42 = mUnknown1C40;
	mUnknown1EC = mUnknown1E8;
	mUnknown1F4 = mUnknown1F0;
	mUnknown16C = mUnknown168;
	mUnknown25C8 = 0;
	mUnknown25CC = 0;
	mUnknown1C58 = 25;
	mUnknown1C5C = 0;
	mUnknown1544 = 0;
	mUnknown1F80 = -1;
	mUnknown14F4 = 0;
	mUnknown218C = false;
	mUnknown21F4 = -1;
	mUnknown1508 = 0;
	mUnknown262A = false;
	mUnknown2198 = true;
	mUnknown2668 = true;
	mUnknown1F8 = 0;
	mUnknown170 = mUnknown14FC;
	mUnknown2404 = -1;
	mUnknown2400 = -1;
	mUnknown2016 = true;
	mUnknown216C = false;
	if (mGameMode == MODE_CHALLENGE && mChallenge != 0) {
		CheckGoal(false);
	}
	if (!theInstant) {
		double aX;
		if (mUnknown1430 != nullptr) {
			aX = (double)mUnknown1430->mX + (double)gGemWidth;
		} else {
			Board* aBoard = gBoard;
			aX = (double)(aBoard->GetColX(0) + (aBoard->mCols * gGemWidth) / 2);
		}
		// Lower when the twist makes no match
		float aPitch = (float)(mUnknown1C40 ? 0 : -5);
		mApp->PlaySoundEx(SOUND_START_ROTATE, GetPan(Ftol(aX)), aPitch, 0.0f, true, true);
	}
	SetupTwist(theX, theY);
	if (theInstant) {
		FinishTwist(true);
	} else {
		mUnknown21D9 = true;
		float aSpeed;
		if (0.0f <= mUnknown1C8C)
			aSpeed = (float)(((double)mUnknown154 * 0.03 + 0.20999999344348907) * (double)mUnknown21D0 +
				3.0999999046325684);
		else
			aSpeed = 1.0f;
		mUnknown1458 = mUnknown145C * aSpeed;
	}
	CountDownBombs(theInstant);
	if (mSparkBlock == 0) {
		int aCount = mCols * mRows;
		Piece** aCell = mGrid;
		while (aCount != 0) {
			Piece* aPiece = *aCell;
			aCell++;
			aCount--;
			if (aPiece != nullptr && 0 < aPiece->mUnknown4FC && !aPiece->mUnknownB0) {
				aPiece->mUnknown4FC--;
				if (aPiece->mUnknown4FC < 1)
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
	int aRoll = Rand85B970() % 10000 - 5000;
	return (float)((double)aRoll / 5000.0);
}

void Board::PreLockPiece(Piece* thePiece)
{
	thePiece->mLockTick = 0;
	thePiece->mUnknown4FC = 2;
	PlayPieceSound(thePiece, SOUND_GEM_PRELOCK, 0.0f, 0.0f, true);
}

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
	if (aLocked < mMaxLocksOnScreen && mLockGem.mUnknown50) {
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
			if (Rand85B970() % 2 == 0) {
				aIndex = Rand85B970() % aTotal;
			} else {
				int aRowPick = Rand85B970() % (mRows - 2);
				int aColPick = Rand85B970() % (mCols - 2);
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
	if (!mUnknown1F6C && 0 <= mDevil.GetCount() && !IsBusy() && mGameMode != MODE_ZEN) {
		anAppear = true;
		if (0 < mUnknown2234[0x21] && (float)((double)mUnknown2234[0x39] / (double)mUnknown2234[0x21]) < 3.0f)
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
		mUnknown1F6C = false;
		anAppear = true;
		int i = 0;
		Piece** aCell = mGrid;
		if (0 < mCols * mRows) {
			do {
				Piece* aPiece = *aCell;
				if (aPiece != nullptr && (aPiece->IsFlagSet(Piece::FLAG_DEVIL) || aPiece->IsFlagSet(Piece::FLAG_12)))
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
	int aCursorX = mUnknown1C00;
	int aCursorY = mUnknown1C04;
	int aTries = 100;
	do {
		while (!mUnknown1F6C) {
			int aRowPick = Rand85B970() % (mRows - 2);
			int aColPick = Rand85B970() % (mCols - 2);
			Piece* aPiece = mGrid[aColPick + (aRowPick + 1) * mCols + 1];
			if (aPiece == nullptr || aPiece->mBlasted || aPiece->mBreaking || 6 < aPiece->mColor || aPiece->mFlags != 0)
				continue;
			Piece* aSaved = mUnknown1430;
			bool aMatched = false;
			MatchInfo anInfo;  // 0x6b97b6
			int aRow = 0;
			do {
				int aCol = 0;
				do {
					anInfo.Reset();  // 0x6afdee
					MoveCursor(aRow, aCol, true, &anInfo);
					if (!mUnknown1C41 && mUnknown1C40) {
						for (unsigned int j = 0; !anInfo.mIds.empty() && j < anInfo.mIds.size(); j++) {
							if (anInfo.mIds[j] == aPiece->mId)
								aMatched = true;
						}
					}
					aCol++;
				} while (aCol < 6);
				aRow++;
			} while (aRow < 8);
			if (!aMatched) {
				mUnknown1430 = aSaved;
				PreLockPiece(aPiece);
				aPiece->SetFlag(Piece::FLAG_12, true);
				mApp->PlaySample(SOUND_DOOMGEM_APPEARS);
				mUnknown1F6C = true;
			}
		}
		aTries--;
	} while (aTries != 0);
	MoveCursor(GetRowAt(aCursorY), GetColAt(aCursorX), true, nullptr);
}

void Board::SetupTwist(int theX, int theY)
{
	mUnknown20F4.clear();
	if (theX < 0)
		theX = mUnknown1C00;
	if (theY < 0)
		theY = mUnknown1C04;
	// The twist's centre
	int aCenterX = gGemWidth + theX;
	int aCenterY = gGemHeight + theY;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece == nullptr)
				continue;
			aPiece->mUnknownE4 = -1.0f;
			aPiece->mUnknownE8 = 0.0f;
			aPiece->mUnknown24 = aPiece->mCol;
			aPiece->mUnknown2C = aPiece->mRow;
			aPiece->mUnknown30 = aPiece->mRow;
			aPiece->mUnknown51D = false;
			aPiece->mPendingKind = 0;
			// The four gems half a gem from the centre turn
			int aDX = std::abs(-(gGemWidth / 2) - Ftol(aPiece->mX) + aCenterX);
			int aDY = std::abs(-(gGemHeight / 2) - Ftol(aPiece->mY) + aCenterY);
			if ((aDX == gGemWidth / 2 && aDY <= gGemHeight / 2) || (aDY == gGemHeight / 2 && aDX <= gGemWidth / 2)) {
				mUnknown20F4.push_back(aPiece);
				aPiece->mUnknownF8 = true;
			} else {
				aPiece->mUnknownF8 = false;
			}
		}
	}
	// Sorted by their quarter about the centre
	float aQuarter = (float)((double)6.2831854820251465f * 0.25);
	std::vector<Piece*> aByQuarter;
	for (int i = 4; i != 0; i--)
		aByQuarter.push_back(nullptr);
	for (unsigned int i = 0; i < mUnknown20F4.size(); i++) {
		Piece* aPiece = mUnknown20F4[i];
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
			mUnknown20F4[aCount] = aByQuarter[i];
			aCount++;
		}
	}
	// Each gem moves to the next one's cell: clockwise, or the other way with +0x2015
	int aSize = (int)mUnknown20F4.size();
	for (int i = 0; i < aSize; i++) {
		int aNext = i + 1;
		if (mUnknown2015)
			aNext -= 2;
		if (aNext < 0)
			aNext += (int)mUnknown20F4.size();
		else if (aNext == (int)mUnknown20F4.size())
			aNext = 0;
		Piece* aFrom = mUnknown20F4[i];
		Piece* aTo = mUnknown20F4[aNext];
		StartPieceMove(mUnknown20F4[i], aTo->mCol - aFrom->mCol, aTo->mRow - aFrom->mRow);
	}
}

bool Board::StartPieceMove(Piece* thePiece, int theDCol, int theDRow)
{
	ClearCellHints();
	mUnknown2004 = false;
	mUnknown3444 = 0;
	// The move's frames are recorded afresh
	gGraphicsRecorder->PopFramesFrom(0);
	if ((theDCol == 0 && theDRow == 0) || thePiece == nullptr)
		return false;
	int aCol = thePiece->mCol + theDCol;
	int aRow = thePiece->mRow + theDRow;
	if (aCol < 0 || mCols <= aCol || aRow < 0 || mRows <= aRow)
		return false;
	mUnknown1558 = true;
	thePiece->mUnknownE0 = 0;
	thePiece->mUnknownB0 = true;
	int aToX = GetColX(thePiece->mCol + theDCol);
	int aToY = GetRowY(thePiece->mRow + theDRow);
	int aCenterX = mUnknown1C00 + gGemWidth / 2;
	int aCenterY = mUnknown1C04 + gGemHeight / 2;
	int aX = Ftol(thePiece->mX);
	thePiece->mUnknownA0 = aX;
	int aY = Ftol(thePiece->mY);
	thePiece->mUnknownA4 = aY;
	thePiece->mUnknownA8 = Ftol((double)aX - (double)aCenterX);
	thePiece->mUnknownAC = Ftol((double)aY - (double)aCenterY);
	thePiece->mUnknown98 = theDCol;
	thePiece->mUnknown9C = theDRow;
	thePiece->mUnknown90 = Ftol((double)aToX - (double)thePiece->mX);
	thePiece->mUnknown94 = Ftol((double)aToY - (double)thePiece->mY);
	return true;
}

void Board::FinishTwist(bool theStop)
{
	if (mUnknown2584 != nullptr)
		mUnknown2584->PointAtNothing();
	std::vector<Piece*> aTurned;
	Piece** aCell = mGrid;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && aPiece->mUnknownB0) {
				if (theStop) {
					aPiece->mUnknownB0 = false;
					aPiece->mUnknown14 = 0.0f;
				}
				aTurned.push_back(aPiece);
				*aCell = nullptr;
			}
			aCell++;
		}
	}
	if (theStop)
		mUnknown21D9 = false;
	// The turned gems in their new cells
	for (unsigned int i = 0; i < aTurned.size(); i++) {
		Piece* aPiece = aTurned[i];
		aPiece->mCol += aPiece->mUnknown98;
		aPiece->mRow += aPiece->mUnknown9C;
		int aRow = aPiece->mRow;
		aPiece->mUnknown30 = aRow;
		aPiece->mX = (float)GetColX(aPiece->mCol);
		aPiece->mY = (float)GetRowY(aRow);
		mGrid[mCols * aRow + aPiece->mCol] = aPiece;
	}
	if (mGameMode == MODE_PUZZLE)
		mUnknown2038 = 1.0f;
}

void Board::CountDownBombs(bool theInstant)
{
	if (!mUnknown2198 || IsSparkBlocked() || (mGameMode == MODE_PUZZLE && mUnknown1324) || IsBusy())
		return;
	Piece** aCell = mGrid;
	mUnknown2010 = mCounter.GetValue();
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = *aCell;
			if (aPiece != nullptr && 0 < aPiece->mBombCount) {
				int aLeft;
				if (theInstant) {
					aLeft = aPiece->mBombCount - 1;
					aPiece->mBombCount = aLeft;
				} else if (mUnknown1C40 && aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
					// A devil gem that this twist's match may clear keeps its count
					aCell++;
					continue;
				} else {
					// The counter pulses; the count itself goes down as the twist ends
					aPiece->mUnknownD0 = 1.0f;
					aPiece->mUnknownD4 = 0.33f;
					aLeft = aPiece->mBombCount - 1;
				}
				// The lowest count left on the board
				int aLow = aLeft < 0 ? 0 : aLeft;
				if (aLow < mUnknown2010)
					mUnknown2010 = aLeft < 0 ? 0 : aLeft;
			}
			aCell++;
		}
	}
	mUnknown1C44 = 255;
	mUnknown2198 = false;
}

void Board::LockPiece(Piece* thePiece)
{
	bool aKept = thePiece->IsFlagSet(Piece::FLAG_12);
	thePiece->ClearSpecial();
	thePiece->SetFlag(Piece::FLAG_LOCKED, true);
	if (aKept)
		thePiece->SetFlag(Piece::FLAG_12, true);
	thePiece->mUnknown4FC = 0;
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
			if (aPiece != nullptr && aPiece->mUnknownB0) {
				aPiece->mX = (float)GetColX(aCol);
				aPiece->mUnknownB0 = false;
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
			if (aPiece != nullptr && aPiece->mUnknownB0)
				aPiece->mUnknownB0 = false;
		}
	}
}

void Board::UpdateTwist()
{
	if (mUnknown25A6)
		return;
	// A gem that cannot move sends the twist back
	bool aStuck = false;
	Piece** aCell = mGrid;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			if (*aCell != nullptr && (*aCell)->mUnknownB0 && !CanMove(*aCell))
				aStuck = true;
			aCell++;
		}
	}
	MarkDirty();
	float aFade = (float)((double)mUnknown20D4 + 0.05000000074505806);
	mUnknown25A4 = false;
	if (1.0f < aFade)
		aFade = 1.0f;
	mUnknown20D4 = aFade;
	float anOld = mUnknown1454;
	float aStep = mUnknown210C ? 90.0f : mUnknown1458;
	float anAngle = mUnknown1454 + aStep;
	mUnknown1454 = anAngle;
	if (90.0f <= anAngle)
		mUnknown1454 = 90.0f;
	else if (anAngle < 0.0f)
		mUnknown1454 = 0.0f;
	// Eased: slow out, fast through the middle, slow in
	float aT = (float)((double)mUnknown1454 / 90.0);
	float anEase;
	if (0.5f > aT) {
		double aPow = (double)(float)std::pow((double)aT, (double)2.25f);
		anEase = (float)((double)(float)std::pow((double)2.0f, (double)2.25f) * 0.5 * aPow);
	} else {
		float aRest = 1.0f - aT;
		double aPow = (double)(float)std::pow((double)aRest, (double)2.25f);
		anEase = (float)(1.0 - (double)(float)std::pow((double)2.0f, (double)2.25f) * 0.5 * aPow);
	}
	float aTurn = mUnknown2015 ? 1.5707963705062866f : -1.5707963705062866f;
	mUnknown20C0 = (float)((double)aTurn * (double)anEase + (double)mUnknown20C4);
	double aDegrees = (double)anEase * 90.0;
	if (90.0 < aDegrees)
		aDegrees = 90.0;
	double aRadians = aDegrees * 3.14159 / 180.0;
	double aSin = std::sin(aRadians);
	if (mUnknown1454 != anOld) {
		aCell = mGrid;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				Piece* aPiece = *aCell;
				if (aPiece != nullptr && aPiece->mUnknownB0 && CanMove(aPiece)) {
					int aDRow = aPiece->mRow - GetRowAt(mUnknown1C04);
					int aDCol = aPiece->mCol - GetColAt(mUnknown1C00);
					if ((aDRow == 0 || aDRow == 1) && (aDCol == 0 || aDCol == 1)) {
						// A gem of the cursor's square circles its centre
						float aCenterX = (float)(gGemWidth / 2 + mUnknown1C00);
						float aCenterY = (float)(gGemHeight / 2 + mUnknown1C04);
						float aX = (float)aPiece->mUnknownA8;
						float aY = (float)aPiece->mUnknownAC;
						float aRadius = sqrtf((float)((double)aX * (double)aX + (double)aY * (double)aY));
						float aStart = (float)std::atan2((double)aY, (double)aX);
						float aNow = (float)(mUnknown2015 ? (double)aStart - aRadians : (double)aStart + aRadians);
						aPiece->mX = (float)((double)cosf(aNow) * (double)aRadius + (double)aCenterX);
						aPiece->mY = (float)((double)sinf(aNow) * (double)aRadius + (double)aCenterY);
					} else {
						// Any other gem slides straight to its cell
						aPiece->mX = (float)((double)aPiece->mUnknown90 * aSin + (double)aPiece->mUnknownA0);
						aPiece->mY = (float)((double)aPiece->mUnknown94 * aSin + (double)aPiece->mUnknownA4);
					}
				}
				aCell++;
			}
		}
	}
	mUnknown15D8 = true;
	if (30.0f <= mUnknown1454 && aStuck) {
		// Back from a third of the way
		mUnknown1454 = 30.0f;
		mUnknown1458 = -0.75f;
		aCell = mGrid;
		for (int aRow = 0; aRow < mRows; aRow++) {
			for (int aCol = 0; aCol < mCols; aCol++) {
				if (*aCell != nullptr && (*aCell)->mUnknownB0)
					CanMove(*aCell);
				aCell++;
			}
		}
	}
	if (90.0f <= mUnknown1454) {
		if (anOld < 90.0f) {
			// The gems arrive: they take their new cells and the board looks for matches
			if (0.0f < mUnknown23EC)
				mApp->PlaySample(SOUND_TUTORIAL_MATCH);
			FinishTwist(false);
			FindMatches(1);
		}
		if (0.0f < mUnknown23EC) {
			// The tutorial's match pulses before the board goes on
			mUnknown23EC = mUnknown23EC - mUnknown23F0;
			for (int aRow = 0; aRow < mRows; aRow++) {
				for (int aCol = 0; aCol < mCols; aCol++) {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr && aPiece->mUnknown2F4) {
						if (0.0f < mUnknown23EC) {
							double aSize = (double)(mUnknown23EC + mUnknown23EC);
							if (1.0 < aSize)
								aSize = 1.0;
							double aWave = std::sin(((double)mUnknown23EC + 0.55) * (double)6.2831854820251465f);
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
			if (aPiece != nullptr && 0 < aPiece->mUnknown4FC && aPiece->mUnknownB0) {
				aPiece->mUnknown4FC--;
				if (aPiece->mUnknown4FC < 1)
					LockPiece(aPiece);
			}
		}
		ClearSparkStarts();
		if (!mUnknown210C && !mUnknown1C40 && !mUnknown1BF8)
			return;
		ClearTurning();
		mUnknown2130 = true;
		mUnknown21D9 = false;
		if (!mUnknown1C42 && mUnknown2041) {
			// A twist that made no match
			if (mGameMode != MODE_CHALLENGE)
				mApp->PlaySoundEx(SOUND_BADMOVE, 0, -2.0f, 0.0f, true, true);
			AddStat(0x22, 1);
			mUnknown240++;
		}
		if (!mUnknown1C6D) {
			if (mUnknown1C6F)
				mUnknown2008 = 10;
			else
				MatchAfterMove();
			if (GetUnknown23F5())
				mUnknown1C8C = 1.0f;
		} else {
			MatchAfterMove();
		}
		mUnknown15D8 = false;
	} else {
		if (0.0f < mUnknown1454)
			return;
		// Back where it started
		UnturnPieces();
		mUnknown15C = 0;
		mUnknown21D9 = false;
	}
	mUnknown1454 = 0.0f;
}
