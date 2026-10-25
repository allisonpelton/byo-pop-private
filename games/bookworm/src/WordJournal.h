#ifndef __BOOKWORM_WORDJOURNAL_H__
#define __BOOKWORM_WORDJOURNAL_H__

// port(mod): the word journal (mod "word-journal", feature "wordJournal"): every word the player plays goes into
// "word-journal.txt" in their player folder, as it is played: a line per word (date, mode, level, word, points),
// a "game" line before a game's first word, an "undo" line when the undo mod takes a word back. The game-over page
// gets a WORD JOURNAL button that opens the journal (JournalWidget.h): the words and games so far, the best and
// the longest word, a heatmap of the letters played against how often each is dealt, and the best word of each
// level.

#include "Common.h"
#include <map>

namespace Sexy
{
class Board;
class BookWormApp;
class WordScore;

struct JournalWord
{
	SexyString			mWord;
	int					mScore;
	int					mLevel;     // 1-based
	int					mDate;      // yyyymmdd
};

// The journal read back: the totals of every word in it (both modes)
struct JournalStats
{
	int					mWords;
	int					mGames;
	long long			mPoints;
	int					mTiles;                 // the tiles the words used (a Qu tile is one)
	int					mFirstDate;             // the first word's date (0: no words)
	JournalWord			mBest;                  // the most points (the first of equals)
	JournalWord			mLongest;               // the most letters (more points among equals)
	int					mTileCounts[26];        // the tiles used, by letter (a Qu tile counts as Q)
	std::map<int, JournalWord> mLevelBests;     // the most points at each level

	JournalStats();
};

namespace WordJournal
{
	bool				Enabled();
	// theWord, just submitted on theBoard (before Submit counts it in the stats)
	void				WordPlayed(Board* theBoard, const WordScore& theWord);
	// The undo mod took the last word back
	void				WordUndone(Board* theBoard);
	// The current player's journal; false when there is none yet
	bool				Load(BookWormApp* theApp, JournalStats* theStats);
}

}

#endif
