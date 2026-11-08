#ifndef __TYPERSHARK_GHOSTSHIP_H__
#define __TYPERSHARK_GHOSTSHIP_H__

// GhostShip.cpp: the ghost galleon boss. A TorpedoBoss that fires cannon balls (CannonBall) with a single letter on
// each, never the same letter as a ball in flight, from three places on its hull, with a rudder that swings back and
// forth. MechaSquid derives from it.
// Original: class Sexy::GhostShip : TorpedoBoss, ProjectileBoss, Enemy, GameObject (all virtual bases), the Enemy part
// at +0 (vftable 0x529ca8, 7 virtuals: GetWord 0x408060 and CreateProjectile 0x40e9a0 replace TorpedoBoss's), vbptr at
// +4, the GameObject part at +0x1d0 (vftable 0x529c74, 0xf0 bytes); the constructor is 0x40e600 (its last argument says
// whether it builds the virtual bases itself), the destructor of the part 0x409920 (a jump to TorpedoBoss's 0x408d00).
// The field comments give the offsets in the original's object; TorpedoBoss's fields end at +0x1b0.
//
// port: the single chain GameObject, Enemy, ProjectileBoss, TorpedoBoss, GhostShip, with MechaSquid after it, so no
// virtual inheritance. The vtordisp thunks (GameObject slots 0 0x40e8a0, 1 0x40f890, 2 0x40e8d0, 3 and 4 0x40e8b0 and
// 0x40e8c0, 7 0x40e920 to TorpedoBoss::SetState) only adjust the pointer, so SetState is not overridden here.

#include "TorpedoBoss.h"

namespace Sexy
{

class GhostShip : public TorpedoBoss
{
public:
	bool					mRudderRising;			// +0x1b0 ctor 1: the rudder's frame counts up (1) or down (0)
	int						mRudderFrame;			// +0x1b4 ctor 0: the frame of the rudder picture (IMAGE_GALLEON_RUDDER, 0 to 4); Update moves it every 13th tick
	int						mRudderHeight;			// +0x1b8 ctor 0x2f: the height of a frame of the rudder
	int						mRudderWidth;			// +0x1bc ctor 0x15: the width of a frame, the frames lie side by side
	int						mRudderFrames;			// +0x1c0 ctor 5

public:
	GhostShip(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig);	// 0x40e600
	virtual ~GhostShip();															// destructor 0x409920, deleting thunk 0x40e8a0

	virtual std::string		GetWord(int theMinLength, int theMaxLength) override;		// Enemy part slot 5, 0x408060
	virtual Projectile*		CreateProjectile() override;								// Enemy part slot 6, 0x40e9a0
	virtual void			Update() override;											// GameObject slot 1, 0x40e930
	virtual void			Draw(Graphics* g) override;									// slot 2, 0x404210
};

}

#endif
