#ifndef __BOOKWORM_MONEYDIALOG_H__
#define __BOOKWORM_MONEYDIALOG_H__

// MoneyDialog.cpp: base of NewUserDialog, SelectUserDialog, DeleteUserDialog and OptionsDialog. The name
// and size come from Big Money (where it drew a chain of running lights); in BookWorm it is a plain Dialog
// plus the app pointer and adds no behaviour.
// Original: class Sexy::MoneyDialog : public Dialog, sizeof 0x120 (inferred: never allocated alone, the
// subclasses' members start at +0x120), vftable 0x500774 (52 slots), ButtonListener 0x500758 (+0x58).
// Offsets are absolute. Behaviour spec: re/bookworm/analysis/dialogs.md section 2.

#include "Compat2004.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class Image;

class MoneyDialog : public Dialog2004
{
public:
	BookWormApp*	mApp;                   // +0x110 ctor arg; never read by BookWorm code (subclasses keep a copy)
	// +0x114 pad
	double			mUnusedLightPhase;      // +0x118 Big Money's mLightPhase: never initialised or used

public:
	MoneyDialog(BookWormApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId,
		bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
		const SexyString& theDialogFooter, int theButtonMode);         // 0x42e3d0
	virtual ~MoneyDialog();                                             // 0x42e420 body, 0x42e450 slot 0

	virtual void	Draw(Graphics* g) override;                         // 0x42e440 slot 16 (= Dialog::Draw)
	virtual void	Update() override;                                  // 0x429290 slot 17 (folded jmp Dialog::Update)
};

}

#endif
