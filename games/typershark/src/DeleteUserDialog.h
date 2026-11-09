#ifndef __TYPERSHARK_DELETEUSERDIALOG_H__
#define __TYPERSHARK_DELETEUSERDIALOG_H__

// DeleteUserDialog.cpp: "DELETE WHO?" (dialog 0x44f, made by WinWSApp::ShowDeleteUserDialog, which the SelectUserDialog's
// delete line calls with the ten names): the names with a warning above them. OK or a double click on a name opens the
// app's YES / NO confirmation (dialog 500, made by ConfirmDelete, which also sets the app's flag at +0x910); its YES
// makes the app's handler call DeleteSelectedUser and kill this dialog.
// Original: class Sexy::DeleteUserDialog : public MoneyDialog, public ListListener, vftable 0x52e074 (52 slots),
// ButtonListener vftable 0x52e058 (+0x58), ListListener vftable 0x52e050 (+0x120), sizeof 0x130 (operator new in
// 0x4267d0), constructor 0x43dcb0, destructor body 0x438f20 (deleting destructor 0x4391e0). The base MoneyDialog
// (constructor 0x4382c0, destructor body 0x438310) is a Dialog that keeps the app pointer at +0x110 and adds nothing
// (its Draw 0x438330 and Update 0x438340 forward to Dialog's): it is not a class of the port, so this class derives from
// Dialog2004 and the app pointer is mApp below.

#include "compat2004/Compat2004.h"
#include "ListListener.h"
#include <string>
#include <vector>

namespace Sexy
{

class WinWSApp;
class WidgetManager;
class ListWidget;

class DeleteUserDialog : public Dialog2004, public ListListener
{
public:
	// +0x120 ListListener vfptr
	WinWSApp*				mApp;					// +0x124 constructor argument (MoneyDialog's copy at +0x110 is the same pointer)
	ListWidget*				mUserList;				// +0x128 id 0, Rockwell 13, centred lines, 5 colours of its own; nothing selected at the start
	// +0x12c pad to 0x130

public:
	DeleteUserDialog(WinWSApp* theApp, std::vector<std::string> theNames);					// 0x43dcb0 (the vector is passed by value)
	virtual ~DeleteUserDialog() override;													// 0x438f20 (deleting 0x4391e0), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x438e00 slot 1 (SelectUserDialog's function)
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x438e30 slot 2 (SelectUserDialog's function)
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x437cb0 slot 13
	virtual int				GetPreferredHeight(int theWidth) override;						// 0x437c60 slot 50 (+ 16 + list height)

	void					ConfirmDelete(std::string theName);								// 0x43a9b0 (the app's confirmation, dialog 500)
	void					DeleteSelectedUser();											// 0x43df60 (called by the app when the confirmation answers YES)

	virtual void			ButtonPress(int theId) override;								// 0x437c20 ButtonListener (+0x58) slot 3 (SelectUserDialog's function)
	virtual void			ButtonDepress(int theId) override;								// 0x43eb20 ButtonListener (+0x58) slot 4
	virtual void			ListClicked(int theId, int theIdx, int theClickCount) override;	// 0x43ab70 ListListener (+0x120) slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
