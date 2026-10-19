#ifndef __BOOKWORM_DELETEUSERDIALOG_H__
#define __BOOKWORM_DELETEUSERDIALOG_H__

// DeleteUserDialog.cpp: dialog 20 (DIALOG_DELETE_USER) listing the five names (it is only offered with
// exactly five users). OK or a double-click opens the app's confirmation (dialog 21, YES/NO), whose YES
// (2021) makes the app call DeleteSelectedUser.
// Original: class Sexy::DeleteUserDialog : public MoneyDialog, public ListListener, sizeof 0x130 (new in
// BookWormApp::ShowDeleteUserDialog 0x418590). Vtables: 0x4fc2b4 (52 slots), ButtonListener 0x4fc298
// (+0x58), ListListener 0x4fc290 (+0x120). Offsets are absolute. Behaviour spec:
// re/bookworm/analysis/dialogs.md section 6.

#include "MoneyDialog.h"
#include "ListListener.h"
#include <vector>
#include <string>

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class ListWidget;

class DeleteUserDialog : public MoneyDialog, public ListListener
{
public:
	enum
	{
		USER_LIST_ID = 0
	};

	// +0x120 ListListener vfptr
	BookWormApp*	mApp2;                  // +0x124 app
	ListWidget*		mUserList;              // +0x128 id 0, Verdana10Bold, colours 0x52ea00, centred, outline
	// +0x12c pad to 0x130

public:
	DeleteUserDialog(BookWormApp* theApp, std::vector<std::string> theNames); // 0x41e370 (vector by value)
	virtual ~DeleteUserDialog();                                        // 0x41ddb0 body, 0x41dfb0 slot 0

	// slots 1, 2, 7, 16 and +0x58 slot 3 are the SelectUserDialog functions (identical-COMDAT folded)
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;    // 0x432680 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;// 0x4326b0 slot 2
	virtual void	SetColor(int theIdx, const Color& theColor) override;        // 0x432670 slot 7 (= Dialog::SetColor)
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x41de10 slot 13
	virtual void	Draw(Graphics* g) override;                         // 0x4326e0 slot 16 (= MoneyDialog::Draw)
	virtual int		GetPreferredHeight(int theWidth) override;          // 0x41ddd0 slot 50 (+ 16 + list height)

	void			ConfirmDelete(const std::string& theName);          // 0x41e020 (app DoDialog 21, YES_NO)
	void			DeleteSelectedUser();                               // 0x41e620 (called by the app on 2021)

	virtual void	ButtonPress(int theId) override;                    // 0x41df60 +0x58 slot 3
	virtual void	ButtonDepress(int theId) override;                  // 0x41e220 +0x58 slot 4
	virtual void	ListClicked(int theId, int theIdx, int theClickCount) override; // 0x41e150 +0x120 slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
