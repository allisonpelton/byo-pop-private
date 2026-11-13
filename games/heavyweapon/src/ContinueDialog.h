#ifndef __HEAVYWEAPON_CONTINUEDIALOG_H__
#define __HEAVYWEAPON_CONTINUEDIALOG_H__

// ContinueDialog: the box the main menu shows when the player starts a campaign that has progress saved: continue at the
// saved mission, begin a new campaign, or cancel. Its buttons report to the main menu (ids 0x3c, 0x3d, 0x3e). Original: a
// Dialog of 0x11c bytes (an extra interface at +0x114 that nothing uses), constructor 0x41d800, vtable 0x52cbe0 (listener
// 0x52cbc0), destructor body 0x41d5f0 (deleting 0x41d7d0).

#include "Compat2004.h"
#include "ButtonListener.h"

namespace Sexy
{

class BracketButton;
class HeavyWeaponApp;

class ContinueDialog : public Dialog2004
{
public:
	enum
	{
		DIALOG_ID = 0x3b,
		BUTTON_CONTINUE = 0x3c,
		BUTTON_NEW_GAME = 0x3d,
		BUTTON_CANCEL = 0x3e
	};

	HeavyWeaponApp*	mApp;				// +0x118
	BracketButton*	mContinueButton;	// +0x11c
	BracketButton*	mNewGameButton;		// +0x120
	BracketButton*	mCancelButton;		// +0x124

public:
	ContinueDialog(HeavyWeaponApp* theApp);														// 0x41d800
	virtual ~ContinueDialog();																	// 0x41d7d0 (body 0x41d5f0)

	virtual void	AddedToManager(WidgetManager* theManager) override;						// 0x41d680 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;					// 0x41d6c0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;			// 0x41d700 slot 13

	using Dialog::Resize;
};

}

#endif
