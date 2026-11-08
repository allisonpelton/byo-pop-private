#ifndef __TYPERSHARK_SQUIDCANNONBALL_H__
#define __TYPERSHARK_SQUIDCANNONBALL_H__

// SquidCannonBall.cpp: the cannon ball that MechaSquid fires: a CannonBall with another hit box (HitsTarget); the
// pictures, Update, Draw, SetState and TypeLetter are CannonBall's and Torpedo's.
// Original: class Sexy::SquidCannonBall : CannonBall, Torpedo, Projectile, Enemy, GameObject (all virtual bases), own
// part of 8 bytes, the GameObject part at +8 (vftable 0x529dbc), the Enemy part at +0x100 (0x529dac), the Projectile
// part at +0x1f0 (0x529da0), the Torpedo part at +0x210, the CannonBall part at +0x218; the constructor is 0x40f150
// (it only constructs CannonBall), the deleting destructor 0x40ebb0 (shared with CannonBall).

#include "CannonBall.h"

namespace Sexy
{

class SquidCannonBall : public CannonBall
{
public:
	SquidCannonBall(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer);	// 0x40f150
	virtual ~SquidCannonBall();															// deleting destructor 0x40ebb0

	virtual bool			HitsTarget() override;										// Projectile slot 1, 0x401590
};

}

#endif
