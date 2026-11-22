// BugBrainCaterpillar: the caterpillar's brain (0x416ea0..0x417840 in the original; BugBrainCaterpillar). A caterpillar is a
// column of eighteen bugs (a head and seventeen bodies, each its own brain with its own body, BugImgCaterpillar): the level
// makes the head, then each body at the head's place after the bugs made before it have walked on for 28 frames, so the
// bodies stand in a line along the wall. Every one of them walks the walls of the level's grid post to post, like the beetle
// (BugBrainBeetle): each step is a track of three keys (the walk to the next post, a turn in place there, and the turn's
// end), at 3.5 frames a pixel. At a post it takes the first way on (not straight back) that the post has a wall for, from
// the one after its heading, and never a random one, so every bug of the column takes the way the one before it took. It
// does not look at the rope. Killing the head kills the column (BugObj::Zap); killing a body holds the ones behind it,
// daze-held for good. Each one that dies is flung off and falls, tumbling, as the death track, and asks for a splat sound.
// Slots this class overrides: Notify (4), Setup (14), StepSegment (15), Vanish (17), SetCommand (18), TakeCommand (19),
// Zap (20), PlaceAt (21), BuildFadeTrack (22) and BuildDeathTrack (23); Update (5) is the base's. StepSegment, Vanish,
// SetCommand, TakeCommand and BuildFadeTrack are one function in the original that several kinds' vtables name: they are in
// BugShared, and each override here calls the shared copy. The step's track is the beetle's too (BugSharedBuildMoveTrack).
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgCaterpillar;
class PipeObj;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum CaterpillarBrainEvent
{
	CATERPILLAR_BRAIN_EVENT_LEG_DONE = 2,   // the leg to the next post is walked (Notify does nothing)
	CATERPILLAR_BRAIN_EVENT_TURN_DONE = 3,  // the turn in place is done (Notify does nothing)
	CATERPILLAR_BRAIN_EVENT_TURN_END = 4,   // the turn's last key: the next leg starts
	CATERPILLAR_BRAIN_EVENT_GONE = 5,       // the fade track's end: the bug goes
	CATERPILLAR_BRAIN_EVENT_TUMBLED = 6,    // the death track's tumble key is done
};

class BugBrainCaterpillar : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object (the same places as the beetle's)
	int mNextHeading;              // +0x338 (the name is a guess): the heading the caterpillar takes at the post it is walking to
	int mUnused33C;                // +0x33c: Zap sets it to 0; nothing reads it
	int mMoveTrack;                // +0x340: the step's track (AddTrack's number)
	AnimKey* mLegKey;              // +0x34c (the name is a guess): the walk from where the caterpillar is to the next post (event 2)
	AnimKey* mTurnKey;             // +0x350 (the name is a guess): the turn in place at the post (event 3)
	AnimKey* mTurnEndKey;          // +0x354 (the name is a guess): the turn's last frame (event 4)
	AnimKey* mFlingKey;            // +0x358 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x35c (the name is a guess): its spin and fade-in (event 6)
	AnimKey* mFadeOutKey;          // +0x360 (the name is a guess): the last key, which fades it out and removes it

	BugBrainCaterpillar();         // 0x416ea0
	void Notify(ObjBase* theObj) override;   // 0x416fd0 (slot 4)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x416f10 (slot 14)
	void StepSegment() override;             // 0x41c620 (slot 15): updates the bug, then its body
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x417aa0 (slot 18; shared)
	int TakeCommand() override;              // 0x41e1f0 (slot 19; shared)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x417320 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x417090 (slot 21)
	void BuildFadeTrack() override;          // 0x415b60 (slot 22)
	void BuildDeathTrack() override;         // 0x4174d0 (slot 23)

private:
	void BeginStep();              // 0x417110 (EAX the bug)
	PipeObj* GetNextPost();        // the post the heading leads to from the place the caterpillar is on (the start of 0x417110), or none
};

} // namespace Sexy
