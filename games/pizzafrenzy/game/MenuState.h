#ifndef __PIZZAFRENZY_MENUSTATE_H__
#define __PIZZAFRENZY_MENUSTATE_H__

// The state of the main menu (0x1c bytes, constructor 0x402920, vtable 0x4f94fc): it shows the main menu, or the
// screen the game came back from (the high scores or the city select), and plays the menu's music. Its actions start
// the games, show the other screens and open the dialogs, which are states of their own that come back to this one.

#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

class MenuState : public engine::State
{
public:
	explicit MenuState(PizzaFrenzy* theApp) : mApp(theApp), mNeedsPlayer(false), mClearGame(false) {}  // 0x402920

	// 0x04: 0x405f90
	void enter() override;
	// 0x0c: 0x403640: a player is asked for when there is none, and the game that ended is cleared away
	void update(engine::UpdateInfo& theInfo) override;
	// 0x10: 0x40c7a0
	void action(const std::string& theAction) override;

private:
	// 0x4022f0: the game that ended is let go; translated next with the game
	void clearEndedGame();
	// The dialog states are made with the screen to show and this state to come back to
	void showDialog(engine::GenericScreen* theScreen);

	PizzaFrenzy* mApp;  // +0x0c
	bool mNeedsPlayer;  // +0x10: there was no player when the state started
	bool mClearGame;    // +0x11: the game that ended is still to be cleared away
};

} // namespace pizzafrenzy

#endif
