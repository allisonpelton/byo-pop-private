#include "GameMode.h"
#include "App.h"
#include "ModeStates.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
// While the time is slowed the music is silent (0x4505f0)
const float SLOWED_MUSIC_VOLUME = 0.0f;
}

// 0x459510 (speed), 0x45b3e0 (concentration): the counts start empty, the factor of the slowed time one
GameMode::GameMode() :
	mMusicVolume(0),
	mLevel(nullptr),
	mSlowFactor(1.0f),
	mSlowTime(0),
	mSlowSound(nullptr),
	mComboTopping(nullptr)
{
	mSpawnDelay.mFirst = 0;
	mSpawnDelay.mSecond = 0;
	mWaitTime.mFirst = 0;
	mWaitTime.mSecond = 0;
}

// 0x45a2e0 (speed), 0x4596d0
GameMode::~GameMode()
{
	mStates.changeState(nullptr);
}

// 0x459330's connection of the right button (0x455cb0 calls slot 0x48)
void GameMode::connectRightButton()
{
	engine::App::instance()->mRightDownSignal.connect(this, &GameMode::onRightDown);
}

// 0x4569c0
void GameMode::update(engine::UpdateInfo& theInfo)
{
	updatePizzas(theInfo);
	mHud->updateHud(theInfo);
	mStats->mElapsedTime = theInfo.mSeconds + mStats->mElapsedTime;
	if (0.0f < mSlowTime)
	{
		mSlowTime = mSlowTime - theInfo.mSeconds;
		if (mSlowSound != nullptr && !mSlowSound->isPlaying())
		{
			mSlowSound->play();
			PizzaFrenzy::instance()->mMusicPlayer->setVolume(SLOWED_MUSIC_VOLUME);
		}
		if (0.0f < mSlowTime)
			theInfo.mSeconds = mSlowFactor * theInfo.mSeconds;
		else
		{
			mSlowTime = 0;
			mSlowFactor = 1.0f;
			if (mSlowSound != nullptr)
				mSlowSound->stop();
			PizzaFrenzy::instance()->mMusicPlayer->setVolume(mMusicVolume);
		}
	}
	mScreen->updateScreen(theInfo);
	mStates.update(theInfo);
}

// 0x459cb0
void GameMode::setLevel(Level* theLevel)
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mLevel = theLevel;
	reset();
	theLevel->setUpKitchens(mMap);
	mMap->collectKitchenToppings();
	theLevel->setUpBoats(mMap);
	mScreen->drawCity();
	mMusicClip = anApp->mTileManifest->getMusic(theLevel->mMusic);
	if (mMusicClip != nullptr)
	{
		mMusicVolume = mMusicClip->getVolume();
		anApp->mMusicPlayer->play(mMusicClip);
	}
	makeEvents();
	mComboTopping = nullptr;
	mStats->mComboLength = 0;
	mMap->showCombo(nullptr, 0);
	// the speed bonus banner (0x44fb70, the game screen's +0x204) starts over here: left out, see GameScreen.h
	beginWaves();
	mStates.changeState(new ModeIntroState(this));
}

// 0x459150
void GameMode::clearLevel()
{
	clearEvents();
	mMap->clearBoatSpawners();
	mLevel = nullptr;
}

// 0x456c20
void GameMode::startPlay()
{
	mHud->showVehicles();
	mStates.changeState(new ModePlayState(this));
}

// 0x456300
void GameMode::setPaused(bool thePaused)
{
	clearPicks();
	mHud->pauseTipJarSound(thePaused);
}

// 0x455fe0
void GameMode::updateMap(engine::UpdateInfo& theInfo)
{
	mMap->update(theInfo);
}

void GameMode::updatePizzas(engine::UpdateInfo& theInfo)
{
}

void GameMode::makeEvents()
{
}

void GameMode::clearEvents()
{
}
