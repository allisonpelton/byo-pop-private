#pragma once

// Test hooks shared by testhooks/TestCommands.cpp, Snapshot.cpp and StateLog.cpp. Everything here only reads the game
// (state, move, -statelog) or is called from a test command; nothing is reachable from a normal run.

#include "Bot.h"

#include <string>
#include <vector>

namespace Sexy
{

class WinWSApp;
class Board;
class Dialog;

// the dialogs the game opens, in the order a driver should meet them (the topmost first): their ids and test names
struct TestDialog
{
	int				mId;
	const char*		mName;
};
extern const TestDialog gTestDialogs[];
extern const int gNumTestDialogs;

// the name of the topmost open dialog ("gameover", "hint", "message", ...), or "" when none is open
std::string TestOpenDialog(WinWSApp* theApp);

// the screen on view: a dialog's name when one is open, else title, stats, highscores, tutor, board, map, menu or none;
// theBase gets the screen under a dialog
std::string TestScreen(WinWSApp* theApp, std::string* theBase);

// every creature of the board as a snapshot record, in the order of the state's list: sharks, piranhas, clams, bonus creatures,
// the boss, the boss's shots (a reads-only walk of the board's lists)
std::vector<BotCreature> TestCollectCreatures(Board* theBoard);

// what the bot looks at (Snapshot.cpp)
BoardSnapshot TestTakeSnapshot(WinWSApp* theApp);

// the line after "STATE " (Snapshot.cpp)
std::string TestStateJson(WinWSApp* theApp);

// the line of -statelog for the current update (StateLog.cpp), "" when there is no board
std::string TestStateLogLine(WinWSApp* theApp);

}
