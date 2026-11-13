#ifndef __HEAVYWEAPON_MEGALASERPART_H__
#define __HEAVYWEAPON_MEGALASERPART_H__

// MegaLaserPart.cpp: a part of the mega laser floating down on its parachute. The tank catches it and it flies to the
// mega laser's meter; the fourth part charges the mega laser. One that reaches the ground blows up.
// Original: 0x38 bytes, constructor 0x42da00, Draw 0x42d9b0, Update 0x42db00; the board keeps them in its list at
// +0x274 (the list's Remove of one node: 0x42daa0).

#include "ObjectList.h"

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;

class MegaLaserPart
{
public:
	enum
	{
		NUM_PARTS = 4
	};

	HeavyWeaponApp*					mApp;			// +0x00
	Board*							mBoard;			// +0x04
	double							mX;				// +0x08 from the screen's centre
	double							mY;				// +0x10
	double							mVelX;			// +0x18
	double							mVelY;			// +0x20
	double							mFrame;			// +0x28 which part: the parts caught before it
	bool							mIsCaught;		// +0x30 the tank caught it: it flies to the meter
	ObjectList<MegaLaserPart>::Node* mNode;		// +0x34 its node in the board's list

public:
	// It adds itself to theBoard's list
	MegaLaserPart(Board* theBoard, int x, int y);	// 0x42da00

	void			Draw(Graphics* g);				// 0x42d9b0
	void			Update();						// 0x42db00
};

}

#endif
