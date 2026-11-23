#include "PoisonMinnow.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The speeds, sight, size, level, points and colour the fish starts with (the fish's file sets its own)
const float POISON_MAX_SPEED_X_LOW = 40.0f;
const float POISON_MAX_SPEED_X_HIGH = 80.0f;
const float POISON_MAX_SPEED_Y = 50.0f;
const float POISON_VISION_RANGE = 65.0f;
const float POISON_VISION_ANGLE = 60.0f;
const float POISON_SPEED_UP = 400.0f;
const float POISON_SLOW_DOWN = 70.0f;
const float POISON_SIZE = 0.75f;
const float POISON_LEVEL = 0.0f;
const float POISON_POINTS = 10.0f;
const float POISON_COLOR_RED = -0.5f;
const float POISON_COLOR_GREEN = 0.1f;
const float POISON_COLOR_BLUE = -0.5f;
// The trail's file
const char* const POISON_TRAIL_FILE = "config\\fx\\poisonTrailFx.xml";
// The seconds the player stays poisoned for
const float POISON_SECONDS = 5.5f;

// 0x40c460
PoisonMinnow::PoisonMinnow() : mTrail(nullptr)
{
	mMaxSpeedX = randomFloat(Point(POISON_MAX_SPEED_X_LOW, POISON_MAX_SPEED_X_HIGH));
	mMaxSpeedY = POISON_MAX_SPEED_Y;
	FishBase::setVision(POISON_VISION_RANGE, POISON_VISION_ANGLE);
	mSpeedUp = POISON_SPEED_UP;
	mSlowDown = POISON_SLOW_DOWN;
	mSize = POISON_SIZE;
	mLevel = POISON_LEVEL;
	mPoints = POISON_POINTS;
	mColorOffset.set(POISON_COLOR_RED, POISON_COLOR_GREEN, POISON_COLOR_BLUE);
	mHidden = true;
}

// 0x40c590
void PoisonMinnow::loadEffects()
{
	mTrail = new ParticleSystem();
	addChild(mTrail);
	mTrail->load(POISON_TRAIL_FILE);
	mTrail->setScale(1.0 / mSize, 1.0 / mSize);
	AIFish::loadEffects();
}

// 0x40c410
bool PoisonMinnow::finishBeingEaten(FishBase* theEater)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (theEater == aPlayer)
	{
		if (aPlayer->getState() == PLAYER_FURY)
			return false;
		aPlayer->poison(POISON_SECONDS);
	}
	return AIFish::finishBeingEaten(theEater);
}

// 0x408bb0
void PoisonMinnow::putInPlay()
{
	mTrail->startEmitting();
	AIFish::putInPlay();
}

// 0x408bd0
void PoisonMinnow::takeOutOfPlay()
{
	mTrail->stopEmitting();
	AIFish::takeOutOfPlay();
}

// 0x405cb0
void PoisonMinnow::decide()
{
	if (mSchool != nullptr)
		beginSchool();
	else
		beginWander();
}

} // namespace feedingfrenzy
