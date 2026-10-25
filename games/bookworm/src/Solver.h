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
// next word must take it. The setting "saveBonus" saves the bonus tiles (green, gold, sapphire, diamond): every
// list prefers words and paths without them (the fire hint after the lowest fire tile), so they stack up on the
// board, and a CASH IN hint after the best words is the best-scoring word that uses them (each adds to the
// multiplier of the whole word's letters: WordScore::CalcScore). The setting "fewerFire" (Classic) orders each word's
// paths, the fire hint's and the best words by the new fire tiles they bring, the fire meter's drain counted as
// fire to come (SolverFireRater); the tag shows the new fire tiles to expect. The setting "hotkeys": the keys
// 1..9 and 0 play the list's first ten words at once (the side panel of the mod "widescreen" numbers them).

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
	SOLVER_HINT_LONGEST,        // the most letters (the better special tile: 7+ mostly sapphire, 11+ diamond)
	SOLVER_HINT_CASHIN          // "saveBonus": the best-scoring word that uses bonus tiles
};

enum SolverSafety
{
	SOLVER_SAFE = 0,
	SOLVER_SETS_FIRE,           // a mod's word that sets fire to new tiles (ModWordGetEffects, mod "rude-words")
	SOLVER_DANGER,              // a fire tile ends on the bottom row: the next word must take it
	SOLVER_FATAL                // classic: a fire tile on the bottom row is left out: the game ends
};

struct SolverWord;
class BookWormApp;

// Every path over theTiles (the board's lettered tiles, in the order to search them) that spells a word, with
// Submit's points and what it does to the fire (the solver's candidates; the greedy bots' too)
void FindSolverWords(const std::vector<Piece*>& theTiles, BookWormApp* theApp, std::vector<SolverWord>* theWords,
	int theLevel = -1);
// The same rating of one path, theWord's mPath (any letters: the typed word's search); sets its mWord too
void RateSolverWord(SolverWord* theWord, const std::vector<Piece*>& theTiles, BookWormApp* theApp, int theLevel = -1);

// The solver's orders (true: a comes first): safer first, then for the fire hint the lowest fire tile taken,
// the most fire tiles, the remaining fire left highest and the points; by score the points, the remaining
// fire left highest and the length
bool SolverBetterForFire(const SolverWord& a, const SolverWord& b);
bool SolverBetterByScore(const SolverWord& a, const SolverWord& b);

struct SolverWord
{
	SexyString			mWord;
	std::vector<Piece*>	mPath;
	int					mScore;         // Submit's points (WordScore); the bonus word scores more on top
	int					mFireTiles;     // fire tiles in the path
	int					mFireRow;       // the lowest of them (rows 0..7, top to bottom), -1 for none
	int					mFireLeftRow;   // the lowest row a fire tile left out lands on once the word's tiles
	                                    // are gone (the tiles under it eaten), -1 for none
	int					mFireDrops;     // the fire tiles left out that fall (tiles under them eaten)
	int					mSafety;        // SolverSafety
	int					mBonusTiles;    // green, gold, sapphire and diamond tiles in the path
	double				mNewFire;       // SolverFireRater: the new fire tiles to expect from the word's refill and the
	                                    // burns it starts
	int					mNewFireSure;   // of them, the fire meter's: certain
	double				mFireCost;      // mNewFire plus the fire meter's drain in fire tiles (the fewest first)
	int					mKind;          // SolverHintKind, once it is a hint
};

// The fire tiles a word brings in Classic (Board::Submit, Refill and BurnTiles). The word's refill and the refill
// once the tiles it sets burning have burned away each bring a fire tile by chance (gFireChanceByLevel; the first
// hit after a bonus word is skipped) and one for sure when the fire meter has run out (the word's length moves it,
// gFireMeterByLength; a refill of the meter goes with the fire tile); but a due fire tile lands only in a column
// that lost exactly one tile, else it is dropped. The cost adds the meter's drain, at the level's refill
// (gFireMeterRefill) to a fire tile: a short word brings the next fire tile closer. For the solver's setting
// "fewerFire" and the greedy bot GREEDY_LEAST_FIRE. The burns are taken to end in one refill (they mostly do).
class SolverFireRater
{
public:
	// theTiles: the board's lettered tiles; the rest the board's (Board::mFireMeter, mSkipRandomFire, mBonusWord;
	// isLevelsFirst: the next refill is the level's first, which skips the meter)
	SolverFireRater(const std::vector<Piece*>& theTiles, bool isClassic, int theLevel, bool isLevelsFirst,
		double theMeter, bool isSkipping, const SexyString& theBonusWord);
	static SolverFireRater	ForBoard(Board* theBoard, const std::vector<Piece*>& theTiles);
	void					Rate(SolverWord* theWord) const;    // mNewFire, mNewFireSure, mFireCost
	static int				CostOrder(const SolverWord& theWord);   // mFireCost in hundredths, for the hint lists

protected:
	std::vector<Piece*>		mGrid;          // theTiles by [row * BOARD_COLS + col]
	std::vector<Piece*>		mFires;
	bool					mClassic;
	int						mLevel;
	bool					mLevelsFirst;
	double					mMeter;
	bool					mSkipping;
	SexyString				mBonusWord;
};

// The setting "hotkeys": 1..9 and 0 play the first ten words of the list (Board::KeyChar, SidePanel)
bool SolverHotkeys();

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
	// The mod "widescreen"'s side panel lists mHints: made again once the board rests after a change, and a
	// click on one selects it (Tab then goes on from it)
	bool					IsCurrent() const;      // mHints were made for the board as it is
	void					Refresh();
	void					SelectListed(int theIdx);
	// The number keys (setting "hotkeys"): mHints[theIdx] played at once, the list made first if it is not current
	void					PlayListed(int theIdx);

protected:
	void					Solve();
	void					SelectHint(int theIdx);
	bool					HintShown() const;      // the selection is mHints[mHintIdx]'s path
	std::string				BoardSignature() const;
};

}

#endif
