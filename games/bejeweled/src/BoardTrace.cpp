// port: -trace (SexyAppBase test hooks); see BoardTrace.h. Not part of the original game. Moved out of
// WinDMApp.cpp (TraceBoard) for the headless verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9).

#include "BoardTrace.h"
#include "Board.h"

#include <cmath>
#include <cstdio>

using namespace Sexy;

void BoardTracer::AfterUpdate(Board* theBoard, int theUpdate)
{
	int aState = (theBoard != NULL) ? theBoard->mState : -1;
	int anUndos = (theBoard != NULL) ? theBoard->mUndosUsed : 0;
	bool settled = (aState == STATE_PLAYING) && ((theBoard != mLastBoard) || (mLastState != STATE_PLAYING) || (anUndos != mLastUndos));
	mLastBoard = theBoard;
	mLastState = aState;
	mLastUndos = anUndos;
	if (settled)
		PrintBoardTrace(theBoard, theUpdate);
}

// port(variant): the move lines of a board with twists or slides (stage 10, games/bejeweled/docs/CODE_MODS_DESIGN.md
// 4.7), in MoveSet's order: a swap's as before; a twist's block, its direction, where the gem that makes the group
// lands and comes from, and the click (a right click anticlockwise) on the point where the four gems meet; a slide's
// line, its segment, k and that gem, and a drag along the segment that makes it (from a cell's centre k cells on, or
// length - k back when that is shorter). The points are screen positions, as the swaps' clicks.
static void PrintVariantMoves(Board* theBoard)
{
	const BoardLayout& aLayout = theBoard->mLayout;
	auto aScreen = [&](int* thePoint)
	{
		if (theBoard->mVariant.mScaled)
		{
			thePoint[0] = (int)floor(aLayout.ToScreenX(thePoint[0]) + 0.5);
			thePoint[1] = (int)floor(aLayout.ToScreenY(thePoint[1]) + 0.5);
		}
	};
	std::vector<MoveSet::Found> aMoves;
	theBoard->FindVariantMoves(&aMoves);
	for (size_t i = 0; i < aMoves.size(); i++)
	{
		const MoveSet::Found& f = aMoves[i];
		const MoveSet::Move& m = f.mMove;
		if (m.mType == GameRules::MOVE_SWAP)
		{
			int aClick[2][2] = {{theBoard->GetColX(m.mA) + 24, theBoard->GetRowY(m.mB) + 24}, {theBoard->GetColX(m.mC) + 24, theBoard->GetRowY(m.mD) + 24}};
			aScreen(aClick[0]);
			aScreen(aClick[1]);
			printf("TRACE   move %d: (%d,%d)->(%d,%d) click %d %d then %d %d\n", (int) i, m.mA, m.mB, m.mC, m.mD,
				aClick[0][0], aClick[0][1], aClick[1][0], aClick[1][1]);
		}
		else if (m.mType == GameRules::MOVE_TWIST)
		{
			int aClick[2] = {theBoard->GetColX(m.mA + 1) - 1, theBoard->GetRowY(m.mB + 1) - 1};
			aScreen(aClick);
			printf("TRACE   move %d: twist (%d,%d) %s lands (%d,%d) from (%d,%d) %s %d %d\n", (int) i, m.mA, m.mB, (m.mC > 0) ? "cw" : "ccw",
				f.mTo.first, f.mTo.second, f.mFrom.first, f.mFrom.second, (m.mC > 0) ? "click" : "rclick", aClick[0], aClick[1]);
		}
		else
		{
			bool isRow = (m.mA == MoveSet::AXIS_ROW);
			bool isBack = (m.mD - m.mE < m.mE);
			int aFrom = isBack ? m.mC + m.mD - 1 : m.mC;
			int aTo = isBack ? aFrom - (m.mD - m.mE) : aFrom + m.mE;
			int aDrag[2][2] = {{theBoard->GetColX(isRow ? aFrom : m.mB) + 24, theBoard->GetRowY(isRow ? m.mB : aFrom) + 24},
				{theBoard->GetColX(isRow ? aTo : m.mB) + 24, theBoard->GetRowY(isRow ? m.mB : aTo) + 24}};
			aScreen(aDrag[0]);
			aScreen(aDrag[1]);
			printf("TRACE   move %d: slide %s %d from %d length %d by %d lands (%d,%d) from (%d,%d) drag %d %d to %d %d\n", (int) i,
				isRow ? "row" : "col", m.mB, m.mC, m.mD, m.mE, f.mTo.first, f.mTo.second, f.mFrom.first, f.mFrom.second,
				aDrag[0][0], aDrag[0][1], aDrag[1][0], aDrag[1][1]);
		}
	}
}

// port(variant): a board with a seed of its own also prints it (a replay's board on the global stream's state has
// an MTRand of its own, but no seed), and a variant board its variant line. A board of a noMoves "continue" rule
// can settle without a move: no move lines then.
void Sexy::PrintBoardTrace(Board* theBoard, int theUpdate)
{
	Board* aBoard = theBoard;
	printf("TRACE update %d: %s level %d score %d board tick %d\n", theUpdate, aBoard->mTimedMode ? "timed" : "normal",
		aBoard->mDisplayLevel, aBoard->mScore, aBoard->mUpdateCnt);
	for (int aRow = 0; aRow < aBoard->mRows; aRow++)
	{
		printf("TRACE   row %d:", aRow);
		for (int aCol = 0; aCol < aBoard->mCols; aCol++)
		{
			// port(variant): a dirt cell (stage 7b) prints as -1 - its layers left (-2..-10), a hole and an empty cell as -1
			if (aBoard->mLayout.IsDirt(aCol, aRow))
				printf(" %d", -1 - (int) aBoard->mLayout.mDirt[aRow][aCol]);
			else
				printf(" %d", (aBoard->mBoard[aRow][aCol] != NULL) ? aBoard->mBoard[aRow][aCol]->mColor : -1);
		}
		printf("\n");
	}
	// port(variant): "user" for a seed the player chose; after the rows, which keep their place under the header
	if (aBoard->mGameSeed != 0)
		printf("TRACE   seed %u %s\n", (unsigned) aBoard->mGameSeed, aBoard->mUserSeeded ? "user" : "pergame");
	// port(variant): a variant board's own state (stage 5: the moves that stood, the kept invalid moves; stage 6:
	// the undos taken, on a board with undo; stage 7c: the direction the next fall takes and the flips so far, when
	// the rules set a gravity rule; stage 7b: the dirt layers left in all and the side the next diagonal slide tries
	// first, when the rules have dirt; stage 9: the groups of each kind the board scored but runs, with match rules)
	if (aBoard->mVariant.mAny)
	{
		printf("TRACE   variant moves %d invalid %d", aBoard->mMoveCount, aBoard->mInvalidMoves);
		if (aBoard->mVariant.mUndo)
			printf(" undos %d", aBoard->mUndosUsed);
		if (aBoard->mRules.HasGravityRule())
			printf(" gravity %s flips %d", aBoard->GravityUp() ? "up" : "down", aBoard->mGravityFlips);
		if (aBoard->mDirtRules)
		{
			int aLayers = 0;
			for (int aRow = 0; aRow < aBoard->mRows; aRow++)
			{
				for (int aCol = 0; aCol < aBoard->mCols; aCol++)
					aLayers += aBoard->mLayout.IsDirt(aCol, aRow) ? aBoard->mLayout.mDirt[aRow][aCol] : 0;
			}
			printf(" dirt %d %s", aLayers, aBoard->mDiagonalLeftFirst ? "left" : "right");
		}
		if (!aBoard->mMatchSpec.IsDefault())
		{
			const int* aCounts = aBoard->mGroupCounts;
			printf(" groups L %d T %d plus %d diagonal %d shapes %d", aCounts[MatchFinder::GROUP_L], aCounts[MatchFinder::GROUP_T],
				aCounts[MatchFinder::GROUP_PLUS], aCounts[MatchFinder::GROUP_DIAGONAL_DOWN] + aCounts[MatchFinder::GROUP_DIAGONAL_UP],
				aCounts[MatchFinder::GROUP_SHAPE]);
		}
		printf("\n");
	}
	// The clicks are screen positions: on a scaled board (stage 7) the cell centres scaled into the window
	const BoardLayout& aLayout = aBoard->mLayout;
	if (aBoard->mVariant.mMoves)
	{
		PrintVariantMoves(aBoard);
		fflush(stdout);
		return;
	}
	int aMove[4];
	for (int i = 0; aBoard->FindMove(aMove, i, true, true); i++)
	{
		int aClick[4] = {aBoard->GetColX(aMove[0]) + 24, aBoard->GetRowY(aMove[1]) + 24, aBoard->GetColX(aMove[2]) + 24, aBoard->GetRowY(aMove[3]) + 24};
		if (aBoard->mVariant.mScaled)
		{
			for (int j = 0; j < 4; j += 2)
			{
				aClick[j] = (int)floor(aLayout.ToScreenX(aClick[j]) + 0.5);
				aClick[j + 1] = (int)floor(aLayout.ToScreenY(aClick[j + 1]) + 0.5);
			}
		}
		printf("TRACE   move %d: (%d,%d)->(%d,%d) click %d %d then %d %d\n", i, aMove[0], aMove[1], aMove[2], aMove[3],
			aClick[0], aClick[1], aClick[2], aClick[3]);
	}
	fflush(stdout);
}
