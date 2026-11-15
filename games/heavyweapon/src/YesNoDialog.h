#ifndef __HEAVYWEAPON_YESNODIALOG_H__
#define __HEAVYWEAPON_YESNODIALOG_H__

// YesNoDialog: a box with a heading, some text, and a green YES and a red NO button. Its buttons report to the listener it
// is given (not to the box), with ids it is given (0x21 and 0x22 when it is given none); whoever made it closes it by its
// id. Original: a Dialog of 0x124 bytes (an extra interface at +0x114 that nothing uses), constructor 0x41d320, vtable
// 0x52cad0 (listener 0x52cab0), destructor body 0x41d230 (deleting 0x41d2f0).

#include "Compat2004.h"
#include "ButtonListener.h"

namespace Sexy
{

class BracketButton;
class HeavyWeaponApp;

class YesNoDialog : public Dialog2004
{
public:
	enum
	{
		DIALOG_ID = 0x20,		// the id it is added to the app under
		DEFAULT_YES_ID = 0x21,
		DEFAULT_NO_ID = 0x22
	};

	HeavyWeaponApp*	mApp;			// +0x118
	BracketButton*	mYesButton2;	// +0x11c
	BracketButton*	mNoButton2;		// +0x120

public:
	YesNoDialog(HeavyWeaponApp* theApp, ButtonListener* theListener, const std::string& theHeader,
		const std::string& theLines, int theWidth, int theHeight, int theYesId, int theNoId);	// 0x41d320
	virtual ~YesNoDialog();																		// 0x41d2f0 (body 0x41d230)

	virtual void	AddedToManager(WidgetManager* theManager) override;						// 0x41d160 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;					// 0x41d2b0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;			// 0x41d1a0 slot 13

	using Dialog::Resize;
};

}

#endif
