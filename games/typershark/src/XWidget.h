#ifndef __TYPERSHARK_XWIDGET_H__
#define __TYPERSHARK_XWIDGET_H__

// XWidget.cpp: an image that pulses (alpha between 150 and 255, 2 per update) and shows a tooltip when the
// pointer rests on it; created by the screens (0x45ad20) with a letter ('A' + n, or 'A').
// Original: class Sexy::XWidget : public Widget, vftable 0x52f60c (constructor 0x4435a0, destructor 0x443680,
// deleting destructor 0x443650).

#include "Widget.h"

namespace Sexy
{

class Image;
class TooltipWidget;
class SexyAppBase;

class XWidget : public Widget
{
public:
	SexyAppBase*			mApp;			// +0x58 the WinWSApp
	Widget*					mOwner;			// +0x5c the screen that made the widget (second constructor argument)
	bool					mFadingUp;		// +0x60 true while the alpha rises (ctor false: it starts by falling)
	int						mAlpha;			// +0x64 ctor 255
	int						mLevel;			// +0x68 the map's level (node) this X stands for; set by the creating screen (0x45ad20), not by the constructor
	TooltipWidget*			mTooltip;		// +0x6c owned; added to the manager with the widget
	Image*					mImage;			// +0x70 drawn at 0, 0
	char					mLetter;		// +0x74 the letter the widget stands for (constructor argument)

public:
	XWidget(SexyAppBase* theApp, Widget* theOwner, Image* theImage, char theLetter);	// 0x4435a0
	virtual ~XWidget() override;											// 0x443680 (deleting 0x443650), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x440f00 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x440f30 slot 2
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x441d70 slot 13
	virtual void			Draw(Graphics* g) override;						// 0x440fb0 slot 16
	virtual void			Update() override;								// 0x440f60 slot 17
	virtual void			MouseEnter() override;							// 0x441e30 slot 23
	virtual void			MouseLeave() override;							// 0x441e50 slot 24
	virtual void			MouseDown(int x, int y, int theClickCount) override;	// 0x458cb0 slot 27 (the 2004 MouseDown(x, y, clickCount); any button)

	using Widget::Resize;
	using Widget::MouseDown;
};

}

#endif
