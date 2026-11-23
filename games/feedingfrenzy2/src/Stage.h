#ifndef __FEEDINGFRENZY2_STAGE_H__
#define __FEEDINGFRENZY2_STAGE_H__

// The game's world (Stage, 0x3e0 bytes, vtable 0x5585c4 (feedingfrenzy2); its constructor 0x403aa0 (feedingfrenzy2)):
// a stage's file read (resource/Stages/*.xml, named by a manifest's entry), its backdrop's layers, its fish and its
// player, under the game's screens. The menu's backgrounds are stages too (the menu manifest's).
//
// Translated so far: the stage's file's attributes and its spawns, the backdrop (the play bounds, the mask container
// over the layers and the layer set), the start, and the run in hundredth-second steps with the spawns' timing. Each
// untranslated step says so where it would run.

#include "BubbleText.h"
#include "Container.h"
#include "KeyInterpolator.h"
#include "LayerSet.h"
#include "Shape.h"
#include "MasterDocHandler.h"
#include "MusicSystem.h"
#include "Spawn.h"
#include "StageManifest.h"
#include "State.h"
#include "Theme.h"
#include "Timetable.h"

#include <map>
#include <string>
#include <vector>

class Actor;
class AIFish;
class BonusItem;
class Fish;
class GameData;
class GameHud;
class PlayerFish;
class PlayerSpawn;
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

	// 0x04 (0x48d440 (feedingfrenzy2)): the game's music started (not yet translated, as the music is not), and with
	// players the first one hears the mouse
	void enter() override;
	// 0x0c (0x43acb0 (feedingfrenzy2)): each hundredth of a second, the stage's steps in their order
	void update(engine::UpdateContext* theContext) override;
};

// The stage's goal reached, or its time up (CompletionState, 0x54 bytes, vtables 0x55874c and 0x55876c
// (feedingfrenzy2); its constructor 0x49f3d0 (feedingfrenzy2)): the stage plays on without the player's part while
// its word bubbles in at once, bursts at 2.5 seconds, and at 3.5 the run ends
class CompletionState : public StageState
{
public:
	// 0x49f3d0 (feedingfrenzy2): three marks, and the theme's sound to play
	CompletionState(Stage* theStage, const std::string& theSound);

	// 0x04 (0x4994b0 (feedingfrenzy2)): the marks at 0, 2.5 and 3.5 seconds started; the time left kept in the game
	// data for the timed mode (added on a bonus stage whose goal was reached), the sound played and the game kept
	// in the player's record and saved
	void enter() override;
	// 0x08 (0x4a3ae0 (feedingfrenzy2)): the word taken off, the sounds stopped
	void exit() override;
	// 0x0c (0x4334b0 (feedingfrenzy2)): the actors, the spawns, the camera, the fish's visuals, the HUD and the
	// ambient sounds updated; at the first mark the word bubbling in over 2 seconds, at the second burst, at the
	// third the run ended (the mystery stage's epilog after one whose goal was reached, not yet translated)
	void update(engine::UpdateContext* theContext) override;

private:
	Timetable mMarks;     // +0x10
	std::string mSound;   // +0x38: the theme's sound
};

// The player's fish died (PlayerDiedState, 0x98 bytes, vtables 0x558784 and 0x5587a4 (feedingfrenzy2); its
// constructor 0x498830 (feedingfrenzy2)): the stage plays on without the player while the word playerDied bubbles
// in, the camera holds for 1.5 seconds and then pans to the top of the middle by 3, the word bursts at 2.5 seconds,
// and at 3.5 the player comes back (or, with no lives left, the run ends)
class PlayerDiedState : public StageState
{
public:
	// 0x498830 (feedingfrenzy2): three marks
	explicit PlayerDiedState(Stage* theStage) : StageState(theStage), mMarks(COMPLETION_MARKS) {}

	// 0x04 (0x4034b0 (feedingfrenzy2)): the theme's sound playerDie (not yet translated), the word set, the marks at
	// 0, 2.5 and 3.5 seconds started, and the camera's path from where it is
	void enter() override;
	// 0x08 (0x4a0690 (feedingfrenzy2)): the word taken off
	void exit() override;
	// 0x0c (0x434cb0 (feedingfrenzy2)): the actors, the spawns, the HUD, the camera on its path, the fish's visuals
	// and the ambient sounds updated; at the first mark the word bubbling in over 2 seconds, at the second burst, at
	// the third the players started again (the stage's slot 0x18)
	void update(engine::UpdateContext* theContext) override;

private:
	static const int COMPLETION_MARKS = 3;

	Timetable mMarks;                         // +0x18
	engine::Vector2KeyInterpolator mCamera;   // +0x40
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

	// 0x14 (0x486950 (feedingfrenzy2)): the stage started: the play's counters reset; with no player in yet, the
	// players brought in and the camera centred, else the player flashed again; and the run begun
	virtual void start();
	// 0x18 (0x490940 (feedingfrenzy2)): with player spawns and lives left, the food eaten so far from the first
	// spawn's phase and its fish brought in, and the run begun; with no lives the machine emptied; with no player
	// spawns only the run
	virtual void startPlayers();
	// 0x1c (0x492680 (feedingfrenzy2)): the run state put in the stage's machine
	virtual void startRun();
	// 0x20 (0x491740 (feedingfrenzy2)): the stage's container stops animating while paused, and the first player
	// stops hearing the mouse, or hears it again (the script's pause not yet translated)
	virtual void pause(bool thePaused);
	// 0x30 (0x4146b0 (feedingfrenzy2)): the play bounds, the mask container and the layer set
	virtual void setupLayers();
	// 0x34 (0x40d1f0 (feedingfrenzy2)): with player spawns, the HUD put on top of the stage's container and made
	// ready, given the first spawn's growth (a bonus stage's count instead, 0x4a3f70 (feedingfrenzy2), not yet
	// translated), the bonus's counter shown on a bonus stage and the suck shown; then the overlays (not yet)
	virtual void setupOverlays();
	// 0x43d0c0 (feedingfrenzy2): the actors the viewer sees, nearest its mouth first (one as near as one before it
	// after that one): in the same water or air, within its sight's distance of its mouth, and within its sight's
	// angle of the way it faces. The list is the stage's, made again by each call
	const std::vector<Actor*>& findSeenActors(AIFish* theViewer);
	// 0x60 (0x43a950 (feedingfrenzy2)): the camera after the first player while it follows, and the night's mask's
	// light at its visual (not yet translated, as the mask is not)
	void updateCamera(engine::UpdateContext* theContext);
	// 0x50 (0x440660 (feedingfrenzy2)): each pair of fish meets once, the one ranked below the other's eater rank
	// offered to that one's eater (the other way only when the first is not); then each player's meetings with the
	// actors (the +0x340 list), not yet translated, as the players are not
	void updateMeetings(engine::UpdateContext* theContext);
	// 0x54 (0x438ee0 (feedingfrenzy2)): each player's eater offered each bonus in the stage
	void updateBonusMeetings(engine::UpdateContext* theContext);
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
	// 0x5c (0x447ef0 (feedingfrenzy2)): with player spawns, while the first one's fish is in, the HUD's update and
	// the stage's time counted down. Not yet translated: the jump's marks, the player's death, the phases' and the
	// times' counts in the game data, the stage's goal and the time's end, which end the stage
	void updatePlay(engine::UpdateContext* theContext);
	// 0x416330 (feedingfrenzy2): the word the stage bubbles at its end (+0x2e8), in the theme's bubbleTextFont,
	// bubbleTextBubble and bubbleTextFx at (400, 200), 8 pixels apart, with glyphs ready for the longest of its words
	void setupMessage();
	// 0x498d20 (feedingfrenzy2): the word set to the string of the id
	void setMessage(const std::string& theStringId);
	// 0x4990f0 (feedingfrenzy2): the word put over the stage and bubbling in from the start over the duration
	void showMessage(float theStart, float theDuration);
	// 0x4959a0 (feedingfrenzy2): the word burst, with the theme's sound bubbleTextBurst (not yet translated)
	void burstMessage();
	// 0x4a3b00 (feedingfrenzy2): the word taken off
	void hideMessage();
	// 0x4a2390 (feedingfrenzy2): every non-player spawn's actors sent away and the spawn turned off once they are
	// gone; the orphans' too (+0x2c0's slot 0x2c, not yet translated, as the orphanage is not)
	void stopSpawns();
	// 0x44b9b0 (feedingfrenzy2): while the first spawn's fish is in, the growth meter at the food eaten (not on a
	// bonus stage), the time, the lives and the suck's charge shown (the bonuses' bubbles not yet)
	void updateHud(engine::UpdateContext* theContext);
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
	// 0x4a1800 (feedingfrenzy2)
	void addPlayerSpawn(PlayerSpawn* theSpawn);
	// 0x4972f0 and 0x497280 (feedingfrenzy2): a push, and a remove of every match
	void addPlayer(PlayerFish* thePlayer);
	// 0x410710 (feedingfrenzy2): the player's points added to the score; an extra life told by the HUD's text
	// extraLife at the player and the theme's sound bonus1Up (not yet translated, as the HUD and the sounds are not)
	void addPlayerScore(int thePoints, PlayerFish* thePlayer);
	// The first player in the stage, null when none is
	PlayerFish* getFirstPlayer() { return mPlayers.empty() ? nullptr : mPlayers.front().get(); }
	void removePlayer(PlayerFish* thePlayer);

	// 0x44c6f0 (feedingfrenzy2): the play area under the water, or between the sky's ceiling and the water
	engine::RectF getArea(bool underwater) const;
	// 0x24 (0x44fbf0 (feedingfrenzy2)): the actors' container of the layer, counted from the base layer
	engine::Container* getLayerContainer(int theLayer);
	// 0x4678b0 (feedingfrenzy2): a point of the layer's in the screen's space, moved by the layer's container's place
	engine::Vector2f toScreen(int theLayer, const engine::Vector2f& thePoint);
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
	// 0x48edc0 and 0x48a950 (feedingfrenzy2)
	void addBonus(BonusItem* theBonus) { mBonuses.push_back(theBonus); }
	void removeBonus(BonusItem* theBonus);
	const std::vector<BonusItem*>& getBonuses() const { return mBonuses; }
	void removeAIFish(AIFish* theFish);

	LayerSet* getLayerSet() { return mLayerSet.get(); }
	const engine::RectF& getBounds() const { return mBounds; }
	float getSizeUnit() const { return mSizeUnit; }
	const engine::Color& getWaterColor() const { return mWaterColor; }
	GameData* getGameData() { return mGameData.get(); }
	GameHud* getHud() { return mHud.get(); }
	bool isBoss() const { return mBoss; }
	bool isFinalBoss() const { return mFinalBoss; }
	const std::string& getBossLayout() const { return mBossLayout; }
	const std::vector<engine::Ref<NonPlayerSpawn>>& getNonPlayerSpawns() const { return mNonPlayerSpawns; }
	const std::vector<engine::Ref<FishSpawn>>& getFishSpawns() const { return mFishSpawns; }
	engine::Container* getContainer() { return mContainer.get(); }
	// Set by the stage's loader from the manifest's entry (0x48a810 (feedingfrenzy2))
	void setStageTime(float theStageTime) { mStageTime = theStageTime; }
	void setCanSuck(bool canSuck) { mCanSuck = canSuck; }
	bool getCanSuck() const { return mCanSuck; }
	bool isBonus() const { return mBonus; }
	bool isCompleted() const { return mCompleted; }
	float getTimeLeft() const { return mTimeLeft; }
	// port (the test command set time): the stage's seconds left
	void setTimeLeft(float theSeconds) { mTimeLeft = theSeconds; }
	MusicSystem* getAmbientSounds() { return mAmbientSounds.get(); }
	engine::StateMgr<StageState>& getStateMgr() { return mStateMgr; }
	// The run's current state, null once the run is over
	StageState* getRunState() { return mStateMgr.getState(); }
	// RunState's enter: with player spawns, the first player hears the mouse
	void connectFirstPlayer();

private:
	friend class StageDocHandler;

	// The game data's food eaten so far as the stage starts (inlined in 0x490940 and 0x486950 (feedingfrenzy2)):
	// the goal of the spawn's phase before the one it reached, 0 at the first phase or on a boss's stage
	void setFoodFromPhase(PlayerSpawn* theSpawn);

	engine::Ref<GameData> mGameData;               // +0x68
	engine::RectF mBounds;                         // +0x6c: the play bounds, the display's less the HUD's band
	engine::RectF mPlayArea;                       // +0x88: the base layer's pan size, from the origin
	shape::Box mViewBox;                           // +0xa0: the play area as a box
	shape::Edge mEdges[STAGE_EDGE_COUNT];          // +0xc4
	StageManifest::Stage mEntry;                   // +0x1c: the manifest's entry, its file at +0x38
	float mScrollRate;                             // +0x184
	float mSizeUnit;                               // +0x188
	bool mBonus;                                   // +0x18c
	float mStageTime;                              // +0x190: the manifest's seconds for the stage (in the timed mode
	                                               // with the time carried in added as it starts)
	float mTimeLeft;                               // +0x198: the stage's time left, from +0x190 as it starts
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
	std::vector<engine::Ref<PlayerSpawn>> mPlayerSpawns;        // +0x290
	std::vector<engine::Ref<NonPlayerSpawn>> mNonPlayerSpawns;  // +0x2a0
	std::vector<engine::Ref<FishSpawn>> mFishSpawns;            // +0x2b0
	engine::Ref<engine::Theme> mTheme;             // +0x2d4: resource/themes/baseTheme.xml
	engine::Ref<LayerSet> mLayerSet;               // +0x2d8
	engine::Ref<engine::Container> mContainer;     // +0x2dc: the application's stage container
	engine::Ref<GameHud> mHud;                     // +0x2e0
	engine::Ref<engine::Container> mOverlays;      // +0x2e4: "StageOverlays", over the stage's container
	engine::Ref<BubbleText> mMessage;              // +0x2e8: the word at the stage's end
	engine::Ref<MaskContainer> mMaskContainer;     // +0x2ec
	bool mNight;                                   // +0x2f0
	float mLight;                                  // +0x2f4: the night's light now, from minLight
	float mLightTarget;                            // +0x2f8
	float mMinLight;                               // +0x2fc
	float mMaxLight;                               // +0x300
	engine::Ref<MusicSystem> mAmbientSounds;       // +0x318
	std::vector<Fish*> mFish;                      // +0x320
	std::vector<AIFish*> mAIFish;                  // +0x330
	std::vector<engine::Ref<PlayerFish>> mPlayers;  // +0x340: the player fish in the stage
	std::vector<BonusItem*> mBonuses;              // +0x360: the bonuses in the stage
	std::vector<Actor*> mActors;                   // +0x370
	std::vector<Actor*> mSeen;                     // +0x380: what the last look about saw, nearest first
	// +0x391: the stage's goal reached, cleared as it starts (set at 0x4f2029 and 0x4f218f (feedingfrenzy2) as the
	// player reaches it; at 0x4f1f19 and 0x541f09 (feedingfrenzy2), on a bonus stage and after a boss, not yet
	// translated); the play's end reads it
	bool mCompleted;
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
	// 0x00 (0x41a300 (feedingfrenzy2)): a <player> and a <spawn>, each read by its spawn's handler; a <script> and
	// an <epilog>, not yet translated
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	void endElement(const std::string& theName) override {}

private:
	Stage* mStage;  // +0x58
};

#endif
