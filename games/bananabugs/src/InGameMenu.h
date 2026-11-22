// InGameMenu: the menu a level (or the map's base) opens from its menu button (0x435c00..0x436460 in the original;
// 0x36c bytes there). A BBWindow of seven by five tiles with three buttons down its middle: resume, options and quit.
// The options button opens the options window on top; the quit button asks "are you sure" first. The owner asks
// GetResult each frame and, on 2 (resume) or 3 (quit), closes the window.
#pragma once

#include "BBWindow.h"

namespace Sexy
{

class AskWin;
class Options;

const int IN_GAME_MENU_BUTTONS = 3;

// What the menu gives back (InGameMenu::GetResult, from mChoice)
enum InGameMenuResult
{
	IN_GAME_MENU_OPEN = 0,       // no answer yet
	IN_GAME_MENU_OPTIONS = 1,    // the options window is open (the owners do not act on it)
	IN_GAME_MENU_RESUME = 2,
	IN_GAME_MENU_QUIT = 3,
};

class InGameMenu : public BBWindow
{
public:
	int mChoice;                 // +0x350: InGameMenuResult
	AskWin* mAskWin;             // +0x354: the "are you sure" window of the quit button, while it is open
	Options* mOptions;           // +0x358: the options window, while it is open
	int mDelay;                  // +0x35c: frames the buttons are deaf for after the menu opens or a window closes
	ObjBase* mMenuButtons[IN_GAME_MENU_BUTTONS]; // +0x360: resume, options, quit (BBWindow's own two stay empty)

	InGameMenu();                // 0x435c00
	~InGameMenu();               // 0x435cb0 (body 0x435cd0)
	bool Update() override;      // 0x435d60
	bool MouseUp(ObjBase* theObj) override;     // 0x435ed0

	int GetResult();             // 0x436010: 0 for a delay still running, else mChoice

private:
	void BuildButtons();         // 0x436030
};

} // namespace Sexy
