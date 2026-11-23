#ifndef __FEEDINGFRENZY_MERMAID_H__
#define __FEEDINGFRENZY_MERMAID_H__

// The mermaid (Mermaid, 0x3a0 bytes, constructor 0x4094f0, vtable 0x4a00e0, destructor 0x4091f0): a fish that crosses
// the stage once, with sparkles, and lets bonus bubbles fall from where it is; the player eats the bubbles, never the
// mermaid. The original's object has a vector of three words at +0x388 that is cleared and never used.

#include "AIFish.h"
#include "FishSpawner.h"
#include "Particles.h"

namespace feedingfrenzy
{

class Mermaid : public AIFish
{
public:
	Mermaid();           // 0x4094f0
	~Mermaid() override;  // 0x4091f0: the spawner of the bubbles goes

	// slot 0xe8: 0x409430: the swim cycle only
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0xec: 0x409300: the spawner of the bubbles that fall (it makes no fish on its own) and the sparkles
	void loadEffects() override;
	// slots 0x138 and 0x140: 0x419740: nobody eats the mermaid, and it eats nothing
	bool canEat(FishBase*) override { return false; }
	bool canBeEatenBy(FishBase*) override { return false; }
	// slot 0x188: 0x4091c0: the sparkles stop
	void takeOutOfPlay() override;
	// slot 0x1c0: 0x4091e0: the bonus bubbles of the stage that were eaten
	int getEatCount() override { return mBonusSpawner->getBonusEatenCount(); }
	// slot 0x260: 0x408e90: when it is in the stage, every half second it lets a bubble fall, while it is not near the
	// side it swims to; when it has left the stage and its bubbles are done it is out of play
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slot 0x284: 0x409840
	void decide() override { beginSpecial(); }
	// slot 0x298: 0x4090e0: it swims across the stage, from the side it is on, with its sparkles in front of it
	void beginSpecial() override;
	// slot 0x2a4: 0x40dcf0
	void beginLeave() override {}

private:
	engine::ParticleSystem* mSparkles;  // +0x398 (the fish's child)
	FishSpawner* mBonusSpawner;         // +0x39c: the falling bonus bubbles
};

} // namespace feedingfrenzy

#endif
