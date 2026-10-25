#ifndef __BOOKWORM_GREEDYBOTS_H__
#define __BOOKWORM_GREEDYBOTS_H__

// port(mod): the greedy bots, mod "greedy-bots" (games/bookworm/mods/greedy-bots, feature "greedyBots"). At the
// start of a Classic game (its first board, before the first word) the board, the game's generator and the
// rest of the state the rules draw on are kept (BotGame::Capture). Three simple players then play their own
// games from that start, a few moves a tick while the player plays (GreedyBots::Update): one always plays the
// best-scoring word, one puts out the most urgent fire first, and one does that too but then brings the fewest
// new fire tiles (the solver's "fewerFire", SolverFireRater). The game-over page shows how far each got (LevelUpWidget), and a saved
// game keeps their results and the games still going (Board's saved-game trailer, GreedyBots::Write; a game
// saved with two bots, before the third, reads with the third absent).
//
// BotGame is a headless model of Classic's rules: Board::Submit, Scramble, Refill, BurnTiles, RemovePieces,
// AwardSpecialTile, PickBonusWord and LevelUp, drawing from its copy of gGameRand in the game's order, and of
// the tile pass of Board::Update as far as it decides when burning tiles are gone (each tick in which tiles
// burn away refills the board once). A move is made with every tile at rest, as the solver and the -autoplay
// bot wait for; the level's summary is left once the tiles rest. The -autoplay bot checks the model against
// the game (-autoplaymodel=1): its moves replayed in a BotGame give the same boards and scores.
// Words score at the game's own level (WordScore::mLevel; the game's scoring reads the board's).
// Not modelled: a shaking tile's jitter (drawn from the board's mRand), so a gold, sapphire or diamond tile
// burning under a fire tile may burn away a few ticks sooner than in the game, which matters only when other
// tiles burn away in between; and the bonus banner's slide (after a bonus word the model has the next one at
// once, the game once the banner is back).

#include "Common.h"
#include "Board.h"
#include "MTRand.h"
#include <cstdio>
#include <string>
#include <vector>

namespace Sexy
{
class BookWormApp;
class Piece;
class WordScore;

// A headless Classic game from a captured start
class BotGame
{
public:
	BookWormApp*	mApp;
	Piece*			mGrid[BOARD_ROWS][BOARD_COLS];  // [row][col] as Board::mBoard (slot [0][even] unused)
	MTRand			mGameRand;                      // gGameRand's draws
	int				mLastLetterIdx;                 // Board's program-wide RandomLetter and Refill state
	int				mLastRefillLevel;
	int				mState;                         // BOARD_PLAYING, BOARD_LEVEL_COMPLETE (until the tiles
	                                                // rest) or BOARD_GAMEOVER_PENDING (the game is over)
	int				mLevel;                         // 0-based
	double			mFireMeter;
	bool			mSkipRandomFire;
	int				mRewardStreak;
	int				mFireTileCount;
	int				mBonusWordsGiven;
	SexyString		mBonusWord;                     // the banner's, "" while none is up
	SexyString		mNextBonusWord;                 // the app's mBonusWord
	int				mScore;                         // the game's stats: points, words, bonus words, scrambles
	int				mWords;
	int				mBonusWords;
	int				mScrambles;
	int				mLevelScore;                    // the level's: points (the excess carried over), words
	int				mLevelWords;
	int				mModFireDue;                    // Board::mModFireDue (a mod's word, rude-words)
	int				mMoves;                         // words and scrambles

public:
	BotGame();
	~BotGame();

	void			Capture(Board* theBoard);       // the board as it is now (at rest), gGameRand's state
	std::vector<Piece*> Tiles() const;              // the lettered tiles in row order (the solver's search)
	Piece*			At(int theRow, int theCol) const { return mGrid[theRow][theCol]; }
	bool			IsOver() const { return mState == BOARD_GAMEOVER_PENDING; }
	bool			Submit(const std::vector<Piece*>& thePath); // a dictionary word's tiles (this game's)
	bool			CanScramble() const;
	bool			Scramble();
	std::string		State() const;                  // the -autoplay bot's BoardState of this game
	void			Write(FILE* theFile) const;     // between moves (no tile burns, every tile at rest)
	bool			Read(FILE* theFile, BookWormApp* theApp);

protected:
	int				RandomLetter();
	Piece*			Below(Piece* thePiece) const;
	void			Refill();
	void			SetModFire(const std::vector<Piece*>& theNewTiles, int theCount);
	void			BurnTiles();
	void			AwardSpecialTile(const WordScore& theWord);
	void			RemoveBonusTiles();
	void			PickBonusWord();
	void			ShowBonusWord();
	void			Settle();
	void			Clear();

private:
	BotGame(const BotGame&);
	BotGame&		operator=(const BotGame&);
};

enum GreedyBotKind
{
	GREEDY_BEST_WORD = 0,           // the best-scoring word (its bonus-word points included) that does not
	                                // end the game at once
	GREEDY_FIRE_FIRST,              // the safest word, one that takes the lowest fire tile first (the
	                                // solver's fire hint), else the best-scoring
	GREEDY_LEAST_FIRE,              // the safest word, one that takes the lowest fire tile first, then the
	                                // fewest new fire tiles (SolverFireRater's cost), then the best-scoring
	NUM_GREEDY_BOTS
};

enum GreedyBotEnd
{
	GREEDY_PLAYING = 0,
	GREEDY_BURNED,                  // a fire tile burned through the bottom
	GREEDY_STUCK,                   // no word and no scramble
	GREEDY_CAPPED,                  // stopped at GREEDY_MAX_MOVES moves, still alive
	GREEDY_ABSENT                   // not in the saved game (saved before this bot was added)
};

struct GreedyBotResult
{
	int				mEnd;           // GreedyBotEnd
	int				mScore;
	int				mLevel;         // 0-based
	int				mWords;
};

class GreedyBots
{
public:
	BotGame*		mGames[NUM_GREEDY_BOTS];
	GreedyBotResult	mResults[NUM_GREEDY_BOTS];
	bool			mHasResults;    // started from this game's start, or read from its saved game
	double			mElapsedMs;     // the time their moves took (this run of the game)

public:
	GreedyBots();
	~GreedyBots();

	void			Start(Board* theBoard);
	bool			Finished() const;
	bool			Update(double theBudgetMs);     // plays moves for about theBudgetMs; true: a game ended
	bool			Read(FILE* theFile, BookWormApp* theApp);  // false: no bots in the file, or a broken record

	// the results and the games still going; theBots NULL: that there are none
	static void		Write(FILE* theFile, const GreedyBots* theBots);
	static bool		PlayMove(BotGame* theGame, int theKind);    // false: no word and no scramble
};

}

#endif
