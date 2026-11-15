#ifndef __PIZZAFRENZY_NEWTOPPINGSTATE_H__
#define __PIZZAFRENZY_NEWTOPPINGSTATE_H__

// The state that shows the player's new toppings (0x18 bytes, constructor 0x402ce0, vtable 0x4f9574): the new topping
// screen is shown over the story's music, and the toppings are chosen one at a time until none is new; then the level
// starts, or the pizza designer opens.

#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

class NewToppingState : public engine::State
{
public:
	explicit NewToppingState(PizzaFrenzy* theApp) : mApp(theApp) {}  // 0x402ce0

	// 0x04: 0x406630
	void enter() override;
	// 0x10: 0x4084b0
	void action(const std::string& theAction) override;

private:
	PizzaFrenzy* mApp;  // +0x0c
};

} // namespace pizzafrenzy

#endif
