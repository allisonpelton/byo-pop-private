#ifndef __BOOKWORM_SOLVER_H__
#define __BOOKWORM_SOLVER_H__

// port(mod): the word solver, mod "solver" (games/bookworm/mods/solver, feature "solver"). It finds every
// word the board's tiles spell, by every path (one word through a gold tile and around it are two
// candidates), scores each exactly as Submit would (WordScore), and offers hints that the HINT button
// (beside SUBMIT) and Tab cycle through. A hint selects its tiles, as clicks would; the player submits it.
// The order: while fire tiles are on the board, the word that takes the lowest one; the level's bonus word
// when the board spells it; the best-scoring words; the longest word. Safe words come first: in Classic a
// word that leaves out a fire tile on the bottom row ends the game (Submit burns before it removes the
// tiles), and a word that eats the tiles under a fire tile drops it, onto the bottom row at worst, where the
// next word must take it.

#include "Common.h"
#include "Piece.h"
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace Sexy
{
class Board;
class Graphics;

// Calls theVisitor for every path over theTiles (adjacent tiles, each at most once) that spells a word of
// theDictionary (3..12 letters, a Q tile spelling "QU"). A path ends where no word starts with its
// letters. Paths start at the tiles in the order given and continue through the neighbours in that order.
// The -autoplay test bot uses it too.
typedef std::function<void(const std::vector<Piece*>& thePath, const SexyString& theWord)> BoardWordVisitor;
void FindBoardWords(const std::vector<Piece*>& theTiles, const std::set<SexyString>& theDictionary,
	const BoardWordVisitor& theVisitor);

enum SolverHintKind
{
	SOLVER_HINT_FIRE = 0,       // takes the most urgent fire tile
	SOLVER_HINT_BONUS,          // the level's bonus word
	SOLVER_HINT_BEST,           // by points
	SOLVER_HINT_LONGEST         // the most letters (the better special tile: 7+ mostly sapphire, 11+ diamond)
};

enum SolverSafety
{
	SOLVER_SAFE = 0,
	SOLVER_DANGER,              // a fire tile ends on the bottom row: the next word must take it
	SOLVER_FATAL                // classic: a fire tile on the bottom row is left out: the game ends
};

struct SolverWord
{
	SexyString			mWord;
	std::vector<Piece*>	mPath;
	int					mScore;         // Submit's points (WordScore); the bonus word scores more on top
	int					mFireTiles;     // fire tiles in the path
	int					mFireRow;       // the lowest of them (rows 0..7, top to bottom), -1 for none
	int					mFireLeftRow;   // the lowest row a fire tile left out lands on once the word's tiles
	                                    // are gone (the tiles under it eaten), -1 for none
	int					mSafety;        // SolverSafety
	int					mKind;          // SolverHintKind, once it is a hint
};

class Solver
{
public:
	Board*					mBoard;
	std::vector<SolverWord>	mHints;         // in the order the hints come
	int						mHintIdx;       // the hint selected last, -1 for none
	std::string				mBoardSig;      // the board mHints were made for
	bool					mPending;       // a hint was asked for while the tiles were moving

public:
	Solver(Board* theBoard);

	void					RequestHint();          // HINT / Tab: the next hint, now or once the tiles rest
	void					Reset();                // the tiles were replaced (an undo): no hint shown
	void					Update();               // a waiting request
	void					Draw(Graphics* g);      // the tag of the hint the selection shows
	const SolverWord*		ShownHint() const;      // that hint, null when the selection is none
	static bool				BoardAtRest(Board* theBoard);

protected:
	void					Solve();
	void					SelectHint(int theIdx);
	bool					HintShown() const;      // the selection is mHints[mHintIdx]'s path
	std::string				BoardSignature() const;
};

}

#endif
