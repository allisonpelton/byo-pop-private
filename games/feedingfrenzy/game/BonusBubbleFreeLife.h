#ifndef __FEEDINGFRENZY_BONUSBUBBLEFREELIFE_H__
#define __FEEDINGFRENZY_BONUSBUBBLEFREELIFE_H__

// The bubble that gives the player a fish (BonusBubbleFreeLife, 0x39c bytes, constructor 0x419a60, vtable 0x4a3a40,
// destructor 0x419ac0): a bonus bubble with png pictures. The timed game has no fish to give, so the bubble is out of
// play as soon as it is in the stage.

#include "BonusBubble2X.h"

namespace feedingfrenzy
{

class BonusBubbleFreeLife : public BonusBubble2X
{
public:
	BonusBubbleFreeLife();  // 0x419a60

	// slot 0x180: 0x419a30
	void onEnterStage() override;
};

} // namespace feedingfrenzy

#endif
