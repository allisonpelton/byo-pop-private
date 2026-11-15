// Test hooks for the identical-play runs (games/rocketmania/docs/IDENTICAL_PLAY.md):
//   -statelog=<file>   each change of the app's and the board's values, one line "<update> <name> <value>" ("-" when there is
//                      no board), the names the original's watches are matched with (tools/identical/values.txt)
//   -uibot=<seed>      a bot that plays whole games through the mouse only (the loading screen's link, the menu, the dialogs,
//                      the board's tiles) and writes what it did as script lines for the original (-uiscript=<file>)
//   -uimodes=<list>    the modes of the games it plays, in turn (0 arcade, 1 classic, 2 strategy; default 0)
//   -uidiffs=<list>    their difficulties (1 to 3; default 2)
//   -uigames=<n>       how many games (default 1)
//   -uicap=<n>         a game is quit (the board's quit button) after that many board updates (default 6000)
//   -uistart=<n>       the update at which the loading screen's link is clicked (default 400)
//   -uistop=<n>        the update at which the run is ended (the window is closed: a game in progress is saved); 0 none
//   -uihold=<n>        the level-up and game-over screen is left alone for that many updates (its pages turn every 150)
//   -uipassive=<n>     after that many board updates the bot makes no more moves (set -uicap very high: the game then ends by itself)
//   -uicontinue=<n>    1: a game with a saved game waiting takes CONTINUE SAVED GAME (default 0: NEW GAME)
//   -uiname=<text>     the name typed for a high score (default BOT)

#ifndef __ROCKETMANIA_UIBOT_H__
#define __ROCKETMANIA_UIBOT_H__

#include <cstdio>
#include <string>
#include <vector>

namespace Sexy
{

class RocketApp;
class Board;
class Widget;

// the settings of the options above
struct UiBotParams
{
	bool				mOn;
	unsigned int		mSeed;
	std::vector<int>	mModes;
	std::vector<int>	mDiffs;
	int					mGames;
	int					mCap;
	int					mStart;
	int					mStop;
	int					mContinue;
	int					mHold;
	int					mPassive;
	std::string			mName;
	std::string			mScriptFile;
	std::string			mStateFile;

	UiBotParams() : mOn(false), mSeed(0), mGames(1), mCap(6000), mStart(400), mStop(0), mContinue(0), mHold(0), mPassive(0), mName("BOT") {}
	bool				Parse(const std::string& theName, const std::string& theValue);
};

class StateLog
{
public:
	enum { NUM_VALUES = 150 };
	RocketApp*		mApp;
	FILE*			mFile;
	std::string		mLast[NUM_VALUES];
	bool			mStarted;

	StateLog(RocketApp* theApp);
	~StateLog();
	void			Update();
};

class UiBot
{
public:
	struct Click
	{
		int mX, mY, mButton;
	};

	RocketApp*		mApp;
	UiBotParams		mParams;
	FILE*			mScriptOut;
	unsigned int	mSeed;
	int				mGame;              // the game being played (index into the lists)
	int				mGamesDone;
	bool			mWasBoard;
	bool			mLinkClicked;
	bool			mQuitting;          // the cap was reached, the game is being quit
	int				mBusyUntil;         // no input before this update
	int				mWaitDialog;        // the dialog a click was last made on, and when
	int				mWaitSince;
	bool			mNameTyped;
	bool			mFinished;
	std::vector<Click>	mQueue;

	UiBot(RocketApp* theApp, const UiBotParams& theParams);
	~UiBot();
	void			Update();
	bool			PlanPath(Board* theBoard);   // the clicks that turn a path across the board, in mQueue

private:
	int				Next(int theRange);
	void			Input(int theDelay, const char* theCommand, const std::string& theArgs);
	void			ClickAt(int theX, int theY, int theButton, int theDelay);
	void			ClickWidget(Widget* theWidget, int theDelay = 1);
	bool			Dialogs();
	void			PlayBoard(Board* theBoard);
};

}

#endif
