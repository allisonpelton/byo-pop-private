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

private:
	void Queue(const std::string& theCommand, const std::string& theArgs);
	void Click(int theX, int theY, const char* theWhat);
	void Shot(int theGame, const char* theEvent);
	bool PickGroup(Board* theBoard, int* theX, int* theY, int* theSize);
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
	std::string mCheckArgs;
	std::string mCheckWhat;
	int mCheckSince = 0;
};

}

#endif
