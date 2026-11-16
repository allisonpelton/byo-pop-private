#ifndef __FEEDINGFRENZY_FISHSPAWNER_H__
#define __FEEDINGFRENZY_FISHSPAWNER_H__

// A kind of fish a stage has (FishSpawner, 0x1f0 bytes, constructor 0x4219d0, destructor 0x421690). It holds what a fish
// file says of the kind (its speeds, size, points, rectangles, the ranges of its decisions, how it reacts to each other
// kind, its school), the pool of fish made from it, and when and where the fish come into the stage: the rules of the
// stage's file say how many of the kind may swim at once, from which size of the player, and how long it takes a new
// one to come. A stage has one for each kind of its fish, and the stage updates each every frame.

#include "CollisionRect.h"
#include "FishBase.h"
#include "Geometry.h"
#include "ImageFilter.h"
#include "Object.h"
#include <map>
#include <string>
#include <vector>

namespace feedingfrenzy
{

class FishSchool;
class StageMgr;

// How a spawner's fish count (+0x168): by which of them are in the stage, and which have been eaten
enum SpawnerMode
{
	SPAWN_NORMAL = 0,         // fish come by the rules, as many as the rule says
	SPAWN_ONLY_UNEATEN = 1,   // each fish comes once: one that has been eaten does not come again
	SPAWN_UNCOUNTED = 2,      // the fish do not count towards the end of the stage
	SPAWN_STOPPED = 3,        // no fish come on their own; the stage puts them in
	SPAWN_PAUSED = 4,         // no fish come, and the spawner does not count as finished while any is in the stage
	SPAWN_COUNTDOWN = 5       // the fish that are left to come (+0x38) come, then no more
};

// How a fish comes into the stage (+0xf0)
enum SpawnType
{
	SPAWN_OFFSCREEN_LR = 0,   // from the left or the right edge of what shows
	SPAWN_OFFSTAGE_LR = 1,    // from the left or the right edge of the stage
	SPAWN_OFFSTAGE_BOTTOM = 2,
	SPAWN_OFFSTAGE_TOP = 3
};

// The stage's group a kind of fish is added to (+0xec)
enum SpawnLayer
{
	LAYER_FOREGROUND = 0,
	LAYER_MIDDLE = 1,
	LAYER_BACKGROUND = 2
};

// What a fish does when it has noticed another kind of fish (the fish file's <react>): an 8-byte record at 0x421940
struct FishReaction
{
	int mKind;       // a Reaction (AIFish.h)
	float mChance;   // how likely each check of the fish makes it do so
};

// A rule of the stage's file (<spawnInfo>, 0x420800): from the size of the player, how many fish of the kind there may
// be at once, and how long it takes one to come after another has gone (a time from the range)
struct SpawnRule
{
	float mMinPlayerLevel;
	int mMaxCount;
	float mRespawnLow;
	float mRespawnHigh;
};

// A fish that is in the stage when it starts (0x420bf0): where, and the value of the fish's slot 0x194
struct SpawnPlacement
{
	float mX;
	float mY;
	bool mMarked;
};

class FishSpawner
{
public:
	FishSpawner();   // 0x4219d0
	~FishSpawner();  // 0x421690

	// 0x41e0c0: reads the fish file of the kind and notes the size of the pool
	bool load(const std::string& theFile, int thePoolSize);
	// 0x420890: makes the fish of the pool, from what the file says
	void createFish();

	// 0x41e060: how the fish of the kind come (a SpawnerMode); a spawner that does not count keeps that
	void setMode(int theMode);
	int getMode() const { return mMode; }  // 0x41dd00
	int getPoolSize() const { return mPoolSize; }  // 0x41dd10
	// 0x41db50: whether the stage's list of the fish eaten shows the kind's picture
	void setDisplayMenuImage(bool theDisplay) { mDisplayMenuImage = theDisplay; }
	bool getDisplayMenuImage() const { return mDisplayMenuImage; }
	// +0x1ec: the kind has its picture in the HUD (and the end screen) already
	bool isShownInHud() const { return mShownInHud; }
	void setShownInHud(bool theShown) { mShownInHud = theShown; }
	// 0x420800: a rule from the stage's file; 0x420bf0: a fish that stands in the stage when it starts
	void addRule(float theMinPlayerLevel, int theMaxCount, float theRespawnLow, float theRespawnHigh);
	void addPlacement(float theX, float theY, bool theMarked);
	int getPlacementCount() const { return (int)mPlacements.size(); }  // 0x41dd20

	// 0x41de30: the rule that holds for the player's size (the last of the list whose size the player has reached)
	void pickRule();
	// 0x41de90: counts the fish of the pool: in the stage, never eaten, and eaten (all, fish and bonus bubbles)
	void recount();
	// 0x41df30: the next fish of the pool that can come (null when there is none)
	FishBase* findFreeFish();
	// 0x41dfd0: the spawner has no fish left to bring or in the stage, by its mode
	bool isDone();
	// 0x41e1e0: a fish of the pool comes into the stage at a place with a velocity; null when none is free
	FishBase* spawnAt(StageMgr* theStage, bool theMarked, float theX, float theY, float theVelocityX, float theVelocityY);
	// 0x41e2e0: a fish comes in, from beyond the edge its kind comes from, or at a place
	void spawnOffscreen(StageMgr* theStage, bool theAtPosition, float theX, float theY, bool theMarked);
	// 0x41e370: a school of the kind comes in together
	void spawnSchool(StageMgr* theStage, int theCount);
	// 0x41e5f0: the fish that stand in the stage when it starts, and the first wait
	void spawnInitial();
	// 0x41e8f0: a frame: the wait runs out and fish come in
	void update(engine::UpdateInfo& theInfo, StageMgr* theStage);
	// 0x41d8c0: a fish is put beyond an edge, where the kind comes from, facing in
	void placeOffscreen(FishBase* theFish);

	// 0x41f640: how the kind reacts to a kind of fish, or null
	const FishReaction* findReaction(const std::string& theTypeName) const;

	// The pool and the counts
	int getInPlayCount() const { return mInPlayCount; }       // +0x24
	int getUneatenCount() const { return mUneatenCount; }     // +0x28
	int getEatenCount() const { return mEatenCount; }         // +0x2c: how often the fish of the kind have been eaten
	int getFishEatenCount() const { return mFishEatenCount; } // +0x30: of those, the fish
	int getBonusEatenCount() const { return mBonusEatenCount; }  // +0x34: and the bonus bubbles
	const std::vector<engine::Ref<FishBase>>& getPool() const { return mPool; }
	const std::string& getMenuImage() const { return mMenuImage; }
	float getLevel() const { return mLevel; }  // +0x8c
	engine::HsvShiftFilter* getColorShift() const { return mColorShift; }  // +0x1e8
	const std::string& getClassName() const { return mClassName; }

	// The fish file's elements, set while it is read (0x41d820 and the setters after it)
	void setSpeeds(const engine::Point& theMaxSpeedX, const engine::Point& theMaxSpeedY, const engine::Point& theMinSpeedX,
	               const engine::Point& theMinSpeedY, float theSpeedUp, float theSlowDown);                  // 0x41d820
	void setSizeAndLevel(float theLevel, float theSize);                                                    // 0x41d870
	void setVision(float theRange, float theAngle);                                                         // 0x41d890
	void setPoints(float thePoints) { mPoints = thePoints; }                                                // 0x41d8b0
	void setBodyRect(const engine::Rect& theRect) { mBodyRect = theRect; }                                  // 0x41db60
	void setIdleRanges(const engine::Point& theDecision, const engine::Point& theState);                    // 0x41db90
	void setChaseRanges(const engine::Point& theDecision, const engine::Point& theState);                   // 0x41dbc0
	void setFleeRanges(const engine::Point& theDecision, const engine::Point& theState);                    // 0x41dbf0
	void setSightCheckRange(float theRange) { mSightCheckRange = theRange; }                                // 0x41dc20
	void setHitRect(const CollisionRect& theRect);                                                          // 0x41dc40
	void setLayer(int theLayer) { mLayer = theLayer; }                                                      // 0x41dc70
	// 0x41dc80
	void setSchool(const engine::Point& theSize, float theDistance, float theNeighbourDistance, float theSeparation,
	               float theAlignment, float theCohesionDivisor, float theTarget, float theChase, float theRun);
	void setColorShift(engine::HsvShiftFilter* theShift) { mColorShift = theShift; }                        // 0x41dcf0
	void setMouthRect(const CollisionRect& theRect) { mMouthRect = theRect; }                               // 0x41dd80
	void setAnimPath(const std::string& thePath) { mAnimPath = thePath; }                                   // 0x41f880
	void setClassName(const std::string& theName) { mClassName = theName; }                                 // 0x41f8e0
	void setEatenSound(const std::string& theName, float thePitch);                                        // 0x41f940
	void setMenuImage(const std::string& theImage) { mMenuImage = theImage; }                               // 0x41f9b0
	void setThoughtBubble(const ThoughtBubbleInfo& theInfo) { mThought = theInfo; }                         // 0x41ff80
	void setSpawnType(const std::string& theName);                                                          // 0x41e760
	void addReaction(const std::string& theTypeName, FishReaction* theReaction);                           // 0x421940

private:
	// The fish file's attributes, as the file says them (each a range, from which each fish draws its own)
	std::vector<SpawnRule> mRules;             // +0x08
	std::vector<SpawnPlacement> mPlacements;   // +0x18
	int mInPlayCount;                          // +0x24: its fish that are in the stage
	int mUneatenCount;                         // +0x28: its fish that have not been eaten
	int mEatenCount;                           // +0x2c
	int mFishEatenCount;                       // +0x30
	int mBonusEatenCount;                      // +0x34
	int mRemaining;                            // +0x38: the fish that are left to come (mode SPAWN_COUNTDOWN)
	int mPoolSize;                             // +0x3c
	float mRespawnSeconds;                     // +0x40: until the next fish comes (negative: not waiting)
	std::string mClassName;                    // +0x44: the class of the fish
	engine::Point mMaxSpeedX;                  // +0x60
	engine::Point mMaxSpeedY;                  // +0x68
	engine::Point mMinSpeedX;                  // +0x70
	engine::Point mMinSpeedY;                  // +0x78
	float mSpeedUp;                            // +0x80
	float mSlowDown;                           // +0x84
	float mSize;                               // +0x88: the pictures' scale (the file's "scale")
	float mLevel;                              // +0x8c: the file's "size", which says who eats whom
	std::string mAnimPath;                     // +0x90
	float mPoints;                             // +0xac
	float mVisionRange;                        // +0xb0
	float mVisionAngle;                        // +0xb4
	engine::Point mWanderDecisionRange;        // +0xb8
	engine::Point mWanderStateRange;           // +0xc0
	engine::Point mChaseDecisionRange;         // +0xc8
	engine::Point mChaseStateRange;            // +0xd0
	engine::Point mRunDecisionRange;           // +0xd8
	engine::Point mRunStateRange;              // +0xe0
	float mSightCheckRange;                    // +0xe8
	int mLayer;                                // +0xec: a SpawnLayer
	int mSpawnType;                            // +0xf0: a SpawnType
	std::string mMenuImage;                    // +0xf4
	bool mDisplayMenuImage;                    // +0x110
	std::string mEatenSoundName;               // +0x114
	float mEatenSoundPitch;                   // +0x130
	std::vector<engine::Ref<FishBase>> mPool;  // +0x138
	engine::Rect mBodyRect;                    // +0x144
	CollisionRect mMouthRect;                  // +0x154
	int mMode;                                 // +0x168
	int mCurrentRule;                          // +0x16c: the place in mRules of the rule that holds (-1: none)
	bool mHasHitRect;                          // +0x174
	CollisionRect mHitRect;                    // +0x178
	engine::Point mSchoolSize;                 // +0x18c: the least and the most fish in a school
	float mSchoolDistance;                     // +0x194
	float mSchoolNeighbourDistance;            // +0x198
	float mSchoolSeparation;                   // +0x19c
	float mSchoolAlignment;                    // +0x1a0
	float mSchoolCohesionDivisor;              // +0x1a4
	float mSchoolTarget;                       // +0x1a8
	float mSchoolChase;                        // +0x1ac
	float mSchoolRun;                          // +0x1b0
	ThoughtBubbleInfo mThought;                // +0x1b4
	std::map<std::string, FishReaction*> mReactions;  // +0x1dc
	engine::HsvShiftFilter* mColorShift;       // +0x1e8
	bool mShownInHud;                          // +0x1ec
};

} // namespace feedingfrenzy

#endif
