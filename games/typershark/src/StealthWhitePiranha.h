#ifndef __TYPERSHARK_STEALTHWHITEPIRANHA_H__
#define __TYPERSHARK_STEALTHWHITEPIRANHA_H__

// StealthWhitePiranha.cpp: a white piranha that starts stealthy. Everything is inherited: the stealth behaviour from
// StealthPiranha (Update, Draw, SetVelX, GetSpeed), the second word from WhitePiranha (TypeLetter, IsSwimming); only
// SetState is its own, since both bases override it.
// Original: class Sexy::StealthWhitePiranha : StealthPiranha, WhitePiranha (both with the virtual bases Piranha, Enemy
// and GameObject), vftable 0x52a140 (StealthPiranha's own part at +0, 2 virtuals), 0x52a14c (WhitePiranha's part at
// +0x30, 1 virtual), 0x52a10c (GameObject part at +0x60), 0x52a0fc (Enemy part at +0x158), 0x52a0f4 (Piranha part at
// +0x248), sizeof 0x278; the constructor is 0x411f50, the deleting destructor 0x412200.

#include "StealthPiranha.h"
#include "WhitePiranha.h"

namespace Sexy
{

class StealthWhitePiranha : public StealthPiranha, public WhitePiranha
{
public:
	StealthWhitePiranha(WinWSApp* theApp, int theType);								// 0x411f50
	virtual ~StealthWhitePiranha();													// deleting destructor 0x412200

	virtual void			SetState(int theState) override;						// GameObject slot 7, 0x406a20
};

}

#endif
