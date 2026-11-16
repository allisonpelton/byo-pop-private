#include "BrineShrimp.h"
#include "Random.h"

using namespace engine;

namespace feedingfrenzy
{

// The speeds, sight and size the fish starts with (the fish's file sets its own)
const float START_MAX_SPEED_X_LOW = 40.0f;
const float START_MAX_SPEED_X_HIGH = 80.0f;
const float START_MAX_SPEED_Y = 50.0f;
const float START_VISION_RANGE = 65.0f;
const float START_VISION_ANGLE = 60.0f;
const float START_SPEED_UP = 400.0f;
const float START_SLOW_DOWN = 70.0f;
const float START_SIZE = 0.75f;
const float START_LEVEL = 0.0f;
const float START_POINTS = 10.0f;

// 0x405d00
BrineShrimp::BrineShrimp()
{
	mMaxSpeedX = randomFloat(Point(START_MAX_SPEED_X_LOW, START_MAX_SPEED_X_HIGH));
	mMaxSpeedY = START_MAX_SPEED_Y;
	FishBase::setVision(START_VISION_RANGE, START_VISION_ANGLE);
	mSpeedUp = START_SPEED_UP;
	mSlowDown = START_SLOW_DOWN;
	mSize = START_SIZE;
	mLevel = START_LEVEL;
	mPoints = START_POINTS;
}

// 0x405e10
void BrineShrimp::loadAnimationsOf(const std::string& theFolder)
{
	std::string aSwim = theFolder + "swim\\swim_cycle.";
	std::string aTurn = theFolder + "turn\\turn_cycle.";
	loadAnimations(aSwim.c_str(), aTurn.c_str(), nullptr, nullptr);
}

// 0x405cb0
void BrineShrimp::decide()
{
	if (mSchool != nullptr)
		beginSchool();
	else
		beginWander();
}

} // namespace feedingfrenzy
