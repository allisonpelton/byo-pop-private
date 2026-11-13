#ifndef __HEAVYWEAPON_USERLISTDIALOG_H__
#define __HEAVYWEAPON_USERLISTDIALOG_H__

// UserListDialog: "WHO ARE YOU?": the list of players with "[Create a New User]" first, and OK, CANCEL, RENAME and DELETE
// buttons. A double click on a name chooses it; DELETE asks "delete this user's profile?" first and, when the last player
// goes, the new-player box opens. Original: a Dialog of 0x13c bytes with a ButtonListener at +0x58 and a ListListener at
// +0x114, constructor 0x430760, vtable 0x530040 (button listener 0x530020, list listener 0x53001c), destructor body
// 0x430380 (deleting 0x4305b0).

#include "Compat2004.h"
#include "ButtonListener.h"
#include "ListListener.h"

namespace Sexy
{

class BracketButton;
class HeavyWeaponApp;
class ListWidget;
class ScrollbarWidget;

class UserListDialog : public Dialog2004, public ListListener
{
public:
	enum
	{
		DIALOG_ID = 0x15,
		BUTTON_OK = 0x16,
		BUTTON_CANCEL = 0x17,
		BUTTON_RENAME = 0x18,
		BUTTON_DELETE = 0x19,
		LIST_ID = 0x1a,
		BUTTON_DELETE_YES = 0x21,		// the answers of the box that asks before a delete
		BUTTON_DELETE_NO = 0x22
	};

	HeavyWeaponApp*		mApp;			// +0x11c
	BracketButton*		mOkButton;		// +0x120 id 0x16
	BracketButton*		mCancelButton;	// +0x124 id 0x17
	BracketButton*		mRenameButton;	// +0x128 id 0x18
	BracketButton*		mDeleteButton;	// +0x12c id 0x19
	ListWidget*			mList;			// +0x130 id 0x1a: line 0 is "[Create a New User]", then each player
	ScrollbarWidget*	mScrollbar;		// +0x134
	int					mSelectedUser;	// +0x138 the player the list has selected (for RENAME and DELETE)

public:
	UserListDialog(HeavyWeaponApp* theApp);												// 0x430760
	virtual ~UserListDialog();															// 0x4305b0 (body 0x430380)

	virtual void	AddedToManager(WidgetManager* theManager) override;				// 0x4311d0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;			// 0x430310 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x430440 slot 13
	virtual void	ButtonPress(int theId) override;									// 0x430590 listener slot 4
	virtual void	ButtonDepress(int theId) override;									// 0x430e40 listener slot 5
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override;	// 0x4305e0 list listener slot 0

	void			Refresh();															// 0x430c80

	using Dialog::Resize;
	using Dialog2004::ButtonPress;
};

}

#endif
