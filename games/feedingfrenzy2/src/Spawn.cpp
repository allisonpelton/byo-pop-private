#include "Spawn.h"

#include "FeedingFrenzyTwo.h"
#include "FishFactory.h"
#include "GameData.h"
#include "Random.h"
#include "Stage.h"

#include <cstdio>

using namespace engine;

namespace
{

// The registry's classes, in the order its constructor adds them, and the spawn each makes
struct RegisteredClass
{
	const char* mName;
	SpawnKind mKind;
};

const RegisteredClass REGISTERED_CLASSES[] = {
	{"AIFish", SPAWN_KIND_FISH},
	{"PufferFish", SPAWN_KIND_FISH},
	{"Mine", SPAWN_KIND_NON_PLAYER},
	{"Balloon", SPAWN_KIND_NON_PLAYER},
	{"Oyster", SPAWN_KIND_NON_PLAYER},
	{"Pearl", SPAWN_KIND_NON_PLAYER},
	{"BlackPearl", SPAWN_KIND_NON_PLAYER},
	{"BonusStar", SPAWN_KIND_NON_PLAYER},
	{"MermaidStar", SPAWN_KIND_NON_PLAYER},
	{"LureFish", SPAWN_KIND_NON_PLAYER},
	{"BonusLight", SPAWN_KIND_NON_PLAYER},
	{"BonusTime", SPAWN_KIND_NON_PLAYER},
	{"BonusStun", SPAWN_KIND_NON_PLAYER},
	{"Bonus2X", SPAWN_KIND_NON_PLAYER},
	{"BonusSpeed", SPAWN_KIND_NON_PLAYER},
	{"BonusShield", SPAWN_KIND_NON_PLAYER},
	{"BonusFury", SPAWN_KIND_NON_PLAYER},
	{"BonusShrinkShroom", SPAWN_KIND_NON_PLAYER},
	{"BonusLure", SPAWN_KIND_NON_PLAYER},
	{"Bonus1Up", SPAWN_KIND_NON_PLAYER},
	{"Mermaid", SPAWN_KIND_NON_PLAYER},
	{"JellyFish", SPAWN_KIND_NON_PLAYER},
	{"PoisonFish", SPAWN_KIND_FISH},
	{"GoldenFish", SPAWN_KIND_FISH},
	{"Barracuda", SPAWN_KIND_FISH},
	{"AlienFish", SPAWN_KIND_FISH},
	{"Butterfly", SPAWN_KIND_FISH},
	{"CuttleFish", SPAWN_KIND_FISH},
	{"GlowFish", SPAWN_KIND_FISH},
	{"Pelican", SPAWN_KIND_FISH},
};

ActorClassRegistry* gActorClassRegistry = nullptr;  // 0x5ac610 (feedingfrenzy2)

} // namespace

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
		mClasses[aClass.mName] = new ActorClassInfo(aClass.mName, aClass.mKind);
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
	  mSupplyLeft(0), mStart(SPAWN_START_OFF), mScheduled(false), mNextSpawn(0), mScheduleTime(0.0f)
{
	mClass = theClass;
	for (int i = 0; i < 3; i++)
		mDetailLayers[i] = 0;
}

SubDocHandler* NonPlayerSpawn::createDocHandler()
{
	return new NonPlayerSpawnDocHandler(this);
}

void NonPlayerSpawn::read(Params& theAttributes)
{
	Spawner::read(theAttributes);
	mSize = theAttributes.getFloat("size", 0.0f);
	mVisualName = theAttributes.getString("visual", mClass->getName());
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
	// +0xb4, 0: read by nothing translated yet
	mSupplyLeft = mSupply;
	if (mPhases.empty())
		addPhase(SpawnPhase());
	findBehavior();
	createVisual();
	theProgress++;
}

void NonPlayerSpawn::spawn()
{
	// An actor taken from the pool or made (0x475b30 (feedingfrenzy2)), set up for the spawn (its slot 0x58,
	// 0x4224a0 (feedingfrenzy2)), placed at the location (the actor's slots 0x24 and 0x28) and added to the stage (its
	// slot 0x18): not yet translated, as the actors are not
	getEngine()->log("NonPlayerSpawn: a %s not yet spawned (the actors not yet translated)\n", mVisualName.c_str());
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
	// A spawn whose start is 2 (set by nothing translated yet) calls its slot 0x34 with 0 once it has no actor left
	// (0x495540 (feedingfrenzy2))
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
	// poolSize actors made by the class (its slot 0x04), each set up for the spawn (the spawn's slot 0x54, 0x41da40
	// (feedingfrenzy2)) and kept in the pool (+0x50c): not yet translated, as the actors are not
	if (mPoolSize > 0)
		getEngine()->log("NonPlayerSpawn: a pool of %d %s not yet made (the actors not yet translated)\n", mPoolSize,
						 mVisualName.c_str());
}

void NonPlayerSpawn::removeDead()
{
	// The live actors (+0x4fc) that died (0x4c26e0 (feedingfrenzy2)) moved back into the pool: not yet translated,
	// as the actors are not
}

int NonPlayerSpawn::getLiveCount() const
{
	// The live actors' count (+0x4fc): none until the actors are translated
	return 0;
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

void FishSpawn::spawn()
{
	if (mFishBehavior->mSchoolSize.mHigh < 2)
	{
		NonPlayerSpawn::spawn();
		return;
	}
	// A school (SchoolOfFish, 0xa8 bytes, its constructor 0x4101b0 (feedingfrenzy2)) of the behaviour's school size
	// drawn (0x41ce50 (feedingfrenzy2)) less the room the phase and the supply leave, its leader brought in as one
	// fish and the rest about it, each drawn in a square twice the separation distance: not yet translated, as the
	// actors are not
	mSpawnCount++;
	getEngine()->log("FishSpawn: a school of %s not yet spawned (the actors not yet translated)\n",
					 mVisualName.c_str());
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
