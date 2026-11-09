#ifndef __TYPERSHARK_SELECTUSERDIALOG_H__
#define __TYPERSHARK_SELECTUSERDIALOG_H__

// SelectUserDialog.cpp: "WHO ARE YOU?" (dialog 0x44e, made by WinWSApp::ShowSelectUserDialog): the list of the players
// (at most 10), with the line "(Create a New User)" first while there are fewer than ten and "(Delete a User)" last when
// there are exactly ten. A click on a name selects it and remembers it in mSelectedName, which the app's handler of the
// OK button (id 0xc1e) reads to make that player the current one; a click on the new-user line opens the NewUserDialog
// and one on the delete line the DeleteUserDialog.
// Original: class Sexy::SelectUserDialog : public MoneyDialog, public ListListener, vftable 0x52e364 (52 slots),
// ButtonListener vftable 0x52e344 (+0x58), ListListener vftable 0x52e33c (+0x120), sizeof 0x148 (operator new in
// 0x4128a0), constructor 0x43edb0, destructor body 0x43a1a0 (deleting destructor 0x43a270). The base MoneyDialog
// (constructor 0x4382c0, destructor body 0x438310) is a Dialog that keeps the app pointer at +0x110 and adds nothing
// (its Draw 0x438330 and Update 0x438340 forward to Dialog's): it is not a class of the port, so this class derives from
// Dialog2004 and the app pointer is mApp below.

#include "compat2004/Compat2004.h"
#include "ListListener.h"
#include <string>

namespace Sexy
{

class WinWSApp;
class WidgetManager;
class ListWidget;

class SelectUserDialog : public Dialog2004, public ListListener
{
public:
	enum
	{
		MAX_LISTED_USERS = 10
	};

	// +0x120 ListListener vfptr
	WinWSApp*				mApp;					// +0x124 constructor argument (MoneyDialog's copy at +0x110 is the same pointer)
	ListWidget*				mUserList;				// +0x128 id 0, Rockwell 13, centred lines, 5 colours of its own
	std::string				mSelectedName;			// +0x12c (pad), +0x130..: the name last clicked in the list ("" at the start)

public:
	SelectUserDialog(WinWSApp* theApp);														// 0x43edb0
	virtual ~SelectUserDialog() override;													// 0x43a1a0 (deleting 0x43a270), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x438e00 slot 1 (shared with DeleteUserDialog)
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x438e30 slot 2 (shared)
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x43e560 slot 13 (UpdateList, list above the buttons)
	virtual int				GetPreferredHeight(int theWidth) override;						// 0x43e520 slot 50 (UpdateList; + 16 + list height)

	void					UpdateList();													// 0x43e220 (refill from the profile manager, select the current user)

	virtual void			ButtonPress(int theId) override;								// 0x437c20 ButtonListener (+0x58) slot 3 (shared with DeleteUserDialog and AbyssModeDialog)
	virtual void			ButtonDepress(int theId) override;								// 0x43d6b0 ButtonListener (+0x58) slot 4
	virtual void			ListClicked(int theId, int theIdx, int theClickCount) override;	// 0x43fa10 ListListener (+0x120) slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
