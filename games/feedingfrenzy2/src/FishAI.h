#ifndef __FEEDINGFRENZY2_FISHAI_H__
#define __FEEDINGFRENZY2_FISHAI_H__

// What a fish is doing: the states of its machine (FishState and the rest, each a State of a FishStateMgr). A fish the
// computer moves (an AIFish) comes in through SpawnAI, which swims it into the play area, then picks between wandering
// (WanderAI) and swimming across (FlyByAI); each looks about now and then and reacts to what it sees. A fish whose
// state says it has gone, out of view after being seen, loses its state, which is what makes it dead to its spawn.
//
// Translated so far: swimming in, wandering, flying by, the reactions to what a fish sees (FleeAI, ChaseAI and
// AvoidAI), eating and being eaten, and schooling (SchoolOfFish and SchoolingAI). Puking and leaving are not yet, and
// each place that calls one says so.

#include "Geometry.h"
#include "Object.h"
#include "State.h"
#include "Timetable.h"
#include "sigslot.h"

#include <cstdint>
#include <string>
#include <vector>

class Actor;
class AIFish;
class Fish;
class FishBehaviorInfo;

// What every state of a fish is (FishState, vtable 0x54fe8c (feedingfrenzy2); its constructor 0x41fed0
// (feedingfrenzy2))
class FishState : public engine::State
{
public:
	explicit FishState(Fish* theFish) : mFish(theFish), mInPlay(false), mEdible(false), mBusy(false) {}

	// 0x1c (0x4448a0 (feedingfrenzy2)): the fish has gone, out of view after it was in view; until then its being in
	// view is kept
	virtual bool isGone();
	// 0x20 (0x502587 (feedingfrenzy2)): in the play area, where the stage keeps it
	bool isInPlay() const { return mInPlay; }
	// 0x24 (0x43d890 (feedingfrenzy2)): another fish may eat this one (the eater not looked at)
	bool isEdible() const { return mEdible; }
	// 0x54 (0x5014fa (feedingfrenzy2)): the fish has begun to eat the prey; nothing by default
	virtual void eat(Actor* thePrey) {}
	// 0x58 (0x502bee (feedingfrenzy2)): the eater may have the fish now; always by default
	virtual bool canBeEatenBy(Fish* theEater) { return true; }
	// 0x50 (0x4a49d0 (feedingfrenzy2)): the state is not to be cut short (the player eating or sucking): the stage's
	// time running out waits for it
	bool isBusy() const { return mBusy; }

protected:
	Fish* mFish;    // +0x0c
	bool mInPlay;   // +0x10
	bool mEdible;   // +0x11
	bool mBusy;     // +0x1a
	// +0x12 to +0x19 and +0x1b to +0x1c: the flags read through the slots 0x28 to 0x4c (0x43aa00, 0x50295d,
	// 0x502955, 0x502959, 0x502951, 0x40af90, 0x50294d, 0x44ee40, 0x4a1fd0 and 0x502949 (feedingfrenzy2)) by code
	// not yet translated: left out until it is
};

// A state of a fish the computer moves (AIFishState, vtable 0x54e994 (feedingfrenzy2); its constructor 0x41fe60
// (feedingfrenzy2))
class AIFishState : public FishState
{
public:
	explicit AIFishState(AIFish* theFish);

	// 0x54 (0x40ff50 (feedingfrenzy2)): the fish's state changed to eating (AIEatingState)
	void eat(Actor* thePrey) override;

protected:
	AIFish* mAIFish;  // +0x20
	// +0x28 (a byte) and +0x30 to +0x4c: zeroed by the constructor, read by nothing translated yet
};

// The fish leaving as the stage ends (EvacuateAI, 0x60 bytes, vtable 0x54eb34 (feedingfrenzy2); its constructor
// 0x4a02c0 (feedingfrenzy2)): out the nearer side of the play area at two and a half times its top speed across, and
// with no state once out of the view. It clears two of the flags AIFishState sets (+0x1b and +0x1c, not yet
// translated).
class EvacuateAI : public AIFishState
{
public:
	explicit EvacuateAI(AIFish* theFish) : AIFishState(theFish) {}

	// 0x04 (0x494870 (feedingfrenzy2)): the fish leaving, and wanting to swim out across the side of the play area
	// nearer it (the left one from the middle)
	void enter() override;
	// 0x0c (0x4282b0 (feedingfrenzy2)): the fish moved; out of the view, no state left
	void update(engine::UpdateContext* theContext) override;
};

// The fish swimming in from where it came in until it is in the play area (SpawnAI, 0x68 bytes, vtable 0x54ea1c
// (feedingfrenzy2); its constructor 0x41fe00 (feedingfrenzy2))
class SpawnAI : public AIFishState
{
public:
	// 0x41fe00 (feedingfrenzy2): edible, not yet in play
	explicit SpawnAI(AIFish* theFish);

	// 0x04 (0x425740 (feedingfrenzy2)): the fish's bounds less 80 at each side, the visual shown and swimming
	void enter() override;
	// 0x08 (0x481790 (feedingfrenzy2)): the fish counts as seen, so leaving the view after it is its going
	void exit() override;
	// 0x0c (0x4454e0 (feedingfrenzy2)): in play once the stage's area holds it; across the side it came from at its
	// top speed while it is outside the narrowed bounds, else its next state picked
	void update(engine::UpdateContext* theContext) override;

private:
	engine::RectF mEntryArea;  // +0x50: the fish's bounds less the margins at the sides
};

// The fish wandering about the play area (WanderAI, 0x118 bytes, vtable 0x54eaa4 (feedingfrenzy2); its constructor
// 0x421c20 (feedingfrenzy2)): every so often it picks a move (a dart, back to the centre, a random way or a turn
// back), by weights from its behaviour, and its next state when its time with this one is up
class WanderAI : public AIFishState
{
public:
	// 0x421c20 (feedingfrenzy2): in play and edible
	explicit WanderAI(AIFish* theFish);

	// 0x04 (0x460d40 (feedingfrenzy2)): the area (the bounds less 30 at the sides and 40 at the top and bottom), the
	// visual shown, the sight's and the state's times started, the moves' weights read, and the first move picked
	void enter() override;
	// 0x08 (0x41eed0 (feedingfrenzy2)): the fish's acceleration put back
	void exit() override;
	// 0x0c (0x4452f0 (feedingfrenzy2)): the fish moved; then the first of the move's, the sight's and the state's
	// times that has come: the next move, a look about, or the next state
	void update(engine::UpdateContext* theContext) override;
	// 0x68 (0x53240f (feedingfrenzy2)): the debug display's line of the move's time and name: not translated, as no
	// display reaches it

private:
	// 0x462530 (feedingfrenzy2): a move picked by the weights and its velocity set, its time started
	void pickMove();

	engine::RectF mArea;          // +0x50
	Timetable mMoveTime;          // +0x60: the move's time
	Timetable mSightTime;         // +0x88: the next look about
	Timetable mStateTime;         // +0xb0: the time with this state
	float mDartWeight;            // +0xd8: "dartFreq"; from the enter on, each weight the sum of those before it too
	float mCenterWeight;          // +0xdc: "centerFreq"
	float mRandomWeight;          // +0xe0: "randomFreq"
	float mReverseWeight;         // +0xe4: "reverseFreq"
	float mWeightTotal;           // +0xe8
	float mAccelRate;             // +0xec: the fish's when the state began, a dart doubling it
	std::string mMoveName;        // +0xf0: the move's name, for the debug display
};

// The fish swimming across the play area to the other side (FlyByAI, 0xb0 bytes, vtable 0x54eccc (feedingfrenzy2);
// its constructor 0x41e910 (feedingfrenzy2))
class FlyByAI : public AIFishState
{
public:
	// 0x41e910 (feedingfrenzy2): in play and edible
	explicit FlyByAI(AIFish* theFish);

	// 0x04 (0x4231b0 (feedingfrenzy2)): across to the side away from where the fish came in, the visual shown, and
	// the sight's time started
	void enter() override;
	// 0x0c (0x446fc0 (feedingfrenzy2)): the fish moved; a new speed when the move's time comes, a look about when the
	// sight's does
	void update(engine::UpdateContext* theContext) override;

private:
	// 0x423c20 (feedingfrenzy2): the fish's top speed across with a random speed up or down, and the move's time
	// started
	void setVelocity();

	bool mRightward;       // +0x50: the fish came in on the left half
	Timetable mMoveTime;   // +0x58
	Timetable mSightTime;  // +0x80
};

// The fish swimming away from an actor that eats it (FleeAI, 0xd8 bytes, vtable 0x54ebbc (feedingfrenzy2); its
// constructor 0x415740 (feedingfrenzy2))
class FleeAI : public AIFishState
{
public:
	// 0x415740 (feedingfrenzy2): in play and edible, the threat kept
	FleeAI(AIFish* theFish, Actor* theThreat);

	// 0x04 (0x4156c0 (feedingfrenzy2)): the visual shown, the sight's and the state's times started, and the first
	// move picked
	void enter() override;
	// 0x0c (0x45f0b0 (feedingfrenzy2)): the fish moved; then the first of the move's, the state's and the sight's
	// times that has come: the next move, the next state, or a look about
	void update(engine::UpdateContext* theContext) override;

private:
	// 0x41af30 (feedingfrenzy2): straight away from the threat at twice the top speeds, the move's time started;
	// the next state when the threat is gone or dead
	void pickMove();

	engine::Ref<Actor> mThreat;  // +0x50
	Timetable mMoveTime;         // +0x58
	Timetable mSightTime;        // +0x80
	Timetable mStateTime;        // +0xa8
};

// The fish swimming after an actor it eats (ChaseAI, 0xd8 bytes, vtable 0x54ec44 (feedingfrenzy2); its constructor
// 0x47d020 (feedingfrenzy2))
class ChaseAI : public AIFishState
{
public:
	// 0x47d020 (feedingfrenzy2): in play and edible, the prey kept
	ChaseAI(AIFish* theFish, Actor* thePrey);

	// 0x04 (0x47d1a0 (feedingfrenzy2)): the visual shown, the sight's and the state's times started, and the first
	// move picked
	void enter() override;
	// 0x0c (0x450580 (feedingfrenzy2)): the fish moved; then the next move when the move's time comes, else the next
	// state when the state's does (the sight's time is started but never looked at)
	void update(engine::UpdateContext* theContext) override;

private:
	// 0x42f820 (feedingfrenzy2): from the mouth straight at the prey at the top speeds, the move's time started; the
	// next state when the prey is gone, dead or eats the fish
	void pickMove();

	engine::Ref<Actor> mPrey;  // +0x50
	Timetable mMoveTime;       // +0x58
	Timetable mStateTime;      // +0x80
	Timetable mSightTime;      // +0xa8
};

// The fish turning aside from an actor that eats it while it heads at it (AvoidAI, 0x88 bytes, vtable 0x54ed54
// (feedingfrenzy2); its constructor 0x48f680 (feedingfrenzy2)). It has no enter, so the fish keeps the way it had.
class AvoidAI : public AIFishState
{
public:
	// 0x48f680 (feedingfrenzy2): in play and edible, the threat kept
	AvoidAI(AIFish* theFish, Actor* theThreat);

	// 0x0c (0x433440 (feedingfrenzy2)): the fish moved; the next state once the threat is dead or out of sight, else
	// a turn aside while the fish heads at it
	void update(engine::UpdateContext* theContext) override;

private:
	// 0x434530 (feedingfrenzy2): the way the fish wants to go (or faces, wanting none) is within about 32 degrees of
	// the line to the threat, either way along it
	bool isHeadedAtThreat();
	// 0x48cf70 (feedingfrenzy2): the wanted velocity turned 30 degrees away from the side the threat is on
	void turnAside();

	engine::Ref<Actor> mThreat;  // +0x50
	// +0x58: a timetable the constructor makes, which nothing starts
};

// A fish eating (EatingState, 0x34 bytes and its virtual base, vtable 0x5500dc (feedingfrenzy2); its constructor
// 0x482210 (feedingfrenzy2)): the visual plays its eat, and at its chomp the prey taken is swallowed
class EatingState : public FishState, public sigslot::HasSlots
{
public:
	// 0x482210 (feedingfrenzy2): in play and edible
	explicit EatingState(Fish* theFish);

	// 0x04 (0x418cb0 (feedingfrenzy2)): the visual's chomp connected and its eat started
	void enter() override;
	// 0x08 (0x41d210 (feedingfrenzy2)): the chomp let go; the prey swallowed now if the chomp never came
	void exit() override;
	// 0x74 (0x40e2f0 (feedingfrenzy2)): the chomp: the prey swallowed
	virtual void onChomp();

protected:
	bool mChomped;  // +0x30
};

// A fish the computer moves eating (AIEatingState, 0x40 bytes, vtable 0x54e864 (feedingfrenzy2); its constructor
// 0x40ff00 (feedingfrenzy2))
class AIEatingState : public EatingState
{
public:
	explicit AIEatingState(AIFish* theFish);

	// 0x0c (0x435170 (feedingfrenzy2)): the fish moved
	void update(engine::UpdateContext* theContext) override;
	// 0x74 (0x40e2c0 (feedingfrenzy2)): the eater's chomp, then the fish's next state
	void onChomp() override;

private:
	AIFish* mAIFish;  // +0x34
};

// A fish being eaten (EatenState, 0x40 bytes, vtable 0x54ff44 (feedingfrenzy2); its constructor 0x480130
// (feedingfrenzy2)): drawn into the eater's mouth and shrunk to nothing over the seconds, then hidden. Neither in
// play nor edible, it never goes by itself: the eater's swallow ends it.
class EatenState : public FishState
{
public:
	EatenState(Fish* theFish, Fish* theEater, float theSeconds)
	    : FishState(theFish), mEater(theEater), mStartScale(0.0f), mElapsed(0.0f), mSeconds(theSeconds)
	{
	}

	// 0x04 (0x47dfb0 (feedingfrenzy2)): where it starts and its scale kept; at once hidden when the seconds are none
	void enter() override;
	// 0x0c (0x457580 (feedingfrenzy2)): the time added; on the way to the mouth by its share of the seconds and shrunk
	// by it, hidden at its end
	void update(engine::UpdateContext* theContext) override;
	// 0x1c (0x502bf3 (feedingfrenzy2))
	bool isGone() override { return false; }

private:
	// The visual hidden at its full scale (0x47dfe9 and 0x4575aa (feedingfrenzy2))
	void hide();

	Fish* mEater;               // +0x20: its eater part
	engine::Vector2f mStart;    // +0x24
	float mStartScale;          // +0x2c: the visual's
	float mElapsed;             // +0x30
	float mSeconds;             // +0x34
};

// A school of fish (SchoolOfFish, 0xa8 bytes, vtable 0x553a74 (feedingfrenzy2); its constructor 0x4101b0
// (feedingfrenzy2)): the fish a school spawn brings in together, their centre and mean velocity, and the goal they
// swim to. A fish taken out of the stage stays on the list.
class SchoolOfFish : public engine::Object
{
public:
	SchoolOfFish();

	// 0x407600 (feedingfrenzy2): the behaviour's distances and factors, whether it flies by, and the goal's time
	// started at once
	void setBehavior(FishBehaviorInfo* theBehavior);
	// 0x41d290 (feedingfrenzy2): the fish joins, the school its own
	void addFish(AIFish* theFish);
	// 0x460400 (feedingfrenzy2): once a frame, from the first of its fish to update: the centre and mean velocity,
	// and a new goal when the goal's time comes or the centre is within 16 of the goal
	void update(engine::UpdateContext* theContext);
	// 0x461d90 (feedingfrenzy2): the school takes fright: for 1.5 to 2 seconds drawn when it was calm, the time
	// started again when it already flees
	void startFleeing();
	// 0x43ada0 (feedingfrenzy2): the fish of the school nearest the one, the first of the nearest; none when it is
	// alone
	AIFish* findNearest(AIFish* theFish);

	const std::vector<AIFish*>& getFish() const { return mFish; }
	const engine::Vector2f& getCenter() const { return mCenter; }
	const engine::Vector2f& getGoal() const { return mGoal; }
	float getSeparationDistance() const { return mSeparationDistance; }
	float getSeparationDistanceSquared() const { return mSeparationDistanceSquared; }
	float getSeparationFactor() const { return mSeparationFactor; }
	float getCohesionFactor() const { return mCohesionFactor; }
	float getAlignmentFactor() const { return mAlignmentFactor; }
	float getGoalFactor() const { return mGoalFactor; }
	float getFleeFactor() const { return mFleeFactor; }
	bool isFleeing() const { return mFleeing; }
	uint64_t getLastFrame() const { return mLastFrame; }
	// 0x4195d5 (feedingfrenzy2): one of the fish that joined eaten by the player; true when it was the last
	bool loseOne() { return --mCount == 0; }

private:
	// 0x437f80 (feedingfrenzy2): the centre and the mean velocity of the fish
	void findCenter();
	// The goal's pick (0x4604a4 to 0x460514 and 0x4e6747 (feedingfrenzy2)): past the far side of the first fish's
	// bounds for a fly-by, else a point drawn inside them with the goal's next time drawn
	void pickGoal();

	std::vector<AIFish*> mFish;                  // +0x0c
	engine::Vector2f mCenter;                    // +0x1c
	engine::Vector2f mAverageVelocity;           // +0x24
	float mSeparationDistance;                   // +0x2c
	float mSeparationDistanceSquared;            // +0x30
	float mNeighborDistance;                     // +0x34
	float mNeighborDistanceSquared;              // +0x38
	float mSeparationFactor;                     // +0x3c
	float mCohesionFactor;                       // +0x40
	float mAlignmentFactor;                      // +0x44
	float mGoalFactor;                           // +0x48
	float mFleeFactor;                           // +0x4c
	float mChaseMultiplier;                      // +0x50
	engine::Vector2f mGoal;                      // +0x54
	Timetable mGoalTime;                         // +0x60: the goal kept until it, or the fright's end
	engine::FloatRange mMoveChangeFrequency;     // +0x88: "schoolMoveChangeFreq"
	bool mFleeing;                               // +0x90
	bool mFlyBy;                                 // +0x91: its behaviour's base is "flyBy"
	int mCount;                                  // +0x94: the fish that joined
	uint64_t mLastFrame;                         // +0x98: the frame it last updated in
};

// A fish swimming with its school (SchoolingAI, 0xb0 bytes, vtable 0x54eddc (feedingfrenzy2); its constructor
// 0x41ab70 (feedingfrenzy2)): every 0.05 to 0.1 seconds the pulls on it worked out again (apart from its nearest,
// along with the others' velocity, towards the centre and the goal, and away from what eats it), their sum pushed on
// it each step
class SchoolingAI : public AIFishState
{
public:
	// 0x41ab70 (feedingfrenzy2): in play and edible
	explicit SchoolingAI(AIFish* theFish);

	// 0x04 (0x41a800 (feedingfrenzy2)): the pulls' time started at none, so the first update works them out
	void enter() override;
	// 0x0c (0x445610 (feedingfrenzy2)): the next state once the school is the fish alone; else the school's update
	// once a frame, the pulls worked out when their time comes, their sum pushed on the fish, and the fish moved
	void update(engine::UpdateContext* theContext) override;

private:
	// 0x43a6c0 (feedingfrenzy2): away from the nearest fish within the separation distance, harder the nearer
	engine::Vector2f findSeparation();
	// 0x43a7a0 (feedingfrenzy2): the others' mean velocity less the fish's, by the alignment factor
	engine::Vector2f findAlignment();
	// 0x43bbb0 (feedingfrenzy2): away from each actor in sight that eats the fish, harder the nearer
	engine::Vector2f findFlee();

	Timetable mPullTime;            // +0x50
	engine::Vector2f mAlignment;    // +0x78
	engine::Vector2f mCohesion;     // +0x80
	engine::Vector2f mSeparation;   // +0x88
	engine::Vector2f mGoalPull;     // +0x90
	engine::Vector2f mFlee;         // +0x98
	engine::Vector2f mForce;        // +0xa0: the sum
};

#endif
