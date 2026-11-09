#ifndef __TYPERSHARK_TOOLTIPWIDGET_H__
#define __TYPERSHARK_TOOLTIPWIDGET_H__

// TooltipWidget.cpp: the small yellow label that FadeButton and XWidget show under the pointer after a delay.
// It owns a WSTimer that it adds to the widget manager so the timer runs (and removes it again).
// Original: class Sexy::TooltipWidget : public Widget, vftable 0x52f484, sizeof 0xa8 (new(0xa8), constructor
// 0x443010, destructor 0x443150, deleting destructor 0x443a10).

#include "Widget.h"
#include "Color.h"

namespace Sexy
{

class Font;
class WSTimer;

class TooltipWidget : public Widget
{
public:
	WSTimer*				mTimer;			// +0x58 started when armed; adds itself to the manager through us
	bool					mTipOn;			// +0x5c the tip is showing (drawn and repainted every update)
	bool					mShown;			// +0x5d the tip has been shown and not yet timed out
	bool					mArmed;			// +0x5e waiting for the pointer to rest (SetArmed(true))
	SexyString				mText;			// +0x60 (std::string, 0x1c bytes)
	Font*					mFont;			// +0x7c set by the owner of the tooltip; ctor NULL
	Color					mBackColor;		// +0x80 ctor (255, 255, 225)
	Color					mTextColor;		// +0x90 ctor black
	int						mShowDelay;		// +0xa0 milliseconds of WSTimer time before the tip shows; ctor 1000
	int						mHideDelay;		// +0xa4 milliseconds shown before it hides again; ctor 2000

public:
	TooltipWidget();														// 0x443010
	virtual ~TooltipWidget() override;										// 0x443150 (deleting 0x443a10), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x441600 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x441620 slot 2
	virtual void			Draw(Graphics* g) override;						// 0x4431d0 slot 16
	virtual void			Update() override;								// 0x4416a0 slot 17

	void					SetArmed(bool on);								// 0x441640

	// ---- added for W1 (the StatusBar and MapScreen make their tooltips with it) ----
	void					SetTextAndFont(SexyString theText, Font* theFont);	// 0x449870 sets mText and mFont and sizes the tip to the text (x and y stay)
};

}

#endif
