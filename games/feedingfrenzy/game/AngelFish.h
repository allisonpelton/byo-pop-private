#ifndef __FEEDINGFRENZY_ANGELFISH_H__
#define __FEEDINGFRENZY_ANGELFISH_H__

// A fish that wanders and cruises (AngelFish, 0x38c bytes, constructor 0x404c60, vtable 0x49e6a8, destructor 0x404b80).
// Its body rectangle is its picture's, taken in at the edges.

#include "AIFish.h"

namespace feedingfrenzy
{

class AngelFish : public AIFish
{
public:
	AngelFish();  // 0x404c60

	// slot 0x220: 0x404bb0: the body rectangle is the picture's with its edges taken in, and the size applied; the hit
	// rectangles stay as the fish's file sets them
	void updateRects() override;
	// slot 0x284: 0x404c30: it wanders or cruises, by chance
	void decide() override;
};

} // namespace feedingfrenzy

#endif
