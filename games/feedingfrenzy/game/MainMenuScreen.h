#ifndef __FEEDINGFRENZY_MAINMENUSCREEN_H__
#define __FEEDINGFRENZY_MAINMENUSCREEN_H__

// The main menu (MainMenuScreen, constructor 0x423fa0, vtable 0x4a53e8; the file config\mainMenu.xml): the buttons that
// start a game, show the high scores, the options and the credits, and quit. Under the two buttons that start a game
// a line of help shows while the cursor is on the button.

#include "ScreenLayout.h"

namespace feedingfrenzy
{

class MainMenuScreen : public engine::ScreenLayout
{
public:
	MainMenuScreen() {}  // 0x423fa0

	// 0xfc: 0x423f20: the lines of help are hidden
	virtual void hideHelpers();

	// The button listener (0x4a53dc, at +0x138): the help of the button the cursor is on fades in, and out when it leaves
	void buttonEnter(std::string theName) override;  // 0x423d00
	void buttonExit(std::string theName) override;   // 0x423e50
	// 0x423de0: a press goes on to the screen's own listener
	void buttonPress(std::string theName) override { ScreenLayout::buttonPress(theName); }

private:
	// The help line of a button that starts a game (null for another button)
	engine::FadeContainer* getHelper(const std::string& theButton);
};

} // namespace feedingfrenzy

#endif
