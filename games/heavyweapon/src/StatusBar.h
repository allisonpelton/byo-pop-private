#ifndef __HEAVYWEAPON_STATUSBAR_H__
#define __HEAVYWEAPON_STATUSBAR_H__

// StatusBar: the bar across the top of a game: the score and the tanks in reserve (survival: the time survived), the
// mega laser's parts or its charge, the mission number and the level's progress (survival: its name), and the MENU
// button, which stops the game under the quit menu. After the boss it slides up off the screen.
// Original: a Widget of 0x6c bytes with a ButtonListener at +0x58, constructor 0x42e120, vtable 0x52fa18 (listener
// 0x52f9f8), destructor 0x42df10 (deleting 0x42e0f0).

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class Board;
class BracketButton;
class HeavyWeaponApp;
class QuitDialog;

class StatusBar : public Widget, public ButtonListener
{
public:
	enum
	{
		BUTTON_MENU = 1000
	};

	HeavyWeaponApp*	mApp;				// +0x5c
	Board*			mBoard;				// +0x60 the app's board, taken when the bar is added
	BracketButton*	mMenuButton;		// +0x64
	QuitDialog*		mQuitDialog;		// +0x68 the quit menu the MENU button opened (the app clears it)

public:
	StatusBar(HeavyWeaponApp* theApp);										// 0x42e120
	virtual ~StatusBar();													// 0x42df10 (deleting 0x42e0f0)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;	// 0x42dfa0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x42df70 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x42e010 slot 13
	virtual void	Draw(Graphics* g) override;								// 0x42e220 slot 16
	virtual void	Update() override;										// 0x42dfd0 slot 17
	virtual void	ButtonDepress(int theId) override;						// 0x42e050 (listener slot 5)
};

}

#endif
