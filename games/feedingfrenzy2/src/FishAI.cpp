#include "FishAI.h"

#include "Actor.h"
#include "Draws.h"
#include "Random.h"
#include "Stage.h"
#include "VectorMath.h"
#include "Visual.h"

#include <cmath>

using namespace engine;

namespace
{

const float SPAWN_MARGIN = 80.0f;          // 0x566888 (feedingfrenzy2): the bounds narrowed at each side to swim in
const float WANDER_MARGIN_X = 30.0f;       // 0x5667c8 (feedingfrenzy2)
const float WANDER_MARGIN_Y = 40.0f;       // 0x566ab8 (feedingfrenzy2)
const double HALF = 0.5;                   // 0x566760 (feedingfrenzy2)
const double EVACUATE_SPEED = 2.5;         // 0x566958 (feedingfrenzy2): the top speed's share leaving the stage
const float MINUS_ONE = -1.0f;             // 0x566850 (feedingfrenzy2)
const double MINUS_ONE_DOUBLE = -1.0;      // 0x566938 (feedingfrenzy2)
const double FAR_CENTER_BOOST = 1.4;       // 0x566b08 (feedingfrenzy2): the speed back to a far centre
const double DART_LOW = 0.8;               // 0x5669f0 (feedingfrenzy2): a dart's speed, in shares of the top speed
const double DART_HIGH = 1.8;              // 0x566b00 (feedingfrenzy2)
const float SLOWEST_RISE = 10.0f;          // 0x54de60 (feedingfrenzy2): a speed up or down below it is none
const float DEFAULT_DART_WEIGHT = 0.2f;    // 0x54ddf8 (feedingfrenzy2)
const float DEFAULT_CENTER_WEIGHT = 1.2f;  // 0x566b10 (feedingfrenzy2)
const float DEFAULT_RANDOM_WEIGHT = 0.2f;  // 0x54ddf8 (feedingfrenzy2)
const float DEFAULT_REVERSE_WEIGHT = 0.05f;  // 0x566880 (feedingfrenzy2)
const float DEFAULT_DART_SECONDS = 0.8f;   // 0x566950 (feedingfrenzy2)
const double FLEE_SPEEDUP = 2.0;           // 0x566798 (feedingfrenzy2): a flight's speed, in top speeds
const double HEADED_AT_COS = 0.85;         // 0x566a40 (feedingfrenzy2): the cosine within which a way heads at a threat
const float AVOID_TURN = 0.52359f;         // 0x566974 (feedingfrenzy2): the turn aside, in radians
const float AVOID_TURN_BACK = -0.52359f;   // 0x566970 (feedingfrenzy2)
const float NOTHING = 0.0f;                // 0x5471cc (feedingfrenzy2)
const float GOAL_REACHED_SQUARED = 256.0f;   // 0x566ac4 (feedingfrenzy2): a school's centre this near its goal is there
const double FLY_BY_GOAL_BEYOND = 187.5;     // 0x566ab0 (feedingfrenzy2): a fly-by's goal past the right side
const float FLY_BY_GOAL_BEFORE = -187.5f;    // 0x566ac0 (feedingfrenzy2): and past the left
const double FLY_BY_GOAL_MARGIN = 40.0;      // 0x566ab8 (feedingfrenzy2): its height kept off the top and bottom
const double WANDER_GOAL_MARGIN = 20.0;      // 0x566aa8 (feedingfrenzy2)
const float FRIGHT_SHORTEST = 1.5f;          // 0x56696c (feedingfrenzy2): a school's fright, in seconds
const float FRIGHT_LONGEST = 2.0f;           // 0x54a334 (feedingfrenzy2)
const float PULL_SHORTEST = 0.05f;           // 0x566880 (feedingfrenzy2): the pulls' time, in seconds
const float PULL_LONGEST = 0.1f;             // 0x54ddd4 (feedingfrenzy2)
const float FLEE_GOAL_SHARE = 0.2f;          // 0x54ddf8 (feedingfrenzy2): the goal's pull on a frightened school
const double SEPARATION_LEAST_SQUARED = 1.0; // 0x566768 (feedingfrenzy2): nearer than its root counts as 1 away
const double NEAREST_THREAT = 0.1;           // 0x5667c0 (feedingfrenzy2): a threat nearer counts as this far
const float NEAREST_THREAT_DISTANCE = 0.1f;  // 0x54ddd4 (feedingfrenzy2)

// A speed brought up to the least one, by its sign (0x4626e1 to 0x462755 (feedingfrenzy2)); 0 stays 0
float atLeast(float theSpeed, float theLeast)
{
	if (theSpeed < 0.0f)
	{
		float aBack = -theLeast;
		if (aBack < theSpeed)
			return aBack;
	}
	if (theSpeed > 0.0f && theLeast > theSpeed)
		return theLeast;
	return theSpeed;
}

} // namespace

bool FishState::isGone()
{
	bool inView = mFish->isInView();
	if (mFish->wasSeen() && !inView)
		return true;
	mFish->setSeen(inView);
	return false;
}

AIFishState::AIFishState(AIFish* theFish) : FishState(theFish), mAIFish(theFish) {}

SpawnAI::SpawnAI(AIFish* theFish) : AIFishState(theFish)
{
	mEdible = true;
	mInPlay = false;
}

void EvacuateAI::enter()
{
	mAIFish->setLeaving(true);
	RectF anArea = mAIFish->getStage()->getArea(mFish->isUnderwater());
	const Vector2f& aPosition = mAIFish->getPosition();
	Vector2f aVelocity(0.0f, 0.0f);
	// 0x459e00 (feedingfrenzy2): the width, kept at the x87's precision
	float aMiddle = (float)(((double)anArea.right - anArea.left) * HALF + anArea.left);
	if (aPosition.x <= aMiddle)
		aVelocity.x = -mAIFish->getMaxSpeed().x;
	else
		aVelocity.x = mAIFish->getMaxSpeed().x;
	aVelocity.x = (float)(aVelocity.x * EVACUATE_SPEED);
	mAIFish->setDesiredVelocity(aVelocity);
}

void EvacuateAI::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
	if (!mAIFish->isInView())
		mAIFish->getStateMgr().changeState(nullptr);
}

void SpawnAI::enter()
{
	mEntryArea = mAIFish->getBounds();
	mEntryArea.left = (float)((double)mEntryArea.left + SPAWN_MARGIN);
	mEntryArea.right = (float)((double)mEntryArea.right - SPAWN_MARGIN);
	FishVisual* aVisual = mAIFish->getFishVisual();
	aVisual->setVisible(true);
	aVisual->changeState(aVisual->getSwimState());
}

void SpawnAI::exit()
{
	mFish->setSeen(true);
}

void SpawnAI::update(UpdateContext* theContext)
{
	const Vector2f& aPosition = mFish->getPosition();
	Vector2f aVelocity(0.0f, 0.0f);
	const Vector2f& aMaxSpeed = mFish->getMaxSpeed();
	RectF anArea = mAIFish->getStage()->getArea(mAIFish->isUnderwater());
	if (!mInPlay)
		mInPlay = anArea.contains(aPosition.x, aPosition.y);
	if (mEntryArea.left > aPosition.x)
		aVelocity.x = aMaxSpeed.x;
	else if (mEntryArea.right < aPosition.x)
		aVelocity.x = -aMaxSpeed.x;
	if (lengthSquared(aVelocity) <= 0.0)
	{
		mAIFish->pickState();
		return;
	}
	mAIFish->setDesiredVelocity(aVelocity);
	mAIFish->move(*theContext);
}

WanderAI::WanderAI(AIFish* theFish)
    : AIFishState(theFish), mDartWeight(0.0f), mCenterWeight(0.0f), mRandomWeight(0.0f), mReverseWeight(0.0f),
      mWeightTotal(0.0f), mAccelRate(0.0f)
{
	mInPlay = true;
	mEdible = true;
}

void WanderAI::enter()
{
	mArea = mAIFish->getBounds();
	mArea.left = (float)((double)mArea.left + WANDER_MARGIN_X);
	mArea.right = (float)((double)mArea.right - WANDER_MARGIN_X);
	mArea.top = (float)((double)mArea.top + WANDER_MARGIN_Y);
	mArea.bottom = (float)((double)mArea.bottom - WANDER_MARGIN_Y);
	mAIFish->getFishVisual()->setVisible(true);
	FishBehaviorInfo* aBehavior = mAIFish->getFishBehavior();
	mSightTime.start(aBehavior->mSightCheckFrequency);
	mStateTime.start(drawIn(aBehavior->mWanderStateChangeFrequency));
	mDartWeight = aBehavior->getFloat("dartFreq", DEFAULT_DART_WEIGHT);
	mCenterWeight = aBehavior->getFloat("centerFreq", DEFAULT_CENTER_WEIGHT);
	mRandomWeight = aBehavior->getFloat("randomFreq", DEFAULT_RANDOM_WEIGHT);
	mReverseWeight = aBehavior->getFloat("reverseFreq", DEFAULT_REVERSE_WEIGHT);
	mWeightTotal = (float)((double)mCenterWeight + mDartWeight + mRandomWeight + mReverseWeight);
	mAccelRate = mFish->getAccelRate();
	// Each weight from here on the sum of those up to it, for the draw
	mCenterWeight = (float)((double)mCenterWeight + mDartWeight);
	mRandomWeight = (float)((double)mRandomWeight + mCenterWeight);
	mReverseWeight = (float)((double)mReverseWeight + mRandomWeight);
	mMoveName = "none";
	pickMove();
}

void WanderAI::exit()
{
	if (mAIFish != nullptr)
		mAIFish->setAccelRate(mAccelRate);
}

void WanderAI::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
	float anElapsed = theContext->mElapsed;
	if (mMoveTime.advance(anElapsed))
	{
		mMoveTime.mNext++;
		pickMove();
		return;
	}
	if (mSightTime.advance(anElapsed))
	{
		mSightTime.mNext++;
		mSightTime.restart();
		mAIFish->lookAround();
		return;
	}
	if (mStateTime.advance(anElapsed))
	{
		mStateTime.mNext++;
		mAIFish->pickState();
	}
}

void WanderAI::pickMove()
{
	const Vector2f& aPosition = mAIFish->getPosition();
	// The area's half size, which the original takes for its centre
	float aHalfHeight = (float)(((double)mArea.bottom - mArea.top) * HALF);
	float aHalfWidth = (float)(((double)mArea.right - mArea.left) * HALF);
	Vector2f aCenter(aHalfWidth, aHalfHeight);
	const Vector2f& aMinSpeed = mAIFish->getMinSpeed();
	const Vector2f& aMaxSpeed = mAIFish->getMaxSpeed();
	FishBehaviorInfo* aBehavior = mAIFish->getFishBehavior();
	float aSeconds = drawIn(aBehavior->mWanderMoveChangeFrequency);
	float aDraw = (float)randomFloat(0.0f, mWeightTotal);
	Vector2f aVelocity;
	if (mDartWeight > aDraw)
	{
		mMoveName = "dart";
		aVelocity.x = (float)randomFloat(MINUS_ONE, 1.0f);
		aVelocity.y = (float)randomFloat(MINUS_ONE, 1.0f);
		float aLow = (float)((double)aMaxSpeed.x * DART_LOW);
		float aHigh = (float)((double)aMaxSpeed.x * DART_HIGH);
		float aSpeed = (float)randomFloat(aLow, aHigh);
		aVelocity = scaled(aVelocity, aSpeed);
		mAIFish->setAccelRate((float)((double)mAccelRate + mAccelRate));
		aSeconds = aBehavior->getFloat("dartDuration", DEFAULT_DART_SECONDS);
	}
	else if (mCenterWeight > aDraw)
	{
		float aDirection = aCenter.x > aPosition.x ? 1.0f : MINUS_ONE;
		float aLow = (float)((double)aMinSpeed.x * aDirection);
		float aHigh = (float)((double)aMaxSpeed.x * aDirection);
		aVelocity.x = (float)randomFloat(aLow, aHigh);
		aVelocity.y = (float)randomFloat((float)((double)aMaxSpeed.y * MINUS_ONE_DOUBLE), aMaxSpeed.y);
		float aDistance = (float)fabs((double)(float)((double)aCenter.x - aPosition.x));
		if ((float)((double)aCenter.x * HALF) < aDistance)
			aVelocity.x = (float)((double)aVelocity.x * FAR_CENTER_BOOST);
		mMoveName = "center";
	}
	else if (mRandomWeight > aDraw)
	{
		aVelocity.x = (float)randomFloat((float)((double)aMaxSpeed.x * MINUS_ONE_DOUBLE), aMaxSpeed.x);
		aVelocity.y = (float)randomFloat((float)((double)aMaxSpeed.y * MINUS_ONE_DOUBLE), aMaxSpeed.y);
		mMoveName = "random";
	}
	else
	{
		float aDirection = 0.0f < mFish->getVelocity().x ? MINUS_ONE : 1.0f;
		float aLow = (float)((double)aMinSpeed.x * aDirection);
		float aHigh = (float)((double)aMaxSpeed.x * aDirection);
		aVelocity.x = (float)randomFloat(aLow, aHigh);
		aVelocity.y = (float)randomFloat((float)((double)aMaxSpeed.y * MINUS_ONE_DOUBLE), aMaxSpeed.y);
		mMoveName = "reverse";
	}
	// The way the move goes over its time, its speed up or down cut to stay within the area's top and bottom
	Vector2f aWay((float)((double)aVelocity.x * aSeconds), (float)((double)aSeconds * aVelocity.y));
	Vector2f anEnd((float)((double)aWay.x + aPosition.x), (float)((double)aWay.y + aPosition.y));
	if (mArea.bottom < anEnd.y)
	{
		float aShare = (float)(((double)mArea.bottom - aPosition.y) / aWay.y);
		aVelocity.y = (float)((double)aShare * aVelocity.y);
	}
	else if (mArea.top > anEnd.y)
	{
		float aShare = (float)(((double)mArea.top - aPosition.y) / aWay.y);
		aVelocity.y = (float)((double)aShare * aVelocity.y);
	}
	aVelocity.x = atLeast(aVelocity.x, aMinSpeed.x);
	aVelocity.y = atLeast(aVelocity.y, aMinSpeed.y);
	if ((float)fabs(aVelocity.y) < SLOWEST_RISE)
		aVelocity.y = 0.0f;
	mAIFish->setDesiredVelocity(aVelocity);
	mMoveTime.mNext = 0;
	mMoveTime.mTime = 0.0f;
	mMoveTime.mOn = false;
	mMoveTime.start(aSeconds);
}

FlyByAI::FlyByAI(AIFish* theFish) : AIFishState(theFish), mRightward(false)
{
	mInPlay = true;
	mEdible = true;
}

void FlyByAI::enter()
{
	RectF anArea = mAIFish->getStage()->getArea(mFish->isUnderwater());
	double aHomeX = mAIFish->getHome().x;
	float aMiddle = (float)(((double)anArea.right - anArea.left) * HALF + anArea.left);
	mRightward = aMiddle > aHomeX;
	setVelocity();
	mAIFish->getFishVisual()->setVisible(true);
	mSightTime.start(mAIFish->getFishBehavior()->mSightCheckFrequency);
}

void FlyByAI::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
	float anElapsed = theContext->mElapsed;
	if (mMoveTime.advance(anElapsed))
	{
		mMoveTime.mNext++;
		setVelocity();
	}
	if (mSightTime.advance(anElapsed))
	{
		mSightTime.mNext++;
		mSightTime.restart();
		mAIFish->lookAround();
	}
}

void FlyByAI::setVelocity()
{
	const Vector2f& aMaxSpeed = mAIFish->getMaxSpeed();
	float aRise = (float)randomFloat(-aMaxSpeed.y, aMaxSpeed.y);
	Vector2f aVelocity(aMaxSpeed.x, aRise);
	float aDirection = mRightward ? 1.0f : MINUS_ONE;
	aVelocity.x = (float)((double)aDirection * aVelocity.x);
	mAIFish->setDesiredVelocity(aVelocity);
	mMoveTime.mNext = 0;
	mMoveTime.mTime = 0.0f;
	mMoveTime.mOn = false;
	mMoveTime.start(drawIn(mAIFish->getFishBehavior()->mWanderMoveChangeFrequency));
}

FleeAI::FleeAI(AIFish* theFish, Actor* theThreat) : AIFishState(theFish), mThreat(theThreat)
{
	mInPlay = true;
	mEdible = true;
}

void FleeAI::enter()
{
	mAIFish->getFishVisual()->setVisible(true);
	FishBehaviorInfo* aBehavior = mAIFish->getFishBehavior();
	mSightTime.start(aBehavior->mSightCheckFrequency);
	mStateTime.start(drawIn(aBehavior->mRunStateChangeFrequency));
	pickMove();
}

void FleeAI::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
	float anElapsed = theContext->mElapsed;
	if (mMoveTime.advance(anElapsed))
	{
		mMoveTime.mNext++;
		pickMove();
		return;
	}
	if (mStateTime.advance(anElapsed))
	{
		mStateTime.mNext++;
		mAIFish->pickState();
		return;
	}
	if (mSightTime.advance(anElapsed))
	{
		mSightTime.mNext++;
		mSightTime.restart();
		mAIFish->lookAround();
	}
}

void FleeAI::pickMove()
{
	if (mThreat == nullptr || mThreat->isDead())
	{
		mAIFish->pickState();
		return;
	}
	Vector2f aWay = difference(mAIFish->getPosition(), mThreat->getPosition());
	normalize(aWay);
	const Vector2f& aMaxSpeed = mAIFish->getMaxSpeed();
	float aRise = (float)((double)aMaxSpeed.y * aWay.y * FLEE_SPEEDUP);
	float anAcross = (float)((double)aMaxSpeed.x * aWay.x * FLEE_SPEEDUP);
	mAIFish->setDesiredVelocity(Vector2f(anAcross, aRise));
	mMoveTime.mNext = 0;
	mMoveTime.mTime = 0.0f;
	mMoveTime.mOn = false;
	mMoveTime.start(drawIn(mAIFish->getFishBehavior()->mRunMoveChangeFrequency));
}

ChaseAI::ChaseAI(AIFish* theFish, Actor* thePrey) : AIFishState(theFish), mPrey(thePrey)
{
	mInPlay = true;
	mEdible = true;
}

void ChaseAI::enter()
{
	mAIFish->getFishVisual()->setVisible(true);
	FishBehaviorInfo* aBehavior = mAIFish->getFishBehavior();
	mSightTime.start(aBehavior->mSightCheckFrequency);
	mStateTime.start(drawIn(aBehavior->mChaseStateChangeFrequency));
	pickMove();
}

void ChaseAI::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
	float anElapsed = theContext->mElapsed;
	if (mMoveTime.advance(anElapsed))
	{
		mMoveTime.mNext++;
		pickMove();
		return;
	}
	if (mStateTime.advance(anElapsed))
	{
		mStateTime.mNext++;
		mAIFish->pickState();
	}
}

void ChaseAI::pickMove()
{
	if (mPrey == nullptr || mPrey->isDead() || mPrey->canEat(mAIFish))
	{
		mAIFish->pickState();
		return;
	}
	Vector2f aWay = difference(mPrey->getPosition(), mAIFish->getMouthPosition());
	normalize(aWay);
	const Vector2f& aMaxSpeed = mAIFish->getMaxSpeed();
	float aRise = (float)((double)aMaxSpeed.y * aWay.y);
	float anAcross = (float)((double)aMaxSpeed.x * aWay.x);
	mAIFish->setDesiredVelocity(Vector2f(anAcross, aRise));
	mMoveTime.mNext = 0;
	mMoveTime.mTime = 0.0f;
	mMoveTime.mOn = false;
	mMoveTime.start(drawIn(mAIFish->getFishBehavior()->mChaseMoveChangeFrequency));
}

AvoidAI::AvoidAI(AIFish* theFish, Actor* theThreat) : AIFishState(theFish), mThreat(theThreat)
{
	mInPlay = true;
	mEdible = true;
}

void AvoidAI::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
	if (mThreat->isDead())
	{
		mAIFish->pickState();
		return;
	}
	float aDistance = mAIFish->distanceInSight(mThreat.get());
	if (!(aDistance >= NOTHING))
	{
		mAIFish->pickState();
		return;
	}
	if (isHeadedAtThreat())
		turnAside();
}

bool AvoidAI::isHeadedAtThreat()
{
	if (mThreat == nullptr || mThreat->isDead())
		return false;
	Vector2f aToThreat = difference(mThreat->getPosition(), mAIFish->getPosition());
	normalize(aToThreat);
	const Vector2f& aWanted = mAIFish->getDesiredVelocity();
	Vector2f aHeading;
	if (0.0 == lengthSquared(aWanted))
		aHeading = Vector2f(mAIFish->getFishVisual()->getFacing(), 0.0f);
	else
	{
		aHeading = aWanted;
		normalize(aHeading);
	}
	float aCos = (float)fabs(dot(aHeading, aToThreat));
	return aCos > HEADED_AT_COS;
}

void AvoidAI::turnAside()
{
	Vector2f aToThreat = difference(mThreat->getPosition(), mAIFish->getPosition());
	const Vector2f& aWanted = mAIFish->getDesiredVelocity();
	Vector2f aHeading;
	if (0.0 == lengthSquared(aWanted))
		aHeading = Vector2f(mAIFish->getFishVisual()->getFacing(), 0.0f);
	else
		aHeading = aWanted;
	// The threat's side of the heading, by the sign of their cross product
	float aSide = (float)((double)aToThreat.y * aHeading.x - (double)aToThreat.x * aHeading.y);
	float anAngle = aSide >= NOTHING ? AVOID_TURN_BACK : AVOID_TURN;
	mAIFish->setDesiredVelocity(rotated(aWanted, anAngle));
}

void AIFishState::eat(Actor* thePrey)
{
	mAIFish->changeState(new AIEatingState(mAIFish));
}

EatingState::EatingState(Fish* theFish) : FishState(theFish), mChomped(false)
{
	mInPlay = true;
	mEdible = true;
}

void EatingState::enter()
{
	FishVisual* aVisual = mFish->getFishVisual();
	aVisual->mChomp.connect(this, &EatingState::onChomp);
	// 0x418c90 (feedingfrenzy2): the visual's state's eat
	aVisual->getState()->eat();
	mChomped = false;
}

void EatingState::exit()
{
	mFish->getFishVisual()->mChomp.disconnect(this);
	if (!mChomped)
		mFish->swallow();
}

void EatingState::onChomp()
{
	mChomped = true;
	mFish->swallow();
}

AIEatingState::AIEatingState(AIFish* theFish) : EatingState(theFish), mAIFish(theFish) {}

void AIEatingState::update(UpdateContext* theContext)
{
	mAIFish->move(*theContext);
}

void AIEatingState::onChomp()
{
	EatingState::onChomp();
	mAIFish->pickState();
}

void EatenState::enter()
{
	mStart = mFish->getPosition();
	FishVisual* aVisual = mFish->getFishVisual();
	mStartScale = aVisual->getPulseScale();
	mElapsed = 0.0f;
	if (0.0f == mSeconds)
		hide();
}

void EatenState::update(UpdateContext* theContext)
{
	float anElapsed = (float)((double)theContext->mElapsed + mElapsed);
	mElapsed = anElapsed;
	if (!(mSeconds > anElapsed))
	{
		hide();
		return;
	}
	Vector2f aMouth = mEater->getMouthPosition();
	float aShare = (float)((double)mElapsed / mSeconds);
	float aY = (float)(((double)aMouth.y - mStart.y) * aShare + mStart.y);
	float anX = (float)(((double)aMouth.x - mStart.x) * aShare + mStart.x);
	mFish->setPosition(anX, aY);
	mFish->getFishVisual()->setPulseScale((float)((double)mStartScale - (double)aShare * mStartScale));
}

void EatenState::hide()
{
	FishVisual* aVisual = mFish->getFishVisual();
	aVisual->setVisible(false);
	aVisual->setPulseScale(1.0f);
}

SchoolOfFish::SchoolOfFish()
    : mCenter(0.0f, 0.0f), mAverageVelocity(0.0f, 0.0f), mSeparationDistance(0.0f), mSeparationDistanceSquared(0.0f),
      mNeighborDistance(0.0f), mNeighborDistanceSquared(0.0f), mSeparationFactor(0.0f), mCohesionFactor(0.0f),
      mAlignmentFactor(0.0f), mGoalFactor(0.0f), mFleeFactor(0.0f), mChaseMultiplier(0.0f), mGoal(0.0f, 0.0f),
      mFleeing(false), mFlyBy(false), mCount(0), mLastFrame(0)
{
}

void SchoolOfFish::setBehavior(FishBehaviorInfo* theBehavior)
{
	mSeparationDistance = theBehavior->mSchoolSeparationDistance;
	mSeparationDistanceSquared = (float)((double)mSeparationDistance * mSeparationDistance);
	mNeighborDistance = theBehavior->mSchoolNeighborDistance;
	mNeighborDistanceSquared = (float)((double)mNeighborDistance * mNeighborDistance);
	mSeparationFactor = theBehavior->mSchoolSeparationFactor;
	mAlignmentFactor = theBehavior->mSchoolAlignmentFactor;
	mCohesionFactor = theBehavior->mSchoolCohesionFactor;
	mGoalFactor = theBehavior->mSchoolGoalFactor;
	mFleeFactor = theBehavior->mSchoolFleeFactor;
	mChaseMultiplier = theBehavior->mSchoolChaseMultiplier;
	mMoveChangeFrequency = theBehavior->mSchoolMoveChangeFrequency;
	mFlyBy = theBehavior->mBaseBehavior == "flyBy";
	mGoalTime.start(0.0f);
	mFleeing = false;
}

void SchoolOfFish::addFish(AIFish* theFish)
{
	mFish.push_back(theFish);
	theFish->setSchool(this);
	mCount++;
}

void SchoolOfFish::update(UpdateContext* theContext)
{
	mLastFrame = theContext->mFrame;
	findCenter();
	bool atGoal = false;
	if (mGoalTime.mOn && mGoalTime.mNext < mGoalTime.mTimes.size())
		atGoal = lengthSquared(difference(mGoal, mCenter)) < GOAL_REACHED_SQUARED;
	bool aTimeCame = mGoalTime.advance(theContext->mElapsed);
	if (aTimeCame)
		mGoalTime.mNext++;
	if (aTimeCame || atGoal)
		pickGoal();
}

void SchoolOfFish::pickGoal()
{
	RectF aBounds = mFish.front()->getBounds();
	if (mFlyBy)
	{
		double aWidth = (double)aBounds.right - aBounds.left;
		if (aWidth * HALF > (double)mCenter.x)
			mGoal.x = (float)(aWidth + FLY_BY_GOAL_BEYOND);
		else
			mGoal.x = FLY_BY_GOAL_BEFORE;
		float aHigh = (float)((double)aBounds.bottom - FLY_BY_GOAL_MARGIN);
		float aLow = (float)(FLY_BY_GOAL_MARGIN + aBounds.top);
		mGoal.y = (float)randomFloat(aLow, aHigh);
		mGoalTime.mOn = false;
	}
	else
	{
		float aHigh = (float)((double)aBounds.right - WANDER_GOAL_MARGIN);
		float aLow = (float)(WANDER_GOAL_MARGIN + aBounds.left);
		mGoal.x = (float)randomFloat(aLow, aHigh);
		aHigh = (float)((double)aBounds.bottom - WANDER_GOAL_MARGIN);
		aLow = (float)(WANDER_GOAL_MARGIN + aBounds.top);
		mGoal.y = (float)randomFloat(aLow, aHigh);
		mGoalTime.start(drawIn(mMoveChangeFrequency));
	}
	mGoalTime.mTime = 0.0f;
	mGoalTime.mNext = 0;
	mFleeing = false;
}

void SchoolOfFish::startFleeing()
{
	if (!mFleeing)
	{
		mFleeing = true;
		mGoalTime.start((float)randomFloat(FRIGHT_SHORTEST, FRIGHT_LONGEST));
	}
	mGoalTime.restart();
}

AIFish* SchoolOfFish::findNearest(AIFish* theFish)
{
	const Vector2f& aPosition = theFish->getPosition();
	AIFish* aNearest = nullptr;
	float aNearestSquared = 0.0f;
	for (AIFish* anOther : mFish)
	{
		if (anOther == theFish)
			continue;
		float aSquared = (float)lengthSquared(difference(anOther->getPosition(), aPosition));
		if (aNearest == nullptr || aNearestSquared > aSquared)
		{
			aNearest = anOther;
			aNearestSquared = aSquared;
		}
	}
	return aNearest;
}

void SchoolOfFish::findCenter()
{
	mCenter = Vector2f(0.0f, 0.0f);
	mAverageVelocity = Vector2f(0.0f, 0.0f);
	for (AIFish* aFish : mFish)
	{
		const Vector2f& aPosition = aFish->getPosition();
		mCenter = moved(mCenter, aPosition.x, aPosition.y);
		const Vector2f& aVelocity = aFish->getVelocity();
		mAverageVelocity = moved(mAverageVelocity, aVelocity.x, aVelocity.y);
	}
	if (!mFish.empty())
	{
		float aShare = (float)(1.0 / (double)(int)mFish.size());
		mCenter = scaled(mCenter, aShare);
		mAverageVelocity = scaled(mAverageVelocity, aShare);
	}
}

SchoolingAI::SchoolingAI(AIFish* theFish) : AIFishState(theFish)
{
	mInPlay = true;
	mEdible = true;
}

void SchoolingAI::enter()
{
	mPullTime.start(0.0f);
}

void SchoolingAI::update(UpdateContext* theContext)
{
	SchoolOfFish* aSchool = mAIFish->getSchool();
	if (aSchool->getFish().size() <= 1)
	{
		mAIFish->pickState();
		return;
	}
	if (theContext->mFrame > aSchool->getLastFrame())
		aSchool->update(theContext);
	if (mPullTime.advance(theContext->mElapsed))
	{
		mPullTime.mNext++;
		mPullTime.start((float)randomFloat(PULL_SHORTEST, PULL_LONGEST));
		mSeparation = findSeparation();
		mAlignment = findAlignment();
		const Vector2f& aPosition = mAIFish->getPosition();
		mCohesion = scaled(difference(aSchool->getCenter(), aPosition), aSchool->getCohesionFactor());
		mGoalPull = scaled(difference(aSchool->getGoal(), aPosition), aSchool->getGoalFactor());
		mFlee = findFlee();
		Vector2f aTopSpeed = mAIFish->getMaxSpeed();
		float anAccelRate = mAIFish->getFishBehavior()->mAccelRate;
		if (lengthSquared(mFlee) > NOTHING)
			aSchool->startFleeing();
		if (aSchool->isFleeing())
		{
			aTopSpeed = scaled(aTopSpeed, aSchool->getFleeFactor());
			anAccelRate = (float)((double)anAccelRate + anAccelRate);
			mGoalPull = scaled(mGoalPull, FLEE_GOAL_SHARE);
		}
		mForce = Vector2f(0.0f, 0.0f);
		mForce = moved(mForce, mSeparation.x, mSeparation.y);
		mForce = moved(mForce, mAlignment.x, mAlignment.y);
		mForce = moved(mForce, mCohesion.x, mCohesion.y);
		mForce = moved(mForce, mFlee.x, mFlee.y);
		mForce = moved(mForce, mGoalPull.x, mGoalPull.y);
		mAIFish->setDragRate(0.0f);
		mAIFish->setMaxForce(Vector2f(anAccelRate, anAccelRate));
		mAIFish->setMaxVelocity(aTopSpeed);
	}
	mAIFish->addForce(mForce.x, mForce.y);
	mAIFish->move(*theContext);
	mAIFish->clearTurned();
}

Vector2f SchoolingAI::findSeparation()
{
	Vector2f aPull(0.0f, 0.0f);
	SchoolOfFish* aSchool = mAIFish->getSchool();
	AIFish* aNearest = aSchool->findNearest(mAIFish);
	if (aNearest == nullptr)
		return aPull;
	Vector2f aWay = difference(mAIFish->getPosition(), aNearest->getPosition());
	float aSquared = (float)lengthSquared(aWay);
	if (!(aSchool->getSeparationDistanceSquared() > aSquared))
		return aPull;
	if (!(aSquared > SEPARATION_LEAST_SQUARED))
		aSquared = 1.0f;
	double aStrength = (double)aSchool->getSeparationFactor() * aSchool->getSeparationDistance();
	float aDistance = (float)sqrt((double)aSquared);
	float aScale = (float)(aStrength / aDistance);
	aWay = scaled(aWay, aScale);
	return moved(aPull, aWay.x, aWay.y);
}

Vector2f SchoolingAI::findAlignment()
{
	Vector2f aPull(0.0f, 0.0f);
	SchoolOfFish* aSchool = mAIFish->getSchool();
	int aCount = 0;
	for (AIFish* anOther : aSchool->getFish())
	{
		if (anOther == mAIFish)
			continue;
		const Vector2f& aVelocity = anOther->getVelocity();
		aPull = moved(aPull, aVelocity.x, aVelocity.y);
		aCount++;
	}
	float aShare = aCount > 0 ? (float)(1.0 / (double)aCount) : 0.0f;
	aPull = scaled(aPull, aShare);
	aPull = difference(aPull, mAIFish->getVelocity());
	return scaled(aPull, aSchool->getAlignmentFactor());
}

Vector2f SchoolingAI::findFlee()
{
	SchoolOfFish* aSchool = mAIFish->getSchool();
	const std::vector<Actor*>& aSeen = mAIFish->getStage()->findSeenActors(mAIFish);
	Vector2f aPull(0.0f, 0.0f);
	for (Actor* anActor : aSeen)
	{
		Vector2f aWay = difference(mAIFish->getPosition(), anActor->getPosition());
		if (!anActor->isOfClass("predator", mAIFish))
			continue;
		float aDistance = length(aWay);
		if (!(aDistance > NEAREST_THREAT))
			aDistance = NEAREST_THREAT_DISTANCE;
		double aStrength = (double)mAIFish->getFishBehavior()->mSightDistance * aSchool->getFleeFactor();
		float aScale = (float)(aStrength / ((double)aDistance + aDistance));
		aWay = scaled(aWay, aScale);
		aPull = moved(aPull, aWay.x, aWay.y);
	}
	return aPull;
}
