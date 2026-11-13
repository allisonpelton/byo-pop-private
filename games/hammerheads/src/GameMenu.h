#ifndef __HAMMERHEADS_GAMEMENU_H__
#define __HAMMERHEADS_GAMEMENU_H__

// The menu of a game that is paused (the menu button of the game's screen): back to the game, the options, and the way out.
// Original: class Sexy::GameMenu : public NcMovingDialog (0x178 bytes; the constructor is 0x44f8d0, the destructor 0x44f690).

#include "NcMovingDialog.h"

namespace Sexy
{

class NcButton;

class GameMenu : public NcMovingDialog
{
public:
	NcButton*		mContinueButton;	// +0x168: id 0, "Continue Game" (or "Back To Shop" while the shop is open)
	NcButton*		mOptionsButton;		// +0x16c: id 1
	NcButton*		mMainMenuButton;	// +0x170: id 2

public:
	GameMenu();																			// 0x44f8d0
	virtual ~GameMenu();																// 0x44f690

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x44f720
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;			// 0x44f7c0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;		// 0x44f800
	virtual int		GetPreferredHeight(int theWidth) override;							// 0x44f8a0
	virtual void	ButtonDepress(int theId) override;									// 0x44f840
	virtual void	Closed() override;													// 0x44f880
};

// 0x44fba0: the menu is made, sized and added as dialog 0
GameMenu* MakeGameMenu();

}

#endif
