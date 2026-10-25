#ifndef __BOOKWORM_DAILYBOARD_H__
#define __BOOKWORM_DAILYBOARD_H__

// port(mod): the daily board (mod "daily-board", feature "dailyBoard"): each day deals one game per mode, the same
// for every player that day. A new game seeds the letter generator (gGameRand) and the Board's own (mRand) from
// the date and the mode, and sets what one game hands the next (RandomLetter's previous letter, Refill's previous
// level, the app's bonus word) as a fresh start of the game has them, so the first board and the first bonus word
// follow from the day alone; from there the player's words (and in Action the clock) take each game its own way,
// as the generators' draws follow the tiles used. Lex says the day at the start. The day and both generators are saved with the game (after the
// record, read by Board::LoadGame), so a game continued later deals on as it would have and counts for its own
// day. At game over the result is added to the player's "daily-board.txt" (a line per game: date, mode, score,
// level, words), and the game-over page's balloon shows the day's best game and the player's best day.
// -dailydate=<yyyymmdd> deals another day's board (tests).

#include "Common.h"
#include <cstdio>

namespace Sexy
{
class Board;

class DailyBoard
{
public:
	Board*				mBoard;
	int					mDate;          // yyyymmdd; 0: not a daily game (a game saved without the mod, continued)
	bool				mRecorded;      // the game's result is in the file
	// The game-over balloon's parts, made by GameOver: "Daily board", the day and the results, a line of praise
	SexyString			mTitle;
	SexyString			mSummary;
	SexyString			mPraise;

public:
	DailyBoard(Board* theBoard);
	static DailyBoard*	Create(Board* theBoard);   // NULL with the mod off

	void				Deal();             // the Board ctor: the day's generators (a continued game's are read after)
	SexyString			Greeting() const;   // what Lex says at the start ("" for a game that is not a daily one)
	void				GameOver();         // the result into the player's file (once), and the balloon's parts

	void				Write(FILE* theFile) const;
	bool				Read(FILE* theFile);
};

}

#endif
