#ifndef __TYPERSHARK_BLACKSHARK_H__
#define __TYPERSHARK_BLACKSHARK_H__

// BlackShark.cpp: a shark with a second word. When its first word is typed it takes the second one (mWord2) and
// swims on, so it has to be typed twice. RedShark derives from it (a third word), StealthBlackShark takes it
// together with StealthShark.
// Original: class Sexy::BlackShark : Shark (virtual base; so are Enemy and GameObject), vftable 0x52979c (own part,
// 1 virtual), 0x529768 (GameObject part at +0x28), 0x529758 (Enemy part at +0x120), 0x529750 (the Shark part at
// +0x210), sizeof 0x240; the constructor is 0x410f50, the deleting destructor 0x409d70. The field comments give the
// offsets in the original's own part (vptr at +0, vbptr at +4).
//
// port: Shark is a virtual base here, as in the original, because StealthBlackShark reaches it twice. The most
// derived class constructs Shark, so every class below BlackShark names Shark(theApp) in its initialiser list.

#include "Shark.h"
#include <string>

namespace Sexy
{

class BlackShark : public virtual Shark
{
public:
	std::string				mWord2;					// +0x08 the word taken on when mWord is finished; the Board fills it (nothing here does). The original has an accessor (own slot 0, 0x407030, a copy) that no play reaches: it reads the field, as the port does

public:
	BlackShark(WinWSApp* theApp);													// 0x410f50
	virtual ~BlackShark();															// deleting destructor 0x409d70

	virtual bool			IsSwimming() override;									// Shark part slot 0, 0x4035f0
	virtual void			SetState(int theState) override;						// GameObject slot 7, 0x4065d0
	virtual void			TypeLetter() override;									// Enemy slot 2, 0x409fb0
};

}

#endif
