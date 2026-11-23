#include "FlyByFish.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The speed the fish slows down at
const float FLYBY_SLOW_DOWN = 70.0f;
// A fish that comes in left of this swims to the right
const float FLYBY_MIDDLE_X = 320.0f;
// The seconds a fish that has been touched cannot be touched again, and how much faster it swims
const float TOUCHED_WAIT_SECONDS = 1.5f;
const float TOUCHED_SPEED_FACTOR = 1.2f;
// The seconds the fish keeps its depth and its heading
const float CRUISE_SECONDS_LOW = 6.0f;
const float CRUISE_SECONDS_HIGH = 10.0f;
// The volume the sound of a touch plays at
const float TOUCHED_SOUND_VOLUME = 1.0f;

// 0x408a50
FlyByFish::FlyByFish() : mSwimsRight(false)
{
	mSlowDown = FLYBY_SLOW_DOWN;
	mLungeFactor = 1.0f;
	mLeafSeconds = 0.0f;
}

// 0x408910
void FlyByFish::update(UpdateInfo& theInfo)
{
	if (mLeafSeconds > 0.0f)
		mLeafSeconds -= theInfo.mSeconds;
	AIFish::update(theInfo);
}

// 0x408940
bool FlyByFish::finishBeingEaten(FishBase* theEater)
{
	if (theEater->getLevel() > mLevel)
	{
		AIFish::finishBeingEaten(theEater);
		return true;
	}
	beginChase();
	return true;
}

// 0x408ae0
void FlyByFish::onTouched(FishBase* theFish)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (mLeafSeconds <= 0.0f && theFish == aPlayer)
	{
		mTarget = aPlayer;
		mDesiredVelocity.x = mDesiredVelocity.x * TOUCHED_SPEED_FACTOR;
		mLeafSeconds = TOUCHED_WAIT_SECONDS;
		FishFood::instance()->playSound(mEatenSoundName, TOUCHED_SOUND_VOLUME, mEatenSoundPitch);
	}
}

// 0x408d40
void FlyByFish::onEnterStage()
{
	mSwimsRight = getX() < FLYBY_MIDDLE_X;
}

// 0x408990
void FlyByFish::beginCruise()
{
	if (mState != AI_CRUISE)
	{
		if (mSwimsRight)
			mDesiredVelocity.x = mMaxSpeedX;
		else
			mDesiredVelocity.x = -mMaxSpeedX;
	}
	mState = AI_CRUISE;
	mDesiredVelocity.y = randomFloat(Point(-mMaxSpeedY, mMaxSpeedY));
	mDecisionSeconds = randomFloat(Point(CRUISE_SECONDS_LOW, CRUISE_SECONDS_HIGH));
	mStateSeconds = randomFloat(Point(CRUISE_SECONDS_LOW, CRUISE_SECONDS_HIGH));
}

} // namespace feedingfrenzy
