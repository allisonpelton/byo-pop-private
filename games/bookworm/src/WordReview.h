#ifndef __BOOKWORM_WORDREVIEW_H__
#define __BOOKWORM_WORDREVIEW_H__

// port(mod): each word played, against the best word the board had when it was played (the most points of any
// path, by the word solver's search and Submit's own scoring). Two mods show it:
//   "missed-words" (feature "missedWords"): after a word, in Lex's balloon (with the word's definition), the
//   better word the board had and its points; on the level and game-over pages, the points missed.
//   "par" (feature "par"): each board's par is its best word's points; after a word, its points as a
//   percentage of par, and on the pages the level's and the game's percentage.
// The totals are saved with the game (after the record, Board::WriteSavedGame), so an undo (which restores the
// record) takes a word's review back too.

#include "Common.h"
#include <cstdio>
#include <vector>

namespace Sexy
{
class Board;
class WordScore;

struct ReviewTotals
{
	int					mScore;         // the words' points
	int					mPar;           // the best words' points
	int					mWords;
	int					mMissed;        // the points short of the best, summed
	SexyString			mWorstWord;     // the word that fell furthest short, and the best word then
	int					mWorstScore;
	SexyString			mWorstBest;
	int					mWorstBestScore;

	ReviewTotals();
	void				Add(const SexyString& theWord, int theScore, const SexyString& theBest, int theBestScore);
	int					ParPercent() const;     // -1 with no words
};

class WordReview
{
public:
	Board*				mBoard;
	bool				mShowMissed;
	bool				mShowPar;
	int					mLevel;         // the level mLevelTotals counts (-1 before the first word)
	ReviewTotals		mLevelTotals;
	ReviewTotals		mGameTotals;

public:
	WordReview(Board* theBoard, bool showMissed, bool showPar);
	static WordReview*	Create(Board* theBoard);   // NULL with neither mod on

	// theWord, just submitted, while its tiles are still on the board: the note for Lex's balloon
	SexyString			WordPlayed(const WordScore& theWord);
	// The page's line (theLevel: the level's totals, else the game's)
	SexyString			PageLine(bool theLevel) const;
	const ReviewTotals&	LevelTotals() const;   // empty when the level has no word yet

	void				Write(FILE* theFile) const;
	bool				Read(FILE* theFile);
};

}

#endif
