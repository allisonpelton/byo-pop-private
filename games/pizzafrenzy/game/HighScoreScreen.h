#ifndef __PIZZAFRENZY_HIGHSCORESCREEN_H__
#define __PIZZAFRENZY_HIGHSCORESCREEN_H__

// The high scores screen (0x214 bytes, constructor 0x403480, vtable 0x4f95d8): the engine's screen for a scoreboard,
// with the three modes' boards, which the arrows beside the mode's name go through, and the button that clears the
// board shown. The original's second set of boards, the online ones, and the button that switched to them are not
// here: the port has no online scores.

#include "GenericHighScoreScreen.h"
#include "HighScore.h"

namespace pizzafrenzy
{

class HighScoreScreen : public engine::GenericHighScoreScreen
{
public:
	HighScoreScreen() : mMode(0) {}  // 0x403480

	// 0x17c: 0x4371b0: the engine's, and the name of the mode's board above it
	void setScoreboard(engine::Scoreboard* theBoard) override;
	// 0x180: 0x437a10: the three boards, read from the registry, and the screen's first board
	void init();
	// 0x184: 0x437050: the arrows go through the modes' boards; any other action is the application's
	void action(std::string theAction);
	// 0x188: 0x436f70: the screen shows the board of the mode
	void refresh();
	// 0x18c: 0x437750, as the original calls it for the local boards: the board's own buttons are on
	void showLocalScores();
	// 0x190: 0x436db0, for the local boards: the game's score goes on the mode's board when it earns a place, which
	// is saved; true when it did
	bool addCurrentScore();
	// 0x194: 0x436fd0: the mode's board is emptied, which is saved
	void clear();
	// 0x198: 0x437930: each board read from the registry, with the first scores when it has none
	void loadBoards();

private:
	void onAction(std::string theAction) { action(theAction); }  // 0x436c90
	// 0x437260: the first scores of a board with none, which is saved
	void fillDefaultScores(HighScoreBoard* theBoard);

	static const int MODE_COUNT = 3;

	engine::Ref<HighScoreBoard> mBoards[MODE_COUNT];  // +0x1f0: the speed, memory and concentration modes' boards
	int mMode;                                        // +0x208
};

} // namespace pizzafrenzy

#endif
