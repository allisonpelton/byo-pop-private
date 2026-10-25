#ifndef __ATOMICA_MENU_H__
#define __ATOMICA_MENU_H__

// Menu.cpp: the main menu of Atomica Deluxe (Atomica.exe 0x4198e0, 0x4191d0 and 0x41a010): two sliding doors, and
// behind them a panel with the two kinds of game (strategy, action), the saved game and the exit.
// port: a simplified panel. The original's draws the best scores and the rank of the player (0x41a010) and has the game
// over screen with the high score entry (0x419760); those are not ported yet, and the buttons beside the games switch
// the expert mode on and off.

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Support.h"

namespace Sexy
{

class AtomicaApp;

class MainMenu : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;				// +0x74
	Sprite*			mDoorLeft;			// +0x3020 (of the app in the original)
	Sprite*			mDoorRight;			// +0x3024
	Sprite*			mPanel;				// +0x3028
	int				mDoor;				// +0x78 how far the doors are open (0 to 0x14a)
	int				mPanelY;			// +0x7c
	int				mDoorTick;			// +0x84
	bool			mHasSave;			// +0xb0 a saved game is there
	bool			mGameOver;			// +0xb8 shown after a game
	bool			mOpening;
	bool			mPanelShown;
	int				mExpert[2];			// the expert mode of each game
	float			mSpin;				// +0xe0
	PtrList<Widget>	mButtons;			// +0x5c

	MainMenu(AtomicaApp* theApp, bool theGameOver);	// 0x4198e0
	virtual ~MainMenu();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x431d20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4194e0
	virtual void	Update() override;					// 0x4191d0
	virtual void	Draw(Graphics* g) override;			// 0x41a010
	virtual void	ButtonDepress(int theId) override;	// 0x4195a0
};

}

#endif
