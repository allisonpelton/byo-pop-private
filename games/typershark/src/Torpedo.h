#ifndef __TYPERSHARK_TORPEDO_H__
#define __TYPERSHARK_TORPEDO_H__

// Torpedo.cpp: a torpedo that a ship or boss fires across the board. It flies right at the speed it speeds up to (3.0),
// spins a rotor, carries a word of its own that the player types to turn it back (the last letter sends it back left
// as state 6, then 1), explodes when it hits (state 3). CannonBall and MechaTorpedo derive from it (SquidCannonBall
// through CannonBall).
// Original: class Sexy::Torpedo : Projectile, Enemy, GameObject (all virtual bases), own part with no virtuals (vbptr
// at +0, one int at +4), the GameObject part at +0x10 (vftable 0x529638), the Enemy part at +0x108 (0x529628), the
// Projectile part at +0x1f8 (0x52961c, 2 virtuals), sizeof 0x210; the constructor is 0x40c1a0 (its last argument says
// whether it builds the virtual bases itself), the deleting destructor 0x409b10 (it calls 0x409b40). The field
// comments give the offsets in the original's own part.
//
// States (GameObject::mState): 0 flying right with the launch flash, 1 flying back left (after the word was typed:
// mirrored picture, speeds up by 0.05 to 3.0), 3 exploding (SetState plays the explosion), 4 flying right but
// knocked about by Projectile::Update (slows by 0.05 a tick), 6 turning round (the picture of the turn; ends in state
// 1). The moves in state 0, 1, 4 and 6 are mX += mVelX.
//
// Enemy::mImages here has eight places: 0 swim (states 0, 4), 2 explode (state 3 flying left), 3 mirrored swim (the
// app's darkened copy, state 1), 6 reverse (the turn, state 6), 7 explode again (state 3 flying right); 1, 4 and 5
// stay empty.
//
// port: the single chain GameObject, Enemy, Projectile, Torpedo; no class derives from Torpedo through a second path,
// so no virtual inheritance.

#include "Projectile.h"

namespace Sexy
{

class Graphics;

class Torpedo : public Projectile
{
public:
	int						mLaunchStep;			// +0x04 frame of the launch flash (200x100 pictures of IMAGE_TORPEDO_LAUNCH): Update counts it up every 5 ticks in states 0 and 4 to 8, which ends the flash

public:
	Torpedo(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer);	// 0x40c1a0
	virtual ~Torpedo();																// deleting destructor 0x409b10

	virtual bool			HitsTarget() override;									// Projectile slot 1, 0x401b90
	virtual void			Update() override;										// GameObject slot 1, 0x40c3d0
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x4085e0
	virtual void			SetState(int theState) override;						// slot 7, 0x405080
	virtual void			TypeLetter() override;									// Enemy slot 2, 0x40c590
};

}

#endif
