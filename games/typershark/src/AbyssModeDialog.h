#ifndef __TYPERSHARK_ABYSSMODEDIALOG_H__
#define __TYPERSHARK_ABYSSMODEDIALOG_H__

// AbyssModeDialog.cpp: "CONTINUE?" of the abyss mode (dialog 0x451, made by WinWSApp::ShowAbyssModeDialog): a list with
// "(Start from beginning)" and one line "400 feet", "800 feet" ... for each full 400 feet of the user's best depth, with
// a scroll bar. A click selects a line (and plays the button sample), a double click on a depth line takes it; the yes
// button (ButtonDepress) takes the selected line. mSelectedDepth is the depth in feet that WinWSApp::ButtonDepress
// (id 0xc21) starts the game at (0 = from the beginning).
// Original: class Sexy::AbyssModeDialog : public MoneyDialog, public ListListener, vftable 0x52df7c (52 slots, slot 50 is
// GetPreferredHeight), ButtonListener vftable 0x52df60 (+0x58), ListListener vftable 0x52df58 (+0x120), sizeof 0x134,
// constructor 0x43e7a0, destructor body 0x438ea0 (deleting destructor 0x4391b0). The base MoneyDialog (constructor
// 0x4382c0, destructor body 0x438310) is a Dialog that keeps the app pointer at +0x110 and adds nothing (its Draw 0x438330
// and Update 0x438340 forward to Dialog's): it is not a class of the port, so this class derives from Dialog2004 and the
// app pointer is mApp below.

#include "compat2004/Compat2004.h"
#include "ListListener.h"

namespace Sexy
{

class WinWSApp;
class UserProfile;
class ListWidget;
class ScrollbarWidget;
class Graphics;
class WidgetManager;

class AbyssModeDialog : public Dialog2004, public ListListener
{
public:
	WinWSApp*				mApp;					// +0x124 (MoneyDialog's copy at +0x110 is the same pointer)
	ListWidget*				mList;					// +0x128 id 0, the Rockwell 13 font, centred lines, 5 colours of its own
	ScrollbarWidget*		mScrollbar;				// +0x12c id 0x2593, its listener is the list
	int						mSelectedDepth;			// +0x130 constructor 0; the depth of the selected line, set by a double click and by the yes button

public:
	AbyssModeDialog(WinWSApp* theApp, UserProfile* theUser);						// 0x43e7a0
	virtual ~AbyssModeDialog() override;											// 0x438ea0 (deleting 0x4391b0), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x437a30 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x437a80 slot 2
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x437ac0 slot 13
	virtual int				GetPreferredHeight(int theWidth) override;				// 0x4379f0 slot 50

	virtual void			ButtonPress(int theId) override;						// 0x437c20 ButtonListener (+0x58) slot 3 (SelectUserDialog and DeleteUserDialog share it)
	virtual void			ButtonDepress(int theId) override;						// 0x43a890 ButtonListener (+0x58) slot 4
	virtual void			ListClicked(int theId, int theIdx, int theClickCount) override;	// 0x43a750 ListListener (+0x120) slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;

private:
	void					ResetList();											// 0x43dbd0 (only the constructor calls it)
};

}

#endif
