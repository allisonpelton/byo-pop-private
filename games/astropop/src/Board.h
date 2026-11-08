#ifndef __ASTROPOP_BOARD_H__
#define __ASTROPOP_BOARD_H__

// Board: the playfield screen (original constructor 0x480020, vftable to come, sizeof 0x1c0c). Translated as far as the
// objects on it need so far; the members keep the original's offsets in comments.

#include "Widget.h"
#include "EventBus.h"

namespace Sexy
{

class Brick;
class AstroPopApp;

class Board : public Widget
{
public:
	bool			mFlag8E8;               // +0x8e8
	bool			mFlag8E9;               // +0x8e9
	int				mField193C;             // +0x193c
	int				mDropTicks;             // +0x1c04 updates a mutated brick waits before it drops

	Board();

	AstroPopApp*	mApp;                   // +0x358 the application (the bricks play their sounds through it)
};

}

#endif
