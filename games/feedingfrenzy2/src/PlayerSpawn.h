#ifndef __FEEDINGFRENZY2_PLAYERSPAWN_H__
#define __FEEDINGFRENZY2_PLAYERSPAWN_H__

// A stage's player (the <player> element; PlayerSpawn, 0xa4 bytes, vtable 0x55818c (feedingfrenzy2); its constructor
// 0x493fc0 (feedingfrenzy2)): the player fish's visual and behaviour, its layer, and its phases, each a size and the
// food that grows it to the next. As the stage loads it makes the fish and a visual for each phase; as the stage starts
// it brings the fish in at the phase it reached; each step it takes the fish back once it is dead, and grows it when
// the food eaten reaches the phase's goal.

#include "MasterDocHandler.h"
#include "Spawn.h"
#include "VisualData.h"

#include <string>
#include <vector>

class FishBehaviorInfo;
class PlayerFish;

// A phase of the player (a <phase>, 8 bytes)
struct PlayerPhase
{
	PlayerPhase(float theSize, int theFoodGoal) : mSize(theSize), mFoodGoal(theFoodGoal) {}

	float mSize;    // +0x00: in the stage's size units
	int mFoodGoal;  // +0x04: the food eaten in the stage that brings the next phase
};

class PlayerSpawn : public Spawner
{
public:
	PlayerSpawn(Stage* theStage);

	// 0x04 (0x4a2280 (feedingfrenzy2)): the handler that reads the element
	engine::SubDocHandler* createDocHandler();
	// 0x08 (0x408710 (feedingfrenzy2)): the spawner's, the visual (the alienVisual for the player named alienUser),
	// the behaviour and the layer
	void read(engine::Params& theAttributes) override;
	// 0x0c (0x49b860 (feedingfrenzy2)): once read, put in the stage's spawns and its player spawns
	void finish();
	// 0x10 (0x4040f0 (feedingfrenzy2)): a visual for each phase, the behaviour, and the fish made for the stage
	void setup(int& theProgress) override;
	// 0x14 (0x49bb20 (feedingfrenzy2)): a step for each phase
	int getLoadSteps() override { return (int)mPhases.size(); }
	// 0x1c (0x490830 (feedingfrenzy2)): the fish brought in at the phase reached, unless it is in already
	void start();
	// 0x20 (0x438d40 (feedingfrenzy2)): a dead fish taken out; a live one grown once the food eaten reaches the
	// phase's goal, while there is a phase after it
	void update(engine::UpdateContext* theContext, bool canSpawn) override;

	// A <phase>, in the file's order (0x48fe50 (feedingfrenzy2))
	void addPhase(const PlayerPhase& thePhase) { mPhases.push_back(thePhase); }
	int getPhaseIndex() const { return mPhaseIndex; }
	PlayerFish* getFish() { return mFish.get(); }
	bool isLive() const { return mLive; }
	int getDeaths() const { return mDeaths; }
	// 0x48c940 (feedingfrenzy2)
	int getPhaseCount() const { return (int)mPhases.size(); }
	// port: the fish taken out of the stage while it is in (Stage::unload's, until the play's ends are translated)
	void takeOut();
	const PlayerPhase& getPhase(int theIndex) const { return mPhases[theIndex]; }

private:
	std::string mVisualName;                              // +0x2c
	std::string mBehaviorName;                            // +0x48
	int mLayer;                                           // +0x64: 0 when not given
	std::vector<PlayerPhase> mPhases;                     // +0x68
	std::vector<engine::Ref<FishVisualData>> mVisuals;    // +0x78: one for each phase, at its size
	engine::Ref<FishBehaviorInfo> mBehavior;              // +0x88
	engine::Ref<PlayerFish> mFish;                        // +0x8c
	int mPhaseIndex;                                      // +0x90: the phase reached, kept from stage to start
	int mDeaths;                                          // +0x94: the times the fish died
	bool mLive;                                           // +0x98: the fish is in the stage
};

// The element's handler (PlayerSpawn::DocHandler, 0x5c bytes, its constructor 0x4a22a0 (feedingfrenzy2))
class PlayerSpawnDocHandler : public engine::SubDocHandler
{
public:
	explicit PlayerSpawnDocHandler(PlayerSpawn* theSpawn) : mSpawn(theSpawn) {}

	// 0x04 of its own table: the spawn reads the element
	void begin(engine::Params& theAttributes) override { mSpawn->read(theAttributes); }
	// 0x00 (0x4063e0 (feedingfrenzy2)): a <phase>'s size and foodGoal
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x490390 (feedingfrenzy2)): at its own element's end, the spawn finished
	void endElement(const std::string& theName) override
	{
		if (theName == getRootName())
			mSpawn->finish();
	}

private:
	engine::Ref<PlayerSpawn> mSpawn;  // +0x50
};

#endif
