#ifndef __HAMMERHEADS_SCROLLBARWIDGETEX_H__
#define __HAMMERHEADS_SCROLLBARWIDGETEX_H__

// The game's scroll bar: pictures for the track, the thumb and the buttons; it tells its listener where the thumb is again when
// the mouse is let go. Original: class Sexy::ScrollbarWidgetEx : public ScrollbarWidget (0xe8 bytes; the constructor is 0x405550).

#include "ScrollbarWidget.h"

namespace Sexy
{

class Image;

class ScrollbarWidgetEx : public ScrollbarWidget
{
public:
	Image*			mTrackImage;		// +0xd8: the picture box drawn behind the thumb
	Image*			mThumbImage;		// +0xdc: the picture box that is the thumb

public:
	// theButtonImage is the picture of both buttons (the exe is given a second one for the lower button and does not use it)
	ScrollbarWidgetEx(int theId, Image* theTrackImage, Image* theThumbImage, Image* theButtonImage, ScrollListener* theScrollListener);	// 0x405550

	virtual void	Draw(Graphics* g) override;											// 0x4059d0
	virtual void	DrawThumb(Graphics* g, int theX, int theY, int theWidth, int theHeight) override;	// 0x405960
	virtual void	MouseUp(int x, int y, int theBtnNum, int theClickCount) override;	// 0x405730
};

}

#endif
