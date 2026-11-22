// BugBrainAnt: the ant's brain (0x413f90..0x414fa0 in the original; BugBrainAnt). The ant walks along the walls of the
// level's grid, post to post: each step is a track of three keys (the walk to the next post, a turn in place there,
// and the turn's end), at three frames a pixel. It never stops; at a post it takes the first way on (not straight
// back) that the post has a wall for, from the one after its heading, and a column of ants follows its head along
// the same path. When the rope is shot along a line ahead of it, it hurries on (twice as fast) or, if the line is
// ahead of it, turns round and flips its body over to the other side of the wall. It is killed by anything that
// kills a bug; it flies off and falls, tumbling, as the death track.
// Slots this class overrides: Notify (4), Update (5), Setup (14), StepSegment (15), AvoidRope (16), Vanish (17),
// SetCommand (18), TakeCommand (19), Zap (20), PlaceAt (21), BuildFadeTrack (22) and BuildDeathTrack (23). The shared
// ones (Update, StepSegment, Vanish, SetCommand, TakeCommand and BuildFadeTrack) are one function in the original that
// several kinds' vtables name: they are in BugShared, and each override here calls the shared copy.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgAnt;
class PipeObj;

// What mTurnPhase holds
enum AntTurnPhase
{
	ANT_TURN_NONE = 0,       // the ant is on a leg, and a rope may turn it
	ANT_TURN_AT_POST = 1,    // the turn key has ended: the turn's last frame is going
	ANT_TURN_FLIPPING = 2,   // the body is told to flip: the flip track ends it
};

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum AntBrainEvent
{
	ANT_BRAIN_EVENT_LEG_DONE = 2,   // the leg to the next post is walked (Notify does nothing)
	ANT_BRAIN_EVENT_TURN_DONE = 3,  // the turn in place is done
	ANT_BRAIN_EVENT_TURN_END = 4,   // the turn's last key: the next leg starts
	ANT_BRAIN_EVENT_GONE = 5,       // the fade track's end: the bug goes
	ANT_BRAIN_EVENT_TUMBLED = 6,    // the death track's tumble key is done
};

class BugBrainAnt : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mNextHeading;              // +0x338 (the name is a guess): the heading the ant takes at the post it is walking to
	int mUnused33C;                // +0x33c: Zap sets it to 0; nothing reads it
	int mTurnPhase;                // +0x340 (the name is a guess): an AntTurnPhase; while it is not 0 a rope does not turn the ant
	int mMoveTrack;                // +0x344: the step's track (AddTrack's number)
	AnimKey* mLegKey;              // +0x350 (the name is a guess): the walk from where the ant is to the next post (event 2)
	AnimKey* mTurnKey;             // +0x354 (the name is a guess): the turn in place at the post (event 3)
	AnimKey* mTurnEndKey;          // +0x358 (the name is a guess): the turn's last frame (event 4)
	AnimKey* mFlingKey;            // +0x35c (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x360 (the name is a guess): its spin and fade-in (event 6)
	AnimKey* mFadeOutKey;          // +0x364 (the name is a guess): the last key, which fades it out and removes it

	BugBrainAnt();                 // 0x413f90
	void Notify(ObjBase* theObj) override;   // 0x4140b0 (slot 4)
	bool Update() override;                  // 0x41c640 (slot 5): the base's
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x413ff0 (slot 14)
	void StepSegment() override;             // 0x41c620 (slot 15): updates the bug, then its body
	void AvoidRope(int theVertical, float theX, float theY) override; // 0x4141a0 (slot 16)
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x417aa0 (slot 18)
	int TakeCommand() override;              // 0x41e1f0 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x414850 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x4145b0 (slot 21)
	void BuildFadeTrack() override;          // 0x415b60 (slot 22)
	void BuildDeathTrack() override;         // 0x414c30 (slot 23)

	void ClearTurnPhase() { mTurnPhase = ANT_TURN_NONE; } // 0x4145a0 (EAX the bug): the body's flip is done

private:
	void BuildMoveTrack();         // 0x4149e0 (EBX the bug)
	void BeginStep(bool theRandom);// 0x414630 (EAX the bug, theRandom pushed)
	// The keys of a leg to thePost (the code 0x414630 and 0x4141a0 each have a copy of): the walk from where the ant is, the
	// turn there from the heading to the next one, and their lengths
	void SetLegKeys(PipeObj* thePost, double theFramesPerPixel, int theTurnFrames);
	PipeObj* GetNextPost();        // the post the heading leads to from the place the ant is on (the code of 0x414630 and 0x4141a0), or none
};

} // namespace Sexy
