#include "SpeedGame.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const char* const TITLES_NODE = "titles";
const char* const SLOW_SOUND = "special_timeEffect";
}

// 0x459950
void SpeedGame::setup(GameScreen* theScreen, CityMap* theMap, Hud* theHud, GameStats* theStats)
{
	mScreen = theScreen;
	mMap = theMap;
	mHud = theHud;
	mStats = theStats;
	mTitles = static_cast<engine::FadeContainer*>(mScreen->mGameLayout->getNode(TITLES_NODE));
	mSlowSound = PizzaFrenzy::instance()->mSounds->getVoice(SLOW_SOUND);
	if (mSlowSound != nullptr)
		mSlowSound->setLoop(true);
	mMusicClip = nullptr;
	connectRightButton();
}
