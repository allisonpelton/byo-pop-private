#ifndef __BOOKWORM_UNDO_H__
#define __BOOKWORM_UNDO_H__

// port(mod): undo, mod "undo" (games/bookworm/mods/undo, feature "undo"). Before each word Submit takes and
// each scramble, the board is kept as its saved game would be (Board::SaveGameSnapshot), with the game's two
// generators whole (the record keeps the board generator's state words but not where it is in them), the
// letter generator's program-wide memory (Board::GetGlobalState) and the app's next bonus word (a bonus word
// played picks it, avoiding the one before). UNDO (beside OPTIONS) and Ctrl+Z put the last one back once the
// tiles rest, so the board, score, fire meter, Lex and the bonus word are as they were and the same word
// played again brings the same new tiles. Only in play and within a level: a level-up leaves the earlier
// level's moves behind. The clocks run on.

#include "Buffer.h"
#include "Common.h"
#include "MTRand.h"
#include <vector>

namespace Sexy
{
class Board;

class UndoStack
{
public:
	struct Snapshot
	{
		Buffer		mRecord;        // Board::SaveGameSnapshot
		MTRand		mBoardRand;     // Board::mRand
		MTRand		mGameRand;      // gGameRand
		int			mLastLetterIdx; // Board::GetGlobalState
		int			mLastRefillLevel;
		SexyString	mNextBonusWord; // BookWormApp::mBonusWord
		int			mLevel;
	};

	Board*					mBoard;
	std::vector<Snapshot>	mSnapshots;     // the oldest first
	bool					mPending;       // asked for while the tiles moved

public:
	UndoStack(Board* theBoard);

	void					Push();             // before a word or a scramble changes the board
	void					RequestUndo();      // UNDO / Ctrl+Z: now, or once the tiles rest
	void					Update();           // a waiting request
	int						Count();            // the moves this level that can be undone

protected:
	bool					BoardReady();
	void					Restore();
};

}

#endif
