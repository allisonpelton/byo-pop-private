#ifndef __PIXELUS_SCROLLBAR_H__
#define __PIXELUS_SCROLLBAR_H__

// Scrollbar.cpp: the game's scroll bar (Pixelus.exe 0x41c7d0 constructor, vftable 0x50eb58; its arrow buttons 0x41cc50,
// vftable 0x50ec50): the framework's ScrollbarWidget with its track and thumb drawn as stretched boxes of two pictures,
// the arrows as pictures that move one pixel down and right while held, and, where the owner wants to scroll the list
// itself (the temple glides its puzzles), a go-between that the arrows and the track around the thumb ask first.

#include "ScrollbarWidget.h"
#include "ScrollbuttonWidget.h"

namespace Sexy
{

class Image;
class ScrollListener;

// the +0x60 interface of the temple screen (vftable 0x50d838): each call does the scrolling and returns true, meaning the
// scroll bar has nothing more to do
class ScrollKeys
{
public:
	virtual bool				LineUp() = 0;		// 0x4100b0
	virtual bool				LineDown() = 0;		// 0x4100c0
	virtual bool				PageUp() = 0;		// 0x4100d0
	virtual bool				PageDown() = 0;		// 0x4100e0
};

class PixelusScrollButton : public ScrollbuttonWidget
{
public:
	Image*						mImage;				// +0xf8

	PixelusScrollButton(int theId, Image* theImage, ButtonListener* theListener);	// 0x41cc50
	virtual void				Draw(Graphics* g) override;						// 0x41ccb0
};

class PixelusScrollbar : public ScrollbarWidget
{
public:
	Image*						mTrack;				// +0xb0 the box the whole bar is
	Image*						mThumb;				// +0xb4 the box the thumb is
	ScrollKeys*					mKeys;				// +0xb8 or NULL

	PixelusScrollbar(int theId, Image* theTrack, Image* theThumb, Image* theUpArrow, Image* theDownArrow,
		ScrollListener* theListener, ScrollKeys* theKeys);							// 0x41c7d0

	virtual void				Draw(Graphics* g) override;						// 0x41cb90
	virtual void				DrawThumb(Graphics* g, int theX, int theY, int theWidth, int theHeight) override;	// 0x41cb20
	virtual void				ButtonPress(int theId) override;				// 0x41ca80
	virtual void				ButtonPress(int theId, int theClickCount) override { ButtonPress(theId); }
	virtual void				MouseDown(int x, int y, int theBtnNum, int theClickCount) override;	// 0x41c970
	using ScrollbarWidget::MouseDown;
	virtual void				MouseUp(int x, int y, int theBtnNum, int theClickCount) override;	// 0x41c930
	using ScrollbarWidget::MouseUp;
};

}

#endif
