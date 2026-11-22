// port: the test bot behind the `move` script command (docs/TEST_COMMANDS.md, "The bot behind move"), and the names
// of the game's screens that `state` and `goto` use. It reads the game's state and never changes it; it draws from no
// generator of the game's (it has none of its own).
#pragma once

#include <string>

namespace Sexy
{

// What screen the game shows: popcap, loading, transition (the doors), menu, intro, level, stats, victory, map,
// slots (the monkeytown screen between levels is "map"; "intro" is a level's opening cartoon or the map screen's first
// picture)
const char* BotScreen();

// The screen's own state, as a word (the menu's window, the level's state, the map screen's state)
const char* BotSubState();

// The next move as `MOVE`'s JSON object (input, updates, what), or "null" when the game is over or nothing can be done
std::string BotMoveJson();

// The extra keys of `state bugs`: every bug (kind, x, y, box width, box height, alive) in the 1920x1200 design space
std::string BotBugListJson();

// The main menu's buttons as [id, x, y] in window pixels
std::string BotMenuJson();

// True when the game is over: the lives are gone, or the victory scene shows
bool BotGameOver();

// The window point (800x600) of a ground cell the monkey points at, as the game's own aim finds it; false when the
// level is not on screen
bool BotCellToWindow(int theColumn, int theRow, int* theX, int* theY);

// The pending goto or level takes a step (TestCommands.cpp); called once an update by GameWidget::Update
void TestHooksUpdate();

} // namespace Sexy
