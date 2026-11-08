#ifndef __TYPERSHARK_MENUSCREEN_H__
#define __TYPERSHARK_MENUSCREEN_H__

// MenuScreen.cpp: the main menu: the picture of the game selector with six image buttons on it (Adventure, Abyss, Typing
// Tutor, Hall of Fame, Options and Quit), the greeting "Welcome to Typer Shark" with the name of the current player under
// it, and two text links ("(Click here if this is not you)", which opens the player list, and "Click here to view your
// statistics"). The app makes it in ShowMenuScreen (WinWSAppScreens.cpp: new MenuScreen(this), Resize to the window, enabled,
// with alpha, AddWidget, SetVisible, MarkDirtyFull) and keeps it at WinWSApp::mMenuScreen; every button and link
// reports to the app (the listener), whose ButtonDepress handles the ids below.
// The buttons are FadeButtons (their over picture fades in) when the app's mFastCPU is set, plain ButtonWidgets
// otherwise. Each button's normal picture is a copy of the part of the selector picture it covers, made when the screen
// joins the manager and freed with it.
// Original: class Sexy::MenuScreen : public Widget, vftable 0x52f04c, constructor 0x4436e0, destructor body 0x4410f0
// (deleting 0x441e70, slot 0), sizeof 0x80 (the last field is at +0x78).
//
// The menu has no register, "More Games" or other button that leads out of the game: all eight ids are the game's own.

#include "Widget.h"

namespace Sexy
{

class ButtonWidget;
class HyperlinkWidget;
class WinWSApp;

class MenuScreen : public Widget
{
public:
	enum
	{
		ADVENTURE_BUTTON_ID = 0x64,		// the saved adventure game, or the difficulty dialog
		ABYSS_BUTTON_ID = 0x65,			// the abyss mode (the "CONTINUE?" dialog when a depth is reached)
		TUTOR_BUTTON_ID = 0x66,			// the typing tutor
		HIGH_SCORES_BUTTON_ID = 0x67,	// the high score screen
		QUIT_BUTTON_ID = 0x68,			// asks "LEAVING?"
		NOT_YOU_LINK_ID = 0x69,			// the player list
		STATS_LINK_ID = 0x6a,			// the statistics screen
		OPTIONS_BUTTON_ID = 0x6b		// the options dialog
	};

	// the places of the six buttons in mButtons (the original keeps them in six fields in this order and loops over them)
	enum
	{
		BUTTON_ADVENTURE,
		BUTTON_ABYSS,
		BUTTON_TUTOR,
		BUTTON_QUIT,
		BUTTON_HIGH_SCORES,
		BUTTON_OPTIONS,
		NUM_BUTTONS
	};

public:
	WinWSApp*				mApp;						// +0x58 constructor argument
	ButtonWidget*			mButtons[NUM_BUTTONS];		// +0x5c adventure (id 0x64), +0x60 abyss (0x65), +0x64 tutor (0x66), +0x68 quit (0x68), +0x6c high scores (0x67), +0x70 options (0x6b); owned
	HyperlinkWidget*		mNotYouLink;				// +0x74 (id 0x69) owned
	HyperlinkWidget*		mStatsLink;					// +0x78 (id 0x6a) owned

public:
	MenuScreen(WinWSApp* theApp);														// 0x4436e0
	virtual ~MenuScreen() override;														// 0x4410f0 (deleting 0x441e70), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4482d0 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4411d0 slot 2
	virtual void			SetVisible(bool isVisible) override;						// 0x441190 slot 4
	virtual void			Draw(Graphics* g) override;									// 0x448860 slot 16
};

}

#endif
