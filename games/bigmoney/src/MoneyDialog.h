#ifndef __BIGMONEY_MONEYDIALOG_H__
#define __BIGMONEY_MONEYDIALOG_H__

// MoneyDialog.cpp: the dialog with the chain of running lights around a black panel; base of
// HighScoreDialog, OptionsDialog and RecordScoreDialog (all pass IMAGE_DIALOG / IMAGE_DIALOGBUTTON).
// Original: class Sexy::MoneyDialog : public Dialog, sizeof 0x158, vftable 0x4f670c (60 slots),
// ButtonListener 0x4f66ec. Offsets are absolute.
// Behaviour spec: re/bigmoney/analysis/app.md section 8.

#include "Dialog.h"

namespace Sexy
{

class WinBMApp;
class Graphics;
class Image;

class MoneyDialog : public Dialog
{
public:
	enum
	{
		NUM_LIGHTS = 40
	};

	WinBMApp*		mApp;                       // +0x148
	// +0x14c pad
	double			mLightPhase;                // +0x150 0..1, +0.0005 per update while the app has focus

public:
	MoneyDialog(WinBMApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode); // 0x414e00
	virtual ~MoneyDialog();                                         // 0x414e60 (scalar deleting dtor 0x414ee0)

	virtual void	Update();                                       // 0x414e80 slot 9
	virtual void	Draw(Graphics* g);                              // 0x414f00 slot 13 (the subclasses' Draw calls it)
};

}

#endif
