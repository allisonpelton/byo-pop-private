#ifndef __ALCHEMY_AUTOPLAY_H__
#define __ALCHEMY_AUTOPLAY_H__

// Test hooks: the autoplay bot (-autoplay=<seed>). It plays whole games by calling the board's mouse handlers, so
// everything the game does for a player runs: pieces are put down and lit, rows and columns clear, the forge is used,
// boards are cleared, the game ends and the score table comes up. The bot has its own random numbers (the game's use
// the C library generator and the board's Mersenne twister), so the same -seed and -autoplay always make the same game.
//
//   -autoplay=<seed>         play with the bot (its own seed)
//   -autoplaymode=strategic|timetrial|tutorial   the kind of game (default strategic)
//   -autoplaygames=N         play N games, then quit (default 1)
//   -autoplaycap=N           quit after N updates in any case (default 20000)
//   -autoplaylog=<file>      the log of what happened (tests compare it)
//   -autoplayshots=<prefix>  screenshots as the game goes (the title, the selector, the tutorial's steps, a board cleared, the game over, the scores)
//   -autoplayui              the bot plays through the mouse only (clicks on the title's link, the selector's and the
//                            dialogs' buttons, a right click for the forge, moves and clicks for the pieces) and writes
//                            what it did as script lines to -autoplayscript=<file>, so that a -script run (and origrun
//                            on the original) gets the same input; -autoplaystart=<n> holds the bot back until update n
//   -autoplayskill=<0..2>    (with -autoplayui) clicks that skill level's button before the mode's
//   -autoplaymiss=<n>        (with -autoplayui) one piece in n is put on a cell it cannot go to (the game's messages)
//   -autoplayextra=<n>       (with -autoplayui) before one piece in n the bot clicks the hint button or presses a key (the pause)
//   -statelog=<file>         (any run) each change of the board's values with its update count: StateLog

#include <cstdio>

namespace Sexy
{

class WinAlchApp;
class Board;
class Widget;

class AutoplayBot
{
public:
	WinAlchApp*		mApp;
	unsigned int	mSeed;
	int				mTick;
	int				mGames;             // games started
	int				mWait;              // updates to wait before the next action
	int				mMoves;             // pieces put down in this game
	int				mForges;            // pieces forged in this game
	int				mShots;
	int				mLastState;
	int				mLastBoards;
	int				mClickX;            // where the pointer waits for its click, or -1
	int				mClickY;
	bool			mOverLogged;
	bool			mFinished;
	FILE*			mLog;
	FILE*			mScriptOut;         // -autoplayui: the input as script lines
	int				mPausedFor;         // -autoplayextra: updates the game has been paused
	int				mSelStep;           // -autoplayui: 0 the mode's button comes next, 1 the start button

public:
	AutoplayBot(WinAlchApp* theApp, unsigned int theSeed);
	~AutoplayBot();

	void			Update();

private:
	int				Rand(int theRange);
	void			Log(const char* theFormat, ...);
	void			Shot(const char* theName, int theNumber);
	void			PlayBoard(Board* theBoard);
	void			Finish();
	void			Move(int theX, int theY);
	void			Click(int theX, int theY, int theButton);
	void			ClickWidget(Widget* theWidget);
	void			Char(char theChar);
};

// Test hooks: -statelog=<file> writes a line "<update> <name> <value>" each time one of the board's values changes
// (state, score, moves, discards, boards, the shape and colour of the piece to place, the time trial's pause, the
// forge-full count, the game-over timer, the counts of multiple moves, the run and the longest run, the pause, the
// difficulty (shapes, colours, bomb and skull chances), the elapsed and total seconds, the number of cells that fit and the
// last shape, and the tutorial's flag, step, count of pieces and wait for the button; "-" when there is no board or piece). The original's values for the same names are watched with origrun (--watch), so the two logs are compared
// update by update.
class StateLog
{
public:
	WinAlchApp*		mApp;
	FILE*			mFile;
	enum { NUM_FIELDS = 30 };
	int				mLast[NUM_FIELDS];

public:
	StateLog(WinAlchApp* theApp);
	~StateLog();

	void			Update();
};

}

#endif
