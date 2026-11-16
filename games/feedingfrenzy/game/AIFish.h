#ifndef __FEEDINGFRENZY_AIFISH_H__
#define __FEEDINGFRENZY_AIFISH_H__

// A fish the computer moves (AIFish, 0x388 bytes, constructor 0x402190, vtable 0x49e290, destructor 0x401000). It has
// a state, the part of its life it is in, and a handler for each state that sets where it wants to go; the fish's
// velocity follows (FishBase). It looks at the fish around it and, by what the fish's file says of each kind, avoids,
// chases or runs from it. The fish of the game derive from it.

#include "FishBase.h"
#include "ImageNode.h"
#include "Particles.h"

namespace feedingfrenzy
{

class FishSchool;
class FishSpawner;

// What an AIFish is doing (+0x300)
enum AIState
{
	AI_ENTER = 0,           // the fish swims in from the edge it was put at
	AI_AVOID = 1,           // it turns aside from a fish that comes at it
	AI_CRUISE = 2,          // it swims on a heading, changing its depth now and then
	AI_CHASE = 3,           // it swims at a fish it can eat
	AI_WANDER = 4,          // it swims on a random heading
	AI_WANDER_UNUSED = 5,   // no fish is put in it; it is handled like AI_WANDER
	AI_RUN = 6,             // it swims away from a fish
	AI_SPECIAL = 7,         // the state a "special" reaction starts (a kind of fish gives it its own)
	AI_STUNNED = 9,
	AI_SCHOOL = 10,         // it swims with its school
	AI_FROZEN = 11,         // it holds still (the player's frenzy bonus)
	AI_ALERT = 12,          // it has noticed a fish and turns towards it, showing its thought bubble
	AI_ALERT_RETURN = 13,   // it goes back to what it was doing after its alert
	AI_UNUSED_14 = 14,      // no fish is put in it
	AI_LEAVE = 15,          // it swims away until it is off the screen
	AI_BEING_EATEN = 16,
	AI_NONE = 17            // no state (the last state the fish was in, before the first frame)
};

// How a fish reacts to a kind of fish (the fish's file gives each as <react class= reaction= reactFreq=>)
enum Reaction
{
	REACTION_NONE = 0,
	REACTION_AVOID = 1,
	REACTION_CHASE = 3,
	REACTION_RUN = 6,
	REACTION_SPECIAL = 7
};

class AIFish : public FishBase
{
public:
	AIFish();            // 0x402190
	~AIFish() override;  // 0x401000

	// slot 0x7c: 0x4010a0: a frame of a fish that is in the stage: the state's handler, then the reactions
	void update(engine::UpdateInfo& theInfo) override;

	// slot 0xec: 0x404500: the stun effect and the thought bubble's picture
	void loadEffects() override;

	// slots 0x114 to 0x11c: 0x401b90, 0x401bc0 and 0x401bf0: the ranges, in seconds, of how often the fish picks a new
	// heading (the first two) and how long it stays in the state (the last two), when it wanders, chases and runs
	void setIdleRanges(float theDecisionLow, float theDecisionHigh, float theStateLow, float theStateHigh) override;
	void setChaseRanges(float theDecisionLow, float theDecisionHigh, float theStateLow, float theStateHigh) override;
	void setFleeRanges(float theDecisionLow, float theDecisionHigh, float theStateLow, float theStateHigh) override;
	// slot 0x120: 0x401c20 (the fish's "sightCheckFreq"; nothing reads it)
	void setSightCheckRange(float theRange) override { mSightCheckRange = theRange; }

	bool canEat(FishBase* theVictim) override;               // slot 0x138: 0x401a50
	bool canBeEatenByLevel(float theEaterLevel) override;    // slot 0x13c: 0x401a80
	bool canBeEatenBy(FishBase* theEater) override;          // slot 0x140: 0x401ab0
	void startBeingEaten(FishBase* theEater, float theSeconds) override;  // slot 0x144: 0x403fb0
	bool finishBeingEaten(FishBase* theEater) override;      // slot 0x148: 0x404080
	void eat(FishBase* theVictim) override;                  // slot 0x160: 0x404b20
	void finishEating() override;                            // slot 0x164: 0x404320
	void putInPlay() override;                               // slot 0x17c: 0x403e30
	void takeOutOfPlay() override;                           // slot 0x188: 0x404670
	void stun(float theSeconds, FishBase* theSource, bool theShowEffect) override;  // slot 0x1bc: 0x404200
	void leave() override { beginLeave(); }                  // slot 0x1f4: 0x401d60
	void alert(FishBase* theSubject, float theX, float theY) override;  // slot 0x1fc: 0x402970
	void endAlert() override;                                // slot 0x200: 0x401d70
	bool isAlerted() override { return mState == AI_ALERT; }  // slot 0x204: 0x401e40

	// slot 0x2c0: 0x401b20: the spawner that made the fish, which holds what the fish's file says, among it how the fish reacts to each kind of fish
	virtual void setSpawner(const FishSpawner* theSpawner) { mSpawner = theSpawner; }

	// The handler of each state, by the slot that the state's number picks
	virtual void updateEnter(engine::UpdateInfo& theInfo);        // slot 0x248: 0x401750
	virtual void updateCruise(engine::UpdateInfo& theInfo);       // slot 0x24c: 0x401300
	virtual void updateChase(engine::UpdateInfo& theInfo);        // slot 0x250: 0x4013c0
	virtual void updateWander(engine::UpdateInfo& theInfo);       // slot 0x254: 0x401260
	virtual void updateRun(engine::UpdateInfo& theInfo);          // slot 0x25c: 0x401590
	virtual void updateSpecial(engine::UpdateInfo& theInfo);      // slot 0x260: 0x401830
	virtual void updateAvoid(engine::UpdateInfo& theInfo);        // slot 0x264: 0x402710
	virtual void updateStunned(engine::UpdateInfo& theInfo);      // slot 0x268: 0x401b30
	virtual void updateSchool(engine::UpdateInfo& theInfo);       // slot 0x26c: 0x402fb0
	virtual void updateLeaving(engine::UpdateInfo& theInfo);      // slot 0x270: 0x401d30
	virtual void updateAlert(engine::UpdateInfo& theInfo);        // slot 0x274: 0x402a60
	virtual void updateFrozen(engine::UpdateInfo& theInfo);       // slot 0x278: 0x401f90
	virtual void updateAlertReturn(engine::UpdateInfo& theInfo);  // slot 0x27c: 0x401de0
	// slot 0x280: 0x403cd0: every tenth of a second, the fish looks at the others and reacts to the first it can
	virtual void checkReactions();
	// slot 0x284: 0x401850: the fish has done what it was doing and picks what to do next
	virtual void decide();
	// The states a fish begins
	virtual void beginWander();   // slot 0x288: 0x401940
	virtual void beginCruise();   // slot 0x28c: 0x401880 (a school's fish swim in the school instead)
	virtual void beginRun();      // slot 0x290: 0x4019d0
	virtual void beginChase();    // slot 0x294: 0x401990
	virtual void beginSpecial();  // slot 0x298: 0x40d8a0
	virtual void beginAvoid();    // slot 0x29c: 0x401a10
	virtual void beginSchool();   // slot 0x2a0: 0x401c30
	virtual void beginLeave();    // slot 0x2a4: 0x401c70: it swims off by the nearer side
	// slot 0x2a8: 0x402330: a random heading that keeps the fish in the stage
	virtual void pickWanderHeading();
	// slot 0x2b4: 0x401570, slot 0x2b8: 0x402540: whether the fish sees the player, or a fish: it is within its sight and
	// the heading it swims in is within the angle of it
	virtual bool canSeePlayer();
	virtual bool canSee(FishBase* theFish);
	// slot 0x2c4: 0x403550: the fish reacts to another by what the spawner says of the kind (by chance), and tells whether it did
	virtual bool react(FishBase* theFish);
	// slot 0x2c8: 0x401d90: the fish stops its alert
	virtual void clearAlert();
	// slot 0x2cc: 0x402cb0: the fish holds still, and slot 0x2d0: 0x401fb0: swims on as it did
	virtual void freeze();
	virtual void unfreeze();
	// slot 0x2d4: 0x401e50: the thought bubble grows while the fish waits for the one it has noticed
	virtual void updateThoughtBubble(float theSeconds);

	float mStateSeconds;       // +0x2e8: how long is left of the state
	float mDecisionSeconds;    // +0x2ec: how long is left until the fish picks a new heading (a negative one: not set)
	float mReactionSeconds;    // +0x2f0: how long is left until the fish looks at the others again
	float mStunSeconds;        // +0x2f4
	float mSecondsInState;     // +0x2f8: how long the fish has been in its state
	int mLastState;            // +0x2fc: the state of the last frame
	int mState;                // +0x300: an AIState
	const FishSpawner* mSpawner;  // +0x30c
	engine::Point mWanderDecisionRange;  // +0x310: how often it picks a new heading when it wanders
	engine::Point mWanderStateRange;     // +0x318: how long it stays wandering
	engine::Point mChaseDecisionRange;   // +0x320
	engine::Point mChaseStateRange;      // +0x328
	engine::Point mRunDecisionRange;     // +0x330
	engine::Point mRunStateRange;        // +0x338
	float mSightCheckRange;    // +0x340: set, never read
	FishBase* mTarget;         // +0x344: the fish it avoids, chases or runs from
	engine::ParticleSystem* mStunEffect;  // +0x348 (the fish's child)
	float mLungeFactor;        // +0x34c: how much faster the fish swims while its mouth is open on a victim
	FishBase* mAlertSubject;   // +0x354: the fish it has noticed
	engine::Point mAlertPoint; // +0x358: where it was told to look
	float mSavedSlowDown;      // +0x360: how fast the fish slows, which an alert, a freeze and a stun change
	int mReturnState;          // +0x364: the state it goes back to after an alert
	float mReturnSeconds;      // +0x368
	engine::ImageNode* mThoughtNode;  // +0x36c (the fish's child)
	int mSavedState;           // +0x378: the state it goes back to after a freeze
	engine::Point mSavedVelocity;     // +0x37c: the velocity it takes up again
	float mLeafSeconds;        // +0x384: a timer the kinds of fish use
};

} // namespace feedingfrenzy

#endif
