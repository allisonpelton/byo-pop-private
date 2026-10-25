#ifndef __BIGMONEY_AUTOPLAY_H__
#define __BIGMONEY_AUTOPLAY_H__

// port: test hook for -autoplay=<seed> (see WinBMApp.h). Not part of the original game.
//
// A bot that plays Big Money through script input: after every update it looks at the game and queues
// its next mouse or key event with SexyAppBase::AddScriptCommand, so its input takes exactly the path of
// a -script file's commands. Its choices come from its own std::mt19937, never from the game's
// generators (the Board's MTRand and CrtRand), so for a given -seed and -autoplay seed the run is the
// same every time, and -autoplaylog=<file> writes a -script file that replays it on any build.
//
// What it does: clicks the title screen's play link, picks the game type (-autoplaymode=0..2, Action /
// Strategy / Puzzle) and skill (-autoplayskill=1..3) on the selector and starts, starts the game from
// the help screen, then plays: mostly the largest clickable group, sometimes a random clickable group,
// rarely a random cell (an invalid click). It clicks "play next level" when a level is done, enters a
// name for a high score, and plays again until -autoplaygames games are over (default 1), then quits
// 100 updates after the last game-over screen, or at -autoplaycap (default 200000 updates).
// -autoplayshot=<prefix> saves <prefix>-<game>-<event>.png at level ends and game overs.

#include <cstdio>
#include <random>
#include <string>

namespace Sexy
{

class WinBMApp;
class Board;

class AutoplayBot
{
public:
	AutoplayBot(WinBMApp* theApp, unsigned long theSeed, int theMode, int theSkill, int theGames, int theUpdateCap,
		const std::string& theLogFileName, const std::string& theShotPrefix);
	~AutoplayBot();

	// After each update (WinBMApp::UpdateFrames): looks at the game, queues input for the next update
	void Update();

	// -autoplaysolver[=N]: in Puzzle with the solver on, N% of the clicks are its plan's next group (waiting
	// while it thinks), so a test can see each plan win in the game; -autoplayhints presses H before each
	// (a test of hint prices in replays)
	int mFollowSolver = 0;
	bool mPressHints = false;
	// -autoplayundo[=N]: N% of the bot's turns in a Puzzle game with undos press U, whether or not the coins
	// rest; -autoplaycheck: with the solver's warnings on, a click on a group first moves the mouse there
	// and waits (up to 30 seconds) for the check of its click
	int mPressUndo = 0;
	bool mCheckClicks = false;
	// -autoplaypocket[=N]: with Pickpockets in hand, N% of the bot's turns (and every turn with no group to
	// click) lift a small group: by P and a click, or by a right click, in turn
	int mUsePockets = 0;
	// -autoplaybest=N: N% of the bot's picks are the largest group, the others a random clickable group (default
	// 3 in 4); -autoplaypace=N: about N updates between its clicks, N/2 to 3N/2 - 1 (default 20 to 59). For
	// bots that play like a given player (mod balance benchmarks).
	int mBest = -1;
	int mPace = 0;
	// -autoplaybags: the best pick is a group under a bag when there is one (PickGroup); -autoplaylook[=H,B]:
	// the best pick is the one that looks best one click ahead (its points and the groups it leaves, less H
	// per row of the tallest column, plus B for a bag freed; 10 and 30 by default)
	bool mBags = false;
	bool mLook = false;
	// -autoplaypocketbags: Pickpockets as a player might use them: on a small group right under a bag, or
	// when nothing can be clicked (instead of -autoplaypocket's N% of the turns)
	bool mPocketBags = false;
	int mLookHeight = 10;
	int mLookBag = 30;

private:
	void Queue(const std::string& theCommand, const std::string& theArgs);
	void Click(int theX, int theY, const char* theWhat);
	void Shot(int theGame, const char* theEvent);
	bool PickGroup(Board* theBoard, int* theX, int* theY, int* theSize);
	int ClickDelay();
	bool UsePickpocket(Board* theBoard, int theNow);
	int HoverVerdict(Board* theBoard);

	WinBMApp* mApp;
	std::mt19937 mRand;
	unsigned long mSeed;
	int mMode;
	int mSkill;
	int mGamesWanted;
	int mUpdateCap;
	FILE* mLog;
	std::string mShotPrefix;
	int mNextAction;        // update count at which the bot may act again
	int mGamesOver;         // game-over screens seen
	int mLevelsDone;        // level-done screens seen
	int mLastBoardState;
	Board* mLastBoard;
	int mQuitAt;            // update count of the quit, once decided
	bool mDone;
	bool mCheckPending = false; // -autoplaycheck: the mouse is on a group, the click waits for its check
	int mPocketsUsed = 0;       // -autoplaypocket: by P and a click when odd
	std::string mCheckArgs;
	std::string mCheckWhat;
	int mCheckSince = 0;
};

}

#endif
