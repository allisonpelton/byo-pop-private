#include "BonusBubbleShield.h"

using namespace engine;

namespace feedingfrenzy
{

// The sparkle's particle file, and where it stands from the bubble
const char* const SPARKLE_FILE = "config\\fx\\bonusStarFx.xml";
const Point SPARKLE_OFFSET(0.0f, 20.0f);

// 0x419b90
BonusBubbleShield::BonusBubbleShield()
{
	mPngFrames = true;
}

// 0x419c10
void BonusBubbleShield::loadEffects()
{
	mSparkle = new FishAttachFx();
	mSparkle->init(SPARKLE_FILE, this, SPARKLE_OFFSET, false);
}

// 0x419b70
void BonusBubbleShield::takeOutOfPlay()
{
	mSparkle->stop();
	AIFish::takeOutOfPlay();
}

// 0x419e60
void BonusBubbleShield::updateEnter(UpdateInfo& theInfo)
{
	BonusBubble2X::updateEnter(theInfo);
	mSparkle->start();
}

} // namespace feedingfrenzy
