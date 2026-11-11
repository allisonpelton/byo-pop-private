#ifndef __TYPERSHARK_REDSHARK_H__
#define __TYPERSHARK_REDSHARK_H__

// RedShark.cpp: a black shark with a third word: it has to be typed three times. StealthRedShark takes it together
// with StealthShark.
// Original: class Sexy::RedShark : BlackShark (then Shark, Enemy and GameObject as virtual bases), vftable 0x529838
// (own part, 2 virtuals: BlackShark's and its own), 0x529804 (GameObject part at +0x48), 0x5297f4 (Enemy part at
// +0x140), 0x5297ec (Shark part at +0x230), sizeof 0x260; the constructor is 0x411400, the deleting destructor
// 0x409970. The field comments give the offsets in the original's own part (BlackShark's part is +0..+0x24).

#include "BlackShark.h"

namespace Sexy
{

class RedShark : public BlackShark
{
public:
	std::string				mWord3;					// +0x24 the word taken on when mWord2 is finished; the Board fills it. The original has an accessor (own slot 1, 0x407170, a copy) that no play reaches: it reads the field, as the port does

public:
	RedShark(WinWSApp* theApp);														// 0x411400
	virtual ~RedShark();															// deleting destructor 0x409970

	virtual void			TypeLetter() override;									// Enemy slot 2, 0x40b600
};

}

#endif
