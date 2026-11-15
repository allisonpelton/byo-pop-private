#ifndef __HEAVYWEAPON_PAUSESCREEN_H__
#define __HEAVYWEAPON_PAUSESCREEN_H__

// PauseScreen.cpp: the dark veil and "paused" line over a paused game. It silences the music and sounds while it is up,
// and a click or the space bar takes it away (it deletes itself).
// Original: 0x5c bytes, constructor 0x42bc50, destructor 0x42bce0 (deleting 0x42be10); vtable 0x52ed70 (a Widget:
// 16 Draw 0x42be40, 23 KeyUp 0x42bde0, 28 MouseDown 0x42bdd0).

#include "Widget.h"

namespace Sexy
{

class Board;

class PauseScreen : public Widget
{
public:
	Board*			mBoard;			// +0x58

public:
	PauseScreen(Board* theBoard);												// 0x42bc50
	virtual ~PauseScreen();														// 0x42bce0

	virtual void	Draw(Graphics* g) override;									// 0x42be40 slot 16
	virtual void	KeyUp(KeyCode theKey) override;								// 0x42bde0 slot 23
	virtual void	MouseDown(int x, int y, int theClickCount) override;		// 0x42bdd0 slot 28

	using Widget::MouseDown;
};

}

#endif
