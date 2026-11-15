#ifndef __HEAVYWEAPON_NUKEBLAST_H__
#define __HEAVYWEAPON_NUKEBLAST_H__

// NukeBlast.cpp: a nuke's mushroom cloud behind the field: a stem and a cap of smoke puffs with fire inside the cap,
// drifting out and fading, carried back with the far background.
// Original: 0x3c bytes, constructor 0x4282e0, destructor 0x428a20, Draw 0x428770, Update 0x428ac0; the board keeps
// them in its list at +0x264 (the list's Remove of one node: 0x428a50).

#include "ObjectList.h"

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;

// A puff of the cloud's smoke (0x30 bytes)
struct NukeSmoke
{
	double			mX;				// +0x00 from the blast's centre
	double			mY;				// +0x08 from the screen's middle (240)
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
	int				mBrightness;	// +0x28 tinted by the level's sky (NUKE_SMOKE_TINTS)
	int				mFrame;			// +0x2c 0..2
};

// A fire inside the cloud's cap (0x30 bytes)
struct NukeFire
{
	double			mX;				// +0x00 from the blast's centre
	double			mY;				// +0x08 from the screen's middle
	double			mVelX;			// +0x10
	double			mVelY;			// +0x18
	int				mFrame;			// +0x20 0..2
	double			mHeat;			// +0x28 its colour, from white hot down to 0, 0.75 an update
};

class NukeBlast
{
public:
	HeavyWeaponApp*					mApp;		// +0x00
	Board*							mBoard;		// +0x04
	double							mX;			// +0x08 from the screen's centre
	double							mFade;		// +0x10 1024 down 1 an update; the smoke's alpha (at most 255)
	ObjectList<NukeFire>			mFires;		// +0x18
	ObjectList<NukeSmoke>			mSmoke;		// +0x28
	ObjectList<NukeBlast>::Node*	mNode;		// +0x38 its node in the board's list

public:
	// It adds itself to theBoard's list
	NukeBlast(Board* theBoard);		// 0x4282e0

	void			Draw(Graphics* g);	// 0x428770
	void			Update();			// 0x428ac0
};

}

#endif
