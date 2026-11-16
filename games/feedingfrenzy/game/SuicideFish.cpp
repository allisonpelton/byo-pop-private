#include "SuicideFish.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The speeds, sight, size, level and points the fish starts with (the fish's file sets its own)
const float SUICIDE_MAX_SPEED_X_LOW = 40.0f;
const float SUICIDE_MAX_SPEED_X_HIGH = 80.0f;
const float SUICIDE_MAX_SPEED_Y = 50.0f;
const float SUICIDE_VISION_RANGE = 65.0f;
const float SUICIDE_VISION_ANGLE = 60.0f;
const float SUICIDE_SPEED_UP = 400.0f;
const float SUICIDE_SLOW_DOWN = 70.0f;
const float SUICIDE_SIZE = 0.75f;
const float SUICIDE_LEVEL = 0.0f;
const float SUICIDE_POINTS = 10.0f;
// A fish whose level is above this has eating pictures
const double EATING_LEVEL = 0.1;

// 0x40d980
SuicideFish::SuicideFish()
{
	mMaxSpeedX = randomFloat(Point(SUICIDE_MAX_SPEED_X_LOW, SUICIDE_MAX_SPEED_X_HIGH));
	mMaxSpeedY = SUICIDE_MAX_SPEED_Y;
	FishBase::setVision(SUICIDE_VISION_RANGE, SUICIDE_VISION_ANGLE);
	mSpeedUp = SUICIDE_SPEED_UP;
	mSlowDown = SUICIDE_SLOW_DOWN;
	mSize = SUICIDE_SIZE;
	mLevel = SUICIDE_LEVEL;
	mPoints = SUICIDE_POINTS;
}

// 0x40da90
void SuicideFish::loadAnimationsOf(const std::string& theFolder)
{
	std::string aSwim = theFolder + "swim\\swim_cycle.";
	std::string aTurn = theFolder + "turn\\turn_cycle.";
	std::string anEat = theFolder + "eat\\eat_cycle.";
	if (mLevel > EATING_LEVEL)
		loadAnimations(aSwim.c_str(), aTurn.c_str(), anEat.c_str(), nullptr);
	else
		loadAnimations(aSwim.c_str(), aTurn.c_str(), nullptr, nullptr);
}

// 0x40d8b0
void SuicideFish::updateSpecial(UpdateInfo& theInfo)
{
	Point aHeading;
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	aHeading = aPlayer->getNosePosition();
	aHeading.subtract(mNose);
	aHeading.normalize();
	aHeading.scale(mMaxSpeedX);
	mDesiredVelocity.y = aHeading.y;
	mDesiredVelocity.x = aHeading.x;
	FishBase::update(theInfo);
}

} // namespace feedingfrenzy
