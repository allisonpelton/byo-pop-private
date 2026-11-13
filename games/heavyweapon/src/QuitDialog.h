#ifndef __HEAVYWEAPON_QUITDIALOG_H__
#define __HEAVYWEAPON_QUITDIALOG_H__

// QuitDialog: the menu over a paused game (HeavyWeaponApp::DIALOG_QUIT): RESUME GAME and OPTIONS report to the app
// (HeavyWeaponApp::BUTTON_QUIT_CANCEL and BUTTON_OPTIONS), MAIN MENU asks "retreat to main menu?" in a YesNoDialog whose
// answer comes back here and ends the game. Opening it first tells the board to stop. Original: a Dialog of 0x128 bytes,
// constructor 0x41f500, vtable 0x52d090 (listener 0x52d070), destructor body 0x41f410 (deleting 0x41f4d0).

#include "Compat2004.h"
#include "ButtonListener.h"

namespace Sexy
{

class BracketButton;
class HeavyWeaponApp;

class QuitDialog : public Dialog2004
{
public:
	enum
	{
		BUTTON_MAIN_MENU = 5,	// the id of its own MAIN MENU button
		BUTTON_YES = 0x21,		// the answers of the confirming YesNoDialog
		BUTTON_NO = 0x22,
		CONFIRM_DIALOG_ID = 0x20
	};

	HeavyWeaponApp*	mApp;				// +0x118
	BracketButton*	mResumeButton;		// +0x11c id 4
	BracketButton*	mOptionsButton;		// +0x120 id 6
	BracketButton*	mMainMenuButton;	// +0x124 id 5

public:
	QuitDialog(HeavyWeaponApp* theApp);														// 0x41f500
	virtual ~QuitDialog();																	// 0x41f4d0 (body 0x41f410)

	virtual void	AddedToManager(WidgetManager* theManager) override;						// 0x41f310 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;					// 0x41f490 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;			// 0x41f350 slot 13
	virtual void	ButtonPress(int theId) override;											// 0x42ca70 listener slot 4
	virtual void	ButtonDepress(int theId) override;											// 0x41f8b0 listener slot 5

	using Dialog::Resize;
	using Dialog2004::ButtonPress;
};

}

#endif
