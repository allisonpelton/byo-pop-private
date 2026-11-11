#ifndef __TYPERSHARK_PIRANHA_H__
#define __TYPERSHARK_PIRANHA_H__

// Piranha.cpp: the small fish that swim in from the right, dash across the screen and turn into a quick shock when they
// are typed. The red, white and stealth piranhas derive from it.
// Original: class Sexy::Piranha : Enemy, GameObject (virtual bases), vftable 0x529600 (own part, 1 virtual 0x401770),
// 0x5295cc (GameObject part at +0x30), 0x5295bc (Enemy part at +0x120), sizeof 0x210; constructor 0x40fca0 (the
// part-constructors are 0x401730 and 0x409930). The field comments give the offsets in the original's own part.
//
// States (GameObject::mState): 0 swimming in, 1 swimming back (mirrored picture), 2 dead (not drawn), 3 dying (falls
// up), 4 chased off to the left, 5 shocked (flashing), 6 ignored by SetState.

#include "Enemy.h"

namespace Sexy
{

class Graphics;

class Piranha : public Enemy
{
public:
	bool					mPlayedSound;			// +0x10 the sound of coming in was played (Update, state 1)
	bool					mAlwaysFalseA;				// +0x11 constructor false; never read and never set again
	bool					mAlwaysFalseB;				// +0x12 constructor false; never read and never set again
	int						mType;					// +0x14 constructor argument 2
	double					mSavedVelX;				// +0x18 the speed to come back to after the dash; -1 = none yet
	bool					mEntered;				// +0x20 state 1 has brought the fish on to the screen
	int						mFrameOffset;			// +0x24 mFrame * mFrameWidth: the x of the source rectangle in the picture

public:
	Piranha(WinWSApp* theApp, int theType);											// 0x40fca0
	virtual ~Piranha();																// deleting destructor 0x409930

	virtual bool			IsSwimming();											// own slot 0, 0x401770: state 0 and visible
	virtual void			Update() override;												// slot 1, 0x40a6a0
	virtual void			Draw(Graphics* g) override;										// slot 2, 0x402360
	virtual void			SetState(int theState) override;									// slot 7, 0x404950
};

}

#endif
