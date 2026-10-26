#ifndef __BOARDTRACE_H__
#define __BOARDTRACE_H__

// port: -trace (SexyAppBase test hooks). Not part of the original game. Each time a board settles into
// STATE_PLAYING it prints the grid's gem colours, the score and the valid swaps (as click coordinates), for
// writing -script runs:
//   TRACE update <update>: <normal|timed> level <level> score <score> board tick <tick>
//   TRACE   row <row>: <colour of each column, -1 for an empty cell>
//   TRACE   seed <seed> <user|pergame>        a board with a seed of its own (code mods, stage 2)
//   TRACE   variant moves <n> invalid <n>     a variant board (stage 5: the moves that stood, the kept invalid ones;
//                                             stage 6: then " undos <n>", the undos taken, when its rules have undo)
//   TRACE   gold: <a word a row>             a gold board (stage 12; the variant line then ends " gold <done>/<goal>",
//                                             and " time <left>/<limit>" in ticks with a time limit): 1 gold, 0 not
//                                             yet, . a cell outside the goal
//   TRACE   move <i>: (<col>,<row>)->(<col2>,<row2>) click <x> <y> then <x2> <y2>
// <update> is the caller's count (the app's update count in the game); everything after it is the board's.
// Moved out of WinDMApp.cpp (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9), so the headless verifier
// (games/bejeweled/src/headless) prints the same lines as the game.

#include <cstddef>

namespace Sexy
{

class Board;

// Follows the board on screen from one update to the next: a board settles when it is in STATE_PLAYING after
// an update and was not before (another board, or the same one in another state), or it took an undo since
// (stage 6: the board a move was made from is back)
class BoardTracer
{
public:
	BoardTracer() : mLastBoard(NULL), mLastState(-1), mLastUndos(0) {}

	// After an update: theBoard is the board on screen, NULL for none. Prints its block when it settled.
	void			AfterUpdate(Board* theBoard, int theUpdate);

private:
	const Board*	mLastBoard;
	int				mLastState;
	int				mLastUndos;
};

// The settle block of theBoard (stdout, flushed)
void				PrintBoardTrace(Board* theBoard, int theUpdate);

}

#endif
