// The player's move on the board and what it sets off (Classic, plain gems): picking a gem up and swapping (0x73f140,
// 0x73c510, 0x73d990, 0x75fae0, 0x75ee20), the swap's animation and the swap back (0x759ba0), the match search (0x75af60),
// the matches' own life (0x75dee0), the destruction of the pieces (0x74c770, 0x755b80) and the score (0x74a210, 0x73b6d0).
// port: the special gems (flame, star, hypercube, and the bits of the flags that make them), the level bar, the hint, the
// game's end, the sounds and the effects' pictures come with their steps; each place says what is left out and where.
#include "Board.h"

#include "Bej3App.h"
#include "PIEffect.h"
#include "Res.h"
#include "X86Math.h"

#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <map>
#include <set>

namespace Sexy {

namespace {

// The piece has no bits that make it special (0x1800 and 0x100 are the ones the swap tests; the plain gem has none)
const int FLAGS_SPECIAL_SWAP = 0x1800;
const int FLAGS_NO_SWAP = 0x100;

// What the exe's curves of the swaps are: the 0x85efb0 string for the swap and the 0x85fe00 one for the swap back
const char* const SWAP_CURVE = "b+-1,1,0.033333,1,~x:*    }Ppt+     $#LPR";
const char* const SWAP_BACK_CURVE = "b+-1,1,0.022222,1,#$)h B####    cObb4   z~LPQ";

// The exe's sets of pieces (local_27c, local_224, local_244, local_1fc, local_1dc of 0x75af60) are std::sets ordered by the
// pieces' addresses; here they are ordered by id, which every run of the game keeps the same
struct PieceIdLess {
	bool operator()(const Piece* theFirst, const Piece* theSecond) const { return theFirst->mId < theSecond->mId; }
};
typedef std::set<Piece*, PieceIdLess> PieceSet;

// One run that was kept (the exe's 0x24-byte entry in the vector at local_25c): its pieces and the tag they carry
struct MatchCluster {
	std::vector<Piece*> mPieces;   // +0xc
	int mTag;                      // +0x1c
};

// One entry of the map at local_198 (the key is the centre piece): the piece at the middle of a run of 4 or more, how long the
// run is, and the piece next to it that is made the special instead when the centre is already a star gem
struct SpecialCandidate {
	Piece* mCentre;
	int mCount;
	Piece* mNeighbour;
};

// What 0x75cd40 gives the other pieces of a cluster that merges into a special: the scale curve (0x85f138) and the slide curve (0x85f164)
const char* const MERGE_SCALE_CURVE = "b+0,1,0.033333,1,~zL>         %]Bt(|#:M@";
const char* const MERGE_SLIDE_CURVE = "b+0,1,0.033333,1,#.ht         ~~W[d";

// 0x75b6xx to 0x75b78a, the second test that holds a run back (the exe's local_3b3 + 0x6d): from any of its gems, in the direction of the run
// up to 7 cells of the gem's colour in a row hold a gem that could still settle there (a bVar5 of the exe: not at rest, but free: not
// held by a swap, able to fall, no busy count, no pulse and no slide running, no bit 0x2000), or, across the run, the same-colour cells on both
// sides of the gem number 2 or more and one of them could still settle
bool RunIsHeldBack(Board* theBoard, const std::vector<Piece*>& theRun, bool theAlongRow)
{
	static const int DIRECTIONS[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
	for (size_t k = 0; k < theRun.size(); k++) {
		Piece* aGem = theRun[k];
		bool aAlongCouldSettle = false;
		bool aAcrossCouldSettle = false;
		int aAcrossCount = 0;
		for (int aDirection = 0; aDirection < 4; aDirection++) {
			// directions 0 and 1 go down and up a column, 2 and 3 along a row; the run's own direction is the first pair
			// in the first pass (down a column) and the second pair in the second
			bool aInRunDirection = (aDirection / 2 == (theAlongRow ? 1 : 0));
			for (int aStep = 1; aStep < 8; aStep++) {
				int aRow = aGem->mRow + DIRECTIONS[aDirection][0] * aStep;
				int aCol = aGem->mCol + DIRECTIONS[aDirection][1] * aStep;
				if (aRow < 0 || aRow >= Board::ROWS || aCol < 0 || aCol >= Board::COLS)
					break;
				Piece* aNeighbour = theBoard->mGrid[aRow][aCol];
				if (aNeighbour == nullptr || aNeighbour->mColor != aGem->mColor)
					break;
				bool aCouldSettle = !Board::PieceIsSettled(aNeighbour) && aNeighbour->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA &&
					!theBoard->IsHeldBySwap(aNeighbour) && aNeighbour->mCanFall && aNeighbour->mBusyCount == 0 &&
					aNeighbour->mSlideCurve.GetOutVal() == 0.0 && (aNeighbour->mFlags & 0x2000) == 0;
				if (aInRunDirection)
					aAlongCouldSettle = aAlongCouldSettle || aCouldSettle;
				else {
					aAcrossCount++;
					aAcrossCouldSettle = aAcrossCouldSettle || aCouldSettle;
				}
			}
		}
		if (aAlongCouldSettle || (aAcrossCount >= 2 && aAcrossCouldSettle))
			return true;
	}
	return false;
}

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
bool Board::TrySwap(Piece* thePiece, int theRow, int theCol, bool theForced)
{
	if (!CanSwap(thePiece, (unsigned)theRow, (unsigned)theCol))
		return false;
	PendingSwap aPending;
	aPending.mUpdate = mUpdateCnt;
	aPending.mPieceId = thePiece->mId;
	aPending.mRow = theRow;
	aPending.mCol = theCol;
	aPending.mForced = theForced;
	mPendingSwaps.push_back(aPending);
	mHintTimer = 0;   // port: a move asked for starts the hint's wait again (0x75f852 puts +0x26a4 to 0 in a function that reads the piece's +0x182)
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
	for (int aKey = 0; aKey < COUNTER_COUNT; aKey++)
		aRecord.mCounters[aKey] = 0;
	mMoveRecords.push_back(aRecord);
	return &mMoveRecords.back();
}

// 0x759840 (bejeweled3), see the header
void Board::UpdateMoveRecords()
{
	for (size_t aIndex = 0; aIndex < mMoveRecords.size();) {
		int anId = mMoveRecords[aIndex].mId;
		if (!mMatchGroups.empty()) {
			aIndex++;
			continue;
		}
		bool aHeld = false;
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++) {
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == nullptr || aPiece->mColumnTag != anId)
					continue;
				if (!PieceIsSettled(aPiece)) {
					aHeld = true;
					continue;
				}
				aPiece->mColumnTag = -1;
				aPiece->mMovedUpdate = 0;
			}
		}
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mColumnTag[aCol] == anId)
				aHeld = true;
		}
		if (aHeld) {
			aIndex++;
			continue;
		}
		mMoveRecords.erase(mMoveRecords.begin() + aIndex);
	}
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
bool Board::StartSwap(Piece* thePiece, int theRow, int theCol, bool theForced)
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
	// 0x75f0xx: a hypercube swapped with a gem does not swap: it clears the colour of that gem (0x740f20); one next to a gem that cannot
	// be cleared stays; one with no gem beside it takes the swap below
	bool aFirstIsCube = (thePiece->mFlags & Piece::FLAG_HYPERCUBE) != 0;
	bool aSecondIsCube = aOther != nullptr && (aOther->mFlags & Piece::FLAG_HYPERCUBE) != 0;
	if (aFirstIsCube && aOther != nullptr) {
		if (!aOther->mClearable)
			return false;
		CountDownMoves();   // 0x75f85c
		StartHypercube(thePiece, aOther);
		return true;
	}
	if (aSecondIsCube) {
		if (!thePiece->mClearable)
			return false;
		CountDownMoves();   // 0x75f8be
		StartHypercube(aOther, thePiece);
		return true;
	}
	thePiece->mMovedUpdate = mUpdateCnt;   // 0x75f8fc
	if (aOther != nullptr)
		aOther->mMovedUpdate = mUpdateCnt - 1;
	SwapRecord aSwap;
	aSwap.mFirst = thePiece;
	aSwap.mSecond = aOther;
	aSwap.mDeltaCol = theCol - thePiece->mCol;
	aSwap.mDeltaRow = theRow - thePiece->mRow;
	aSwap.mCurve.SetCurve(SWAP_CURVE);
	aSwap.mIsSwapBack = false;
	aSwap.mDone = false;
	aSwap.mForced = theForced;
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
		if (aMatches != 0 || aSame.mForced) {
			// 0x75a49c: a forced swap (the exe's bVar2) is kept whether or not it made a match; 0x74c070, which starts with 0x73c7e0, is
			// run for matches of a swap that is not forced (+0xf9), then slot 191 sees the swap in either case
			SwapRecord aMatched = mSwaps[i];
			mSwaps.erase(mSwaps.begin() + i);
			if (aMatches != 0 && !aMatched.mForced)
				CountDownMoves();   // 0x75a61e
			OnSwapMatched(aMatched);
			continue;
		}
		// nothing: back they go (the first one's place is now the second's, so its move is the other way)
		if (!aSame.mForced)
			OnSwapFailed(aSame);   // slot 192 (0x300)
		aSame.mDone = false;
		aSame.mIsSwapBack = true;
		aSame.mDeltaCol = -aSame.mDeltaCol;
		aSame.mDeltaRow = -aSame.mDeltaRow;
		aSame.mCurve.SetCurve(SWAP_BACK_CURVE);
		i++;
	}
}

// 0x75af60 (bejeweled3), slot 201, called with three zeros (0x767e20 at 0x768143; 0x759ba0 for a swap that has come to its end).
// The board is read twice: first down each column and then along each row (the exe's outer loop runs over the first index of its grid
// expression, which is the column in the first pass). A cell goes on with the run of the cell before it when its piece is free (its
// scale curve (+0x20) does not run a pulse (type 6), no swap holds it, it can fall, has no busy count, no slide (+0x198) and no
// bit 0x2000) or was already met in this call, and its colour is the run's colour: the piece's colour (+0x220), or the remembered
// one (+0x21c) when it was matched in this update. A cell whose piece is not at rest (0x755ad0) and was not met in this call, or
// sits in a held run, holds the run back (the board is searched again in the next update). A run of 3 or more takes the next id
// of the board's counter (+0xe1c) before it is judged, whether it is held or not. A held run's gems take the run's tag, which is
// the highest tag (+0x188) among them, and go into the set of held runs, so a run that crosses it is held too. A run is also held
// back when RunIsHeldBack says so.
// A run that is kept takes the run's id (+0x190) and tag for its gems and makes a cluster. In it:
//  - a gem with bit 1 is listed to go off (the list of 0x110, below);
//  - a gem with bit 4 that was not matched in this update gets a match group (0x789a90) if it has none, and goes no further;
//  - a gem matched in this update, with a colour, and with no bits or bit 1 (the second visit: it is in two runs, a crossing)
//    gets the scale curve 1 back, goes into the set of targets and the set of gems met twice;
//  - any other gem is met for the first time: its scale curve starts a pulse (0x85f07c when a gem of the run was swapped in this
//    update, else 0x85f0a4), as quick as gravity (slot 0x1dc), it is matched in this update (+0x210), remembers its colour (+0x21c)
//    and goes into the set of gems met; one with no bits is also listed by its last move (+0x218) and by its place.
// The gem at the middle of the run (the one moved last, the middle one of those when more than one was; or, of the plain gems, the
// last with a gem of its colour on both sides of it across the run) and a plain gem next to it (up to two cells from it towards the
// longer side of the run) go into the map of special candidates with the run's length when it is 4 or more (the exe asks for 6 or
// more, 5 or more with a centre that is not a star gem, and 4 or more: the same record each time; the help boxes (0x745bf0) of each
// kind are left out, see below), and both take the run's tag.
// When both passes are over: the listed gems with bit 1 have the flags that clear them noted, busy count 1 + their place in the
// list, themselves as the destroyer and a scale curve 1; each gem met twice with bit 1 goes off at once (busy count 1, flags 0) and
// then, as a crossing, becomes a star gem (slot 0x2a0: the exe's slot 0x174 is true in Classic, where slot 0x298 would do the
// same for a board where it is not); each candidate makes its special on the gem it is for (the centre, or its neighbour when the
// centre is a star gem), if that has no bits: a run of 4 a flame gem (slot 0x298), a run of 5 a hypercube (slot 0x29c), a run of
// 6 or more a star gem and a flame gem; the gem is then matched in this update, remembers its colour (if it has one) and has the
// scale curve 1. Each cluster then
//  - freezes (scale 1, cannot fall) when a gem of it has bit 4 and was not matched in this update;
//  - else, when a gem of it is a target, sends the others to that one (the plain ones and the ones with bit 0x80: a scale curve
//    0x85f138 and a slide curve 0x85f164, each as quick as gravity, and the cell to go to);
//  - scores: (X - 3 + n) * 50 and (n - 4) * 350 more from 5 gems, through AddPoints (X: how many matches the move and its cascades
//    have made, this one counted, at least 1; n: the gems of the cluster).
// The exe returns whether a match was made; here it is how many.
// port: the swap-list flag (+0xf9) of 0x75b02b to 0x75b08b (it holds every run while a swap of that kind is going; where the exe
// sets it was not found), the colour test of slot 0x24c and the second argument (which 0x759ba0 and 0x767e20 never set), the help
// boxes of slot 0x20c (0x73c4c0: true while the player has not seen the help of that special; here every help is seen, see Oddities
// in docs/NOTES.md), the sets that only those boxes and the other modes fill (local_244 and local_13c, the bits 0x10, 0x80, 0x400,
// 0x800, 0x1000, 0x2000, 0x8000, 0x80000 of the flags), the speed bonus (+0x1371), the stats of 0x73d170 (only the count of matches,
// stat 0x1a, is kept), the sounds, the effects and the pulse's speed (+0x170, which the exe sets to 0 for a piece that is at rest
// anyway). The map of candidates is ordered by the pieces' addresses in the exe, here by id
int Board::FindMatches()
{
	PieceSet aMatchedHere;      // local_27c: the gems met for the first time in this call
	PieceSet aHeldRunGems;      // local_224: the gems of the runs that were held
	PieceSet aTargets;          // local_1fc: where the specials are made, and the gems met twice
	PieceSet aMetTwice;         // local_1dc: the gems that two runs share
	std::map<int, SpecialCandidate> aCandidates;   // local_198
	std::vector<MatchCluster> aClusters;           // local_25c
	std::vector<Piece*> aGoingOff;                 // the list at 0x110
	int aMatchCount = 0;
	const int aUpdate = mUpdateCnt;

	// one run of 3 or more is over: the cells it covers are (mFirstCol, mFirstRow) to (mLastCol, mLastRow)
	struct RunEnd {
		int mPass;
		int mLastCol;
		int mLastRow;
		int mFirstCol;
		int mFirstRow;
		bool mHeld;
	};
	auto aEndRun = [&](const RunEnd& theRun, const std::vector<Piece*>& thePieces) {
		int aRunId = mMatchCounter++;
		int aRunTag = -1;
		for (size_t k = 0; k < thePieces.size(); k++)
			aRunTag = std::max(aRunTag, thePieces[k]->mColumnTag);
		if (theRun.mHeld || RunIsHeldBack(this, thePieces, theRun.mPass == 1)) {
			for (size_t k = 0; k < thePieces.size(); k++) {
				thePieces[k]->mColumnTag = aRunTag;
				aHeldRunGems.insert(thePieces[k]);
			}
			return;
		}
		aMatchCount++;
		// 0x6f of the exe: some piece of the run was swapped in this update (a pulse of 3 updates, not 30)
		bool aSwapMade = false;
		for (size_t k = 0; k < thePieces.size(); k++)
			aSwapMade = aSwapMade || thePieces[k]->mSwapUpdate == aUpdate;
		MatchCluster aCluster;
		aCluster.mPieces = thePieces;
		aCluster.mTag = aRunTag;
		std::vector<Piece*> aRecent;   // the plain gems met for the first time that moved last
		std::vector<Piece*> aPlain;    // the plain gems met for the first time
		int aRecentUpdate = INT_MIN;
		for (size_t k = 0; k < thePieces.size(); k++) {
			Piece* aPiece = thePieces[k];
			aPiece->mGroupId = aRunId;
			aPiece->mColumnTag = aRunTag;
			if ((aPiece->mFlags & Piece::FLAG_FLAME) != 0)
				aGoingOff.push_back(aPiece);
			if ((aPiece->mFlags & Piece::FLAG_STAR) != 0 && aPiece->mMatchedUpdate != aUpdate) {
				// 0x75bac5 to 0x75bb0f: a star gem that was not matched in this update gets a group of its own, unless it has
				// one; it takes no part in the plain gems' clear. The group is type 3, or 2 when the board's +0xe8 is set; a piece
				// with bit 1 makes it type 6 (0x789a90). port: the counter of 0x73d170 (stat 1, the move's tag) and the box that
				// 0x742d00 shows for a type 2 or 7 group are left out
				if (FindMatchGroup(aPiece) == -1)
					MakeMatchGroup(aPiece, 3 - (mGroupTypeSwitch != 0 ? 1 : 0));
				continue;
			}
			if (aPiece->mMatchedUpdate == aUpdate) {
				// 0x75bbdb to 0x75bcc3, the second visit: the gem is in two runs
				if (aPiece->mColor > -1 && aPiece->mColor < Piece::COLOR_COUNT &&
					(aPiece->mFlags == 0 || (aPiece->mFlags & Piece::FLAG_FLAME) != 0)) {
					aPiece->mScaleCurve.SetConstant(1.0);   // 0x437760 with 0x7cd048
					aPiece->mMatchedUpdate = aUpdate;
					aTargets.insert(aPiece);
					aPiece->mColumnTag = aRunTag;
					aMetTwice.insert(aPiece);
				}
				continue;
			}
			// 0x75bcd0 to 0x75bf8b, a gem matched for the first time in this update: the pulse starts on its scale curve (+0x20) and
			// its step is scaled by the fall factor (slot 0x1dc)
			aPiece->mScaleCurve.SetCurve(aSwapMade ? "b+1,1.2,0.333333,1,#+Ky         ~~###" : "b+1,1.22,0.033333,1,#+Kx      uw*7u   ,~###");
			aPiece->mScaleCurve.mIncRate = (double)GetFallFactor() * aPiece->mScaleCurve.mIncRate;
			aPiece->mMatchedUpdate = aUpdate;
			aPiece->mRememberedColor = aPiece->mColor;
			aMatchedHere.insert(aPiece);
			if (aPiece->mFlags == 0) {
				if (aPiece->mMovedUpdate > aRecentUpdate) {
					aRecentUpdate = aPiece->mMovedUpdate;
					aRecent.clear();
				}
				if (aPiece->mMovedUpdate == aRecentUpdate)
					aRecent.push_back(aPiece);
				aPlain.push_back(aPiece);
			}
		}
		if (!aPlain.empty()) {
			// 0x75bffd to 0x75c273: the centre
			Piece* aCentre = aRecent[aRecent.size() / 2];
			for (size_t k = 0; k < aPlain.size(); k++) {
				Piece* aGem = aPlain[k];
				Piece* aBefore = nullptr;
				Piece* aAfter = nullptr;
				if (theRun.mPass == 0) {
					aBefore = aGem->mCol - 1 >= 0 ? mGrid[aGem->mRow][aGem->mCol - 1] : nullptr;
					aAfter = aGem->mCol + 1 < COLS ? mGrid[aGem->mRow][aGem->mCol + 1] : nullptr;
				}
				else {
					aBefore = aGem->mRow - 1 >= 0 ? mGrid[aGem->mRow - 1][aGem->mCol] : nullptr;
					aAfter = aGem->mRow + 1 < ROWS ? mGrid[aGem->mRow + 1][aGem->mCol] : nullptr;
				}
				if (aBefore != nullptr && aBefore->mColor == aGem->mColor && aAfter != nullptr && aAfter->mColor == aGem->mColor)
					aCentre = aGem;
			}
			// the plain gem next to it: up to two cells from it, towards the longer side of the run
			int aToFirst = std::max(std::abs(theRun.mFirstCol - aCentre->mCol), std::abs(theRun.mFirstRow - aCentre->mRow));
			int aToLast = std::max(std::abs(theRun.mLastCol - aCentre->mCol), std::abs(theRun.mLastRow - aCentre->mRow));
			Piece* aNeighbour = nullptr;
			for (int aStep = 1; aStep <= 2; aStep++) {
				int aCol = aCentre->mCol;
				int aRow = aCentre->mRow;
				if (aToFirst > aToLast) {
					if (theRun.mPass == 0)
						aRow = std::max(theRun.mFirstRow, aCentre->mRow - aStep);
					else
						aCol = std::max(theRun.mFirstCol, aCentre->mCol - aStep);
				}
				else {
					if (theRun.mPass == 0)
						aRow = std::min(theRun.mLastRow, aCentre->mRow + aStep);
					else
						aCol = std::min(theRun.mLastCol, aCentre->mCol + aStep);
				}
				aNeighbour = (aCol >= 0 && aCol < COLS && aRow >= 0 && aRow < ROWS) ? mGrid[aRow][aCol] : nullptr;
				if (aNeighbour != nullptr && aNeighbour->mFlags == 0)
					break;
			}
			// slot 0x170 is true in Classic; 0x75c27f to 0x75c4d7
			int aLength = (int)thePieces.size();
			if (aLength >= 4) {
				aCentre->mColumnTag = aRunTag;
				if (aNeighbour != nullptr)
					aNeighbour->mColumnTag = aRunTag;
				SpecialCandidate aCandidate;
				aCandidate.mCentre = aCentre;
				aCandidate.mCount = aLength;
				aCandidate.mNeighbour = aNeighbour;
				aCandidates.insert(std::make_pair(aCentre->mId, aCandidate));
			}
			// slot 0x334 (0x73ba20) is false in Classic: no scale curve 1 for the gem here
		}
		aClusters.push_back(aCluster);
	};

	for (int aPass = 0; aPass < 2; aPass++) {
		for (int aOuter = 0; aOuter < COLS; aOuter++) {
			int aRunColor = -1;
			int aRunCount = 0;
			RunEnd aRun;
			aRun.mPass = aPass;
			aRun.mLastCol = aRun.mLastRow = aRun.mFirstCol = aRun.mFirstRow = 0;
			aRun.mHeld = false;
			std::vector<Piece*> aRunPieces;
			for (int aInner = 0; aInner < ROWS; aInner++) {
				int aCol = aPass == 0 ? aOuter : aInner;
				int aRow = aPass == 0 ? aInner : aOuter;
				Piece* aPiece = mGrid[aRow][aCol];
				bool aMet = aPiece != nullptr && aMatchedHere.count(aPiece) != 0;
				bool aEligible = aPiece != nullptr && (PieceIsSettled(aPiece) || aMet) && aHeldRunGems.count(aPiece) == 0;
				int aColor = -1;
				if (aPiece != nullptr) {
					bool aFree = aPiece->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA && !IsHeldBySwap(aPiece) && aPiece->mCanFall &&
						aPiece->mBusyCount == 0 && aPiece->mSlideCurve.GetOutVal() == 0.0 && (aPiece->mFlags & 0x2000) == 0;
					if (aFree || aMet)
						aColor = aPiece->mMatchedUpdate == aUpdate ? aPiece->mRememberedColor : aPiece->mColor;
				}
				if (aColor != -1 && aColor == aRunColor) {
					aRunCount++;
					aRun.mHeld = aRun.mHeld || !aEligible;
					aRun.mLastCol = aCol;
					aRun.mLastRow = aRow;
					aRunPieces.push_back(aPiece);
					continue;
				}
				if (aRunCount >= 3)
					aEndRun(aRun, aRunPieces);
				// 0x75c794: a run starts at this cell
				aRunColor = aColor;
				aRunCount = 1;
				aRun.mHeld = !aEligible;
				aRun.mFirstCol = aRun.mLastCol = aCol;
				aRun.mFirstRow = aRun.mLastRow = aRow;
				aRunPieces.clear();
				if (aColor != -1)
					aRunPieces.push_back(aPiece);
			}
			if (aRunCount >= 3)
				aEndRun(aRun, aRunPieces);
		}
	}

	// 0x75c831 to 0x75c88d: the gems with bit 1 that were in a run go off one after the other
	for (size_t k = 0; k < aGoingOff.size(); k++) {
		Piece* aPiece = aGoingOff[k];
		aPiece->mDestroyerFlags |= aPiece->mFlags;
		aPiece->mBusyCount = (int)k + 1;
		aPiece->mDestroyerId = aPiece->mId;
		aPiece->mScaleCurve.SetConstant(1.0);
	}
	// 0x75c8b0 to 0x75c9d9: the gems that two runs share
	for (PieceSet::iterator aIt = aMetTwice.begin(); aIt != aMetTwice.end(); ++aIt) {
		Piece* aPiece = *aIt;
		if ((aPiece->mFlags & Piece::FLAG_FLAME) != 0) {
			aPiece->mBusyCount = 1;
			aPiece->mDestroyerId = aPiece->mId;
			aPiece->mDestroyerFlags = aPiece->mFlags;
			aPiece->mFlags = 0;
		}
		MakeStarGem(aPiece);   // slot 0x174 is true in Classic (slot 0x298, MakeFlameGem, where it is not)
	}
	// 0x75ca00 to 0x75cc9c: the specials are made
	for (std::map<int, SpecialCandidate>::iterator aIt = aCandidates.begin(); aIt != aCandidates.end(); ++aIt) {
		const SpecialCandidate& aCandidate = aIt->second;
		Piece* aTarget = (aCandidate.mCentre->mFlags & Piece::FLAG_STAR) != 0 ? aCandidate.mNeighbour : aCandidate.mCentre;
		if (aTarget == nullptr || aTarget->mFlags != 0)
			continue;   // the exe reads the flags of a null neighbour here
		// the set of blocked gems (local_244) is empty in Classic
		aTargets.insert(aTarget);
		if (aCandidate.mCount > 5) {
			MakeStarGem(aTarget);
			MakeFlameGem(aTarget);
			aTarget->mScaleCurve.SetConstant(1.0);
		}
		else if (aCandidate.mCount > 4) {
			MakeHypercube(aTarget, true);
			aTarget->mScaleCurve.SetConstant(1.0);
		}
		else
			MakeFlameGem(aTarget);
		aTarget->mMatchedUpdate = aUpdate;
		if (aTarget->mColor != Piece::NO_COLOR)
			aTarget->mRememberedColor = aTarget->mColor;
		aTarget->mScaleCurve.SetConstant(1.0);
	}
	for (size_t aClusterIndex = 0; aClusterIndex < aClusters.size(); aClusterIndex++) {
		MatchCluster& aCluster = aClusters[aClusterIndex];
		OnRunFound(aCluster.mPieces);   // slot 211 (0x34c)
		Piece* aTarget = nullptr;
		bool aFrozen = false;
		for (size_t k = 0; k < aCluster.mPieces.size(); k++) {
			Piece* aPiece = aCluster.mPieces[k];
			if (aTargets.count(aPiece) != 0)
				aTarget = aPiece;
			if ((aPiece->mFlags & Piece::FLAG_STAR) != 0 && aPiece->mMatchedUpdate != aUpdate)
				aFrozen = true;
		}
		// slot 0x148 (recording) is false and the cluster holds no blocked gem in Classic
		if (aFrozen) {
			for (size_t k = 0; k < aCluster.mPieces.size(); k++) {
				aCluster.mPieces[k]->mScaleCurve.SetConstant(1.0);
				aCluster.mPieces[k]->mCanFall = false;
			}
		}
		else if (aTarget != nullptr) {
			// 0x75d4f8 to 0x75d837. port: the curve of +0x90 and the effect for a flame gem are drawn only
			for (size_t k = 0; k < aCluster.mPieces.size(); k++) {
				Piece* aPiece = aCluster.mPieces[k];
				if (aPiece == aTarget || (aPiece->mFlags != 0 && (aPiece->mFlags & 0x80) == 0))
					continue;
				aPiece->mScaleCurve.SetCurve(MERGE_SCALE_CURVE);
				aPiece->mScaleCurve.mIncRate = (double)GetFallFactor() * aPiece->mScaleCurve.mIncRate;
				aPiece->mSlideCurve.SetCurve(MERGE_SLIDE_CURVE);
				aPiece->mSlideCurve.mIncRate = (double)GetFallFactor() * aPiece->mSlideCurve.mIncRate;
				aPiece->mSlideCol = aTarget->mCol;
				aPiece->mSpecialState = aTarget->mRow;
			}
		}
		int aPieceCount = (int)aCluster.mPieces.size();
		MoveRecord* aRecord = FindMoveRecord(aCluster.mTag);
		if (aRecord != nullptr)
			aRecord->mCounters[COUNTER_MATCHES]++;
		int aMatchesInMove = aRecord != nullptr ? aRecord->mCounters[COUNTER_MATCHES] : 0;
		if (aMatchesInMove < 1)
			aMatchesInMove = 1;
		int aPoints = (aMatchesInMove - 3 + aPieceCount) * 0x32;
		if (aPieceCount >= 5)
			aPoints += (aPieceCount - 4) * 0x15e;
		AddPoints(aPoints, aCluster.mTag);
	}
	// 0x75cd40 to 0x75ce51: slot 0x238 (0x73d2b0) for each gem met, which only counts
	return aMatchCount;
}

// 0x789a90 (bejeweled3): a group for a piece, made
// by 0x75af60 (a flame gem, bit 4, in a match) and by 0x74c570, and pushed at the end of the list at +0x125c. It takes the piece's
// place at the middle of the cell, its flags, tag (+0x188) and run id (+0x190), puts the piece's id first in its list, sets the
// piece's +0x174 and puts its timer (+0x178) at 0.9 (not for type 7). A piece with bit 1 makes it type 6. A group of type 7 (the
// hypercube's, 0x7870f0) starts with the colour -1, and one for a piece with no colour holds that piece (+0x17c). No Rand() is drawn here (the two
// streaks it makes, 0x789860, take their places from the cell and draw only when they run). port: the streaks (their own
// update, 0x786dd0, draws the app's Rand() every fifth update, 0x78a390 runs it for each), the curves that the group sets
// ("b+0,1.5,..." for a type 6 group) and the fields that only they read (+0x10, +0x18, +0x20, +0x24, +0x40, +0x44, +0x50 to
// +0x270) are not translated
MatchGroup* Board::MakeMatchGroup(Piece* thePiece, int theType)
{
	std::unique_ptr<MatchGroup> aGroup(new MatchGroup());
	aGroup->mType = (thePiece->mFlags & 1) == 0 ? theType : 6;
	aGroup->mAge = 0;
	aGroup->mFade = 1.0f;
	aGroup->mLifeCountdown = 0;
	aGroup->mColor = -1;
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
	if (aGroup->mType != 7)
		thePiece->mMatchTimer = 0.89999998f;
	if (thePiece->mColor == Piece::NO_COLOR)
		thePiece->mClearHeld = true;
	aGroup->mRingTimer = 0.0f;
	aGroup->mLife = 1.0f;
	aGroup->mRing = 0;
	aGroup->mMaxDistance = (aGroup->mType != 3 ? 4 : 0) + 3;
	MatchGroup* aMade = aGroup.get();
	mMatchGroups.push_back(std::move(aGroup));
	return aMade;
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
// notes what cleared it. port: the counter of 0x73d170 (stat 1) that a flame gem's group adds is left out: nothing translated gives a piece
// bit 1, 2 or 0x10
bool Board::ClearPiece(Piece* thePiece, Piece* theBy)
{
	if (thePiece->mClearHeld)
		return false;
	int aFlags = thePiece->mFlags;
	if ((aFlags & 1) == 0 || (aFlags & 4) != 0) {
		if ((aFlags & 0x80000) == 0) {
			if ((thePiece->mFlags & 2) != 0 && FindMatchGroup(thePiece) == -1) {
				// a hypercube that goes off: the colour it clears is the clearer's (its remembered one when it has none), or
				// its own remembered colour with no clearer (a random one, from the board's generator, when it has none)
				int aColor;
				if (theBy == nullptr) {
					aColor = thePiece->mRememberedColor;
					if (aColor == -1)
						aColor = (int)((unsigned)mRand.Next() % 7);
				}
				else {
					aColor = theBy->mColor;
					if (aColor == -1)
						aColor = theBy->mRememberedColor;
				}
				MakeHypercubeGroup(thePiece, aColor);
				return true;
			}
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
	const bool aSlowMode = IsSlowMode();   // slot 0x1c4 (0x73c1f0): 1 on the Zen board (0x70e3a0)
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
					OnCellHit(aCell.mCol, aCell.mRow);   // slot 0x314 with the cell (Board's 0x73bb40 returns at once; Wall Blast's goal takes it)
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
							OnCellHit(aFirst->mCol, aFirst->mRow);   // slot 0x314 with the piece's cell (0x75ec41)
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
		else if (aGroup->mType == 7)
			UpdateHypercubeGroup(aGroup);
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

// 0x767e20 (bejeweled3), 0x768f7b to 0x7690b8, run just before 0x75dee0: every piece's scale curve is stepped. While it runs
// nothing else is done. When it has ended: above 1 (a pulse that has run out) a shrink from 1.2 to 0 in 20 updates starts;
// at 1 the curve is put back to the constant 1; at 0 (the shrink has run out) and with the piece's own swap curve at rest the
// piece is taken from the board (slot 0x2c0, RemovePiece) with no busy count, so neither 0x74c770's 50 points and sparks nor
// its draws of Rand() happen: a plain match is scored once, by 0x75af60
// 0x7690d0 to 0x769149 (and the same loop at 0x767bbf in UpdateTick, whose condition 0x75a6e0 is not translated): a gem that is picked up
// or already turning turns 0.02 of its way further; at a whole turn it is at rest again
void Board::UpdatePieceSpins()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr)
				continue;
			if (aPiece->mSpinPhase != 0.0f || aPiece->mSelected) {
				aPiece->mSpinPhase = (float)((double)aPiece->mSpinPhase + 0.019999999552965164);
				if (aPiece->mSpinPhase >= 1.0f)
					aPiece->mSpinPhase = 0.0f;
			}
		}
	}
}

void Board::UpdatePiecePulses()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || aPiece->mScaleCurve.IncInVal())
				continue;
			double aScale = aPiece->mScaleCurve.GetOutVal();
			if (aScale > 1.0)
				aPiece->mScaleCurve.SetCurve("b+0,1.2,0.05,1,~###         ~#Blc");
			else if (aScale == 1.0)
				aPiece->mScaleCurve.SetConstant(1.0);
			else if (aScale == 0.0 && !IsHeldBySwap(aPiece) && !aPiece->mSlideCurve.IsDoingCurve())
				RemovePiece(aPiece);
		}
	}
}

// 0x745ca0 (bejeweled3), slot 0x298: a run of 4 (or the second part of a run of 6 or more) makes the gem a flame gem. It needs a colour; it sets
// bit 1, clears the bits that 0x2e0 names and starts the countdown (+0x184) at 25, during which a clear leaves it standing. It makes the
// creation effects (two PopAnimEffects, type 0x17, which draw no Rand(): MakeFlameCreationEffects). port: the sound (0x76aaf0) is left out
void Board::MakeFlameGem(Piece* thePiece)
{
	if (thePiece->mColor == Piece::NO_COLOR)
		return;
	thePiece->mFlags |= Piece::FLAG_FLAME;
	thePiece->mFlags &= ~Piece::FLAGS_SPECIAL_MADE;
	thePiece->mSpecialCountdown = 25;
	MakeFlameCreationEffects(thePiece);
}

// 0x7494d0 (bejeweled3), slot 0x2a0: a crossing (or a run of 6 or more) makes the gem a star gem; the same as 0x745ca0 with bit 4 and, at the end, 0x749050: its two effects
// (MakeStarEffects), whose Rand() draws come at their second updates. port: the sound (0x76aaf0) is left out
void Board::MakeStarGem(Piece* thePiece)
{
	if (thePiece->mColor == Piece::NO_COLOR)
		return;
	thePiece->mFlags |= Piece::FLAG_STAR;
	thePiece->mFlags &= ~Piece::FLAGS_SPECIAL_MADE;
	thePiece->mSpecialCountdown = 25;
	MakeStarEffects(thePiece);
}

// 0x749570 (bejeweled3), slot 0x29c: a run of 5 makes the gem a hypercube: bit 2 and nothing else, no colour (it remembers the old one,
// +0x21c) and the countdown at 25. theWithEffect (1 from 0x75af60) starts its effect (MakeHypercubeEffect: it draws its Rand() at its
// second update). port: the sound (0x76aaf0) is left out
void Board::MakeHypercube(Piece* thePiece, bool theWithEffect)
{
	thePiece->mFlags = Piece::FLAG_HYPERCUBE;
	thePiece->mMatchedUpdate = mUpdateCnt;
	thePiece->mRememberedColor = thePiece->mColor;
	thePiece->mColor = Piece::NO_COLOR;
	thePiece->mSpecialCountdown = 25;
	if (theWithEffect)
		MakeHypercubeEffect(thePiece);
}

// 0x73f8e0 (bejeweled3), with its wrapper 0x74d940: a flame gem at (theX, theY), in 1200ths from the window's corner, goes off. The nine cells
// around that point (the centre last, which is slot 8) are looked at in this order. A cell with no piece, or a piece that is busy (not
// the centre), is left alone. For a neighbour the 474 Rand() draws of its sparks are made (DrawExplosionRandoms). A piece whose countdown
// (+0x184) has run out then takes the centre's tag and run id (the centre takes the board's next id when it has none), and: a
// neighbour with bit 1 gets busy count 15 (it goes off in a chain); any other piece without bit 0x10 is cleared by 0x74c570 or, when that
// does not take it (or the piece is a flame gem only), removed at once for 20 points. port: the types 0 and 1 of the exe's group removal
// (a different mode), the sounds and the effects
void Board::Explode(int theX, int theY)
{
	static const int OFFSETS[9][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }, { 0, 0 } };
	Piece* aCentre = GetPieceAt((float)(theX - GetGridX()), (float)(theY - GetGridY()));
	for (int aIndex = 0; aIndex < 9; aIndex++) {
		int aDeltaX = OFFSETS[aIndex][0];
		int aDeltaY = OFFSETS[aIndex][1];
		// slot 197 (0x314) for the cell, before the piece is looked at: with the middle's piece it is that piece's cell moved by the offsets, without one the cell that
		// holds the point (−1 where it is off the grid) for all nine
		unsigned aHitCol;
		unsigned aHitRow;
		if (aCentre != nullptr) {
			aHitCol = (unsigned)(aCentre->mCol + aDeltaX);
			aHitRow = (unsigned)(aCentre->mRow + aDeltaY);
		}
		else {
			const int aCentreX = theX - GetGridX();
			const int aCentreY = theY - GetGridY();
			aHitCol = aCentreX >= 0 && aCentreX < COLS * CELL ? (unsigned)(aCentreX / CELL) : 0xffffffffu;
			aHitRow = aCentreY >= 0 && aCentreY < ROWS * CELL ? (unsigned)(aCentreY / CELL) : 0xffffffffu;
		}
		if (aHitCol < (unsigned)COLS && aHitRow < (unsigned)ROWS)
			OnCellHit((int)aHitCol, (int)aHitRow);
		Piece* aPiece = GetPieceAt((float)(aDeltaX * 128 + theX - GetGridX()), (float)(aDeltaY * 128 + theY - GetGridY()));
		if (aPiece == nullptr || (aPiece->mBusyCount != 0 && aIndex != 8))
			continue;
		bool aIsFlame = false;
		if (aDeltaX != 0 || aDeltaY != 0) {
			DrawExplosionRandoms();
			aIsFlame = (aPiece->mFlags & Piece::FLAG_FLAME) != 0;
		}
		if (aPiece->mSpecialCountdown != 0)
			continue;
		if (aCentre != nullptr) {
			aPiece->mColumnTag = std::max(aCentre->mColumnTag, aPiece->mColumnTag);
			if (aCentre->mGroupId == -1)
				aCentre->mGroupId = mMatchCounter++;
			aPiece->mGroupId = aCentre->mGroupId;
		}
		int aCentreId = aCentre != nullptr ? aCentre->mId : -1;
		int aCentreFlags = aCentre != nullptr ? aCentre->mFlags : 0;
		if (aIsFlame) {
			aPiece->mBusyCount = 15;
			aPiece->mDestroyerId = aCentreId;
			aPiece->mDestroyerFlags |= aCentreFlags;
		}
		else if ((aPiece->mFlags & 0x10) == 0) {
			bool aFlameOnly = (aPiece->mFlags & Piece::FLAG_FLAME) != 0 && (aPiece->mFlags & Piece::FLAG_STAR) == 0;
			if ((aFlameOnly || !ClearPiece(aPiece, aCentre)) && aPiece->mClearable) {
				aPiece->mDestroyed = true;   // 0x73f8e0 sets +0x17d here
				aPiece->mDestroyerId = aCentreId;
				aPiece->mDestroyerFlags |= aCentreFlags;
				AddPoints(20, aPiece->mColumnTag);
				RemovePiece(aPiece);
			}
		}
	}
}

// 0x73f8e0 (bejeweled3), for each neighbour of the explosion: the draws of the app's Rand() of its sparks. 0x729c20 makes particles of
// two kinds in rings and their constructors draw: 14 particles of 10 draws, 25 of 10 and 14 of 6, all with the speed 0 (so the
// branch that moves them draws nothing more). The values are dropped
void Board::DrawExplosionRandoms()
{
	const int aDraws = 14 * 10 + 25 * 10 + 14 * 6;
	for (int aDraw = 0; aDraw < aDraws; aDraw++)
		Rand();
}

// 0x7492c0 (bejeweled3): a piece that is not a butterfly yet loses its flags and the effects that hold it, takes the bit 0x80, gets the effects of
// its flags (0x749220: for the bit 0x80 the butterfly's own, 0x748ad0) and the creation burst (a copy of PIEFFECT_BUTTERFLY_CREATE that follows it
// and updates twice in each update). port: the three tints of the colour's layers and the sound at +0x5d8 of the app are left out
void Board::MakeButterflyGem(Piece* thePiece)
{
	if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
		return;
	thePiece->mFlags = 0;
	for (Effect* anEffect : thePiece->mEffects) {
		anEffect->mPieceId = 0;   // 0x71ffd0: the piece lets go of the effects that held it
		anEffect->mUsers--;
	}
	thePiece->mEffects.clear();
	thePiece->mFlags |= Piece::FLAG_BUTTERFLY;
	MakeButterflyEffect(thePiece);
	if (PIEFFECT_BUTTERFLY_CREATE != nullptr) {
		ParticleEffect* aCreate = new ParticleEffect(PIEFFECT_BUTTERFLY_CREATE);
		aCreate->mPieceId = thePiece->mId;
		aCreate->mUpdateTwice = true;
		mEffectsOver->Add(aCreate);
	}
}

// 0x73c7e0 (bejeweled3), see the header
void Board::CountDownMoves()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & 0x140) != 0)
				continue;
			CountDownPiece(aPiece, false);   // slot 212
		}
	}
}

// slot 212 (0x350), 0x73c710 (bejeweled3), the base: a butterfly gem (not a bomb, bit 0x200) with moves left counts one down. The base's other branches are the bombs'
// (TimeBombBoard::CountDownPiece)
void Board::CountDownPiece(Piece* thePiece, bool theTick)
{
	if (theTick || thePiece->mTimeGemSeconds <= 0 || (thePiece->mFlags & 0x200) != 0)
		return;
	if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
		thePiece->mTimeGemSeconds--;
}

// 0x740f20 (bejeweled3), for the player's swap of a hypercube and a gem: the cube's group takes the colour of the gem. port: the sound,
// the lightning bolt and the effect 0x14 draw no Rand() and are left out
void Board::StartHypercube(Piece* theCube, Piece* thePartner)
{
	MakeHypercubeGroup(theCube, thePartner->mColor);
}

// 0x74c570 and 0x740f20 (bejeweled3): a group of type 7 for the cube, which holds the cube (+0x17c) and clears theColor
void Board::MakeHypercubeGroup(Piece* theCube, int theColor)
{
	theCube->mClearHeld = true;
	MatchGroup* aGroup = MakeMatchGroup(theCube, 7);
	aGroup->mColor = theColor;
}

// 0x7870f0 (bejeweled3), run by 0x75dee0 for a group of type 7 after its age and fade: the hypercube picks the gems of its colour one by
// one. While its life countdown (+0x2e4) runs it does nothing. Every piece is looked at in rows: a busy one is noted; a piece that no
// pick has marked (+0x174) is a candidate when it has the group's colour (or the group has none), is not above the board and has
// none of the bits 0x1800; a piece that was picked has its timer (+0x178) raised by 0.01 (the cube) or 0.015, and above 1 it is cleared by
// 0x74c570 (busy count 1 when that does not take it), else it is listed, and listed as new when under 0.04. A pick is made when there is a
// candidate and either no bolt is on or the board's generator (the +0x258 one, not the app's Rand()) gives 0 modulo
// 0x14 / (listed + 1) + 5 (half of it for a group without a colour). It takes a new gem (or else a listed one) at random as the
// source of a bolt: the target is then the last candidate and a bolt of life 0 begins; with no source the target is a random candidate.
// Each bolt's life goes up by 0.012; up to 1 it draws 40 Rand() of the app every fourth update of the group (its points), above 1 it is gone. When nothing is busy,
// listed or left to pick and no bolt was on, every piece's speed is put to 0 and the group's countdown to 0x32. port: the bolt's drawing
void Board::UpdateHypercubeGroup(MatchGroup* theGroup)
{
	if (theGroup->mLifeCountdown > 0)
		return;
	std::vector<Piece*> aCandidates;
	std::vector<Piece*> aListed;
	std::vector<Piece*> aNew;
	bool aAnyBusy = false;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr)
				continue;
			if (aPiece->mBusyCount > 0)
				aAnyBusy = true;
			if (!aPiece->mIsGroupOrigin) {
				if ((aPiece->mColor == theGroup->mColor || theGroup->mColor == -1) && (float)GetGridY() + aPiece->mY > -128.0f &&
					(aPiece->mFlags & 0x1800) == 0)
					aCandidates.push_back(aPiece);
			}
			else {
				aPiece->mMatchTimer = (float)((double)aPiece->mMatchTimer +
					((aPiece->mFlags & Piece::FLAG_HYPERCUBE) != 0 ? 0.009999999776482582 : 0.014999999664723873));
				if (aPiece->mMatchTimer > 1.0f) {
					aPiece->mColumnTag = std::max(theGroup->mTag, aPiece->mColumnTag);
					aPiece->mDestroyerId = theGroup->mOriginId;
					aPiece->mDestroyerFlags |= theGroup->mFlags;
					if (!ClearPiece(aPiece, FindPieceById(theGroup->mOriginId)))
						aPiece->mBusyCount = 1;
				}
				else {
					aListed.push_back(aPiece);
					if (aPiece->mMatchTimer < 0.04f)
						aNew.push_back(aPiece);
				}
			}
		}
	}
	const bool aNoBoltsAtStart = theGroup->mBoltLives.empty();
	int aNeed = 0x14 / ((int)aListed.size() + 1) + 5;
	if (theGroup->mColor == -1)
		aNeed /= 2;
	bool aPick = false;
	if (!aCandidates.empty()) {
		if (aNoBoltsAtStart)
			aPick = true;
		else
			aPick = (unsigned)mRand.Next() % (unsigned)aNeed == 0;
	}
	if (aPick) {
		Piece* aSource = nullptr;
		if (!aNew.empty())
			aSource = aNew[(unsigned)mRand.Next() % (unsigned)aNew.size()];
		else if (!aListed.empty())
			aSource = aListed[(unsigned)mRand.Next() % (unsigned)aListed.size()];
		Piece* aTarget = nullptr;
		if (aSource == nullptr)
			aTarget = aCandidates[(unsigned)mRand.Next() % (unsigned)aCandidates.size()];
		else {
			aTarget = aCandidates.back();
			theGroup->mBoltLives.push_back(0.0f);   // 0x786be0 makes the bolt with life 0
		}
		aTarget->mIsGroupOrigin = true;
	}
	for (size_t aBolt = 0; aBolt < theGroup->mBoltLives.size(); aBolt++) {
		float aLife = (float)((double)theGroup->mBoltLives[aBolt] + 0.012000000104308128);
		theGroup->mBoltLives[aBolt] = aLife;
		if (aLife <= 1.0f) {
			if ((theGroup->mAge & 3) == 0) {
				// 8 points of 2 draws and 6 middle points of 4 draws
				for (int aDraw = 0; aDraw < 8 * 2 + 6 * 4; aDraw++)
					Rand();
			}
		}
		else {
			theGroup->mBoltLives.erase(theGroup->mBoltLives.begin() + aBolt);
			aBolt--;
		}
	}
	if (!aAnyBusy && aListed.empty() && aCandidates.empty() && aNoBoltsAtStart) {
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++) {
				if (mGrid[aRow][aCol] != nullptr)
					mGrid[aRow][aCol]->mFallSpeed = 0.0f;
			}
		}
		theGroup->mLifeCountdown = 0x32;
	}
}

// 0x767e20 (bejeweled3), the busy counts (+0x230), after the countdowns: a piece whose count goes from 1 to 0 goes off. A flame gem, or a
// star gem whose countdown (+0x184) still runs, explodes at its middle for 20 points (Explode); a piece with bit 2, 4, 0x400 or 0x10 is
// cleared by 0x74c570 (with no one as the clearer) and is left when that takes it; any other is destroyed (0x74c770; 300 points
// more for a piece with a bit of 0x1800)
void Board::UpdateBusyPieces()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || aPiece->mBusyCount <= 0)
				continue;
			aPiece->mBusyCount--;
			if (aPiece->mBusyCount != 0)
				continue;
			int aFlags = aPiece->mFlags;
			if ((aFlags & Piece::FLAG_FLAME) != 0 || ((aFlags & Piece::FLAG_STAR) != 0 && aPiece->mSpecialCountdown > 0)) {
				AddPoints(20, aPiece->mColumnTag);
				float aSumX = (float)((float)GetGridX() + aPiece->mX);
				float aSumY = (float)((float)GetGridY() + aPiece->mY);
				Explode(Ftol((double)(float)((double)aSumX + 64.0)), Ftol((double)(float)((double)aSumY + 64.0)));
				continue;
			}
			if ((aFlags & (Piece::FLAG_HYPERCUBE | Piece::FLAG_STAR | 0x400 | 0x10)) != 0 && ClearPiece(aPiece, nullptr))
				continue;
			if ((aFlags & FLAGS_SPECIAL_SWAP) != 0)
				AddPoints(300, aPiece->mColumnTag);
			DestroyPiece(aPiece);
		}
	}
}

// 0x767ad0 (bejeweled3), run by 0x767e20 before the match search: the slide curve (+0x198) of every piece goes on
void Board::StepSlideCurves()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr)
				mGrid[aRow][aCol]->mSlideCurve.IncInVal();
		}
	}
}

// 0x767e20 (bejeweled3), 0x769189: a piece whose slide curve is off 0 is put on the line from its cell to the cell it slides to
void Board::UpdateSlidePositions()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr)
				continue;
			double aOut = aPiece->mSlideCurve.GetOutVal();
			if (aOut == 0.0)
				continue;
			aPiece->mX = (float)((1.0 - aOut) * (double)(aPiece->mCol << 7) + aOut * (double)(aPiece->mSlideCol << 7));
			aPiece->mY = (float)((1.0 - aOut) * (double)(aPiece->mRow << 7) + aOut * (double)(aPiece->mSpecialState << 7));
		}
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
		AddPoints(0x32, thePiece->mColumnTag);
	DrawSparkRandoms();
	thePiece->mDestroyed = true;   // 0x74c770 ends with +0x17d set (a replay, +0x12f8, would not) and slot 176
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
	if (OnPieceRemoving(thePiece))
		return;
	OnPieceClearing(thePiece, true);   // 0x755b80 first calls slot 142 (0x73d2b0) with the piece and 1: QuestBoard's override tells the goal, then Board's calls slot 143
	OnPieceCleared(thePiece);
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

// 0x73d170 (bejeweled3): a counter goes up: the board's own (+0xc84), the profile's statistics (while the game is not a replay or
// a demonstration: +0x1410 and +0x1412 are 0) and the move's record (when it has one: theMoveId is not -1). Each stops at 0x7fffffff
void Board::AddCounter(int theKey, int theValue, int theMoveId)
{
	mCounters[theKey] += theValue;
	if (mCounters[theKey] < 0)
		mCounters[theKey] = 0x7fffffff;
	// port: the profile's statistics (Profile::mStats) are kept by the profile itself and the board does not reach it yet
	if (theMoveId != -1) {
		MoveRecord* aRecord = FindMoveRecord(theMoveId);
		if (aRecord != nullptr) {
			aRecord->mCounters[theKey] += theValue;
			if (aRecord->mCounters[theKey] < 0)
				aRecord->mCounters[theKey] = 0x7fffffff;
		}
	}
}

// 0x73cf10 (bejeweled3): a counter of a move (0 when the record is gone)
int Board::GetMoveCounter(int theMoveId, int theKey)
{
	MoveRecord* aRecord = FindMoveRecord(theMoveId);
	return aRecord != nullptr ? aRecord->mCounters[theKey] : 0;
}

// slots 154 and 155 (0x73bf70, 0x73bf60): how much the level bar needs in this level, and how far it has come
int Board::GetLevelMax() const
{
	return mLevelIndex * 0x2ee + 0x9c4;
}

// slot 188 (0x2f0), 0x744b10 on Board: the level bar's target is the progress over the level's size, at most 1, 0 when the size is 0
float Board::GetBarTarget()
{
	int aMax = GetLevelMax();
	if (aMax == 0)
		return 0.0f;
	float aTarget = (float)GetLevelProgress() / (float)aMax;
	return aTarget > 1.0f ? 1.0f : aTarget;
}

// 0x74df80 with 0x74a210 (bejeweled3): the points times the level number (+0xe0c, when theUsesMultiplier) go up in steps of 10.
// For each step, when theCountsForProgress: the level bar's progress (+0xe00) rises by the 0.8 power of what the move has made
// (the move's points counter over the level's multiplier, +0xe04 + 1) with this step, less the same without it, times 3. Each step is
// worth the level's multiplier (+0xe04 + 1) to the score at +0xd24 and to the points counters. port: the text of the points that
// rises from the place (0x71f030) is left out, and so are its draws of the app's Rand()
void Board::AddPoints(int thePoints, int theMoveId, bool theCountsForProgress, bool theUsesMultiplier)
{
	if (thePoints < 1)
		return;
	float aMultiplier = theUsesMultiplier ? (float)mLevelNumber : 1.0f;
	int aLeft = Ftol((double)((float)thePoints * aMultiplier));
	float aLevelFactor = GetLevelFactor();   // slot 122 (0x73af50, Classic's: +0xe04 + 1; Zen 0x70e330: 1; Speed 0x714090: 5)
	while (aLeft > 0) {
		int aChunk = aLeft < 10 ? aLeft : 10;
		if (theCountsForProgress) {
			int aBase = Ftol((double)GetMoveCounter(theMoveId, COUNTER_POINTS) / (double)aLevelFactor);
			double aRise = std::pow((double)(aBase + aChunk), 0.8) - std::pow((double)aBase, 0.8);
			mLevelProgress += Ftol(aRise * 3.0);
		}
		aLeft -= aChunk;
		int aGain = Ftol((double)aLevelFactor * (double)aChunk);
		AddCounter(COUNTER_POINTS, aGain, theMoveId);
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
