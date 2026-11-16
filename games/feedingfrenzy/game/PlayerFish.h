#ifndef __FEEDINGFRENZY_PLAYERFISH_H__
#define __FEEDINGFRENZY_PLAYERFISH_H__

// The player's fish (PlayerFish, constructor 0x414840, vtable 0x4a2910).

#include "FishBase.h"

namespace feedingfrenzy
{

class PlayerFish : public FishBase
{
public:
	virtual void onSchoolEaten() {}
};

} // namespace feedingfrenzy

#endif
