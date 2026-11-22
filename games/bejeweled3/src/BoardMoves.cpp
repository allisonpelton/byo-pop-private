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

// 0x75af60 (bejeweled3), for plain gems. The board is read twice, row by row and then column by column: a run is gems
// of one colour that are all at rest in their cells (0x755ad0) and none has no colour. A run of 3 or more is a match;
// runs that share a gem make one match. A run waits (nothing is made) while a gem of its colour that is not at rest, but
// could be, lies in line with it. The match's pieces carry the highest move id among them, and the match scores
// (X - 3 + n) * 50 points, X being how many matches the move has made counting this one (at least 1), n the pieces, with
// (n - 4) * 350 more from 5 pieces. port: left out are the runs' neighbours across the run (their own test of 0x75b5xx
// that two of them in a row may hold a match back), the flame, star and hypercube that a run of 4, 5 or a crossing makes,
// the pieces with bits (0x1, 0x4, 0x10, 0x400, 0x80000), the speed bonus, the lists of 0x75bxxx for the sounds, the
// text of the points (it is shown by the exe where it is made) and the effect objects (their pictures and the rings that
// hit gems around the match). The stats 0xc84 + idx * 4 of 0x73d170 are only kept for the counter of matches
int Board::FindMatches()
{
	struct Run {
		int mStartRow;
		int mStartCol;
		int mLength;
		bool mAcross;
		std::vector<Piece*> mPieces;
	};
	std::vector<Run> aRuns;
	for (int aPass = 0; aPass < 2; aPass++) {
		for (int aLine = 0; aLine < 8; aLine++) {
			int aRunColor = -1;
			int aRunLength = 0;
			int aRunStart = 0;
			for (int aPosition = 0; aPosition <= 8; aPosition++) {
				Piece* aPiece = nullptr;
				int aColor = -1;
				if (aPosition < 8) {
					aPiece = aPass == 0 ? mGrid[aLine][aPosition] : mGrid[aPosition][aLine];
					if (aPiece != nullptr && PieceIsSettled(aPiece) && aPiece->mMatchTimer == 0.0f)   // a piece already matched is not matched again
						aColor = aPiece->mColor;
					else
						aPiece = nullptr;
				}
				if (aPosition < 8 && aColor != -1 && aColor == aRunColor) {
					aRunLength++;
					continue;
				}
				if (aRunLength >= 3) {
					Run aRun;
					aRun.mAcross = aPass == 0;
					aRun.mStartRow = aPass == 0 ? aLine : aRunStart;
					aRun.mStartCol = aPass == 0 ? aRunStart : aLine;
					aRun.mLength = aRunLength;
					for (int k = 0; k < aRunLength; k++)
						aRun.mPieces.push_back(aPass == 0 ? mGrid[aLine][aRunStart + k] : mGrid[aRunStart + k][aLine]);
					aRuns.push_back(aRun);
				}
				aRunColor = aColor;
				aRunLength = aColor == -1 ? 0 : 1;
				aRunStart = aPosition;
			}
		}
	}
	// the runs that share a piece are one match
	std::vector<std::vector<Piece*>> aMatches;
	std::vector<bool> aWaiting;
	for (size_t aRunIndex = 0; aRunIndex < aRuns.size(); aRunIndex++) {
		Run& aRun = aRuns[aRunIndex];
		bool aHold = false;
		// a gem of the colour in line with the run (either end, up to 7 away) that is moving but free to settle holds it back
		int aStepCol = aRun.mAcross ? 1 : 0;
		int aStepRow = aRun.mAcross ? 0 : 1;
		for (int aEnd = 0; aEnd < 2 && !aHold; aEnd++) {
			int aRow = aEnd == 0 ? aRun.mStartRow - aStepRow : aRun.mStartRow + aStepRow * aRun.mLength;
			int aCol = aEnd == 0 ? aRun.mStartCol - aStepCol : aRun.mStartCol + aStepCol * aRun.mLength;
			for (int aStep = 0; aStep < 7; aStep++) {
				if (aRow < 0 || aRow >= ROWS || aCol < 0 || aCol >= COLS)
					break;
				Piece* aNeighbour = mGrid[aRow][aCol];
				if (aNeighbour == nullptr || aNeighbour->mColor != aRun.mPieces[0]->mColor)
					break;
				if (!PieceIsSettled(aNeighbour) && aNeighbour->mCanFall && aNeighbour->mBusyCount == 0 &&
					(aNeighbour->mFlags & 0x2000) == 0 && !IsHeldBySwap(aNeighbour)) {
					aHold = true;
					break;
				}
				aRow += aEnd == 0 ? -aStepRow : aStepRow;
				aCol += aEnd == 0 ? -aStepCol : aStepCol;
			}
		}
		if (aHold)
			continue;
		// join it to a match that has one of its pieces, or begin one
		int aJoined = -1;
		for (size_t aMatchIndex = 0; aMatchIndex < aMatches.size(); aMatchIndex++) {
			std::vector<Piece*>& aMatch = aMatches[aMatchIndex];
			bool aShares = false;
			for (size_t k = 0; k < aRun.mPieces.size() && !aShares; k++)
				aShares = std::find(aMatch.begin(), aMatch.end(), aRun.mPieces[k]) != aMatch.end();
			if (!aShares)
				continue;
			if (aJoined == -1) {
				aJoined = (int)aMatchIndex;
				for (size_t k = 0; k < aRun.mPieces.size(); k++) {
					if (std::find(aMatch.begin(), aMatch.end(), aRun.mPieces[k]) == aMatch.end())
						aMatch.push_back(aRun.mPieces[k]);
				}
			}
			else {
				// the run joins two matches: they are one
				std::vector<Piece*>& aFirstMatch = aMatches[aJoined];
				for (size_t k = 0; k < aMatch.size(); k++) {
					if (std::find(aFirstMatch.begin(), aFirstMatch.end(), aMatch[k]) == aFirstMatch.end())
						aFirstMatch.push_back(aMatch[k]);
				}
				aMatch.clear();
			}
		}
		if (aJoined == -1)
			aMatches.push_back(aRun.mPieces);
	}
	int aCount = 0;
	for (size_t aMatchIndex = 0; aMatchIndex < aMatches.size(); aMatchIndex++) {
		std::vector<Piece*>& aMatch = aMatches[aMatchIndex];
		if (aMatch.empty())
			continue;
		aCount++;
		MatchGroup aGroup;
		aGroup.mOriginId = aMatch[0]->mId;
		aGroup.mUpdate = mUpdateCnt;
		int aChainId = -1;
		for (size_t k = 0; k < aMatch.size(); k++)
			aChainId = std::max(aChainId, aMatch[k]->mColumnTag);
		aGroup.mChainId = aChainId;
		mMatchCounter++;   // 0x75b3e9: the match takes an id of its own
		for (size_t k = 0; k < aMatch.size(); k++) {
			Piece* aPiece = aMatch[k];
			aPiece->mColumnTag = aChainId;
			aPiece->mMatchedUpdate = mUpdateCnt;
			// 0x789a90: the match's piece is marked at 0.9 and the match lets it go when it passes 1
			aPiece->mMatchTimer = 0.89999998f;
			aGroup.mPieces.push_back(aPiece);
			aGroup.mPieceIds.push_back(aPiece->mId);
		}
		// the score: 0x75d86f to 0x75d965
		int aPieceCount = (int)aMatch.size();
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

// 0x74c770 (bejeweled3), for a plain piece: 50 points for it (a piece without the bits 0x1800 calls slot 152, 0x74df80, at 0x74c87e with 0x32 as the
// points; ClassicBoard's vtable 0x86511c holds AddPoints at 0x260, and its other callers are the special gems' own: 0x73d380,
// 0x73f8e0, 0x749650, 0x74c070 and three in 0x767e20), then it goes from the grid. port: the pieces' sparks (the
// app's Rand() is drawn for them: 14 times 5 for the first ring, 25 times about 13, 14 times 6, and more) and the sounds,
// the pictures of the destruction and the bits of the flags are not translated, so the app's random numbers are not drawn
// as the exe draws them
void Board::DestroyPiece(Piece* thePiece)
{
	if ((thePiece->mFlags & FLAGS_SPECIAL_SWAP) == 0)
		AddPoints(0x32);
	RemovePiece(thePiece);
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
