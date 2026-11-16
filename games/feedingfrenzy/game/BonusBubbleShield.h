#ifndef __FEEDINGFRENZY_BONUSBUBBLESHIELD_H__
#define __FEEDINGFRENZY_BONUSBUBBLESHIELD_H__

// The bubble that gives the player's fish a shield (BonusBubbleShield, 0x3a0 bytes, constructor 0x419b90, vtable
// 0x4a3d20, destructor 0x419bf0): a bonus bubble with png pictures and a sparkle that stays with it.

#include "BonusBubble2X.h"
#include "FishAttachFx.h"

namespace feedingfrenzy
{

class BonusBubbleShield : public BonusBubble2X
{
public:
	BonusBubbleShield();  // 0x419b90

	// slot 0xec: 0x419c10: the sparkle (the stun effect and the thought bubble of other fish are not made)
	void loadEffects() override;
	// slot 0x188: 0x419b70: the sparkle stops
	void takeOutOfPlay() override;
	// slot 0x248: 0x419e60: the sparkle starts
	void updateEnter(engine::UpdateInfo& theInfo) override;

private:
	engine::Ref<FishAttachFx> mSparkle;  // +0x39c
};

} // namespace feedingfrenzy

#endif
