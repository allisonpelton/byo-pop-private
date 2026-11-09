#ifndef __TYPERSHARK_STEALTHBLACKSHARK_H__
#define __TYPERSHARK_STEALTHBLACKSHARK_H__

// StealthBlackShark.cpp: a black shark that starts stealthy. Everything is inherited: the stealth behaviour from
// StealthShark (Update, Draw, SetVelX, GetSpeed), the second word from BlackShark (TypeLetter, IsSwimming); only
// SetState is its own, since both bases override it.
// Original: class Sexy::StealthBlackShark : StealthShark, BlackShark (both with the virtual bases Shark, Enemy and
// GameObject), vftable 0x52a05c (StealthShark's own part at +0, 2 virtuals), 0x52a068 (BlackShark's part at +0x30, 1
// virtual), 0x52a028 (GameObject part at +0x60), 0x52a018 (Enemy part at +0x158), 0x52a010 (Shark part at +0x248),
// sizeof 0x278; the constructor is 0x411710, the deleting destructor 0x4119b0.

#include "StealthShark.h"
#include "BlackShark.h"

namespace Sexy
{

class StealthBlackShark : public StealthShark, public BlackShark
{
public:
	StealthBlackShark(WinWSApp* theApp);											// 0x411710
	virtual ~StealthBlackShark();													// deleting destructor 0x4119b0

	virtual void			SetState(int theState) override;						// GameObject slot 7, 0x4068c0
};

}

#endif
