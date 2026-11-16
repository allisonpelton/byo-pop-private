#ifndef __FEEDINGFRENZY_BARRACUDA_H__
#define __FEEDINGFRENZY_BARRACUDA_H__

// A huge fish that crosses the stage (Barracuda, 0x398 bytes, constructor 0x4050c0, vtable 0x49e990, destructor
// 0x404d60). A blinking danger sign at the edge it will come in at warns the player first. A fish that is not larger
// can only touch it, which makes it swim at the touching fish faster; the player bites its tail instead.

#include "AIFish.h"
#include "Blinker.h"

namespace feedingfrenzy
{

class Barracuda : public AIFish
{
public:
	Barracuda();  // 0x4050c0

	// slot 0x7c: 0x404fc0: the time it waits to be touched again runs down, and the danger sign stops when it has
	// come in
	void update(engine::UpdateInfo& theInfo) override;
	// slot 0x144: 0x405030: a fish that is not larger touches it; a larger one starts to eat it
	void startBeingEaten(FishBase* theEater, float theSeconds) override;
	// slot 0x148: 0x405430: the player that is not larger bites its tail (a message and points); a larger fish eats it
	bool finishBeingEaten(FishBase* theEater) override;
	// slot 0x14c: 0x405390: the player touches it and it swims faster at the player, with a sound
	void onTouched(FishBase* theFish) override;
	// slot 0x17c: 0x404d90: it waits for its warning, and the danger sign is made
	void putInPlay() override;
	// slot 0x180: 0x405250: the danger sign shows at the edge it comes in at, with a sound
	void onEnterStage() override;
	// slot 0x188: 0x404e60: the danger sign stops
	void takeOutOfPlay() override;
	// slot 0x1bc: 0x405160: it swims away from the one that stunned it
	void stun(float theSeconds, FishBase* theSource, bool theShowEffect) override;
	// slot 0x248: 0x404e80: it holds still while it warns, then swims in; the danger sign follows its depth
	void updateEnter(engine::UpdateInfo& theInfo) override;
	// slot 0x284: 0x4088f0
	void decide() override { beginCruise(); }

private:
	float mTouchedSeconds;             // +0x388: the seconds left before it can be touched again
	engine::Ref<Blinker> mDangerSign;  // +0x38c
};

} // namespace feedingfrenzy

#endif
