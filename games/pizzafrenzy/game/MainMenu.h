#ifndef __PIZZAFRENZY_MAINMENU_H__
#define __PIZZAFRENZY_MAINMENU_H__

// The main menu (0x1ec bytes, constructor 0x43a730, vtable 0x4fdb90): the engine's generic screen with a welcome text
// that types itself out, in the speech bubble beside the player's name. A button the mouse is over has it say what the
// button does; when the mouse leaves, it welcomes the player again.

#include "GenericScreen.h"
#include "TextTyper.h"

namespace pizzafrenzy
{

class MainMenu : public engine::GenericScreen
{
public:
	MainMenu() : mHovering(false) {}  // 0x43a730

	// 0x17c: 0x43a9e0: the welcome text, and the screen's parts that go with the player's name
	void setup();
	// 0x180: 0x43b3d0: the welcome again, and the locked pizza designer shown for as long as it is locked
	void refresh();

	// The button listener (+0x144, vtable 0x4fdb78): 0x43adb0, the button the mouse is over says what it does
	void buttonEnter(std::string theName) override;
	// 0x43a940: the welcome again, when a button's text showed
	void buttonExit(std::string theName) override;

private:
	// 0x43a800: the welcome to the current player (or to no one), typed out
	void showWelcome();
	// 0x43b000: the node "editPizzaLocked" shows while the player has fewer than five toppings
	void showDesignerLock();

	engine::Ref<engine::Node> mUserProperties;   // +0x1d4: the node "userProperties", which holds the welcome
	engine::Ref<engine::Node> mLorenzoBubble;    // +0x1d8: the node "lorenzoBubble"
	engine::Ref<engine::TextTyper> mWelcomeTyper;  // +0x1dc
	bool mHovering;                              // +0x1e0: a button's text shows
};

} // namespace pizzafrenzy

#endif
