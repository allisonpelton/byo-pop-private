#ifndef __PIZZAFRENZY_PIZZAEDITORSTATE_H__
#define __PIZZAFRENZY_PIZZAEDITORSTATE_H__

// The state that shows the pizza editor (0x1c bytes, constructor 0x402e00, vtable 0x4f959c): the editor's screen is
// shown, and its buttons' actions go to the editor (PizzaEditor). Its exit goes on to the level, with the pizzas the
// player designed counted, when the designer's introduction opened it, and else to the menu; naming a pizza shows the
// name screen as a dialog over it.

#include "PizzaFrenzy.h"
#include "State.h"

namespace pizzafrenzy
{

class PizzaEditorState : public engine::State
{
public:
	// 0x402e00: the application, and whether the designer's introduction opened the editor
	PizzaEditorState(PizzaFrenzy* theApp, bool theFromNewTopping) :
		mApp(theApp), mFromNewTopping(theFromNewTopping)
	{
	}

	// 0x04: 0x4027d0: the editor's screen is shown (the game's water is drawn) and the music stops
	void enter() override;
	// 0x08: 0x401150
	void exit() override;
	// 0x10: 0x408250: "editorExit" leaves and "namePizza" shows the name screen
	void action(const std::string& theAction) override;

private:
	PizzaFrenzy* mApp;     // +0x0c
	bool mFromNewTopping;  // +0x10
};

} // namespace pizzafrenzy

#endif
