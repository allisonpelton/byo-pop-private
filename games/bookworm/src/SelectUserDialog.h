#ifndef __BOOKWORM_SELECTUSERDIALOG_H__
#define __BOOKWORM_SELECTUSERDIALOG_H__

// SelectUserDialog.cpp: dialog 19 (DIALOG_SELECT_USER) with the player list: up to five names, plus the
// new-user line (GT_USER_LIST_CREATE) first while there are fewer than five and the delete line
// (GT_USER_LIST_DELETE) last when there are exactly five. Disables the game selector while it exists.
// Original: class Sexy::SelectUserDialog : public MoneyDialog, public ListListener, sizeof 0x130 (new in
// BookWormApp::DoSelectUserDialog 0x410520). Vtables: 0x501004 (52 slots), ButtonListener 0x500fe8
// (+0x58), ListListener 0x500fe0 (+0x120). Offsets are absolute. Behaviour spec:
// re/bookworm/analysis/dialogs.md section 5.

#include "MoneyDialog.h"
#include "ListListener.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class ListWidget;

class SelectUserDialog : public MoneyDialog, public ListListener
{
public:
	enum
	{
		USER_LIST_ID = 0,
		MAX_LISTED_USERS = 5
	};

	// +0x120 ListListener vfptr
	BookWormApp*	mApp2;                  // +0x124 app
	ListWidget*		mUserList;              // +0x128 id 0, Verdana10Bold, colours 0x52f788, centred, outline
	// +0x12c pad to 0x130

public:
	SelectUserDialog(BookWormApp* theApp);                              // 0x432c90 (disables the game selector)
	virtual ~SelectUserDialog();                                        // 0x4325f0 body (re-enables it), 0x4326f0 slot 0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;    // 0x432680 slot 1 (shared with DeleteUserDialog)
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;// 0x4326b0 slot 2 (shared)
	virtual void	SetColor(int theIdx, const Color& theColor) override;        // 0x432670 slot 7 (= Dialog::SetColor; shared)
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x432b40 slot 13 (ResetUserListWidget, list above the buttons)
	virtual void	Draw(Graphics* g) override;                         // 0x4326e0 slot 16 (= MoneyDialog::Draw; shared)
	virtual int		GetPreferredHeight(int theWidth) override;          // 0x432b00 slot 50 (ResetUserListWidget; + 16 + list height)

	void			ResetUserListWidget();                                       // 0x432810 (refill from the UserManager, select the current user)

	virtual void	ButtonPress(int theId) override;                    // 0x41df60 +0x58 slot 3 (shared with DeleteUserDialog)
	virtual void	ButtonDepress(int theId) override;                  // 0x432720 +0x58 slot 4 (OK: switch user, LoadUserConfig)
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override; // 0x432f00 +0x120 slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
