// BugBrainRoach: the roach's brain (0x41c4f0..0x41d0d0 in the original; BugBrainRoach). The roach walks along the walls of
// the level's grid, post to post, like the beetle (BugBrainBeetle) but with no body to flip over to the other side of
// the wall: each step is a track of three keys (the walk to the next post, a turn in place there, and the turn's end).
// At a post it takes the first way on that the post has a wall for, from the one after its heading (at random after the
// first step), and, unlike the beetle, it may go straight back when there is no other way. A leg longer than 128 pixels is
// walked at a quarter of a frame a pixel and eased like a cosine; a shorter one at 1.8 frames a pixel and linearly (both
// plus one frame); the turn takes 12 frames. It does not look at the rope. It is killed by anything that kills a bug; it
// flies off and falls, tumbling, as the death track.
// Slots this class overrides: Notify (4), Update (5), Setup (14), StepSegment (15), Vanish (17), SetCommand (18),
// TakeCommand (19), Zap (20), PlaceAt (21), BuildFadeTrack (22) and BuildDeathTrack (23). The shared ones (Update,
// StepSegment, Vanish, SetCommand, TakeCommand and BuildFadeTrack) are one function in the original that other kinds'
// vtables name too: they are in BugShared, and each override here calls the shared copy.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgRoach;
class PipeObj;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum RoachBrainEvent
{
	ROACH_BRAIN_EVENT_LEG_DONE = 3,   // the leg to the next post is walked (Notify does nothing)
	ROACH_BRAIN_EVENT_TURN_DONE = 4,  // the turn in place is done (Notify does nothing)
	ROACH_BRAIN_EVENT_TURN_END = 5,   // the turn's last key: the next leg starts
	ROACH_BRAIN_EVENT_GONE = 6,       // the fade track's end: the bug goes
	ROACH_BRAIN_EVENT_TUMBLED = 7,    // the death track's tumble key is done
};

class BugBrainRoach : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mNextHeading;              // +0x338 (the name is a guess): the heading the roach takes at the post it is walking to
	int mUnused33C;                // +0x33c: Zap sets it to 0; nothing reads it
	int mMoveTrack;                // +0x340: the step's track (AddTrack's number)
	AnimKey* mLegKey;              // +0x34c (the name is a guess): the walk from where the roach is to the next post (event 3)
	AnimKey* mTurnKey;             // +0x350 (the name is a guess): the turn in place at the post (event 4)
	AnimKey* mTurnEndKey;          // +0x354 (the name is a guess): the turn's last frame (event 5)
	AnimKey* mFlingKey;            // +0x358 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x35c (the name is a guess): its spin and fade-in (event 7)
	AnimKey* mFadeOutKey;          // +0x360 (the name is a guess): the last key, which fades it out and removes it

	BugBrainRoach();               // 0x41c4f0
	void Notify(ObjBase* theObj) override;   // 0x41c650 (slot 4)
	bool Update() override;                  // 0x41c640 (slot 5): the base's
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x41c560 (slot 14)
	void StepSegment() override;             // 0x41c620 (slot 15): updates the bug, then its body
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x416420 (slot 18)
	int TakeCommand() override;              // 0x418cd0 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x41c980 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x41c710 (slot 21)
	void BuildFadeTrack() override;          // 0x418130 (slot 22)
	void BuildDeathTrack() override;         // 0x41cd60 (slot 23)

private:
	void BuildMoveTrack();         // 0x41cb10 (EBX the bug)
	void BeginStep(bool theRandom);// 0x41c790 (EAX the bug, theRandom pushed)
	PipeObj* GetNextPost();        // the post the heading leads to from the place the roach is on (the start of 0x41c790), or none
};

} // namespace Sexy
