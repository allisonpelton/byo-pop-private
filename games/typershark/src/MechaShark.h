#ifndef __TYPERSHARK_MECHASHARK_H__
#define __TYPERSHARK_MECHASHARK_H__

// MechaShark.cpp: the mechanical shark boss. A TorpedoBoss with other pictures and numbers that fires MechaTorpedos from
// three places; its fin takes the place of the submarine's rotor and goes through 10 frames of its own.
// Original: class Sexy::MechaShark : TorpedoBoss, ProjectileBoss, Enemy, GameObject (all virtual bases), the Enemy part
// at +0 (vftable 0x529d78, 7 virtuals: only CreateProjectile 0x40f0a0 differs from TorpedoBoss's), vbptr at +4, the
// GameObject part at +0x1b8 (vftable 0x529d44, 0xf0 bytes, like TorpedoBoss's); the constructor is 0x40ebe0 (its last
// argument says whether it builds the virtual bases itself), the destructor of the part 0x40ef10 (a jump to
// TorpedoBoss's 0x408d00). It has no fields of its own. The offsets in the field comments of the parent classes are
// the original's.
//
// port: the single chain GameObject, Enemy, ProjectileBoss, TorpedoBoss, MechaShark, so no virtual inheritance. The
// vtordisp thunks (GameObject slots 0 0x40eeb0 (to the deleting destructor 0x40eed0), 2 0x40eec0, 7 0x40f880) only adjust
// the pointer; slots 1, 3 and 4 are TorpedoBoss's.

#include "TorpedoBoss.h"

namespace Sexy
{

class MechaShark : public TorpedoBoss
{
public:
	MechaShark(WinWSApp* theApp, GameObject* thePlayer, LessonManager* theConfig);	// 0x40ebe0
	virtual ~MechaShark();															// destructor 0x40ef10, deleting thunk 0x40eeb0

	virtual Projectile*		CreateProjectile() override;								// Enemy part slot 6, 0x40f0a0
	virtual void			Draw(Graphics* g) override;									// GameObject slot 2, 0x404460
	virtual void			SetState(int theState) override;							// slot 7, 0x40ef20
};

}

#endif
