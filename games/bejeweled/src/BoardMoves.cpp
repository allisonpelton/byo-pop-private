// port(variant): twists and slides (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.4, 4.5 and 4.7): states
// 10..13, the input layer of a board with those moves, and their draw. Not part of the original game.
//
// A twist turns a 2x2 block a quarter turn in TWIST_STEPS ticks, at MoveSet's integer cos and sin; a slide moves a
// segment's gems k cells on in moves.slide.snapTicks ticks, wrapping inside the segment. Both run as a swap runs
// (UpdateSwapping, 0x4cd84f): at the end the move is made in the grid and FindSets decides; a move without a group
// goes back (with bad2), or stays with moves.allowInvalid (KeepInvalidMove). One that goes back is undone in the grid
// at once and only its gems' positions turn back, so the grid holds the board before the move in all four states.

#include "Board.h"
#include "WinDMApp.h"
#include "Res.h"
#include "ReplayRecorder.h"
#include "Graphics.h"

#include <algorithm>
#include <cstdlib>

using namespace Sexy;

namespace
{

// The move that takes a move back
MoveSet::Move Inverse(const MoveSet::Move& theMove)
{
	MoveSet::Move aMove = theMove;
	if (aMove.mType == GameRules::MOVE_TWIST)
		aMove.mC = -aMove.mC;
	else if (aMove.mType == GameRules::MOVE_SLIDE)
		aMove.mE = aMove.mD - aMove.mE;
	return aMove;
}

// x modulo theSpan into 0 .. theSpan - 1
int Wrap(int theValue, int theSpan)
{
	return ((theValue % theSpan) + theSpan) % theSpan;
}

}

MoveSet::Options Board::MoveOptions(bool horizontal, bool vertical) const
{
	MoveSet::Options anOptions;
	anOptions.mTypes = mRules.mMoves.mTypes;
	anOptions.mTwistBoth = mRules.mMoves.mTwistBoth;
	anOptions.mHorizontal = horizontal;
	anOptions.mVertical = vertical;
	return anOptions;
}

bool Board::FindVariantMove(MoveSet::Found* theFound, int theMoveNum)
{
	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	return MoveSet::FindMove(aGrid, mLayout, mMatchSpec, MoveOptions(true, true), theMoveNum, theFound);
}

void Board::FindVariantMoves(std::vector<MoveSet::Found>* theMoves)
{
	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	MoveSet::FindMoves(aGrid, mLayout, mMatchSpec, MoveOptions(true, true), theMoves);
}

// What the input layer records and a TWIST event must meet (WatchController): the board in play and open to input
// (MouseDown's gates), the rules' twists in that direction, four gems in the block
bool Board::CanTwist(int theCol, int theRow, int theDir)
{
	if (!(mRules.mMoves.mTypes & GameRules::MOVE_TWIST) || !((theDir == 1) || ((theDir == -1) && mRules.mMoves.mTwistBoth)))
		return false;
	if ((mState != STATE_PLAYING) || (mUserPauseCount != 0) || (mInputDelay != 0))
		return false;
	if ((theCol < 0) || (theRow < 0) || (theCol + 1 >= mCols) || (theRow + 1 >= mRows))
		return false;
	for (int aRow = theRow; aRow <= theRow + 1; aRow++)
	{
		for (int aCol = theCol; aCol <= theCol + 1; aCol++)
		{
			if (!mLayout.IsPlayable(aCol, aRow) || (mBoard[aRow][aCol] == NULL))
				return false;
		}
	}
	return true;
}

// ... and a SLIDE event: a whole segment of gems that starts at theStart, 1 <= k < its length, the release offset
// within one length either way
bool Board::CanSlide(int theAxis, int theLine, int theStart, int theK, int theOffset)
{
	if (!(mRules.mMoves.mTypes & GameRules::MOVE_SLIDE))
		return false;
	if ((mState != STATE_PLAYING) || (mUserPauseCount != 0) || (mInputDelay != 0))
		return false;
	if ((theAxis != MoveSet::AXIS_ROW) && (theAxis != MoveSet::AXIS_COL))
		return false;
	bool isRow = (theAxis == MoveSet::AXIS_ROW);
	if ((theLine < 0) || (theLine >= (isRow ? mRows : mCols)) || (theStart < 0) || (theStart >= (isRow ? mCols : mRows)))
		return false;
	int aStart = 0;
	int aLength = 0;
	if (!MoveSet::SegmentAt(mLayout, theAxis, isRow ? theStart : theLine, isRow ? theLine : theStart, &aStart, &aLength) ||
		(aStart != theStart) || (aLength < 2) || (theK < 1) || (theK >= aLength))
		return false;
	if ((theOffset < -aLength * CELL_SIZE) || (theOffset > aLength * CELL_SIZE))
		return false;
	for (int i = 0; i < aLength; i++)
	{
		if ((isRow ? mBoard[theLine][aStart + i] : mBoard[aStart + i][theLine]) == NULL)
			return false;
	}
	return true;
}

// A twist's gems at theStep of its quarter turn about the block's centre, from the cells they are in
static void SetTwistPositions(Board* theBoard, int theStep)
{
	const MoveSet::Move& aMove = theBoard->mMoveInPlay;
	int aCenterX = (theBoard->GetColX(aMove.mA) + theBoard->GetColX(aMove.mA + 1)) / 2;
	int aCenterY = (theBoard->GetRowY(aMove.mB) + theBoard->GetRowY(aMove.mB + 1)) / 2;
	int aCos = 0;
	int aSin = 0;
	MoveSet::TwistCosSin(theStep, &aCos, &aSin);
	aSin *= aMove.mC;
	for (int aRow = aMove.mB; aRow <= aMove.mB + 1; aRow++)
	{
		for (int aCol = aMove.mA; aCol <= aMove.mA + 1; aCol++)
		{
			Piece* aPiece = theBoard->mBoard[aRow][aCol];
			int anOffX = theBoard->GetColX(aCol) - aCenterX;
			int anOffY = theBoard->GetRowY(aRow) - aCenterY;
			aPiece->mX = aCenterX + (anOffX * aCos - anOffY * aSin) / 1024;
			aPiece->mY = aCenterY + (anOffX * aSin + anOffY * aCos) / 1024;
		}
	}
}

// A slide's gems theOffset px on from the cells they are in, wrapped inside the segment
static void SetSlidePositions(Board* theBoard, int theOffset)
{
	const MoveSet::Move& aMove = theBoard->mMoveInPlay;
	int aSpan = aMove.mD * Board::CELL_SIZE;
	for (int i = 0; i < aMove.mD; i++)
	{
		int aPos = Wrap(i * Board::CELL_SIZE + theOffset, aSpan);
		if (aMove.mA == MoveSet::AXIS_ROW)
		{
			Piece* aPiece = theBoard->mBoard[aMove.mB][aMove.mC + i];
			aPiece->mX = theBoard->GetColX(aMove.mC) + aPos;
			aPiece->mY = theBoard->GetRowY(aMove.mB);
		}
		else
		{
			Piece* aPiece = theBoard->mBoard[aMove.mC + i][aMove.mB];
			aPiece->mX = theBoard->GetColX(aMove.mB);
			aPiece->mY = theBoard->GetRowY(aMove.mC) + aPos;
		}
	}
}

// The move made in the grid, its gems at rest in their new cells
static void CommitMove(Board* theBoard, const MoveSet::Move& theMove)
{
	MoveSet::Apply(theBoard->mBoard, theMove);
	MoveSet::Cell aFrom[MoveSet::MAX_MOVED];
	MoveSet::Cell aTo[MoveSet::MAX_MOVED];
	int aCount = MoveSet::MovedCells(theMove, aFrom, aTo);
	for (int i = 0; i < aCount; i++)
	{
		Piece* aPiece = theBoard->mBoard[aTo[i].second][aTo[i].first];
		aPiece->mCol = aTo[i].first;
		aPiece->mRow = aTo[i].second;
		aPiece->mX = theBoard->GetColX(aPiece->mCol);
		aPiece->mY = theBoard->GetRowY(aPiece->mRow);
	}
	// arrows.turn: a twist turns the arrows it carries with it, and the twist back turns them back
	if ((theMove.mType == GameRules::MOVE_TWIST) && theBoard->mRules.mArrows.mTurn)
	{
		for (int i = 0; i < aCount; i++)
		{
			Piece* aPiece = theBoard->mBoard[aTo[i].second][aTo[i].first];
			if (aPiece->mArrow > 0)
				aPiece->mArrow = Board::TurnArrow(aPiece->mArrow, theMove.mC);
		}
	}
}

void Board::StartTwist(int theCol, int theRow, int theDir)
{
	if (mVariant.mUndo) TakeUndoSnapshot();
	mSelectedPiece = NULL;
	mMoveInPlay = MoveSet::Move();
	mMoveInPlay.mType = GameRules::MOVE_TWIST;
	mMoveInPlay.mA = theCol;
	mMoveInPlay.mB = theRow;
	mMoveInPlay.mC = theDir;
	mMoveTick = 0;
	mState = STATE_TWISTING;
	MarkDirty();
}

void Board::StartSlide(int theAxis, int theLine, int theStart, int theK, int theOffset)
{
	bool isRow = (theAxis == MoveSet::AXIS_ROW);
	int aStart = 0;
	int aLength = 0;
	MoveSet::SegmentAt(mLayout, theAxis, isRow ? theStart : theLine, isRow ? theLine : theStart, &aStart, &aLength);
	if (mVariant.mUndo) TakeUndoSnapshot();
	mSelectedPiece = NULL;
	mMoveInPlay = MoveSet::Move();
	mMoveInPlay.mType = GameRules::MOVE_SLIDE;
	mMoveInPlay.mA = theAxis;
	mMoveInPlay.mB = theLine;
	mMoveInPlay.mC = aStart;
	mMoveInPlay.mD = aLength;
	mMoveInPlay.mE = theK;
	// The animation goes from the release to k cells on, the way round that is nearer: a drag of one cell back is
	// k = length - 1, and slides one cell back. Its length in ticks is the rules', whatever the offset
	int aSpan = aLength * CELL_SIZE;
	int aTo = theK * CELL_SIZE;
	mSlideFrom = theOffset;
	mSlideTo = (abs(aTo - aSpan - theOffset) < abs(aTo - theOffset)) ? aTo - aSpan : aTo;
	mMoveTick = 0;
	mState = STATE_SLIDING;
	SetSlidePositions(this, mSlideFrom);
	MarkDirty();
}

// UpdateSwapping's end (0x4cd84f) for a twist or a slide that has run: the move stands with a group, goes back, or with
// moves.allowInvalid stays
static void EndForwardMove(Board* theBoard, WinDMApp* theApp, int theBackState)
{
	CommitMove(theBoard, theBoard->mMoveInPlay);
	theBoard->mCascadeCount = 1;
	theBoard->mMoveMatchCount = 0;
	if (theBoard->FindSets())
	{
		theBoard->mState = STATE_CLEARING;
		if (theBoard->mShowTutorial)
		{
			theBoard->mShowTutorial = false;
			theApp->KillDialog(DIALOG_INSTRUCTIONS);
			theApp->mShowInstructions = false;
		}
		theBoard->mIsNewGame = false;
		theBoard->OnMoveMade();
	}
	else if (!theBoard->mVariant.mInvalid)
	{
		// Undone in the grid at once; the gems turn or slide back from where the move left them
		CommitMove(theBoard, Inverse(theBoard->mMoveInPlay));
		if (theBackState == STATE_TWISTING_BACK)
			SetTwistPositions(theBoard, MoveSet::TWIST_STEPS);
		else
			SetSlidePositions(theBoard, theBoard->mSlideTo);
		theBoard->mMoveTick = 0;
		theApp->PlaySample(SOUND_BAD);
		theBoard->mState = theBackState;
	}
	else
		theBoard->KeepInvalidMove();
}

void Board::UpdateTwist()
{
	MarkDirty();
	mMoveTick++;
	bool isBack = (mState == STATE_TWISTING_BACK);
	if (mMoveTick < MoveSet::TWIST_STEPS)
		SetTwistPositions(this, isBack ? MoveSet::TWIST_STEPS - mMoveTick : mMoveTick);
	else if (!isBack)
		EndForwardMove(this, mApp, STATE_TWISTING_BACK);
	else
	{
		SetTwistPositions(this, 0);
		mState = STATE_PLAYING;
		if (mVariant.mUndo) PopUndoSnapshot();
	}
}

void Board::UpdateSlide()
{
	MarkDirty();
	mMoveTick++;
	int aTicks = mRules.mMoves.mSlideSnapTicks;
	bool isBack = (mState == STATE_SLIDING_BACK);
	if (mMoveTick < aTicks)
		SetSlidePositions(this, isBack ? mSlideTo - mSlideTo * mMoveTick / aTicks : mSlideFrom + (mSlideTo - mSlideFrom) * mMoveTick / aTicks);
	else if (!isBack)
		EndForwardMove(this, mApp, STATE_SLIDING_BACK);
	else
	{
		SetSlidePositions(this, 0);
		mState = STATE_PLAYING;
		if (mVariant.mUndo) PopUndoSnapshot();
	}
}

// SaveGame drops a swap in progress to STATE_PLAYING; a twist or a slide too, its gems back in their cells (the
// grid holds the board before the move)
void Board::EndVariantMove()
{
	MoveSet::Cell aFrom[MoveSet::MAX_MOVED];
	MoveSet::Cell aTo[MoveSet::MAX_MOVED];
	int aCount = MoveSet::MovedCells(mMoveInPlay, aFrom, aTo);
	for (int i = 0; i < aCount; i++)
	{
		Piece* aPiece = mBoard[aFrom[i].second][aFrom[i].first];
		if (aPiece == NULL)
			continue;
		aPiece->mX = GetColX(aPiece->mCol);
		aPiece->mY = GetRowY(aPiece->mRow);
	}
	mMoveTick = 0;
	mState = STATE_PLAYING;
}

// The block a click at (x, y) twists (4.5): the nearest intersection of four cells, clamped inside the board;
// with isNearOnly only within 13 px (a quarter cell) of it, as on a board that also swaps
bool Board::TwistAt(int x, int y, bool isNearOnly, int* theCol, int* theRow)
{
	if ((mCols < 2) || (mRows < 2))
		return false;
	if ((x < GetColX(0)) || (x >= GetColX(mCols - 1) + GEM_SIZE) || (y < GetRowY(0)) || (y >= GetRowY(mRows - 1) + GEM_SIZE))
		return false;
	int anIc = std::max(1, std::min(mCols - 1, (x - mLayout.mOriginX + 26) / CELL_SIZE));
	int anIr = std::max(1, std::min(mRows - 1, (y - mLayout.mOriginY + 26) / CELL_SIZE));
	if (isNearOnly && ((abs(x - (GetColX(anIc) - 1)) > 13) || (abs(y - (GetRowY(anIr) - 1)) > 13)))
		return false;
	*theCol = anIc - 1;
	*theRow = anIr - 1;
	return true;
}

// MouseDown (0x4ca6b0) after its gates (no user pause, no input delay, not in a game over, the vortex or the wipe)
// on a board with twists or slides, by the move types (4.4's table): a click where four gems meet twists them
// clockwise (on a board without swaps, a click anywhere: the nearest such point), a right click there anticlockwise
// with "both"; with slides a press that twists waits for the release, since a drag of it slides. False leaves the
// press to MouseDown: a swap board's select and target, or the right click's deselect.
bool Board::MouseDownVariant(int x, int y, int theClickCount)
{
	int aTypes = mRules.mMoves.mTypes;
	bool hasSwap = (aTypes & GameRules::MOVE_SWAP) != 0;
	bool hasTwist = (aTypes & GameRules::MOVE_TWIST) != 0;
	bool hasSlide = (aTypes & GameRules::MOVE_SLIDE) != 0;
	mPressCol = -1;
	mPressRow = -1;
	mPressTwist = false;
	mDragAxis = -1;
	mDragOffset = 0;

	int aCol = 0;
	int aRow = 0;
	bool isTwistPoint = hasTwist && (mState == STATE_PLAYING) && TwistAt(x, y, hasSwap, &aCol, &aRow);
	if (theClickCount <= 0)
	{
		if (!isTwistPoint || !mRules.mMoves.mTwistBoth)
			return false;
		if (CanTwist(aCol, aRow, -1))
		{
			if (mRecorder != NULL) mRecorder->OnTwist(aCol, aRow, -1);
			if (mStats != NULL) mStats->OnTwist();
			StartTwist(aCol, aRow, -1);
		}
		return true;
	}

	// The cell a drag would slide: the one under the press
	if (hasSlide)
	{
		int aCellX = x - mLayout.mOriginX;
		int aCellY = y - mLayout.mOriginY;
		if ((aCellX >= 0) && (aCellY >= 0) && (aCellX / CELL_SIZE < mCols) && (aCellY / CELL_SIZE < mRows) &&
			(mBoard[aCellY / CELL_SIZE][aCellX / CELL_SIZE] != NULL))
		{
			mPressCol = aCellX / CELL_SIZE;
			mPressRow = aCellY / CELL_SIZE;
		}
	}
	if (isTwistPoint)
	{
		if (hasSlide)
			mPressTwist = true;
		else if (CanTwist(aCol, aRow, 1))
		{
			if (mRecorder != NULL) mRecorder->OnTwist(aCol, aRow, 1);
			if (mStats != NULL) mStats->OnTwist();
			StartTwist(aCol, aRow, 1);
		}
		return true;
	}
	return !hasSwap;
}

// MouseDrag (0x4ca8c1) on a board with slides: past 16 px (ties vertical, as there) the drag takes the axis and the
// segment of the pressed cell, and moves that line with the mouse, for the draw only; no drag-swap. False on a board
// without slides: the original's drag-swap.
bool Board::MouseDragVariant(int x, int y)
{
	if (!(mRules.mMoves.mTypes & GameRules::MOVE_SLIDE))
		return false;
	if (!mMouseDown || (mPressCol < 0) || (mState != STATE_PLAYING))
		return true;
	int aDeltaX = x - mMouseDownX;
	int aDeltaY = y - mMouseDownY;
	if (mDragAxis < 0)
	{
		if ((abs(aDeltaX) < 16) && (abs(aDeltaY) < 16))
			return true;
		int anAxis = (abs(aDeltaX) > abs(aDeltaY)) ? MoveSet::AXIS_ROW : MoveSet::AXIS_COL;
		int aStart = 0;
		int aLength = 0;
		mPressTwist = false;
		if (!MoveSet::SegmentAt(mLayout, anAxis, mPressCol, mPressRow, &aStart, &aLength) || (aLength < 2))
		{
			mPressCol = -1;
			return true;
		}
		mDragAxis = anAxis;
		mDragStart = aStart;
		mDragLength = aLength;
	}
	int aSpan = mDragLength * CELL_SIZE;
	mDragOffset = std::max(-aSpan, std::min(aSpan, (mDragAxis == MoveSet::AXIS_ROW) ? aDeltaX : aDeltaY));
	MarkDirty();
	return true;
}

// MouseUp (0x4ca8b7) on a board with twists or slides: a drag slides its line k = round(offset / 51) cells on,
// modulo the segment (none: it snaps back, no move), and a press that waited twists, both if the board is still open
// to them
void Board::MouseUpVariant(int x, int y)
{
	bool hasSwap = (mRules.mMoves.mTypes & GameRules::MOVE_SWAP) != 0;
	if (mDragAxis >= 0)
	{
		int anOffset = mDragOffset;
		int aCells = (anOffset >= 0) ? (anOffset + 25) / CELL_SIZE : -((25 - anOffset) / CELL_SIZE);
		int aK = Wrap(aCells, mDragLength);
		int aLine = (mDragAxis == MoveSet::AXIS_ROW) ? mPressRow : mPressCol;
		if ((aK != 0) && CanSlide(mDragAxis, aLine, mDragStart, aK, anOffset))
		{
			if (mRecorder != NULL) mRecorder->OnSlide(mDragAxis, aLine, mDragStart, aK, anOffset);
			if (mStats != NULL) mStats->OnSlide();
			StartSlide(mDragAxis, aLine, mDragStart, aK, anOffset);
		}
		MarkDirty();
	}
	else if (mPressTwist)
	{
		int aCol = 0;
		int aRow = 0;
		if (TwistAt(mMouseDownX, mMouseDownY, hasSwap, &aCol, &aRow) && CanTwist(aCol, aRow, 1))
		{
			if (mRecorder != NULL) mRecorder->OnTwist(aCol, aRow, 1);
			if (mStats != NULL) mStats->OnTwist();
			StartTwist(aCol, aRow, 1);
		}
	}
	mPressCol = -1;
	mPressRow = -1;
	mPressTwist = false;
	mDragAxis = -1;
	mDragOffset = 0;
}

bool Board::DrawsSlideCell(int theCol, int theRow)
{
	int anAxis = 0;
	int aLine = 0;
	int aStart = 0;
	int aLength = 0;
	if ((mState == STATE_SLIDING) || (mState == STATE_SLIDING_BACK))
	{
		anAxis = mMoveInPlay.mA;
		aLine = mMoveInPlay.mB;
		aStart = mMoveInPlay.mC;
		aLength = mMoveInPlay.mD;
	}
	else if ((mState == STATE_PLAYING) && (mDragAxis >= 0))
	{
		anAxis = mDragAxis;
		aLine = (mDragAxis == MoveSet::AXIS_ROW) ? mPressRow : mPressCol;
		aStart = mDragStart;
		aLength = mDragLength;
	}
	else
		return false;
	int aPos = (anAxis == MoveSet::AXIS_ROW) ? theCol : theRow;
	return (((anAxis == MoveSet::AXIS_ROW) ? theRow : theCol) == aLine) && (aPos >= aStart) && (aPos < aStart + aLength);
}

// A sliding line (or the line a drag shows): its gems clipped to the segment, each one that sticks out past an end
// drawn again at the other
void Board::DrawMovesUnder(Graphics* g)
{
	bool isSliding = (mState == STATE_SLIDING) || (mState == STATE_SLIDING_BACK);
	if (!isSliding && !((mState == STATE_PLAYING) && (mDragAxis >= 0)))
		return;
	int anAxis = isSliding ? mMoveInPlay.mA : mDragAxis;
	int aLine = isSliding ? mMoveInPlay.mB : (mDragAxis == MoveSet::AXIS_ROW) ? mPressRow : mPressCol;
	int aStart = isSliding ? mMoveInPlay.mC : mDragStart;
	int aLength = isSliding ? mMoveInPlay.mD : mDragLength;
	bool isRow = (anAxis == MoveSet::AXIS_ROW);
	int aSpan = aLength * CELL_SIZE;
	int anOriginX = isRow ? GetColX(aStart) : GetColX(aLine);
	int anOriginY = isRow ? GetRowY(aLine) : GetRowY(aStart);

	Graphics aLineG(*g);
	aLineG.ClipRect(anOriginX, anOriginY, isRow ? aSpan - 2 : GEM_SIZE, isRow ? GEM_SIZE : aSpan - 2);
	for (int i = 0; i < aLength; i++)
	{
		Piece* aPiece = isRow ? mBoard[aLine][aStart + i] : mBoard[aStart + i][aLine];
		if (aPiece == NULL)
			continue;
		int aPos = isSliding ? (int)((isRow ? aPiece->mX - anOriginX : aPiece->mY - anOriginY)) : Wrap(i * CELL_SIZE + mDragOffset, aSpan);
		Rect aSrc(aPiece->mSpinFrame * GEMSHEET_PITCH + 2, aPiece->mColor * GEMSHEET_COLOR_HEIGHT, GEM_SIZE, GEM_SIZE);
		for (int aCopy = 0; aCopy < 2; aCopy++)
		{
			int anAt = aPos - aCopy * aSpan;
			if ((aCopy == 1) && (aPos + GEM_SIZE <= aSpan))
				break;
			int aX = isRow ? anOriginX + anAt : anOriginX;
			int aY = isRow ? anOriginY : anOriginY + anAt;
			if (!mVariant.mArrows || !DrawArrowGem(&aLineG, aPiece, aX, aY)) // an arrow gem slides as one
				aLineG.DrawImage(mApp->mImages[IMAGE_GEMSHEET], aX, aY, aSrc);
		}
	}
}

// The ring of the block a click would twist (the hovered one; the turning one while it turns): the four corners of
// the selection cursor around the block
void Board::DrawMovesOver(Graphics* g)
{
	if (!(mRules.mMoves.mTypes & GameRules::MOVE_TWIST))
		return;
	int aCol = 0;
	int aRow = 0;
	if ((mState == STATE_TWISTING) || (mState == STATE_TWISTING_BACK))
	{
		aCol = mMoveInPlay.mA;
		aRow = mMoveInPlay.mB;
	}
	else if ((mState != STATE_PLAYING) || (mDragAxis >= 0) || (mMouseX < 0) ||
		!TwistAt(mMouseX, mMouseY, (mRules.mMoves.mTypes & GameRules::MOVE_SWAP) != 0, &aCol, &aRow))
		return;
	int aFrame = (mCursorFrame % 6) * CURSOR_FRAME_SIZE;
	int aX = GetColX(aCol) - 2;
	int aY = GetRowY(aRow) - 2;
	Image* aCursor = mApp->mImages[IMAGE_TARGET];
	g->DrawImage(aCursor, aX, aY, Rect(aFrame, 0, 26, 26));
	g->DrawImage(aCursor, aX + 78, aY, Rect(aFrame + 26, 0, 26, 26));
	g->DrawImage(aCursor, aX, aY + 78, Rect(aFrame, 26, 26, 26));
	g->DrawImage(aCursor, aX + 78, aY + 78, Rect(aFrame + 26, 26, 26, 26));
}
