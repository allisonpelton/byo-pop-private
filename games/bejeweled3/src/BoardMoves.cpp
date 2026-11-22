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
	if (!mMatchGroups.empty())
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
	if (!mMatchGroups.empty())
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
		MatchGroup aGroup;
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
			aPiece->mMatchedUpdate = mUpdateCnt;
			if (aPiece->mMatchTimer == 0.0f) {
				// port: stands in for the pulse that holds the gem while it is cleared (see UpdateMatchGroups)
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
			mMatchGroups.push_back(aGroup);
	}
	return aCount;
}

// 0x75dee0 (bejeweled3). port: what a match does here is only the part that frees its pieces: the exe lets each one's timer
// (+0x178, from 0.9) rise by 0.015 every update and, when it is past 1, sets the piece's busy count to 1 (UpdateGame
// destroys it in the next update) and takes it off the match's list. The match itself lives on in the exe as an effect for
// about 50 updates (rings out of its first piece that hit the pieces up to 3 away at every tenth update, a +0x30 that
// goes down by 0.25 at each ring from the third), which is not translated: here the match ends in the update its last
// piece is let go. The rings, the particles (the app's Rand() is drawn some 150 times for them) and the sounds are left out
void Board::UpdateMatchGroups()
{
	for (size_t i = 0; i < mMatchGroups.size();) {
		MatchGroup& aGroup = mMatchGroups[i];
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
		// the match ends when its pieces are gone
		if (aGroup.mPieceIds.empty()) {
			bool aStillThere = false;
			for (size_t k = 0; k < aGroup.mPieces.size() && !aStillThere; k++)
				aStillThere = FindPieceById(aGroup.mPieces[k]->mId) == aGroup.mPieces[k] && aGroup.mPieces[k]->mBusyCount > 0;
			if (!aStillThere) {
				mMatchGroups.erase(mMatchGroups.begin() + i);
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
