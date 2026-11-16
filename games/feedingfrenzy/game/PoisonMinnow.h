#ifndef __FEEDINGFRENZY_POISONMINNOW_H__
#define __FEEDINGFRENZY_POISONMINNOW_H__

// The poison minnow (PoisonMinnow, 0x390 bytes, constructor 0x40c460, vtable 0x4a0d40, destructor 0x40c3e0): a small
// fish that leaves a trail and poisons the player that eats it. The player's hints skip it.

#include "AIFish.h"
#include "Particles.h"

namespace feedingfrenzy
{

class PoisonMinnow : public AIFish
{
public:
	PoisonMinnow();  // 0x40c460

	// slot 0xec: 0x40c590: the trail, then the stun effect and the thought bubble's picture
	void loadEffects() override;
	// slot 0x148: 0x40c410: the player that eats it is poisoned, unless the player is in the feeding fury
	bool finishBeingEaten(FishBase* theEater) override;
	// slot 0x17c: 0x408bb0: the trail starts
	void putInPlay() override;
	// slot 0x188: 0x408bd0: the trail stops
	void takeOutOfPlay() override;
	// slot 0x284: 0x405cb0: it swims with its school, or else wanders
	void decide() override;

private:
	engine::ParticleSystem* mTrail;  // +0x384 (the fish's child)
};

} // namespace feedingfrenzy

#endif
