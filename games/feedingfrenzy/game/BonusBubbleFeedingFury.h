#ifndef __FEEDINGFRENZY_BONUSBUBBLEFEEDINGFURY_H__
#define __FEEDINGFRENZY_BONUSBUBBLEFEEDINGFURY_H__

// The bubble that gives the player the feeding fury (BonusBubbleFeedingFury, 0x39c bytes, constructor 0x419980, vtable
// 0x4a3760, destructor 0x4199e0): a bonus bubble with png pictures.

#include "BonusBubble2X.h"

namespace feedingfrenzy
{

class BonusBubbleFeedingFury : public BonusBubble2X
{
public:
	BonusBubbleFeedingFury();  // 0x419980
};

} // namespace feedingfrenzy

#endif
