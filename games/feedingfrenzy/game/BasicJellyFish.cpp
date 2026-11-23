#include "BasicJellyFish.h"
#include "AnimImage.h"
#include "Random.h"

using namespace engine;

namespace feedingfrenzy
{

// The seconds its pulse timer starts at, which is already over
const float START_PULSE_SECONDS = -1.0f;
// The picture sequence's rate, in pictures a second
const float JELLY_RATE = 30.0f;
// The chance, of one, that it swims to the left, and the opacity it shows with
const float LEFT_CHANCE = 0.5f;
const float JELLY_OPACITY = 0.75f;
// The level that it has in play: no fish's is as high, so no fish eats it
const float IN_PLAY_LEVEL = 9999.0f;
// The seconds a fish that touches it is stunned for
const float TOUCH_STUN_SECONDS = 1.0f;
// The seconds that the picture has played before the jelly swims
const double SWIM_DELAY_SECONDS = 0.1;
// The ranges of seconds that a burst of swimming lasts and that it rests
const Point SWIM_SECONDS(0.6f, 1.0f);
const Point REST_SECONDS(1.2f, 1.5f);

// 0x405800
BasicJellyFish::BasicJellyFish()
{
	mLeafSeconds = START_PULSE_SECONDS;
	mStings = true;
	mKind = FISH_KIND_HAZARD;
}

// 0x405b70
void BasicJellyFish::loadAnimationsOf(const std::string& theFolder)
{
	std::string aPath = theFolder + "jelly.";
	loadAnimations(aPath.c_str(), nullptr, nullptr, nullptr);
	mAnimations->select(0, 0.0f, true);
	static_cast<AnimImage*>(mAnimations->getSelected())->setLoop(false);
	static_cast<AnimImage*>(mAnimations->getSelected())->setRate(JELLY_RATE);
}

// 0x4056d0
void BasicJellyFish::putInPlay()
{
	mAnimations->select(0, 0.0f, true);
	AIFish::putInPlay();
	if (randomFloat(Point(0.0f, 1.0f)) < LEFT_CHANCE)
		mMaxSpeedX = mMaxSpeedX * -1.0f;
	setScale(mMaxSpeedX <= 0.0f ? mSize : -mSize, mSize);
	setOpacity(JELLY_OPACITY);
	mLevel = IN_PLAY_LEVEL;
}

// 0x405780
void BasicJellyFish::onContact(FishBase* theFish)
{
	if (mLevel != theFish->getLevel())
	{
		if (theFish->isStunnable())
			theFish->stun(TOUCH_STUN_SECONDS, this, true);
	}
}

// 0x405580
void BasicJellyFish::updateEnter(UpdateInfo& theInfo)
{
	beginSpecial();
	FishBase::update(theInfo);
}

// 0x4055a0
void BasicJellyFish::updateSpecial(UpdateInfo& theInfo)
{
	if (getY() < -mBounds.height())
	{
		takeOutOfPlay();
		return;
	}
	mLeafSeconds = mLeafSeconds - theInfo.mSeconds;
	if (mLeafSeconds < 0.0f)
	{
		if (0.0f <= mDesiredVelocity.y)
		{
			AnimImage* aPulse = static_cast<AnimImage*>(mAnimations->getSelected());
			if (!aPulse->isPlaying())
			{
				aPulse->setFrame(0);
				aPulse->play();
			}
			if (-SWIM_DELAY_SECONDS <= mLeafSeconds)
			{
				FishBase::update(theInfo);
				return;
			}
			mDesiredVelocity.x = mMaxSpeedX;
			mDesiredVelocity.y = -mMaxSpeedY;
			mLeafSeconds = randomFloat(SWIM_SECONDS);
		}
		else
		{
			mDesiredVelocity.y = 0.0f;
			mLeafSeconds = randomFloat(REST_SECONDS);
		}
	}
	FishBase::update(theInfo);
}

} // namespace feedingfrenzy
