#ifndef __FEEDINGFRENZY_FLYBYFISH_H__
#define __FEEDINGFRENZY_FLYBYFISH_H__

// A fish that crosses the stage from the edge it came in at, never turning, and swims faster at the player when the
// player touches it (FlyByFish, 0x394 bytes, constructor 0x408a50, vtable 0x49fb00, destructor 0x4088c0).

#include "AIFish.h"

namespace feedingfrenzy
{

class FlyByFish : public AIFish
{
public:
	FlyByFish();  // 0x408a50

	// slot 0x7c: 0x408910: the time it waits to be touched again runs down
	void update(engine::UpdateInfo& theInfo) override;
	// slot 0x148: 0x408940: a larger eater eats it; any other makes it turn on the eater
	bool finishBeingEaten(FishBase* theEater) override;
	// slot 0x14c: 0x408ae0: the player touches it and it swims faster, with a sound
	void onTouched(FishBase* theFish) override;
	// slot 0x180: 0x408d40: it swims to the right when it came in at the left half
	void onEnterStage() override;
	// slot 0x284: 0x4088f0
	void decide() override { beginCruise(); }
	// slot 0x28c: 0x408990: it swims across, and picks a depth
	void beginCruise() override;

private:
	bool mSwimsRight;  // +0x388
};

} // namespace feedingfrenzy

#endif
