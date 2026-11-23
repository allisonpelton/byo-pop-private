#ifndef __FEEDINGFRENZY_SOLDIERFISH_H__
#define __FEEDINGFRENZY_SOLDIERFISH_H__

// A fish that keeps to its school and chases smaller fish (SoldierFish, 0x38c bytes, constructor 0x40d7e0, vtable
// 0x4a1658, destructor 0x40d840).

#include "AIFish.h"

namespace feedingfrenzy
{

class SoldierFish : public AIFish
{
public:
	SoldierFish();  // 0x40d7e0

	// slot 0x284: 0x40d720: it swims with its school, or else wanders or cruises
	void decide() override;
};

} // namespace feedingfrenzy

#endif
