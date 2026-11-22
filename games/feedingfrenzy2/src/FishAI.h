#ifndef __FEEDINGFRENZY2_FISHAI_H__
#define __FEEDINGFRENZY2_FISHAI_H__

// What a fish is doing: the states of its machine (FishState and the rest, each a State of a FishStateMgr). A fish the
// computer moves (an AIFish) comes in through SpawnAI, which swims it into the play area, then picks between wandering
// (WanderAI) and swimming across (FlyByAI); each looks about now and then and reacts to what it sees. A fish whose
// state says it has gone, out of view after being seen, loses its state, which is what makes it dead to its spawn.
//
// Translated so far: swimming in, wandering, flying by, the reactions to what a fish sees (FleeAI, ChaseAI and
// AvoidAI), eating and being eaten. Schooling, puking and leaving are not yet, and each place that calls one says so.

#include "Geometry.h"
#include "Object.h"
#include "State.h"
#include "sigslot.h"

#include <string>
#include <vector>

class Actor;
class AIFish;
class Fish;

// A list of times to wait for, one after another (0x28 bytes inside the states; made by 0x421c00 (feedingfrenzy2)):
// each step adds the frame's time, and once the time reaches the next of the list, that one is passed. The states use
// it with one time at a time, starting it over when it passes.
struct Timetable
{
	Timetable() : mOn(false), mNext(0), mTime(0.0f) {}

	// 0x473150 (feedingfrenzy2) with the clock cleared and the timetable on, as each state does after it: the one time
	// waited for, from now
	void start(float theSeconds)
	{
		mTimes.clear();
		mTimes.push_back(theSeconds);
		mNext = 0;
		mTime = 0.0f;
		mOn = true;
	}
	// The same time waited for again, from now
	void restart()
	{
		mNext = 0;
		mTime = 0.0f;
		mOn = true;
	}
	// The frame's time added while it has a time left to wait for; true when the time has reached it, which the
	// caller then passes
	bool advance(float theElapsed)
	{
		if (!mOn || mNext >= mTimes.size())
			return false;
		mTime = (float)((double)mTime + theElapsed);
		return mTimes[mNext] <= mTime;
	}

	bool mOn;                    // +0x00
	std::vector<float> mTimes;   // +0x08
	unsigned int mNext;          // +0x14: the first time not yet passed
	float mTime;                 // +0x18: the seconds since the start
	// +0x20 and +0x24: zeroed by the constructor, read by nothing translated yet
};

// What every state of a fish is (FishState, vtable 0x54fe8c (feedingfrenzy2); its constructor 0x41fed0
// (feedingfrenzy2))
class FishState : public engine::State
{
public:
	explicit FishState(Fish* theFish) : mFish(theFish), mInPlay(false), mEdible(false) {}

	// 0x1c (0x4448a0 (feedingfrenzy2)): the fish has gone, out of view after it was in view; until then its being in
	// view is kept
	virtual bool isGone();
	// 0x20 (0x502587 (feedingfrenzy2)): in the play area, where the stage keeps it
	bool isInPlay() const { return mInPlay; }
	// 0x24 (0x43d890 (feedingfrenzy2)): another fish may eat this one (the eater not looked at)
	bool isEdible() const { return mEdible; }
	// 0x54 (0x5014fa (feedingfrenzy2)): the fish has begun to eat the prey; nothing by default
	virtual void eat(Fish* thePrey) {}
	// 0x58 (0x502bee (feedingfrenzy2)): the eater may have the fish now; always by default
	virtual bool canBeEatenBy(Fish* theEater) { return true; }

protected:
	Fish* mFish;    // +0x0c
	bool mInPlay;   // +0x10
	bool mEdible;   // +0x11
	// +0x12 to +0x1c: the flags read through the slots 0x28 to 0x50 (0x43aa00, 0x50295d, 0x502955, 0x502959,
	// 0x502951, 0x40af90, 0x50294d, 0x44ee40, 0x4a49d0, 0x4a1fd0 and 0x502949 (feedingfrenzy2)) by code not yet
	// translated: left out until it is
};

// A state of a fish the computer moves (AIFishState, vtable 0x54e994 (feedingfrenzy2); its constructor 0x41fe60
// (feedingfrenzy2))
class AIFishState : public FishState
{
public:
	explicit AIFishState(AIFish* theFish);

	// 0x54 (0x40ff50 (feedingfrenzy2)): the fish's state changed to eating (AIEatingState)
	void eat(Fish* thePrey) override;

protected:
	AIFish* mAIFish;  // +0x20
	// +0x28 (a byte) and +0x30 to +0x4c: zeroed by the constructor, read by nothing translated yet
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

#endif
