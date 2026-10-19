#ifndef __BIGMONEY_LOCKABLEBUTTON_H__
#define __BIGMONEY_LOCKABLEBUTTON_H__

// LockableButton.cpp: a ButtonWidget with a lock flag that nothing ever sets or reads (a registration
// leftover); GameSelector's three game-type buttons.
// Original: class Sexy::LockableButton : public ButtonWidget, sizeof 0x120, vftable 0x4f6604 (57 slots).
// Behaviour spec: re/bigmoney/analysis/app.md section 9.

#include "ButtonWidget.h"

namespace Sexy
{

class WinBMApp;
class Graphics;

class LockableButton : public ButtonWidget
{
public:
	WinBMApp*		mApp;                       // +0x118 never read
	bool			mLocked;                    // +0x11c false; never read or set again
	// +0x11d..+0x11f pad

public:
	LockableButton(WinBMApp* theApp, int theId, ButtonListener* theButtonListener); // 0x414d80
	virtual ~LockableButton();                                      // 0x414dc0 (scalar deleting dtor 0x414de0)

	virtual void	Draw(Graphics* g) override;                              // 0x414dd0 slot 13 (jmp ButtonWidget::Draw)
};

}

#endif
