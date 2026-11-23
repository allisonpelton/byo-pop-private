#ifndef __FEEDINGFRENZY2_SPAWN_H__
#define __FEEDINGFRENZY2_SPAWN_H__

// A stage's spawns. Each <spawn> of a stage's file names a class of actor (its class attribute, looked up in the
// actor classes' registry) and says how the stage brings that actor in: how many at once in each phase and how long
// between two, where they come in, how they look and behave. The class makes the spawn: a FishSpawn for a fish, a
// NonPlayerSpawn for anything else the computer moves. A spawn registers itself with the stage once its element is
// read; the stage sets every spawn up as it loads and updates them each step of its run.
//
// Translated so far: the registry, the spawns' reading, registration, setup and timing. The actors themselves (their
// pool, their making and adding to the stage, their visuals) are not yet, and each step that needs them says so
// where it would run.

#include "ActorInfo.h"
#include "Engine.h"
#include "Geometry.h"
#include "MasterDocHandler.h"
#include "Object.h"
#include "Params.h"
#include "VisualData.h"
#include "sigslot.h"

#include <map>
#include <string>
#include <vector>

class Actor;
class NonPlayerSpawn;
class Stage;

// What makes an actor of a class (the template's actor class, ActorClassInfoImpl's 0x04)
typedef Actor* (*ActorMaker)();

// The kind of spawn a class of actor makes (the second argument of ActorClassInfoImpl's template)
enum SpawnKind
{
	SPAWN_KIND_FISH = 0,       // FishSpawn
	SPAWN_KIND_NON_PLAYER = 1  // NonPlayerSpawn
};

// A class of actor in the registry (ActorClassInfo, 0x44 bytes; its constructor 0x416890 (feedingfrenzy2), and
// ActorClassInfoImpl<Actor, Spawn> for each class, its vtable's 0x04 the actor's making and 0x08 the spawn's)
class ActorClassInfo : public engine::Object
{
public:
	ActorClassInfo(const char* theName, const char* theVisualName, SpawnKind theKind, ActorMaker theMaker)
	    : mName(theName), mVisualName(theVisualName), mKind(theKind), mMaker(theMaker)
	{
	}

	// 0x04: a new actor of the class (an AIFish's, 0x41d4b0 (feedingfrenzy2)); null, logged, for a class not yet
	// translated
	Actor* createActor();

	// 0x08 (0x4a08d0 and 0x535445 for a fish, 0x535675 and 0x4a41e0 for the rest (feedingfrenzy2)): a spawn of the
	// class for the stage
	NonPlayerSpawn* createSpawn(Stage* theStage);

	const std::string& getName() const { return mName; }
	const std::string& getVisualName() const { return mVisualName; }

private:
	std::string mName;        // +0x0c
	std::string mVisualName;  // +0x28: the visual a spawn of the class takes when its element names none
	SpawnKind mKind;    // the template's spawn class
	ActorMaker mMaker;  // the template's actor class, null for one not yet translated
};

// Every class of actor a stage can spawn, by name (made on first use, 0x405440 (feedingfrenzy2), at 0x5ac610; its
// constructor 0x485a60 (feedingfrenzy2))
class ActorClassRegistry
{
public:
	// 0x405440 (feedingfrenzy2)
	static ActorClassRegistry* get();

	// 0x485010 (feedingfrenzy2): the class of the name (in its case), or null, with a message box, when there is none
	ActorClassInfo* find(const std::string& theName) const;

private:
	ActorClassRegistry();

	std::map<std::string, engine::Ref<ActorClassInfo>> mClasses;  // +0x0c
};

// A phase of a spawn (a <phase>, 0x0c bytes; its constructor 0x409c90 (feedingfrenzy2)): the stage's spawn phase
// (the game data's) picks one, the last for any past the end
struct SpawnPhase
{
	SpawnPhase() : mMax(1) {}
	SpawnPhase(int theMax, const engine::FloatRange& theDelay) : mMax(theMax), mDelay(theDelay) {}

	int mMax;                    // +0x00: the most of the actor in the stage at once
	engine::FloatRange mDelay;   // +0x04: the seconds before the next comes in, drawn in the range
};

// Where a spawn's actors come in (the location attribute, NonPlayerSpawn's +0x9c)
enum SpawnLocation
{
	SPAWN_LOCATION_SIDE = 0,      // "side"
	SPAWN_LOCATION_LEFT = 1,      // "left"
	SPAWN_LOCATION_RIGHT = 2,     // "right"
	SPAWN_LOCATION_LEADING = 3,   // "leading"
	SPAWN_LOCATION_TRAILING = 4,  // "trailing"
	SPAWN_LOCATION_BOTTOM = 5,    // "bottom"
	SPAWN_LOCATION_TOP = 6,       // "top"
	SPAWN_LOCATION_POINT = 7      // anything else: the attribute read as a point (+0xa0)
};

// Whether a spawn brings its actors in (NonPlayerSpawn's +0xb8)
enum SpawnStart
{
	SPAWN_START_OFF = 0,  // the start attribute anything but "on"
	SPAWN_START_ON = 1,   // "on", any case
	SPAWN_START_LEAVE = 2 // the stage's end (0x4a2390 (feedingfrenzy2)): its actors sent away; once it has no actor
	                      // left it turns itself off
};

// What every spawn is (Spawner, 0x2c bytes and its virtual base, vtables 0x558278 and 0x5582a8 (feedingfrenzy2); its
// constructor 0x415d30 (feedingfrenzy2))
class Spawner : public engine::Object
{
public:
	explicit Spawner(Stage* theStage) : mStage(theStage) {}

	// 0x08 of its Interface table (0x47c6b0 (feedingfrenzy2)): the name attribute
	virtual void read(engine::Params& theAttributes);
	// 0x10: set up as the stage loads, the load's progress counted
	virtual void setup(int& theProgress) = 0;
	// 0x14: the steps setup takes in the load's progress
	virtual int getLoadSteps() = 0;
	// 0x20: each step of the stage's run; the actors come in only while the stage lets them
	virtual void update(engine::UpdateContext* theContext, bool canSpawn) = 0;

	// 0x24 (0x489580 (feedingfrenzy2))
	const std::string& getName() const { return mName; }

protected:
	std::string mName;  // +0x0c
	Stage* mStage;      // +0x28
};

// A spawn of an actor the computer moves (NonPlayerSpawn, 0x528 bytes with its virtual base, vtable 0x558024
// (feedingfrenzy2); its constructor 0x41d2f0 (feedingfrenzy2))
class NonPlayerSpawn : public Spawner, public sigslot::HasSlots
{
public:
	NonPlayerSpawn(ActorClassInfo* theClass, Stage* theStage);
	// 0x41d420 (feedingfrenzy2): the live actors taken out first (slot 0x60)
	~NonPlayerSpawn() override;

	// 0x04 (0x403390 (feedingfrenzy2)): the handler that reads the spawn's element
	engine::SubDocHandler* createDocHandler();
	// 0x08 (0x469960 (feedingfrenzy2))
	void read(engine::Params& theAttributes) override;
	// 0x0c (0x4856d0 (feedingfrenzy2)): once read, put in the stage's spawns, its spawns of the kind and its spawns
	// by name
	virtual void finish();
	// 0x10 (0x40e3a0 (feedingfrenzy2)): the pool filled, the supply full, a phase given if the file gave none, and
	// the behaviour and the visual found
	void setup(int& theProgress) override;
	// 0x14 (0x4033b0 (feedingfrenzy2))
	int getLoadSteps() override { return 1; }
	// 0x1c (0x474d50 (feedingfrenzy2)): one actor taken from the pool (or made), set up, put at its entry point and
	// added to the stage
	virtual Actor* spawn();
	// 0x20 (0x43a180 (feedingfrenzy2)): the dead taken out; while the stage lets them come and the phase has room,
	// the next one's time drawn, and once it has passed, the actor brought in
	void update(engine::UpdateContext* theContext, bool canSpawn) override;
	// 0x2c (0x4a9080 (feedingfrenzy2)): an actor of the spawn died: one less of the supply left, one more dead
	void onActorDied();
	// 0x30 (0x406dd0 (feedingfrenzy2)): a <phase>, in the file's order
	void addPhase(const SpawnPhase& thePhase) { mPhases.push_back(thePhase); }
	// 0x34 (0x495540 (feedingfrenzy2)): the start changed; to SPAWN_START_LEAVE, the actors sent away
	void setStart(int theStart);
	// 0x44 (0x43a4b0 (feedingfrenzy2)): the phase of the number, the last for any past the end
	const SpawnPhase& getPhase(int thePhaseIndex) const;
	// 0x48 (0x40e400 (feedingfrenzy2)): the behaviour of the behavior attribute's name, with the tweaks laid over
	virtual void findBehavior();
	// 0x4c (0x408a00 (feedingfrenzy2)): the visual of the visual attribute's name, at the spawn's size
	virtual void createVisual();
	// 0x50 (0x47d580 (feedingfrenzy2)): the pool made, poolSize actors ready to come in
	void fillPool();
	// 0x54 (0x41da40 (feedingfrenzy2)): a new actor made ready for the stage and its death heard
	void setupActor(Actor* theActor);
	// 0x58 (0x4224a0 (feedingfrenzy2)): the actor given the layers, the behaviour, the visual and its bounds
	virtual void prepareActor(Actor* theActor);
	// 0x5c (0x439fa0 (feedingfrenzy2)): the actors that died taken out of the live ones, back into the pool
	void removeDead();
	// 0x60 (0x477520 (feedingfrenzy2)): every live actor taken out of the stage, each then passed to slot 0x6c, and
	// all put back into the pool
	void removeAll();
	// 0x64 (0x48fcc0 (feedingfrenzy2)): each live actor leaving (its slot 0xe8)
	void evacuate();
	// 0x68 (0x43a550 (feedingfrenzy2)): the actors of the spawn in the stage
	int getLiveCount() const { return (int)mLive.size(); }
	// 0x6c: a dead actor taken out (a fish spawn's, 0x41d6d0 (feedingfrenzy2))
	virtual void onActorRemoved(Actor* theActor) {}
	// 0x475b30 (feedingfrenzy2): the pool's last actor, or a new one made ready, moved to the live ones
	Actor* takeActor();

	engine::Params& getTweaks() { return mTweaks; }
	const std::string& getVisualName() const { return mVisualName; }
	ActorClassInfo* getClass() const { return mClass.get(); }
	int getDeaths() const { return mDeaths; }
	int getSupply() const { return mSupply; }
	BehaviorInfo* getBehavior() const { return mBehavior.get(); }

protected:
	engine::Ref<ActorClassInfo> mClass;     // +0x3c
	float mSize;                            // +0x40: the actor's size, in the stage's size units
	std::string mVisualName;                // +0x44: the class's name when the element names none
	std::string mBehaviorName;              // +0x60
	std::vector<SpawnPhase> mPhases;        // +0x80
	int mLayer;                             // +0x8c: the layer the actors swim in
	int mDetailLayers[3];                   // +0x90: the layer at each detail level, the layer when not given
	int mLocation;                          // +0x9c: a SpawnLocation
	engine::Vector2f mLocationPoint;        // +0xa0: SPAWN_LOCATION_POINT's
	int mPoolSize;                          // +0xa8: the actors made ready as the stage loads
	int mSupply;                            // +0xac: how many ever come in, none for no end
	int mSupplyLeft;                        // +0xb0: from the supply at setup
	int mDeaths;                            // +0xb4: the actors of the spawn that died
	int mStart;                             // +0xb8: a SpawnStart
	bool mScheduled;                        // +0xc0: the next actors' times drawn
	std::vector<float> mSpawnTimes;         // +0xc4: the times drawn, from the scheduling
	unsigned int mNextSpawn;                // +0xd4: the first of them not yet passed
	float mScheduleTime;                    // +0xd8: the seconds since the scheduling
	engine::Ref<VisualData> mVisual;        // +0x4d4: the actors' frames, at the spawn's size
	engine::Ref<BehaviorInfo> mBehavior;    // +0x4d8: a fish spawn's is its fish behaviour (+0x534)
	engine::Params mTweaks;                 // +0x4dc: the <tweaks> element's attributes, laid over the behaviour's
	std::vector<engine::Ref<Actor>> mLive;  // +0x4fc: in the stage, in the order they came in
	std::vector<engine::Ref<Actor>> mPool;  // +0x50c: ready to come in, the last first
};

// A spawn of a fish (FishSpawn, 0x548 bytes with its virtual base, vtable 0x55785c (feedingfrenzy2); its
// constructor 0x40c420 (feedingfrenzy2))
class FishSpawn : public NonPlayerSpawn
{
public:
	FishSpawn(ActorClassInfo* theClass, Stage* theStage);
	// 0x40eb60 (feedingfrenzy2): the live fish taken out first (slot 0x60), each passed to the fish spawn's slot 0x6c
	~FishSpawn() override { removeAll(); }

	// 0x08 (0x4802e0 (feedingfrenzy2)): the spawn's, and the fish's rank, food value and where it shows
	void read(engine::Params& theAttributes) override;
	// 0x0c (0x406040 (feedingfrenzy2)): the spawn's, and put in the stage's fish spawns
	void finish() override;
	// 0x10 (0x4061b0 (feedingfrenzy2))
	void setup(int& theProgress) override;
	// 0x1c (0x474dd0 (feedingfrenzy2)): a school of fish where the behaviour schools, else the spawn's one
	Actor* spawn() override;
	// 0x48 (0x410270 (feedingfrenzy2)): the fish behaviour of the name, or a new one from the tweaks alone
	void findBehavior() override;
	// 0x4c (0x40e310 (feedingfrenzy2)): the fish visual, in the stage's water colour unless the behaviour is in the
	// air
	void createVisual() override;
	// 0x58 (0x4225a0 (feedingfrenzy2)): the spawn's, then the fish's food value, visual, ranks, behaviour and bounds
	void prepareActor(Actor* theActor) override;
	// 0x6c (0x41d6d0 (feedingfrenzy2)): a fish of a school taken out of it, one less school when it is empty: not
	// yet translated, as the schools are not
	void onActorRemoved(Actor* theActor) override {}

	int getRank() const { return mRank; }
	int getFoodValue() const { return mFoodValue; }
	bool showsOnStageComplete() const { return mShowStageComplete; }
	FishVisualData* getFishVisual() const { return mFishVisual.get(); }

private:
	int mRank;                                   // +0x520: -1 when not given
	int mFoodValue;                              // +0x524
	bool mShowMenu;                              // +0x528: shown on the menus' fish lists
	bool mShowStageComplete;                     // +0x529: shown on the stage's end
	int mSpawnCount;                             // +0x52c: the spawns made, each school one
	engine::Ref<FishVisualData> mFishVisual;     // +0x530: the same as the spawn's visual
	engine::Ref<FishBehaviorInfo> mFishBehavior; // +0x534
	// +0x538: set to 0 by setup, read by nothing translated yet: left out
};

// A spawn's element's handler (NonPlayerSpawn::DocHandler, 0x5c bytes, vtables 0x5580b8 and 0x5580c4
// (feedingfrenzy2); its constructor 0x403340 (feedingfrenzy2))
class NonPlayerSpawnDocHandler : public engine::SubDocHandler
{
public:
	explicit NonPlayerSpawnDocHandler(NonPlayerSpawn* theSpawn) : mSpawn(theSpawn) {}

	// 0x04 of its own table (0x49cfe0 (feedingfrenzy2)): the spawn reads the element
	void begin(engine::Params& theAttributes) override { mSpawn->read(theAttributes); }
	// 0x00 (0x47ae40 (feedingfrenzy2)): a <phase>, or a <tweaks> whose attributes are kept
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x417d10 (feedingfrenzy2)): at its own element's end, the spawn finished
	void endElement(const std::string& theName) override
	{
		if (theName == getRootName())
			mSpawn->finish();
	}

private:
	engine::Ref<NonPlayerSpawn> mSpawn;  // +0x50
};

#endif
