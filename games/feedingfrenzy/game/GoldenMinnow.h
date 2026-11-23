#ifndef __FEEDINGFRENZY_GOLDENMINNOW_H__
#define __FEEDINGFRENZY_GOLDENMINNOW_H__

// The golden minnow (GoldenMinnow, 0x394 bytes, constructor 0x408c30, vtable 0x49fdd8, destructor 0x408b70): a small
// fish that leaves a trail, wanders and flees, and that only the player can eat.

#include "AIFish.h"
#include "Particles.h"

namespace feedingfrenzy
{

class GoldenMinnow : public AIFish
{
public:
	GoldenMinnow();  // 0x408c30

	// slot 0xec: 0x408d80: the trail goes behind the pictures, then the stun effect and the thought bubble's picture
	void loadEffects() override;
	// slot 0x140: 0x408c00: only the player can eat it
	bool canBeEatenBy(FishBase* theEater) override;
	// slot 0x17c: 0x408bb0: the trail starts
	void putInPlay() override;
	// slot 0x180: 0x408d40
	void onEnterStage() override;
	// slot 0x188: 0x408bd0: the trail stops
	void takeOutOfPlay() override;
	// slot 0x284: 0x408bf0
	void decide() override { beginWander(); }

private:
	engine::ParticleSystem* mTrail;  // +0x384 (the fish's child)
	bool mSwimsRight;                // +0x388: it came in at the left half; nothing reads it
};

} // namespace feedingfrenzy

#endif
