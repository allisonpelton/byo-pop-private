#ifndef __TYPERSHARK_KEYDIALOG_H__
#define __TYPERSHARK_KEYDIALOG_H__

// KeyDialog.cpp: the game's plain message box (dialog 500, made by WinWSApp::ShowMessageDialog and ShowConfirmDialog) and
// the base of DifficultyDialog. A Dialog that also answers the keyboard: with mCloseOnKey off any key closes it, with it
// on only the keys in the two lists press its buttons (the "cancel" keys press the no button, the "ok" keys the yes
// button).
// Original: class Sexy::KeyDialog : public Dialog, vftable 0x52ec4c (52 slots, slot 20 KeyChar), ButtonListener vftable
// 0x52ec30 (+0x58), sizeof 0x138, constructor 0x43ec30, destructor body 0x43a2d0 (deleting destructor 0x43ecf0).

#include "compat2004/Compat2004.h"
#include <vector>

namespace Sexy
{

class WinWSApp;

class KeyDialog : public Dialog2004
{
public:
	std::vector<unsigned char>	mCancelKeys;	// +0x110 (pad), +0x114..+0x11c: the keys that press the no button; the constructor starts it with escape
	std::vector<unsigned char>	mOkKeys;		// +0x120 (pad), +0x124..+0x12c: the keys that press the yes button; starts empty
	WinWSApp*					mApp;			// +0x130 constructor argument
	bool						mCloseOnKey;	// +0x134 the callers set it: only the listed keys act (off: any key kills the dialog)

public:
	KeyDialog(WinWSApp* theApp, Image* theComponentImage, Image* theButtonComponentImage, int theId, bool isModal,
		const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter,
		int theButtonMode);															// 0x43ec30
	virtual ~KeyDialog() override;													// 0x43a2d0 (deleting 0x43ecf0), slot 0

	virtual void			KeyChar(SexyChar theChar) override;						// 0x439210 slot 20

	void					RemoveCancelKey(char theKey);							// 0x43a3e0 (the first entry of that key leaves the list at +0x110)
	void					AddOkKey(char theKey);									// 0x43ed20 (adds to the list at +0x120 unless it is in it)
};

}

#endif
