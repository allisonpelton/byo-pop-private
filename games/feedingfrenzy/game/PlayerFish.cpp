#include "PlayerFish.h"
#include "AIFish.h"
#include "App.h"
#include "FishFood.h"
#include "GameSession.h"
#include "GrowMeter.h"
#include "Oyster.h"
#include "PlayerShieldFx.h"
#include "PlayerThoughtBubble.h"
#include "Random.h"
#include "StageMgr.h"
#include "StunBubbleFx.h"

#include <cmath>
#include <cstdio>

using namespace engine;

namespace feedingfrenzy
{

// The keys the fish answers to (the system's key codes): the control key, the space bar and the arrow keys
const int KEY_CONTROL = 0x11;
const int KEY_SPACE = 0x20;
const int KEY_LEFT = 0x25;
const int KEY_UP = 0x26;
const int KEY_RIGHT = 0x27;
const int KEY_DOWN = 0x28;

// What the fish starts with (the stage's file sets its own speeds, sizes and levels)
const float START_MAX_SPEED_X = 145.0f;
const float START_MAX_SPEED_Y = 110.0f;
const float START_SPEED_UP = 850.0f;
const float START_SLOW_DOWN = 250.0f;
const float START_LEVEL = 0.5f;
const float START_SIZE = 0.6f;
const float START_BODY_HALF_SIZE = 6.0f;
const char* const PLAYER_TYPE_NAME = "Player";

// Where the pointer is put back after each reading, in the screen's pixels
const float CURSOR_CENTRE_X = 325.0f;
const float CURSOR_CENTRE_Y = 265.0f;

// The mouse's movement becomes a speed: the pixels it moved, over the seconds of the frame times this much, doubled;
// a movement under the dead zone is none, and a poisoned fish moves the other way at this part of it
const float MOUSE_SPEED_NUMERATOR = 2.0f;
const float MOUSE_SPEED_DENOMINATOR = 900.0f;
const double MOUSE_DEAD_ZONE = 0.1;
const float POISONED_MOUSE_FACTOR = -0.7f;

// How a click of either button shakes off the poison
const float POISON_SHAKE_SECONDS = 0.6f;

// The frames' rectangles: how far the fish's bounds are drawn in from its picture's
const float BOUNDS_INSET = 10.0f;
// The part of the stage's height at which a falling fish has come in, and the speed it falls at
const float SPAWN_DEPTH_FACTOR = 0.4f;
const float SPAWN_FALL_SPEED = 100.0f;
const float SPAWN_START_MARGIN = 50.0f;
// How long a fish that has just come in cannot be hurt
const float SPAWN_INVINCIBLE_SECONDS = 3.0f;
const float FLICKER_SECONDS = 0.2f;
const float FLICKER_BLINK_SECONDS = 0.1f;
const float FLICKER_DARK_LEVEL = 0.25f;

// The edge of the stage that the fish stops against
const int EDGE_MARGIN = 5;

// The dash: how long it lasts, how long until the next, the seconds the mouse is followed for to start it and how much
// faster than its greatest speed it goes
const float DASH_SECONDS = 0.25f;
const float DASH_COOLDOWN_SECONDS = 0.4f;
const float DASH_FOLLOW_SECONDS = 0.03f;
const float DASH_SPEED_FACTOR = 3.0f;
// While it dashes the water slows the fish this much less
const float DASH_DRAG_FACTOR = 0.5f;

// How long the fish waits dead, and how long it takes to grow to the next size
const float DYING_SECONDS = 3.0f;
const float GROW_SECONDS = 0.2f;
// The growing: the part of the time (it counts down from 0.2, so five times the left) at which it has the size, and the
// curve of its way there
const float GROW_PROGRESS_RATE = 5.0f;
const double GROW_CURVE_SPEED = 2.356;
const double GROW_CURVE_OFFSET = 0.293;

// The stun: how long the fish cannot be stunned again, how far in front of it the stars are and the span the effect
// has from the nose
const float STUN_IMMUNE_SECONDS = 1.5f;
const float STUN_STARS_X = -12.0f;
const float STUN_STARS_Y = 0.0f;
// The stun of every fish on the screen that the bonus bubble makes: how long, and how long the stage waits
const float STUN_ALL_SECONDS = 2.5f;
const float STUN_ALL_START_DELAY = 2.5f;

// The poison: the fraction its colour starts at, how fast it comes and goes (a part of the whole a second), the bar's
// size, where it is over the fish, and the colour of the fish
const float POISON_START_BLEND = 0.1f;
const float POISON_BLEND_RATE = 4.0f;
const float POISON_BAR_WIDTH = 82.0f;
const float POISON_BAR_HEIGHT = 8.0f;
const float POISON_BAR_GAP = 5.0f;
const float POISON_RED_FACTOR = -0.6f;
const float POISON_GREEN_FACTOR = 0.1f;
const float POISON_BAR_BACK_RED = 0.1f;
const float POISON_BAR_BACK_GREEN = 0.1f;
const float POISON_BAR_BACK_BLUE = 0.3f;
const float POISON_BAR_FRONT_RED = 0.1f;
const float POISON_BAR_FRONT_GREEN = 0.9f;
const float POISON_BAR_FRONT_BLUE = 0.1f;

// The seconds a bonus lasts: the double score, the speed, and how much faster the speed makes the fish
const float DOUBLE_SCORE_SECONDS = 5.0f;
const float DOUBLE_SCORE_FACTOR = 2.0f;
const float SPEED_BOOST_SECONDS = 4.0f;
const float SPEED_BOOST_FACTOR = 1.8f;
// The seconds a star adds to a timed stage
const float STAR_TIME_BONUS_SECONDS = 3.0f;

// The shield: how long the one that hit it cannot hit it again
const float SHIELD_IMMUNE_SECONDS = 1.6f;

// The chain of fish eaten: its longest, its steps, and how its time runs out
const int CHAIN_LONGEST = 14;
const int CHAIN_STEP = 7;
const float CHAIN_STEP_TIME_FACTOR = 1.5f;
const float CHAIN_DROP_STEP_FACTOR = 1.4f;
const float CHAIN_DROP_LOW_FACTOR = 0.33333334f;
const float CHAIN_DROP_HIGH_FACTOR = 0.25f;
const int CHAIN_LOW_COUNT = 8;
const int FRENZY_SECOND_LEVEL = 2;
const int FRENZY_THIRD_LEVEL = 3;

// The score waiting to show, how long between each, and the school's bonus
const float PENDING_SCORE_SECONDS = 0.05f;
const int SCHOOL_BONUS_POINTS = 500;
// How far over the fish the thought bubble is, from the top of its bounds
const float THOUGHT_BUBBLE_HEIGHT = 91.0f;

// The most fish it takes in its mouth together
const size_t MOST_VICTIMS = 5;
// How long a fish that is eaten takes to reach the mouth
const float EATEN_BY_SECONDS = 0.2f;
// The size of the growth that has no end
const int NO_GROWTH_END = 999998;

// The chomp of the fish eaten: how far it is put around the nose (a fish eaten by the player puts it in front of the
// nose, where the fish swims), the chances of each size and how many fish at once are many
const float CHOMP_SPREAD = 15.0f;
const float CHOMP_FORWARD_FACING_LEFT = -15.0f;
const float CHOMP_FORWARD_FACING_RIGHT = 1.0f;
const float CHOMP_SMALL_CHANCE = 0.5f;
const float CHOMP_MEDIUM_CHANCE = 0.65f;
const int CHOMP_MANY_FISH = 3;

// The feeding fury: how fast the fish swims at the one it hunts and back, and how near the place it started it must
// come (squared)
const float FURY_SPEED = 1600.0f;
const float FURY_ARRIVE_DISTANCE_SQUARED = 200.0f;

// The effects' files, and the pictures of the stars that go over a stunned or poisoned fish
const char* const SPAWN_BUBBLES_FILE = "config\\FX\\MineFallFx.xml";
const char* const DASH_FILE = "config\\FX\\dashFX.xml";
const char* const STUN_FILE = "config\\Fx\\stunnedEffect.xml";
const char* const STARS_PATH = "resources\\Fx\\Stun\\stars.";
const char* const STARS_EXTENSION = ".png";
const char* const EAT_FISH_FILE = "config\\FX\\playerEatFishFx.xml";
const char* const EAT_BONUS_FILE = "config\\FX\\playerEatBonusFx.xml";

// port: the sound of the bank that plays at its own volume and pitch (0x42d230)
static void playSound(const char* theName)
{
	FishFood::instance()->playSound(theName, 1.0f, 1.0f);
}

// 0x42c0c0
static StageMgr* getStage()
{
	return FishFood::instance()->getStageMgr();
}

// 0x42beb0
static GameSession* getSession()
{
	return FishFood::instance()->getSession();
}

// 0x414840
PlayerFish::PlayerFish()
    : mMouseMove(0.0f, 0.0f), mCursorCentre(0.0f, 0.0f), mCursorLocked(true), mFluidDrag(0.0f), mSpawnSoundPlayed(false),
      mKeyUp(false), mKeyDown(false), mKeyLeft(false), mKeyRight(false), mDyingSeconds(0.0f), mDashSeconds(0.0f),
      mDashCooldown(0.0f), mInvincibleSeconds(0.0f), mFlickerSeconds(0.0f), mGrowToIndex(0), mGrowFromIndex(0),
      mGrowSeconds(0.0f), mStunSeconds(0.0f), mStunImmuneSeconds(0.0f), mPoisonRamped(false), mPoisonTotal(0.0f),
      mPoisonLeft(0.0f), mPoisonBlend(0.0f), mPoisonCooldown(0.0f), mDoubleScoreSeconds(-1.0f),
      mScoreMultiplier(1.0f), mSpeedBoostSeconds(-1.0f), mSpeedMultiplier(1.0f), mScore(0.0f), mState(PLAYER_SWIM),
      mTarget(-1.0f, -1.0f), mTargetVelocity(0.0f, 0.0f), mGrowthIndex(0), mChainCount(0), mChainSeconds(0.0f),
      mFrenzyTime(0.0f), mFrenzyLevel(1), mPendingScoreSeconds(0.0f), mShieldCount(0), mShieldImmuneSeconds(0.0f),
      mShieldEater(nullptr), mFuryTarget(nullptr), mFuryPhase(FURY_PICK), mFuryReturn(0.0f, 0.0f), mEating(false)
{
	mMaxSpeedX = START_MAX_SPEED_X;
	mMaxSpeedY = START_MAX_SPEED_Y;
	mSpeedUp = START_SPEED_UP;
	mSlowDown = START_SLOW_DOWN;
	mDesiredVelocity.set(0.0f, 0.0f);
	mLevel = START_LEVEL;
	mSize = START_SIZE;
	mInPlay = true;
	mBodyRect.set(-START_BODY_HALF_SIZE, -START_BODY_HALF_SIZE, START_BODY_HALF_SIZE, START_BODY_HALF_SIZE);
	mTypeName = PLAYER_TYPE_NAME;
	mPoisonProof = false;
	mReactable = false;
	mTargetable = true;
	mStunnable = true;
}

// 0x4142c0
PlayerFish::~PlayerFish() {}

// 0x40f970
bool PlayerFish::keyDown(int theKey)
{
	switch (theKey)
	{
	case KEY_CONTROL:
		useSpecial();
		return true;
	case KEY_SPACE:
		if (mDashCooldown < 0.0f && (mState == PLAYER_SWIM || mState == PLAYER_INVINCIBLE))
			useSpecial();
		return true;
	case KEY_LEFT:
		mKeyLeft = true;
		return true;
	case KEY_UP:
		mKeyUp = true;
		return true;
	case KEY_RIGHT:
		mKeyRight = true;
		return true;
	case KEY_DOWN:
		mKeyDown = true;
		return true;
	default:
		return false;
	}
}

// 0x40fa30
bool PlayerFish::keyUp(int theKey)
{
	switch (theKey)
	{
	case KEY_LEFT:
		mKeyLeft = false;
		return true;
	case KEY_UP:
		mKeyUp = false;
		return true;
	case KEY_RIGHT:
		mKeyRight = false;
		return true;
	case KEY_DOWN:
		mKeyDown = false;
		return true;
	default:
		return false;
	}
}

// 0x40fa80
void PlayerFish::setTarget(float theX, float theY, bool theHasTarget)
{
	if (!theHasTarget)
	{
		mTarget.set(-1.0f, -1.0f);
		mTargetVelocity.set(0.0f, 0.0f);
	}
	else
	{
		mTarget = getStage()->screenToWorld(Point(theX, theY));
	}
}

// 0x40fb30
void PlayerFish::updateRects()
{
	FishBase::updateRects();
	const Rect& aPicture = *mAnimations->getSelected()->getBounds();
	mBounds = aPicture;
	mBounds.left = (mBounds.left + BOUNDS_INSET) * mSize;
	mBounds.right = (mBounds.right - BOUNDS_INSET) * mSize;
	mBounds.top = (mBounds.top + BOUNDS_INSET) * mSize;
	mBounds.bottom = (mBounds.bottom - BOUNDS_INSET) * mSize;
	setPosition(getX(), getY());
}

// 0x40fbf0
void PlayerFish::updateDying(UpdateInfo& theInfo)
{
	mDyingSeconds -= theInfo.mSeconds;
	if (0.0f <= mDyingSeconds)
	{
		if (mEater != nullptr)
			setPosition(mEater->getX(), mEater->getY());
		Container::update(theInfo);
	}
	else
	{
		if (getSession()->getLives() < 1)
		{
			getStage()->showGameOver();
			return;
		}
		int aType = getStage()->getType();
		if (aType != STAGE_BONUS && aType != STAGE_TIMED_BONUS)
			startLife();
	}
}

// 0x40fcb0
void PlayerFish::updateSwimming(UpdateInfo& theInfo)
{
	mDesiredVelocity.set(0.0f, 0.0f);
	mSpawnBubblesFx->stopEmitting();
	followMouse(theInfo.mSeconds);
	keepInStage();
	FishBase::update(theInfo);
}

// 0x40fd00
void PlayerFish::keepInStage()
{
	StageMgr* aStage = getStage();
	int aWorldWidth = aStage->getWorldWidth();
	int aWorldHeight = aStage->getWorldHeight();
	if (mState == PLAYER_FURY)
		return;

	const Rect& aHit = mScaledHitRect.getRect();
	int aLeftLimit;
	if (!isMirrored())
	{
		aLeftLimit = EDGE_MARGIN;
		aWorldWidth -= (int)aHit.width();
	}
	else
	{
		aLeftLimit = (int)aHit.width() + EDGE_MARGIN;
	}
	int aHalfHeight = (int)aHit.height() / 2;
	if ((mNose.x < (float)aLeftLimit && (mVelocity.x < 0.0f || mDesiredVelocity.x < 0.0f)) ||
	    ((float)(aWorldWidth - EDGE_MARGIN) < mNose.x && (0.0f < mVelocity.x || 0.0f < mDesiredVelocity.x)))
	{
		mDesiredVelocity.x = 0.0f;
		mVelocity.x = 0.0f;
	}
	if ((mNose.y < (float)aHalfHeight && (mVelocity.y < 0.0f || mDesiredVelocity.y < 0.0f)) ||
	    ((float)(aWorldHeight - aHalfHeight) < mNose.y && (0.0f < mVelocity.y || 0.0f < mDesiredVelocity.y)))
	{
		mDesiredVelocity.y = 0.0f;
		mVelocity.y = 0.0f;
	}
}

// 0x40fed0
void PlayerFish::updateInvincible(UpdateInfo& theInfo)
{
	updateFlicker(theInfo);
	mInvincibleSeconds -= theInfo.mSeconds;
	if (mInvincibleSeconds < 0.0f)
	{
		mColorOffset.set(0.0f, 0.0f, 0.0f);
		mState = PLAYER_SWIM;
		mTargetable = true;
		mStunnable = true;
		mPoisonProof = false;
	}
	updateSwimming(theInfo);
}

// 0x40ff40
void PlayerFish::updateFlicker(UpdateInfo& theInfo)
{
	mFlickerSeconds -= theInfo.mSeconds;
	if (mFlickerSeconds < 0.0f)
	{
		mFlickerSeconds = FLICKER_BLINK_SECONDS;
		if (0.0f < mColorOffset.r)
			mColorOffset.set(0.0f, 0.0f, 0.0f);
		else
			mColorOffset.set(FLICKER_DARK_LEVEL, FLICKER_DARK_LEVEL, FLICKER_DARK_LEVEL);
	}
}

// 0x40ffb0
bool PlayerFish::isInvincible()
{
	return mState == PLAYER_INVINCIBLE || mState == PLAYER_SPAWN;
}

// 0x40ffd0
bool PlayerFish::canEat(FishBase*)
{
	if (getStage()->getType() != STAGE_MENU && mState != PLAYER_SPAWN)
	{
		if (isVisible() && !isDying() && !mBeingEaten)
			return true;
	}
	return false;
}

// 0x410020
void PlayerFish::addPoints(float thePoints)
{
	getSession()->addScore((int)thePoints);
}

// 0x410040
bool PlayerFish::canBeEatenByLevel(float theEaterLevel)
{
	StageMgr* aStage = getStage();
	if (aStage->getType() != STAGE_MENU && !mBeingEaten && (mState == PLAYER_SWIM || mState == PLAYER_SPECIAL))
	{
		if (aStage->getState() == STATE_MERMAID || aStage->getState() == STATE_ENDING)
			return false;
		if (mLevel < theEaterLevel)
			return true;
	}
	return false;
}

// 0x4100c0
bool PlayerFish::canBeEatenBy(FishBase* theEater)
{
	if (!canBeEatenByLevel(theEater->getLevel()))
		return false;
	if (mShieldImmuneSeconds <= 0.0f || theEater != mShieldEater)
	{
		if (mShieldCount < 1)
			return true;
		mShieldEater = theEater;
		mShieldImmuneSeconds = SHIELD_IMMUNE_SECONDS;
		return true;
	}
	return false;
}

// 0x410410
void PlayerFish::updateSpecial(UpdateInfo& theInfo)
{
	updateSwimming(theInfo);
}

// 0x410460
int PlayerFish::updateFrenzyLevel()
{
	mFrenzyLevel = mChainCount / CHAIN_STEP + 1;
	return mFrenzyLevel;
}

// 0x4104a0
void PlayerFish::leftButtonDown()
{
	if (mPoisonLeft <= 0.0f)
	{
		if (mDashCooldown < 0.0f)
			dash();
	}
	else
	{
		mPoisonLeft = mPoisonLeft - POISON_SHAKE_SECONDS;
	}
}

// 0x4104f0
void PlayerFish::rightButtonDown()
{
	if (0.0f < mPoisonLeft)
		mPoisonLeft = mPoisonLeft - POISON_SHAKE_SECONDS;
}

// 0x410530
void PlayerFish::updatePoisonColor()
{
	if (mPoisonBlend <= 0.0f)
	{
		mColorOffset.set(0.0f, 0.0f, 0.0f);
		return;
	}
	float aRed = mPoisonBlend * POISON_RED_FACTOR;
	mColorOffset.set(aRed, mPoisonBlend * POISON_GREEN_FACTOR, aRed);
}

// 0x410590
void PlayerFish::setInvincible(float theSeconds)
{
	mInvincibleSeconds = theSeconds;
	mState = PLAYER_INVINCIBLE;
}

// 0x4105b0
void PlayerFish::gainShield()
{
	mShieldCount++;
	mShieldFx->show();
	if (1 < mShieldCount)
		mShieldCount = 1;
}

// 0x410690
void PlayerFish::move(float theSeconds)
{
	Point aInput(mMouseMove.x, mMouseMove.y);
	if (mState != PLAYER_FURY)
	{
		aInput.scale(mSpeedUp);
		if (mStunSeconds <= 0.0f)
		{
			mVelocity.x = aInput.x * theSeconds + mVelocity.x;
			mVelocity.y = aInput.y * theSeconds + mVelocity.y;
		}
		float aSpeedFactor = FishFood::instance()->getSettings().getMouseSpeedFactor();
		float aDrag = (float)((double)mVelocity.lengthSquared() * mFluidDrag * aSpeedFactor + mSlowDown);
		if (0.0f < mDashSeconds)
			aDrag = aDrag * DASH_DRAG_FACTOR;
		Point aSlowing(mVelocity.x, mVelocity.y);
		aSlowing.normalize();
		float aSlowBy;
		if (fabsf(mVelocity.length()) <= aDrag)
			aSlowBy = fabsf(mVelocity.length());
		else
			aSlowBy = aDrag;
		aSlowing.scale(aSlowBy * -1.0f);
		mVelocity.add(aSlowing);
		if (aSpeedFactor * mMaxSpeedX < mVelocity.length())
		{
			mVelocity.normalize();
			mVelocity.scale(aSpeedFactor * mMaxSpeedX);
		}
		mVelocity.add(mPush);
		mVelocity.add(mPushAgainst);
	}
	keepInStage();
	float aVelocityX = mVelocity.x;
	setPosition((float)((double)theSeconds * aVelocityX + getX()),
	            (float)((double)theSeconds * mVelocity.y + getY()));
}

// 0x4108a0
void PlayerFish::followMouse(float theSeconds)
{
	if (getStage()->isWaiting())
		return;

	if (mDashSeconds < 0.0f)
		mDashFx->stopEmitting();

	Point aCursor = App::instance()->getMousePosition();
	mMouseMove = aCursor;
	mMouseMove.subtract(mCursorCentre);
	mMouseMove.scale(MOUSE_SPEED_NUMERATOR / (theSeconds * MOUSE_SPEED_DENOMINATOR));
	if (fabs((double)mMouseMove.x) < MOUSE_DEAD_ZONE)
		mMouseMove.x = 0.0f;
	if (fabs((double)mMouseMove.y) < MOUSE_DEAD_ZONE)
		mMouseMove.y = 0.0f;
	if (mCursorLocked)
	{
		App::instance()->warpMousePosition((int)CURSOR_CENTRE_X, (int)CURSOR_CENTRE_Y);
		mCursorCentre.set(CURSOR_CENTRE_X, CURSOR_CENTRE_Y);
	}
	if (0.0f < mPoisonLeft)
	{
		mMouseMove.x = mMouseMove.x * POISONED_MOUSE_FACTOR;
		mMouseMove.y = mMouseMove.y * POISONED_MOUSE_FACTOR;
	}
}

// 0x410ad0
int PlayerFish::getGrowthLevelCount()
{
	return (int)mGrowthLevels.size();
}

// 0x410b00
void PlayerFish::updateFury(UpdateInfo& theInfo)
{
	if (mFuryPhase == FURY_CHASE && mFuryTarget != nullptr && mFuryTarget->isInPlay())
	{
		Point aTargetNose = mFuryTarget->getNosePosition();
		Point aBodyCentre((mPlacedBodyRect.right + mPlacedBodyRect.left) * 0.5f,
		                  (mPlacedBodyRect.bottom + mPlacedBodyRect.top) * 0.5f);
		Point aOffset = *getPosition();
		aOffset.subtract(aBodyCentre);
		aTargetNose.add(aOffset);
		mVelocity = aTargetNose;
		mVelocity.subtract(*getPosition());
		mVelocity.normalize();
		mVelocity.scale(FURY_SPEED);

		CollisionRect aTargetHit = *mFuryTarget->getPlacedHitRect();
		CollisionRect aOwnHit = *getPlacedHitRect();
		if (canEat(mFuryTarget) && aOwnHit.intersects(aTargetHit) && mFuryTarget->canBeEatenBy(this))
			eat(mFuryTarget);
	}
	else if (mFuryPhase == FURY_RETURN)
	{
		Point aWay = mFuryReturn;
		aWay.subtract(*getPosition());
		if (FURY_ARRIVE_DISTANCE_SQUARED <= aWay.lengthSquared())
		{
			mVelocity = aWay;
			mVelocity.normalize();
			float aDistance = aWay.length();
			if (FURY_SPEED <= aDistance / theInfo.mSeconds)
				mVelocity.scale(FURY_SPEED);
			else
				mVelocity.scale(aDistance / theInfo.mSeconds);
		}
		else
		{
			endFury();
		}
	}
	else
	{
		mFuryTarget = findFuryTarget();
		if (mFuryTarget == nullptr)
			finishFury();
		else
			mFuryPhase = FURY_CHASE;
	}
	mDesiredVelocity = mVelocity;
	FishBase::update(theInfo);
}

// 0x410e80
void PlayerFish::layout()
{
	mBounds.set(0.0f, 0.0f, 0.0f, 0.0f);
	for (std::vector<Node*>::iterator anIt = mChildren.begin(); anIt != mChildren.end(); ++anIt)
	{
		Node* aChild = *anIt;
		if (aChild->hasFlag(NODE_LAYOUT))
			aChild->layout();
		if (aChild != mStars && aChild != mPoisonGroup)
			mBounds.unite(*aChild->getBounds());
	}
	mBounds.scale(fabsf(mScale.x), fabsf(mScale.y));
	mBounds.offset(mPosition.x, mPosition.y);
	clearFlags(NODE_LAYOUT);
}

// 0x410f20
void PlayerFish::stunAll()
{
	const std::vector<FishBase*>& aFish = getStage()->getFishList();
	for (std::vector<FishBase*>::const_iterator anIt = aFish.begin(); anIt != aFish.end(); ++anIt)
	{
		FishBase* aOther = *anIt;
		if (aOther->isInStage() && aOther->isStunnable() && aOther != this)
			aOther->stun(STUN_ALL_SECONDS, this, true);
	}
	getStage()->setStartDelay(STUN_ALL_START_DELAY);
	mStunBubbleFx->start(mNose);
}

// 0x410ff0
FishBase* PlayerFish::findFuryTarget()
{
	const std::vector<FishBase*>& aFish = getStage()->getFishList();
	float aNearest = 9999999.0f;
	FishBase* aTarget = nullptr;
	for (std::vector<FishBase*>::const_iterator anIt = aFish.begin(); anIt != aFish.end(); ++anIt)
	{
		FishBase* aOther = *anIt;
		if (aOther->isOnScreen() && aOther != this && aOther->canBeEatenBy(this) && !aOther->isHidden())
		{
			Point aDistance = getNosePosition();
			aDistance.subtract(aOther->getNosePosition());
			float aSquared = aDistance.lengthSquared();
			if (aSquared < aNearest)
			{
				aTarget = aOther;
				aNearest = aSquared;
			}
		}
	}
	return aTarget;
}

// 0x411180
void PlayerFish::updateGrow(UpdateInfo& theInfo)
{
	mGrowSeconds -= theInfo.mSeconds;
	if (mGrowSeconds < 0.0f)
		mGrowSeconds = 0.0f;
	float aFromSize = mGrowthLevels[mGrowFromIndex].mSize;
	const GrowthLevel& aTo = mGrowthLevels[mGrowToIndex];
	double aProgress = 1.0 - (double)mGrowSeconds * GROW_PROGRESS_RATE;
	float aSize;
	if (aProgress < 1.0)
	{
		double aCurve = sin(aProgress * GROW_CURVE_SPEED);
		aSize = (float)(((double)aTo.mSize - aFromSize) * (aCurve + GROW_CURVE_OFFSET) + aFromSize);
	}
	else
	{
		aSize = aTo.mSize;
	}
	mSize = aSize;
	float aScaleX = aSize;
	if (mScale.x < 0.0f)
		aScaleX = -aSize;
	setScale(aScaleX, aSize);
	followMouse(theInfo.mSeconds);
	keepInStage();
	FishBase::update(theInfo);
	if (mGrowSeconds <= 0.0f)
		mState = PLAYER_SWIM;
}

// 0x411290
float PlayerFish::getScoreToNextLevel()
{
	int aLast = getGrowthLevelCount() - 1;
	for (int i = 0; i < aLast; i++)
	{
		if (mGrowthLevels[i].mLevel == mLevel)
			return (float)mGrowthLevels[i + 1].mScore - mScore;
	}
	return 0.0f;
}

// 0x411310
float PlayerFish::getGrowthProgress()
{
	if (mGrowthIndex == getGrowthLevelCount() - 1)
		return 1.0f;
	const GrowthLevel& aLevel = mGrowthLevels[mGrowthIndex];
	const GrowthLevel& aNext = mGrowthLevels[mGrowthIndex + 1];
	if (NO_GROWTH_END < aNext.mScore - aLevel.mScore)
		return 0.0f;
	return (float)(((double)mScore - aLevel.mScore) / (double)(aNext.mScore - aLevel.mScore));
}

// 0x411390
float PlayerFish::getGrowthScore(int theIndex)
{
	return (float)mGrowthLevels[theIndex].mScore;
}

// 0x411620
void PlayerFish::update(UpdateInfo& theInfo)
{
	float aSeconds = theInfo.mSeconds;
	if (mState != PLAYER_INVINCIBLE && mState != PLAYER_SPAWN && mPoisonBlend <= 0.0f)
		mColorOffset.set(0.0f, 0.0f, 0.0f);

	if (0.0f <= mDashCooldown)
		mDashCooldown = mDashCooldown - aSeconds;
	if (0.0f <= mShieldImmuneSeconds)
		mShieldImmuneSeconds = mShieldImmuneSeconds - aSeconds;
	if (0.0f <= mDashSeconds)
		mDashSeconds = mDashSeconds - aSeconds;
	if (0.0f <= mStunSeconds)
	{
		mStunSeconds = mStunSeconds - aSeconds;
		if (mStunSeconds < 0.0f)
		{
			mStunImmuneSeconds = STUN_IMMUNE_SECONDS;
			mStunFx->stopEmitting();
			mStars->stop();
			mStars->setVisible(false);
		}
	}

	// The poison comes on over a quarter of a second, and goes as its time runs out
	if (mPoisonLeft <= 0.0f)
	{
		if (0.0f < mPoisonBlend)
		{
			mPoisonBlend = mPoisonBlend - aSeconds * POISON_BLEND_RATE;
			if (mPoisonBlend <= 0.0f)
				mPoisonBlend = 0.0f;
			updatePoisonColor();
		}
	}
	else
	{
		mPoisonLeft = mPoisonLeft - aSeconds;
		if (mPoisonBlend < 1.0f && !mPoisonRamped)
		{
			mPoisonBlend = aSeconds * POISON_BLEND_RATE + mPoisonBlend;
			if (1.0f <= mPoisonBlend)
			{
				mPoisonBlend = 1.0f;
				mPoisonRamped = true;
			}
		}
		else if (mPoisonRamped)
		{
			mPoisonBlend = mPoisonLeft / mPoisonTotal;
		}
		updatePoisonColor();
	}
	mPoisonFront->setValue(mPoisonBlend);
	if (mPoisonBlend <= 0.0f)
	{
		mPoisonGroup->setVisible(false);
		mStars->stop();
		mStars->setVisible(false);
	}

	if (0.0f <= mStunImmuneSeconds)
		mStunImmuneSeconds = mStunImmuneSeconds - aSeconds;
	if (0.0f <= mSpeedBoostSeconds)
	{
		mSpeedBoostSeconds = mSpeedBoostSeconds - aSeconds;
		if (mSpeedBoostSeconds < 0.0f)
		{
			playSound("playerSpeedDown");
			float aRestore = 1.0f / mSpeedMultiplier;
			mSpeedMultiplier = 1.0f;
			mMaxSpeedX = aRestore * mMaxSpeedX;
			mMaxSpeedY = aRestore * mMaxSpeedY;
			mSpeedUp = aRestore * mSpeedUp;
		}
	}

	// The chain of fish eaten runs down: each fish is lost a little faster than the last
	if (0.0f < mChainSeconds)
	{
		mChainSeconds = mChainSeconds - aSeconds;
		if (mChainSeconds < 0.0f && 0 < mChainCount)
		{
			mChainCount--;
			float aSecondsLeft = mFrenzyTime;
			if (mChainCount % CHAIN_STEP == 0)
				aSecondsLeft = aSecondsLeft * CHAIN_DROP_STEP_FACTOR;
			else if (mChainCount < CHAIN_LOW_COUNT)
				aSecondsLeft = aSecondsLeft * CHAIN_DROP_LOW_FACTOR;
			else
				aSecondsLeft = aSecondsLeft * CHAIN_DROP_HIGH_FACTOR;
			mChainSeconds = aSecondsLeft;
		}
	}

	if (0.0f <= mDoubleScoreSeconds)
	{
		mDoubleScoreSeconds = mDoubleScoreSeconds - aSeconds;
		if (mDoubleScoreSeconds < 0.0f)
			mScoreMultiplier = 1.0f;
	}

	// The scores of the fish eaten show one at a time
	if (!mPendingScores.empty())
	{
		if (0.0f < mPendingScoreSeconds)
			mPendingScoreSeconds = mPendingScoreSeconds - aSeconds;
		if (mPendingScoreSeconds <= 0.0f)
		{
			mPendingScoreSeconds = 0.0f;
			getStage()->showEatScore((float)mPendingScores.front());
			mPendingScores.erase(mPendingScores.begin());
			if (!mPendingScores.empty())
				mPendingScoreSeconds = PENDING_SCORE_SECONDS;
		}
	}

	float aBubbleY = mBounds.top;
	float aBubbleX;
	if (0.0f <= mScale.x)
		aBubbleX = mPlacedHitRect.getRect().left;
	else
		aBubbleX = mPlacedHitRect.getRect().right;
	mThoughtBubble->setPosition(aBubbleX, aBubbleY - THOUGHT_BUBBLE_HEIGHT);
	mShieldFx->update(theInfo);

	switch (mState)
	{
	case PLAYER_SWIM:
		updateSwimming(theInfo);
		break;
	case PLAYER_GROW:
		updateGrow(theInfo);
		break;
	case PLAYER_BEING_EATEN:
		updateBeingEaten(theInfo);
		break;
	case PLAYER_DYING:
		updateDying(theInfo);
		break;
	case PLAYER_SPAWN:
		updateSpawn(theInfo);
		break;
	case PLAYER_INVINCIBLE:
		updateInvincible(theInfo);
		break;
	case PLAYER_FURY:
		updateFury(theInfo);
		break;
	case PLAYER_SPECIAL:
		updateSpecial(theInfo);
		break;
	}
}

// 0x411be0
void PlayerFish::die()
{
	StageMgr* aStage = getStage();
	if (aStage->getType() == STAGE_MENU)
		return;

	aStage->onPlayerEaten();
	int aType = aStage->getType();
	if (mThoughtBubble != nullptr)
		mThoughtBubble->show(false, false);
	mPoisonLeft = 0.0f;
	if (aType == STAGE_NORMAL || aType == STAGE_TIMED_NORMAL || aType == STAGE_SURVIVAL)
	{
		if (aType == STAGE_SURVIVAL)
			aStage->resetSecondsLeft();
		getSession()->loseLife();
	}
	if (aStage->getType() != STAGE_BONUS && aStage->getType() != STAGE_TIMED_BONUS)
		aStage->showLostBanner();
	else
		aStage->endTimeUp();

	if (0.0f < mDashSeconds)
	{
		mDashSeconds = -1.0f;
		mDashFx->stopEmitting();
	}
	setScale(0.0f, 0.0f);
	playSound("playerDie");
	mState = PLAYER_DYING;
	mDyingSeconds = DYING_SECONDS;
	mDashCooldown = 0.0f;
	if (!mVictims.empty())
		finishEating();
	if (mStunBubbleFx != nullptr)
		mStunBubbleFx->stop();
	mStunnable = false;
}

// 0x411e70
void PlayerFish::startLife()
{
	mEater = nullptr;
	mEating = false;
	mSpawnSoundPlayed = false;
	mVictims.clear();
	if (mGrowthIndex < 0)
		mGrowthIndex = 0;
	const GrowthLevel& aLevel = mGrowthLevels[mGrowthIndex];
	mScore = (float)aLevel.mScore + 1.0f;
	mLevel = aLevel.mLevel;
	mSize = aLevel.mSize;
	mState = PLAYER_SPAWN;
	mFlickerSeconds = FLICKER_SECONDS;
	mVelocity.set(0.0f, 0.0f);
	setOpacity(1.0f);
	mTargetable = false;
	mStunnable = false;
	mPoisonProof = false;
	setPosition(getStage()->getWorldWidth() * 0.5f, -mBounds.height() - SPAWN_START_MARGIN);
	setScale(mSize, mSize);
	mPush.y = SPAWN_FALL_SPEED;
	mBeingEaten = false;
	if (mDashFx == nullptr)
		loadEffects();
	mStunImmuneSeconds = -1.0f;
	mStunSeconds = -1.0f;
	mStunFx->stopEmitting();
	mStars->stop();
	mStars->setVisible(false);
	mDashCooldown = -1.0f;
	mPendingScoreSeconds = 0.0f;
	mSpawnBubblesFx->setScale(1.0f / mSize, 1.0f / mSize);
	mSpawnBubblesFx->startEmitting();
	if (1.0f < mSpeedMultiplier)
	{
		mSpeedBoostSeconds = -1.0f;
		float aRestore = 1.0f / mSpeedMultiplier;
		mSpeedMultiplier = 1.0f;
		mMaxSpeedX = aRestore * mMaxSpeedX;
		mMaxSpeedY = aRestore * mMaxSpeedY;
		mSpeedUp = aRestore * mSpeedUp;
	}
	if (1.0f < mScoreMultiplier)
	{
		mDoubleScoreSeconds = -1.0f;
		mScoreMultiplier = 1.0f;
	}
	updateGrowth();
	Point aCursor = App::instance()->getMousePosition();
	mMouseMove.set(0.0f, 0.0f);
	mCursorCentre = aCursor;
	mChainCount = 0;
	mFrenzyLevel = 1;
	mChainSeconds = mFrenzyTime;
	getStage()->fadeMusicIn();
	getStage()->hideEndBanner();
}

// 0x412200
void PlayerFish::updateSpawn(UpdateInfo& theInfo)
{
	if (!mSpawnSoundPlayed)
	{
		if (getStage()->getType() != STAGE_MENU)
		{
			playSound("playerSpawn");
			mSpawnSoundPlayed = true;
		}
	}
	move(theInfo.mSeconds);
	updateAnimation();
	updatePlacedRects();
	updatePlacedHitRect();
	Container::update(theInfo);
	updateFlicker(theInfo);
	if ((float)getStage()->getWorldHeight() * SPAWN_DEPTH_FACTOR < mNose.y)
	{
		mDesiredVelocity.set(0.0f, 0.0f);
		mPush.set(0.0f, 0.0f);
		setOpacity(1.0f);
		if (FishFood::instance()->getSettings().areTipsEnabled())
		{
			mThoughtBubble->reset();
			mThoughtBubble->show(true, false);
		}
		mSpawnBubblesFx->stopEmitting();
		setInvincible(SPAWN_INVINCIBLE_SECONDS);
		mPoisonProof = false;
		mTargetable = true;
	}
}

// 0x412350
void PlayerFish::dash()
{
	if (mStunSeconds <= 0.0f && mState != PLAYER_SPECIAL && mState != PLAYER_DYING && mState != PLAYER_BEING_EATEN &&
	    mState != PLAYER_FURY && mState != PLAYER_SPAWN)
	{
		playSound("playerDash");
		followMouse(DASH_FOLLOW_SECONDS);
		if (mVelocity.lengthSquared() <= 0.0f)
		{
			if (0.0f <= mScale.x)
				mVelocity.x = -1.0f;
			else
				mVelocity.x = 1.0f;
		}
		float aSpeed;
		if (mVelocity.length() <= mMaxSpeedX)
			aSpeed = mMaxSpeedX;
		else
			aSpeed = mVelocity.length();
		mVelocity.normalize();
		mVelocity.scale(aSpeed * DASH_SPEED_FACTOR);
		mDesiredVelocity = mVelocity;
		mDashCooldown = DASH_COOLDOWN_SECONDS;
		mDashSeconds = DASH_SECONDS;
		Point aFromTail = mTail;
		aFromTail.subtract(Point(getX(), getY() - mBounds.height() * 0.5f));
		mDashFx->setPosition(aFromTail.x, aFromTail.y);
		mDashFx->startEmitting();
	}
}

// 0x412560
void PlayerFish::onAteFish(int thePoints)
{
	playSound("bite1");
	mScore = (float)thePoints + mScore;
	getSession()->addScore(thePoints);
	updateGrowth();
}

// 0x4125d0
void PlayerFish::addToChain(int theCount)
{
	mChainCount += theCount;
	if (CHAIN_LONGEST < mChainCount)
		mChainCount = CHAIN_LONGEST;
	if (mChainCount % CHAIN_STEP == 0)
		mChainSeconds = mFrenzyTime * CHAIN_STEP_TIME_FACTOR;
	else
		mChainSeconds = mFrenzyTime;
	int aPreviousLevel = mFrenzyLevel;
	mFrenzyLevel = mChainCount / CHAIN_STEP + 1;
	if (aPreviousLevel < mFrenzyLevel)
	{
		if (mFrenzyLevel == FRENZY_SECOND_LEVEL)
		{
			playSound("frenzyVox");
			playSound("frenzyChime");
			getStage()->showFrenzy();
		}
		else if (mFrenzyLevel == FRENZY_THIRD_LEVEL)
		{
			playSound("doubleFrenzyVox");
			playSound("frenzyChime");
			getStage()->showDoubleFrenzy();
		}
	}
}

// 0x412720
void PlayerFish::finishEating()
{
	if (mEating)
		return;
	mEating = true;

	int aHighestRank = 1;
	bool aHadVictims = !mVictims.empty();
	int aFoodCount = 0;
	float aHighestLevel = 0.0f;
	bool aAteObject = false;
	bool aAteFish = false;
	while (!mVictims.empty())
	{
		FishBase* aVictim = mVictims.front();
		int aRank = 1;
		for (int i = 0; i < mGrowthIndex; i++)
		{
			if (mGrowthLevels[i].mLevel < aVictim->getLevel())
				aRank++;
		}
		if (aHighestRank <= aRank)
			aHighestRank = aRank;
		onAte(aVictim);
		if (aVictim->countsAsFood())
		{
			aFoodCount++;
			if (aHighestLevel <= aVictim->getLevel())
				aHighestLevel = aVictim->getLevel();
		}
		int aEdibleKind = aVictim->getEdibleKind();
		if (aEdibleKind == EDIBLE_FISH)
		{
			aAteFish = true;
		}
		else
		{
			aAteObject = true;
			if (aEdibleKind != EDIBLE_OBJECT)
				aAteFish = true;
		}
		if (aVictim != mEater)
			aVictim->finishBeingEaten(this);
		mVictims.erase(mVictims.begin());
	}
	if (aAteFish)
	{
		mEatFishFx->stopEmitting();
		mEatFishFx->clear();
		mEatFishFx->setPosition(mNose.x, mNose.y);
		mEatFishFx->startEmitting();
	}
	if (aAteObject)
	{
		mEatBonusFx->stopEmitting();
		mEatBonusFx->clear();
		mEatBonusFx->setPosition(mNose.x, mNose.y);
		mEatBonusFx->startEmitting();
	}
	if (0 < aFoodCount)
	{
		Point aPlace;
		aPlace.x = randomFloat(Point(-CHOMP_SPREAD, CHOMP_SPREAD));
		aPlace.y = -randomFloat(Point(-CHOMP_SPREAD, CHOMP_SPREAD));
		float aForward;
		if (mVelocity.x <= 0.0f)
			aForward = CHOMP_FORWARD_FACING_LEFT;
		else
			aForward = CHOMP_FORWARD_FACING_RIGHT;
		aPlace.x = aForward + aPlace.x;
		int aSize = 0;
		for (int i = 0; i < getGrowthLevelCount(); i++)
		{
			if (mGrowthLevels[i].mLevel < aHighestLevel)
				aSize++;
		}
		aPlace.add(mNose);
		float aChance = randomFloat(Point(0.0f, 1.0f));
		if (aSize == 0)
		{
			if (CHOMP_MANY_FISH <= aFoodCount || aChance < CHOMP_SMALL_CHANCE)
				getStage()->showChomp(0, aPlace.x, aPlace.y);
		}
		else if (aSize == 1)
		{
			if (!(aFoodCount < CHOMP_MANY_FISH && CHOMP_MEDIUM_CHANCE <= aChance))
				getStage()->showChomp(1, aPlace.x, aPlace.y);
		}
		else
		{
			getStage()->showChomp(aSize, aPlace.x, aPlace.y);
		}
	}
	if (aHadVictims)
	{
		char aName[16];
		snprintf(aName, sizeof(aName), "bite%d", aHighestRank);
		playSound(aName);
	}
	updateGrowth();
	mEating = false;
}

// 0x412d60
void PlayerFish::updateGrowth()
{
	if (mState == PLAYER_DYING || mState == PLAYER_FURY)
		return;

	mGrowthIndex = 0;
	for (int i = 0; i < getGrowthLevelCount(); i++)
	{
		if ((float)mGrowthLevels[i].mScore <= mScore)
			mGrowthIndex = i;
	}
	const GrowthLevel& aLevel = mGrowthLevels[mGrowthIndex];
	if (aLevel.mLevel != mLevel)
	{
		if (aLevel.mLevel <= mLevel)
		{
			mLevel = aLevel.mLevel;
			mSize = aLevel.mSize;
			setScale(mSize, mSize);
		}
		else
		{
			mGrowToIndex = mGrowthIndex;
			mGrowFromIndex = mGrowthIndex - 1;
			if (mGrowFromIndex < 0)
				mGrowFromIndex = 0;
			if (mGrowthLevels[mGrowFromIndex].mSize < mGrowthLevels[mGrowToIndex].mSize)
			{
				getStage()->showLevelUp();
				playSound("playerGrow");
				if (FishFood::instance()->getSettings().areTipsEnabled())
				{
					mThoughtBubble->reset();
					mThoughtBubble->show(true, false);
				}
			}
			mState = PLAYER_GROW;
			mGrowSeconds = GROW_SECONDS;
			mLevel = mGrowthLevels[mGrowthIndex].mLevel;
		}
		if (mGrowthIndex == getGrowthLevelCount() - 1)
			getStage()->clearStage();
	}
}

// 0x412ff0
void PlayerFish::startBeingEaten(FishBase* theEater, float theSeconds)
{
	if (mShieldImmuneSeconds <= 0.0f || theEater != mShieldEater)
	{
		mBeingEatenSeconds = theSeconds;
		mEater = theEater;
		mState = PLAYER_BEING_EATEN;
		mBeingEatenLeft = theSeconds;
		FishBase::startBeingEaten(theEater, EATEN_BY_SECONDS);
	}
}

// 0x413100
bool PlayerFish::finishBeingEaten(FishBase* theEater)
{
	if (isInvincible())
		return false;
	if (mShieldImmuneSeconds <= 0.0f || theEater != mShieldEater)
	{
		if (mEater == nullptr)
			mEater = theEater;
		Point aPlace(0.0f, -randomFloat(Point(-CHOMP_SPREAD, CHOMP_SPREAD)));
		aPlace.add(mNose);
		getStage()->showChomp(mGrowToIndex, aPlace.x, aPlace.y);
		die();
		return true;
	}
	loseShield();
	return false;
}

// 0x413380
void PlayerFish::stun(float theSeconds, FishBase*, bool theShowEffect)
{
	if (mStunImmuneSeconds <= 0.0f && mStunSeconds <= 0.0f)
	{
		mStunSeconds = theSeconds;
		float aHalfLength = fabsf((mTail.x - mNose.x) * 0.5f);
		if (theShowEffect)
		{
			mStars->setPosition(STUN_STARS_X, STUN_STARS_Y);
			float aScale = 1.0f / mSize;
			mStars->setScale(fabsf(aScale), fabsf(aScale));
			mStars->play();
			mStars->setVisible(true);
			mStunFx->setPosition(aHalfLength, 0.0f);
			aScale = 1.0f / mSize;
			mStunFx->setScale(fabsf(aScale), fabsf(aScale));
			mStunFx->startEmitting();
			playSound("playerStunned");
			mDesiredVelocity.set(0.0f, 0.0f);
		}
	}
}

// 0x413580
void PlayerFish::poison(float theSeconds)
{
	if (mPoisonCooldown <= 0.0f && mPoisonLeft <= 0.0f)
	{
		mPoisonTotal = theSeconds;
		mPoisonLeft = theSeconds;
		mPoisonBlend = POISON_START_BLEND;
		mPoisonGroup->setPosition(0.0f, (float)(((double)mNose.y - mPosition.y) - (double)mBounds.height() * 0.5 -
		                                         POISON_BAR_GAP));
		float aScale = 1.0f / mSize;
		mPoisonGroup->setScale(fabsf(aScale), fabsf(aScale));
		mPoisonGroup->setVisible(true);
		getStage()->showMessage(FishFood::instance()->getStrings().get("poisoned"));
		playSound("playerPoisoned");
		mPoisonRamped = false;
		mStars->setPosition(STUN_STARS_X, STUN_STARS_Y);
		aScale = fabsf(1.0f / mSize);
		mStars->setScale(aScale, aScale);
		mStars->play();
		mStars->setVisible(true);
	}
}

// 0x413780
void PlayerFish::loseShield()
{
	mShieldCount--;
	if (mShieldCount == 0)
		mShieldFx->hide();
	mEatBonusFx->startEmitting();
	mEatBonusFx->setPosition(*mShieldFx->getPosition());
	playSound("playerLostShield");
}

// 0x413810
void PlayerFish::startFeedingFury()
{
	if (mState == PLAYER_SPECIAL)
		releaseSpecial();
	beginFury();
	getStage()->fadeMusicOut();
	playSound("starPickup");
	playSound("furyStart");
}

// 0x4138b0
void PlayerFish::beginFury()
{
	const std::vector<FishBase*>& aFish = getStage()->getFishList();
	for (std::vector<FishBase*>::const_iterator anIt = aFish.begin(); anIt != aFish.end(); ++anIt)
	{
		FishBase* aOther = *anIt;
		if (aOther->isInStage() && aOther != this)
			static_cast<AIFish*>(aOther)->freeze();
	}
	mFuryTarget = findFuryTarget();
	mState = PLAYER_FURY;
	mFuryPhase = FURY_CHASE;
	mFuryReturn = *getPosition();
	mStunnable = false;
}

// 0x413a20
void PlayerFish::endFury()
{
	const std::vector<FishBase*>& aFish = getStage()->getFishList();
	for (std::vector<FishBase*>::const_iterator anIt = aFish.begin(); anIt != aFish.end(); ++anIt)
	{
		FishBase* aOther = *anIt;
		if (aOther->isInPlay() && aOther != this)
			static_cast<AIFish*>(aOther)->unfreeze();
	}
	mState = PLAYER_SWIM;
	mVelocity.set(0.0f, 0.0f);
	updateGrowth();
	mStunnable = true;
}

// 0x413b70
void PlayerFish::finishFury()
{
	mFuryPhase = FURY_RETURN;
	playSound("furyEnd");
	getStage()->fadeMusicIn();
}

// 0x414af0
void PlayerFish::loadAnimationsOf(const std::string& theFolder)
{
	std::string aSwim = theFolder + "swim\\swim_cycle.";
	std::string aTurn = theFolder + "turn\\turn_cycle.";
	std::string anEat = theFolder + "eat\\eat_cycle.";
	std::string anIdle = theFolder + "idle\\idle_cycle.";
	loadAnimations(aSwim.c_str(), aTurn.c_str(), anEat.c_str(), anIdle.c_str());
	mSpawnBubblesFx = new ParticleSystem();
	addChild(mSpawnBubblesFx);
	mSpawnBubblesFx->load(SPAWN_BUBBLES_FILE);
}

// 0x415010
void PlayerFish::loadEffects()
{
	mDashFx = new ParticleSystem();
	addChild(mDashFx);
	mDashFx->load(DASH_FILE);

	mStunFx = new ParticleSystem();
	addChild(mStunFx);
	mStunFx->load(STUN_FILE);

	mStars = new AnimImage();
	addChild(mStars);
	mStars->load(STARS_PATH, STARS_EXTENSION, 1);
	mStars->setVisible(false);

	mEatFishFx = new ParticleSystem();
	getStage()->addToPlayGroup(mEatFishFx);
	mEatFishFx->load(EAT_FISH_FILE);

	mEatBonusFx = new ParticleSystem();
	getStage()->addToPlayGroup(mEatBonusFx);
	mEatBonusFx->load(EAT_BONUS_FILE);

	Rect aBarRect(0.0f, 0.0f, POISON_BAR_WIDTH, POISON_BAR_HEIGHT);
	mPoisonGroup = new Container();
	addChild(mPoisonGroup);
	mPoisonGroup->setVisible(false);

	mPoisonBack = new GrowMeter();
	mPoisonGroup->addChild(mPoisonBack);
	mPoisonBack->setBarColor(POISON_BAR_BACK_RED, POISON_BAR_BACK_GREEN, POISON_BAR_BACK_BLUE);
	mPoisonBack->setValue(1.0f);
	mPoisonBack->setRect(aBarRect);

	mPoisonFront = new GrowMeter();
	mPoisonGroup->addChild(mPoisonFront);
	mPoisonFront->setBarColor(POISON_BAR_FRONT_RED, POISON_BAR_FRONT_GREEN, POISON_BAR_FRONT_BLUE);
	mPoisonFront->setValue(0.0f);
	mPoisonFront->setRect(aBarRect);

	mThoughtBubble = new PlayerThoughtBubble();
	getStage()->addToOverlay(mThoughtBubble);
	mThoughtBubble->init(this);

	mStunBubbleFx = new StunBubbleFx();
	mStunBubbleFx->init();

	mShieldFx = new PlayerShieldFx();
	mShieldFx->init();
	mShieldFx->attach(this);
}

// 0x4154b0
void PlayerFish::eat(FishBase* theVictim)
{
	if (isDying() || theVictim == nullptr)
		return;

	CollisionRect aVictimHit = *theVictim->getPlacedHitRect();
	if (!aVictimHit.intersects(mPlacedBodyRect) && mState != PLAYER_FURY)
	{
		// A bonus bubble is picked up by its mouth too
		CollisionRect aVictimMouth = *theVictim->getMouthRect();
		if (!theVictim->isPickup() || !aVictimMouth.intersects(mPlacedBodyRect))
			return;
	}
	if (MOST_VICTIMS <= mVictims.size() && mState != PLAYER_FURY)
		return;

	if (mState == PLAYER_FURY && theVictim == mFuryTarget)
		mFuryPhase = FURY_PICK;
	addToChain(1);
	theVictim->startBeingEaten(this, EATEN_BY_SECONDS);
	mVictims.push_back(theVictim);
	if (mState == PLAYER_FURY)
	{
		mEatFinished = false;
		finishEating();
	}
	else
	{
		playEatAnimation();
	}
}

// 0x415810
void PlayerFish::onAte(FishBase* theVictim)
{
	FishFood* aFood = FishFood::instance();
	std::string aName = theVictim->getTypeName();
	if (aName == "BonusBubble2X")
	{
		mScoreMultiplier = DOUBLE_SCORE_FACTOR;
		mDoubleScoreSeconds = DOUBLE_SCORE_SECONDS;
		getStage()->showMessage(aFood->getStrings().get("2xBonus"));
		return;
	}
	if (aName == "BonusBubbleSpeed")
	{
		mSpeedBoostSeconds = SPEED_BOOST_SECONDS;
		playSound("playerSpeedUp");
		getStage()->showMessage(aFood->getStrings().get("speedBonus"));
		if (1.0f < mSpeedMultiplier)
			return;
		mSpeedMultiplier = SPEED_BOOST_FACTOR;
		mMaxSpeedX = mMaxSpeedX * SPEED_BOOST_FACTOR;
		mMaxSpeedY = mMaxSpeedY * SPEED_BOOST_FACTOR;
		mSpeedUp = mSpeedUp * SPEED_BOOST_FACTOR;
		return;
	}
	if (aName == "BonusBubbleFreeLife")
	{
		getSession()->addLives(1);
		getStage()->showMessage(aFood->getStrings().get("extraLife"));
		playSound("extraLife");
		return;
	}
	if (aName == "BonusBubbleFeedingFury")
	{
		if (mState != PLAYER_SWIM && mState != PLAYER_INVINCIBLE)
			return;
		startFeedingFury();
		return;
	}
	if (aName == "BonusBubbleStun")
	{
		stunAll();
		return;
	}
	if (aName == "BonusBubbleShield")
	{
		gainShield();
		return;
	}
	bool aIsStar = aName == "BonusBubbleStar";
	if (getStage()->getType() == STAGE_TIMED_NORMAL && aIsStar)
		getStage()->showTimeBonus(STAR_TIME_BONUS_SECONDS);

	if (aName == "Oyster")
	{
		playSound("oysterPearl");
		int aPearl = static_cast<Oyster*>(theVictim)->getPearlKind();
		if (aPearl == PEARL_LIFE)
		{
			getSession()->addLives(1);
			getStage()->showMessage(aFood->getStrings().get("extraLife"));
			playSound("extraLife");
		}
		else
		{
			if (aPearl == PEARL_BLACK)
				getStage()->showMessage(aFood->getStrings().get("blackPearl"));
			mScore = (float)((double)theVictim->getPoints() * mScoreMultiplier + mScore);
		}
	}
	else if (aName == "GoldenMinnow")
	{
		mScore = getScoreToNextLevel() + mScore;
		getStage()->showMessage(aFood->getStrings().get("goldenMinnow"));
	}
	else if (!theVictim->isHuge() && 0.0f < theVictim->getPoints())
	{
		mScore = (float)((double)theVictim->getPoints() * mScoreMultiplier + mScore);
	}

	if (theVictim->getPoints() != 0.0f)
		mPendingScores.push_back((int)theVictim->getPoints());
	if (aIsStar)
		playSound("starPickup");
}

// 0x416060
void PlayerFish::addGrowthLevel(float theLevel, float theSize, int theScore)
{
	GrowthLevel aLevel;
	aLevel.mLevel = theLevel;
	aLevel.mSize = theSize;
	aLevel.mScore = theScore;
	mGrowthLevels.push_back(aLevel);
}

// 0x416090
void PlayerFish::onSchoolEaten()
{
	getStage()->showMessage(FishFood::instance()->getStrings().get("schoolBonus"));
	mPendingScores.push_back(SCHOOL_BONUS_POINTS);
	playSound("schoolBonus");
}

} // namespace feedingfrenzy
