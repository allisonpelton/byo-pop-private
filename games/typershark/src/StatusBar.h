#ifndef __TYPERSHARK_STATUSBAR_H__
#define __TYPERSHARK_STATUSBAR_H__

// StatusBar.cpp: the strip along the bottom of the board: the shark zapper's charge meter with "CHARGING" or a hint
// ("READY!", and after a while "PRESS ENTER TO FIRE!" while the zapper is charged and unused), and three image buttons
// (pause, options, quit). The app makes it with the board (WinWSAppGame.cpp: new StatusBar(this), Resize to the bottom
// of the window, AddWidget) and keeps it at WinWSApp::mStatusBar. Its height is that of mImages[IMAGE_STATUSBARX].
// AddedToManager makes the three buttons (listener: the app) and a tooltip under the pointer.
// Original: class Sexy::StatusBar : public Widget, vftable 0x52f3c4, sizeof 0x88 (constructor 0x448a40, destructor body
// 0x442920, deleting destructor 0x4439e0).
//
// The Board calls HideButtons, ShowButton(bool) and the Widget virtuals MouseLeave, SetVisible and SetDisabled.
// Not translated: the std::string members' code (0x443c10 copies the tooltip's text for MouseEnter and MouseLeave).

#include "Widget.h"

namespace Sexy
{

class FadeButton;
class TooltipWidget;
class WinWSApp;

class StatusBar : public Widget
{
public:
	enum
	{
		PAUSE_BUTTON_ID = 0xc8,			// WinWSApp::ButtonDepress pauses or resumes the board
		QUIT_BUTTON_ID = 0xc9,			// ends the game (asks first)
		OPTIONS_BUTTON_ID = 0xcb		// shows the options dialog
	};

public:
	WinWSApp*				mApp;				// +0x58 constructor argument
	FadeButton*				mQuitButton;		// +0x5c made by AddedToManager (NULL before)
	FadeButton*				mPauseButton;		// +0x60 made by AddedToManager (NULL before)
	FadeButton*				mOptionsButton;		// +0x64 made by AddedToManager (NULL before; the original leaves it unset)
	TooltipWidget*			mTooltip;			// +0x68 owned; the hint for the zapper, placed at (15, 415) and sized to its text
	SexyString				mHint;				// +0x6c "READY!" (ctor); Update flips it to "PRESS ENTER TO FIRE!" and back

public:
	StatusBar(WinWSApp* theApp);											// 0x448a40
	virtual ~StatusBar() override;											// 0x442920 (deleting 0x4439e0), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x450720 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4413e0 slot 2
	virtual void			OrderInManagerChanged() override;				// 0x441390 slot 3
	virtual void			SetVisible(bool isVisible) override;			// 0x441430 slot 4
	virtual void			SetDisabled(bool isDisabled) override;			// 0x441470 slot 10
	virtual void			Draw(Graphics* g) override;						// 0x448c00 slot 16
	virtual void			Update() override;								// 0x448b00 slot 17
	virtual void			MouseEnter() override;							// 0x443eb0 slot 23
	virtual void			MouseLeave() override;							// 0x443f80 slot 24

	void					HideButtons();									// 0x441f10 no tooltip shows; the hint tooltip goes to the back of the manager
	void					ShowButton(bool on);							// 0x441f80 shows (or hides) the hint tooltip, in front of everything when shown
};

}

#endif
