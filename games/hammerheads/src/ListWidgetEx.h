#ifndef __HAMMERHEADS_LISTWIDGETEX_H__
#define __HAMMERHEADS_LISTWIDGETEX_H__

// The game's list of lines (the player list): a framework list on a picture box, whose selected line is shaded over the whole
// width of the list. Original: class Sexy::ListWidgetEx : public ListWidget (0xf8 bytes; the constructor is 0x401000).

#include "ListWidget.h"

namespace Sexy
{

class Image;

class ListWidgetEx : public ListWidget
{
public:
	Image*			mBoxImage;			// +0xf0: the picture box drawn behind the list

public:
	ListWidgetEx(int theId, Font* theFont, Image* theBoxImage, ListListener* theListListener);	// 0x401000

	virtual void	Draw(Graphics* g) override;											// 0x4010c0
};

}

#endif
