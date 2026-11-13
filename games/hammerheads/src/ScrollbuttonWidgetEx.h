#ifndef __HAMMERHEADS_SCROLLBUTTONWIDGETEX_H__
#define __HAMMERHEADS_SCROLLBUTTONWIDGETEX_H__

// A button of the game's scroll bar: its picture stretched over the button, a pixel down and to the right while it is held.
// Original: class Sexy::ScrollbuttonWidgetEx : public ScrollbuttonWidget (0x128 bytes; the constructor is 0x405a90).

#include "ScrollbuttonWidget.h"

namespace Sexy
{

class Image;

class ScrollbuttonWidgetEx : public ScrollbuttonWidget
{
public:
	Image*			mImage;				// +0x120

public:
	ScrollbuttonWidgetEx(int theId, Image* theImage, ButtonListener* theButtonListener);	// 0x405a90

	virtual void	Draw(Graphics* g) override;											// 0x405af0
};

}

#endif
