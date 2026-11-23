#include "Spawn.h"

#include "Actor.h"
#include "BonusItem.h"
#include "Draws.h"
#include "FeedingFrenzyTwo.h"
#include "FishFactory.h"
#include "GameData.h"
#include "Random.h"
#include "Stage.h"
#include "VectorMath.h"

#include <algorithm>
#include <cstdio>

using namespace engine;

namespace
{

const float MINUS_ONE = -1.0f;      // 0x566850 (feedingfrenzy2)
const float SCHOOL_OFFSET = -0.5f;  // 0x566a0c (feedingfrenzy2): a school's box set back by half its size

// The registry's classes, in the order its constructor adds them, and the spawn each makes
struct RegisteredClass
{
	const char* mName;
	const char* mVisualName;
	SpawnKind mKind;
	ActorMaker mMaker;
};

// 0x41d4b0 (feedingfrenzy2)
Actor* makeAIFish()
{
	return new AIFish();
}

// 0x4a41a0 (feedingfrenzy2)
Actor* makeBonusTime()
{
	return new BonusTime();
}

// 0x5354e3 (feedingfrenzy2)
Actor* makeBonus1Up()
{
	return new Bonus1Up();
}

const RegisteredClass REGISTERED_CLASSES[] = {
	{"AIFish", "minnowBrown", SPAWN_KIND_FISH, makeAIFish},
	{"PufferFish", "puffer", SPAWN_KIND_FISH},
	{"Mine", "mine", SPAWN_KIND_NON_PLAYER},
	{"Balloon", "balloon", SPAWN_KIND_NON_PLAYER},
	{"Oyster", "oyster", SPAWN_KIND_NON_PLAYER},
	{"Pearl", "whitePearl", SPAWN_KIND_NON_PLAYER},
	{"BlackPearl", "blackPearl", SPAWN_KIND_NON_PLAYER},
	{"BonusStar", "bonusStar", SPAWN_KIND_NON_PLAYER},
	{"MermaidStar", "bonusStar", SPAWN_KIND_NON_PLAYER},
	{"LureFish", "lureFish", SPAWN_KIND_NON_PLAYER},
	{"BonusLight", "bonusLight", SPAWN_KIND_NON_PLAYER},
	{"BonusTime", "bonusTime", SPAWN_KIND_NON_PLAYER, makeBonusTime},
	{"BonusStun", "bonusStun", SPAWN_KIND_NON_PLAYER},
	{"Bonus2X", "bonus2X", SPAWN_KIND_NON_PLAYER},
	{"BonusSpeed", "bonusSpeed", SPAWN_KIND_NON_PLAYER},
	{"BonusShield", "bonusShield", SPAWN_KIND_NON_PLAYER},
	{"BonusFury", "bonusFury", SPAWN_KIND_NON_PLAYER},
	{"BonusShrinkShroom", "bonusShrinkShroom", SPAWN_KIND_NON_PLAYER},
	{"BonusLure", "bonusLure", SPAWN_KIND_NON_PLAYER},
	{"Bonus1Up", "bonus1Up", SPAWN_KIND_NON_PLAYER, makeBonus1Up},
	{"Mermaid", "mermaid", SPAWN_KIND_NON_PLAYER},
	{"JellyFish", "jellyfish", SPAWN_KIND_NON_PLAYER},
	{"PoisonFish", "minnowPoison", SPAWN_KIND_FISH},
	{"GoldenFish", "minnowBrown", SPAWN_KIND_FISH},
	{"Barracuda", "barracudaBlue", SPAWN_KIND_FISH},
	{"AlienFish", "alienFish", SPAWN_KIND_FISH},
	{"Butterfly", "butterfly", SPAWN_KIND_FISH},
	{"CuttleFish", "cuttleFish", SPAWN_KIND_FISH},
	{"GlowFish", "glowFish", SPAWN_KIND_FISH},
	{"Pelican", "evilPelican", SPAWN_KIND_FISH},
};

ActorClassRegistry* gActorClassRegistry = nullptr;  // 0x5ac610 (feedingfrenzy2)

} // namespace

Actor* ActorClassInfo::createActor()
{
	if (mMaker == nullptr)
	{
		getEngine()->log("ActorClassInfo: a %s not yet made (the class not yet translated)\n", mName.c_str());
		return nullptr;
	}
	return mMaker();
}

NonPlayerSpawn* ActorClassInfo::createSpawn(Stage* theStage)
{
	if (mKind == SPAWN_KIND_FISH)
		return new FishSpawn(this, theStage);
	return new NonPlayerSpawn(this, theStage);
}

ActorClassRegistry* ActorClassRegistry::get()
{
	if (gActorClassRegistry == nullptr)
		gActorClassRegistry = new ActorClassRegistry();
	return gActorClassRegistry;
}

ActorClassRegistry::ActorClassRegistry()
{
	// Each class added under its name (0x485240 (feedingfrenzy2))
	for (const RegisteredClass& aClass : REGISTERED_CLASSES)
		mClasses[aClass.mName] = new ActorClassInfo(aClass.mName, aClass.mVisualName, aClass.mKind, aClass.mMaker);
}

ActorClassInfo* ActorClassRegistry::find(const std::string& theName) const
{
	std::map<std::string, Ref<ActorClassInfo>>::const_iterator anIt = mClasses.find(theName);
	if (anIt == mClasses.end())
	{
		// The original's message box (the engine's slot 0xd8, MessageBoxA titled "Bad Name"); no stage's file names
		// such a class, so the port logs it
		getEngine()->log("There is no such actor class as %s.\n", theName.c_str());
		return nullptr;
	}
	return anIt->second.get();
}

void Spawner::read(Params& theAttributes)
{
	mName = theAttributes.getString("name", "");
}

NonPlayerSpawn::NonPlayerSpawn(ActorClassInfo* theClass, Stage* theStage)
	: Spawner(theStage), mSize(0.0f), mLayer(0), mLocation(SPAWN_LOCATION_SIDE), mPoolSize(0), mSupply(0),
	  mSupplyLeft(0), mDeaths(0), mStart(SPAWN_START_OFF), mScheduled(false), mNextSpawn(0), mScheduleTime(0.0f)
{
	mClass = theClass;
	for (int i = 0; i < 3; i++)
		mDetailLayers[i] = 0;
}

NonPlayerSpawn::~NonPlayerSpawn()
{
	removeAll();
}

SubDocHandler* NonPlayerSpawn::createDocHandler()
{
	return new NonPlayerSpawnDocHandler(this);
}

void NonPlayerSpawn::read(Params& theAttributes)
{
	Spawner::read(theAttributes);
	mSize = theAttributes.getFloat("size", 0.0f);
	mVisualName = theAttributes.getString("visual", mClass->getVisualName());
	mBehaviorName = theAttributes.getString("behavior", "");
	mPoolSize = theAttributes.getInt("poolSize", 0);
	mSupply = theAttributes.getInt("supply", 0);
	mLayer = theAttributes.getInt("layer", 0);
	for (int i = 0; i < 3; i++)
		mDetailLayers[i] = mLayer;
	std::string aDetailLayers = theAttributes.getString("detailLayers", "");
	sscanf(aDetailLayers.c_str(), " %d , %d , %d ", &mDetailLayers[0], &mDetailLayers[1], &mDetailLayers[2]);
	std::string aStart = toLower(theAttributes.getString("start", "on"));
	mStart = aStart == "on" ? SPAWN_START_ON : SPAWN_START_OFF;
	std::string aLocation = toLower(theAttributes.getString("location", "side"));
	if (aLocation == "leading")
		mLocation = SPAWN_LOCATION_LEADING;
	else if (aLocation == "trailing")
		mLocation = SPAWN_LOCATION_TRAILING;
	else if (aLocation == "left")
		mLocation = SPAWN_LOCATION_LEFT;
	else if (aLocation == "right")
		mLocation = SPAWN_LOCATION_RIGHT;
	else if (aLocation == "top")
		mLocation = SPAWN_LOCATION_TOP;
	else if (aLocation == "bottom")
		mLocation = SPAWN_LOCATION_BOTTOM;
	else if (aLocation == "side")
		mLocation = SPAWN_LOCATION_SIDE;
	else
	{
		mLocation = SPAWN_LOCATION_POINT;
		// The attribute's own value, not the lowered one (0x46d910 (feedingfrenzy2), whole numbers)
		Vector2<int> aPoint = theAttributes.getIntPair("location", Vector2<int>(0, 0));
		mLocationPoint = Vector2f((float)aPoint.x, (float)aPoint.y);
	}
}

void NonPlayerSpawn::finish()
{
	// 0x4861b0 and 0x485630 (feedingfrenzy2)
	mStage->addSpawner(this);
	mStage->addNonPlayerSpawn(this);
}

void NonPlayerSpawn::setup(int& theProgress)
{
	fillPool();
	mDeaths = 0;
	mSupplyLeft = mSupply;
	if (mPhases.empty())
		addPhase(SpawnPhase());
	findBehavior();
	createVisual();
	theProgress++;
}

Actor* NonPlayerSpawn::spawn()
{
	Actor* anActor = takeActor();
	if (anActor == nullptr)
		return nullptr;
	prepareActor(anActor);
	int aLocation = mLocation;
	Vector2f aPoint = mLocationPoint;
	Vector2f anEntry = anActor->getEntryPoint(mStage, aLocation, aPoint);
	anActor->setPosition(anEntry.x, anEntry.y);
	anActor->addToStage(mStage, aLocation);
	return anActor;
}

void NonPlayerSpawn::setStart(int theStart)
{
	if (theStart == mStart)
		return;
	mStart = theStart;
	if (theStart == SPAWN_START_LEAVE)
		evacuate();
}

void NonPlayerSpawn::evacuate()
{
	for (size_t i = 0; i < mLive.size(); i++)
		mLive[i]->evacuate();
}

void NonPlayerSpawn::onActorDied()
{
	if (mSupplyLeft > 0)
		mSupplyLeft--;
	mDeaths++;
}

void NonPlayerSpawn::setupActor(Actor* theActor)
{
	theActor->create(mStage);
	theActor->mDied.connect(this, &NonPlayerSpawn::onActorDied);
}

void NonPlayerSpawn::prepareActor(Actor* theActor)
{
	// The original's array is indexed by the engine's detail level past the runtime's range check (0x4f2f34
	// (feedingfrenzy2)); the levels are 0 to 2
	mLayer = mDetailLayers[getEngine()->getDetailLevel()];
	int* aLayers = theActor->getDetailLayers();
	for (int i = 0; i < 3; i++)
		aLayers[i] = mDetailLayers[i];
	theActor->setLayer(mLayer);
	theActor->setBehavior(mBehavior.get());
	theActor->setVisualData(mVisual.get());
	theActor->setBounds(mStage, false);
}

Actor* NonPlayerSpawn::takeActor()
{
	if (!mPool.empty())
	{
		Ref<Actor> anActor = mPool.back();
		mLive.push_back(anActor);
		mPool.pop_back();
		return anActor.get();
	}
	Ref<Actor> anActor = mClass->createActor();
	if (!anActor)
		return nullptr;
	setupActor(anActor.get());
	mLive.push_back(anActor);
	return anActor.get();
}

void NonPlayerSpawn::update(UpdateContext* theContext, bool canSpawn)
{
	removeDead();
	if (mStart == SPAWN_START_ON && canSpawn)
	{
		const SpawnPhase& aPhase = getPhase(getApp()->mGameData->getSpawnPhase());
		int aLiveCount = getLiveCount();
		int aRoom = aPhase.mMax;
		if (mSupply > 0 && mSupplyLeft < aRoom)
			aRoom = mSupplyLeft;
		if (aLiveCount >= aRoom)
		{
			mScheduleTime = 0.0f;
			mScheduled = false;
			mNextSpawn = 0;
			return;
		}
		if (mScheduled && mNextSpawn < mSpawnTimes.size())
		{
			mScheduleTime = mScheduleTime + theContext->mElapsed;
			if (mSpawnTimes[mNextSpawn] <= mScheduleTime)
			{
				mNextSpawn++;
				spawn();
			}
			return;
		}
		float aDelay = (float)randomFloat(aPhase.mDelay.mLow, aPhase.mDelay.mHigh);
		mSpawnTimes.clear();
		mSpawnTimes.push_back(aDelay);
		mScheduleTime = 0.0f;
		mNextSpawn = 0;
		mScheduled = true;
	}
	else if (mStart == SPAWN_START_LEAVE && mLive.empty())
		setStart(SPAWN_START_OFF);
}

const SpawnPhase& NonPlayerSpawn::getPhase(int thePhaseIndex) const
{
	int aLast = (int)mPhases.size() - 1;
	if (aLast < thePhaseIndex)
		thePhaseIndex = aLast;
	return mPhases[thePhaseIndex];
}

void NonPlayerSpawn::findBehavior()
{
	FishFactory* aFactory = FishFactory::get();
	if (!mBehaviorName.empty())
	{
		// 0x517f4f (feedingfrenzy2)
		mBehavior = aFactory->findBehavior(mBehaviorName);
		if (mBehavior == nullptr)
			getEngine()->log("There is no such behavior as %s.\n", mBehaviorName.c_str());
	}
	if (mBehavior != nullptr)
	{
		if (!mTweaks.getValues().empty())
		{
			// 0x5321e5 (feedingfrenzy2): a copy with the tweaks laid over
			BehaviorInfo* aCopy = new BehaviorInfo();
			aCopy->merge(*mBehavior);
			aCopy->merge(mTweaks);
			aCopy->read();
			mBehavior = aCopy;
		}
	}
	else
	{
		mBehavior = new BehaviorInfo();
		mBehavior->load(mTweaks);
	}
}

void NonPlayerSpawn::createVisual()
{
	// 0x408a50 (feedingfrenzy2): the visual of the name from the fish factory, at the size times the stage's size unit
	FishFactory* aFactory = FishFactory::get();
	float aSize = (float)((double)mSize * (double)mStage->getSizeUnit());
	VisualInfo* anInfo = aFactory->findVisual(mVisualName);
	VisualData* aData = nullptr;
	if (anInfo)
	{
		aData = new VisualData();
		aData->setup(anInfo, aFactory->mAnimBase, aSize);
	}
	mVisual = aData;
}

void NonPlayerSpawn::fillPool()
{
	// 0x4143b0 (feedingfrenzy2): the pool resized, each place given a new actor of the class
	mPool.resize(mPoolSize < 0 ? 0 : mPoolSize);
	for (size_t i = 0; i < mPool.size(); i++)
	{
		mPool[i] = mClass->createActor();
		if (!mPool[i])
		{
			// port: a class not yet translated makes no actor; its pool stays empty, so its spawns make none in
			// place of taking an empty place from the pool
			mPool.clear();
			break;
		}
		setupActor(mPool[i].get());
	}
}

void NonPlayerSpawn::removeDead()
{
	// The living put first and the dead last, as the original's partition does it (from both ends, a dead one from
	// the front swapped with a living one from the back)
	size_t aFirst = 0;
	size_t aLast = mLive.size();
	for (;;)
	{
		while (aFirst != aLast && !mLive[aFirst]->isDead())
			aFirst++;
		if (aFirst == aLast)
			break;
		bool isSwapped = false;
		while (aFirst != --aLast)
		{
			if (!mLive[aLast]->isDead())
			{
				std::swap(mLive[aFirst], mLive[aLast]);
				aFirst++;
				isSwapped = true;
				break;
			}
		}
		if (!isSwapped)
			break;
	}
	if (aFirst == mLive.size())
		return;
	for (size_t i = aFirst; i < mLive.size(); i++)
		mLive[i]->remove();
	for (size_t i = aFirst; i < mLive.size(); i++)
		onActorRemoved(mLive[i].get());
	mPool.insert(mPool.end(), mLive.begin() + aFirst, mLive.end());
	mLive.erase(mLive.begin() + aFirst, mLive.end());
}

void NonPlayerSpawn::removeAll()
{
	for (size_t i = 0; i < mLive.size(); i++)
		mLive[i]->remove();
	for (size_t i = 0; i < mLive.size(); i++)
		onActorRemoved(mLive[i].get());
	mPool.insert(mPool.end(), mLive.begin(), mLive.end());
	mLive.clear();
}

FishSpawn::FishSpawn(ActorClassInfo* theClass, Stage* theStage)
	: NonPlayerSpawn(theClass, theStage), mRank(0), mFoodValue(0), mShowMenu(false), mShowStageComplete(false),
	  mSpawnCount(0)
{
}

void FishSpawn::read(Params& theAttributes)
{
	NonPlayerSpawn::read(theAttributes);
	mRank = theAttributes.getInt("rank", -1);
	mFoodValue = theAttributes.getInt("foodValue", 0);
	mShowMenu = theAttributes.getBool("showMenu", true);
	mShowStageComplete = theAttributes.getBool("showStageComplete", true);
}

void FishSpawn::finish()
{
	NonPlayerSpawn::finish();
	// 0x405ae0 (feedingfrenzy2)
	mStage->addFishSpawn(this);
}

void FishSpawn::setup(int& theProgress)
{
	NonPlayerSpawn::setup(theProgress);
	// +0x538, 0: read by nothing translated yet
}

Actor* FishSpawn::spawn()
{
	if (mFishBehavior->mSchoolSize.mHigh < 2)
		return NonPlayerSpawn::spawn();
	Ref<SchoolOfFish> aSchool = new SchoolOfFish();
	aSchool->setBehavior(mFishBehavior.get());
	// The school's size drawn (0x41ce50 (feedingfrenzy2)), the leader and the rest, these no more than the phase's
	// most (or the supply left beyond those in the stage, when that is fewer) less the leader
	int aFollowers = drawWhole(mFishBehavior->mSchoolSize) - 1;
	int aRoom = getPhase(getApp()->mGameData->getSpawnPhase()).mMax;
	int aLiveCount = getLiveCount();
	if (mSupply > 0 && mSupplyLeft - aLiveCount < aRoom)
		aRoom = mSupplyLeft - aLiveCount;
	if (aRoom - 1 < aFollowers)
		aFollowers = aRoom - 1;
	mSpawnCount++;
	Actor* anActor = takeActor();
	prepareActor(anActor);
	int aLocation = mLocation;
	Vector2f aPoint = mLocationPoint;
	Vector2f anEntry = anActor->getEntryPoint(mStage, aLocation, aPoint);
	anActor->setPosition(anEntry.x, anEntry.y);
	anActor->addToStage(mStage, aLocation);
	aSchool->addFish(static_cast<AIFish*>(anActor));
	// The rest drawn in a box twice the separation distance on each side, behind the leader from the side it came
	// in (its spread across and its offset). From a location other than these the original leaves both as the
	// stack held them; the port takes none
	Vector2f aSpread(0.0f, 0.0f);
	Vector2f anOffset(0.0f, 0.0f);
	switch (aLocation)
	{
	case SPAWN_LOCATION_LEFT:
		aSpread = Vector2f(MINUS_ONE, 1.0f);
		anOffset = Vector2f(0.0f, SCHOOL_OFFSET);
		break;
	case SPAWN_LOCATION_RIGHT:
		aSpread = Vector2f(1.0f, 1.0f);
		anOffset = Vector2f(0.0f, SCHOOL_OFFSET);
		break;
	case SPAWN_LOCATION_LEADING:
		aSpread = Vector2f(1.0f, 1.0f);
		anOffset = Vector2f(SCHOOL_OFFSET, 0.0f);
		break;
	case SPAWN_LOCATION_TRAILING:
		aSpread = Vector2f(1.0f, MINUS_ONE);
		anOffset = Vector2f(SCHOOL_OFFSET, 0.0f);
		break;
	default:
		break;
	}
	float aBoxSize = (float)((double)mFishBehavior->mSchoolSeparationDistance + mFishBehavior->mSchoolSeparationDistance);
	aSpread = scaled(aSpread, aBoxSize);
	aBoxSize = (float)((double)mFishBehavior->mSchoolSeparationDistance + mFishBehavior->mSchoolSeparationDistance);
	anOffset = scaled(anOffset, aBoxSize);
	while (aFollowers > 0)
	{
		aFollowers--;
		float anX = (float)(randomFloat(0.0f, 1.0f) * aSpread.x + ((double)anOffset.x + anEntry.x));
		float aY = (float)(randomFloat(0.0f, 1.0f) * aSpread.y + ((double)anOffset.y + anEntry.y));
		anActor = takeActor();
		prepareActor(anActor);
		anActor->setPosition(anX, aY);
		aSchool->addFish(static_cast<AIFish*>(anActor));
		anActor->addToStage(mStage, mLocation);
	}
	return anActor;
}

void FishSpawn::prepareActor(Actor* theActor)
{
	NonPlayerSpawn::prepareActor(theActor);
	Fish* aFish = static_cast<Fish*>(theActor);
	aFish->setFoodValue(mFoodValue);
	aFish->setFishVisualData(mSize, mFishVisual.get());
	aFish->setEaterRank(mRank);
	aFish->setRank(mRank);
	aFish->setFishBehavior(mFishBehavior.get());
	aFish->setBounds(mStage, mFishBehavior->mInAir);
}

void FishSpawn::findBehavior()
{
	FishFactory* aFactory = FishFactory::get();
	if (!mBehaviorName.empty())
	{
		// 0x488030 (feedingfrenzy2)
		mFishBehavior = aFactory->findFishBehavior(mBehaviorName);
		if (mFishBehavior == nullptr)
			getEngine()->log("There is no such fishBehavior as %s.\n", mBehaviorName.c_str());
	}
	if (mFishBehavior != nullptr)
	{
		if (!mTweaks.getValues().empty())
		{
			// 0x535eac (feedingfrenzy2): a copy with the tweaks laid over, and the reactions after the copy's own
			FishBehaviorInfo* aCopy = new FishBehaviorInfo();
			aCopy->merge(*mFishBehavior);
			aCopy->merge(mTweaks);
			aCopy->read();
			aCopy->mReactions.insert(aCopy->mReactions.end(), mFishBehavior->mReactions.begin(),
									 mFishBehavior->mReactions.end());
			mFishBehavior = aCopy;
		}
	}
	else
	{
		mFishBehavior = new FishBehaviorInfo();
		mFishBehavior->load(mTweaks);
	}
	// The spawn's behaviour is the fish one too (0x4102cf (feedingfrenzy2)), given to each actor by prepareActor
	mBehavior = mFishBehavior.get();
}

void FishSpawn::createVisual()
{
	// The fish visual of the name from the fish factory, at the size times the stage's size unit, in the stage's
	// water colour, or none ((0, 0, 0, 0)) when the behaviour is in the air; never set up as in the air
	FishFactory* aFactory = FishFactory::get();
	float aUnit = mStage->getSizeUnit();
	Color aTint = mFishBehavior->mInAir ? Color(0.0f, 0.0f, 0.0f, 0.0f) : mStage->getWaterColor();
	float aSize = (float)((double)mSize * (double)aUnit);
	// 0x40e250 (feedingfrenzy2)
	FishVisualInfo* anInfo = aFactory->findFishVisual(mVisualName);
	FishVisualData* aData = nullptr;
	if (anInfo)
	{
		aData = new FishVisualData();
		aData->setup(anInfo, aFactory->mAnimBase, aSize, aTint, false);
	}
	mFishVisual = aData;
	mVisual = mFishVisual.get();
}

void NonPlayerSpawnDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "phase")
	{
		SpawnPhase aPhase;
		aPhase.mMax = theAttributes.getInt("max", aPhase.mMax);
		aPhase.mDelay = theAttributes.getRange("delay", aPhase.mDelay);
		mSpawn->addPhase(aPhase);
	}
	else if (theName == "tweaks")
		mSpawn->getTweaks().merge(theAttributes);
}
