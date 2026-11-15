#ifndef __HAMMERHEADS_CREDITSDIALOG_H__
#define __HAMMERHEADS_CREDITSDIALOG_H__

// The credits: a picture with the names, and an "Ok" button under it. Original: class Sexy::CreditsDialog : public NcMovingDialog,
// public FlashListener (0x178 bytes; the constructor is 0x44f3e0, the destructor 0x44f0a0). The Flash widget it keeps at +0x170 is
// never made, so the listener's two functions (0x44f300, 0x44f370; they only write to the debug output) and the code that
// places and adds the widget are left out.

#include "NcMovingDialog.h"

namespace Sexy
{

class NcButton;

class CreditsDialog : public NcMovingDialog
{
public:
	NcButton*		mOkButton;			// +0x16c: id 0

public:
	CreditsDialog();														// 0x44f3e0
	virtual ~CreditsDialog();												// 0x44f0a0

	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x44f120
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;			// 0x44f1e0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;		// 0x44f220
	virtual void	Draw(Graphics* g) override;								// 0x44f260
	virtual int		GetPreferredHeight(int theWidth) override;				// 0x44f2d0
	virtual void	ButtonDepress(int theId) override;						// 0x44f290
	virtual void	Closed() override;										// 0x44f2b0
};

// 0x44f5d0: the box is made, sized and added as dialog 0x11
CreditsDialog* MakeCreditsDialog();

}

#endif
