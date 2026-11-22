#ifndef __FEEDINGFRENZY2_MAINMENU_H__
#define __FEEDINGFRENZY2_MAINMENU_H__

// The main menu (MainMenu, 0x1a8 bytes, vtable 0x559e0c (feedingfrenzy2), GenericScreen's slots unchanged; its
// constructor 0x49fd00 (feedingfrenzy2)): a screen of resource/shell/mainMenu.xml, made by the splash's loader, its
// actions passed to the application.

#include "GenericScreen.h"

class MainMenu : public engine::GenericScreen
{
public:
	// 0x48d4a0 (feedingfrenzy2): the layout read with the shell's theme, quarter-second fades in and out; the debug
	// level select's button hidden and switched off, and so the link button
	void init();
};

#endif
