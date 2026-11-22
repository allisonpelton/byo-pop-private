#ifndef __FEEDINGFRENZY2_MENUSTATE_H__
#define __FEEDINGFRENZY2_MENUSTATE_H__

// The application's state between games (FeedingFrenzyTwo::MenuState, 0x1c bytes, vtable 0x55a858 (feedingfrenzy2);
// its constructor 0x49a350 (feedingfrenzy2)): the main menu over the menu's stage, or the map or the high scores when
// a game sends the player back to them, with the player's name and welcome. Once the main menu is first shown, a
// player with no name is asked for one. Translated so far: the screens shown and hidden, the player's texts and the
// name entry's popup; the stage, the music, the other popups and the menu's actions are not yet translated.

#include "FeedingFrenzyTwo.h"

class MenuState : public AppState
{
public:
	// 0x49a350 (feedingfrenzy2): the player not yet looked at
	explicit MenuState(FeedingFrenzyTwo* theApp) : AppState(theApp), mPlayerChecked(false) {}

	// 0x04 (0x48d320 (feedingfrenzy2)): the menu's screen shown with the player's texts, the stage running
	void enter() override;
	// 0x08 (0x49ad10 (feedingfrenzy2)): the screen hidden (the map stays, for the game it starts)
	void exit() override;
	// 0x0c (0x431530 (feedingfrenzy2)): once the main menu is shown, a player with no name asked for one
	void update(engine::UpdateContext* theContext) override;
	// 0x10 (0x4112f0 (feedingfrenzy2)): the main menu's, the map's, the high scores' and the credits' buttons
	void action(const std::string& theAction) override;
	// 0x14 (0x492df0 (feedingfrenzy2)): the screen hidden under a popup
	void suspend() override;
	// 0x18 (0x48fc00 (feedingfrenzy2)): the menu's screen shown again, with the player's texts
	void resume() override;
	// (0x1c (0x53f2f1 (feedingfrenzy2)), the F key switching between the window and the full screen, is called by
	// the application's key slot, not yet translated)

private:
	// 0x419fc0 (feedingfrenzy2): the main menu's player name and welcome, and its trophies shown once a mode's every
	// stage is reached
	void showPlayer();
	// The newGame and newTimedGame actions (inlined in 0x4112f0 (feedingfrenzy2)): a new game of the mode, then the
	// question whether to go on with the saved one, the map once a stage of the mode has been reached, or else the
	// first stage's load
	void startGame(int theMode);
	// An action holding levelStart (inlined in 0x4112f0 (feedingfrenzy2)), the map's: a new game of the mode from
	// the stage of the id after the ':', its record's saved game dropped
	void startStage(const std::string& theStageId);

	bool mPlayerChecked;  // +0x10
};

#endif
