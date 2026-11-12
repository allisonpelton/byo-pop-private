#ifndef __BOOKWORM_HINTDIALOG_H__
#define __BOOKWORM_HINTDIALOG_H__

// HintDialog.cpp: dialog 15 (DIALOG_HINT), the board's tutorial hints (Board.cpp gHintHeaders/gHintLines).
// The footer button (2015, GT_HINT_CONTINUE) is moved up above a second button (GT_HINT_DISABLE,
// BUTTON_HINT_DISABLE, 26), which turns hints off for the session.
// Original: class Sexy::HintDialog : public Dialog, sizeof 0x120 (new in BookWormApp::ShowHint 0x4105d0).
// Vtables: 0x4fd434 (52 slots), ButtonListener 0x4fd414 (+0x58). Offsets are absolute. Behaviour spec:
// re/bookworm/analysis/dialogs.md section 8.

#include "Compat2004.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class DialogButton;

class HintDialog : public Dialog2004
{
public:
	BookWormApp*	mApp;                   // +0x110 ctor arg
	DialogButton*	mDisableButton;         // +0x114 id BUTTON_HINT_DISABLE (26), listener this, colours 0x52f27c
	int				mHintId;                // +0x118 ctor arg (the board's BoardHintId); never read
	// +0x11c (an int the ctor zeroes, never read): not ported

public:
	HintDialog(BookWormApp* theApp, int theHintId, const SexyString& theDialogHeader,
		const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode); // 0x42af50
	virtual ~HintDialog();                                              // 0x42a820 body, 0x42a990 slot 0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;    // 0x42a890 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;// 0x42a8c0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x42b160 slot 13 (uppercases the header)
	virtual void	Draw(Graphics* g) override;                         // 0x42e440 slot 16 (= Dialog::Draw, folded with MoneyDialog's)
	virtual void	Update() override;                                  // 0x429290 slot 17 (= Dialog::Update)
	virtual int		GetPreferredHeight(int theWidth) override;          // 0x42a8f0 slot 50 (+4 + max(yes height, 32))

	virtual void	ButtonPress(int theId, int theClickCount) override; // 0x42a920 +0x58 slot 2 (Dialog::ButtonPress(id) + click)
	virtual void	ButtonDepress(int theId) override;                  // 0x42a950 +0x58 slot 4 (26: hints off, KillDialog(15))

	static bool		WasHintShown(int theHintId);                        // 0x42b3e0 (cdecl; ignores the id, map never filled: false)

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
