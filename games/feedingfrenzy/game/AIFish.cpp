#include "AIFish.h"
#include "App.h"
#include "FishFood.h"
#include "FishSpawner.h"
#include "FishSchool.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"

using namespace engine;

namespace feedingfrenzy
{

// The line down the middle of the stage that a cruising fish swims away from
const float CRUISE_SIDE_X = 320.0f;
// A fish put at the edge swims in until it is this far inside the stage
const float ENTER_MARGIN = 80.0f;
// The seconds a fish cruises on a heading before it picks its depth again, and how long it cruises
const float CRUISE_DEPTH_SECONDS_LOW = 10.0f;
const float CRUISE_DEPTH_SECONDS_HIGH = 15.0f;
const float CRUISE_SECONDS_LOW = 6.0f;
const float CRUISE_SECONDS_HIGH = 10.0f;
// A fish that has decided to wander does so for this part of the time (a number from 0 to 10 below it)
const int WANDER_CHANCE_OF_TEN = 8;
// The longest time a fish avoids another
const float AVOID_SECONDS_LOW = 1.0f;
const float AVOID_SECONDS_HIGH = 5.0f;
// A fish that avoids another turns its heading aside by this angle when the other is this much ahead or behind
const float AVOID_TURN_DEGREES = 30.0f;
const float AVOID_AHEAD_COSINE = 0.7f;
// The speed a fish runs at, as a multiple of its greatest, and the time it keeps a heading
const float RUN_SPEED_FACTOR = 2.0f;
const float RUN_DECISION_SECONDS = 2.0f;
// A fish that leaves swims off at this multiple of its speed
const float LEAVE_SPEED_FACTOR = 2.5f;
// The stage's edges a wandering fish turns away from
const float WANDER_RIGHT_EDGE = 620.0f;
const float WANDER_LEFT_EDGE = 30.0f;
const float WANDER_BOTTOM_EDGE = 400.0f;
const float WANDER_TOP_EDGE = 40.0f;
// A wandering fish that would swim up or down more slowly than this holds its depth
const float WANDER_LEAST_VERTICAL_SPEED = 10.0f;
// How slowly a fish slows down while it is stunned, and while it is frozen or alert (pixels a second a second)
const float STUN_SLOW_DOWN = 200.0f;
const float HOLD_SLOW_DOWN = 280.0f;
// The seconds a fish that is being eaten keeps its states for (more than the stage takes)
const float BEING_EATEN_STATE_SECONDS = 99999.0f;
// How long a fish takes to turn from what it was eaten by to finish being eaten (the base's time that it ignores)
const float BEING_EATEN_BASE_SECONDS = 0.2f;
// The volume of the sound of a fish being eaten, by the player and by another fish
const float EATEN_BY_PLAYER_VOLUME = 1.0f;
const float EATEN_BY_FISH_VOLUME = 0.6f;
// How often a fish looks at the others, in seconds
const float REACTION_INTERVAL = 0.1f;
// The farthest a fish stands from the subject of its alert before it follows it, and the margins of its sight of it
const float ALERT_SPAN_MARGIN = 30.0f;
const float ALERT_HOLD_DISTANCE = 25.0f;
const float ALERT_DEPTH_MARGIN = 50.0f;
const float ALERT_SPEED_FACTOR = 0.5f;
// The part of its stage's width that a fish in a school keeps to the stage's edge before it turns
const float SCHOOL_PUSH_FACTOR = 1.4f;
const float SCHOOL_SPEED_LIMIT_FACTOR = 2.6f;
const float SCHOOL_DECISION_LOW = 0.1f;
const float SCHOOL_DECISION_HIGH = 0.2f;
// The thought bubble grows by twice the frame's seconds a frame
const float THOUGHT_GROWTH_FACTOR = 2.0f;
// The stun effect's file
const char* const STUN_EFFECT_FILE = "config\\Fx\\stunnedEffect.xml";

// 0x402190
AIFish::AIFish()
    : mStateSeconds(0.0f), mDecisionSeconds(0.0f), mReactionSeconds(REACTION_INTERVAL), mStunSeconds(0.0f),
      mSecondsInState(0.0f), mLastState(AI_NONE), mState(AI_ENTER), mSpawner(nullptr), mSightCheckRange(0.0f),
      mTarget(nullptr), mStunEffect(nullptr), mLungeFactor(2.0f), mAlertSubject(nullptr), mSavedSlowDown(0.0f),
      mReturnState(0), mReturnSeconds(0.0f), mThoughtNode(nullptr), mSavedState(0), mLeafSeconds(0.0f)
{
	setVision(65.0f, 60.0f);
	mBeingEatenLeft = -1.0f;
	mReactable = true;
}

// 0x401000
AIFish::~AIFish() {}

// 0x4010a0
void AIFish::update(UpdateInfo& theInfo)
{
	if (mState == mLastState)
	{
		mSecondsInState += theInfo.mSeconds;
	}
	else
	{
		mSecondsInState = 0.0f;
		mLastState = mState;
	}

	if (!mInPlay)
		return;

	if (mAnimState == ANIM_EAT)
	{
		float aSpeed = mLungeFactor * mMaxSpeedX;
		if (0.0f < mScale.x)
			aSpeed = -aSpeed;
		mDesiredVelocity.x = aSpeed;
		FishBase::update(theInfo);
		return;
	}

	switch (mState)
	{
	case AI_ENTER:
		updateEnter(theInfo);
		break;
	case AI_AVOID:
		updateAvoid(theInfo);
		break;
	case AI_CRUISE:
		updateCruise(theInfo);
		break;
	case AI_CHASE:
		updateChase(theInfo);
		break;
	case AI_WANDER:
	case AI_WANDER_UNUSED:
	case AI_NONE:
		updateWander(theInfo);
		break;
	case AI_RUN:
		updateRun(theInfo);
		break;
	case AI_SPECIAL:
		updateSpecial(theInfo);
		break;
	case AI_STUNNED:
		updateStunned(theInfo);
		break;
	case AI_SCHOOL:
		updateSchool(theInfo);
		break;
	case AI_FROZEN:
		updateFrozen(theInfo);
		break;
	case AI_ALERT:
		updateAlert(theInfo);
		break;
	case AI_ALERT_RETURN:
		updateAlertReturn(theInfo);
		break;
	case AI_LEAVE:
		updateLeaving(theInfo);
		break;
	case AI_BEING_EATEN:
		updateBeingEaten(theInfo);
		break;
	}

	mReactionSeconds -= theInfo.mSeconds;
	checkReactions();
}

// 0x401750
void AIFish::updateEnter(UpdateInfo& theInfo)
{
	updatePlacedRects();

	if (ENTER_MARGIN <= mNose.x)
	{
		int aWorldWidth = FishFood::instance()->getStageMgr()->getWorldWidth();
		if (mNose.x <= (float)(aWorldWidth - (int)ENTER_MARGIN))
			decide();
		else
			mDesiredVelocity.x = -mMaxSpeedX;
	}
	else
	{
		mDesiredVelocity.x = mMaxSpeedX;
	}

	FishBase::update(theInfo);
}

// 0x401300
void AIFish::updateCruise(UpdateInfo& theInfo)
{
	mDecisionSeconds -= theInfo.mSeconds;
	mStateSeconds -= theInfo.mSeconds;

	if (!isInStage())
	{
		takeOutOfPlay();
		return;
	}

	if (mDecisionSeconds < 0.0f)
	{
		mDesiredVelocity.y = randomFloat(Point(-mMaxSpeedY, mMaxSpeedY));
		mDecisionSeconds = randomFloat(Point(CRUISE_DEPTH_SECONDS_LOW, CRUISE_DEPTH_SECONDS_HIGH));
	}

	if (mStateSeconds < 0.0f)
		decide();

	FishBase::update(theInfo);
}

// 0x4013c0
void AIFish::updateChase(UpdateInfo& theInfo)
{
	mDecisionSeconds -= theInfo.mSeconds;
	mStateSeconds -= theInfo.mSeconds;
	if (mStateSeconds < 0.0f)
	{
		decide();
		return;
	}

	if (mTarget != nullptr && mTarget->isInPlay() && mTarget->getLevel() <= mLevel)
	{
		if (mDecisionSeconds < 0.0f)
		{
			Point aToTarget(mTarget->getX(), mTarget->getY());
			aToTarget.subtract(Point(mNose.x, mNose.y));
			aToTarget.normalize();
			aToTarget.scale(mMaxSpeedX);
			mDesiredVelocity = aToTarget;
			mDecisionSeconds = randomFloat(mChaseDecisionRange);
		}
	}
	else
	{
		decide();
	}

	FishBase::update(theInfo);
}

// 0x401570
bool AIFish::canSeePlayer()
{
	return canSee(FishFood::instance()->getStageMgr()->getPlayer());
}

// 0x401590
void AIFish::updateRun(UpdateInfo& theInfo)
{
	if (!isInStage())
	{
		takeOutOfPlay();
		return;
	}

	if (mTarget->getLevel() < mLevel)
	{
		decide();
		FishBase::update(theInfo);
		return;
	}

	mDecisionSeconds -= theInfo.mSeconds;
	if (mDecisionSeconds < 0.0f)
	{
		Point aToTarget(mTarget->getX(), mTarget->getY());
		aToTarget.subtract(Point(mNose.x, mNose.y));
		aToTarget.normalize();
		mDesiredVelocity.x = aToTarget.x * mMaxSpeedX * -RUN_SPEED_FACTOR;
		mDesiredVelocity.y = aToTarget.y * mMaxSpeedY * -RUN_SPEED_FACTOR;
		mDecisionSeconds = randomFloat(Point(RUN_DECISION_SECONDS, RUN_DECISION_SECONDS));
	}

	mStateSeconds -= theInfo.mSeconds;
	if (mStateSeconds < 0.0f)
		decide();

	FishBase::update(theInfo);
}

// 0x401830
void AIFish::updateSpecial(UpdateInfo& theInfo)
{
	beginWander();
	FishBase::update(theInfo);
}

// 0x401850
void AIFish::decide()
{
	if (randomInt(0, 10) < WANDER_CHANCE_OF_TEN)
		beginWander();
	else
		beginCruise();
}

// 0x401880
void AIFish::beginCruise()
{
	if (mSchool != nullptr)
	{
		mSchool->resetDestination();
		beginSchool();
		return;
	}

	if (mState != AI_CRUISE)
	{
		if (getX() >= CRUISE_SIDE_X)
			mDesiredVelocity.x = -mMaxSpeedX;
		else
			mDesiredVelocity.x = mMaxSpeedX;
	}

	mState = AI_CRUISE;
	mDesiredVelocity.y = randomFloat(Point(-mMaxSpeedY, mMaxSpeedY));
	mDecisionSeconds = randomFloat(Point(CRUISE_SECONDS_LOW, CRUISE_SECONDS_HIGH));
	mStateSeconds = randomFloat(Point(CRUISE_SECONDS_LOW, CRUISE_SECONDS_HIGH));
}

// 0x401940
void AIFish::beginWander()
{
	mState = AI_WANDER;
	pickWanderHeading();
	mDecisionSeconds = randomFloat(mWanderDecisionRange);
	mStateSeconds = randomFloat(mWanderStateRange);
}

// 0x401990
void AIFish::beginChase()
{
	mState = AI_CHASE;
	mDecisionSeconds = -1.0f;
	mStateSeconds = randomFloat(mChaseStateRange);
}

// 0x4019d0
void AIFish::beginRun()
{
	mState = AI_RUN;
	mDecisionSeconds = -1.0f;
	mStateSeconds = randomFloat(mRunStateRange);
}

// 0x401a10
void AIFish::beginAvoid()
{
	mState = AI_AVOID;
	mDecisionSeconds = -1.0f;
	mStateSeconds = randomFloat(Point(AVOID_SECONDS_LOW, AVOID_SECONDS_HIGH));
}

// 0x40d8a0
void AIFish::beginSpecial()
{
	mState = AI_SPECIAL;
}

// 0x401a50
bool AIFish::canEat(FishBase* theVictim)
{
	if (mState != AI_STUNNED && (mState < AI_ALERT || AI_ALERT_RETURN < mState) && mInPlay)
		return FishBase::canEat(theVictim);
	return false;
}

// 0x401a80
bool AIFish::canBeEatenByLevel(float theEaterLevel)
{
	return mInPlay && !mBeingEaten && mLevel < theEaterLevel;
}

// 0x401ab0
bool AIFish::canBeEatenBy(FishBase* theEater)
{
	return canBeEatenByLevel(theEater->getLevel());
}

// 0x401b30
void AIFish::updateStunned(UpdateInfo& theInfo)
{
	mStunSeconds -= theInfo.mSeconds;
	FishBase::update(theInfo);
	if (mStunSeconds < 0.0f)
	{
		if (mStunEffect != nullptr)
			mStunEffect->stopEmitting();
		mSlowDown = mSavedSlowDown;
		decide();
	}
}

// 0x401b90
void AIFish::setIdleRanges(float theDecisionLow, float theDecisionHigh, float theStateLow, float theStateHigh)
{
	mWanderDecisionRange.set(theDecisionLow, theDecisionHigh);
	mWanderStateRange.set(theStateLow, theStateHigh);
}

// 0x401bc0
void AIFish::setChaseRanges(float theDecisionLow, float theDecisionHigh, float theStateLow, float theStateHigh)
{
	mChaseDecisionRange.set(theDecisionLow, theDecisionHigh);
	mChaseStateRange.set(theStateLow, theStateHigh);
}

// 0x401bf0
void AIFish::setFleeRanges(float theDecisionLow, float theDecisionHigh, float theStateLow, float theStateHigh)
{
	mRunDecisionRange.set(theDecisionLow, theDecisionHigh);
	mRunStateRange.set(theStateLow, theStateHigh);
}

// 0x401c30
void AIFish::beginSchool()
{
	mState = AI_SCHOOL;
	mDecisionSeconds = randomFloat(Point(0.0f, 0.2f));
}

// 0x401c70
void AIFish::beginLeave()
{
	Point aScreenPosition = FishFood::instance()->getStageMgr()->worldToScreen(Point(mNose.x, mNose.y));
	int aScreenWidth = FishFood::instance()->getStageMgr()->getScreenWidth();
	float aDirection = aScreenPosition.x <= (float)(aScreenWidth / 2) ? -1.0f : 1.0f;

	mState = AI_LEAVE;
	mStunnable = false;
	mDesiredVelocity.x = aDirection * mMaxSpeedX * LEAVE_SPEED_FACTOR;
}

// 0x401d30
void AIFish::updateLeaving(UpdateInfo& theInfo)
{
	if (!isOnScreen())
	{
		takeOutOfPlay();
		return;
	}
	FishBase::update(theInfo);
}

// 0x401d70
void AIFish::endAlert()
{
	if (mState == AI_ALERT)
	{
		mState = AI_ALERT_RETURN;
		mReturnSeconds = 1.0f;
	}
}

// 0x401d90
void AIFish::clearAlert()
{
	if (mAlertSubject != nullptr)
	{
		mAlertSubject = nullptr;
		mSlowDown = mSavedSlowDown;
		decide();
		mDecisionSeconds = 0.0f;
	}
	if (mThoughtNode != nullptr)
		mThoughtNode->setVisible(false);
}

// 0x401de0
void AIFish::updateAlertReturn(UpdateInfo& theInfo)
{
	mReturnSeconds -= theInfo.mSeconds;
	if (mReturnSeconds <= 0.0f)
	{
		mState = mReturnState;
		clearAlert();
	}
	updateThoughtBubble(theInfo.mSeconds);
	FishBase::update(theInfo);
}

// 0x401e50
void AIFish::updateThoughtBubble(float theSeconds)
{
	if (mThoughtNode == nullptr || mState != AI_ALERT)
		return;

	float aX = mScale.x >= 0.0f ? mThought.mOffset.x : (mNose.x - mTail.x) - mThought.mOffset.x;
	mThoughtNode->setPosition(aX, mThought.mOffset.y);

	if (mDesiredVelocity.x == 0.0f && mDesiredVelocity.y == 0.0f)
	{
		if (mAnimState != ANIM_SWIM && mAnimState != ANIM_IDLE)
		{
			mThoughtNode->setVisible(false);
			return;
		}

		mThoughtNode->setVisible(true);
		float aScale = mThoughtNode->mScale.y;
		if (aScale < mThought.mScale)
		{
			aScale = THOUGHT_GROWTH_FACTOR * theSeconds + aScale;
			if (mThought.mScale < aScale)
				aScale = mThought.mScale;
			mThoughtNode->setScale(-aScale, aScale);
		}
	}
}

// 0x401f90
void AIFish::updateFrozen(UpdateInfo& theInfo)
{
	mDesiredVelocity.set(0.0f, 0.0f);
	mVelocity.set(0.0f, 0.0f);
	FishBase::update(theInfo);
}

// 0x401fb0
void AIFish::unfreeze()
{
	if (mState == AI_FROZEN)
	{
		mSlowDown = mSavedSlowDown;
		mState = mSavedState;
		mDesiredVelocity = mSavedVelocity;
		mVelocity = mSavedVelocity;
	}
}

// 0x402330
void AIFish::pickWanderHeading()
{
	float aSpeedX = randomFloat(Point(-mMaxSpeedX, mMaxSpeedX));
	mDesiredVelocity.x = aSpeedX;
	if ((WANDER_RIGHT_EDGE < mNose.x && 0.0f < aSpeedX) || (mNose.x < WANDER_LEFT_EDGE && aSpeedX < 0.0f))
		mDesiredVelocity.x = aSpeedX * -1.0f;
	if (mDesiredVelocity.x < 0.0f && -mMinSpeedX < mDesiredVelocity.x)
		mDesiredVelocity.x = -mMinSpeedX;
	if (0.0f < mDesiredVelocity.x && mDesiredVelocity.x < mMinSpeedX)
		mDesiredVelocity.x = mMinSpeedX;

	float aSpeedY = randomFloat(Point(-mMaxSpeedY, mMaxSpeedY));
	mDesiredVelocity.y = aSpeedY;
	if ((WANDER_BOTTOM_EDGE < mNose.y && 0.0f < aSpeedY) || (mNose.y < WANDER_TOP_EDGE && aSpeedY < 0.0f))
		mDesiredVelocity.y = aSpeedY * -1.0f;
	if (mDesiredVelocity.y < 0.0f && -mMinSpeedY < mDesiredVelocity.y)
		mDesiredVelocity.y = -mMinSpeedY;
	if (0.0f < mDesiredVelocity.y && mDesiredVelocity.y < mMinSpeedY)
		mDesiredVelocity.y = mMinSpeedY;
	if (fabsf(mDesiredVelocity.y) < WANDER_LEAST_VERTICAL_SPEED)
		mDesiredVelocity.y = 0.0f;
}

// 0x401260
void AIFish::updateWander(UpdateInfo& theInfo)
{
	mDecisionSeconds -= theInfo.mSeconds;
	if (!isInStage())
	{
		takeOutOfPlay();
		return;
	}

	if (mDecisionSeconds < 0.0f)
	{
		pickWanderHeading();
		mDecisionSeconds = randomFloat(mWanderDecisionRange);
	}

	mStateSeconds -= theInfo.mSeconds;
	if (mStateSeconds < 0.0f)
		decide();

	FishBase::update(theInfo);
}

// 0x402540
bool AIFish::canSee(FishBase* theFish)
{
	if (mVisionRange <= 0.0f)
		return false;

	Point aToFish(mNose.x, mNose.y);
	aToFish.subtract(*theFish->getPosition());
	if (aToFish.lengthSquared() <= mVisionRangeSquared)
	{
		Point aDirection = aToFish;
		aDirection.normalize();

		Point aHeading;
		if (mVelocity.lengthSquared() == 0.0f)
		{
			aHeading.y = 0.0f;
			aHeading.x = mNose.x <= mTail.x ? -1.0f : 1.0f;
		}
		else
		{
			aHeading = mVelocity;
			aHeading.normalize();
		}

		if (mVisionCosine < fabsf(aDirection.x * aHeading.x + aDirection.y * aHeading.y))
			return true;
	}
	return false;
}

// 0x402710
void AIFish::updateAvoid(UpdateInfo& theInfo)
{
	if (!isInStage())
	{
		takeOutOfPlay();
		return;
	}

	if (mTarget->getLevel() < mLevel)
	{
		decide();
		FishBase::update(theInfo);
		return;
	}

	if (!canSee(mTarget))
	{
		decide();
		FishBase::update(theInfo);
	}

	mDecisionSeconds -= theInfo.mSeconds;
	if (mDecisionSeconds < 0.0f)
	{
		Point aToTarget = mTarget->getNosePosition();
		aToTarget.subtract(getNosePosition());
		aToTarget.normalize();

		Point aHeading;
		if (mVelocity.lengthSquared() == 0.0f)
		{
			aHeading.y = 0.0f;
			aHeading.x = mNose.x <= mTail.x ? -1.0f : 1.0f;
		}
		else
		{
			aHeading = mVelocity;
			aHeading.normalize();
		}

		if (AVOID_AHEAD_COSINE < fabsf(aToTarget.dot(aHeading)))
		{
			float aTurn = 0.0f <= aToTarget.y * aHeading.x - aToTarget.x * aHeading.y ? -AVOID_TURN_DEGREES
			                                                                           : AVOID_TURN_DEGREES;
			mDesiredVelocity.rotate(aTurn);
		}
		mDecisionSeconds = randomFloat(mWanderDecisionRange);
	}

	mStateSeconds -= theInfo.mSeconds;
	if (mStateSeconds < 0.0f)
		decide();

	FishBase::update(theInfo);
}

// 0x402970
void AIFish::alert(FishBase* theSubject, float theX, float theY)
{
	if (mReactable && mState != AI_ALERT)
	{
		if (mThoughtNode != nullptr)
			mThoughtNode->setScale(0.0f, 0.0f);

		if (!mVictims.empty())
			finishEating();

		mAlertSubject = theSubject;
		mAlertPoint.set(theX, theY);
		if (mState == AI_ALERT)
		{
			mReturnState = AI_WANDER;
		}
		else
		{
			mReturnState = mState;
			mSavedSlowDown = mSlowDown;
			mSlowDown = HOLD_SLOW_DOWN;
		}
		mState = AI_ALERT;
	}
}

// 0x402a60
void AIFish::updateAlert(UpdateInfo& theInfo)
{
	if (!isInStage())
	{
		takeOutOfPlay();
		return;
	}

	float aLeft = mAlertSubject->getNosePosition().x;
	float aRight = mAlertSubject->getTailPosition().x;
	updateThoughtBubble(theInfo.mSeconds);

	if (aRight < aLeft)
	{
		float aSwap = aRight;
		aRight = aLeft;
		aLeft = aSwap;
	}
	aLeft -= ALERT_SPAN_MARGIN;
	aRight += ALERT_SPAN_MARGIN;

	if (aRight <= mTail.x || mTail.x <= aLeft)
	{
		if (mNose.x <= aRight || fabsf(mNose.x - aRight) <= ALERT_HOLD_DISTANCE)
		{
			if (aLeft <= mNose.x || fabsf(mNose.x - aLeft) <= ALERT_HOLD_DISTANCE)
				mDesiredVelocity.x = 0.0f;
			else
				mDesiredVelocity.x = mMaxSpeedX * ALERT_SPEED_FACTOR;
		}
		else
		{
			mDesiredVelocity.x = mMaxSpeedX * -ALERT_SPEED_FACTOR;
		}
	}
	else if (0.0f <= mScale.x)
	{
		mDesiredVelocity.x = mMaxSpeedX * -1.0f;
	}
	else
	{
		mDesiredVelocity.x = mMaxSpeedX * 1.0f;
	}

	float aDepth = mNose.y - mAlertSubject->getNosePosition().y;
	if (aDepth < -ALERT_DEPTH_MARGIN)
		mDesiredVelocity.y = mMaxSpeedX * ALERT_SPEED_FACTOR;
	else if (ALERT_DEPTH_MARGIN < aDepth)
		mDesiredVelocity.y = mMaxSpeedX * -ALERT_SPEED_FACTOR;
	else
		mDesiredVelocity.y = 0.0f;

	FishBase::update(theInfo);
}

// 0x402cb0
void AIFish::freeze()
{
	if (mState != AI_FROZEN)
	{
		if (!mVictims.empty())
			finishEating();

		if (mState == AI_FROZEN)
		{
			mSavedState = AI_WANDER;
		}
		else
		{
			mSavedState = mState;
			mSavedSlowDown = mSlowDown;
			mSlowDown = HOLD_SLOW_DOWN;
		}
		mSavedVelocity = mVelocity;
		mState = AI_FROZEN;
	}
}

// 0x402fb0
void AIFish::updateSchool(UpdateInfo& theInfo)
{
	mDecisionSeconds -= theInfo.mSeconds;
	if (mSchool == nullptr)
		beginWander();

	if (!isInStage())
	{
		takeOutOfPlay();
		return;
	}

	if (0.0f <= mDecisionSeconds)
	{
		FishBase::update(theInfo);
		return;
	}

	pickWanderHeading();

	// Away from the nearest fish of the school
	Point aSeparation;
	FishBase* aNearest = mSchool->findNearest(this);
	if (aNearest != nullptr)
	{
		Point aToNearest = aNearest->getNosePosition();
		aToNearest.subtract(Point(mNose.x, mNose.y));
		if (fabsf(aToNearest.x) < mSchool->getSeparationDistance())
			aSeparation.x = (aToNearest.x <= 0.0f ? 1.0f : -1.0f) * mMaxSpeedX * SCHOOL_PUSH_FACTOR;
		if (fabsf(aToNearest.y) < mSchool->getSeparationDistance())
			aSeparation.y = (aToNearest.y <= 0.0f ? 1.0f : -1.0f) * mMaxSpeedY * SCHOOL_PUSH_FACTOR;
	}
	mDesiredVelocity = aSeparation;

	// With the velocity of the school's fish near it
	Point aAlignment;
	Point aNeighboursCentre;
	mSchool->averageNeighbours(this, aAlignment, aNeighboursCentre);
	aAlignment.scale(mSchool->getAlignmentMultiplier());
	mDesiredVelocity.add(aAlignment);

	// Towards the school's centre
	Point aCohesion = mSchool->getCentre();
	aCohesion.subtract(Point(mNose.x, mNose.y));
	aCohesion.normalize();
	aCohesion.scale(mMaxSpeedX / mSchool->getCohesionDivisor());
	mDesiredVelocity.add(aCohesion);

	// Towards where the school is going
	Point aTarget = mSchool->getDestination();
	aTarget.subtract(Point(mNose.x, mNose.y));
	aTarget.normalize();
	aTarget.scale(mSchool->getTargetMultiplier());
	aTarget.scale(mMaxSpeedX);
	mDesiredVelocity.add(aTarget);

	mDecisionSeconds = randomFloat(Point(SCHOOL_DECISION_LOW, SCHOOL_DECISION_HIGH));

	// Towards the fish it can eat and away from the ones that can eat it
	const std::vector<FishBase*>& aFish = FishFood::instance()->getStageMgr()->getFishList();
	bool aThreatened = false;
	for (std::vector<FishBase*>::const_iterator anIt = aFish.begin(); anIt != aFish.end(); ++anIt)
	{
		FishBase* anOther = *anIt;
		Point aToOther = anOther->getNosePosition();
		aToOther.subtract(Point(mNose.x, mNose.y));
		if (aToOther.lengthSquared() < mVisionRangeSquared)
		{
			aToOther.normalize();
			if (anOther->getLevel() - mLevel <= 0.0f)
			{
				if (anOther->canBeEatenBy(this))
				{
					aToOther.scale(mSchool->getChaseMultiplier() * mMaxSpeedX);
					mDesiredVelocity.add(aToOther);
				}
			}
			else
			{
				aToOther.scale(-(mSchool->getRunMultiplier() * mMaxSpeedX));
				mDesiredVelocity.add(aToOther);
				aThreatened = true;
			}
		}
	}

	float aLimitX = mMaxSpeedX;
	float aLimitY = mMaxSpeedY;
	if (aThreatened)
	{
		aLimitX = mMaxSpeedX * SCHOOL_SPEED_LIMIT_FACTOR;
		aLimitY = mMaxSpeedY * SCHOOL_SPEED_LIMIT_FACTOR;
	}
	if (aLimitX < mDesiredVelocity.x)
		mDesiredVelocity.x = aLimitX;
	if (aLimitY < mDesiredVelocity.y)
		mDesiredVelocity.y = aLimitY;
	if (mDesiredVelocity.x < -aLimitX)
		mDesiredVelocity.x = -aLimitX;
	if (mDesiredVelocity.y < -aLimitY)
		mDesiredVelocity.y = -aLimitY;

	FishBase::update(theInfo);
}

// 0x403550
bool AIFish::react(FishBase* theFish)
{
	const FishReaction* aReaction = mSpawner->findReaction(theFish->getTypeName());
	if (aReaction == nullptr)
		return false;

	if (randomFloat(Point(0.0f, 1.0f)) <= aReaction->mChance)
	{
		switch (aReaction->mKind)
		{
		case REACTION_AVOID:
			mTarget = theFish;
			beginAvoid();
			return true;
		case REACTION_CHASE:
			mTarget = theFish;
			beginChase();
			return true;
		case REACTION_RUN:
			if (theFish->getLevel() >= mLevel)
			{
				mTarget = theFish;
				beginRun();
				return true;
			}
			break;
		case REACTION_SPECIAL:
			mTarget = theFish;
			beginSpecial();
			return true;
		}
	}
	return false;
}

// 0x403cd0
void AIFish::checkReactions()
{
	switch (mState)
	{
	case AI_ENTER:
	case AI_CHASE:
	case AI_STUNNED:
	case AI_SCHOOL:
	case AI_FROZEN:
	case AI_ALERT:
	case AI_ALERT_RETURN:
	case AI_UNUSED_14:
	case AI_LEAVE:
	case AI_BEING_EATEN:
		break;
	default:
		if (0.0f < mVisionRange && mReactionSeconds < 0.0f)
		{
			StageMgr* aStage = FishFood::instance()->getStageMgr();
			const std::vector<FishBase*>& aFish = aStage->getFishList();
			bool aReacted = false;
			for (std::vector<FishBase*>::const_iterator anIt = aFish.begin(); anIt != aFish.end(); ++anIt)
			{
				if (aReacted)
					break;
				if (*anIt != this && canSee(*anIt))
					aReacted = react(*anIt);
			}

			FishBase* aPlayer = aStage->getPlayer();
			if (canSeePlayer())
				react(aPlayer);
			mReactionSeconds = REACTION_INTERVAL;
		}
	}
}

// 0x403e30
void AIFish::putInPlay()
{
	mEaten = false;
	mPush.set(0.0f, 0.0f);
	mDesiredVelocity.set(0.0f, 0.0f);
	mVelocity.set(0.0f, 0.0f);
	mTarget = nullptr;
	mState = AI_ENTER;
	FishBase::putInPlay();
	std::vector<FishBase*>().swap(mVictims);
}

// 0x403fb0
void AIFish::startBeingEaten(FishBase* theEater, float theSeconds)
{
	mStateSeconds = BEING_EATEN_STATE_SECONDS;
	mDecisionSeconds = BEING_EATEN_STATE_SECONDS;
	mBeingEatenSeconds = theSeconds;
	mState = AI_BEING_EATEN;
	mBeingEatenLeft = theSeconds;
	mEater = theEater;
	FishBase::startBeingEaten(theEater, BEING_EATEN_BASE_SECONDS);

	float aVolume = theEater == FishFood::instance()->getStageMgr()->getPlayer() ? EATEN_BY_PLAYER_VOLUME
	                                                                              : EATEN_BY_FISH_VOLUME;
	if (!mEatenSoundName.empty())
		FishFood::instance()->playSound(mEatenSoundName, aVolume, 1.0f);
}

// 0x404080
bool AIFish::finishBeingEaten(FishBase* theEater)
{
	if (theEater == FishFood::instance()->getStageMgr()->getPlayer())
	{
		mEatCount++;
		if (mSchool != nullptr)
		{
			mSchool->onFishEaten();
			if (mSchool->getAliveCount() == 0)
				FishFood::instance()->getStageMgr()->getPlayer()->onSchoolEaten();
		}
	}

	if (!mVictims.empty())
		finishEating();

	takeOutOfPlay();
	return true;
}

// 0x404200
void AIFish::stun(float theSeconds, FishBase*, bool theShowEffect)
{
	if (theShowEffect && mStunEffect != nullptr)
	{
		mStunEffect->setPosition(fabsf((mTail.x - mNose.x) * 0.5f), 0.0f);
		mStunEffect->startEmitting();
	}

	mStunSeconds = theSeconds;
	mDesiredVelocity.set(0.0f, 0.0f);
	mSavedSlowDown = mSlowDown;
	mSlowDown = STUN_SLOW_DOWN;
	mState = AI_STUNNED;
}

// 0x404320
void AIFish::finishEating()
{
	std::vector<FishBase*>::iterator aVictim = mVictims.begin();
	while (aVictim != mVictims.end())
	{
		FishBase* aFish = *aVictim;
		if (aFish != this)
			aFish->finishBeingEaten(this);
		aVictim = mVictims.erase(aVictim);
	}
}

// 0x404500
void AIFish::loadEffects()
{
	mStunEffect = new ParticleSystem();
	addChild(mStunEffect);
	mStunEffect->load(STUN_EFFECT_FILE);

	if (!mThought.mImage.empty())
	{
		Image* anImage = App::instance()->getImage(mThought.mImage.c_str());
		anImage->setPivot(1);
		mThoughtNode = new ImageNode(anImage);
		addChild(mThoughtNode);
		mThoughtNode->setVisible(false);
		mThoughtNode->setPosition(mThought.mOffset);
		mThoughtNode->setScale(0.0f, 0.0f);
	}
}

// 0x404670
void AIFish::takeOutOfPlay()
{
	if (mStunEffect != nullptr)
		mStunEffect->stopEmitting();
	clearAlert();
	FishBase::takeOutOfPlay();
}

// 0x404b20
void AIFish::eat(FishBase* theVictim)
{
	if (!mHasEatAnimation)
	{
		theVictim->finishBeingEaten(this);
		return;
	}

	theVictim->startBeingEaten(this, 0.2f);
	mVictims.push_back(theVictim);
	playEatAnimation();
	mDesiredVelocity.x = mLungeFactor * mDesiredVelocity.x;
}

} // namespace feedingfrenzy
