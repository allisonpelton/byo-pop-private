#ifndef __FEEDINGFRENZY_SHARK_H__
#define __FEEDINGFRENZY_SHARK_H__

// The shark that crosses the stage (Shark, 0x394 bytes, constructor 0x40d680, vtable 0x4a1368, destructor 0x40d6d0): a
// FlyByFish that the fish files give their own size and speed.

#include "FlyByFish.h"

namespace feedingfrenzy
{

class Shark : public FlyByFish
{
public:
	Shark() {}  // 0x40d680
};

} // namespace feedingfrenzy

#endif
