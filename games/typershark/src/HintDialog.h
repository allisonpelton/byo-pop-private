#ifndef __TYPERSHARK_HINTDIALOG_H__
#define __TYPERSHARK_HINTDIALOG_H__

// HintDialog.cpp: the hint box (dialog 0x3f3, made by WinWSApp::ShowHintDialog): a header and the hint, one footer button
// and a check box "Click here to disable hints" under the text. Enter, space and escape press the button; ButtonDepress
// (0x431630, id 0xbc3) reads the check box to switch the hints off for the user.
// Original: class Sexy::HintDialog : public Dialog, public CheckboxListener, vftable 0x52da6c (52 slots, slot 50 returns the
// height 400), ButtonListener vftable 0x52da4c (+0x58), CheckboxListener vftable 0x52da44 (+0x110, one empty slot),
// sizeof 0x128, constructor 0x439ef0, destructor body 0x4380e0 (deleting destructor 0x438f40).

#include "compat2004/Compat2004.h"
#include "CheckboxListener.h"
#include "Checkbox.h"

namespace Sexy
{

class WinWSApp;
class Graphics;
class MemoryImage;
class WidgetManager;

class HintDialog : public Dialog2004, public CheckboxListener
{
public:
	WinWSApp*				mApp;					// +0x114 constructor argument
	MemoryImage*			mCheckedImage;			// +0x118 the right half of the check box picture (the checked box)
	MemoryImage*			mUncheckedImage;		// +0x11c the left half
	int						mButtonSample;				// +0x120 the sample every button press plays (WinWSApp::ShowHintDialog sets 5, the button click)
	Checkbox*				mDisableHintsCheckbox;				// +0x124 the "disable hints" check box

public:
	HintDialog(WinWSApp* theApp, int theId, const SexyString& theDialogHeader, const SexyString& theDialogLines,
		const SexyString& theDialogFooter, Image* theDialogImage, Image* theButtonImage, Image* theCheckedImage);	// 0x439ef0
	virtual ~HintDialog() override;													// 0x4380e0 (deleting 0x438f40), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4381c0 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4381f0 slot 2
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x438220 slot 13
	virtual void			Draw(Graphics* g) override;								// 0x43bbe0 slot 16
	virtual void			KeyChar(SexyChar theChar) override;						// 0x438180 slot 20
	// slot 50, GetPreferredHeight (0x438170, always 400), is not translated: ShowHintDialog sizes the box itself and nothing asks

	virtual void			ButtonPress(int theId) override;						// 0x438280 ButtonListener (+0x58) slot 3

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
