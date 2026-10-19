#ifndef __BOOKWORM_NEWUSERDIALOG_H__
#define __BOOKWORM_NEWUSERDIALOG_H__

// NewUserDialog.cpp: dialog 22 (DIALOG_NEW_USER) with the player-name edit box drawn in the
// IMAGE_EDITBOX frame. OK or Enter = 2022 (the app trims the name and creates the user), CANCEL = 3022.
// Original: class Sexy::NewUserDialog : public MoneyDialog, public EditListener, sizeof 0x130 (new in
// BookWormApp::ShowNewUserDialog 0x4108c0). Vtables: 0x50087c (52 slots), ButtonListener 0x50085c (+0x58),
// EditListener 0x500848 (+0x120). Offsets are absolute. Behaviour spec: re/bookworm/analysis/dialogs.md
// section 4 (name rules: section 9).

#include "MoneyDialog.h"
#include "EditListener.h"
#include "Rect.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class EditWidget;

class NewUserDialog : public MoneyDialog, public EditListener
{
public:
	enum
	{
		NAME_EDIT_ID = 0,
		NAME_MAX_CHARS = 22,
		NAME_MAX_PIXELS = 200,
		EXTRA_HEIGHT = 56
	};

	// +0x120 EditListener vfptr
	BookWormApp*	mApp2;                  // +0x124 app
	EditWidget*		mNameEdit;              // +0x128 id 0, Verdana12Bold, colours 0x52f530; the app reads mString
	// +0x12c pad to 0x130

public:
	NewUserDialog(BookWormApp* theApp, const SexyString& theDialogLines); // 0x42e7c0
	virtual ~NewUserDialog();                                           // 0x42e480 body, 0x42e620 slot 0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;    // 0x42e510 slot 1 (adds and focuses the edit)
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;// 0x42e550 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x42e580 slot 13
	virtual void	Draw(Graphics* g) override;                         // 0x42ea10 slot 16
	virtual int		GetPreferredHeight(int theWidth) override;          // 0x42e4f0 slot 50 (+56)

	void			DrawEditBox(Graphics* g, const Rect& theRect);      // 0x42e650 (IMAGE_EDITBOX three-slice frame)

	virtual void	EditWidgetText(int theId, const SexyString& theString) override; // 0x42e5d0 +0x120 slot 0 -> app ButtonDepress(mId + 2000)
	virtual bool	AllowChar(int theId, SexyChar theChar) override;    // 0x42e5f0 +0x120 slot 2 (letters and space)

	using Widget::Resize;
};

}

#endif
