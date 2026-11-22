// BugBrainRolly: the roly-poly's brain (0x41d0d0..0x41def0 in the original; BugBrainRolly). The roly-poly walks along the
// walls of the level's grid, post to post, like the roach (BugBrainRoach), but it does not walk on: at the end of a leg it
// stops and curls into a ball (the body is told to curl: 0x6b), stays 200 frames, bursts a puff and sounds (event 5), then
// rolls four legs at 0.18 frames a pixel with a turn of 3 frames at each post, uncurls (0x6c) and walks one leg at 3
// frames a pixel with a turn of 15 frames, and curls again. It avoids going straight back where it can, as the beetle
// does. It does not look at the rope. It is killed by anything that kills
// a bug; it flies off and falls, tumbling, as the death track.
// Slots this class overrides: Notify (4), Update (5), Setup (14), StepSegment (15), Vanish (17), SetCommand (18),
// TakeCommand (19), Zap (20), PlaceAt (21), BuildFadeTrack (22) and BuildDeathTrack (23). The shared ones (Update,
// StepSegment, Vanish, SetCommand, TakeCommand and BuildFadeTrack) are one function in the original that other kinds'
// vtables name too: they are in BugShared, and each override here calls the shared copy.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgRolly;
class PipeObj;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum RollyBrainEvent
{
	ROLLY_BRAIN_EVENT_LEG_DONE = 2,   // the leg to the next post is walked (Notify does nothing)
	ROLLY_BRAIN_EVENT_TURN_DONE = 3,  // the turn in place is done (Notify does nothing)
	ROLLY_BRAIN_EVENT_TURN_END = 4,   // the turn's last key: the next leg (or the curl) starts
	ROLLY_BRAIN_EVENT_CURLED = 5,     // the 200 frames curled are over: the first leg of the roll starts, with a puff and a sound
	ROLLY_BRAIN_EVENT_GONE = 6,       // the fade track's end: the bug goes
	ROLLY_BRAIN_EVENT_TUMBLED = 7,    // the death track's tumble key is done
};

class BugBrainRolly : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mRolling;                  // +0x338 (the name is a guess): the roly-poly has curled and is rolling its legs (the body is a ball)
	int mNextHeading;              // +0x33c (the name is a guess): the heading it takes at the post it is walking to
	int mUnused340;                // +0x340: Zap sets it to 0; nothing reads it
	float mFramesPerPixel;         // +0x344 (the name is a guess): 3 walking, 0.18 rolling: the frames a pixel of a leg takes
	int mMoveTrack;                // +0x348: the step's track (AddTrack's number)
	int mCurlTrack;                // +0x34c (the name is a guess): 200 frames, then a frame raising event 5: the time spent curled
	AnimKey* mLegKey;              // +0x358 (the name is a guess): the walk from where it is to the next post (event 2)
	AnimKey* mTurnKey;             // +0x35c (the name is a guess): the turn in place at the post (event 3)
	AnimKey* mTurnEndKey;          // +0x360 (the name is a guess): the turn's last frame (event 4)
	AnimKey* mFlingKey;            // +0x364 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x368 (the name is a guess): its spin and fade-in (event 7)
	AnimKey* mFadeOutKey;          // +0x36c (the name is a guess): the last key, which fades it out and removes it
	int mRollLegsLeft;             // +0x374 (the name is a guess): the legs of the roll still to come, counted down from 4 as each starts

	BugBrainRolly();               // 0x41d0d0
	void Notify(ObjBase* theObj) override;   // 0x41d290 (slot 4)
	bool Update() override;                  // 0x41c640 (slot 5): the base's
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x41d1c0 (slot 14)
	void StepSegment() override;             // 0x41c620 (slot 15): updates the bug, then its body
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x416420 (slot 18)
	int TakeCommand() override;              // 0x418cd0 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x41d700 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x41d410 (slot 21)
	void BuildFadeTrack() override;          // 0x418130 (slot 22)
	void BuildDeathTrack() override;         // 0x41db80 (slot 23)

private:
	void BuildMoveTrack();         // 0x41d890 (EBX the bug)
	void BuildCurlTrack();         // 0x41dae0 (EBX the bug)
	void BeginStep(bool theRandom);// 0x41d490 (EAX the bug, theRandom pushed)
	PipeObj* GetNextPost();        // the post the heading leads to from the place it is on (in 0x41d490), or none
};

} // namespace Sexy
