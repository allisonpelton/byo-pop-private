#ifndef __MAINMENU_H__
#define __MAINMENU_H__

// MainMenu.h: the game selector (Sexy::MainMenu, 0x5c80e1-0x5c9dcd). TODO: a stub until it is translated.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinDMApp;
class Image;

class MainMenu : public Widget, public ButtonListener
{
public:
	WinDMApp*		mApp;               // +0x8c
	int				mState;
	bool			mBackdropClear;
	int				mTimer;
	Image*			mBackdropImage;
	int				mWelcomeTimer;      // +0x194

public:
	MainMenu(WinDMApp* theApp) { mApp = theApp; mState = 0; mBackdropClear = false; mTimer = 0; mBackdropImage = NULL; mWelcomeTimer = 0; }
	virtual void	ButtonDepress(int theId) override {}

	void			Show() {}
	void			Hide() {}
	void			StartShutdownTimer() {}
	void			CancelNewGame() {}
	void			SyncProfile() {}
};

}

#endif
