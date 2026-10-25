#ifndef __MATCHFINDER_H__
#define __MATCHFINDER_H__

// port(variant): the match rules' finder (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.6, stage 9). Not part of the
// original game. Framework-free (GameRules and BoardLayout only), so tools/rulestest runs its fixtures.
//
// FindGroups emits the groups of a grid of colours in this order: the vertical runs (columns left to right, each
// top to bottom: the original pass 0), the horizontal runs (rows top to bottom: pass 1), then with match.diagonal
// the "\" runs and the "/" runs, then the cell patterns (match.shapes ids in sorted order, anchors row-major,
// orientations in GameRules::ShapeOrientations order). A pattern instance whose cells all lie in one group found
// before it is skipped. With match.crossings "merge", runs of one colour that share a cell are one group, at the
// place of its first run, an L, a T or a plus by where the runs cross. With the default rules the groups are the
// original's runs in the original's order.

#include "GameRules.h"

#include <string>
#include <utility>
#include <vector>

namespace Sexy
{

namespace MatchFinder
{

enum
{
	MAX_COLS = BoardLayout::MAX_COLS,
	MAX_ROWS = BoardLayout::MAX_ROWS,
	NO_GEM = -1
};

typedef std::pair<int, int> Cell;           // (col, row)

// The colour of each cell, NO_GEM for a hole, an empty cell or a cell off the grid
struct ColorGrid
{
	int		mCols = 0;
	int		mRows = 0;
	int		mColor[MAX_ROWS][MAX_COLS];

	int		At(int theCol, int theRow) const
	{
		return ((theCol < 0) || (theRow < 0) || (theCol >= mCols) || (theRow >= mRows)) ? (int) NO_GEM : mColor[theRow][theCol];
	}
};

enum GroupKind
{
	GROUP_VERTICAL,
	GROUP_HORIZONTAL,
	GROUP_DIAGONAL_DOWN,                    // "\": down and to the right
	GROUP_DIAGONAL_UP,                      // "/": down and to the left
	GROUP_L,                                // merged runs that cross at an end of each
	GROUP_T,                                // ... at an end of one and inside the other
	GROUP_PLUS,                             // ... inside both
	GROUP_SHAPE
};

struct Group
{
	GroupKind			mKind = GROUP_VERTICAL;
	int					mShape = -1;        // GROUP_SHAPE: its index in Spec::mShapes
	int					mColor = NO_GEM;
	std::vector<Cell>	mCells;             // a run's in its scan order; a merged group's runs' in order, each cell once; a shape's in its orientation's order
	int					mRunLength = 0;     // a straight run's length (vertical, horizontal, diagonal); 0 for the others

	bool				IsRun() const { return mKind <= GROUP_DIAGONAL_UP; }
	bool				Contains(int theCol, int theRow) const;
};

// The match rules as the finder reads them
struct Spec
{
	struct Shape
	{
		std::string							mId;
		std::vector<std::vector<Cell> >		mOrientations;
	};

	int					mMinRun = 3;
	bool				mDiagonal = false;
	int					mDiagonalMinRun = 3;
	bool				mMerge = false;
	std::vector<Shape>	mShapes;            // ids in sorted order

	static Spec			FromRules(const GameRules& theRules);
	bool				IsDefault() const;   // the original's: runs of 3, nothing else
};

void		FindGroups(const ColorGrid& theGrid, const Spec& theSpec, std::vector<Group>* theGroups);
// Whether the grid has any group (the fill's test, HasMatchVariant)
bool		AnyGroup(const ColorGrid& theGrid, const Spec& theSpec);
// Whether some group of FindGroups would hold the cell: with theHorizontal alone a horizontal run through it, with
// theVertical alone a vertical run, with both any group (FindMove's contract). A local scan around the cell.
bool		InGroup(const ColorGrid& theGrid, const Spec& theSpec, int theCol, int theRow, bool theHorizontal, bool theVertical);
// The group's match.bonus key ("diagonal", "L", "T", "plus" or a shape id); NULL for a vertical or horizontal run
const char*	BonusKey(const Group& theGroup, const Spec& theSpec);

}

}

#endif
