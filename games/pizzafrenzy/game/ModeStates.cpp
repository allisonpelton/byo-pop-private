#include "ModeStates.h"
#include "App.h"

using namespace pizzafrenzy;

namespace
{
const float WAIT_SECONDS = 0.5f;   // 0x3f000000
const float SHOWN_SECONDS = 3.0f;  // 0x40400000
const float FADING_SECONDS = 1.0f; // 0x3f800000
const float FADE_SECONDS = 0.5f;
const float FADE_TARGET = 1.0f;
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
