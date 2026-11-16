#include "AngelFish.h"
#include "Random.h"

using namespace engine;

namespace feedingfrenzy
{

// The level, speeds, size and points the fish starts with (the fish's file sets its own)
const float ANGEL_LEVEL = 2.0f;
const float ANGEL_MAX_SPEED_X_LOW = 80.0f;
const float ANGEL_MAX_SPEED_X_HIGH = 110.0f;
const float ANGEL_MAX_SPEED_Y = 50.0f;
const float ANGEL_SPEED_UP = 180.0f;
const float ANGEL_SLOW_DOWN = 40.0f;
const float ANGEL_SIZE = 0.85f;
const float ANGEL_POINTS = 100.0f;
// How far the body rectangle is taken in from each edge of the picture (the sum and the product are one calculation
// in double)
const double BODY_INSET = 8.0;
// A fish that decides wanders when a number from 0 to 10 is below this, else it cruises
const int WANDER_CHANCE_OF_TEN = 5;

// 0x404c60
AngelFish::AngelFish()
{
	mLevel = ANGEL_LEVEL;
	mMaxSpeedX = randomFloat(Point(ANGEL_MAX_SPEED_X_LOW, ANGEL_MAX_SPEED_X_HIGH));
	mMaxSpeedY = ANGEL_MAX_SPEED_Y;
	mSpeedUp = ANGEL_SPEED_UP;
	mSlowDown = ANGEL_SLOW_DOWN;
	mSize = ANGEL_SIZE;
	mPoints = ANGEL_POINTS;
}

// 0x404bb0
void AngelFish::updateRects()
{
	mBounds = *mAnimations->getSelected()->getBounds();
	mBounds.left = (mBounds.left + BODY_INSET) * mSize;
	mBounds.right = (mBounds.right - BODY_INSET) * mSize;
	mBounds.top = (mBounds.top + BODY_INSET) * mSize;
	mBounds.bottom = (mBounds.bottom - BODY_INSET) * mSize;
}

// 0x404c30
void AngelFish::decide()
{
	if (randomInt(0, 10) < WANDER_CHANCE_OF_TEN)
		beginWander();
	else
		beginCruise();
}

} // namespace feedingfrenzy
