#ifndef __TYPERSHARK_MECHATORPEDO_H__
#define __TYPERSHARK_MECHATORPEDO_H__

// MechaTorpedo.cpp: the torpedo that MechaShark fires: a Torpedo with another hit box (HitsTarget); the pictures,
// Update, Draw, SetState and TypeLetter are Torpedo's.
// Original: class Sexy::MechaTorpedo : Torpedo, Projectile, Enemy, GameObject (all virtual bases), own part of 8 bytes,
// the GameObject part at +8 (vftable 0x529cec), the Enemy part at +0x100 (0x529cdc), the Projectile part at +0x1f0
// (0x529cd0), the Torpedo part at +0x210, sizeof 0x218; the constructor is 0x40ea50 (it only constructs Torpedo; the
// export cuts it in two at 0x40ea73), the deleting destructor 0x40ebb0 (shared with CannonBall).

#include "Torpedo.h"

namespace Sexy
{

class MechaTorpedo : public Torpedo
{
public:
	MechaTorpedo(WinWSApp* theApp, GameObject* theHitTarget, GameObject* thePlayer);	// 0x40ea50
	virtual ~MechaTorpedo();															// deleting destructor 0x40ebb0

	virtual bool			HitsTarget() override;										// Projectile slot 1, 0x401430
};

}

#endif
