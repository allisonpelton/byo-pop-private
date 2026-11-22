#ifndef __FEEDINGFRENZY2_FISHAI_H__
#define __FEEDINGFRENZY2_FISHAI_H__

// What a fish is doing: the states of its machine (FishState and the rest, each a State of a FishStateMgr). A fish the
// computer moves (an AIFish) comes in through SpawnAI, which swims it into the play area, then picks between wandering
// (WanderAI) and swimming across (FlyByAI); each looks about now and then and reacts to what it sees. A fish whose
// state says it has gone, out of view after being seen, loses its state, which is what makes it dead to its spawn.
//
// Translated so far: the states the menu's fish reach before they see anything (SpawnAI, WanderAI and FlyByAI). The
// sight check (0x437ac0 (feedingfrenzy2)) and the states it leads to (fleeing, chasing, avoiding, schooling, eating,
// being eaten, puking and leaving) are not yet, and each place that calls one says so.

#include "Geometry.h"
#include "State.h"

#include <string>
#include <vector>

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
	// 0x54: the fish eats the prey (AIFishState's, 0x40ff50 (feedingfrenzy2), not yet translated)

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

#endif
