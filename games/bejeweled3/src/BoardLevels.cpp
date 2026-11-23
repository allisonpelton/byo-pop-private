// Classic's level bar, the level change, the move search, the hint and the end of the game (bejeweled3, docs/NOTES.md,
// "Levels, hints and the end of the game"). Addresses are of the exe.
#include "Board.h"
#include "EndLevelDialog.h"

#include "Bej3App.h"
#include "Profile.h"
#include "X86Math.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>

namespace Sexy {

// The test a piece has to pass to be moved by a move (0x75a8e0): not a pulse (scale curve type 6), not held by a swap, free to fall,
// not busy, no slide shown, none of the bits 0x2100, and free to be swapped
static bool CanBeMoved(Board* theBoard, Piece* thePiece)
{
	return thePiece->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA && !theBoard->IsHeldBySwap(thePiece) && thePiece->mCanFall &&
		thePiece->mBusyCount == 0 && thePiece->mSlideCurve.GetOutVal() == 0.0 && (thePiece->mFlags & 0x2100) == 0 && thePiece->mCanSwap;
}

// The test a piece has to pass to be the other piece of a move or to be counted along a run: the same without the swap bit and with
// 0x2000 only
static bool IsFree(Board* theBoard, Piece* thePiece)
{
	return thePiece->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA && !theBoard->IsHeldBySwap(thePiece) && thePiece->mCanFall &&
		thePiece->mBusyCount == 0 && thePiece->mSlideCurve.GetOutVal() == 0.0 && (thePiece->mFlags & 0x2000) == 0;
}

// 0x75a8e0 (bejeweled3). Rows are taken from the top, or from the bottom when theReverse, columns from the left. For each piece
// the neighbours are tried to the right, to the left, below and above. A move is accepted when the swap leaves a run of three or
// more through the cell (or, with theNeedBonus, of four or more or a crossing), or when the piece is a hypercube moved to a cell with
// a piece; with theMustInclude it has to hold that piece (or be the hypercube of its colour)
bool Board::FindMove(BoardMove* theOut, int theSkip, bool theReverse, const Piece* theMustInclude, bool theNeedBonus)
{
	static const int aDirections[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	int aFound = 0;
	int aRowStart = theReverse ? ROWS - 1 : 0;
	int aRowStep = theReverse ? -1 : 1;
	int aRowEnd = theReverse ? -1 : ROWS;
	for (int aRow = aRowStart; aRow != aRowEnd; aRow += aRowStep) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || !CanBeMoved(this, aPiece))
				continue;
			for (int aDirection = 0; aDirection < 4; aDirection++) {
				int aToCol = aCol + aDirections[aDirection][0];
				int aToRow = aRow + aDirections[aDirection][1];
				if ((unsigned)aToCol >= (unsigned)COLS || (unsigned)aToRow >= (unsigned)ROWS)
					continue;
				bool aBonus = false;
				bool aInclude = theMustInclude == nullptr;
				Piece* aOther = mGrid[aToRow][aToCol];
				if ((aPiece->mFlags & Piece::FLAG_HYPERCUBE) != 0 && aOther != nullptr) {
					if (theMustInclude != nullptr && aPiece->mColor == theMustInclude->mColor)
						aInclude = true;
					aBonus = true;
				}
				bool aAccept;
				if (aOther != nullptr && aOther->mColor != Piece::NO_COLOR && IsFree(this, aOther)) {
					mGrid[aRow][aCol] = aOther;
					mGrid[aToRow][aToCol] = aPiece;
					int aColor = aOther->mColor;
					if (theMustInclude == aOther)
						aInclude = true;
					int aLeft = aCol;
					while (aLeft > 0 && mGrid[aRow][aLeft - 1] != nullptr && mGrid[aRow][aLeft - 1]->mColor == aColor &&
						IsFree(this, mGrid[aRow][aLeft - 1])) {
						aLeft--;
						if (theMustInclude == mGrid[aRow][aLeft])
							aInclude = true;
					}
					int aRight = aCol;
					while (aRight < COLS - 1 && mGrid[aRow][aRight + 1] != nullptr && mGrid[aRow][aRight + 1]->mColor == aColor &&
						IsFree(this, mGrid[aRow][aRight + 1])) {
						aRight++;
						if (theMustInclude == mGrid[aRow][aRight])
							aInclude = true;
					}
					int aUp = aRow;
					while (aUp > 0 && mGrid[aUp - 1][aCol] != nullptr && mGrid[aUp - 1][aCol]->mColor == aColor &&
						IsFree(this, mGrid[aUp - 1][aCol])) {
						aUp--;
						if (theMustInclude == mGrid[aUp][aCol])
							aInclude = true;
					}
					int aDown = aRow;
					while (aDown < ROWS - 1 && mGrid[aDown + 1][aCol] != nullptr && mGrid[aDown + 1][aCol]->mColor == aColor &&
						IsFree(this, mGrid[aDown + 1][aCol])) {
						aDown++;
						if (theMustInclude == mGrid[aDown][aCol])
							aInclude = true;
					}
					mGrid[aRow][aCol] = aPiece;
					mGrid[aToRow][aToCol] = aOther;
					int aAcross = aRight - aLeft;
					int aDownward = aDown - aUp;
					if (theNeedBonus) {
						if (aAcross >= 3 || aDownward >= 3)
							aBonus = true;
						aAccept = (aAcross >= 2 && aDownward >= 2) || aBonus;
					}
					else
						aAccept = aAcross >= 2 || aDownward >= 2 || aBonus;
				}
				else
					aAccept = aBonus;
				if (aAccept && aInclude) {
					if (aFound == theSkip) {
						if (theOut != nullptr) {
							theOut->mFromCol = aCol;
							theOut->mFromRow = aRow;
							theOut->mToCol = aToCol;
							theOut->mToRow = aToRow;
						}
						return true;
					}
					aFound++;
				}
			}
		}
	}
	return false;
}

// 0x75ae10 (bejeweled3): two draws of the app's Rand(), 2 for the direction of the first pass and 10 for how many moves it may skip
bool Board::FindHint(BoardMove* theOut)
{
	bool aReverse = Rand(2) == 0;
	int aSkips = (int)Rand(10);
	for (int aPass = 0; aPass < 2; aPass++) {
		for (int aSkip = aSkips; aSkip >= 0; aSkip--) {
			if (FindMove(theOut, aSkip, aReverse, nullptr, false))
				return true;
		}
		aReverse = !aReverse;
	}
	return false;
}

// slot 98, 0x73d4f0 (bejeweled3). port: the overlays' bit (+0x181) is left out, there are none in Classic's play
bool Board::HintAllowed() const
{
	if (!mSettledOnce || mGameOverTicks != 0 || mHyperspace != nullptr)
		return false;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr && (mGrid[aRow][aCol]->mFlags & Piece::FLAG_RANDOM_PICK) != 0)
				return false;
		}
	}
	return true;
}

// slot 202, 0x75aeb0 (bejeweled3): the piece the hint is for is the one at the move's second cell, or the first when that is a Hypercube; the
// player's hint makes its flash (0x748480: MakeHintEffect, which draws one Rand() of the app at its first update, after the two of
// FindHint). port: the help tip (0x742cd0, slot 0x148 with 0x10) and the sound are not made
void Board::ShowHint(bool theUser)
{
	if (theUser)
		mHintFlash = 250;
	mHintTimer = 0;
	if (theUser || mAutoHints) {
		BoardMove aMove;
		if (FindHint(&aMove)) {
			Piece* aPiece = mGrid[aMove.mToRow][aMove.mToCol];
			Piece* aFrom = mGrid[aMove.mFromRow][aMove.mFromCol];
			if (aFrom != nullptr && (aFrom->mFlags & Piece::FLAG_HYPERCUBE) != 0)
				aPiece = aFrom;
			if (aPiece != nullptr)
				MakeHintEffect(aPiece, theUser);
		}
	}
}

// 0x755940 (bejeweled3) and slot 217 (0x73b810). The bar follows its target (the progress over the level's size, at most 1): up by
// 0.025 of the way and 0.0005, down by 0.05 of the way and 0.0001. When it is 1 and the progress is the level's size (and +0x1188 is
// 0), the level change is asked for (slot 156, 0x74e190: nothing else is under way). port: the log line it writes is left out
void Board::UpdateLevelBar()
{
	if (!HasLevels())
		return;
	float aTarget = GetBarTarget();   // 0x755940 calls slot 188
	bool aSlowCurve = mBarRateCurve.GetOutVal() == 0.0;
	if (aTarget > mLevelBar) {
		double aStep = aSlowCurve ? 0.0255 : 0.025;
		double aAdd = aSlowCurve ? 0.0012 : 0.0005;
		float aNext = (float)((double)(float)((double)(aTarget - mLevelBar) * aStep) + (double)mLevelBar + aAdd);
		mLevelBar = aNext < aTarget ? aNext : aTarget;
	}
	else {
		float aNext = (float)((double)(float)((double)(aTarget - mLevelBar) * 0.05) + (double)mLevelBar - 0.0001);
		mLevelBar = aNext > aTarget ? aNext : aTarget;
	}
	// slot 215 (0x73daf0) puts the bar's parts where they are drawn: not translated
	if (IsLevelFull()) {
		if (!mGameOver && !mLevelUpPending && mHyperspace == nullptr && mGameOverTicks < 1)
			OnLevelFull();
	}
}

// slot 217, 0x73b810 (bejeweled3): with no time limit (slot 104 is 0: Classic and Zen) the bar is full and the progress is at the level's size;
// with one (Lightning) the size must be above 0 and the progress (slot 155) must have reached it
bool Board::IsLevelFull() const
{
	const int aMax = GetLevelMax();
	if (GetTimeLimit() == 0)
		return mLevelBar >= 1.0f && mLevelProgress >= aMax;
	return aMax > 0 && aMax <= GetLevelProgress();
}

// slot 156, 0x74e190 (bejeweled3), run when the level is full: the level change is asked for (it starts when the board is at rest). QuestBoard's
// own slot 156 (0x6ca7e0) is the Quest's end of a level
void Board::OnLevelFull()
{
	mLevelUpPending = true;
}

// 0x767e20 (bejeweled3), after the match groups: with the board at rest and nothing asked for, the level change's overlay is made.
// port: the list at +0x26b8 is empty in Classic; the overlay's drawing (0x724110) is not made; the 3D one is the exe's other branch
void Board::StartLevelChange()
{
	if (!mLevelUpPending || !IsSettled() || !mPendingSwaps.empty())
		return;
	mHyperspace.reset(new Hyperspace());
	mHyperspace->mCurve.SetCurve("b+0,1,0.0025,1,~###  .~###   4####    b####");
	mLevelUpPending = false;
	// 0x769c36 (and 0x760bcd): the next background (slot 164 with 1). port: the exe first starts the old one's fade (slot 82, 0x77dde0) and
	// hides the new one until the overlay's last event (the overlay's drawing, 0x724110, is not made, so the new one shows at once)
	SetBackground(1);
}

// 0x725ef0 (bejeweled3): one update of the overlay. Its count goes up first; the curve runs 400 updates and the update after it ends
// the change (event 5); at the 200th the level goes up (event 2), at the 250th the gems are put in a new order
void Board::UpdateHyperspace()
{
	if (mHyperspace == nullptr)
		return;
	mHyperspace->mAge++;
	int aAge = mHyperspace->mAge;
	bool aRunning = mHyperspace->mCurve.IncInVal();
	if (aAge == 200)
		LevelEvent(2);
	if (aAge == 250)
		ReshuffleBoard();
	if (!aRunning)
		LevelEvent(5);
}

// slot 157, 0x760ac0 (bejeweled3). Event 2 is the level going up, event 5 the end of the change. port: the events 0, 1, 3 and 4 only
// change the background and the flame gems' look; event 5's save of the player (0x71bb10) and the board (0x742f10) and the
// announcement of the new level's number are not made
void Board::LevelEvent(int theEvent)
{
	if (theEvent == 2) {
		mLevelIndex++;
		mLevelProgress = 0;
		mLevelCounter = 0;
	}
	else if (theEvent == 5)
		mHyperspace.reset();
}

// 0x7604d0 (bejeweled3): the gems are mirrored, turned in 2 by 2 blocks until a move shows, then swapped at random until the first
// move (counted from the bottom, skipping 3) is in the lower half. Every draw is the board's own generator's (+0x258). port: the
// details of the first loop's turning were read from the decompile
void Board::ReshuffleBoard()
{
	std::vector<Piece*> aPieces;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr)
				aPieces.push_back(mGrid[aRow][aCol]);
		}
	}
	auto aPlace = [&]() {
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++)
				mGrid[aRow][aCol] = nullptr;
		}
		for (Piece* aPiece : aPieces)
			mGrid[aPiece->mSpecialState][aPiece->mSlideCol] = aPiece;
	};
	bool aFound;
	do {
		for (Piece* aPiece : aPieces) {
			int aOldCol = aPiece->mCol;
			aPiece->mCol = 7 - aOldCol;
			aPiece->mSlideCol = 7 - aOldCol;
			int aOldRow = aPiece->mRow;
			aPiece->mRow = 7 - aOldRow;
			aPiece->mSpecialState = 7 - aOldRow;
		}
		aPlace();
		for (int aTry = 0; (aTry < 16 || !FindMove(nullptr, 0, false, nullptr, false)) && aTry < 20; aTry++) {
			unsigned long aFirst = mRand.Next();
			unsigned long aSecond = mRand.Next();
			int aCol = (int)(aFirst & 6);
			int aRow = (int)(aSecond & 6);
			Piece* aMover = nullptr;
			for (Piece* aPiece : aPieces) {
				if (aPiece->mSlideCol == aCol && aPiece->mSpecialState == aRow)
					aMover = aPiece;
			}
			if (aMover == nullptr)
				continue;
			static const int aOffsets[3][2] = {{1, 0}, {1, 1}, {0, 1}};
			int aTurns = 0;
			for (;;) {
				for (int aStep = 0; aStep < 3; aStep++) {
					Piece* aOther = mGrid[aRow + aOffsets[aStep][1]][aCol + aOffsets[aStep][0]];
					if (aOther == nullptr)
						continue;
					std::swap(aMover->mSlideCol, aOther->mSlideCol);
					std::swap(aMover->mSpecialState, aOther->mSpecialState);
					mGrid[aMover->mSpecialState][aMover->mSlideCol] = aMover;
					mGrid[aOther->mSpecialState][aOther->mSlideCol] = aOther;
				}
				if ((aTurns % 2 == 0 && !HasMatches()) || ++aTurns > 3)
					break;
			}
		}
		aFound = FindMove(nullptr, 0, false, nullptr, false);
		aPlace();
	} while (!aFound);
	for (Piece* aPiece : aPieces) {
		aPiece->mCol = aPiece->mSlideCol;
		aPiece->mRow = aPiece->mSpecialState;
		mGrid[aPiece->mRow][aPiece->mCol] = aPiece;
	}
	BoardMove aMove;
	do {
		for (int aRound = 0; aRound < 3; aRound++) {
			int aFirstRow = (int)(mRand.Next() & 7);
			int aFirstCol = (int)(mRand.Next() & 7);
			Piece* aFirst = mGrid[aFirstRow][aFirstCol];
			int aSecondRow = (int)(mRand.Next() & 7);
			int aSecondCol = (int)(mRand.Next() & 7);
			Piece* aSecond = mGrid[aSecondRow][aSecondCol];
			if (aFirst == nullptr || aSecond == nullptr)
				continue;
			int aSwaps = 0;
			do {
				std::swap(mGrid[aFirst->mRow][aFirst->mCol], mGrid[aSecond->mRow][aSecond->mCol]);
				std::swap(aFirst->mCol, aSecond->mCol);
				std::swap(aFirst->mRow, aSecond->mRow);
			} while (HasMatches() && ++aSwaps < 2);
		}
	} while (!FindMove(&aMove, 3, true, nullptr, false) || aMove.mFromRow < 4);
	for (Piece* aPiece : aPieces) {
		aPiece->mX = (float)(aPiece->mCol << 7);
		aPiece->mY = (float)(aPiece->mRow << 7);
	}
}

// 0x6728b0 (LocalLeaderboard::SubmitScore, bejeweled3): the score goes in front of the first that is less, the list holds ten.
// port: the file the exe writes after it (users\hiscores.dat) is not written
void Board::SubmitScore(int theScore)
{
	size_t aAt = 0;
	while (aAt < mHighScores.size() && mHighScores[aAt] >= theScore)
		aAt++;
	if (aAt >= 10) {
		mLastScoreRank = -1;
		return;
	}
	std::string aName;
	if (gApp->mProfile != nullptr)
		aName = WStringToString(gApp->mProfile->mName);
	mHighScores.insert(mHighScores.begin() + aAt, theScore);
	mHighScoreNames.resize(mHighScores.size() - 1);
	mHighScoreNames.insert(mHighScoreNames.begin() + aAt, aName);
	if (mHighScores.size() > 10) {
		mHighScores.resize(10);
		mHighScoreNames.resize(10);
	}
	mLastScoreRank = (int)aAt;
}

// 0x73b2d0 (Classic's slot 161, run when the exe finds no move) with 0x7602c0 (bejeweled3): the end screen's wait is counted down
// and, when it runs out, the score (+0xd24) is handed to the leaderboard and the wait is set to 0x113; then the game ends, unless
// it already has or something else is under way. port: the announcement (slot 158, 0x73afe0), the sounds, the music events "Break"
// and "Play", the player's save (0x71bb10) and the leaderboard's flush are left out
void Board::NoMoreMoves(bool theAnnounce)
{
	mEndDialogWait--;
	if (mEndDialogWait < 1) {
		mEndDialogWait = 0x113;
		SubmitScore(mScorePoints);
	}
	EndGame();
}

// 0x7602c0 (bejeweled3): the game is over (the board's flag, +0x58c, and the end's counter, +0xd1c, at 1), when no level change is
// going. Classic's NoMoreMoves (0x73b2d0), Zen's own (never) and Lightning's (0x719350) all end here
void Board::EndGame()
{
	if (!mGameOver && mGameOverTicks < 1 && !mLevelUpPending && mHyperspace == nullptr) {
		mGameOver = true;
		mGameOverTicks = 1;
		// port: test only: a game the bot played says how it ended
		if (mBotMoveCount > 0)
			SDL_Log("BOT GAMEOVER update=%d points=%d level=%d moves=%d", mUpdateCnt, mScorePoints, mLevelIndex + 1, mBotMoveCount);
	}
}

// slot 233 (0x73b100): Classic's end dialog (the other modes' boards make their own)
ClassicEndLevelDialog* Board::MakeEndLevelDialog()
{
	return new ClassicEndLevelDialog(this);
}

// 0x767e20 (bejeweled3): while the game is over the counter goes up each update; at 400 the end screen is asked for (slot 233,
// 0x73b100): it comes when the wait has run out and no other dialog is up (the exe's check of its dialog list, +0xe7 of the app).
// port: app +0x39c is 0 and the curve at +0x2f18 is not running
void Board::UpdateGameOver()
{
	if (mGameOverTicks <= 0)
		return;
	mGameOverTicks++;
	if (mGameOverTicks >= 400) {
		mEndDialogWait--;
		if (mEndDialogWait <= 0 && !mEndScreenShown && gApp->GetDialogCount() == 0) {
			mEndScreenShown = true;
			ClassicEndLevelDialog* aDialog = MakeEndLevelDialog();
			gApp->AddDialog(ClassicEndLevelDialog::DIALOG_ID, aDialog);
			aDialog->Move(0, 0);
		}
	}
}

// port: test only. The exe has no bot: this plays the first move of the move search (the order of 0x75a8e0) as the player would,
// when the board is at rest and the game is on
void Board::UpdateBot()
{
	if (mBotMoves == 0 || !BotMayMove() || mGameOver || mHyperspace != nullptr || mLevelUpPending || !IsSettled() || !mPendingSwaps.empty())
		return;
	BoardMove aMove;
	if (!FindMove(&aMove, 0, false, nullptr, false))
		return;
	Piece* aPiece = mGrid[aMove.mFromRow][aMove.mFromCol];
	if (aPiece != nullptr && TrySwap(aPiece, aMove.mToRow, aMove.mToCol)) {
		mBotMoveCount++;
		if (mBotMoves > 0)
			mBotMoves--;
	}
}

}  // namespace Sexy
