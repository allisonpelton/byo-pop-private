#ifndef __FEEDINGFRENZY2_STAGE_H__
#define __FEEDINGFRENZY2_STAGE_H__

// The game's world (Stage, 0x3e0 bytes, vtable 0x5585c4 (feedingfrenzy2); its constructor 0x403aa0 (feedingfrenzy2)):
// a stage's file read (resource/Stages/*.xml, named by a manifest's entry), its backdrop's layers, its fish and its
// player, under the game's screens. The menu's backgrounds are stages too (the menu manifest's).
//
// Translated so far: the stage's file's attributes and its spawns, the backdrop (the play bounds, the mask container
// over the layers and the layer set), the start, and the run in hundredth-second steps with the spawns' timing. Each
// untranslated step says so where it would run.

#include "Container.h"
#include "LayerSet.h"
#include "Shape.h"
#include "MasterDocHandler.h"
#include "MusicSystem.h"
#include "Spawn.h"
#include "StageManifest.h"
#include "State.h"
#include "Theme.h"

#include <map>
#include <string>
#include <vector>

class Actor;
class AIFish;
class Fish;
class GameData;
class Stage;

// The play area's edges (shape::Edge, each 0x20 bytes, at +0xc4 and every 0x20 after), each keeping what is inside it
// on the side its normal points to
enum StageEdge
{
	STAGE_EDGE_SKY = 0,           // +0xc4: below the sky's ceiling
	STAGE_EDGE_BOTTOM = 1,        // +0xe4: above the bottom
	STAGE_EDGE_LEFT = 2,          // +0x104: right of the left side
	STAGE_EDGE_RIGHT = 3,         // +0x124: left of the right side
	STAGE_EDGE_BELOW_WATER = 4,   // +0x144: below the water's surface
	STAGE_EDGE_ABOVE_WATER = 5,   // +0x164: above it
	STAGE_EDGE_COUNT = 6
};

// A state of the stage's run (StageState, vtable 0x55870c for its AbstractState (feedingfrenzy2)); the stage's
// machine (+0x3b8) runs one
class StageState : public engine::State
{
public:
	explicit StageState(Stage* theStage) : mStage(theStage) {}

protected:
	Stage* mStage;  // +0x0c
};

// The stage running (RunState, 0x18 bytes, vtable 0x5586ec (feedingfrenzy2); its constructor 0x495eb0
// (feedingfrenzy2))
class RunState : public StageState
{
public:
	explicit RunState(Stage* theStage) : StageState(theStage) {}

	// 0x0c (0x43acb0 (feedingfrenzy2)): each hundredth of a second, the stage's steps in their order
	void update(engine::UpdateContext* theContext) override;
};

// The container the stage's layers are drawn in, darkened through a mask on a night stage (MaskContainer, 0x1a8
// bytes, vtable for its Interface overriding the destructor, 0x90, 0x94, 0xa0 and 0xa4 (feedingfrenzy2); its
// constructor 0x409200 (feedingfrenzy2))
class MaskContainer : public engine::Container
{
public:
	MaskContainer() : mMaskOn(false) {}

	// 0x90 (0x51609a (feedingfrenzy2)): the mask brought in, once
	void enable() override;
	// 0xa0 (0x44f1a0 (feedingfrenzy2)): the container's own while the mask is off and not fading
	void paint(engine::DrawContext* theContext) override;

	// The stage's slot 13 sets it directly (a night stage starts with the mask on)
	void setMaskOn(bool theMaskOn) { mMaskOn = theMaskOn; }

private:
	bool mMaskOn;   // +0xc4
	// +0x17c, the mask's fade: not yet translated (0, none, until it is)
};

class Stage : public engine::Object
{
public:
	// 0x403aa0 (feedingfrenzy2): the game data and the container the stage goes in kept, and its init (0x4038d0
	// (feedingfrenzy2))
	Stage(GameData* theGameData, engine::Container* theContainer);
	// 0x48bcd0 (feedingfrenzy2): the stage unloaded first (slot 0x08)
	~Stage() override;

	// 0x04 (0x410980 (feedingfrenzy2)): the entry's file read and the stage set up from it; false when it cannot be
	// read
	virtual bool load(const StageManifest::Stage& theEntry);
	// 0x08 (0x409a90 (feedingfrenzy2)): what the last load made let go of: the spawns (their actors taken out of
	// the stage), the container emptied, the mask and the layers, and the ambient sounds stopped
	virtual void unload();

	// 0x14 (0x486950 (feedingfrenzy2)): the stage started: the play's counters reset, the camera centred and the
	// run begun (a stage with players starts them first, not yet translated)
	virtual void start();
	// 0x18 (0x490940 (feedingfrenzy2)): with no players, the run begun; the players' entry, not yet translated
	virtual void startPlayers();
	// 0x1c (0x492680 (feedingfrenzy2)): the run state put in the stage's machine
	virtual void startRun();
	// 0x20 (0x491740 (feedingfrenzy2)): the stage's container stops animating while paused, and the players and the
	// script follow (not yet translated)
	virtual void pause(bool thePaused);
	// 0x34 (0x4146b0 (feedingfrenzy2)): the play bounds, the mask container and the layer set
	virtual void setupLayers();
	// 0x43d0c0 (feedingfrenzy2): the actors the viewer sees, nearest its mouth first (one as near as one before it
	// after that one): in the same water or air, within its sight's distance of its mouth, and within its sight's
	// angle of the way it faces. The list is the stage's, made again by each call
	const std::vector<Actor*>& findSeenActors(AIFish* theViewer);
	// 0x50 (0x440660 (feedingfrenzy2)): each pair of fish meets once, the one ranked below the other's eater rank
	// offered to that one's eater (the other way only when the first is not); then each player's meetings with the
	// actors (the +0x340 list), not yet translated, as the players are not
	void updateMeetings(engine::UpdateContext* theContext);
	// 0x48 (0x4448e0 (feedingfrenzy2)): each actor's update, over a copy of the actors, one no longer among them
	// logged (the mask's lights cleared first, not yet translated)
	void updateActors(engine::UpdateContext* theContext);
	// 0x68 (0x439c10 (feedingfrenzy2)): each fish's visual given the fish's velocity (the wanted one after a turn),
	// the scroll rate, and its scales passed to the fish's shapes; at night the light's step after (not yet
	// translated)
	void updateFishVisuals(engine::UpdateContext* theContext);
	// 0x3c (0x48f900 (feedingfrenzy2)): the ambient sounds made, read from the stage's ambientSound file, at 0.7
	void loadAmbientSounds();
	// 0x6c (0x439b80 (feedingfrenzy2)): the ambient sounds' update
	void updateAmbientSounds(engine::UpdateContext* theContext);
	// 0x58 (0x43a330 (feedingfrenzy2)): each spawn's update, the actors coming in once the script's waves (not yet
	// translated) are through
	virtual void updateSpawns(engine::UpdateContext* theContext);

	// 0x43a570 (feedingfrenzy2): the frame's time run in hundredth-second steps of the current state, the rest kept
	// for the next frame; a frame shorter than a step runs as one step of its own time
	void update(engine::UpdateContext* theContext);

	// 0x4861b0 (feedingfrenzy2)
	void addSpawner(Spawner* theSpawner) { mSpawners.push_back(theSpawner); }
	// 0x485630 (feedingfrenzy2): kept also by its name
	void addNonPlayerSpawn(NonPlayerSpawn* theSpawn)
	{
		mNonPlayerSpawns.push_back(theSpawn);
		mSpawnsByName[theSpawn->getName()] = theSpawn;
	}
	// 0x405ae0 (feedingfrenzy2)
	void addFishSpawn(FishSpawn* theSpawn) { mFishSpawns.push_back(theSpawn); }

	// 0x44c6f0 (feedingfrenzy2): the play area under the water, or between the sky's ceiling and the water
	engine::RectF getArea(bool underwater) const;
	// 0x24 (0x44fbf0 (feedingfrenzy2)): the actors' container of the layer, counted from the base layer
	engine::Container* getLayerContainer(int theLayer);
	// 0x515e4e (feedingfrenzy2): the layer's offset from the base layer
	engine::Vector2f getLayerOffset(int theLayer) { return mLayerSet->getLayerOffset(theLayer); }
	const shape::Box& getViewBox() const { return mViewBox; }
	Shape* getEdge(int theEdge) { return &mEdges[theEdge]; }
	engine::Theme* getTheme() { return mTheme.get(); }
	// The lists the actors put themselves in and take themselves out of (0x41de40, 0x41d940, 0x41ea00, 0x4760b0,
	// 0x476030 and 0x476170 (feedingfrenzy2): a push, and a remove of every match)
	void addActor(Actor* theActor) { mActors.push_back(theActor); }
	void removeActor(Actor* theActor);
	void addFish(Fish* theFish) { mFish.push_back(theFish); }
	void removeFish(Fish* theFish);
	void addAIFish(AIFish* theFish) { mAIFish.push_back(theFish); }
	void removeAIFish(AIFish* theFish);

	LayerSet* getLayerSet() { return mLayerSet.get(); }
	const engine::RectF& getBounds() const { return mBounds; }
	float getSizeUnit() const { return mSizeUnit; }
	const engine::Color& getWaterColor() const { return mWaterColor; }
	GameData* getGameData() { return mGameData.get(); }
	engine::Container* getContainer() { return mContainer.get(); }
	// Set by the stage's loader from the manifest's entry (0x48a810 (feedingfrenzy2))
	void setStageTime(float theStageTime) { mStageTime = theStageTime; }
	void setCanSuck(bool canSuck) { mCanSuck = canSuck; }

private:
	friend class StageDocHandler;

	engine::Ref<GameData> mGameData;               // +0x68
	engine::RectF mBounds;                         // +0x6c: the play bounds, the display's less the HUD's band
	engine::RectF mPlayArea;                       // +0x88: the base layer's pan size, from the origin
	shape::Box mViewBox;                           // +0xa0: the play area as a box
	shape::Edge mEdges[STAGE_EDGE_COUNT];          // +0xc4
	StageManifest::Stage mEntry;                   // +0x1c: the manifest's entry, its file at +0x38
	float mScrollRate;                             // +0x184
	float mSizeUnit;                               // +0x188
	bool mBonus;                                   // +0x18c
	float mStageTime;                              // +0x190: the manifest's seconds for the stage
	float mStepTime;                               // +0x19c: the time not yet run in steps
	engine::Color mWaterColor;                     // +0x1a0
	std::string mSetPath;                          // +0x1b0: the backdrop's set file
	bool mCanSuck;                                 // +0x1d0: the manifest's canSuck, false from the constructor
	std::string mAmbientSound;                     // +0x1d4
	std::string mGameMusic;                        // +0x1f0
	bool mCameraFollows;                           // +0x260: the camera follows the players (set as the stage starts)
	float mMaskFadeTime;                           // +0x264: a night's mask's fade left, 0 as the stage starts
	std::map<std::string, engine::Ref<NonPlayerSpawn>> mSpawnsByName;  // +0x274
	std::vector<engine::Ref<Spawner>> mSpawners;   // +0x280: every spawn, in the file's order
	// +0x290, the players' spawns: not yet translated
	std::vector<engine::Ref<NonPlayerSpawn>> mNonPlayerSpawns;  // +0x2a0
	std::vector<engine::Ref<FishSpawn>> mFishSpawns;            // +0x2b0
	engine::Ref<engine::Theme> mTheme;             // +0x2d4: resource/themes/baseTheme.xml
	engine::Ref<LayerSet> mLayerSet;               // +0x2d8
	engine::Ref<engine::Container> mContainer;     // +0x2dc: the application's stage container
	engine::Ref<MaskContainer> mMaskContainer;     // +0x2ec
	bool mNight;                                   // +0x2f0
	float mLight;                                  // +0x2f4: the night's light now, from minLight
	float mLightTarget;                            // +0x2f8
	float mMinLight;                               // +0x2fc
	float mMaxLight;                               // +0x300
	engine::Ref<MusicSystem> mAmbientSounds;       // +0x318
	std::vector<Fish*> mFish;                      // +0x320
	std::vector<AIFish*> mAIFish;                  // +0x330
	std::vector<Actor*> mActors;                   // +0x370
	std::vector<Actor*> mSeen;                     // +0x380: what the last look about saw, nearest first
	bool mBoss;                                    // +0x392
	bool mFinalBoss;                               // +0x393
	std::string mBossLayout;                       // +0x394
	engine::StateMgr<StageState> mStateMgr;        // +0x3b8
};

// The stage file's handler (Stage::DocHandler, its begin 0x47c730 (feedingfrenzy2))
class StageDocHandler : public engine::SubDocHandler
{
public:
	explicit StageDocHandler(Stage* theStage) : mStage(theStage) {}

	// 0x04 of its own table (0x47c730 (feedingfrenzy2)): the <stage> element's attributes
	void begin(engine::Params& theAttributes) override;
	// 0x00 (0x41a300 (feedingfrenzy2)): a <spawn>, read by its spawn's handler; a <player>, a <script> and an
	// <epilog>, not yet translated
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	void endElement(const std::string& theName) override {}

private:
	Stage* mStage;  // +0x58
};

#endif
