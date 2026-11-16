#include "PufferFish.h"
#include "AnimImagePivot.h"
#include "FishAnimImage.h"
#include "FishFood.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The puffing's pictures: the end of the folder that has them, and their extension
const char* const PUFF_PICTURES = "puff\\puff_cycle.";
const char* const PICTURE_EXTENSION = ".jpg";
// The seconds a puffer wanders before it picks a new heading, and stays wandering (a fish's file may give others)
const Point WANDER_DECISION_RANGE(0.5f, 5.0f);
const Point WANDER_STATE_RANGE(3.0f, 5.0f);
// How much taller the hit rectangle is when the puffing's pictures have all played, as a share of its height
const float PUFF_GROWTH = 0.75f;
// The seconds a fish is in a state before it can puff
const float MIN_SECONDS_IN_STATE = 1.5f;
// The seconds the puffer waits before its pictures play, and how long it stays puffed up
const float WAIT_SECONDS = 0.05f;
const float HOLD_SECONDS = 1.8f;
// How fast the puffing's pictures play, inflating and deflating (pictures a second)
const float INFLATE_RATE = 22.0f;
const float DEFLATE_RATE = -15.0f;
// How far the puffing's pictures are put from the swimming ones, as a share of the difference of their heights, and
// of the fish's size
const float PICTURE_SHIFT_FACTOR = 0.5f;
// The seconds since the puffer cost the player points that it starts with, so that the first touch does
const float PENALTY_READY_SECONDS = 10000.0f;
// The seconds that have to have gone since the puffer cost the player points for it to do so again, and the share of
// the puffer's points the player pays
const float PENALTY_INTERVAL_SECONDS = 0.5f;
const float PENALTY_FACTOR = -0.5f;
// The chance that a puffer that decides what to do puffs, and the range the chance is drawn from
const float PUFF_CHANCE = 0.3f;
const Point CHANCE_RANGE(0.0f, 1.0f);
// The seconds a fish that touches the puffed fish is stunned
const float STUN_SECONDS = 0.4f;
// How hard the puffed fish pushes away the player's fish and any other fish, as a factor of the puffer's own push away
const float PLAYER_PUSH_FACTOR = -2.8f;
const float FISH_PUSH_FACTOR = -1.8f;
// The sound of the player's fish being pushed away, its volume and its pitch
const char* const BOUNCE_SOUND = "pufferBounce";
const float BOUNCE_SOUND_VOLUME = 1.0f;
const float BOUNCE_SOUND_PITCH = 1.0f;

// 0x40ce50
PufferFish::PufferFish() :
    mPuffIndex(0), mPhaseSeconds(0.0f), mSecondsSincePenalty(0.0f), mPuffPhase(PUFF_NONE), mPuffAnim(nullptr),
    mPuffGrowth(PUFF_GROWTH)
{
	mWanderDecisionRange = WANDER_DECISION_RANGE;
	mWanderStateRange = WANDER_STATE_RANGE;
	mStings = true;
}

// 0x40d470
void PufferFish::loadAnimationsOf(const std::string& theFolder)
{
	FishBase::loadAnimationsOf(theFolder);
	std::string aPath = theFolder + PUFF_PICTURES;
	Ref<FishAnimImage> aPuff(new FishAnimImage());
	aPuff->loadShifted(aPath.c_str(), PICTURE_EXTENSION, 1, mColorShift);
	mAnimations->addChild(aPuff);
	mPuffIndex = mAnimations->getCount() - 1;
	for (int aPicture = 0; aPicture < mAnimations->getCount(); aPicture++)
	{
		mAnimations->select(aPicture, 0.0f, true);
		setAnimationPivot(static_cast<AnimImage*>(mAnimations->getSelected()), 1);
	}
	updateRects();
	mAnimations->select(0, 0.0f, true);
}

// 0x40cf60
void PufferFish::setHitRect(const CollisionRect& theRect)
{
	mHitRect.set(theRect.getRect());
	mBaseHitRect.set(theRect.getRect());
	updatePlacedHitRect();
}

// 0x40c6e0
bool PufferFish::canEat(FishBase* theVictim)
{
	if (mState == AI_SPECIAL)
		return false;
	return AIFish::canEat(theVictim);
}

// 0x40cda0
bool PufferFish::canBeEatenBy(FishBase* theEater)
{
	if (mState == AI_SPECIAL && 1 < mPuffPhase)
		return false;
	return AIFish::canBeEatenBy(theEater);
}

// 0x40d250
void PufferFish::onContact(FishBase* theFish)
{
	if (mState != AI_SPECIAL || mPuffPhase <= 1)
		return;
	if (!theFish->isStunnable())
		return;

	// The puffer moves away from the fish, at its greatest horizontal speed
	Point aAway(theFish->getX(), theFish->getY());
	aAway.subtract(Point(getX(), getY()));
	aAway.normalize();
	mVelocity.x = aAway.x;
	mVelocity.y = aAway.y;
	mVelocity.scale(-mMaxSpeedX);
	Point aPush = mVelocity;

	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (theFish == aPlayer)
	{
		if (aPlayer->getState() != PLAYER_FURY)
		{
			aPush.scale(PLAYER_PUSH_FACTOR);
			theFish->setVelocity(aPush.x, aPush.y);
			theFish->stun(STUN_SECONDS, this, false);
			FishFood::instance()->playSound(BOUNCE_SOUND, BOUNCE_SOUND_VOLUME, BOUNCE_SOUND_PITCH);
			if (PENALTY_INTERVAL_SECONDS < mSecondsSincePenalty)
			{
				FishFood::instance()->getStageMgr()->showEatScore(mPoints * PENALTY_FACTOR);
				mSecondsSincePenalty = 0.0f;
			}
		}
	}
	else
	{
		if (theFish->isTargetable())
		{
			aPush.scale(FISH_PUSH_FACTOR);
			theFish->setVelocity(aPush.x, aPush.y);
		}
		theFish->stun(STUN_SECONDS, this, false);
	}
}

// 0x40cd80
void PufferFish::updateAnimation()
{
	if (mState != AI_SPECIAL)
		FishBase::updateAnimation();
}

// 0x40caa0
void PufferFish::updateSpecial(UpdateInfo& theInfo)
{
	mPhaseSeconds = mPhaseSeconds - theInfo.mSeconds;
	mSecondsSincePenalty = mSecondsSincePenalty + theInfo.mSeconds;
	switch (mPuffPhase)
	{
	case PUFF_NONE:
		beginWander();
		setScale(mSize, mSize);
		mAnimations->select(0, 0.0f, true);
		mStateSeconds = randomFloat(mWanderStateRange);
		mHitRect.set(mBaseHitRect.getRect());
		FishBase::update(theInfo);
		return;
	case PUFF_WAIT:
		mDesiredVelocity.set(0.0f, 0.0f);
		mSecondsSincePenalty = PENALTY_READY_SECONDS;
		if (mPhaseSeconds < 0.0f)
		{
			startPuff();
			FishBase::update(theInfo);
			return;
		}
		break;
	case PUFF_INFLATE:
		updatePuffHitRect();
		if (mPuffAnim->getFrame() == mPuffAnim->getCount() - 1)
		{
			mPhaseSeconds = HOLD_SECONDS;
			mPuffPhase = PUFF_HOLD;
			FishBase::update(theInfo);
			return;
		}
		break;
	case PUFF_HOLD:
		if (mPhaseSeconds < 0.0f)
		{
			startDeflate();
			FishBase::update(theInfo);
			return;
		}
		break;
	case PUFF_DEFLATE:
		updatePuffHitRect();
		if (mPuffAnim->getFrame() < 1)
		{
			mPuffPhase = PUFF_NONE;
			mAnimState = ANIM_SWIM;
			mSecondsInState = 0.0f;
		}
		break;
	}
	FishBase::update(theInfo);
}

// 0x40c960
void PufferFish::checkReactions()
{
	PlayerFish* aPlayer = FishFood::instance()->getStageMgr()->getPlayer();
	if (mState == AI_SPECIAL)
		return;

	if (MIN_SECONDS_IN_STATE < mSecondsInState && mReactionSeconds < 0.0f)
	{
		if (mLevel < aPlayer->getLevel())
		{
			Point aToPlayer = aPlayer->getNosePosition();
			aToPlayer.subtract(mNose);
			if (aToPlayer.lengthSquared() < (double)mVisionRange * (double)mVisionRange)
			{
				beginSpecial();
				return;
			}
		}
	}
	if (mState != AI_SPECIAL)
		AIFish::checkReactions();
}

// 0x40c700
void PufferFish::decide()
{
	if (mState == AI_SPECIAL)
	{
		mStateSeconds = randomFloat(mWanderStateRange);
		return;
	}
	if (randomFloat(CHANCE_RANGE) < PUFF_CHANCE)
		beginSpecial();
	else
		beginWander();
}

// 0x40c760
void PufferFish::beginSpecial()
{
	if (mState == AI_BEING_EATEN || mState == AI_SPECIAL || mState == AI_STUNNED || mState == AI_LEAVE ||
	    mState == AI_FROZEN)
		return;
	if (mSecondsInState < MIN_SECONDS_IN_STATE)
		return;

	mPuffAnim = static_cast<AnimImage*>(mAnimations->getSelected());
	if (mPuffAnim != nullptr)
		mPuffAnim->stop();
	mPuffPhase = PUFF_WAIT;
	mPhaseSeconds = WAIT_SECONDS;
	mState = AI_SPECIAL;
}

// 0x40cdd0
void PufferFish::freeze()
{
	if (mState == AI_SPECIAL && mPuffPhase < PUFF_DEFLATE)
	{
		mPuffAnim->setRate(DEFLATE_RATE);
		mPuffAnim->play();
		mPuffPhase = PUFF_DEFLATE;
	}
	AIFish::freeze();
}

// 0x40cc70 (the share of the pictures that have played is kept as a float, and the height is not rounded to one)
void PufferFish::updatePuffHitRect()
{
	float aShare = (float)((double)mPuffAnim->getFrame() / (double)mPuffAnim->getCount() * (double)mPuffGrowth + 1.0);
	const Rect& aBase = mBaseHitRect.getRect();
	double aHeight = (double)aBase.bottom - (double)aBase.top;
	float aGrow = (float)((((double)aShare * aHeight) - aHeight) * 0.5);
	mHitRect.set(Rect(aBase.left, aBase.top - aGrow, aBase.right, aBase.bottom + aGrow));
	updatePlacedHitRect();
}

// 0x40c800
void PufferFish::startPuff()
{
	mPuffPhase = PUFF_INFLATE;
	mAnimations->select(0, 0.0f, true);
	Node* aSwim = mAnimations->getSelected();
	mAnimations->select(mPuffIndex, 0.0f, true);
	mPuffAnim = static_cast<AnimImage*>(mAnimations->getSelected());
	setScale(mVelocity.x < 0.0f ? mSize : -mSize, mSize);
	float aPuffHeight = mPuffAnim->getBounds()->height();
	const Rect* aSwimBounds = aSwim->getBounds();
	double aSwimHeight = (double)aSwimBounds->bottom - (double)aSwimBounds->top;
	mPuffAnim->setPosition(0.0f, (float)(((double)aPuffHeight - aSwimHeight) * (double)PICTURE_SHIFT_FACTOR *
	                                     (double)mSize));
	mPuffAnim->setLoop(false);
	mPuffAnim->setRate(INFLATE_RATE);
	mPuffAnim->setFrame(0);
	mPuffAnim->play();
}

// 0x40c930
void PufferFish::startDeflate()
{
	mPuffAnim->setRate(DEFLATE_RATE);
	mPuffAnim->play();
	mPuffPhase = PUFF_DEFLATE;
}

} // namespace feedingfrenzy
