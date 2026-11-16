#ifndef __FEEDINGFRENZY_PLAYERFISH_H__
#define __FEEDINGFRENZY_PLAYERFISH_H__

// The player's fish (PlayerFish, 0x40c bytes, constructor 0x414840, vtable 0x4a2910, key handler at +0x2e8). It swims
// where the mouse takes it, eats the fish smaller than it, grows as it eats, and is eaten by bigger ones. It keeps the
// player's score for the stage, the chain of fish eaten that makes a frenzy, and the effects of the bonus bubbles
// (twice the score, speed, shield, stun, feeding fury) and of what hurts it (stings and poison). The stage's file says
// the kind of player fish (this class or JohnDoryPlayer), its speeds, its pictures and the sizes it grows through.

#include "AnimImage.h"
#include "FishBase.h"
#include "Node.h"
#include "Particles.h"

#include <vector>

namespace feedingfrenzy
{

class GrowMeter;
class PlayerShieldFx;
class PlayerThoughtBubble;
class StunBubbleFx;

// What the player's fish is doing (+0x388)
enum PlayerState
{
	PLAYER_SWIM = 0,         // it swims where the mouse takes it
	PLAYER_GROW = 1,         // it is growing to the next size
	PLAYER_BEING_EATEN = 2,  // a bigger fish has it in its mouth
	PLAYER_DYING = 3,        // it was eaten or stung and waits to come back
	PLAYER_SPAWN = 4,        // it comes into the stage from above, and cannot be hurt
	PLAYER_INVINCIBLE = 5,   // it cannot be hurt for a time
	PLAYER_FURY = 6,         // the feeding fury: it swims on its own to the fish and eats them
	PLAYER_SPECIAL = 7       // the kind's special power is in use
};

// A size the player's fish grows through (the stage's file lists them, 0xc bytes each)
struct GrowthLevel
{
	float mLevel;  // +0x00: the fish's level (who it can eat) while it has this size
	float mSize;   // +0x04: the picture's scale
	int mScore;    // +0x08: the score the size starts at
};

// What the feeding fury does (+0x3f4)
enum FuryPhase
{
	FURY_PICK = 0,    // it looks for a fish to eat
	FURY_CHASE = 1,   // it swims to the fish
	FURY_RETURN = 2   // it swims back to where it began
};

class PlayerFish : public FishBase, public engine::KeyHandler
{
public:
	PlayerFish();            // 0x414840
	~PlayerFish() override;  // 0x4142c0

	// 0x300: the mouse's movement is the fish's, and the cursor is kept in the window's middle; the menu's fish is not
	void setCursorLocked(bool theLocked) { mCursorLocked = theLocked; }
	void setFluidDrag(float theDrag) { mFluidDrag = theDrag; }  // how much the water slows the fish by its speed

	// The mouse's buttons (0x248 to 0x254): the left button's going up and down, the right's
	virtual void leftButtonUp() { releaseSpecial(); }  // 0x248: 0x410490
	virtual void leftButtonDown();                     // 0x24c: 0x4104a0
	virtual void rightButtonUp() {}                    // 0x250: 0x40dcf0
	virtual void rightButtonDown();                    // 0x254: 0x4104f0
	// 0x258: 0x40fa80: the point of the stage the cursor is at; none when the flag is off
	virtual void setTarget(float theX, float theY, bool theHasTarget);

	// The fish's frame by what it is doing (0x25c to 0x274)
	virtual void updateDying(engine::UpdateInfo& theInfo);       // 0x25c: 0x40fbf0
	virtual void updateSwimming(engine::UpdateInfo& theInfo);        // 0x260: 0x40fcb0
	virtual void updateInvincible(engine::UpdateInfo& theInfo);  // 0x264: 0x40fed0
	virtual void updateSpawn(engine::UpdateInfo& theInfo);       // 0x268: 0x412200
	virtual void updateSpecial(engine::UpdateInfo& theInfo);     // 0x26c: 0x410410 (the kinds make their own)
	virtual void updateGrow(engine::UpdateInfo& theInfo);        // 0x270: 0x411180
	virtual void updateFury(engine::UpdateInfo& theInfo);        // 0x274: 0x410b00
	virtual void updateFlicker(engine::UpdateInfo& theInfo);     // 0x278: 0x40ff40: the blink of a fish that cannot be hurt
	virtual void keepInStage();                                  // 0x27c: 0x40fd00: it stops at the stage's edges
	virtual int getState() { return mState; }                    // 0x280: 0x40df90
	virtual void addToChain(int theCount);                       // 0x284: 0x4125d0: the frenzy chain grows
	virtual void onAte(FishBase* theVictim);                     // 0x288: 0x415810: what eating a fish gives
	virtual void addPoints(float thePoints);                     // 0x28c: 0x410020: to the player's score in the game
	virtual void onAteFish(int thePoints);                       // 0x290: 0x412560
	virtual void onSchoolEaten();                                // 0x294: 0x416090
	virtual void die();                                          // 0x298: 0x411be0
	virtual void startLife();                                    // 0x29c: 0x411e70
	virtual void dash();                                         // 0x2a0: 0x412350
	virtual bool isInvincible();                                 // 0x2a4: 0x40ffb0 (spawning or invincible)
	virtual bool isDying() { return mState == PLAYER_DYING; }    // 0x2a8: 0x410130
	virtual float getGrowthProgress();                           // 0x2ac: 0x411310: how far to the next size, 0 to 1
	virtual float getScore() { return mScore; }                  // 0x2b0: 0x410430
	virtual float getScoreToNextLevel();                         // 0x2b4: 0x411290
	virtual void addGrowthLevel(float theLevel, float theSize, int theScore);  // 0x2b8: 0x416060
	virtual void useSpecial() { dash(); }                        // 0x2bc: 0x410420: the control key
	virtual void releaseSpecial() {}                             // 0x2c0: 0x40dcf0
	virtual void poison(float theSeconds);                       // 0x2c4: 0x413580
	virtual void setInvincible(float theSeconds);                // 0x2c8: 0x410590
	virtual float getDoubleScoreSeconds() { return mDoubleScoreSeconds; }  // 0x2cc: 0x40dfa0
	virtual float getSpeedBoostSeconds() { return mSpeedBoostSeconds; }    // 0x2d0: 0x40dfb0
	virtual void setFrenzyTime(float theSeconds) { mFrenzyTime = theSeconds; }  // 0x2d4: 0x410440
	virtual int getChainCount() { return mChainCount; }          // 0x2d8: 0x410450
	virtual int updateFrenzyLevel();                             // 0x2dc: 0x410460: the level, 1 for each 7 in the chain
	virtual int getGrowthLevelCount();                           // 0x2e0: 0x410ad0
	virtual float getGrowthScore(int theIndex);                  // 0x2e4: 0x411390: the score a size starts at
	virtual bool isSpecialReady() { return false; }              // 0x2e8: 0x4608c0
	virtual float getSpecialProgress() { return 0.0f; }          // 0x2ec: 0x410520
	virtual void updateGrowth();                                 // 0x2f0: 0x412d60: the size follows the score
	// 0x2f4: 0x4108a0: the fish's speed follows how far the mouse moved in the seconds
	virtual void followMouse(float theSeconds);
	virtual void updatePoisonColor();                            // 0x2f8: 0x410530
	virtual void startFeedingFury();                             // 0x2fc: 0x413810
	virtual void beginFury();                                    // 0x300: 0x4138b0
	virtual FishBase* findFuryTarget();                          // 0x304: 0x410ff0
	virtual void finishFury();                                   // 0x308: 0x413b70
	virtual void endFury();                                      // 0x30c: 0x413a20
	virtual void stunAll();                                      // 0x310: 0x410f20
	virtual void gainShield();                                   // 0x314: 0x4105b0
	virtual void loseShield();                                   // 0x318: 0x413780

	// FishBase
	void update(engine::UpdateInfo& theInfo) override;              // 0x7c: 0x411620
	void layout() override;                                         // 0x80: 0x410e80
	void loadAnimationsOf(const std::string& theFolder) override;   // 0xe8: 0x414af0
	void loadEffects() override;                                    // 0xec: 0x415010
	bool canEat(FishBase* theVictim) override;                      // 0x138: 0x40ffd0
	bool canBeEatenByLevel(float theEaterLevel) override;           // 0x13c: 0x410040
	bool canBeEatenBy(FishBase* theEater) override;                 // 0x140: 0x4100c0
	void startBeingEaten(FishBase* theEater, float theSeconds) override;  // 0x144: 0x412ff0
	bool finishBeingEaten(FishBase* theEater) override;             // 0x148: 0x413100
	void eat(FishBase* theVictim) override;                         // 0x160: 0x4154b0
	void finishEating() override;                                   // 0x164: 0x412720
	void stun(float theSeconds, FishBase* theSource, bool theShowEffect) override;  // 0x1bc: 0x413380
	void updateRects() override;                                    // 0x220: 0x40fb30
	void move(float theSeconds) override;                           // 0x22c: 0x410690

	// The key handler (+0x2e8): the arrow keys, the space bar and the control key
	bool keyDown(int theKey) override;  // 0x40f970
	bool keyUp(int theKey) override;    // 0x40fa30

protected:
	engine::Point mMouseMove;           // +0x2f0: how far the mouse moved this frame, as a speed
	engine::Point mCursorCentre;        // +0x2f8: where the mouse is measured from: where it was when the life began, then the point it is put back to
	bool mCursorLocked;                 // +0x300: the mouse's movement is the fish's (not in the menu)
	float mFluidDrag;                   // +0x304: how much the water slows the fish by its speed
	bool mSpawnSoundPlayed;             // +0x308
	bool mKeyUp;                        // +0x309
	bool mKeyDown;                      // +0x30a
	bool mKeyLeft;                      // +0x30b
	bool mKeyRight;                     // +0x30c
	float mDyingSeconds;                // +0x310: how long it waits dead
	float mDashSeconds;                 // +0x314: how long a dash lasts
	float mDashCooldown;                // +0x318: how long before it can dash again
	float mInvincibleSeconds;           // +0x31c
	float mFlickerSeconds;              // +0x320
	int mGrowToIndex;                   // +0x324: the size it grows to
	int mGrowFromIndex;                 // +0x328: and the one it grew from
	float mGrowSeconds;                 // +0x32c: how long the growing has left
	float mStunSeconds;                 // +0x330
	float mStunImmuneSeconds;           // +0x334: the seconds after a stun that it cannot be stunned
	engine::Ref<engine::ParticleSystem> mStunFx;          // +0x338
	engine::Ref<engine::AnimImage> mStars;                 // +0x33c: the stars over a stunned or poisoned fish
	engine::Ref<engine::ParticleSystem> mSpawnBubblesFx;   // +0x340
	bool mPoisonRamped;                 // +0x344: the poison's colour has come to its full
	float mPoisonTotal;                 // +0x348
	float mPoisonLeft;                  // +0x34c
	float mPoisonBlend;                 // +0x350: how much of the poison's colour shows
	float mPoisonCooldown;              // +0x354: the seconds after poison that it cannot be poisoned
	engine::Ref<engine::Container> mPoisonGroup;           // +0x358
	engine::Ref<GrowMeter> mPoisonFront;                   // +0x35c
	engine::Ref<GrowMeter> mPoisonBack;                    // +0x360
	engine::Ref<PlayerThoughtBubble> mThoughtBubble;       // +0x364
	float mDoubleScoreSeconds;          // +0x368
	float mScoreMultiplier;             // +0x36c
	float mSpeedBoostSeconds;           // +0x370
	float mSpeedMultiplier;             // +0x374
	float mScore;                       // +0x37c
	int mState;                         // +0x388
	engine::Point mTarget;              // +0x38c
	engine::Point mTargetVelocity;      // +0x394
	std::vector<GrowthLevel> mGrowthLevels;  // +0x39c
	int mGrowthIndex;                   // +0x3ac
	engine::Ref<engine::ParticleSystem> mDashFx;           // +0x3b0
	engine::Ref<engine::ParticleSystem> mEatFishFx;        // +0x3b4
	engine::Ref<engine::ParticleSystem> mEatBonusFx;       // +0x3b8
	engine::Ref<StunBubbleFx> mStunBubbleFx;               // +0x3bc
	int mChainCount;                    // +0x3c0
	float mChainSeconds;                // +0x3c4
	float mFrenzyTime;                  // +0x3c8
	int mFrenzyLevel;                   // +0x3cc
	std::vector<int> mPendingScores;    // +0x3d0
	float mPendingScoreSeconds;         // +0x3e0
	int mShieldCount;                   // +0x3e4
	float mShieldImmuneSeconds;         // +0x3e8
	FishBase* mShieldEater;             // +0x3ec
	FishBase* mFuryTarget;              // +0x3f0
	int mFuryPhase;                     // +0x3f4
	engine::Point mFuryReturn;          // +0x3f8
	bool mEating;                       // +0x400
	engine::Ref<PlayerShieldFx> mShieldFx;                 // +0x404
};

} // namespace feedingfrenzy

#endif
