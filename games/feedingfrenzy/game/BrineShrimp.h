#ifndef __FEEDINGFRENZY_BRINESHRIMP_H__
#define __FEEDINGFRENZY_BRINESHRIMP_H__

// The smallest fish (BrineShrimp, 0x38c bytes, constructor 0x405d00, vtable 0x49efa8, destructor 0x405c80). It swims in
// schools, is eaten by every other fish and eats nothing, and its pictures are a swim cycle and a turn cycle only.

#include "AIFish.h"

namespace feedingfrenzy
{

class BrineShrimp : public AIFish
{
public:
	BrineShrimp();  // 0x405d00

	// slot 0xe8: 0x405e10: the swim cycle and the turn cycle, no eating pictures
	void loadAnimationsOf(const std::string& theFolder) override;
	// slot 0x138: 0x419740
	bool canEat(FishBase*) override { return false; }
	// slot 0x284: 0x405cb0: it swims with its school, or else wanders
	void decide() override;
};

} // namespace feedingfrenzy

#endif
