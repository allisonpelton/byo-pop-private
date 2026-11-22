// GameMode.h: the kinds of game the board can be started for (the original keeps the number at offset 0x760 of the app).
// The names come from the main menu's buttons and are checked when the board is translated.

#ifndef __GAMEMODE_H__
#define __GAMEMODE_H__

namespace Sexy
{

enum GameMode
{
	GAME_MODE_NONE = 0,
	GAME_MODE_ADVENTURE = 1,   // the main menu's first button: the campaign, stage by stage
	GAME_MODE_QUICK_PLAY = 2,  // the second
	GAME_MODE_DUEL = 3,		   // the third: two players
	GAME_MODE_CHALLENGE = 4,   // the fourth
	GAME_MODE_REPLAY = 5,	   // ShowReplay sets it
	GAME_MODE_DEMO = 6		   // "This mode shows how the Masters play": the screen saver plays it too
};

} // namespace Sexy

#endif
