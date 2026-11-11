#ifndef __ASTROPOP_ASTROSCROLLBAR_H__
#define __ASTROPOP_ASTROSCROLLBAR_H__

// The scroll bar of the player list (ListDialog, UserSelectDialog). The game's exe links a ScrollbarWidget and a
// ScrollbuttonWidget of its own (vtables 0x5640a0 and 0x563fc0) that draw in the game's colours; the framework's are
// the plain grey ones of SAF 1.3. Everything but the two Draw functions is the framework's.

#include "ScrollbarWidget.h"
#include "ScrollbuttonWidget.h"

namespace Sexy
{

// 0x4356b0: the arrows of the two buttons are amber while the bar can scroll (the grey of a disabled button as in the framework)
class AstroScrollbutton : public ScrollbuttonWidget
{
public:
	AstroScrollbutton(int theId, ButtonListener* theButtonListener);

	virtual void Draw(Graphics* g) override;
};

// 0x4354d0: the track of the bar is dark teal (a paging press on it dark grey); a horizontal bar, which the game never has, keeps
// the framework's light grey after its thumb
class AstroScrollbar : public ScrollbarWidget
{
public:
	AstroScrollbar(int theId, ScrollListener* theScrollListener);

	virtual void Draw(Graphics* g) override;
};

}

#endif
