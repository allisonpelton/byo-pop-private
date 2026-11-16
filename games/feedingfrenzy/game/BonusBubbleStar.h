#ifndef __FEEDINGFRENZY_BONUSBUBBLESTAR_H__
#define __FEEDINGFRENZY_BONUSBUBBLESTAR_H__

// The star bubble (BonusBubbleStar, 0x3a0 bytes, constructor 0x419e80, vtable 0x4a4300, destructor 0x419f40): a bonus
// bubble that gives points, from ten to fifty at random, and a sparkle that stays with it. In the timed game it is a
// time bubble, with its own pictures.

#include "BonusBubble2X.h"
#include "FishAttachFx.h"

namespace feedingfrenzy
{

class BonusBubbleStar : public BonusBubble2X
{
public:
	BonusBubbleStar();  // 0x419e80

	// slot 0xe8: 0x419f60: the pictures of the time bubble in the timed game, the folder's otherwise
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0xec: 0x41a010: the sparkle (the stun effect and the thought bubble of other fish are not made)
	void loadEffects() override;
	// slot 0xfc: 0x419e40: the fish's file gives the points as a factor of the ones it has
	void setPoints(float theFactor) override;
	// slot 0x188: 0x419b70: the sparkle stops
	void takeOutOfPlay() override;
	// slot 0x248: 0x419e60: the sparkle starts
	void updateEnter(engine::UpdateInfo& theInfo) override;

private:
	engine::Ref<FishAttachFx> mSparkle;  // +0x39c
};

} // namespace feedingfrenzy

#endif
