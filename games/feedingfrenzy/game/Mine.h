#ifndef __FEEDINGFRENZY_MINE_H__
#define __FEEDINGFRENZY_MINE_H__

// The sea mine (Mine, 0x3b0 bytes, constructor 0x409c30, vtable 0x4a03f8, destructor 0x4095b0): it sinks to a depth and
// bobs there, and stings what touches it. The player's stun sets it off, and so does a fish that touches it (the
// jellyfish, the other mines and the bonus bubbles excepted); the fish it touches is eaten by it. When the stage is
// left the mines that are in play go off, one after another. The original's object has a flag at +0x3a8 that is set when
// the effects are made and never read, and a second bobbing rate and time (+0x38c and +0x390) that nothing reads.

#include "AIFish.h"
#include "AnimImage.h"
#include "Particles.h"

namespace feedingfrenzy
{

class Mine : public AIFish
{
public:
	Mine();  // 0x409c30

	// 0x409be0: the mines that leave count from the first again
	static void resetLeaveCount() { sLeaveCount = 0; }

	// slot 0x7c: 0x409760: a mine that is going off has its own update; the bobbing clocks run
	void update(engine::UpdateInfo& theInfo) override;
	// slot 0xe4: 0x40a020: one picture sequence, always of jpeg files, each centred; the fall and explosion particles
	void loadAnimations(const char* theMinePath, const char* theTurnPath, const char* theEatPath,
	                    const char* theExtraPath) override;
	// slot 0xe8: 0x40a320: the sequence is "mine"
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0xec: 0x409630: the explosion's pictures (the stun effect and the thought bubble of other fish are not made)
	void loadEffects() override;
	// slots 0x138, 0x13c, 0x140 and 0x148: 0x419740: nothing eats a mine and it eats nothing
	bool canEat(FishBase*) override { return false; }
	bool canBeEatenByLevel(float) override { return false; }
	bool canBeEatenBy(FishBase*) override { return false; }
	bool finishBeingEaten(FishBase*) override { return false; }
	// slot 0x160: 0x416820
	void eat(FishBase*) override {}
	// slot 0x17c: 0x409870: the mine picks the depth it sinks to, and the fall particles start
	void putInPlay() override;
	// slot 0x188: 0x40dcf0
	void takeOutOfPlay() override {}
	// slot 0x1bc: 0x409b10: the player's stun sets it off
	void stun(float theSeconds, FishBase* theSource, bool theShowEffect) override;
	// slot 0x1e4: 0x409e80: a fish that touches it is eaten, and the mine goes off
	void onContact(FishBase* theFish) override;
	// slot 0x224: 0x409d90: the hit rectangle as the mine stands in the stage (the one that faces the other way)
	void updatePlacedHitRect() override;
	// slot 0x228: 0x409960: the nose and the tail at the middle of the hit rectangle, and the body rectangle round them
	void updatePlacedRects() override;
	// slot 0x230: 0x419550
	void updateAnimation() override;
	// slot 0x248: 0x4097a0: it sinks until it is at its depth
	void updateEnter(engine::UpdateInfo& theInfo) override;
	// slot 0x260: 0x409800: it bobs; it is out of play when it has left the stage
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slot 0x270: 0x409b90: the time to wait before it goes off runs down
	void updateLeaving(engine::UpdateInfo& theInfo) override;
	// slots 0x284 and 0x288: 0x409840
	void decide() override { beginSpecial(); }
	void beginWander() override { beginSpecial(); }
	// slot 0x298: 0x409850: it holds its depth and the fall particles stop
	void beginSpecial() override;
	// slot 0x2a4: 0x409b50: it goes off after a time that is longer for each mine of the stage that leaves
	void beginLeave() override;

private:
	// slot 0x2d8: 0x4098f0: it is done when the explosion's particles and pictures are
	void updateExploding(engine::UpdateInfo& theInfo);
	// slot 0x2dc: 0x40a260: it goes off, with a shake of the camera and a sound
	void explode();
	// slot 0x2e0: 0x409d60: the vertical wish follows a sine of the time
	void bob();

	static int sLeaveCount;  // 0x4bfd98: the mines that have been told to leave since the stage began

	float mBobRate;                         // +0x388
	float mSwayRate;                        // +0x38c
	float mSwayTime;                        // +0x390
	float mBobTime;                         // +0x394
	bool mExploding;                        // +0x398
	engine::ParticleSystem* mExplosionFx;   // +0x39c (the fish's child)
	engine::Ref<engine::AnimImage> mExplosion;  // +0x3a0
	engine::ParticleSystem* mFallFx;        // +0x3a4 (the fish's child)
	float mLeaveSeconds;                    // +0x3ac
	float mDepth;                           // +0x384: where it stops sinking
};

} // namespace feedingfrenzy

#endif
