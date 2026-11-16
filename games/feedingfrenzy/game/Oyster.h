#ifndef __FEEDINGFRENZY_OYSTER_H__
#define __FEEDINGFRENZY_OYSTER_H__

// The oyster (Oyster, constructor 0x40af30, vtable 0x4a0778): a fish that sits and opens its shell, with a pearl in it
// that the player may take.

#include "AIFish.h"

namespace feedingfrenzy
{

// What the pearl in an oyster is (0x40a470)
enum PearlKind
{
	PEARL_WHITE = 0,  // it scores
	PEARL_BLACK = 1,  // it scores, with a message
	PEARL_LIFE = 2    // it gives an extra fish
};

class Oyster : public AIFish
{
public:
	// slot 0x2dc: 0x40a470
	int getPearlKind() { return mPearlKind; }

private:
	int mPearlKind;
};

} // namespace feedingfrenzy

#endif
