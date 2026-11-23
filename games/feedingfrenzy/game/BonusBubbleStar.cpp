#include "BonusBubbleStar.h"
#include "FishFood.h"
#include "GameSession.h"
#include "Random.h"

using namespace engine;

namespace feedingfrenzy
{

// The pictures of the time bubble, the star's in the timed game
const char* const TIME_BUBBLE_FOLDER = "resources\\bonus\\timeBubble";
// The sparkle's particle file, and where it stands from the bubble in the timed game and in the other
const char* const SPARKLE_FILE = "config\\fx\\bonusStarFx.xml";
const Point TIMED_SPARKLE_OFFSET(20.0f, 20.0f);
const Point SPARKLE_OFFSET(0.0f, 20.0f);
// The star's points are a number of tens, from the low to the high number
const int STAR_TENS_LOW = 1;
const int STAR_TENS_HIGH = 5;
const float STAR_POINTS_PER_TEN = 10.0f;

// 0x419e80
BonusBubbleStar::BonusBubbleStar()
{
	mPoints = (float)randomInt(STAR_TENS_LOW, STAR_TENS_HIGH) * STAR_POINTS_PER_TEN;
}

// 0x419f60
void BonusBubbleStar::loadAnimationsOf(const std::string& theFolder)
{
	if (FishFood::instance()->getSession()->getMode() == GAME_MODE_TIMED)
	{
		mPngFrames = false;
		BonusBubble2X::loadAnimationsOf(TIME_BUBBLE_FOLDER);
	}
	else
	{
		mPngFrames = true;
		BonusBubble2X::loadAnimationsOf(theFolder);
	}
}

// 0x41a010
void BonusBubbleStar::loadEffects()
{
	mSparkle = new FishAttachFx();
	if (FishFood::instance()->getSession()->getMode() == GAME_MODE_TIMED)
		mSparkle->init(SPARKLE_FILE, this, TIMED_SPARKLE_OFFSET, false);
	else
		mSparkle->init(SPARKLE_FILE, this, SPARKLE_OFFSET, false);
}

// 0x419e40
void BonusBubbleStar::setPoints(float theFactor)
{
	mPoints = theFactor * mPoints;
}

// 0x419b70
void BonusBubbleStar::takeOutOfPlay()
{
	mSparkle->stop();
	AIFish::takeOutOfPlay();
}

// 0x419e60
void BonusBubbleStar::updateEnter(UpdateInfo& theInfo)
{
	BonusBubble2X::updateEnter(theInfo);
	mSparkle->start();
}

} // namespace feedingfrenzy
