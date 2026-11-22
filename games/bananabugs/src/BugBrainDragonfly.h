// BugBrainDragonfly: the dragonfly's brain (0x418880..0x4198c0 in the original; BugBrainDragonFly there). The dragonfly
// hovers on its row, winds up, and darts along the row (to the left or the right, as its picture faces) until three places
// short of the first wall, where it turns round and begins again. Each flight is one track of four keys: a hover (100
// frames), the wind-up (71 frames), the dart (a seventh of the pixels in frames) and a last frame, whose end starts the next
// flight; the body is told what each key is (BugImgDragonfly). It dies when a wall is built on its place, and when a wall
// lies within 62 pixels ahead it turns round at once. It is killed by anything that kills a bug; it is flung off, falls
// and tumbles as the death track.
// Slots this class overrides: Notify (4), Update (5), Setup (14), SetCommand (18), Zap (20), PlaceAt (21), BuildFadeTrack
// (22) and BuildDeathTrack (23); Vanish (17) and TakeCommand (19) are one function in the original that several kinds' vtables
// name: they are in BugShared and each override here calls the shared copy. StepSegment (15) and AvoidRope (16) are the base's.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgDragonfly;
struct FillGridCell;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj). The
// event is heard when the key has begun, so each one starts what the key is for
enum DragonflyBrainEvent
{
	DRAGONFLY_BRAIN_EVENT_IDLE = 2,     // the hover key has begun (the body hovers)
	DRAGONFLY_BRAIN_EVENT_WINDUP = 3,   // the wind-up key has begun (the wind-up sounds)
	DRAGONFLY_BRAIN_EVENT_DART = 4,     // the dart key has begun (the dart sounds)
	DRAGONFLY_BRAIN_EVENT_UNUSED = 5,   // no key has it: the body is told to dart, with no sound
	DRAGONFLY_BRAIN_EVENT_END = 6,      // the last key has begun: the body hovers and the next flight starts
	DRAGONFLY_BRAIN_EVENT_GONE = 7,     // the fade track's end: the bug goes
	DRAGONFLY_BRAIN_EVENT_TUMBLED = 8,  // the death track's tumble key is done
};

class BugBrainDragonfly : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	FillGridCell* mTargetCell;     // +0x338 (the name is a guess): the place the dart goes to, none when the wall is within three places
	int mLastColumn;               // +0x33c (the name is a guess): the column the last flight's scan ended at (-1 at first)
	bool mFarFlight;               // +0x340 (the name is a guess): the dart is longer than 5 (its length over a seventh, times the picture's scale and 1.8); nothing reads it
	int mMoveTrack;                // +0x344: the flight's track (AddTrack's number)
	AnimKey* mIdleKey;             // +0x348 (the name is a guess): the hover, 100 frames (event 2)
	AnimKey* mWindupKey;           // +0x34c (the name is a guess): the wind-up, 71 frames (event 3)
	AnimKey* mDartKey;             // +0x350 (the name is a guess): the dart, whose length the flight sets (event 4)
	AnimKey* mEndKey;              // +0x354 (the name is a guess): the last, a frame (event 6), which holds the place the dart goes to
	AnimKey* mFlingKey;            // +0x35c (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x360 (the name is a guess): its spin and fade-in (event 8)
	AnimKey* mFadeOutKey;          // +0x364 (the name is a guess): the last key, which fades it out and removes it

	BugBrainDragonfly();           // 0x418880
	void Notify(ObjBase* theObj) override;   // 0x418aa0 (slot 4)
	bool Update() override;                  // 0x4189f0 (slot 5)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x418920 (slot 14)
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x418c50 (slot 18)
	int TakeCommand() override;              // 0x418cd0 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x419070 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x418d20 (slot 21)
	void BuildFadeTrack() override;          // 0x419170 (slot 22)
	void BuildDeathTrack() override;         // 0x4194d0 (slot 23)

private:
	void BuildMoveTrack();         // 0x419300 (EBX the bug)
	bool IsBlockedAhead();         // 0x418d40 (EAX the bug): a wall, or the edge of the grid, within 62 pixels along the row
	void ChooseTarget();           // 0x418df0 (the bug pushed): the heading by the picture's scale, and the place the dart goes to
	void BeginFlight();            // 0x418f10 (EAX the bug): turns the picture round, chooses the target and starts the flight
};

} // namespace Sexy
