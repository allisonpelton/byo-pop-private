#ifndef __TYPERSHARK_FADEBUTTON_H__
#define __TYPERSHARK_FADEBUTTON_H__

// FadeButton.cpp: an image button whose "over" image fades in or out over the normal image (mAlpha 0..255,
// raised by mFadeStep every mFadeInterval updates), with a tooltip. Used by the menus, the map and the status bar.
// Original: class Sexy::FadeButton : public ButtonWidget, vftable 0x52f544, sizeof 0xfc (constructor 0x443440,
// destructor 0x443540, deleting destructor 0x443510).

#include "ButtonWidget.h"

namespace Sexy
{

class TooltipWidget;

class FadeButton : public ButtonWidget
{
public:
	int						mAlpha;			// +0xe0 alpha of the fading image; ctor 255
	bool					mReleasedOver;	// +0xe4 a mouse-up happened over the button (ctor leaves it unset; port: false)
	bool					mFadeOnEnter;	// +0xe5 MouseEnter restarts the fade from 0, and it runs while the pointer rests
	bool					mFadeOnLeave;	// +0xe6 MouseLeave restarts the fade from 0 and it runs while the pointer is away
	bool					mFadeOnDown;	// +0xe7 the fade runs while the button is held
	bool					mFadeOnUp;		// +0xe8 MouseUp restarts the fade from 0; it runs while the pointer is over
	int						mFadeInterval;	// +0xec updates between steps (ctor 1; a divisor, never 0)
	int						mFadeStep;		// +0xf0 alpha added per step; ctor 27 (0x1b)
	TooltipWidget*			mTooltip;		// +0xf4 owned; added to the manager with the button
	bool					mTooltipOn;		// +0xf8 MouseEnter arms the tooltip (when it has text); ctor true

public:
	FadeButton(int theId, ButtonListener* theListener);						// 0x443440
	virtual ~FadeButton() override;											// 0x443540 (deleting 0x443510), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x440e70 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x440eb0 slot 2
	virtual void			OrderInManagerChanged() override;				// 0x440ee0 slot 3
	virtual void			SetDisabled(bool isDisabled) override;			// 0x441d40 slot 10
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x441ca0 slot 13
	virtual void			Draw(Graphics* g) override;						// 0x446b10 slot 16
	virtual void			Update() override;								// 0x440d60 slot 17
	virtual void			MouseEnter() override;							// 0x443c50 slot 23
	virtual void			MouseLeave() override;							// 0x443d40 slot 24
	// 0x440d10 is the 2004 MouseUp(x, y) (slot 30); the port's ButtonWidget does its work in the 4-argument one
	virtual void			MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
	// 0x440cc0 is the 2004 MouseDown(x, y, clickCount) (slot 27); likewise done in the 4-argument one
	virtual void			MouseDown(int x, int y, int theBtnNum, int theClickCount) override;

	using ButtonWidget::MouseUp;
	using ButtonWidget::MouseDown;
};

}

#endif
