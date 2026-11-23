#include "FishSpawner.h"
#include "App.h"
#include "AIFish.h"
#include "DocHandler.h"
#include "FishFactory.h"
#include "FishFood.h"
#include "FishSchool.h"
#include "Layout.h"
#include "PlayerFish.h"
#include "Random.h"
#include "StageMgr.h"
#include "Stream.h"
#include <cstdio>

using namespace engine;

namespace feedingfrenzy
{

namespace
{
// The value the fish file's attributes take when the file does not say
const float DEFAULT_MAX_SPEED_LOW = 40.0f;
const float DEFAULT_MAX_SPEED_HIGH = 80.0f;
const float DEFAULT_SPEED_UP = 400.0f;
const float DEFAULT_SLOW_DOWN = 70.0f;
const float DEFAULT_SCALE = 1.0f;
const float DEFAULT_LEVEL = 0.5f;
const float DEFAULT_POINTS = 666.0f;
const float DEFAULT_VISION_RANGE = 80.0f;
const float DEFAULT_VISION_ANGLE = 30.0f;
const float DEFAULT_SIGHT_CHECK = 0.1f;
const float DEFAULT_SCHOOL_DISTANCE = 20.0f;
const float DEFAULT_SCHOOL_NEIGHBOUR_DISTANCE = 80.0f;
const float DEFAULT_SCHOOL_SEPARATION = 1.4f;
const float DEFAULT_SCHOOL_ALIGNMENT = 0.95f;
const float DEFAULT_SCHOOL_COHESION_DIVISOR = 5.0f;
const float DEFAULT_SCHOOL_TARGET = 0.5f;
const float DEFAULT_SCHOOL_CHASE = 1.0f;
const float DEFAULT_SCHOOL_RUN = 2.5f;

// What a spawner holds before its file is read
const float INITIAL_SCHOOL_DISTANCE = 50.0f;
const float INITIAL_SCHOOL_NEIGHBOUR_DISTANCE = 80.0f;

// The chance of the first fish of a school being put on the left edge
const float EDGE_CHANCE = 0.5f;
// How far apart the fish of a school are put, by the school's distance
const float SCHOOL_SPREAD_FACTOR = 2.5f;

// The reactions a <react> element names (AIFish.h)
int reactionByName(const std::string& theName)
{
	if (theName == "run")
		return REACTION_RUN;
	if (theName == "avoid")
		return REACTION_AVOID;
	if (theName == "chase")
		return REACTION_CHASE;
	if (theName == "special")
		return REACTION_SPECIAL;
	return REACTION_NONE;
}

// The four numbers of a rectangle, as the fish file writes them ("left,top,right,bottom")
Rect rectFromString(const std::string& theText)
{
	float aLeft = 0.0f, aTop = 0.0f, aRight = 0.0f, aBottom = 0.0f;
	sscanf(theText.c_str(), "%f,%f,%f,%f", &aLeft, &aTop, &aRight, &aBottom);
	return Rect(aLeft, aTop, aRight, aBottom);
}

// The handler of a fish file (0x41ae90; vtable 0x4a4e3c): the <fish> element sets what a kind of fish is, the elements
// inside it its sound, its menu picture, how it reacts to other kinds and its thought bubble
class FishFileHandler : public BaseDocHandler
{
public:
	FishFileHandler(const std::string& theName, DocHandlerStack* theParent, FishSpawner* theSpawner) :
		BaseDocHandler(theName, theParent), mSpawner(theSpawner)
	{
	}

	// 0x41b3c0
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override
	{
		if (theName == "fish")
		{
			readFish(theAttributes);
		}
		else if (theName == "sound")
		{
			std::string aName = getStringAttribute(theAttributes, "name", "NO SOUND");
			float aPitch = getFloatAttribute(theAttributes, "pitch", 1.0f);
			if (getStringAttribute(theAttributes, "class", "none") == "eaten")
				mSpawner->setEatenSound(aName, aPitch);
		}
		else if (theName == "menuImage")
		{
			mSpawner->setMenuImage(getStringAttribute(theAttributes, "onImage", "NO IMAGE"));
		}
		else if (theName == "react")
		{
			// 0x41af90
			std::string aClass = getStringAttribute(theAttributes, "class", "NO CLASS");
			std::string aReaction = getStringAttribute(theAttributes, "reaction", "none");
			FishReaction* aRecord = new FishReaction;
			aRecord->mKind = reactionByName(aReaction);
			aRecord->mChance = getFloatAttribute(theAttributes, "reactFreq", 1.0f);
			mSpawner->addReaction(aClass, aRecord);
		}
		else if (theName == "hypnoFx")
		{
			ThoughtBubbleInfo anInfo;
			anInfo.mImage = getStringAttribute(theAttributes, "eyeImg", "");
			anInfo.mOffset = getPointAttribute(theAttributes, "eyeLoc", Point(0.0f, 0.0f));
			anInfo.mScale = getFloatAttribute(theAttributes, "eyeScale", 1.0f);
			mSpawner->setThoughtBubble(anInfo);
		}
	}

private:
	void readFish(const XmlAttributes& theAttributes)
	{
		mSpawner->setClassName(getStringAttribute(theAttributes, "class", "No Class"));

		FloatPair aMaxSpeedX = getFloatRangeAttribute(theAttributes, "maxSpeedX",
		                                              FloatPair{DEFAULT_MAX_SPEED_LOW, DEFAULT_MAX_SPEED_HIGH});
		FloatPair aMaxSpeedY = getFloatRangeAttribute(theAttributes, "maxSpeedY",
		                                              FloatPair{DEFAULT_MAX_SPEED_LOW, DEFAULT_MAX_SPEED_HIGH});
		FloatPair aMinSpeedX = getFloatRangeAttribute(theAttributes, "minSpeedX", FloatPair{0.0f, 0.0f});
		FloatPair aMinSpeedY = getFloatRangeAttribute(theAttributes, "minSpeedY", FloatPair{0.0f, 0.0f});
		float aSpeedUp = getFloatAttribute(theAttributes, "accelRate", DEFAULT_SPEED_UP);
		float aSlowDown = getFloatAttribute(theAttributes, "dragRate", DEFAULT_SLOW_DOWN);
		mSpawner->setSpeeds(Point(aMaxSpeedX.mFirst, aMaxSpeedX.mSecond), Point(aMaxSpeedY.mFirst, aMaxSpeedY.mSecond),
		                    Point(aMinSpeedX.mFirst, aMinSpeedX.mSecond), Point(aMinSpeedY.mFirst, aMinSpeedY.mSecond),
		                    aSpeedUp, aSlowDown);

		float aScale = getFloatAttribute(theAttributes, "scale", DEFAULT_SCALE);
		float aLevel = getFloatAttribute(theAttributes, "size", DEFAULT_LEVEL);
		float aPoints = getFloatAttribute(theAttributes, "foodValue", DEFAULT_POINTS);
		float aVisionRange = getFloatAttribute(theAttributes, "sightDist", DEFAULT_VISION_RANGE);
		float aVisionAngle = getFloatAttribute(theAttributes, "sightAngle", DEFAULT_VISION_ANGLE);
		std::string anAnimPath = getStringAttribute(theAttributes, "animPath", "BAD PATH");

		FloatPair aWanderDecision = getFloatRangeAttribute(theAttributes, "wanderMoveChangeFreq", FloatPair{2.0f, 6.0f});
		FloatPair aWanderState = getFloatRangeAttribute(theAttributes, "wanderStateChangeFreq", FloatPair{5.0f, 10.0f});
		mSpawner->setIdleRanges(Point(aWanderDecision.mFirst, aWanderDecision.mSecond),
		                        Point(aWanderState.mFirst, aWanderState.mSecond));
		FloatPair aRunDecision = getFloatRangeAttribute(theAttributes, "runMoveChangeFreq", FloatPair{0.5f, 3.0f});
		FloatPair aRunState = getFloatRangeAttribute(theAttributes, "runStateChangeFreq", FloatPair{1.0f, 5.0f});
		mSpawner->setFleeRanges(Point(aRunDecision.mFirst, aRunDecision.mSecond), Point(aRunState.mFirst, aRunState.mSecond));
		FloatPair aChaseDecision = getFloatRangeAttribute(theAttributes, "chaseMoveChangeFreq", FloatPair{0.05f, 0.5f});
		FloatPair aChaseState = getFloatRangeAttribute(theAttributes, "chaseStateChangeFreq", FloatPair{5.0f, 15.0f});
		mSpawner->setChaseRanges(Point(aChaseDecision.mFirst, aChaseDecision.mSecond),
		                         Point(aChaseState.mFirst, aChaseState.mSecond));
		mSpawner->setSightCheckRange(getFloatAttribute(theAttributes, "sightCheckFreq", DEFAULT_SIGHT_CHECK));

		mSpawner->setSizeAndLevel(aLevel, aScale);
		mSpawner->setAnimPath(anAnimPath);
		mSpawner->setPoints(aPoints);
		mSpawner->setVision(aVisionRange, aVisionAngle);

		mSpawner->setBodyRect(rectFromString(getStringAttribute(theAttributes, "hotSpot", "-24,-24,24,24")));
		CollisionRect aMouthRect;
		aMouthRect.set(rectFromString(getStringAttribute(theAttributes, "tailSpot", "-14,-14,0,14")));
		mSpawner->setMouthRect(aMouthRect);
		std::string aCollision = getStringAttribute(theAttributes, "collision", "");
		if (!aCollision.empty())
		{
			CollisionRect aHitRect;
			aHitRect.set(rectFromString(aCollision));
			mSpawner->setHitRect(aHitRect);
		}

		mSpawner->setSpawnType(getStringAttribute(theAttributes, "spawnType", "offscreenLR"));
		std::string aLayer = getStringAttribute(theAttributes, "layer", "fg");
		if (aLayer == "mg")
			mSpawner->setLayer(LAYER_MIDDLE);
		else if (aLayer == "bg")
			mSpawner->setLayer(LAYER_BACKGROUND);
		else if (aLayer == "fg")
			mSpawner->setLayer(LAYER_FOREGROUND);

		// The colour shift: hue in degrees, then the saturation and the value
		float aHue = 0.0f, aSaturation = 0.0f, aValue = 0.0f;
		sscanf(getStringAttribute(theAttributes, "hsvOffset", "0,0,0").c_str(), "%f,%f,%f", &aHue, &aSaturation, &aValue);
		if (aHue != 0.0f || aSaturation != 0.0f || aValue != 0.0f)
		{
			HsvShiftFilter* aShift = new HsvShiftFilter();
			aShift->setShift(aHue, aSaturation, aValue);
			mSpawner->setColorShift(aShift);
		}

		FloatPair aSchoolSize = getFloatRangeAttribute(theAttributes, "schoolSize", FloatPair{1.0f, 1.0f});
		float aSchoolDistance = getFloatAttribute(theAttributes, "schoolDist", DEFAULT_SCHOOL_DISTANCE);
		float aSchoolNeighbourDistance =
		    getFloatAttribute(theAttributes, "schoolNeighborDist", DEFAULT_SCHOOL_NEIGHBOUR_DISTANCE);
		float aSeparation = getFloatAttribute(theAttributes, "schoolSepMult", DEFAULT_SCHOOL_SEPARATION);
		float anAlignment = getFloatAttribute(theAttributes, "schoolAlignMult", DEFAULT_SCHOOL_ALIGNMENT);
		float aCohesionDivisor = getFloatAttribute(theAttributes, "schoolCohesionDiv", DEFAULT_SCHOOL_COHESION_DIVISOR);
		float aTarget = getFloatAttribute(theAttributes, "schoolTargetMult", DEFAULT_SCHOOL_TARGET);
		float aChase = getFloatAttribute(theAttributes, "schoolChaseMult", DEFAULT_SCHOOL_CHASE);
		float aRun = getFloatAttribute(theAttributes, "schoolRunMult", DEFAULT_SCHOOL_RUN);
		mSpawner->setSchool(Point(aSchoolSize.mFirst, aSchoolSize.mSecond), aSchoolDistance, aSchoolNeighbourDistance,
		                    aSeparation, anAlignment, aCohesionDivisor, aTarget, aChase, aRun);
	}

	FishSpawner* mSpawner;
};
}

// 0x4219d0
FishSpawner::FishSpawner() :
    mInPlayCount(0), mUneatenCount(0), mEatenCount(0), mFishEatenCount(0), mBonusEatenCount(0), mRemaining(0),
    mPoolSize(0), mRespawnSeconds(-1.0f), mSpeedUp(0.0f), mSlowDown(0.0f), mSize(0.0f), mLevel(0.0f), mPoints(0.0f),
    mVisionRange(0.0f), mVisionAngle(0.0f), mSightCheckRange(0.0f), mLayer(LAYER_FOREGROUND),
    mSpawnType(SPAWN_OFFSCREEN_LR), mDisplayMenuImage(false), mEatenSoundPitch(1.0f), mMode(SPAWN_NORMAL),
    mCurrentRule(-1), mHasHitRect(false), mSchoolSize(1.0f, 1.0f), mSchoolDistance(INITIAL_SCHOOL_DISTANCE),
    mSchoolNeighbourDistance(INITIAL_SCHOOL_NEIGHBOUR_DISTANCE), mSchoolSeparation(0.0f), mSchoolAlignment(0.0f),
    mSchoolCohesionDivisor(0.0f), mSchoolTarget(0.0f), mSchoolChase(0.0f), mSchoolRun(0.0f), mColorShift(nullptr),
    mShownInHud(false)
{
	mThought.mScale = 1.0f;
}

// 0x421690
FishSpawner::~FishSpawner()
{
	for (std::map<std::string, FishReaction*>::iterator anIt = mReactions.begin(); anIt != mReactions.end(); ++anIt)
		delete anIt->second;
	delete mColorShift;
}

// 0x41e0c0
bool FishSpawner::load(const std::string& theFile, int thePoolSize)
{
	mPoolSize = thePoolSize;
	Ref<DocHandlerStack> aStack = new DocHandlerStack();
	Ref<FishFileHandler> aHandler = new FishFileHandler("fish", aStack, this);
	aStack->addHandler(aHandler);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(*aStack, aStream);
	return true;
}

// 0x420890
void FishSpawner::createFish()
{
	for (int anIndex = 0; anIndex < mPoolSize; anIndex++)
	{
		FishBase* aFish = createFishOfClass(mClassName);
		aFish->setName(mClassName);
		mPool.push_back(aFish);

		aFish->setSize(mSize);
		aFish->setLevel(mLevel);
		float aMinSpeedY = randomFloat(mMinSpeedY);
		float aMinSpeedX = randomFloat(mMinSpeedX);
		float aMaxSpeedY = randomFloat(mMaxSpeedY);
		float aMaxSpeedX = randomFloat(mMaxSpeedX);
		aFish->setSpeeds(aMaxSpeedX, aMaxSpeedY, aMinSpeedX, aMinSpeedY, mSpeedUp, mSlowDown);
		aFish->setPoints(mPoints);
		aFish->setVision(mVisionRange, mVisionAngle);
		aFish->setColorShift(mColorShift);
		aFish->loadAnimationsOf(mAnimPath);
		if (mHasHitRect)
			aFish->setHitRect(mHitRect);
		aFish->setBodyRect(mBodyRect.left, mBodyRect.top, mBodyRect.right, mBodyRect.bottom);
		aFish->setMouthRect(mMouthRect);
		aFish->setThoughtBubble(mThought);
		aFish->loadEffects();
		aFish->setEatenSound(mEatenSoundName, mEatenSoundPitch);
		aFish->setIdleRanges(mWanderDecisionRange.x, mWanderDecisionRange.y, mWanderStateRange.x, mWanderStateRange.y);
		aFish->setChaseRanges(mChaseDecisionRange.x, mChaseDecisionRange.y, mChaseStateRange.x, mChaseStateRange.y);
		aFish->setFleeRanges(mRunDecisionRange.x, mRunDecisionRange.y, mRunStateRange.x, mRunStateRange.y);
		aFish->setSightCheckRange(mSightCheckRange);
		static_cast<AIFish*>(aFish)->setSpawner(this);
	}
}

void FishSpawner::setSpeeds(const Point& theMaxSpeedX, const Point& theMaxSpeedY, const Point& theMinSpeedX,
                            const Point& theMinSpeedY, float theSpeedUp, float theSlowDown)
{
	mMaxSpeedX = theMaxSpeedX;
	mMaxSpeedY = theMaxSpeedY;
	mMinSpeedX = theMinSpeedX;
	mMinSpeedY = theMinSpeedY;
	mSpeedUp = theSpeedUp;
	mSlowDown = theSlowDown;
}

// 0x41d870
void FishSpawner::setSizeAndLevel(float theLevel, float theSize)
{
	mSize = theSize;
	mLevel = theLevel;
}

// 0x41d890
void FishSpawner::setVision(float theRange, float theAngle)
{
	mVisionRange = theRange;
	mVisionAngle = theAngle;
}

void FishSpawner::setIdleRanges(const Point& theDecision, const Point& theState)
{
	mWanderDecisionRange = theDecision;
	mWanderStateRange = theState;
}

void FishSpawner::setChaseRanges(const Point& theDecision, const Point& theState)
{
	mChaseDecisionRange = theDecision;
	mChaseStateRange = theState;
}

void FishSpawner::setFleeRanges(const Point& theDecision, const Point& theState)
{
	mRunDecisionRange = theDecision;
	mRunStateRange = theState;
}

// 0x41dc40
void FishSpawner::setHitRect(const CollisionRect& theRect)
{
	mHasHitRect = true;
	mHitRect = theRect;
}

// 0x41dc80
void FishSpawner::setSchool(const Point& theSize, float theDistance, float theNeighbourDistance, float theSeparation,
                            float theAlignment, float theCohesionDivisor, float theTarget, float theChase, float theRun)
{
	mSchoolSize = theSize;
	mSchoolDistance = theDistance;
	mSchoolNeighbourDistance = theNeighbourDistance;
	mSchoolSeparation = theSeparation;
	mSchoolAlignment = theAlignment;
	mSchoolCohesionDivisor = theCohesionDivisor;
	mSchoolTarget = theTarget;
	mSchoolChase = theChase;
	mSchoolRun = theRun;
}

// 0x41f940
void FishSpawner::setEatenSound(const std::string& theName, float thePitch)
{
	mEatenSoundName = theName;
	mEatenSoundPitch = thePitch;
}

// 0x41e760
void FishSpawner::setSpawnType(const std::string& theName)
{
	if (theName == "offscreenLR")
		mSpawnType = SPAWN_OFFSCREEN_LR;
	else if (theName == "offstageLR" || theName == "offStageLR")
		mSpawnType = SPAWN_OFFSTAGE_LR;
	else if (theName == "offstageBottom")
		mSpawnType = SPAWN_OFFSTAGE_BOTTOM;
	else if (theName == "offstageTop")
		mSpawnType = SPAWN_OFFSTAGE_TOP;
}

// 0x421940
void FishSpawner::addReaction(const std::string& theTypeName, FishReaction* theReaction)
{
	FishReaction*& aSlot = mReactions[theTypeName];
	delete aSlot;
	aSlot = theReaction;
}

// 0x41f640
const FishReaction* FishSpawner::findReaction(const std::string& theTypeName) const
{
	std::map<std::string, FishReaction*>::const_iterator anIt = mReactions.find(theTypeName);
	if (anIt == mReactions.end())
		return nullptr;
	return anIt->second;
}

// 0x420800
void FishSpawner::addRule(float theMinPlayerLevel, int theMaxCount, float theRespawnLow, float theRespawnHigh)
{
	SpawnRule aRule;
	aRule.mMinPlayerLevel = theMinPlayerLevel;
	aRule.mMaxCount = theMaxCount;
	aRule.mRespawnLow = theRespawnLow;
	aRule.mRespawnHigh = theRespawnHigh;
	mRules.push_back(aRule);
}

// 0x420bf0
void FishSpawner::addPlacement(float theX, float theY, bool theMarked)
{
	SpawnPlacement aPlacement;
	aPlacement.mX = theX;
	aPlacement.mY = theY;
	aPlacement.mMarked = theMarked;
	mPlacements.push_back(aPlacement);
}

// 0x41de30
void FishSpawner::pickRule()
{
	mCurrentRule = -1;
	float aPlayerLevel = FishFood::instance()->getStageMgr()->getPlayer()->getLevel();
	for (size_t anIndex = 0; anIndex < mRules.size(); anIndex++)
	{
		if (mRules[anIndex].mMinPlayerLevel <= aPlayerLevel)
			mCurrentRule = (int)anIndex;
	}
}

// 0x41de90
void FishSpawner::recount()
{
	mInPlayCount = 0;
	mUneatenCount = 0;
	mEatenCount = 0;
	mFishEatenCount = 0;
	mBonusEatenCount = 0;
	for (size_t anIndex = 0; anIndex < mPool.size(); anIndex++)
	{
		FishBase* aFish = mPool[anIndex];
		if (aFish->isInPlay())
			mInPlayCount++;
		if (aFish->getEatCount() == 0)
			mUneatenCount++;
		else
			mEatenCount += aFish->getEatCount();
		int aKind = aFish->getKind();
		if (aKind == FISH_KIND_FISH)
			mFishEatenCount += aFish->getEatCount();
		else if (aKind == FISH_KIND_BONUS)
			mBonusEatenCount += aFish->getEatCount();
	}
}

// 0x41df30
FishBase* FishSpawner::findFreeFish()
{
	for (size_t anIndex = 0; anIndex < mPool.size(); anIndex++)
	{
		FishBase* aFish = mPool[anIndex];
		switch (mMode)
		{
		case SPAWN_NORMAL:
		case SPAWN_UNCOUNTED:
		case SPAWN_STOPPED:
			if (!aFish->isInPlay())
				return aFish;
			break;
		case SPAWN_ONLY_UNEATEN:
			if (!aFish->isInPlay() && aFish->getEatCount() < 1)
				return aFish;
			break;
		case SPAWN_PAUSED:
			return nullptr;
		case SPAWN_COUNTDOWN:
			if (!aFish->isInPlay())
			{
				mRemaining--;
				return aFish;
			}
			break;
		}
	}
	return nullptr;
}

// 0x41dfd0
bool FishSpawner::isDone()
{
	switch (mMode)
	{
	case SPAWN_NORMAL:
		return mCurrentRule >= 0 && mRules[mCurrentRule].mMaxCount == 0 && mInPlayCount == 0;
	case SPAWN_ONLY_UNEATEN:
		recount();
		return mUneatenCount == 0;
	case SPAWN_UNCOUNTED:
		return true;
	case SPAWN_STOPPED:
	case SPAWN_PAUSED:
		recount();
		return mInPlayCount == 0;
	case SPAWN_COUNTDOWN:
		recount();
		return mInPlayCount == 0 && mRemaining < 1;
	}
	return false;
}

// 0x41e060
void FishSpawner::setMode(int theMode)
{
	if (theMode == SPAWN_COUNTDOWN)
	{
		pickRule();
		if (mMode == SPAWN_UNCOUNTED)
		{
			mRemaining = 0;
			mInPlayCount = 0;
		}
		else
		{
			mRemaining = (mCurrentRule >= 0 ? mRules[mCurrentRule].mMaxCount : 0) - mInPlayCount;
			if (mRemaining < 0)
				mRemaining = 0;
		}
	}
	if (mMode != SPAWN_UNCOUNTED)
		mMode = theMode;
}

// 0x41d8c0
void FishSpawner::placeOffscreen(FishBase* theFish)
{
	float aCoin = randomFloat(Point(0.0f, 1.0f));
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	theFish->layout();
	float aWidth = theFish->getBounds()->right - theFish->getBounds()->left;
	float aHeight = theFish->getBounds()->bottom - theFish->getBounds()->top;
	float aScreenWidth = (float)aStage->getScreenWidth();
	float aScreenHeight = (float)aStage->getScreenHeight();
	float aWorldWidth = (float)aStage->getWorldWidth();
	float aWorldHeight = (float)aStage->getWorldHeight();
	Point aScreenOrigin = aStage->screenToWorld(Point(0.0f, 0.0f));
	float aSize = theFish->getSize();

	float aX;
	float anY;
	switch (mSpawnType)
	{
	case SPAWN_OFFSTAGE_LR:
		aX = aCoin <= EDGE_CHANCE ? -aWidth : aWorldWidth;
		anY = randomFloat(Point(0.0f, aWorldHeight - aHeight));
		theFish->setScale(aX < 0.0f ? -aSize : aSize, aSize);
		break;
	case SPAWN_OFFSTAGE_BOTTOM:
		aX = randomFloat(Point(0.0f, aWorldWidth));
		anY = aWorldHeight;
		break;
	case SPAWN_OFFSTAGE_TOP:
		aX = randomFloat(Point(0.0f, aWorldWidth));
		anY = -aHeight;
		break;
	default:
		aX = aCoin <= EDGE_CHANCE ? aScreenOrigin.x - aWidth : aScreenOrigin.x + aScreenWidth;
		anY = randomFloat(Point(0.0f, aScreenHeight));
		theFish->setScale(aX < 0.0f ? -aSize : aSize, aSize);
		break;
	}
	theFish->setPosition(aX, anY);
}

// 0x41e1e0
FishBase* FishSpawner::spawnAt(StageMgr* theStage, bool theMarked, float theX, float theY, float theVelocityX,
                               float theVelocityY)
{
	FishBase* aFish = findFreeFish();
	if (aFish == nullptr)
		return nullptr;
	aFish->putInPlay();
	aFish->setPosition(theX, theY);
	aFish->setSwallowed(theMarked);
	theStage->addFish(aFish, mLayer);
	aFish->onEnterStage();
	aFish->setVelocity(theVelocityX, theVelocityY);
	recount();
	return aFish;
}

// 0x41e2e0
void FishSpawner::spawnOffscreen(StageMgr* theStage, bool theAtPosition, float theX, float theY, bool theMarked)
{
	FishBase* aFish = findFreeFish();
	if (aFish == nullptr)
		return;
	aFish->putInPlay();
	if (theAtPosition)
	{
		aFish->setPosition(theX, theY);
		aFish->setSwallowed(true);
		aFish->setMarked(theMarked);
	}
	else
	{
		placeOffscreen(aFish);
	}
	theStage->addFish(aFish, mLayer);
	aFish->onEnterStage();
	recount();
}

// 0x41e370
void FishSpawner::spawnSchool(StageMgr* theStage, int theCount)
{
	if ((int)mPool.size() - mInPlayCount < theCount)
		return;
	int aLimit;
	switch (mMode)
	{
	case SPAWN_ONLY_UNEATEN:
		aLimit = mUneatenCount;
		break;
	case SPAWN_PAUSED:
		return;
	case SPAWN_COUNTDOWN:
		aLimit = mRemaining;
		break;
	default:
		aLimit = -1;
		break;
	}
	if (aLimit != -1)
	{
		if (aLimit < 1)
			return;
		if (aLimit < theCount)
			theCount = aLimit;
	}

	FishSchool* aSchool = new FishSchool();
	aSchool->setSeparationDistance(mSchoolDistance);
	aSchool->setNeighbourDistance(mSchoolNeighbourDistance);
	aSchool->setMultipliers(mSchoolSeparation, mSchoolAlignment, mSchoolCohesionDivisor, mSchoolTarget, mSchoolChase,
	                        mSchoolRun);
	FishBase* aFirst = findFreeFish();
	if (aFirst == nullptr)
	{
		delete aSchool;
		return;
	}

	aFirst->putInPlay();
	placeOffscreen(aFirst);
	theStage->addFish(aFirst, mLayer);
	aFirst->onEnterStage();
	aSchool->addFish(aFirst);
	recount();
	Point aFirstPosition = *aFirst->getPosition();
	for (int aLeft = theCount - 1; aLeft > 0; aLeft--)
	{
		FishBase* aFish = findFreeFish();
		if (aFish != nullptr)
		{
			aFish->putInPlay();
			Point anOffset;
			anOffset.x = randomFloat(Point(0.0f, 1.0f));
			anOffset.y = randomFloat(Point(0.0f, 1.0f));
			anOffset.scale(mSchoolDistance * SCHOOL_SPREAD_FACTOR);
			anOffset.add(aFirstPosition);
			aFish->setPosition(anOffset.x, anOffset.y);
			theStage->addFish(aFish, mLayer);
			aFish->onEnterStage();
			aSchool->addFish(aFish);
		}
	}
	theStage->addSchool(aSchool);
	recount();
}

// 0x41e5f0
void FishSpawner::spawnInitial()
{
	StageMgr* aStage = FishFood::instance()->getStageMgr();
	for (size_t anIndex = 0; anIndex < mPlacements.size(); anIndex++)
	{
		const SpawnPlacement& aPlacement = mPlacements[anIndex];
		FishBase* aFish = findFreeFish();
		if (aFish != nullptr)
		{
			aFish->putInPlay();
			aFish->setPosition(aPlacement.mX, aPlacement.mY);
			aFish->setSwallowed(true);
			aFish->setMarked(aPlacement.mMarked);
			aStage->addFish(aFish, mLayer);
			aFish->onEnterStage();
			recount();
		}
	}

	pickRule();
	if (mCurrentRule >= 0)
	{
		const SpawnRule& aRule = mRules[mCurrentRule];
		if (mSchoolSize.x < (float)(aRule.mMaxCount - mInPlayCount) && mRespawnSeconds < 0.0f)
			mRespawnSeconds = randomFloat(Point(aRule.mRespawnLow, aRule.mRespawnHigh));
	}
}

// 0x41e8f0
void FishSpawner::update(UpdateInfo& theInfo, StageMgr* theStage)
{
	if ((mMode == SPAWN_COUNTDOWN && mRemaining <= 0) || mMode == SPAWN_STOPPED || mMode == SPAWN_PAUSED)
		return;

	pickRule();
	if (mCurrentRule < 0)
		return;

	const SpawnRule& aRule = mRules[mCurrentRule];
	if (0.0f <= mRespawnSeconds)
	{
		mRespawnSeconds -= theInfo.mSeconds;
		if (mRespawnSeconds < 0.0f)
		{
			int aCount = (int)randomFloat(mSchoolSize);
			if (aCount < 2)
				spawnOffscreen(theStage, false, 0.0f, 0.0f, false);
			else
				spawnSchool(theStage, aCount);
		}
	}
	if (mInPlayCount < aRule.mMaxCount && mRespawnSeconds < 0.0f)
		mRespawnSeconds = randomFloat(Point(aRule.mRespawnLow, aRule.mRespawnHigh));
}

} // namespace feedingfrenzy
