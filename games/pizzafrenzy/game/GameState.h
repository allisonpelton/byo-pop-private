#ifndef __PIZZAFRENZY_GAMESTATE_H__
#define __PIZZAFRENZY_GAMESTATE_H__

// The state of a level that plays (0x18 bytes, constructor 0x4029b0, vtable 0x4f9510): the game screen is shown, the
// game is saved, and each frame is the mode's game's (the application's +0x110). The game's actions end the level
// or the game, or pause it.

#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

class GameState : public engine::State
{
public:
	explicit GameState(PizzaFrenzy* theApp) : mApp(theApp) {}  // 0x4029b0

	// 0x04: 0x4063b0
	void enter() override;
	// 0x0c: 0x402720
	void update(engine::UpdateInfo& theInfo) override;
	// 0x10: 0x4075e0
	void action(const std::string& theAction) override;

private:
	PizzaFrenzy* mApp;  // +0x0c
};

} // namespace pizzafrenzy

#endif
