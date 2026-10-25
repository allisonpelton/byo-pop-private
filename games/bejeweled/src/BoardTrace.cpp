// port: -trace (SexyAppBase test hooks); see BoardTrace.h. Not part of the original game. Moved out of
// WinDMApp.cpp (TraceBoard) for the headless verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9).

#include "BoardTrace.h"
#include "Board.h"

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
			printf(" %d", (aBoard->mBoard[aRow][aCol] != NULL) ? aBoard->mBoard[aRow][aCol]->mColor : -1);
		printf("\n");
	}
	// port(variant): "user" for a seed the player chose; after the rows, which keep their place under the header
	if (aBoard->mGameSeed != 0)
		printf("TRACE   seed %u %s\n", (unsigned) aBoard->mGameSeed, aBoard->mUserSeeded ? "user" : "pergame");
	// port(variant): a variant board's own state (stage 5: the moves that stood, the kept invalid moves; stage 6:
	// the undos taken, on a board with undo)
	if (aBoard->mVariant.mAny && aBoard->mVariant.mUndo)
		printf("TRACE   variant moves %d invalid %d undos %d\n", aBoard->mMoveCount, aBoard->mInvalidMoves, aBoard->mUndosUsed);
	else if (aBoard->mVariant.mAny)
		printf("TRACE   variant moves %d invalid %d\n", aBoard->mMoveCount, aBoard->mInvalidMoves);
	int aMove[4];
	for (int i = 0; aBoard->FindMove(aMove, i, true, true); i++)
		printf("TRACE   move %d: (%d,%d)->(%d,%d) click %d %d then %d %d\n", i, aMove[0], aMove[1], aMove[2], aMove[3],
			aBoard->GetColX(aMove[0]) + 24, aBoard->GetRowY(aMove[1]) + 24, aBoard->GetColX(aMove[2]) + 24, aBoard->GetRowY(aMove[3]) + 24);
	fflush(stdout);
}
