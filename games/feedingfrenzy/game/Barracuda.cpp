#include "Barracuda.h"
#include "App.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "StageMgr.h"
#include <cmath>

using namespace engine;

namespace feedingfrenzy
{

// The speed the fish slows down at, and how much faster it swims while its mouth is open on a victim
const float BARRACUDA_SLOW_DOWN = 70.0f;
const float BARRACUDA_LUNGE_FACTOR = 1.0f;
// The seconds the fish holds still, warning, after it is put in play, and the points it has in them
const float WARNING_SECONDS = 0.8f;
const float WARNING_POINTS = 100.0f;
// The danger sign: its picture, how often it blinks, and how far it stands from the edge of the view
const char* const DANGER_SIGN_IMAGE = "resources\\HUD\\dangerSign.png";
const float DANGER_SIGN_BLINK_SECONDS = 0.1f;
const float DANGER_SIGN_MARGIN = 10.0f;
// A fish that stands at least this far from the left of the stage comes in from the right (the sign shows at the
// right of the view), and the same limit while the fish waits to swim in
const float COMES_FROM_RIGHT_X = 100.0f;
const float WARNING_RIGHT_X = 10.0f;
// The sound of the warning, with its volume and pitch
const char* const DANGER_SOUND = "dangerSound";
const float DANGER_SOUND_VOLUME = 1.0f;
const float DANGER_SOUND_PITCH = 1.0f;
// The seconds the fish is given to be eaten in by a larger fish (the base's eating takes its own)
const float BEING_EATEN_SECONDS = 0.2f;
// A fish that touches it makes it swim this much faster, and it cannot be touched again for the seconds; the volume of
// the sound of the touch
const float TOUCHED_SPEED_FACTOR = 1.2f;
const float TOUCHED_WAIT_SECONDS = 1.0f;
const float TOUCHED_SOUND_VOLUME = 1.0f;
// The message the player's bite of its tail shows (the text's id), and the points the bite adds to the fish
const char* const TAIL_BITE_MESSAGE = "tailBite";
const float TAIL_BITE_POINTS = 100.0f;
// The seconds a stunned fish keeps its heading
const float STUNNED_STATE_SECONDS = 5000.0f;
// How far the stun effect stands from the fish's end, as part of the fish's length
const float STUN_EFFECT_FRACTION = 0.5f;

// 0x4050c0
Barracuda::Barracuda() : mTouchedSeconds(0.0f)
{
	mHuge = true;
	mPickup = true;
	mSlowDown = BARRACUDA_SLOW_DOWN;
	mLungeFactor = BARRACUDA_LUNGE_FACTOR;
}

// 0x404fc0
void Barracuda::update(UpdateInfo& theInfo)
{
	if (0.0f < mTouchedSeconds)
	{
		mTouchedSeconds -= theInfo.mSeconds;
		if (mTouchedSeconds < 0.0f)
			mPickup = true;
	}
	if (mState != AI_ENTER)
		mDangerSign->stop();
	AIFish::update(theInfo);
}

// 0x405030
void Barracuda::startBeingEaten(FishBase* theEater, float)
{
	if (theEater != nullptr)
	{
		if (theEater->getLevel() <= mLevel && mTouchedSeconds <= 0.0f)
		{
			onTouched(theEater);
			return;
		}
	}
	AIFish::startBeingEaten(theEater, BEING_EATEN_SECONDS);
}

// 0x405430
bool Barracuda::finishBeingEaten(FishBase* theEater)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (theEater != nullptr)
	{
		if (theEater->getLevel() <= mLevel)
		{
			if (theEater != aPlayer)
				return false;
			beginChase();
			FishFood::instance()->getStageMgr()->showMessage(
			    FishFood::instance()->getStrings().get(TAIL_BITE_MESSAGE));
			mPoints = mPoints + TAIL_BITE_POINTS;
			return true;
		}
	}
	return AIFish::finishBeingEaten(theEater);
}

// 0x405390
void Barracuda::onTouched(FishBase* theFish)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (mTouchedSeconds <= 0.0f && theFish == aPlayer)
	{
		mTarget = aPlayer;
		mDesiredVelocity.x = mDesiredVelocity.x * TOUCHED_SPEED_FACTOR;
		mPickup = false;
		mTouchedSeconds = TOUCHED_WAIT_SECONDS;
		FishFood::instance()->playSound(mEatenSoundName, TOUCHED_SOUND_VOLUME, mEatenSoundPitch);
	}
}

// 0x404d90
void Barracuda::putInPlay()
{
	mLeafSeconds = WARNING_SECONDS;
	AIFish::putInPlay();
	if (mDangerSign == nullptr)
	{
		Image* anImage = App::instance()->getImage(DANGER_SIGN_IMAGE);
		mDangerSign = new Blinker(anImage);
		mDangerSign->setInterval(DANGER_SIGN_BLINK_SECONDS);
		FishFood::instance()->getStageMgr()->addToOverlay(mDangerSign);
	}
}

// 0x405250
void Barracuda::onEnterStage()
{
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	Point aViewLeft = aStage->screenToWorld(Point(0.0f, 0.0f));
	float aSignX;
	if (COMES_FROM_RIGHT_X <= getX())
	{
		float aViewRight = (float)aStage->getScreenWidth() + aViewLeft.x;
		aSignX = (aViewRight - (float)mDangerSign->getImageWidth()) - DANGER_SIGN_MARGIN;
	}
	else
	{
		aSignX = aViewLeft.x;
	}
	mDangerSign->setPosition(aSignX, getY());
	mDangerSign->start();
	FishFood::instance()->playSound(DANGER_SOUND, DANGER_SOUND_VOLUME, DANGER_SOUND_PITCH);
}

// 0x404e60
void Barracuda::takeOutOfPlay()
{
	mDangerSign->stop();
	AIFish::takeOutOfPlay();
}

// 0x405160
void Barracuda::stun(float, FishBase* theSource, bool theShowEffect)
{
	if (theShowEffect)
	{
		mStunEffect->setPosition(fabsf((mTail.x - mNose.x) * STUN_EFFECT_FRACTION), 0.0f);
		mStunEffect->startEmitting();
	}
	mDecisionSeconds = STUNNED_STATE_SECONDS;
	mStateSeconds = STUNNED_STATE_SECONDS;
	if (!mVictims.empty())
		finishEating();
	mState = AI_CRUISE;
	float aSourceX = theSource->getNosePosition().x;
	if (aSourceX <= mTail.x)
		mDesiredVelocity.x = mMaxSpeedX;
	else
		mDesiredVelocity.x = -mMaxSpeedX;
	mDesiredVelocity.y = -mVelocity.y;
}

// 0x404e80
void Barracuda::updateEnter(UpdateInfo& theInfo)
{
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	if (mLeafSeconds <= 0.0f)
	{
		AIFish::updateEnter(theInfo);
	}
	else
	{
		mPoints = WARNING_POINTS;
		mLeafSeconds = mLeafSeconds - theInfo.mSeconds;
	}
	Point aViewLeft = aStage->screenToWorld(Point(0.0f, 0.0f));
	float aSignX;
	if (WARNING_RIGHT_X <= getX())
	{
		float aViewRight = (float)aStage->getScreenWidth() + aViewLeft.x;
		aSignX = (aViewRight - (float)mDangerSign->getImageWidth()) - DANGER_SIGN_MARGIN;
	}
	else
	{
		aSignX = aViewLeft.x;
	}
	mDangerSign->setPosition(aSignX, getY());
}

} // namespace feedingfrenzy
