#include "BonusBubble2X.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The seconds the kind of fish's timer starts at, which is no time running (nothing in a bubble reads it)
const float NO_TIMER_SECONDS = -1.0f;
// The size a bubble that grows starts at
const float GROW_START_SIZE = 0.1f;

// 0x4197e0
BonusBubble2X::BonusBubble2X() :
    mGrowing(false), mGrowSecondsLeft(0.0f), mGrowSeconds(0.0f), mGrowSize(0.0f)
{
	mHuge = true;
	mEdibleKind = EDIBLE_OBJECT;
	mKind = FISH_KIND_BONUS;
	mLeafSeconds = NO_TIMER_SECONDS;
	mReactable = false;
	mCountsAsFood = false;
}

// 0x419890
void BonusBubble2X::loadAnimationsOf(const std::string& theFolder)
{
	loadAnimations(theFolder.c_str(), nullptr, nullptr, nullptr);
}

// 0x4196e0
bool BonusBubble2X::canBeEatenBy(FishBase* theEater)
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (theEater == aPlayer && !mBeingEaten)
		return true;
	return false;
}

// 0x419710
void BonusBubble2X::putInPlay()
{
	mAnimations->select(0, 0.0f, true);
	AIFish::putInPlay();
	mGrowing = false;
}

// 0x419550
void BonusBubble2X::updateAnimation()
{
	mAnimations->select(0, 0.0f, true);
}

// 0x405580
void BonusBubble2X::updateEnter(UpdateInfo& theInfo)
{
	beginSpecial();
	FishBase::update(theInfo);
}

// 0x419570
void BonusBubble2X::updateSpecial(UpdateInfo& theInfo)
{
	if (mVelocity.y < 0.0f)
	{
		if (getY() < -mBounds.height())
		{
			takeOutOfPlay();
			return;
		}
	}
	if (0.0f < mVelocity.y)
	{
		StageMgr* aStage = FishFood::instance()->getStageMgr();
		float aY = getY();
		if ((float)aStage->getWorldHeight() < aY)
		{
			takeOutOfPlay();
			return;
		}
	}
	if (mGrowing && 0.0f < mGrowSecondsLeft)
	{
		mGrowSecondsLeft = mGrowSecondsLeft - theInfo.mSeconds;
		if (mGrowSecondsLeft < 0.0f)
		{
			mGrowSecondsLeft = 0.0f;
			mGrowing = false;
		}
		setSize((float)((1.0 - mGrowSecondsLeft / mGrowSeconds) * mGrowSize));
		setScale(mSize, mSize);
		layout();
		updatePlacedHitRect();
	}
	FishBase::update(theInfo);
}

// 0x4196b0
void BonusBubble2X::beginSpecial()
{
	bool aSwallowed = isSwallowed();
	mState = AI_SPECIAL;
	if (!aSwallowed)
		mDesiredVelocity.y = -mMaxSpeedY;
}

// 0x419750
void BonusBubble2X::startGrowing(float theSeconds, float theSize)
{
	mGrowSeconds = theSeconds;
	mGrowSecondsLeft = theSeconds;
	mGrowing = true;
	mGrowSize = theSize;
	setSize(GROW_START_SIZE);
	setScale(mSize, mSize);
	layout();
	updatePlacedHitRect();
}

} // namespace feedingfrenzy
