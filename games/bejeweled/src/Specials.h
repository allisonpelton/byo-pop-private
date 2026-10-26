#ifndef __SPECIALS_H__
#define __SPECIALS_H__

// port(variant): flame and lightning gems (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.8, stage 11). Not part of the
// original game. Framework-free (GameRules, BoardLayout and MatchFinder only), so tools/rulestest runs its fixtures.
//
// One resolve (Board::ProcessMatchesVariant) in two steps, on the groups MatchFinder found:
//   Creations: each group in the finder's order whose pattern a special's createdBy names (lightning checked
//   first) keeps one gem alive as that special: the first gem the move put into the group (in the move's own
//   resolve: the swapped pair, the twisted four, the slid segment, in MoveSet::MovedCells' order), else the
//   group's middle cell in its scan order and on round from there, never a special or a cell already taken.
//   Blasts: the specials the groups took, in the groups' order, then (with specials.chain) each special a blast
//   catches, in turn: a flame takes the cells within its radius (a square), lightning its row and column. A
//   created special is spared; each blast destroys the gems not yet dying in its area.
// "runN" names a vertical or horizontal run of N gems or more; "diagonal", "L", "T", "plus" and a shape id the
// groups of that kind.

#include "MatchFinder.h"

#include <vector>

namespace Sexy
{

namespace Specials
{

enum Kind
{
	KIND_NONE = 0,
	KIND_FLAME = 1,
	KIND_LIGHTNING = 2
};

typedef MatchFinder::Cell Cell;

// The board one resolve sees: the gems' kinds and which gems the groups took
struct Grid
{
	int		mCols = 0;
	int		mRows = 0;
	bool	mGem[MatchFinder::MAX_ROWS][MatchFinder::MAX_COLS] = {};
	int		mKind[MatchFinder::MAX_ROWS][MatchFinder::MAX_COLS] = {};
	bool	mDying[MatchFinder::MAX_ROWS][MatchFinder::MAX_COLS] = {};  // in: the groups' gems; Blasts adds its own
};

struct Creation
{
	Cell	mCell;
	int		mKind = KIND_NONE;
	int		mGroup = 0;                 // its group's index
};

struct Blast
{
	Cell				mCell;          // the special's
	int					mKind = KIND_NONE;
	std::vector<Cell>	mArea;          // the cells it reaches that are no hole (dirt included), row-major
	std::vector<Cell>	mDestroyed;     // the gems of mArea it took, row-major
};

// Whether the pattern name ("run4", "L", a shape id, ...) names the group
bool	Matches(const MatchFinder::Group& theGroup, const MatchFinder::Spec& theSpec, const std::string& thePattern);
// The special the group makes, KIND_NONE for none: lightning first, then flame
int		KindFor(const MatchFinder::Group& theGroup, const MatchFinder::Spec& theSpec, const GameRules::SpecialRules& theRules);
// The creations of a resolve, in the groups' order; theMoved the cells the move put gems into (empty after a cascade)
void	Creations(const std::vector<MatchFinder::Group>& theGroups, const MatchFinder::Spec& theSpec, const GameRules::SpecialRules& theRules,
			const std::vector<Cell>& theMoved, const Grid& theGrid, std::vector<Creation>* theCreations);
// The blasts of a resolve in the order they go off; marks their gems dying in theGrid. theGrid's dying cells are
// the groups' gems with the creations already spared (not dying).
void	Blasts(const std::vector<MatchFinder::Group>& theGroups, const BoardLayout& theLayout, const GameRules::SpecialRules& theRules,
			const std::vector<Creation>& theCreations, Grid* theGrid, std::vector<Blast>* theBlasts);

}

}

#endif
