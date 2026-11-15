#ifndef __HEAVYWEAPON_PLAYTIPDIALOG_H__
#define __HEAVYWEAPON_PLAYTIPDIALOG_H__

// PlayTipDialog: the box that gives a hint in the first levels, with an OK button and a "Disable all hints" checkbox
// (it sets bit 1 of the player's flags at +0x42 of the record, and each click flips it). Original: a Dialog of 0x128
// bytes with a ButtonListener at +0x58 and a CheckboxListener at +0x114, constructor 0x423a30, vtable 0x52db20
// (button listener 0x52db04, checkbox listener 0x52db00), destructor body 0x4237f0 (deleting 0x4239b0).

#include "Compat2004.h"
#include "ButtonListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class BracketButton;
class Board;
class Checkbox;
class HeavyWeaponApp;

class PlayTipDialog : public Dialog2004, public CheckboxListener
{
public:
	enum
	{
		DIALOG_ID = 0x2c,			// the id it is added to the app under and kills itself by
		CONSTRUCTED_ID = 7			// the id its constructor gives the Dialog (AddDialog replaces it)
	};

	Board*			mBoard;			// +0x118 the board, to give the focus back to
	HeavyWeaponApp*	mApp;			// +0x11c
	Checkbox*		mCheckbox;		// +0x120 "Disable all hints"
	BracketButton*	mOkButton;		// +0x124

public:
	PlayTipDialog(HeavyWeaponApp* theApp, const std::string& theTip);					// 0x423a30
	virtual ~PlayTipDialog();															// 0x4239b0 (body 0x4237f0)

	virtual void	AddedToManager(WidgetManager* theManager) override;				// 0x4238b0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;			// 0x423870 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4238f0 slot 13
	virtual void	Draw(Graphics* g) override;											// 0x423ce0 slot 16
	virtual void	ButtonPress(int theId) override;									// 0x423960 listener slot 4
	virtual void	ButtonDepress(int theId) override;									// 0x423980 listener slot 5
	virtual void	CheckboxChecked(int theId, bool checked) override;					// 0x4239e0 checkbox listener slot 0

	using Dialog::Resize;
	using Dialog2004::ButtonPress;
};

}

#endif
