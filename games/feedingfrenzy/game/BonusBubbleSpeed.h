#ifndef __FEEDINGFRENZY_BONUSBUBBLESPEED_H__
#define __FEEDINGFRENZY_BONUSBUBBLESPEED_H__

// The bubble that makes the player's fish faster (BonusBubbleSpeed, 0x39c bytes, constructor 0x419d30, vtable 0x4a4020,
// destructor 0x419d90): a bonus bubble with jpeg pictures.

#include "BonusBubble2X.h"

namespace feedingfrenzy
{

class BonusBubbleSpeed : public BonusBubble2X
{
public:
	BonusBubbleSpeed();  // 0x419d30
};

} // namespace feedingfrenzy

#endif
