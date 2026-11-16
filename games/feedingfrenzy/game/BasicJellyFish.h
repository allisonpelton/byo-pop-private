#ifndef __FEEDINGFRENZY_BASICJELLYFISH_H__
#define __FEEDINGFRENZY_BASICJELLYFISH_H__

// The jellyfish (BasicJellyFish, 0x390 bytes, constructor 0x405800, vtable 0x49ecb0, destructor 0x405550): it pulses
// up the stage in bursts, nobody eats it, and it stuns the fish it touches.

#include "AIFish.h"

namespace feedingfrenzy
{

class BasicJellyFish : public AIFish
{
public:
	BasicJellyFish();  // 0x405800

	// slot 0xe8: 0x405b70: one picture sequence, "jelly.", that plays once
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0x148: 0x419740: nothing eats it
	bool finishBeingEaten(FishBase*) override { return false; }
	// slot 0x160: 0x416820
	void eat(FishBase*) override {}
	// slot 0x17c: 0x4056d0: it faces a side by chance, is not quite opaque, and has a level no fish reaches
	void putInPlay() override;
	// slot 0x1e4: 0x405780: a fish of another level that can be stunned is stunned
	void onContact(FishBase* theFish) override;
	// slot 0x230: 0x416820: its picture plays only when its pulse says
	void updateAnimation() override {}
	// slot 0x248: 0x405580
	void updateEnter(engine::UpdateInfo& theInfo) override;
	// slot 0x260: 0x4055a0: a pulse: the picture plays, the jelly swims up for a time, and it rests; it is out of play
	// when it is above the stage
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slot 0x284: 0x409840 and slot 0x288: 0x409840
	void decide() override { beginSpecial(); }
	void beginWander() override { beginSpecial(); }
	// slot 0x2a4: 0x40dcf0
	void beginLeave() override {}
};

} // namespace feedingfrenzy

#endif
