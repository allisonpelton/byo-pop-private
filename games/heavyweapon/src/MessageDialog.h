#ifndef __HEAVYWEAPON_MESSAGEDIALOG_H__
#define __HEAVYWEAPON_MESSAGEDIALOG_H__

// MessageDialog: a box with a heading, some text and one OK button that closes it (the armory's "not yet" box). Original:
// a Dialog of 0x11c bytes with a ButtonListener at +0x58, constructor 0x429b00, vtable 0x52e828 (listener 0x52e808),
// destructor body 0x4299f0.

#include "Compat2004.h"
#include "ButtonListener.h"

namespace Sexy
{

class BracketButton;
class HeavyWeaponApp;

class MessageDialog : public Dialog2004
{
public:
	enum
	{
		DIALOG_ID = 0x1f,		// the id it is added to the app under and kills itself by
		BUTTON_OK = 0
	};

	HeavyWeaponApp*	mApp;			// +0x114
	BracketButton*	mOkButton;		// +0x118

public:
	MessageDialog(HeavyWeaponApp* theApp, int theWidth, int theHeight, const std::string& theHeader,
		const std::string& theLines);												// 0x429b00
	virtual ~MessageDialog();														// 0x429ad0 (body 0x4299f0)

	virtual void	AddedToManager(WidgetManager* theManager) override;			// 0x429950 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;		// 0x429a50 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x429980 slot 13
	virtual void	ButtonPress(int theId) override;								// 0x429a80 listener slot 4
	virtual void	ButtonDepress(int theId) override;								// 0x429aa0 listener slot 5

	using Dialog::Resize;
};

}

#endif
