#ifndef __MOVESET_H__
#define __MOVESET_H__

// port(variant): the moves of the move rules (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.4, 4.5 and 4.7, stage 10):
// swaps, twists of a 2x2 block and slides of a line. Not part of the original game. Framework-free (GameRules,
// BoardLayout and MatchFinder only), so tools/rulestest checks the search against a brute force.
//
// The search visits the moves that make a group in a fixed order: the swaps in the original FindMove's (cells
// row-major, then the directions right, left, down, up; a swap counts when the gem it brings into the cell lands in
// a group there, so each swap is found from both of its cells), then the twists (anchors row-major; clockwise, then
// anticlockwise with moves.twist.direction "both"), then the slides (rows top to bottom, then columns left to
// right, each line's segments in order, k = 1 .. length - 1). A twist or a slide counts once, at the first of its
// moved gems (in MovedCells' order) that lands in a group. With the swaps alone it finds what stage 9's search found.

#include "BoardLayout.h"
#include "GameRules.h"
#include "MatchFinder.h"

#include <vector>

namespace Sexy
{

namespace MoveSet
{

enum
{
	AXIS_ROW = 0,               // a slide along a row: its gems move right
	AXIS_COL = 1,               // ... along a column: down
	MAX_MOVED = (BoardLayout::MAX_COLS > BoardLayout::MAX_ROWS) ? BoardLayout::MAX_COLS : BoardLayout::MAX_ROWS,
	TWIST_STEPS = 18,           // a twist's ticks: 5 degrees each, the swap's pace
	TWIST_STEP_DEGREES = 5
};

typedef MatchFinder::Cell Cell;

struct Move
{
	int		mType = GameRules::MOVE_SWAP;
	// swap: the gems of (mA, mB) and (mC, mD) change places (FindMove's {col, row, col2, row2})
	// twist: the 2x2 block whose top-left cell is (mA, mB) turns, mC 1 clockwise on screen (top-left to top-right,
	//   to bottom-right, to bottom-left), -1 anticlockwise
	// slide: mA the axis, mB the row or column, mC the segment's first cell along it, mD its length, mE = k: every
	//   gem of the segment goes k cells on (right or down), 1 <= k < length, wrapping inside the segment
	int		mA = 0;
	int		mB = 0;
	int		mC = 0;
	int		mD = 0;
	int		mE = 0;
};

// A move the search found: theMove brings the gem at mFrom into mTo, where it lands in a group
struct Found
{
	Move	mMove;
	Cell	mTo;
	Cell	mFrom;
};

struct Options
{
	int		mTypes = GameRules::MOVE_SWAP;  // moves.type
	bool	mTwistBoth = false;             // moves.twist.direction "both"
	bool	mHorizontal = true;             // FindMove's: what counts as the group the moved gem lands in
	bool	mVertical = true;               //   (MatchFinder::InGroup)
};

// The gems a move moves: the gem at theFrom[i] goes to theTo[i]; the count (2, 4 or the segment's length)
int			MovedCells(const Move& theMove, Cell* theFrom, Cell* theTo);

// The move made on a grid ([row][col], of colours or of pieces)
template<class T> void Apply(T (*theGrid)[BoardLayout::MAX_COLS], const Move& theMove)
{
	Cell aFrom[MAX_MOVED];
	Cell aTo[MAX_MOVED];
	int aCount = MovedCells(theMove, aFrom, aTo);
	T aMoved[MAX_MOVED];
	for (int i = 0; i < aCount; i++)
		aMoved[i] = theGrid[aFrom[i].second][aFrom[i].first];
	for (int i = 0; i < aCount; i++)
		theGrid[aTo[i].second][aTo[i].first] = aMoved[i];
}

// The theMoveNum-th move of the search (0 the first) into theFound (may be NULL); false when there are fewer
bool		FindMove(const MatchFinder::ColorGrid& theGrid, const BoardLayout& theLayout, const MatchFinder::Spec& theSpec,
				const Options& theOptions, int theMoveNum, Found* theFound);
// Every move of the search, in its order
void		FindMoves(const MatchFinder::ColorGrid& theGrid, const BoardLayout& theLayout, const MatchFinder::Spec& theSpec,
				const Options& theOptions, std::vector<Found>* theMoves);

// The segment of gem cells (theLayout's maximal run of playable cells) along theAxis through (theCol, theRow):
// its first cell along the axis and its length; false for a cell that is not playable
bool		SegmentAt(const BoardLayout& theLayout, int theAxis, int theCol, int theRow, int* theStart, int* theLength);

// cos and sin of a twist's angle (TWIST_STEP_DEGREES a step, 0..TWIST_STEPS steps) times 1024, truncated as C
// truncates: baked, so a twist's positions are the same integers on every platform
void		TwistCosSin(int theStep, int* theCos, int* theSin);

}

}

#endif
