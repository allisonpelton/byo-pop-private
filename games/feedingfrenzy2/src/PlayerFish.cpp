#include "PlayerFish.h"

#include "Draws.h"
#include "Engine.h"
#include "GameData.h"
#include "GameHud.h"
#include "Mouse.h"
#include "Random.h"
#include "ScoreSplat.h"
#include "Stage.h"
#include "VectorMath.h"
#include "Visual.h"

#include <cmath>

using namespace engine;

namespace
{

const float NORMAL_SPEED_SCALE = 1.0f;
const double FALL_FORCE = 2000.0;           // 0x566878 (feedingfrenzy2): the pull down on the share out of the water
const float SPAWN_FORCE = 2000.0f;          // 0x5669a4 (feedingfrenzy2): the push down while dropping in
const float SPAWN_FLASH_SECONDS = 3.0f;     // 0x54dda0 (feedingfrenzy2)
const float SPAWN_DEPTH = 500.0f;           // 0x566ce8 (feedingfrenzy2): the deepest the drop in goes
const double SPAWN_DEPTH_SHARE = 2.5;       // 0x566958 (feedingfrenzy2): the water's height over it, if shallower
const double MOTION_SCALE = 900.0;          // 0x566a00 (feedingfrenzy2): the mouse's motion per second for a force
const float MOTION_DEAD_ZONE = 0.1f;        // 0x5667c0 (feedingfrenzy2), a double of the float
const double AIR_MOTION_SHARE = 0.5;        // 0x566760 (feedingfrenzy2): across the mouse's motion out of the water
const float STEER_SCALE = 1.25f;            // 0x5669f8 (feedingfrenzy2)
const float WATER_STEER_SHARE = 0.8f;       // 0x5669f0 (feedingfrenzy2), a double of the float: the steer's part the
const float AIR_STEER_SHARE = 0.2f;         // 0x5669c8 (feedingfrenzy2), likewise: water's share gives, and the rest
const double RISE_AT_SURFACE = 0.30000001192092896;  // 0x5668f0 (feedingfrenzy2): a rise slowed under the surface
const double ENTRY_FALL_SHARE = 0.5;        // 0x566760 (feedingfrenzy2): the fall kept as the fish meets the water
const double CENTRE = 0.5;                  // 0x566760 (feedingfrenzy2)
const int FRENZY_MOST = 35;                 // 0x47c56d (feedingfrenzy2): the frenzy's count at most
const int FRENZY_STEP = 7;                  // 0x47c58a (feedingfrenzy2): the fish of each step of the multiplier
const double FRENZY_STEP_HOLD = 1.5;        // 0x566b68 (feedingfrenzy2): the hold's scale as a step is reached
const double FRENZY_STEP_DROP = 1.399999976158142;  // 0x566b08 (feedingfrenzy2): the time's scale as the count drops
                                            // to a step
const double FRENZY_HIGH_DROP = 0.25;       // 0x566a80 (feedingfrenzy2): likewise above the first step
const double FRENZY_LOW_DROP = 3.0;         // 0x566868 (feedingfrenzy2): the divisor of the time within the first
const float CHOMP_SPREAD = 15.0f;           // 0x566858 and 0x566ce0 (feedingfrenzy2): the chomp's place's draws,
                                            // either way
const float CHOMP_LEAD_RIGHT = 1.0f;        // the chomp's place ahead of the mouth facing right
const float CHOMP_LEAD_LEFT = -15.0f;       // 0x566ce0 (feedingfrenzy2): and facing left
const float SMALL_CHOMP_CHANCE = 0.5f;      // 0x54ddb0 (feedingfrenzy2): below it a bite of rank 0 splats
const float MEDIUM_CHOMP_CHANCE = 0.65f;    // 0x566d20 (feedingfrenzy2): likewise of rank 1
const int SPLATTING_BITE = 2;               // more prey than this always splats

// The options' mouse sensitivity at each of their MouseSpeed's steps (0x54dd70 (feedingfrenzy2))
const float MOUSE_SENSITIVITIES[4] = {0.8f, 1.0f, 1.15f, 1.3f};
// MouseSpeed's default (FeedingFrenzyTwoOptions's load, 0x40ba40 (feedingfrenzy2))
const int DEFAULT_MOUSE_SPEED = 1;

// The options' slot 0x44 (0x43ae70 (feedingfrenzy2)): the sensitivity of the MouseSpeed held to 0 to 3. The options
// (the application's +0xb8, FeedingFrenzyTwoOptions) are not yet translated: their MouseSpeed is its default until
// they are
float getMouseSensitivity()
{
	int aSpeed = DEFAULT_MOUSE_SPEED;
	if (aSpeed < 0)
		aSpeed = 0;
	else if (aSpeed > 3)
		aSpeed = 3;
	return MOUSE_SENSITIVITIES[aSpeed];
}

} // namespace

PlayerFish::PlayerFish()
    : mSpawn(nullptr), mMotion(0, 0), mLastMotion(0, 0), mMouse(nullptr), mWaterShare(1.0f), mInWater(false),
      mSucking(false), mCanSuck(false), mDashTime(0.0f), mPush(0.0f, 0.0f), mSuckCharge(1.0f), mBiggestRankEaten(0), mBonusEat(false),
      mPointsScale(1), mSpeedScale(NORMAL_SPEED_SCALE), mFrenzyCount(0), mFrenzyTime(0.0f), mFrenzyHoldTime(0.0f),
      mFrenzyMultiplier(1), mJumpHeight(0.0f), mJumpPeakShown(false), mAirChain(0), mKeptBetweenSides(false)
{
}

void PlayerFish::create(Stage* theStage)
{
	Fish::create(theStage);
	// The theme's playerStunned particles in the visual's first effect, and its playerDashFx, playerFallFx,
	// playerEatFx and playerEatBonusFx (+0x210 to +0x21c) and the angler's glow: not yet translated, as the effects
	// are not
}

void PlayerFish::addToStage(Stage* theStage, int theLocation)
{
	Fish::addToStage(theStage, theLocation);
	mPush = Vector2f(0.0f, 0.0f);
	// The swallowed list (+0x328) emptied and its timer (+0x338) at -1, the visual's 0x40a820 (feedingfrenzy2), and
	// the eat effects (+0x214 and +0x218) put in the layer's container: not yet translated, as the eating and the
	// effects are not
	mCanSuck = theStage->getCanSuck();
	theStage->addPlayer(this);
	// The HUD's rank (0x48e6e0 (feedingfrenzy2), the eater rank): not yet translated, as the HUD is not
	RectF anArea = theStage->getArea(true);
	float anX = (float)(((double)anArea.left + anArea.right) * CENTRE);
	float aY = (float)(((double)anArea.top + anArea.bottom) * CENTRE);
	setPosition(anX, aY);
	mSpeedScale = NORMAL_SPEED_SCALE;
	mBiggestRankEaten = 0;
	mBonusEat = false;
	mPointsScale = 1;
	// The bonus timers (+0x238 and +0x268, 0x406fb0 (feedingfrenzy2)) reset: read by code not yet translated
	mDashTime = 0.0f;
	// +0x1e0 and +0x1e4 at 0: read by code not yet translated
	mFrenzyCount = 0;
	mFrenzyMultiplier = 1;
	mFrenzyTime = mFrenzyHoldTime;
	showFrenzy();
	mSuckCharge = 1.0f;
	mAirChain = 0;
	// +0x2ac, +0x1a0 and +0x2b4 at 0: read by code not yet translated
	mKeptBetweenSides = true;
	// The name at +0x30c cleared (0x4574c0 (feedingfrenzy2)): read by code not yet translated
	changeState(new SpawnState(this));
}

void PlayerFish::remove()
{
	// 0x4774f0 (feedingfrenzy2): the eater's prey let go
	mPrey.clear();
	mStateMgr.changeState(nullptr);
	// The dash effect stopped, and the eat effects taken out of the visual's parent: not yet translated, as the
	// effects are not
	mStage->removePlayer(this);
	Fish::remove();
}

void PlayerFish::move(UpdateContext& theContext)
{
	float anElapsed = theContext.mElapsed;
	mForce.y = (float)((1.0 - mWaterShare) * FALL_FORCE + mForce.y);
	addScaled(mVelocity, mForce, anElapsed);

	// The drag (0x43ae90 (feedingfrenzy2)): the water's on the square of the speed, at the share in the water and the
	// sensitivity, and the drag rate; a speed below it stops (and sets +0xc9, read by nothing translated yet)
	float aSensitivity = getMouseSensitivity();
	float aDrag = (float)(lengthSquared(mVelocity) * mFishBehavior->mFluidDrag * ((double)mWaterShare * aSensitivity) +
	                      mDragRate);
	if ((double)length(mVelocity) < (double)aDrag)
		mVelocity = Vector2f(0.0f, 0.0f);
	else
	{
		Vector2f aDirection = mVelocity;
		normalize(aDirection);
		addScaled(mVelocity, aDirection, -aDrag);
	}

	aSensitivity = getMouseSensitivity();
	if (!(0.0f < mDashTime))
	{
		// 0x43aa10 (feedingfrenzy2) twice: the top speed at the speed's scale and the sensitivity
		Vector2f aLimit = scaled(scaled(mMaxSpeed, mSpeedScale), aSensitivity);
		clampToEllipse(mVelocity, aLimit);
	}
	// The visual's third effect (+0x328) while it plays drifts the fish: not yet translated, as the effects are not
	Vector2f aDrift(0.0f, 0.0f);
	addScaled(mPosition, mVelocity, anElapsed);
	addScaled(mPosition, aDrift, anElapsed);
	addScaled(mPosition, mPush, anElapsed);
	updateTranslation();
	keepInWater();
	mVisual->setPosition(mPosition.x, mPosition.y);
	mForce = Vector2f(0.0f, 0.0f);
}

void PlayerFish::update(UpdateContext& theContext)
{
	checkLayer(theContext);
	if (0.0f < mHoldTime)
	{
		float aLeft = (float)((double)mHoldTime - theContext.mElapsed);
		mHoldTime = aLeft;
		if (!(aLeft > 0.0f))
			mHoldTime = 0.0f;
	}
	// The swallowed list (0x455950 (feedingfrenzy2)), the bonuses' timers (+0x238 and +0x268, the bonus's speed's end
	// putting the speed's scale back with the sound bonusSpeedEnd), and +0x20c brought up towards 1 and the suck
	// begun while the right button is held (0x472110 and 0x494c00 (feedingfrenzy2)) when the state allows it: not yet
	// translated, as the swallowed list, the bonuses and the suck are not
	// The points scored, oldest first, each added to the score and its bubble sent from the mouth, in the stage's
	// space from the fish's layer, by its multiplier
	while (!mScored.empty())
	{
		ScoredPoints aScored = mScored.front();
		mScored.erase(mScored.begin());
		mStage->addPlayerScore(aScored.mPoints, this);
		Vector2f aMouth = getTransformedShape(ACTOR_SHAPE_MOUTH)->getCenter();
		Vector2f aLayer = mStage->getLayerContainer(mLayer)->getAbsolutePosition();
		Vector2f aFrom((float)((double)aMouth.x + aLayer.x), (float)((double)aLayer.y + aMouth.y));
		addPointsBubble(aFrom, mStage->getHud(), aScored.mPoints, aScored.mMultiplier);
	}
	// The tilt (0x44ebc0 (feedingfrenzy2)) and the puke (+0x110): not yet translated
	mStateMgr.update(&theContext);
}

void PlayerFish::eat(Actor* thePrey, float theSeconds)
{
	Fish::eat(thePrey, theSeconds);
	mBiggestRankEaten = maxOf(mBiggestRankEaten, thePrey->getRank());
	// The theme's sound fishEaten: not yet translated, as the sounds are not
}

void PlayerFish::swallow()
{
	if (!mPrey.empty())
	{
		// The eat effect (+0x214) at the mouth started, and played in the water; with the bonus's bite the bonus's
		// effect (+0x218) too: not yet translated, as the effects are not
		// The chomp's place: the mouth (times 1) moved by a draw either side, 1 more ahead facing right and 15 behind
		// facing left, and a draw up or down, in the stage's space from the fish's layer (0x4678b0 (feedingfrenzy2));
		// then a draw is the chance a small bite splats
		Vector2f aMouth = getTransformedShape(ACTOR_SHAPE_MOUTH)->getCenter();
		float anAside = (float)randomFloat(-CHOMP_SPREAD, CHOMP_SPREAD);
		float anUp = (float)-randomFloat(-CHOMP_SPREAD, CHOMP_SPREAD);
		float aLead = CHOMP_LEAD_LEFT;
		if (0.0f < mFishVisual->getFacing())
			aLead = CHOMP_LEAD_RIGHT;
		anAside = (float)((double)aLead + (double)anAside);
		Vector2f aPlace = moved(aMouth, anAside, anUp);
		Vector2f aLayer = mStage->getLayerContainer(mLayer)->getAbsolutePosition();
		Vector2f aChompAt((float)((double)aPlace.x + aLayer.x), (float)((double)aLayer.y + aPlace.y));
		float aChance = (float)randomFloat(0.0f, 1.0f);
		int aBite = (int)mPrey.size();
		addFrenzy(aBite, 0.0f);
		bool isSplat;
		if (mBiggestRankEaten == 0)
			isSplat = aBite > SPLATTING_BITE || aChance < SMALL_CHOMP_CHANCE;
		else if (mBiggestRankEaten == 1)
			isSplat = aBite > SPLATTING_BITE || aChance < MEDIUM_CHOMP_CHANCE;
		else
			isSplat = true;
		if (isSplat)
			mStage->getHud()->splatChomp(mBiggestRankEaten, aChompAt);
		// The theme's sound fishChomp1 to fishChomp3 by the rank: not yet translated, as the sounds are not
	}
	for (const Ref<Actor>& aPrey : mPrey)
	{
		int aFood = aPrey->getFoodValue();
		int aPoints = aPrey->getFoodValueNow() * mFrenzyMultiplier * mPointsScale;
		if (!aPrey->isUnderwater())
		{
			mAirChain++;
			aPoints *= mAirChain;
		}
		addScore(aPoints);
		mStage->getGameData()->addFood(aFood);
		aPrey->eatenByPlayer(this);
	}
	if (mInWater)
		mAirChain = 0;
	mBiggestRankEaten = 0;
	mBonusEat = false;
	Fish::swallow();
}

void PlayerFish::addScore(int thePoints)
{
	if (thePoints > 0)
		mScored.push_back(ScoredPoints(thePoints, mFrenzyMultiplier));
}

void PlayerFish::addFrenzy(int theCount, float theSeconds)
{
	mFrenzyCount += theCount;
	if (mFrenzyCount > FRENZY_MOST)
		mFrenzyCount = FRENZY_MOST;
	if (0.0f < theSeconds)
		mFrenzyTime = theSeconds;
	else
	{
		float aHold = mFrenzyHoldTime;
		if (mFrenzyCount % FRENZY_STEP == 0)
			aHold = (float)(mFrenzyHoldTime * FRENZY_STEP_HOLD);
		mFrenzyTime = maxOf(mFrenzyTime, aHold);
	}
	int aMultiplier = mFrenzyCount / FRENZY_STEP + 1;
	int aLast = mFrenzyMultiplier;
	mFrenzyMultiplier = aMultiplier;
	if (aMultiplier > aLast)
	{
		// The theme's sounds frenzy1 to frenzy5 by the step and frenzyChime: not yet translated, as the sounds are not
		mStage->getHud()->splatFrenzy(mFrenzyMultiplier - 1);
	}
	showFrenzy();
}

void PlayerFish::showFrenzy()
{
	mStage->getHud()->showFrenzy(getFrenzyCount(), getFrenzyMultiplier());
}

void PlayerFish::updatePlayTimers(UpdateContext& theContext)
{
	// 0x437950 (feedingfrenzy2): while +0x1ec is above 0 (set by a bonus not yet translated), a fish added to the
	// frenzy each tenth of a second: not yet translated
	if (!(0.0f < mFrenzyTime))
		return;
	float aLeft = (float)((double)mFrenzyTime - theContext.mElapsed);
	mFrenzyTime = aLeft;
	if (!(0.0f > aLeft) || mFrenzyCount <= 0)
		return;
	mFrenzyCount--;
	double aTime = mFrenzyHoldTime;
	if (mFrenzyCount % FRENZY_STEP == 0)
		aTime *= FRENZY_STEP_DROP;
	else if (mFrenzyCount > FRENZY_STEP)
		aTime *= FRENZY_HIGH_DROP;
	else
		aTime /= FRENZY_LOW_DROP;
	mFrenzyTime = (float)aTime;
	showFrenzy();
}

void PlayerFish::keepInWater()
{
	pushOut(mStage->getEdge(STAGE_EDGE_BOTTOM));
	if (mKeptBetweenSides)
	{
		pushOut(mStage->getEdge(STAGE_EDGE_LEFT));
		pushOut(mStage->getEdge(STAGE_EDGE_RIGHT));
	}
	shape::Edge* aSurface = static_cast<shape::Edge*>(mStage->getEdge(STAGE_EDGE_BELOW_WATER));
	if (aSurface->mDistance != 0.0f)
		keepAtSurface(aSurface);
	else
		pushOut(mStage->getEdge(STAGE_EDGE_SKY));
}

void PlayerFish::setPlayerVisualData(float theSize, FishVisualData* theData)
{
	setFishVisualData(theSize, theData);
	// In the stage, the rank's name over the fish (0x405630 (feedingfrenzy2)): not yet translated
}

void PlayerFish::startPlay()
{
	changeState(new PlayState(this));
}

void PlayerFish::pushOut(Shape* theEdge)
{
	ShapeContact aContact;
	getTransformedShape(ACTOR_SHAPE_BODY)->collide(*theEdge, aContact);
	if (0.0f > aContact.mSeparation)
	{
		addScaled(mPosition, aContact.mNormal, aContact.mSeparation);
		updateTranslation();
		float aCross = (float)-dot(mVelocity, aContact.mNormal);
		addScaled(mVelocity, aContact.mNormal, aCross);
	}
}

void PlayerFish::connectMouse()
{
	if (mMouse != nullptr)
		return;
	mMouse = getEngine()->getMouse();
	mMouse->mMotionSignal.connect(this, &PlayerFish::onMouseMotion);
	mMouse->mButtonDownSignal.connect(this, &PlayerFish::onMouseDown);
	mMouse->mButtonUpSignal.connect(this, &PlayerFish::onMouseUp);
	mMotion = Vector2<int>(0, 0);
}

void PlayerFish::disconnectMouse()
{
	if (mMouse == nullptr)
		return;
	// The original leaves the releases connected, so each pause and resume adds another connection to them
	mMouse->mMotionSignal.disconnect(this);
	mMouse->mButtonDownSignal.disconnect(this);
	mMouse = nullptr;
	mSucking = false;
}

void PlayerFish::steer(UpdateContext& theContext)
{
	float aSensitivity = getMouseSensitivity();
	// 0x4375a0 (feedingfrenzy2)
	Vector2f aMotion((float)mMotion.x, (float)mMotion.y);
	mLastMotion = mMotion;
	mMotion = Vector2<int>(0, 0);
	float aScale = (float)(((double)aSensitivity + aSensitivity) / ((double)theContext.mElapsed * MOTION_SCALE));
	aMotion = scaled(aMotion, aScale);
	float aSize = std::fabs(aMotion.x);
	if ((double)MOTION_DEAD_ZONE > (double)aSize)
		aMotion.x = 0.0f;
	aSize = std::fabs(aMotion.y);
	if ((double)aSize < (double)MOTION_DEAD_ZONE)
		aMotion.y = 0.0f;
	if (!mInWater)
	{
		aMotion.y = 0.0f;
		aMotion.x = (float)(aMotion.x * AIR_MOTION_SHARE);
	}
	// While the visual's effect at +0x2a0 plays its first steps (its event 1 or 2), the motion is turned back at 0.7:
	// not yet translated, as the effects are not
	Vector2f aForce = scaled(aMotion, (float)((double)mFishBehavior->mAccelRate * mSpeedScale));
	aForce = scaled(aForce, STEER_SCALE);
	// 0x43aa10 (feedingfrenzy2)
	aForce = scaled(aForce, (float)((double)mWaterShare * (double)WATER_STEER_SHARE + (double)AIR_STEER_SHARE));
	mForce = moved(mForce, aForce.x, aForce.y);
}

void PlayerFish::updateTimers(UpdateContext& theContext)
{
	float anElapsed = theContext.mElapsed;
	if (mSpawnFlash.advance(anElapsed))
	{
		mSpawnFlash.mNext++;
		// 0x4941e0 (feedingfrenzy2): the visual's flash taken off and its tint set again: not yet translated, as the
		// visual's modifiers are not
	}
	if (0.0f < mDashTime)
	{
		float aLeft = (float)((double)mDashTime - anElapsed);
		mDashTime = aLeft;
		if (!(0.0f < aLeft))
		{
			// 0x40f660 (feedingfrenzy2): the dash over, the drag rate the behaviour's again (the dash effect stopped
			// and the visual's dash ended, not yet translated)
			mDragRate = mFishBehavior->mDragRate;
			mDashTime = 0.0f;
		}
	}
	// +0x1dc, +0x1e0 and +0x1e4 counted down (each set by code not yet translated), then the fall effect (+0x21c)
	// started when it is not playing and the fish rises: not yet translated, as the effects are not
}

void PlayerFish::keepAtSurface(Shape* theSurface)
{
	Shape* aBody = getTransformedShape(ACTOR_SHAPE_BODY);
	float aLevel = theSurface->getCenter().y;
	float aShare = aBody->getShareBelow(aLevel);
	mWaterShare = aShare;
	// The visual tinted the stage's water colour at its alpha times the share (the visual's slot 0x48): not yet
	// translated, as the visual's tint is not
	if (1.0f > mWaterShare && !(0.0f < mDashTime) && mInWater && 0.0f > mVelocity.y)
	{
		mVelocity.y = (float)(mVelocity.y * RISE_AT_SURFACE);
		return;
	}
	if (0.0f == mWaterShare)
	{
		if (mInWater)
		{
			mInWater = false;
			leaveWater();
			return;
		}
		float aHeight = (float)((double)theSurface->getCenter().y - mPosition.y);
		if (mJumpHeight < aHeight)
		{
			mJumpHeight = aHeight;
			return;
		}
		if (mJumpPeakShown)
			return;
		mJumpPeakShown = true;
		// The jump's height in size units: above the player's highest (the user data's +0x3c) a jump height indicator
		// (JumpHeightIndicator, 0x2f0 bytes) at the fish, and the HUD told of it (0x53a435 (feedingfrenzy2)): not yet
		// translated, as the indicator and the HUD are not
		return;
	}
	if (!mInWater)
	{
		mInWater = true;
		enterWater();
	}
}

void PlayerFish::enterWater()
{
	// The splash at the fish, its size from the jump's height over 300 (the fish's slot 0x10c and the stage's
	// 0x51cc8f (feedingfrenzy2)), and the fall effect stopped: not yet translated, as the effects are not
	mVelocity.y = (float)(mVelocity.y * ENTRY_FALL_SHARE);
	// The visual's tint (0x48fac0 (feedingfrenzy2)), then with +0x2ac at 0 or less the sound playerSplash and, after
	// flips (+0x1a0), the bubble flipx%d and their score; else +0x19d set: not yet translated, as the visual's tint,
	// the sounds, the tilt and the score are not
}

void PlayerFish::leaveWater()
{
	mJumpHeight = 0.0f;
	mJumpPeakShown = false;
	// +0x2ac, +0x1a0, +0x1a4 and +0x2a8 at 0, the dash and eat effects stopped and the visual's slot 0x50: not yet
	// translated, as what reads them and the effects are not
}

Vector2f PlayerFish::getCameraTarget()
{
	Shape* aBody = getShape(ACTOR_SHAPE_BODY);
	Vector2f aCentre(0.0f, 0.0f);
	if (aBody != nullptr)
		aCentre = aBody->getCenter();
	return transformed(aCentre, mTransform);
}

void SpawnState::enter()
{
	// The theme's sound playerSplash: not yet translated, as the sounds are not
	mPlayer->setVelocity(Vector2f(0.0f, 0.0f));
	Stage* aStage = mPlayer->getStage();
	RectF anArea = aStage->getArea(true);
	// 0x459e00 (feedingfrenzy2): the area's width
	float anX = (float)(((double)anArea.right - anArea.left) * CENTRE);
	mPlayer->setPosition(anX, 0.0f);
	anArea = aStage->getArea(true);
	if (0.0f < anArea.top)
	{
		mPlayer->setJumpPeakShown(true);
		mPlayer->setInWater(false);
	}
	else
		mPlayer->setInWater(true);
	// +0x19d cleared, the visual's tint (0x48fac0 (feedingfrenzy2)) and the fall effect stopped: not yet translated,
	// as the visual's modifiers and the effects are not
	FishVisual* aVisual = mPlayer->getFishVisual();
	aVisual->changeState(aVisual->getSwimState());
	// The visual's flash put on (0x494080 (feedingfrenzy2)): not yet translated, as the visual's modifiers are not
	mPlayer->getSpawnFlash().mTimes.clear();
	mPlayer->getSpawnFlash().mTimes.push_back(SPAWN_FLASH_SECONDS);
	// The rank's name over the fish (0x405630 (feedingfrenzy2)): not yet translated
}

void SpawnState::exit()
{
	mPlayer->getSpawnFlash().restart();
	mPlayer->clearMotion();
}

void SpawnState::update(UpdateContext* theContext)
{
	mPlayer->setForce(Vector2f(0.0f, SPAWN_FORCE));
	mPlayer->move(*theContext);
	mPlayer->updateTimers(*theContext);
	RectF anArea = mPlayer->getStage()->getArea(true);
	// 0x459670 (feedingfrenzy2): the area's height
	float aDepth = (float)(((double)anArea.bottom - anArea.top) / SPAWN_DEPTH_SHARE);
	// 0x46d360 (feedingfrenzy2)
	float aLimit = SPAWN_DEPTH <= aDepth ? SPAWN_DEPTH : aDepth;
	if (!(aLimit < mPlayer->getMouthPosition().y))
		return;
	mPlayer->setForce(Vector2f(0.0f, 0.0f));
	// The fall effect stopped: not yet translated, as the effects are not
	mPlayer->startPlay();
}

void PlayState::eat(Actor* thePrey)
{
	mPlayer->changeState(new PlayerEatingState(mPlayer));
}

void PlayerEatingState::update(UpdateContext* theContext)
{
	mPlayer->steer(*theContext);
	mPlayer->move(*theContext);
	mPlayer->updateTimers(*theContext);
	mPlayer->updatePlayTimers(*theContext);
	// With +0x19d set (by code not yet translated), the cold path 0x540579 (feedingfrenzy2) instead: not yet
	// translated
	if (mChomped)
		mPlayer->startPlay();
}

void PlayState::update(UpdateContext* theContext)
{
	mPlayer->steer(*theContext);
	mPlayer->move(*theContext);
	mPlayer->updatePlayTimers(*theContext);
	mPlayer->updateTimers(*theContext);
	// With +0x19d set (by code not yet translated), the cold path 0x4f2ee7 (feedingfrenzy2): not yet translated
}
