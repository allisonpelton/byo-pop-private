#ifndef __FEEDINGFRENZY_SUICIDEFISH_H__
#define __FEEDINGFRENZY_SUICIDEFISH_H__

// A fish that swims straight at the player (SuicideFish, 0x38c bytes, constructor 0x40d980, vtable 0x4a1930,
// destructor 0x40d860).

#include "AIFish.h"

namespace feedingfrenzy
{

class SuicideFish : public AIFish
{
public:
	SuicideFish();  // 0x40d980

	// slot 0xe8: 0x40da90: the swim and turn cycles, and the eat cycle when the fish's level is above a tenth
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0x260: 0x40d8b0: the fish's wish is the player's nose, at its greatest speed
	void updateSpecial(engine::UpdateInfo& theInfo) override;
	// slot 0x284: 0x409840
	void decide() override { beginSpecial(); }
};

} // namespace feedingfrenzy

#endif
