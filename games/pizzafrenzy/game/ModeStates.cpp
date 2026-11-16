#include "ModeStates.h"
#include "App.h"
#include "PizzaFrenzy.h"

using namespace pizzafrenzy;

namespace
{
const float WAIT_SECONDS = 0.5f;   // 0x3f000000
const float SHOWN_SECONDS = 3.0f;  // 0x40400000
const float FADING_SECONDS = 1.0f; // 0x3f800000
const float FADE_SECONDS = 0.5f;
const float FADE_TARGET = 1.0f;

// the prize blimp starts at the right edge, high up, and flies to the left; the state lasts this long at least
const float BLIMP_START_X = 830.0f;
const float BLIMP_START_Y = 200.0f;
const float BLIMP_FLIGHT_SECONDS = 7.5f;
const char* const MUSIC_BLIMP = "music_blimpFlyby";

// the title shows for two and a half seconds, and goes for two
const float TITLE_SHOWN_SECONDS = 2.5f;
const float TITLE_LEAVING_SECONDS = 2.0f;
const char* const MUSIC_LEVEL_COMPLETE = "music_levelComplete";
const char* const MUSIC_GAME_OVER = "music_gameOver";
const char* const ACTION_LEVEL_END = "levelEnd";
const char* const ACTION_GAME_OVER = "gameOver";
const int STRING_LEVEL_COMPLETE = 0xd0;
const int STRING_LEVEL_PERFECT = 0xd1;
const int STRING_GAME_OVER = 0xd2;
}

// 0x45a1a0
void ModeIntroState::enter()
{
	mMode->getTitles()->setVisible(false);
	mMode->getTitles()->setOpacity(0);
	mTimeLeft = WAIT_SECONDS;
	mStage = STAGE_WAIT;
	engine::App::instance()->mMouseDownSignal.connect(this, &ModeIntroState::onMouseDown);
}

// 0x456540
void ModeIntroState::exit()
{
	mMode->getTitles()->setVisible(false);
}

// 0x4564a0
void ModeIntroState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (mTimeLeft < 0.0f)
	{
		if (mStage == STAGE_WAIT)
		{
			mStage = STAGE_SHOWN;
			mTimeLeft = SHOWN_SECONDS;
			mMode->getTitles()->fade(true, FADE_SECONDS, FADE_TARGET, false);
		}
		else if (mStage == STAGE_SHOWN)
		{
			mStage = STAGE_FADING;
			mTimeLeft = FADING_SECONDS;
			mMode->getTitles()->fade(false, FADE_SECONDS, FADE_TARGET, false);
		}
		else if (mStage == STAGE_FADING)
			mMode->startPlay();
	}
}

// 0x455bb0
void ModeIntroState::onMouseDown(engine::Point& thePoint)
{
	if (0.0f < mTimeLeft && mStage < STAGE_FADING)
	{
		mTimeLeft = 0;
		mStage = STAGE_SHOWN;
	}
}

// 0x455be0
void ModePlayState::update(engine::UpdateInfo& theInfo)
{
	mMode->updateWaves(theInfo);
	mMode->updateMap(theInfo);
	mMode->checkLevelEnd();
}

// 0x4588b0: the blimp is made, set to fly to the left from the right edge, and put on the screen (a blimp that could
// not be made leaves the state with nothing to wait for)
void PrizeBlimpState::enter()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	mBlimp = new PrizeBlimp();
	mBlimp->loadType();
	mBlimp->setDirection(engine::Point(-1.0f, 0.0f));
	mBlimp->start(mMode->getMap(), engine::Point(BLIMP_START_X, BLIMP_START_Y));
	mMode->addVehicleNode(mBlimp);
	anApp->mMusicPlayer->play(anApp->mTileManifest->getMusic(MUSIC_BLIMP));
	mTimeLeft = BLIMP_FLIGHT_SECONDS;
	CityMap* aMap = mMode->getMap();
	for (size_t i = 0; i < aMap->mKitchens.size(); i++)
		static_cast<BuildingTile*>(aMap->mKitchens[i].get())->hangUp();
	mMode->getHud()->slideCouponsOut();
}

// 0x456f40
void PrizeBlimpState::update(engine::UpdateInfo& theInfo)
{
	mMode->updateMap(theInfo);
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (mBlimp != nullptr)
	{
		if (!mBlimp->isDone())
			return;
		if (0.0f < mTimeLeft)
			return;
		if (!mMode->getMap()->isIdle())
			return;
	}
	mMode->mStates.changeState(new ModeEndState(mMode, false));
}

// 0x458490
void ModeEndState::enter()
{
	PizzaFrenzy* anApp = PizzaFrenzy::instance();
	if (mMode->getHud() != nullptr)
		mMode->getHud()->stopTipJar();
	int aText = STRING_LEVEL_COMPLETE;
	if (mGameOver)
		aText = STRING_GAME_OVER;
	else if (mMode->getStats()->mPizzasDelivered == mMode->getStats()->mPizzasOrdered)
		aText = STRING_LEVEL_PERFECT;
	mTitle = makeEndTitle(mMode->getScreen(), aText);
	mTitleLeaving = false;
	mTimeLeft = TITLE_SHOWN_SECONDS;
	mMode->finishLevel();
	mMode->getScreen()->disconnect();
	anApp->mMusicPlayer->play(anApp->mTileManifest->getMusic(mGameOver ? MUSIC_GAME_OVER : MUSIC_LEVEL_COMPLETE));
}

// 0x4587a0: the title shows and then its letters leave, and the application is told the level is over
void ModeEndState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (!mTitleLeaving)
	{
		if (mTimeLeft < 0.0f)
		{
			mTitle->leave();
			mTitleLeaving = true;
			mTimeLeft = TITLE_LEAVING_SECONDS;
		}
		return;
	}
	if (mTimeLeft < 0.0f)
		PizzaFrenzy::instance()->screenAction(mGameOver ? ACTION_GAME_OVER : ACTION_LEVEL_END);
}
