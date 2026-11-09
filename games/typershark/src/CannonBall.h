#ifndef __TYPERSHARK_CANNONBALL_H__
#define __TYPERSHARK_CANNONBALL_H__

// CannonBall.cpp: the cannon ball that the ghost ship fires. It moves and counts frames like a Torpedo (Update and the
// word's TypeLetter are Torpedo's), but has its own pictures (a 74x44 frame of IMAGE_CANNONBALL in 12 steps), its own
// hit box and its own sizes for the states. SquidCannonBall derives from it.
// Original: class Sexy::CannonBall : Torpedo, Projectile, Enemy, GameObject (all virtual bases; the original makes
// the Torpedo part a virtual base too, at +0x210), own part of 8 bytes (vbptr at +0), the GameObject part at +8
// (vftable 0x529c30), the Enemy part at +0x100 (0x529c20), the Projectile part at +0x1f0 (0x529c14), the Torpedo
// part at +0x210, sizeof 0x218; the constructor is 0x40e400, the deleting destructor 0x40ebb0 (it calls 0x40f310).
//
// States as in Torpedo (0 and 4 flying right, 1 flying back, 3 exploding, 6 turning); the pictures in Enemy::mImages:
// 0 the ball, 3 the app's darkened copy for state 1, 6 the turn (IMAGE_CANNONBALL_TURN), 2 and 7 the explosion of
// Torpedo.
//
// port: the single chain GameObject, Enemy, Projectile, Torpedo, CannonBall; SquidCannonBall is the only class below.

#include "Torpedo.h"

namespace Sexy
{

class Graphics;

class CannonBall : public Torpedo
{
public:
	CannonBall(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer);	// 0x40e400
	virtual ~CannonBall();															// deleting destructor 0x40ebb0

	virtual bool			HitsTarget() override;									// Projectile slot 1, 0x401370
	virtual void			Draw(Graphics* g) override;								// slot 2, 0x407a90
	virtual void			SetState(int theState) override;						// slot 7, 0x404030
};

}

#endif
