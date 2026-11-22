// The player's move on the board and what it sets off (Classic, plain gems): picking a gem up and swapping (0x73f140,
// 0x73c510, 0x73d990, 0x75fae0, 0x75ee20), the swap's animation and the swap back (0x759ba0), the match search (0x75af60),
// the matches' own life (0x75dee0), the destruction of the pieces (0x74c770, 0x755b80) and the score (0x74a210, 0x73b6d0).
// port: the special gems (flame, star, hypercube, and the bits of the flags that make them), the level bar, the hint, the
// game's end, the sounds and the effects' pictures come with their steps; each place says what is left out and where.
#include "Board.h"

#include "Bej3App.h"
#include "X86Math.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Sexy {

namespace {

// The piece has no bits that make it special (0x1800 and 0x100 are the ones the swap tests; the plain gem has none)
const int FLAGS_SPECIAL_SWAP = 0x1800;
const int FLAGS_NO_SWAP = 0x100;

// What the exe's curves of the swaps are: the 0x85efb0 string for the swap and the 0x85fe00 one for the swap back
const char* const SWAP_CURVE = "b+-1,1,0.033333,1,~x:*    }Ppt+     $#LPR";
const char* const SWAP_BACK_CURVE = "b+-1,1,0.022222,1,#$)h B####    cObb4   z~LPQ";

// 0x73bc50 (bejeweled3): the piece whose cell (as it is drawn, in 1200ths from the grid's corner) holds the place
}  // namespace

Piece* Board::GetPieceAt(float theX, float theY) const
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && aPiece->mX <= theX && aPiece->mY <= theY && theX < aPiece->mX + 128.0f &&
				theY < aPiece->mY + 128.0f)
				return aPiece;
		}
	}
	return nullptr;
}

// 0x73bf00 (bejeweled3): the first piece, by rows, that the player has picked up
Piece* Board::GetSelectedPiece() const
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && aPiece->mSelected)
				return aPiece;
		}
	}
	return nullptr;
}

// 0x73e370 (bejeweled3)
Piece* Board::FindPieceById(int theId) const
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && aPiece->mId == theId)
				return aPiece;
		}
	}
	return nullptr;
}

// 0x73f140 (bejeweled3). port: the exe also gives up while +0x1410 or +0x12c4 is set or +0x12cc is above 0.5 (all 0 in a
// game that is played); slot 98 (0x73d4f0) says the board can be played: it needs the board to have settled once
// (+0x1190), the rest of its tests are 0 in Classic; the second and later pieces' help box (dialog 0x12, in the first
// two levels, 0x76aaf0 and the sound 0xc4 of the app's helper) and the sounds of picking a piece up are left out
void Board::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (!mSettledOnce)
		return;
	mMouseDownX = x;
	mMouseDownY = y;
	mMouseDown = true;
	Piece* aSelected = GetSelectedPiece();
	// the click is only the left button's
	if (theBtnNum != 0) {
		if (aSelected != nullptr)
			aSelected->mSelected = false;
		return;
	}
	// the mouse is in pixels of the art size in use, the pieces in 1200ths of its height
	float aScale = (float)((double)gApp->mArtRes / 1200.0);
	int aTop = Ftol((double)((float)y / aScale));
	int aLeft = Ftol((double)((float)x / aScale));
	Piece* aClicked = GetPieceAt((float)(aLeft - GetGridX()), (float)(aTop - GetGridY()));
	if (aClicked == aSelected && aClicked != nullptr)
		return;
	// no gem there: the exe looks the cell up again with the coordinates as they are (0x73cc10, 0x73cbe0 count from 0 and
	// not from the grid's corner), and in any case what it finds is let go
	if (aClicked == nullptr || !aClicked->mCanSwap) {
		if (aSelected != nullptr)
			aSelected->mSelected = false;
		return;
	}
	if (aSelected == nullptr) {
		// 0x71ff40: a gem with the bits 0x1800 would be swapped with itself by the click; the plain gem is picked up
		if ((aClicked->mFlags & FLAGS_SPECIAL_SWAP) == 0) {
			aClicked->mSelected = true;
			return;
		}
		TrySwap(aClicked, aClicked->mRow, aClicked->mCol);
		return;
	}
	// a gem is in hand: the effects list must be empty (as for every swap)
	if (HasLiveMatches())
		return;
	if (TrySwap(aSelected, aClicked->mRow, aClicked->mCol))
		return;
	// not allowed (not next to it): the first is let go and the one clicked is picked up
	aSelected->mSelected = false;
	aClicked->mSelected = true;
}

// 0x73d990 (bejeweled3): the button comes up. A gem that is picked up the second time round is put down
void Board::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	mMouseDown = false;
	Piece* aSelected = GetSelectedPiece();
	if (aSelected != nullptr && aSelected == mSelectedAtMouseUp && !IsHeldBySwap(aSelected)) {
		aSelected->mSelected = false;
		mSelectedAtMouseUp = nullptr;
		return;
	}
	mSelectedAtMouseUp = aSelected;
}

// 0x73c510 (bejeweled3). The art size's 40 1200ths is how far the mouse must go from where it went down; the swap goes
// the way it went most
void Board::MouseDrag(int x, int y)
{
	if (!mSettledOnce)
		return;
	Piece* aSelected = GetSelectedPiece();
	if (!mMouseDown || aSelected == nullptr)
		return;
	int aAcross = x - mMouseDownX;
	int aDown = y - mMouseDownY;
	int aReach = gApp->mArtRes * 0x28 / 0x4b0;
	int aAbsAcross = std::abs(aAcross);
	int aAbsDown = std::abs(aDown);
	if (aReach > aAbsAcross && aReach > aAbsDown)
		return;
	int aRow;
	int aCol;
	if (aAbsDown < aAbsAcross) {
		if (aAcross >= 1 && aSelected->mCol <= 6) {
			aRow = aSelected->mRow;
			aCol = aSelected->mCol + 1;
		}
		else {
			if (aAcross >= 0 || aSelected->mCol < 1)
				return;
			aRow = aSelected->mRow;
			aCol = aSelected->mCol - 1;
		}
	}
	else {
		if (aDown >= 1 && aSelected->mRow <= 6) {
			aRow = aSelected->mRow + 1;
		}
		else {
			if (aDown >= 0 || aSelected->mRow < 1)
				return;
			aRow = aSelected->mRow - 1;
		}
		aCol = aSelected->mCol;
	}
	TrySwap(aSelected, aRow, aCol);
}

// 0x740e80 (bejeweled3). port: the exe also writes the move into the lists at +0xc54 and +0x1338 (the game's record,
// for replays and the last move), which nothing translated reads. It is a request: the swap itself begins in the next
// update (0x75ee20 is run by UpdateTick for the entries of the list at +0xc60, which the exe's live move reaches
// the same way; whether the exe starts it in the very update of the click or the next one is not settled)
bool Board::TrySwap(Piece* thePiece, int theRow, int theCol)
{
	if (!CanSwap(thePiece, (unsigned)theRow, (unsigned)theCol))
		return false;
	PendingSwap aPending;
	aPending.mUpdate = mUpdateCnt;
	aPending.mPieceId = thePiece->mId;
	aPending.mRow = theRow;
	aPending.mCol = theCol;
	mPendingSwaps.push_back(aPending);
	// the piece has been spoken for: it is not picked up any more
	thePiece->mSelected = false;
	return true;
}

// 0x75fae0 (bejeweled3). port: the list at +0x26b8 (0x68 bytes each) is empty in a plain game
bool Board::CanSwap(Piece* thePiece, unsigned theRow, unsigned theCol)
{
	if (HasLiveMatches())
		return false;
	if (!thePiece->mCanSwap)
		return false;
	Piece* aOther = (theRow < 8 && theCol < 8) ? mGrid[theRow][theCol] : nullptr;
	if (!PieceIsSettled(thePiece))
		return false;
	if ((thePiece->mFlags & FLAGS_NO_SWAP) != 0)
		return false;
	if (aOther != nullptr) {
		if (!aOther->mCanSwap)
			return false;
		if (!PieceIsSettled(aOther))
			return false;
		if ((aOther->mFlags & FLAGS_NO_SWAP) != 0)
			return false;
	}
	int aColDistance = (int)theCol - thePiece->mCol;
	int aRowDistance = (int)theRow - thePiece->mRow;
	bool aSpecial = (thePiece->mFlags & FLAGS_SPECIAL_SWAP) != 0;
	int aDistance = std::abs(aRowDistance) + std::abs(aColDistance);
	return (!aSpecial || aDistance == 0) && (aSpecial || aDistance == 1);
}

// 0x744660 (bejeweled3): the id is the board's counter, which goes up
MoveRecord* Board::NewMoveRecord()
{
	MoveRecord aRecord;
	aRecord.mId = mMatchCounter++;
	aRecord.mMatchCount = 0;
	mMoveRecords.push_back(aRecord);
	return &mMoveRecords.back();
}

MoveRecord* Board::FindMoveRecord(int theId)
{
	for (size_t i = 0; i < mMoveRecords.size(); i++) {
		if (mMoveRecords[i].mId == theId)
			return &mMoveRecords[i];
	}
	return nullptr;
}

// 0x75ee20 (bejeweled3), a plain gem moved by the player (the move's record is made, both pieces carry its id, the swap
// begins). port: the counters of moves (+0x1374, +0x1370, +0x1371 at 0x75ee2f to 0x75ef10), the game's record, the
// branches for the gems with the bits 0x1000 (the hypercube, which picks gems of its own) and the sounds are left out
bool Board::StartSwap(Piece* thePiece, int theRow, int theCol)
{
	if (thePiece == nullptr || theRow < 0 || theRow >= ROWS || theCol < 0 || theCol >= COLS)
		return false;
	if (!CanSwap(thePiece, (unsigned)theRow, (unsigned)theCol))
		return false;
	thePiece->mSelected = false;
	int aMoveId = NewMoveRecord()->mId;
	Piece* aOther = mGrid[theRow][theCol];
	thePiece->mColumnTag = aMoveId;
	if (aOther != nullptr)
		aOther->mColumnTag = aMoveId;
	SwapRecord aSwap;
	aSwap.mFirst = thePiece;
	aSwap.mSecond = aOther;
	aSwap.mDeltaCol = theCol - thePiece->mCol;
	aSwap.mDeltaRow = theRow - thePiece->mRow;
	aSwap.mCurve.SetCurve(SWAP_CURVE);
	aSwap.mIsSwapBack = false;
	aSwap.mDone = false;
	mSwaps.push_back(aSwap);
	return true;
}

// 0x73caa0 (bejeweled3): the piece is in a swap that is still going
bool Board::IsHeldBySwap(const Piece* thePiece) const
{
	for (size_t i = 0; i < mSwaps.size(); i++) {
		const SwapRecord& aSwap = mSwaps[i];
		if (!aSwap.mDone && (aSwap.mFirst == thePiece || aSwap.mSecond == thePiece))
			return true;
	}
	return false;
}

// 0x759ba0 (bejeweled3). Each swap goes along its curve (both pieces are put at their places on it: the first goes from
// its cell to the next, the second the other way). When the curve ends the pieces change cells and the board is searched
// for matches; with none the pieces go back along the other curve and change cells again, and the swap is over.
// port: the swap's second curve (the exe steps two), the sounds of the swap and the refused swap, the counters of 0x73d170
// and the dialog of the first two levels (0x12) are left out. The curve runs from 1 to -1 (its
// output is 1 at its start), so the pieces start at their own cells: x = the cells' middle - delta * curve * 64
void Board::UpdateSwaps()
{
	for (size_t i = 0; i < mSwaps.size();) {
		SwapRecord& aSwap = mSwaps[i];
		Piece* aFirst = aSwap.mFirst;
		Piece* aSecond = aSwap.mSecond;
		bool aEnded = false;
		if (!aSwap.mDone) {
			// IncInVal says true while the input is inside its range (CheckClamping); the swap ends when it is not
			aEnded = !aSwap.mCurve.IncInVal();
			double aCurve = aSwap.mCurve.GetOutVal();
			double aValue = aCurve;
			int aMidX = aFirst->mCol * 128 + (aSwap.mDeltaCol << 7) / 2;
			int aMidY = aFirst->mRow * 128 + (aSwap.mDeltaRow << 7) / 2;
			aFirst->mX = (float)((double)aMidX - (double)aSwap.mDeltaCol * aValue * 128.0 * 0.5);
			aFirst->mY = (float)((double)aMidY - (double)aSwap.mDeltaRow * aValue * 128.0 * 0.5);
			if (aSecond != nullptr) {
				aSecond->mX = (float)(aValue * (double)aSwap.mDeltaCol * 128.0 * 0.5 + (double)aMidX);
				aSecond->mY = (float)(aValue * (double)aSwap.mDeltaRow * 128.0 * 0.5 + (double)aMidY);
			}
		}
		if (!aEnded) {
			i++;
			continue;
		}
		// the pieces change cells
		if (aSecond != nullptr) {
			std::swap(aFirst->mCol, aSecond->mCol);
			std::swap(aFirst->mRow, aSecond->mRow);
			mGrid[aFirst->mRow][aFirst->mCol] = aFirst;
			mGrid[aSecond->mRow][aSecond->mCol] = aSecond;
		}
		else {
			aFirst->mCol += aSwap.mDeltaCol;
			aFirst->mRow += aSwap.mDeltaRow;
		}
		aFirst->mX = (float)(aFirst->mCol << 7);
		aFirst->mY = (float)(aFirst->mRow << 7);
		if (aSecond != nullptr) {
			aSecond->mX = (float)(aSecond->mCol << 7);
			aSecond->mY = (float)(aSecond->mRow << 7);
		}
		aFirst->mSwapUpdate = mUpdateCnt;
		if (aSecond != nullptr)
			aSecond->mSwapUpdate = mUpdateCnt;
		if (aSwap.mIsSwapBack) {
			mSwaps.erase(mSwaps.begin() + i);
			continue;
		}
		// the pieces are no longer held while the board is searched
		aSwap.mDone = true;
		int aMatches = FindMatches();
		// FindMatches does not change the list of swaps
		SwapRecord& aSame = mSwaps[i];
		if (aMatches != 0) {
			mSwaps.erase(mSwaps.begin() + i);
			continue;
		}
		// nothing: back they go (the first one's place is now the second's, so its move is the other way)
		aSame.mDone = false;
		aSame.mIsSwapBack = true;
		aSame.mDeltaCol = -aSame.mDeltaCol;
		aSame.mDeltaRow = -aSame.mDeltaRow;
		aSame.mCurve.SetCurve(SWAP_BACK_CURVE);
		i++;
	}
}

// 0x75af60 (bejeweled3), for plain gems. The board is read twice: first down each column and then along each row
// (the exe's outer loop runs over the first index of its grid expression, which is the column in the first pass). A cell
// takes part when its piece is at rest (0x755ad0) or was already matched in this update (then its remembered colour counts,
// +0x21c); a run is such cells of one colour, and a run of 3 or more is one match of its own: a crossing is two matches that
// share a gem, not one. Every run of 3 or more takes the next id of the board's counter (+0xe1c) before it is judged, whether
// it is kept or not. A run is held back (the board is searched again in the next update) when, from any of its gems, in
// the direction of the run up to 7 cells of the gem's colour in a row hold a gem that could still settle there (a bVar5 of
// the exe: not at rest, but free: not held by a swap, able to fall, no busy count, no curve running, no bit 0x2000), or
// when, across the run, the same-colour cells on both sides of the gem number 2 or more and one of them could still settle.
// A kept run's gems take the highest tag (+0x188) among them and the run's id (+0x190), and the run scores
// (X - 3 + n) * 50 (X: how many matches the move has made, this one counted, at least 1) and (n - 4) * 350 more from 5 gems,
// through AddPoints. port: left out are the set of 0x75b0xx that holds a run back (local_224 of the exe, its meaning not
// found) and the swap-list flag (+0xf9) of the same test, the flame, star and hypercube that a run of 4, 5 or a crossing
// makes (the lists of 0x75c0xx to 0x75cdxx), the pieces with bits (0x1, 0x4, 0x10, 0x400, 0x80000), the speed bonus, the
// sounds, the pulse curve of 0x85f0a4 and 0x85f0d0 on a matched gem (it keeps 0x755ad0 false for the gem: here the match
// timer does) and the effect objects. The stats of 0x73d170 are only kept for the count of matches (stat 0x1a)
int Board::FindMatches()
{
	struct Run {
		std::vector<Piece*> mPieces;
		bool mAlongRow;
	};
	std::vector<Run> aRuns;
	for (int aPass = 0; aPass < 2; aPass++) {
		for (int aOuter = 0; aOuter < 8; aOuter++) {
			int aRunColor = -1;
			std::vector<Piece*> aRun;
			for (int aInner = 0; aInner <= 8; aInner++) {
				Piece* aPiece = nullptr;
				int aColor = -1;
				if (aInner < 8) {
					aPiece = aPass == 0 ? mGrid[aInner][aOuter] : mGrid[aOuter][aInner];
					if (aPiece != nullptr && aPiece->mMatchTimer == 0.0f && PieceIsSettled(aPiece))
						aColor = aPiece->mColor;
					else
						aPiece = nullptr;
				}
				if (aColor != -1 && aColor == aRunColor) {
					aRun.push_back(aPiece);
					continue;
				}
				if (aRun.size() >= 3) {
					Run aFound;
					aFound.mPieces = aRun;
					aFound.mAlongRow = aPass == 1;
					aRuns.push_back(aFound);
				}
				aRun.clear();
				aRunColor = aColor;
				if (aColor != -1)
					aRun.push_back(aPiece);
			}
		}
	}
	int aCount = 0;
	for (size_t aRunIndex = 0; aRunIndex < aRuns.size(); aRunIndex++) {
		Run& aRun = aRuns[aRunIndex];
		int aRunId = mMatchCounter++;
		// the gems that were taken by an earlier run of this search are still there for this one, as in the exe
		bool aHold = false;
		static const int DIRECTIONS[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
		for (size_t k = 0; k < aRun.mPieces.size() && !aHold; k++) {
			Piece* aGem = aRun.mPieces[k];
			bool aAlongCouldSettle = false;
			bool aAcrossCouldSettle = false;
			int aAcrossCount = 0;
			for (int aDirection = 0; aDirection < 4; aDirection++) {
				// directions 0 and 1 go down and up a column, 2 and 3 along a row; the run's own direction is the first pair
				// in the first pass (down a column) and the second pair in the second
				bool aInRunDirection = (aDirection / 2 == (aRun.mAlongRow ? 1 : 0));
				for (int aStep = 1; aStep < 8; aStep++) {
					int aRow = aGem->mRow + DIRECTIONS[aDirection][0] * aStep;
					int aCol = aGem->mCol + DIRECTIONS[aDirection][1] * aStep;
					if (aRow < 0 || aRow >= ROWS || aCol < 0 || aCol >= COLS)
						break;
					Piece* aNeighbour = mGrid[aRow][aCol];
					if (aNeighbour == nullptr || aNeighbour->mColor != aGem->mColor)
						break;
					bool aCouldSettle = !PieceIsSettled(aNeighbour) && !IsHeldBySwap(aNeighbour) && aNeighbour->mCanFall &&
						aNeighbour->mBusyCount == 0 && (aNeighbour->mFlags & 0x2000) == 0;
					if (aInRunDirection)
						aAlongCouldSettle = aAlongCouldSettle || aCouldSettle;
					else {
						aAcrossCount++;
						aAcrossCouldSettle = aAcrossCouldSettle || aCouldSettle;
					}
				}
			}
			if (aAlongCouldSettle || (aAcrossCount >= 2 && aAcrossCouldSettle))
				aHold = true;
		}
		if (aHold)
			continue;
		aCount++;
		PlainClear aGroup;
		aGroup.mOriginId = aRun.mPieces[0]->mId;
		aGroup.mUpdate = mUpdateCnt;
		int aChainId = -1;
		for (size_t k = 0; k < aRun.mPieces.size(); k++)
			aChainId = std::max(aChainId, aRun.mPieces[k]->mColumnTag);
		aGroup.mChainId = aChainId;
		for (size_t k = 0; k < aRun.mPieces.size(); k++) {
			Piece* aPiece = aRun.mPieces[k];
			aPiece->mColumnTag = aChainId;
			aPiece->mGroupId = aRunId;
			if ((aPiece->mFlags & 4) != 0 && aPiece->mMatchedUpdate != mUpdateCnt) {
				// 0x75bac5 to 0x75bb0f: a flame gem (bit 4) that was not matched in this update gets a group of its own, unless it has
				// one; it takes no part in the plain gems' clear. The group is type 3, or 2 when the board's +0xe8 is set; a piece with
				// bit 1 makes it type 6 (0x789a90). port: the counter of 0x73d170 (stat 1, the move's tag) and the box that 0x742d00
				// shows for a type 2 or 7 group are left out; nothing translated makes a flame gem
				if (FindMatchGroup(aPiece) == -1)
					MakeMatchGroup(aPiece, 3 - (mGroupTypeSwitch != 0 ? 1 : 0));
				continue;
			}
			aPiece->mMatchedUpdate = mUpdateCnt;
			if (aPiece->mMatchTimer == 0.0f) {
				// port: stands in for the pulse that holds the gem while it is cleared (see UpdatePlainClears)
				aPiece->mMatchTimer = 0.89999998f;
				aGroup.mPieces.push_back(aPiece);
				aGroup.mPieceIds.push_back(aPiece->mId);
			}
		}
		int aPieceCount = (int)aRun.mPieces.size();
		MoveRecord* aRecord = FindMoveRecord(aChainId);
		if (aRecord != nullptr)
			aRecord->mMatchCount++;
		int aMatchesInMove = aRecord != nullptr ? aRecord->mMatchCount : 0;
		if (aMatchesInMove < 1)
			aMatchesInMove = 1;
		int aPoints = (aMatchesInMove - 3 + aPieceCount) * 0x32;
		if (aPieceCount >= 5)
			aPoints += (aPieceCount - 4) * 0x15e;
		AddPoints(aPoints);
		if (!aGroup.mPieces.empty())
			mPlainClears.push_back(aGroup);
	}
	return aCount;
}

// 0x789a90 (bejeweled3), for every type but 7 (the hypercube's, which 0x7870f0 runs: not translated): a group for a piece, made
// by 0x75af60 (a flame gem, bit 4, in a match) and by 0x74c570, and pushed at the end of the list at +0x125c. It takes the piece's
// place at the middle of the cell, its flags, tag (+0x188) and run id (+0x190), puts the piece's id first in its list, sets the
// piece's +0x174 and puts its timer (+0x178) at 0.9. A piece with bit 1 makes it type 6. No Rand() is drawn here (the two
// streaks it makes, 0x789860, take their places from the cell and draw only when they run). port: the streaks (their own
// update, 0x786dd0, draws the app's Rand() every fifth update, 0x78a390 runs it for each), the curves that the group sets
// ("b+0,1.5,..." for a type 6 group) and the fields that only they read (+0x10, +0x18, +0x20, +0x24, +0x40, +0x44, +0x50 to
// +0x270) are not translated
void Board::MakeMatchGroup(Piece* thePiece, int theType)
{
	std::unique_ptr<MatchGroup> aGroup(new MatchGroup());
	aGroup->mType = (thePiece->mFlags & 1) == 0 ? theType : 6;
	aGroup->mAge = 0;
	aGroup->mFade = 1.0f;
	aGroup->mLifeCountdown = 0;
	aGroup->mFlags = thePiece->mFlags;
	aGroup->mTag = thePiece->mColumnTag;
	aGroup->mRunId = thePiece->mGroupId;
	aGroup->mOriginId = thePiece->mId;
	aGroup->mPieceIds.push_back(thePiece->mId);
	// the piece's place and 64, less the grid's corner, which slots 171 and 172 (0x73bd80 and 0x73bd70) also add
	float aSumX = (float)((float)GetGridX() + thePiece->mX);
	float aMiddleX = (float)((double)aSumX + 64.0);
	aGroup->mPixelX = Ftol((double)aMiddleX - (double)GetGridX());
	float aSumY = (float)((float)GetGridY() + thePiece->mY);
	float aMiddleY = (float)((double)aSumY + 64.0);
	aGroup->mPixelY = Ftol((double)aMiddleY - (double)GetGridY());
	thePiece->mIsGroupOrigin = true;
	thePiece->mMatchTimer = 0.89999998f;
	aGroup->mRingTimer = 0.0f;
	aGroup->mLife = 1.0f;
	aGroup->mRing = 0;
	aGroup->mMaxDistance = (aGroup->mType != 3 ? 4 : 0) + 3;
	mMatchGroups.push_back(std::move(aGroup));
}

// 0x73c9d0 (bejeweled3)
int Board::FindMatchGroup(const Piece* thePiece) const
{
	for (size_t i = 0; i < mMatchGroups.size(); i++) {
		if (mMatchGroups[i]->mOriginId == thePiece->mId)
			return (int)i;
	}
	return -1;
}

// slot 200, 0x74c570 (bejeweled3): theBy (the exe's third argument; 0x75dee0 passes the piece itself) takes the piece out. A piece
// that another clear holds is not touched. A flame gem (bit 4) without a group gets one of type 2 and the clear waits for it; a
// piece with bit 0x10, bit 0x80000, or bit 1 without bit 4 gets busy count 1; a plain piece is not taken (false). The piece then
// notes what cleared it. port: a piece with bit 2 and no group goes to slot 0x2d0 (0x745bf0's kind of effect), which is not
// translated, and the counter of 0x73d170 (stat 1) that a flame gem's group adds is left out: nothing translated gives a piece
// bit 1, 2 or 0x10
bool Board::ClearPiece(Piece* thePiece, Piece* theBy)
{
	if (thePiece->mClearHeld)
		return false;
	int aFlags = thePiece->mFlags;
	if ((aFlags & 1) == 0 || (aFlags & 4) != 0) {
		if ((aFlags & 0x80000) == 0) {
			if ((thePiece->mFlags & 4) != 0 && FindMatchGroup(thePiece) == -1) {
				thePiece->mClearHeld = true;
				MakeMatchGroup(thePiece, 2);
				return true;
			}
			if ((thePiece->mFlags & 0x10) == 0)
				return false;
			thePiece->mClearHeld = true;
			thePiece->mBusyCount = 1;
		}
		else
			thePiece->mBusyCount = 1;
	}
	else
		thePiece->mBusyCount = 1;
	thePiece->mDestroyerId = theBy != nullptr ? theBy->mId : -1;
	if (theBy == nullptr)
		thePiece->mDestroyerFlags = -1;
	else
		thePiece->mDestroyerFlags |= theBy->mFlags;
	return true;
}

// 0x75dee0 (bejeweled3), the draws of the app's Rand() that the 20 particles one of its rings makes for a piece it finds: each is
// made by 0x729c20 with type 0xd, whose constructor (0x729460) draws 3 (a count, two angles), and then the ring sets its place
// and speed with 4 more. The values are dropped
void Board::DrawRingParticleRandoms()
{
	for (int aParticle = 0; aParticle < 20; aParticle++) {
		for (int aDraw = 0; aDraw < 3 + 4; aDraw++)
			Rand();
	}
}

// 0x75dee0 (bejeweled3), run by 0x767e20 at its end: the glow, then each match group in the list (see MatchGroup). In its own
// update (0x78a390) a group counts its age, steps its curves and runs its streaks. Then, for a group below type 7:
//  - the pieces of its list from the second on, the ones with +0x182 set, take 0.015 (0.01 when slot 0x1c4, 0x73c1f0, is true:
//    the Zen and Butterflies boards) on their timers (+0x178); a piece at 1 or more is let go (that is, it leaves the list):
//    unless another clear holds it, it takes the group's tag as a minimum, the group's run id, the group's first piece's id and
//    the group's flags, and gets busy count 1 (UpdateGame destroys it in the next update; slot 320, 0x74c570, may take it
//    instead), or 5 with bit 0x10; a piece below 1 keeps the group from its last ring
//  - the ring cells go on by the same step and leave at 1
//  - the ring timer goes down by 0.01 and at 0 or below makes a ring: the cells at the group's ring distance from its first piece in
//    the directions its type gives (0: along the row, 1: along the column, 4: all eight, 5: the border of a square, the others
//    the four sides) are looked at; each cell on the board but the first piece's adds a ring cell, and the piece there (unless
//    a clear holds it or any group's list has it) keeps the group from its last ring and, when its timer is 0, joins the group's
//    list with timer 0.01 and 20 particles. The ring's distance counts up; once it is at the group's farthest (3 for a type 3
//    group, else 7) and every piece found was already in a group, each ring clears the first piece's bit 4, sets it busy (or has
//    slot 320 take it), takes 0.25 off the group's life and stops all the pieces on the board; at life 0 the group ends
//  - it also ends when its countdown (+0x2e4) runs out, and then the fill is asked for if no group is left
// A group's age gives the others a turn: the loop stops at a group whose age is below 25 times the number of groups before it.
// port: the sound that the first ring starts, the particles themselves (their draws are DrawRingParticleRandoms), the children of
// 0x78a390 and what 0x76aaf0 spawns for a type 6 group are not translated
void Board::UpdateMatchGroups()
{
	UpdatePlainClears();
	const bool aSlowMode = false;   // slot 0x1c4 (0x73c1f0): 0 in every board translated so far
	const bool aReplayFlag = false;   // +0x12f8: the saved-game and replay mode, which Classic's new game does not start
	float aGlow = (float)((double)mGroupGlow - 0.019999999552965164);
	if (aGlow <= 0.0f)
		aGlow = 0.0f;
	mGroupGlow = aGlow;
	if (mMatchGroups.empty())
		return;
	double aSum = (double)aGlow + 0.05000000074505806;
	mGroupGlow = 1.0 <= aSum ? 1.0f : (float)aSum;
	int aTurnLimit = 0;
	size_t aIndex = 0;
	do {
		MatchGroup* aGroup = mMatchGroups[aIndex].get();
		// 0x78a390
		aGroup->mAge++;
		float aFade = (float)((double)aGroup->mFade - 0.009999999776482582);
		aGroup->mFade = aFade < 0.0f ? 0.0f : aFade;
		bool aEnded = false;
		if ((unsigned)aGroup->mType < 7) {
			int aSweep = aGroup->mType == 6 ? 1 : 0;
			bool aAllLetGo = true;
			for (size_t k = 1; k < aGroup->mPieceIds.size(); k++) {
				Piece* aPiece = FindPieceById(aGroup->mPieceIds[k]);
				if (aPiece != nullptr && aPiece->mClearable) {
					aPiece->mMatchTimer = (float)((double)aPiece->mMatchTimer + (aSlowMode ? 0.009999999776482582 : 0.014999999664723873));
					if (aPiece->mMatchTimer >= 1.0f) {
						if (!aPiece->mClearHeld) {
							aPiece->mColumnTag = std::max(aPiece->mColumnTag, aGroup->mTag);
							aPiece->mGroupId = aGroup->mRunId;
							aPiece->mDestroyerId = aGroup->mOriginId;
							aPiece->mDestroyerFlags |= aGroup->mFlags;
							if ((aPiece->mFlags & 0x10) == 0) {
								if (!ClearPiece(aPiece, aPiece) && !aPiece->mClearHeld)
									aPiece->mBusyCount = 1;
							}
							else
								aPiece->mBusyCount = 5;
						}
						aGroup->mPieceIds.erase(aGroup->mPieceIds.begin() + k);
						k--;
					}
					else
						aAllLetGo = false;
				}
			}
			for (size_t k = 0; k < aGroup->mRingCells.size(); k++) {
				RingCell& aCell = aGroup->mRingCells[k];
				aCell.mTimer = (float)((double)aCell.mTimer + (aSlowMode ? 0.009999999776482582 : 0.014999999664723873));
				if (aCell.mTimer >= 1.0f) {
					// slot 0x314 (0x73bb40) is called with the cell and returns at once
					aGroup->mRingCells.erase(aGroup->mRingCells.begin() + k);
					k--;
				}
			}
			float aRingTimer = (float)((double)aGroup->mRingTimer - 0.009999999776482582);
			aGroup->mRingTimer = aRingTimer;
			if (aRingTimer <= 0.0f) {
				aGroup->mRingTimer = aSlowMode ? 0.15000000596046448f : 0.10000000149011612f;
				// the eight directions, as x and y; each type uses a run of them (type 5 builds its own list)
				int aDirections[2 * 64] = { 1, 0, -1, 0, 0, 1, 0, -1, -1, -1, -1, 1, 1, -1, 1, 1 };
				int aFirstDirection = 0;
				int aDirectionCount = 4;
				if (aGroup->mType == 0)
					aDirectionCount = 2;
				if (aGroup->mType == 1)
					aFirstDirection = 2;
				if (aGroup->mType == 4)
					aDirectionCount = 8;
				if (aGroup->mType == 5) {
					int aRadius = aGroup->mRing;
					if (aRadius > 6)
						aRadius = 7;
					int aNegative = -aRadius;
					aDirectionCount = 0;
					for (int aStep = aNegative; aStep <= aRadius; aStep++) {
						aDirections[aDirectionCount * 2] = aStep;
						aDirections[aDirectionCount * 2 + 1] = aNegative;
						aDirections[(aDirectionCount + 1) * 2] = aStep;
						aDirections[(aDirectionCount + 1) * 2 + 1] = aRadius;
						aDirectionCount += 2;
					}
					for (int aStep = aNegative + 1; aStep <= aRadius - 1; aStep++) {
						aDirections[aDirectionCount * 2 + 1] = aStep;
						aDirections[aDirectionCount * 2] = aNegative;
						aDirections[(aDirectionCount + 1) * 2 + 1] = aStep;
						aDirections[(aDirectionCount + 1) * 2] = aRadius;
						aDirectionCount += 2;
					}
				}
				for (int aDirection = aFirstDirection; aDirection < aDirectionCount; aDirection++) {
					int aDeltaX = aDirections[aDirection * 2];
					int aDeltaY = aDirections[aDirection * 2 + 1];
					for (int aSide = -aSweep; aSide <= aSweep; aSide++) {
						int aDistance = aGroup->mType == 5 ? 1 : aGroup->mRing;
						unsigned aX = (unsigned)((aDeltaX * aDistance + aDeltaY * aSide) * 0x80 + aGroup->mPixelX);
						unsigned aY = (unsigned)((aDeltaY * aDistance + aDeltaX * aSide) * 0x80 + aGroup->mPixelY);
						Piece* aFound = GetPieceAt((float)(int)aX, (float)(int)aY);   // 0x73bc50
						if (aDistance <= aGroup->mMaxDistance && aX < 0x400 && aY < 0x400) {
							if (aX != (unsigned)aGroup->mPixelX || aY != (unsigned)aGroup->mPixelY) {
								RingCell aCell;
								aCell.mCol = (int)(aX >> 7);
								aCell.mRow = (int)(aY >> 7);
								aCell.mTimer = 0.0099999998f;
								aGroup->mRingCells.push_back(aCell);
							}
							if (aFound != nullptr && !aFound->mClearHeld) {
								bool aClaimed = false;
								for (size_t g = 0; g < mMatchGroups.size(); g++) {
									for (size_t m = 0; m < mMatchGroups[g]->mPieceIds.size(); m++) {
										if (aFound->mId == mMatchGroups[g]->mPieceIds[m])
											aClaimed = true;
									}
								}
								if (!aClaimed) {
									aAllLetGo = false;
									if (aFound->mMatchTimer == 0.0f) {
										aGroup->mPieceIds.push_back(aFound->mId);
										aFound->mMatchTimer = 0.0099999998f;
										DrawRingParticleRandoms();
									}
									float aHighest = aFound->mRingMark <= aFound->mMatchTimer ? aFound->mMatchTimer : aFound->mRingMark;
									aFound->mRingMark = aHighest <= 1.0f ? aHighest : 1.0f;
								}
							}
						}
					}
				}
				// port: with no ring made yet (+0x48 is 0) the exe starts a sound through the app here
				aGroup->mRing++;
				if (aGroup->mMaxDistance <= aGroup->mRing && aAllLetGo) {
					mRingFlash = 1.0f;
					Piece* aFirst = aGroup->mPieceIds.empty() ? nullptr : FindPieceById(aGroup->mPieceIds[0]);
					if (aFirst != nullptr) {
						aFirst->mFlags &= ~4;
						if ((aFirst->mFlags & 2) == 0) {
							// slot 0x314 with the piece's cell, which returns at once
							aFirst->mDestroyerFlags |= aFirst->mFlags;
							aFirst->mBusyCount = 1;
							aFirst->mDestroyerId = aFirst->mId;
						}
						else
							ClearPiece(aFirst, aFirst);
					}
					aGroup->mLife = (float)((double)aGroup->mLife - 0.25);
					for (int aRow = 0; aRow < ROWS; aRow++) {
						for (int aCol = 0; aCol < COLS; aCol++) {
							if (mGrid[aRow][aCol] != nullptr)
								mGrid[aRow][aCol]->mFallSpeed = 0.0f;
						}
					}
					if (aGroup->mLife <= 0.0f)
						aEnded = true;
				}
			}
		}
		bool aCountdownEnded = false;
		if (aGroup->mLifeCountdown > 0) {
			aGroup->mLifeCountdown--;
			aCountdownEnded = aGroup->mLifeCountdown == 0;
		}
		if (aCountdownEnded || aEnded) {
			mMatchGroups.erase(mMatchGroups.begin() + aIndex);
			if (mMatchGroups.empty() && !aReplayFlag)
				FillInBlanks(true);   // slot 203 with 1, as the asm pushes
			aIndex--;
		}
		else {
			if (aGroup->mAge < aTurnLimit)
				break;
			aTurnLimit += 0x19;
		}
		aIndex++;
	} while ((int)aIndex < (int)mMatchGroups.size());
}

// port: the stand-in for a plain match's clear (see PlainClear), which the exe does not make a group for. Each piece's timer
// (+0x178, from 0.9) rises by 0.015 every update and, when it is past 1, the piece's busy count is set to 1 (UpdateGame
// destroys it in the next update) and it leaves the list; the clear ends in the update its last piece is let go. A group lets
// a piece go at 1 or more (UpdateMatchGroups); this keeps the port's earlier steps
void Board::UpdatePlainClears()
{
	for (size_t i = 0; i < mPlainClears.size();) {
		PlainClear& aGroup = mPlainClears[i];
		for (size_t k = 0; k < aGroup.mPieceIds.size();) {
			Piece* aPiece = FindPieceById(aGroup.mPieceIds[k]);
			if (aPiece == nullptr) {
				aGroup.mPieceIds.erase(aGroup.mPieceIds.begin() + k);
				continue;
			}
			aPiece->mMatchTimer = (float)((double)aPiece->mMatchTimer + 0.014999999664723873);
			if (aPiece->mMatchTimer > 1.0f) {
				aPiece->mColumnTag = std::max(aPiece->mColumnTag, aGroup.mChainId);
				aPiece->mBusyCount = 1;
				aGroup.mPieceIds.erase(aGroup.mPieceIds.begin() + k);
				continue;
			}
			k++;
		}
		// the clear ends when its pieces are gone
		if (aGroup.mPieceIds.empty()) {
			bool aStillThere = false;
			for (size_t k = 0; k < aGroup.mPieces.size() && !aStillThere; k++)
				aStillThere = FindPieceById(aGroup.mPieces[k]->mId) == aGroup.mPieces[k] && aGroup.mPieces[k]->mBusyCount > 0;
			if (!aStillThere) {
				mPlainClears.erase(mPlainClears.begin() + i);
				continue;
			}
		}
		i++;
	}
}

// 0x74c770 (bejeweled3), for a plain piece: 50 points for it (a piece without the bits 0x1800 calls slot 152, 0x74df80, at
// 0x74c87e with 0x32 as the points; ClassicBoard's vtable 0x86511c holds AddPoints at 0x260, and its other callers are the
// special gems' own: 0x73d380, 0x73f8e0, 0x749650, 0x74c070 and three in 0x767e20); then the sparks are made, which draw
// from the app's generator (DrawSparkRandoms); then it goes from the grid. port: the sparks themselves, the sounds and the
// bits of the flags are not translated. The points text that AddPoints makes (0x71f030, its effect 0x71e5f0 draws 5 times
// in a loop) is not translated either, so its draws are missing too
void Board::DestroyPiece(Piece* thePiece)
{
	if ((thePiece->mFlags & FLAGS_SPECIAL_SWAP) == 0)
		AddPoints(0x32);
	DrawSparkRandoms();
	RemovePiece(thePiece);
}

// 0x74c770 (bejeweled3), the draws of the app's Rand() (0x439a90, the Mersenne twister at 0x5865b0) that the sparks of a
// plain gem make, in the exe's order and count, for a gem at rest (so its place equals the place the caller passes: the
// speed 0x74c770 reads from their difference is 0, and the 25 sparks of the middle ring take their short branch) in a
// board whose slot 0x1c4 (0x73c1f0) is 0, with no bit 0x400, and a match timer (+0x178) of 0. Each spark is made by
// 0x729c20, whose constructor 0x729460 draws first (a draw by 0x712da0 is one Rand()):
//   14 sparks of type 0xe: the constructor draws 2, the loop 5 (angle, radius, x, y, size)         = 7 each
//   25 sparks of type 0xb: the constructor draws 2 (0xb: one Rand() and one 0x712da0), the loop 8                      = 10 each
//   14 sparks of type 0xe: the constructor draws 2, the loop 4                                                        = 6 each
// = 432 draws. port: a gem with a match timer above 0 makes 15 more sparks of type 10 (3 draws and the constructor's), a
// zero speed is not checked, and the draws' values are dropped
void Board::DrawSparkRandoms()
{
	const int SPARK_DRAWS = 14 * 7 + 25 * 10 + 14 * 6;
	for (int i = 0; i < SPARK_DRAWS; i++)
		Rand();
}

// 0x755b80 (bejeweled3): any swap of the piece is dropped; every piece above in its column takes the piece's move id when
// it is higher, and so does the column's tag, which the fill gives to the pieces it makes; the cell is emptied
void Board::RemovePiece(Piece* thePiece)
{
	for (size_t i = 0; i < mSwaps.size();) {
		SwapRecord& aSwap = mSwaps[i];
		if (aSwap.mFirst == thePiece || aSwap.mSecond == thePiece) {
			// the other piece goes where it is meant to be
			Piece* aOther = aSwap.mFirst == thePiece ? aSwap.mSecond : aSwap.mFirst;
			if (aOther != nullptr) {
				aOther->mX = (float)(aOther->mCol << 7);
				aOther->mY = (float)(aOther->mRow << 7);
			}
			mSwaps.erase(mSwaps.begin() + i);
			continue;
		}
		i++;
	}
	for (int aRow = 0; aRow < thePiece->mRow; aRow++) {
		Piece* aAbove = mGrid[aRow][thePiece->mCol];
		if (aAbove != nullptr)
			aAbove->mColumnTag = std::max(thePiece->mColumnTag, aAbove->mColumnTag);
	}
	mColumnTag[thePiece->mCol] = std::max(mColumnTag[thePiece->mCol], thePiece->mColumnTag);
	mGrid[thePiece->mRow][thePiece->mCol] = nullptr;
	if (mSelectedAtMouseUp == thePiece)
		mSelectedAtMouseUp = nullptr;
	delete thePiece;
}

// 0x74df80 with 0x74a210 (bejeweled3): the points times the level number (+0xe0c) go up in steps of 10, each worth the
// level's multiplier (+0xe04 + 1), to the score at +0xd24. port: the text of the points that rises from the place, the
// bonus of +0xe00 and the counters of 0x73d170 (stat 1) are left out
void Board::AddPoints(int thePoints)
{
	if (thePoints < 1)
		return;
	int aLeft = Ftol((double)((float)thePoints * (float)mLevelNumber));
	while (aLeft > 0) {
		int aChunk = aLeft < 10 ? aLeft : 10;
		aLeft -= aChunk;
		int aGain = Ftol((double)aChunk * (double)((float)mLevelIndex + 1.0f));
		mScorePoints += aGain;
		if (mScorePoints < 0)
			mScorePoints = 0x7fffffff;
	}
}

// 0x73b6d0 (bejeweled3): every fourth update the shown score goes a fifth of the way to the score, and 1 more
void Board::UpdateShownScore()
{
	if ((mUpdateCnt & 0x80000003) != 0)
		return;
	if (mScore < mScorePoints)
		mScore = mScore - Ftol((double)(mScorePoints - mScore) * -0.20000000298023224) + 1;
	else if (mScore > mScorePoints)
		mScore = mScore - Ftol((double)(mScorePoints - mScore) * -0.20000000298023224) - 1;
}

}  // namespace Sexy
