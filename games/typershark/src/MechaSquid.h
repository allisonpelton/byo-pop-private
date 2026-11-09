#ifndef __TYPERSHARK_MECHASQUID_H__
#define __TYPERSHARK_MECHASQUID_H__

// MechaSquid.cpp: the mechanical squid boss. A GhostShip (so the letter words, one letter on each shot) that fires
// SquidCannonBalls from three places; instead of the galleon's rudder it has a fin that turns through 10 frames and an
// eye that looks back and forth through 5.
// Original: class Sexy::MechaSquid : GhostShip, TorpedoBoss, ProjectileBoss, Enemy, GameObject (all virtual bases), the
// Enemy part at +0 (vftable 0x529e78, 7 virtuals: only CreateProjectile 0x40f7a0 differs from GhostShip's), vbptr at +4,
// the GameObject part at +0x1d8 (vftable 0x529e44, 0xf0 bytes); the constructor is 0x40f390 (its last argument says
// whether it builds the virtual bases itself), the destructor of the part 0x40f6d0 (a jump to TorpedoBoss's 0x408d00).
// The field comments give the offsets in the original's object. The fin uses GhostShip's rudder fields (mRudderFrame,
// mRudderHeight, mRudderWidth and mRudderFrames) and MechaSquid has two of its own.
//
// port: the single chain GameObject, Enemy, ProjectileBoss, TorpedoBoss, GhostShip, MechaSquid, so no virtual
// inheritance. The vtordisp thunks (GameObject slots 0 0x40f650 (to the deleting destructor 0x40f690), 1 0x40f870, 2
// 0x40f680, 3 and 4 0x40f660 and 0x40f670, 7 0x40f6e0 to TorpedoBoss::SetState) only adjust the pointer.

#include "GhostShip.h"

namespace Sexy
{

class MechaSquid : public GhostShip
{
public:
	int						mEyeFrame;				// +0x1c8 ctor 0: the frame of the eye (IMAGE_BOSS_SQUID_EYE, 12x12 pixels each, 0 to 4); Update moves it every 8th tick
	bool					mEyeRising;				// +0x1cc ctor 1: the eye's frame counts up (1) or down (0)

public:
	MechaSquid(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig);	// 0x40f390
	virtual ~MechaSquid();															// destructor 0x40f6d0, deleting thunk 0x40f650

	virtual Projectile*		CreateProjectile() override;								// Enemy part slot 6, 0x40f7a0
	virtual void			Update() override;											// GameObject slot 1, 0x40f6f0
	virtual void			Draw(Graphics* g) override;									// slot 2, 0x4046a0
};

}

#endif
