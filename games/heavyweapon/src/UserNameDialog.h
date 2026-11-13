#ifndef __HEAVYWEAPON_USERNAMEDIALOG_H__
#define __HEAVYWEAPON_USERNAMEDIALOG_H__

// UserNameDialog: the box that asks for a player's name: for a new player or to rename one. Letters, digits and spaces only,
// up to 20 characters; a name must be there and differ (in capitals) from every other player's. With no player yet the
// box has no Cancel button. Original: a Dialog of 0x130 bytes with a ButtonListener at +0x58 and an EditListener at +0x114,
// constructor 0x428e60, vtable 0x52e680 (button listener 0x52e660, edit listener 0x52e650), destructor body 0x428d20
// (deleting 0x428e30).

#include "Compat2004.h"
#include "ButtonListener.h"
#include "EditListener.h"

namespace Sexy
{

class BracketButton;
class EditWidget;
class HeavyWeaponApp;

class UserNameDialog : public Dialog2004, public EditListener
{
public:
	enum
	{
		DIALOG_ID = 0x1b,
		BUTTON_OK = 0x1c,
		BUTTON_CANCEL = 0x1d,
		EDIT_ID = 0x1e,
		MAX_NAME_LENGTH = 20
	};

	HeavyWeaponApp*	mApp;				// +0x118
	BracketButton*	mOkButton;			// +0x11c id 0x1c
	BracketButton*	mCancelButton;		// +0x120 id 0x1d (not added while there is no player)
	EditWidget*		mEdit;				// +0x124 id 0x1e
	bool			mFirstUser;			// +0x128 there is no player yet
	bool			mRename;			// +0x129 the name of mUserIndex is being changed
	int				mUserIndex;			// +0x12c

public:
	UserNameDialog(HeavyWeaponApp* theApp, bool isRename, int theUserIndex);			// 0x428e60
	virtual ~UserNameDialog();															// 0x428e30 (body 0x428d20)

	virtual void	AddedToManager(WidgetManager* theManager) override;				// 0x428bf0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;			// 0x428db0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x428c50 slot 13
	virtual void	ButtonPress(int theId) override;									// 0x42ca70 listener slot 4
	virtual void	ButtonDepress(int theId) override;									// 0x4298f0 listener slot 5
	virtual void	EditWidgetText(int theId, const SexyString& theString) override;	// 0x429940 edit listener slot 0
	virtual bool	AllowChar(int theId, SexyChar theChar) override;					// 0x428e00 edit listener slot 2

	using Dialog::Resize;
	using Dialog2004::ButtonPress;

private:
	void			Accept();															// 0x429300
};

}

#endif
