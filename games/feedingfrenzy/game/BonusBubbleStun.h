#ifndef __FEEDINGFRENZY_BONUSBUBBLESTUN_H__
#define __FEEDINGFRENZY_BONUSBUBBLESTUN_H__

// The bubble that stuns the fish on the screen (BonusBubbleStun, 0x39c bytes, constructor 0x41a180, vtable 0x4a4608,
// destructor 0x41a1e0): a bonus bubble with png pictures.

#include "BonusBubble2X.h"

namespace feedingfrenzy
{

class BonusBubbleStun : public BonusBubble2X
{
public:
	BonusBubbleStun();  // 0x41a180
};

} // namespace feedingfrenzy

#endif
