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
