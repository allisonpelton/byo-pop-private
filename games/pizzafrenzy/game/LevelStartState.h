#ifndef __PIZZAFRENZY_LEVELSTARTSTATE_H__
#define __PIZZAFRENZY_LEVELSTARTSTATE_H__

// The state of a level's start (0x1c bytes, constructor 0x402c40, vtable 0x4f9560): the level start screen is shown
// over the story's music for the player to pick the vans' toppings, and when they are picked the level is set up. A
// game with fewer toppings than that goes straight on.

#include "Level.h"
#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

class LevelStartState : public engine::State
{
public:
	// 0x402c40: the level the start is of
	LevelStartState(PizzaFrenzy* theApp, Level* theLevel) : mApp(theApp), mLevel(theLevel) {}

	// 0x04: 0x407850
	void enter() override;
	// 0x10: 0x407940
	void action(const std::string& theAction) override;

private:
	PizzaFrenzy* mApp;  // +0x0c
	Level* mLevel;      // +0x10: nothing reads it
};

} // namespace pizzafrenzy

#endif
