#include "Actor.h"

#include "Container.h"
#include "Draws.h"
#include "PlayerFish.h"
#include "Random.h"
#include "Stage.h"
#include "VectorMath.h"
#include "Visual.h"

#include <cmath>

using namespace engine;

namespace
{

const char* const SHAPE_ROLES[3] = {"visual", "body", "mouth"};  // 0x54de64, 0x54de6c and 0x54de74 (feedingfrenzy2)
const double HALF = 0.5;                         // 0x566760 (feedingfrenzy2)
const float NO_DESIRE = 2.384185791015625e-07f;  // 0x566890 (feedingfrenzy2): a wanted speed's square below it is none
const float STEER_GAIN = 10.0f;                  // 0x54de60 (feedingfrenzy2): the wanted change's force per second
const float STOPPED_SPEED_SQUARED = 0.3125f;     // 0x566b98 (feedingfrenzy2): slower than its root is still
const float CURRENT = 0.0f;                      // 0x5ac618 (feedingfrenzy2): a drift to the left, never set
const float MINUS_ONE = -1.0f;                   // 0x566850 (feedingfrenzy2)
const float SIZE_PULSE_SECONDS = 0.25f;          // 0x5668b0 (feedingfrenzy2)
const float SWIM_RATE_SCALE = 0.0025f;           // 0x566884 (feedingfrenzy2)
const float SWIM_RATE_BASE = 0.05f;              // 0x566880 (feedingfrenzy2)
const float DEFAULT_CHASE_FREQUENCY = 0.5f;      // 0x54ddb0 (feedingfrenzy2)
const int SCHOOL_BONUS_POINTS = 500;             // 0x4f0ed6 (feedingfrenzy2): the last of a school eaten
const float DEFAULT_FLYBY_FREQUENCY = 0.3f;      // 0x54ddac (feedingfrenzy2)
const float NOT_SEEN = -1.0f;                    // 0x566850 (feedingfrenzy2): the distance to what is out of sight
const double ZERO = 0.0;                         // 0x566828 (feedingfrenzy2)
const float LONGEST_DRAW_IN = 0.2f;              // 0x54ddf8 (feedingfrenzy2): the most seconds a prey takes to the mouth

// The sign of a number, 0 for 0 (as the move compares the force's with the wanted one's)
int signOf(float theValue)
{
	if (0.0f < theValue)
		return 1;
	if (0.0f == theValue)
		return 0;
	return -1;
}

} // namespace

Actor::Actor()
    : mStage(nullptr), mLayer(0), mSeen(false), mLeaving(false), mFoodValue(0), mFoodValueNow(0),
      mPosition(0.0f, 0.0f), mForce(0.0f, 0.0f), mMaxForce(0.0f, 0.0f), mVelocity(0.0f, 0.0f),
      mMaxVelocity(0.0f, 0.0f), mDesiredVelocity(0.0f, 0.0f), mTurned(false), mAccelRate(0.0f), mDragRate(0.0f),
      mUnderwater(true), mRank(0)
{
	for (int i = 0; i < 3; i++)
		mDetailLayers[i] = 0;
}

void Actor::setupShapes()
{
	for (int i = 0; i < ACTOR_SHAPE_COUNT; i++)
		mShapes[i].mTransform = &mTransform;
	for (int i = 0; i < 3; i++)
		mShapes[i].mShape = mVisualData ? mVisualData->getShape(SHAPE_ROLES[i]) : nullptr;
}

void Actor::setVisualData(VisualData* theData)
{
	mVisual->setData(theData);
	mVisualData = theData;
	setupShapes();
}

void Actor::addToStage(Stage* theStage, int theLocation)
{
	mForce = Vector2f(0.0f, 0.0f);
	mVelocity = Vector2f(0.0f, 0.0f);
	mDesiredVelocity = Vector2f(0.0f, 0.0f);
	mTurned = false;
	mSeen = false;
	mStage = theStage;
	theStage->addActor(this);
	addVisualToLayer();
}

void Actor::remove()
{
	removeVisual();
	mStage->removeActor(this);
	mStage = nullptr;
}

Vector2f Actor::getEntryPoint(Stage* theStage, int& theLocation, const Vector2f& thePoint)
{
	if (theLocation == SPAWN_LOCATION_SIDE)
	{
		// 0x41ce50 (feedingfrenzy2): 0 or 1
		int aSide = crtRand() % 2;
		theLocation = aSide != 0 ? SPAWN_LOCATION_LEFT : SPAWN_LOCATION_RIGHT;
	}
	RectF aBounds = mBounds;
	Shape* anOutline = getShape(ACTOR_SHAPE_VISUAL);
	float aHalfWidth = (float)(anOutline->getWidth() * HALF);
	float aHalfHeight = (float)(anOutline->getHeight() * HALF);
	Vector2f aPoint = thePoint;
	if (theLocation >= SPAWN_LOCATION_LEFT && theLocation <= SPAWN_LOCATION_RIGHT)
	{
		float aLow = (float)((double)aBounds.top + aHalfHeight);
		float aHigh = (float)((double)aBounds.bottom - aHalfHeight);
		aPoint.y = (float)randomFloat(aLow, aHigh);
	}
	else if (theLocation >= SPAWN_LOCATION_BOTTOM && theLocation <= SPAWN_LOCATION_TOP)
	{
		float aLow = (float)((double)aBounds.left + aHalfWidth);
		float aHigh = (float)((double)aBounds.right - aHalfWidth);
		aPoint.x = (float)randomFloat(aLow, aHigh);
	}
	switch (theLocation)
	{
	case SPAWN_LOCATION_LEFT:
		aPoint.x = (float)((double)aBounds.left - aHalfWidth);
		break;
	case SPAWN_LOCATION_RIGHT:
		aPoint.x = (float)((double)aBounds.right + aHalfWidth);
		break;
	case SPAWN_LOCATION_BOTTOM:
		aPoint.y = (float)((double)aBounds.bottom + aHalfHeight);
		break;
	case SPAWN_LOCATION_TOP:
		aPoint.y = (float)((double)aBounds.top - aHalfHeight);
		break;
	default:
		break;
	}
	return aPoint;
}

void Actor::setPosition(float x, float y)
{
	mPosition = Vector2f(x, y);
	updateTranslation();
	if (mVisual)
		mVisual->setPosition(x, y);
}

bool Actor::isInView()
{
	return mStage->getViewBox().overlaps(*getTransformedShape(ACTOR_SHAPE_VISUAL));
}

void Actor::move(UpdateContext& theContext)
{
	float anElapsed = theContext.mElapsed;
	if (lengthSquared(mForce) == 0.0)
	{
		float aWanted = (float)(lengthSquared(mDesiredVelocity) - 0.0);
		if (aWanted <= 0.0f)
			aWanted = -aWanted;
		if (NO_DESIRE > aWanted)
		{
			// Slowed by the drag, never past a stop
			Vector2f aDrag((float)((double)mVelocity.x * -mDragRate), (float)((double)-mDragRate * mVelocity.y));
			aDrag = Vector2f((float)((double)aDrag.x * anElapsed), (float)((double)anElapsed * aDrag.y));
			if ((double)(float)lengthSquared(aDrag) > lengthSquared(mVelocity))
				aDrag = Vector2f((float)((double)mVelocity.x * MINUS_ONE), (float)((double)MINUS_ONE * mVelocity.y));
			mVelocity = moved(mVelocity, aDrag.x, aDrag.y);
		}
		else
		{
			// Steered towards the wanted velocity, the force no more than the acceleration
			Vector2f aChange = difference(mDesiredVelocity, mVelocity);
			aChange = Vector2f((float)((double)aChange.x * STEER_GAIN), (float)((double)STEER_GAIN * aChange.y));
			clampLength(aChange, mAccelRate);
			mForce = aChange;
		}
		addScaled(mVelocity, mForce, anElapsed);
	}
	else
	{
		if (lengthSquared(mMaxForce) > 0.0)
			clampToEllipse(mForce, mMaxForce);
		addScaled(mVelocity, mForce, anElapsed);
		if (0.0 < lengthSquared(mMaxVelocity))
			clampToEllipse(mVelocity, mMaxVelocity);
		if (signOf(mDesiredVelocity.x) != signOf(mForce.x))
			mTurned = true;
	}
	mForce = Vector2f(0.0f, 0.0f);
	if (lengthSquared(mVelocity) < STOPPED_SPEED_SQUARED)
		mVelocity = Vector2f(0.0f, 0.0f);
	addScaled(mPosition, mVelocity, anElapsed);
	mPosition.x = (float)((double)mPosition.x - (double)CURRENT * anElapsed);
	keepInWater();
	updateTranslation();
	if (mVisual)
		mVisual->setPosition(mPosition.x, mPosition.y);
}

Shape* Actor::getTransformedShape(int theIndex)
{
	ActorShape& aShape = mShapes[theIndex];
	if (!aShape.mShape)
		return nullptr;
	aShape.mTransformed = aShape.mShape->copyTo(aShape.mTransformed.get());
	aShape.mTransformed->transform(*aShape.mTransform);
	return aShape.mTransformed.get();
}

void Actor::setBounds(Stage* theStage, bool inAir)
{
	mUnderwater = !inAir;
	bool isWater = true;
	if (0.0f < (float)theStage->getLayerSet()->getWaterLevel())
		isWater = isUnderwater();
	mBounds = theStage->getArea(isWater);
	if (mBehavior && mBehavior->mDepth.mLow <= mBehavior->mDepth.mHigh)
	{
		// 0x50a40a (feedingfrenzy2): the water level, as a float
		float aWater = (float)theStage->getLayerSet()->getWaterLevel();
		mBounds.top = (float)((double)mBehavior->mDepth.mLow + aWater);
		mBounds.bottom = (float)((double)mBehavior->mDepth.mHigh + aWater);
	}
}

void Actor::addVisualToLayer()
{
	getLayerContainer(mStage)->addChild(mVisual.get());
}

Container* Actor::getLayerContainer(Stage* theStage)
{
	return theStage->getLayerContainer(mLayer);
}

void Actor::removeVisual()
{
	Container* aParent = mVisual->getParent();
	if (aParent != nullptr)
		aParent->removeChild(mVisual.get());
}

void Actor::changeLayer(int theLayer)
{
	removeVisual();
	Vector2f anOld = mStage->getLayerOffset(mLayer);
	Vector2f aNew = mStage->getLayerOffset(theLayer);
	Vector2f aShift((float)((double)aNew.x - anOld.x), (float)((double)aNew.y - anOld.y));
	mPosition = moved(mPosition, aShift.x, aShift.y);
	mLayer = theLayer;
	addVisualToLayer();
}

void Actor::updateTranslation()
{
	Vector2f aTranslation = mPosition;
	if (mStage != nullptr && mLayer != 0)
	{
		Vector2f anOffset = mStage->getLayerOffset(mLayer);
		aTranslation = Vector2f((float)((double)anOffset.x + aTranslation.x), (float)((double)anOffset.y + aTranslation.y));
	}
	mTransform.mTranslation = aTranslation;
}

void Actor::checkLayer(UpdateContext& theContext)
{
	// The original's array is indexed by the detail level unchecked past the runtime's range check (0x4f2bf6
	// (feedingfrenzy2)); the engine's levels are 0 to 2
	int aLayer = mDetailLayers[theContext.mDetailLevel];
	if (aLayer != mLayer)
	{
		changeLayer(aLayer);
		updateTranslation();
	}
}

Fish::Fish() : mEaterRank(0), mSize(0.0f), mMinSpeed(0.0f, 0.0f), mMaxSpeed(0.0f, 0.0f), mHoldTime(0.0f) {}

void Fish::create(Stage* theStage)
{
	mFishVisual = new FishVisual(theStage->getTheme(), this);
	mFishVisual->setupFish();
	mVisual = mFishVisual.get();
}

void Fish::addToStage(Stage* theStage, int theLocation)
{
	Actor::addToStage(theStage, theLocation);
	theStage->addFish(this);
	mTransform.mScale = 1.0f;
	mFishVisual->setPulseScale(1.0f);
	mFishVisual->setVisible(true);
	if (theLocation == SPAWN_LOCATION_LEFT)
		mFishVisual->setFacing(1.0f);
	else if (theLocation == SPAWN_LOCATION_RIGHT)
		mFishVisual->setFacing(MINUS_ONE);
	mHoldTime = 0.0f;
}

void Fish::remove()
{
	mStage->removeFish(this);
	// 0x475f50 (feedingfrenzy2): each of the visual's four effects still playing stopped, then the image's colour put
	// back (its slots 0x50 and 0x44): the effects are not yet translated, so only the colour
	mFishVisual->resetEffects();
	removeVisual();
	mStage->removeActor(this);
	mStage = nullptr;
}

void Fish::setFishVisualData(float theSize, FishVisualData* theData)
{
	mFishVisual->setFishData(theData);
	if (0.0f < mSize)
		mFishVisual->startSizePulse((float)((double)mSize / theSize), 1.0f, SIZE_PULSE_SECONDS);
	mSize = theSize;
	mFishVisualData = theData;
	mVisualData = theData;
	setupShapes();
}

bool Fish::isOfClass(const std::string& theClass, Actor* theViewer)
{
	if (theClass == "predator")
		return getEaterRank() > theViewer->getRank();
	if (theClass == "prey")
	{
		// The viewer is the AI fish looking about, so a fish with its eater part
		return getRank() < static_cast<Fish*>(theViewer)->getEaterRank();
	}
	return false;
}

bool Fish::isEdibleBy(Fish* theEater)
{
	FishState* aState = mStateMgr.getState();
	return aState != nullptr && aState->isEdible();
}

bool Fish::canBeEatenBy(Fish* theEater)
{
	FishState* aState = mStateMgr.getState();
	return aState != nullptr && aState->canBeEatenBy(theEater);
}

void Fish::beginBeingEaten(Fish* theEater, float theSeconds)
{
	changeState(new EatenState(this, theEater, theSeconds));
}

bool Fish::canEatNow()
{
	FishState* aState = mStateMgr.getState();
	if (aState == nullptr)
		return false;
	FishVisualState* aVisualState = mFishVisual->getState();
	if (aVisualState != nullptr && !aVisualState->canEat())
		return false;
	return aState->isInPlay() && 0.0f == mHoldTime;
}

void Fish::consider(Actor* thePrey)
{
	Shape* aMouth = getMouthShape();
	if (aMouth == nullptr || !canEatNow())
		return;
	if (!thePrey->isEdibleBy(this) || !aMouth->overlaps(*thePrey->getBodyShape()) || !thePrey->canBeEatenBy(this))
		return;
	// 0x47d610 (feedingfrenzy2): the visual's state's time to its chomp, none without a state
	FishVisualState* aVisualState = mFishVisual->getState();
	float aTimeToChomp = aVisualState != nullptr ? aVisualState->getTimeToChomp() : 0.0f;
	// 0x46d360 (feedingfrenzy2): the lesser
	float aSeconds = minOf(aTimeToChomp, LONGEST_DRAW_IN);
	eat(thePrey, aSeconds);
}

void Fish::eat(Actor* thePrey, float theSeconds)
{
	// 0x4779e0 (feedingfrenzy2): the prey starts being drawn in and is kept until the chomp
	thePrey->beginBeingEaten(this, theSeconds);
	mPrey.push_back(thePrey);
	FishState* aState = mStateMgr.getState();
	if (aState != nullptr)
		aState->eat(thePrey);
}

void Fish::swallow()
{
	for (const Ref<Actor>& aPrey : mPrey)
		aPrey->beSwallowed(this);
	mPrey.clear();
}

Vector2f Fish::getMouthPosition() const
{
	return transformed(mFishVisualData->getMouth(), mTransform);
}

void Fish::setFishBehavior(FishBehaviorInfo* theBehavior)
{
	mFishBehavior = theBehavior;
	if (!mFishBehavior)
		return;
	float aMaxX = drawIn(mFishBehavior->mMaxSpeedX);
	float aMaxY = drawIn(mFishBehavior->mMaxSpeedY);
	mMaxSpeed = Vector2f(aMaxX, aMaxY);
	mFishVisual->setSwimRate(SWIM_RATE_SCALE, SWIM_RATE_BASE);
	float aMinX = drawIn(mFishBehavior->mMinSpeedX);
	float aMinY = drawIn(mFishBehavior->mMinSpeedY);
	mMinSpeed = Vector2f(aMinX, aMinY);
	setAccelRate(mFishBehavior->mAccelRate);
	setDragRate(mFishBehavior->mDragRate);
}

AIFish::AIFish() : mHome(0.0f, 0.0f), mChaseFrequency(0.0f), mFlybyFrequency(0.0f) {}

void AIFish::evacuate()
{
	if (mStateMgr.getState() != nullptr)
		changeState(new EvacuateAI(this));
}

void AIFish::create(Stage* theStage)
{
	Fish::create(theStage);
	// The theme's "fishStunned" particles put in the visual's first effect (+0x218) and added to its front group: not
	// yet translated, as the effects are not
}

void AIFish::addToStage(Stage* theStage, int theLocation)
{
	Fish::addToStage(theStage, theLocation);
	mHome = getPosition();
	theStage->addAIFish(this);
	// 0x421980 (feedingfrenzy2)
	changeState(new SpawnAI(this));
}

void AIFish::remove()
{
	// 0x4774f0 (feedingfrenzy2): the eater's prey let go (and the meals puked, which an AI fish has none of)
	mPrey.clear();
	mStateMgr.changeState(nullptr);
	mStage->removeAIFish(this);
	Fish::remove();
}

void AIFish::update(UpdateContext& theContext)
{
	checkLayer(theContext);
	if (0.0f < mHoldTime)
	{
		float aLeft = (float)((double)mHoldTime - theContext.mElapsed);
		mHoldTime = aLeft;
		if (!(aLeft > 0.0f))
			mHoldTime = 0.0f;
	}
	FishState* aState = mStateMgr.getState();
	if (aState != nullptr && aState->isGone())
	{
		mStateMgr.changeState(nullptr);
		return;
	}
	// A fish with meals in it (+0x110) whose state lets it puke (the state's 0x40) pukes (AIPukingState): not yet
	// translated, as the eating is not, so no fish has meals
	mStateMgr.update(&theContext);
}

void AIFish::keepInWater()
{
	if (isUnderwater())
	{
		Shape* aSurface = mStage->getEdge(STAGE_EDGE_BELOW_WATER);
		if (static_cast<shape::Edge*>(aSurface)->mDistance != 0.0f)
			keepInside(aSurface);
		return;
	}
	// In the air: kept above the water where there is one (0x501583 (feedingfrenzy2); the original goes on with no
	// edge where there is none), then below the sky
	shape::Edge* aWater = static_cast<shape::Edge*>(mStage->getEdge(STAGE_EDGE_ABOVE_WATER));
	if (aWater->mDistance != 0.0f)
		keepInside(aWater);
	keepInside(mStage->getEdge(STAGE_EDGE_SKY));
}

void AIFish::keepInside(Shape* theEdge)
{
	ShapeContact aContact;
	getTransformedShape(ACTOR_SHAPE_BODY)->collide(*theEdge, aContact);
	if (0.0f > aContact.mSeparation)
	{
		addScaled(mPosition, aContact.mNormal, aContact.mSeparation);
		updateTranslation();
	}
}

void AIFish::pickState()
{
	float aDraw = (float)randomFloat(0.0f, 1.0f);
	if (mSchool != nullptr && mSchool->getFish().size() > 1)
	{
		joinSchool();
		return;
	}
	FishBehaviorInfo* aBehavior = getFishBehavior();
	if (aBehavior->mBaseBehavior == "chase")
	{
		// The stage's player (0x4916a0 (feedingfrenzy2)), alive and not able to eat the fish, chased when the draw is
		// below the chase frequency (ChaseAI, 0x47d020 (feedingfrenzy2)): not yet translated, as the players are not,
		// so no stage has one
	}
	if (aBehavior->mBaseBehavior == "flyBy" || mFlybyFrequency > aDraw)
	{
		changeState(new FlyByAI(this));
		return;
	}
	changeState(new WanderAI(this));
}

void AIFish::setFishBehavior(FishBehaviorInfo* theBehavior)
{
	Fish::setFishBehavior(theBehavior);
	if (!mFishBehavior)
		return;
	mChaseFrequency = mFishBehavior->getFloat("chaseFreq", DEFAULT_CHASE_FREQUENCY);
	mFlybyFrequency = mFishBehavior->getFloat("flybyFreq", DEFAULT_FLYBY_FREQUENCY);
}

void AIFish::eat(Actor* thePrey, float theSeconds)
{
	// The stage's theme's sound fishEaten at full volume, pitch 0.6 (0x4747a0 (feedingfrenzy2)): not yet
	// translated, as the sounds are not
	Fish::eat(thePrey, theSeconds);
}

bool AIFish::react(FishReaction* theReaction, Actor* theOther)
{
	const std::string& aReaction = theReaction->mReaction;
	if (aReaction == "run")
	{
		changeState(new FleeAI(this, theOther));
		return true;
	}
	if (aReaction == "avoid")
	{
		avoid(theOther);
		return true;
	}
	if (aReaction == "chase")
	{
		changeState(new ChaseAI(this, theOther));
		return true;
	}
	return false;
}

bool AIFish::canReact(FishReaction* theReaction, Actor* theOther)
{
	const std::string& aReaction = theReaction->mReaction;
	if (aReaction == "run" || aReaction == "avoid")
		return theOther->canEat(this);
	if (aReaction == "chase")
		return getEaterRank() > theOther->getRank();
	return true;
}

void AIFish::joinSchool()
{
	changeState(new SchoolingAI(this));
}

void AIFish::eatenByPlayer(PlayerFish* thePlayer)
{
	if (mSchool && mSchool->loseOne() && getFoodValue() > 0)
	{
		// The HUD's text schoolBonus (swarmBonus in the air) at the fish, and the theme's sound schoolBonus (0x4f0e20
		// to 0x4f0ed3 (feedingfrenzy2)): not yet translated, as the HUD and the sounds are not
		thePlayer->addScore(SCHOOL_BONUS_POINTS);
	}
	Actor::eatenByPlayer(thePlayer);
}

void AIFish::avoid(Actor* theOther)
{
	changeState(new AvoidAI(this, theOther));
}

float AIFish::distanceInSight(Actor* theOther)
{
	FishBehaviorInfo* aBehavior = mFishBehavior.get();
	if (!(0.0f < aBehavior->mSightDistance) || theOther->isUnderwater() != isUnderwater())
		return NOT_SEEN;
	Vector2f aMouth = getMouthPosition();
	Vector2f aWay = difference(theOther->getPosition(), aMouth);
	double aSightDistance = aBehavior->mSightDistance;
	if (!(aSightDistance * aSightDistance >= lengthSquared(aWay)))
		return NOT_SEEN;
	Vector2f aDirection = aWay;
	normalize(aDirection);
	float aCos = (float)(aDirection.x * mFishVisual->getFacing() + aDirection.y * ZERO);
	if (!(aBehavior->mSightCos < aCos))
		return NOT_SEEN;
	return length(aWay);
}

void AIFish::lookAround()
{
	const std::vector<Actor*>& aSeen = mStage->findSeenActors(this);
	for (Actor* anActor : aSeen)
	{
		FishReaction* aReaction = findReaction(anActor);
		if (aReaction != nullptr)
		{
			react(aReaction, anActor);
			return;
		}
	}
}

FishReaction* AIFish::findReaction(Actor* theOther)
{
	for (const Ref<FishReaction>& aReaction : mFishBehavior->mReactions)
	{
		if (!theOther->isOfClass(aReaction->mClass, this) || !canReact(aReaction.get(), theOther))
			continue;
		float aDraw = (float)randomFloat(0.0f, 1.0f);
		if (aDraw <= aReaction->mFrequency)
			return aReaction.get();
	}
	return nullptr;
}

void Fish::changeState(FishState* theState)
{
	Ref<FishState> aState = theState;
	if (mLeaving)
		return;
	mStateMgr.changeState(theState);
}
