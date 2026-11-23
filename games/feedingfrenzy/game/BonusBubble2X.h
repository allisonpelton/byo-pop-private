#ifndef __FEEDINGFRENZY_BONUSBUBBLE2X_H__
#define __FEEDINGFRENZY_BONUSBUBBLE2X_H__

// A bonus bubble (BonusBubble2X, 0x39c bytes, constructor 0x4197e0, vtable 0x4a3480, destructor 0x419520): the double
// points bubble, and the base of the other bonus bubbles. It rises up the stage, only the player eats it (touching it
// is enough), and nobody stuns it or reacts to it. Where it comes from it can grow from nothing, as it does when the
// mermaid drops it.

#include "AIFish.h"

namespace feedingfrenzy
{

class BonusBubble2X : public AIFish
{
public:
	BonusBubble2X();  // 0x4197e0

	// slot 0xe8: 0x419890: one picture sequence, the folder's
	void loadAnimationsOf(const std::string& theFolder) override;
	// slots 0x138 and 0x13c: 0x419740: a bubble eats nothing and no level eats it
	bool canEat(FishBase*) override { return false; }
	bool canBeEatenByLevel(float) override { return false; }
	// slot 0x140: 0x4196e0: the player, when it is not being eaten already
	bool canBeEatenBy(FishBase* theEater) override;
	// slot 0x17c: 0x419710
	void putInPlay() override;
	// slot 0x1bc: 0x476dd0: nothing stuns it
	void stun(float, FishBase*, bool) override {}
	// slot 0x230: 0x419550: the first picture shows
	void updateAnimation() override;
	// slot 0x248: 0x405580
	void updateEnter(engine::UpdateInfo& theInfo) override;
	// slot 0x260: 0x419570: it is out of play when it has left the stage, up or down; it grows while it is growing
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slots 0x284 and 0x288: 0x409840
	void decide() override { beginSpecial(); }
	void beginWander() override { beginSpecial(); }
	// slot 0x298: 0x4196b0: it rises, unless it is swallowed
	void beginSpecial() override;
	// slot 0x2a4: 0x40dcf0
	void beginLeave() override {}

	// slot 0x2dc: 0x419750: it grows from a tenth of its size to the size given, for the seconds given
	virtual void startGrowing(float theSeconds, float theSize);

private:
	bool mGrowing;           // +0x38c
	float mGrowSecondsLeft;  // +0x390
	float mGrowSeconds;      // +0x394
	float mGrowSize;         // +0x398: the size it grows to
};

} // namespace feedingfrenzy

#endif
