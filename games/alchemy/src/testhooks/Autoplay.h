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
//   -autoplayshots=<prefix>  screenshots at the game's milestones

#include <cstdio>

namespace Sexy
{

class AlchemyApp;
class Board;

class AutoplayBot
{
public:
	AlchemyApp*		mApp;
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

public:
	AutoplayBot(AlchemyApp* theApp, unsigned int theSeed);
	~AutoplayBot();

	void			Update();

private:
	int				Rand(int theRange);
	void			Log(const char* theFormat, ...);
	void			Shot(const char* theName, int theNumber);
	void			PlayBoard(Board* theBoard);
	void			Finish();
};

}

#endif
