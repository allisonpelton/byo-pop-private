#include "GoldenMinnow.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The speeds, sight, size, level, points and colour the fish starts with (the fish's file sets its own)
const float GOLDEN_MAX_SPEED_X = 100.0f;
const float GOLDEN_MAX_SPEED_Y = 100.0f;
const float GOLDEN_VISION_RANGE = 20.0f;
const float GOLDEN_VISION_ANGLE = 30.0f;
const float GOLDEN_SPEED_UP = 400.0f;
const float GOLDEN_SLOW_DOWN = 70.0f;
const float GOLDEN_SIZE = 0.75f;
const float GOLDEN_LEVEL = 0.0f;
const float GOLDEN_POINTS = 200.0f;
const float GOLDEN_COLOR_RED = -1.0f;
const float GOLDEN_COLOR_GREEN = 0.0f;
const float GOLDEN_COLOR_BLUE = 0.0f;
// The trail's file
const char* const GOLDEN_TRAIL_FILE = "config\\fx\\goldenTrail.xml";
// A fish that comes in left of this is on the left half of the stage
const float GOLDEN_MIDDLE_X = 320.0f;

// 0x408c30
GoldenMinnow::GoldenMinnow() : mTrail(nullptr), mSwimsRight(false)
{
	mMaxSpeedX = GOLDEN_MAX_SPEED_X;
	mMaxSpeedY = GOLDEN_MAX_SPEED_Y;
	FishBase::setVision(GOLDEN_VISION_RANGE, GOLDEN_VISION_ANGLE);
	mSpeedUp = GOLDEN_SPEED_UP;
	mSlowDown = GOLDEN_SLOW_DOWN;
	mSize = GOLDEN_SIZE;
	mLevel = GOLDEN_LEVEL;
	mPoints = GOLDEN_POINTS;
	mColorOffset.set(GOLDEN_COLOR_RED, GOLDEN_COLOR_GREEN, GOLDEN_COLOR_BLUE);
}

// 0x408d80
void GoldenMinnow::loadEffects()
{
	mTrail = new ParticleSystem();
	// The pictures are taken off and put back on so that the trail is behind them; the reference keeps them alive
	Ref<Selector> aKeptAnimations(mAnimations);
	removeAllChildren();
	addChild(mTrail);
	addChild(mAnimations);
	mTrail->load(GOLDEN_TRAIL_FILE);
	mTrail->setScale(1.0 / mSize, 1.0 / mSize);
	AIFish::loadEffects();
}

// 0x408c00
bool GoldenMinnow::canBeEatenBy(FishBase* theEater)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (theEater == aPlayer)
		return AIFish::canBeEatenBy(theEater);
	return false;
}

// 0x408bb0
void GoldenMinnow::putInPlay()
{
	mTrail->startEmitting();
	AIFish::putInPlay();
}

// 0x408d40
void GoldenMinnow::onEnterStage()
{
	mSwimsRight = getX() < GOLDEN_MIDDLE_X;
}

// 0x408bd0
void GoldenMinnow::takeOutOfPlay()
{
	mTrail->stopEmitting();
	AIFish::takeOutOfPlay();
}

} // namespace feedingfrenzy
