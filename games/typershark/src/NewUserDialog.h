#ifndef __TYPERSHARK_NEWUSERDIALOG_H__
#define __TYPERSHARK_NEWUSERDIALOG_H__

// NewUserDialog.cpp: "NEW USER" (dialog 0x44d, made by WinWSApp::ShowNewUserDialog): the prompt text (the dialog's
// lines), an edit box for the player's name drawn in a three-slice frame of IMAGE_EDITBOX, and OK / CANCEL. OK or Enter
// is id 0x44d + 2000 (the app reads mNameEdit->mString and makes the user), CANCEL is 0x44d + 3000.
// Original: class Sexy::NewUserDialog : public MoneyDialog, public NaziEditListener, vftable 0x52dc74 (52 slots),
// ButtonListener vftable 0x52dc58 (+0x58), edit listener vftable 0x52dc44 (+0x120), sizeof 0x130 (operator new in
// 0x414270), constructor 0x43bcc0, destructor body 0x438360 (deleting destructor 0x438fa0). The base MoneyDialog
// (constructor 0x4382c0, destructor body 0x438310) is a Dialog that keeps the app pointer at +0x110 and adds nothing
// (its Draw 0x438330 and Update 0x438340 forward to Dialog's): it is not a class of the port, so this class derives from
// Dialog2004 and the app pointer is mApp below. NaziEditListener is the game's EditListener: its slot 2 (AllowChar,
// 0x438350) refuses '^' (the colour-code character of WriteString2004), slot 0 is a pure stub and slots 1 and 3 are the
// defaults; NewUserDialog overrides slot 0 only.

#include "compat2004/Compat2004.h"
#include "EditListener.h"
#include "Rect.h"

namespace Sexy
{

class WinWSApp;
class Graphics;
class WidgetManager;
class EditWidget;

class NewUserDialog : public Dialog2004, public EditListener
{
public:
	WinWSApp*				mApp;					// +0x124 constructor argument (MoneyDialog's copy at +0x110 is the same pointer)
	EditWidget*				mNameEdit;				// +0x128 id 0, Rockwell 13, at most 24 characters and 240 pixels; the app reads mString
	// +0x12c pad to 0x130

public:
	NewUserDialog(WinWSApp* theApp, const SexyString& theDialogLines);						// 0x43bcc0
	virtual ~NewUserDialog() override;														// 0x438360 (deleting 0x438fa0), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4383f0 slot 1 (adds and focuses the edit)
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x438430 slot 2
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x438460 slot 13
	virtual void			Draw(Graphics* g) override;										// 0x4392d0 slot 16
	virtual int				GetPreferredHeight(int theWidth) override;						// 0x4383d0 slot 50 (+ 56)

	void					DrawEditBox(Graphics* g, const Rect& theRect);					// 0x438fd0 (IMAGE_EDITBOX three-slice frame)

	virtual void			EditWidgetText(int theId, const SexyString& theString) override;	// 0x4384b0 EditListener (+0x120) slot 0 -> app ButtonDepress(mId + 2000)
	virtual bool			AllowChar(int theId, SexyChar theChar) override;				// 0x438350 EditListener (+0x120) slot 2 (NaziEditListener's: anything but '^')

	using Widget::Resize;
};

}

#endif
