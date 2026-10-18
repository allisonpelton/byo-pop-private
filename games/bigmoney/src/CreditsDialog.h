#ifndef __BIGMONEY_CREDITSDIALOG_H__
#define __BIGMONEY_CREDITSDIALOG_H__

// CreditsDialog.cpp: the credits (dialog 14, from the Options dialog): static lines over drifting and
// rotating clouds.
// Original: class Sexy::CreditsDialog : public Dialog, sizeof 0x168 (new in WinBMApp::ShowCreditsDialog
// 0x41baf0), vftable 0x4f5aac (60 slots), ButtonListener vftable 0x4f5a88. Offsets are absolute (Dialog is
// 0x148 bytes in 1.2). Behaviour spec: re/bigmoney/analysis/screens.md.

#include "Dialog.h"

namespace Sexy
{

class WinBMApp;
class Graphics;

class CreditsDialog : public Dialog
{
public:
	enum
	{
		PREFERRED_HEIGHT = 396
	};

	WinBMApp*		mApp;                   // +0x148
	Widget*			mUnused14C;             // +0x14c deleted by the dtor if set; never set
	double			mCloudRot;              // +0x150 rotation of the big cloud layer (radians), += 0.004
	double			mCloudX;                // +0x158 horizontal scroll, += 0.2, -512 when > 0
	double			mCloudY;                // +0x160 vertical scroll, += 0.3, -512 when > 0

public:
	CreditsDialog(WinBMApp* theApp);                                // 0x40ef30
	virtual ~CreditsDialog();                                       // 0x40ee10 (scalar deleting dtor 0x40ef10) slot 0
	virtual void	Update();                                       // 0x40ee90 slot 9
	virtual void	Draw(Graphics* g);                              // 0x40f190 slot 13
	virtual int		GetPreferredHeight(int theWidth);               // 0x40ee80 slot 57 (COMDAT shared with HighScoreDialog)
};

}

#endif
