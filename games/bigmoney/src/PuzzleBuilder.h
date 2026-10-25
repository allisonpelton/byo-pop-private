#ifndef __BIGMONEY_PUZZLEBUILDER_H__
#define __BIGMONEY_PUZZLEBUILDER_H__

// port(mods): Puzzle boards that can always be won (feature "winnablePuzzles", mods/winnable-puzzles; Board's
// code for it is marked port(mods)). Not in the original.
//
// The original builds a Puzzle board backwards too (Board::GenerateBoard): it pushes small groups of one
// colour into the columns, the coins above moving up, as if clicks were taken back. But a group can come
// out split (grown under one of its own coins, another coin ends up between them) or touching coins of its
// colour, so taking the groups away in the reverse order is not always a legal win, and a board is not
// always winnable. This builder grows its groups the same way, from the board's own random numbers, and
// keeps a group only if it is exactly what a click on it takes: connected, and touching no other coin of its
// colour. Clicking the groups in the reverse order then undoes each push exactly (the coins above fall back
// where they were). A column is started only next to the ones already there, and by one group at most, so
// the columns also empty one at a time from the outside in and no gap ever closes (two emptied at once could
// leave a bag landing in the outer one, and the game closes the gap between it and the board before the bag
// leaves). So the reverse order clears the board and drops every bag: a win,
// which Board checks on the solver's model of the board (PuzzleModel) and hands to the solver, if it is on.

#include "PuzzleSolver.h"
#include <vector>

namespace Sexy
{

class MTRand;

class PuzzleBuilder
{
public:
	// The coins of a board (no bags) from theRand, with colours 0..theColors-1, in the first theCols columns
	// and on the floor of the model, theRows high at most: every column 3/5 full at least (9 of 15, the
	// original's "row 6") and two rows short of the top at most; thePlan gets the clicks that clear it, in
	// order
	static void	Build(MTRand& theRand, int theColors, int theCols, int theRows, bool isDiagonal, PuzzleModel* theBoard,
					std::vector<PuzzleSolver::Move>* thePlan);
};

}

#endif
