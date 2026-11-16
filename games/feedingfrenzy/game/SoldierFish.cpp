#include "SoldierFish.h"
#include "Random.h"

using namespace engine;

namespace feedingfrenzy
{

// The speed the fish slows down at
const float SOLDIER_SLOW_DOWN = 70.0f;
// How likely a soldier fish that is not in a school wanders when it decides (else it cruises)
const float WANDER_CHANCE = 0.4f;

// 0x40d7e0
SoldierFish::SoldierFish()
{
	mSlowDown = SOLDIER_SLOW_DOWN;
}

// 0x40d720
void SoldierFish::decide()
{
	float aRoll = randomFloat(Point(0.0f, 1.0f));
	if (mSchool != nullptr)
		beginSchool();
	else if (aRoll < WANDER_CHANCE)
		beginWander();
	else
		beginCruise();
}

} // namespace feedingfrenzy
