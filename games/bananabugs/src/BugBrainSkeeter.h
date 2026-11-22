// BugBrainSkeeter: the skeeter's brain (0x41def0..0x41ee10 in the original; BugBrainSkeeter). The skeeter flies free of
// the walls: it looks round in every tenth of a turn for the direction in which it can go farthest (a ray a pixel at a
// time, up to 1400 pixels, that ends at the first wall or the edge of the grid), flies to the last place that ray
// reached over 0.7 frames a pixel, waits 70 frames there, and chooses again. It looks 41 pixels ahead each frame and
// chooses again at once when a wall or a cut place is in the way. It mirrors its picture to the way it is flying. It is
// killed by anything that kills a bug; it flies off and falls, spinning, as the death track.
// Slots this class overrides: Notify (4), Update (5), Setup (14), Zap (20), PlaceAt (21) and BuildDeathTrack (23); Vanish
// (17), SetCommand (18), TakeCommand (19) and BuildFadeTrack (22) are one function in the original that several kinds'
// vtables name: they are in BugShared and each override here calls the shared copy. StepSegment (15) and AvoidRope (16)
// are the base's.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgSkeeter;
struct FillGridCell;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum SkeeterBrainEvent
{
	SKEETER_BRAIN_EVENT_LEG_DONE = 2,   // the flight to the chosen place is done (Notify does nothing)
	SKEETER_BRAIN_EVENT_WAIT_DONE = 3,  // the wait is done (Notify does nothing)
	SKEETER_BRAIN_EVENT_CHOOSE = 4,     // the track's end: it chooses where to fly next
	SKEETER_BRAIN_EVENT_GONE = 5,       // the fade track's end: the bug goes
	SKEETER_BRAIN_EVENT_TUMBLED = 6,    // the death track's tumble key is done
};

class BugBrainSkeeter : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	FillGridCell* mTargetCell;     // +0x338 (the name is a guess): the place the ray last reached, which the flight goes to; none when the ray was blocked at once
	float mUnused33C;              // +0x33c: set to 0 by the constructor; nothing reads it
	int mMoveTrack;                // +0x340: the flight's track (AddTrack's number)
	AnimKey* mLegKey;              // +0x344 (the name is a guess): the flight from where the skeeter is to the chosen place (event 2)
	AnimKey* mWaitKey;             // +0x348 (the name is a guess): the wait at the place, 70 frames (event 3)
	AnimKey* mEndKey;              // +0x34c (the name is a guess): the last, no longer than a frame (event 4)
	AnimKey* mFlingKey;            // +0x350 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x354 (the name is a guess): its spin and fade-in (event 6)
	AnimKey* mFadeOutKey;          // +0x358 (the name is a guess): the last key, which fades it out and removes it
	float mFlightAngle;            // +0x35c (the name is a guess): the angle (in radians, up is a quarter turn) of the ray the skeeter looks along
	float mFacing;                 // +0x360 (the name is a guess): 1 or -1: the way the skeeter was last seen flying, which mFacingFrames counts
	int mFacingFrames;             // +0x364 (the name is a guess): the frames in a row the whole part of its speed was mFacing
	float mFacingScale;            // +0x368 (the name is a guess): the width scale of the brain: 1 or -1, which mirrors the picture

	BugBrainSkeeter();             // 0x41def0
	void Notify(ObjBase* theObj) override;   // 0x41e140 (slot 4)
	bool Update() override;                  // 0x41e060 (slot 5)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x41df90 (slot 14)
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x417aa0 (slot 18)
	int TakeCommand() override;              // 0x41e1f0 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x41e720 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x41e260 (slot 21)
	void BuildFadeTrack() override;          // 0x415b60 (slot 22)
	void BuildDeathTrack() override;         // 0x41ea20 (slot 23)

private:
	void BuildMoveTrack();         // 0x41e820 (EBX the bug)
	bool IsBlockedAhead();         // 0x41e280 (EAX the bug): a wall or a cut place within 41 pixels along the ray
	float ScanRay(float theAngle, float theX, float theY); // 0x41e370 (EDI the bug, the three pushed): how far the ray goes; mTargetCell is where it ends
	float ChooseDirection();       // 0x41e4a0 (EAX the bug): the ray of the 36 that goes farthest; its length
	void BeginMove();              // 0x41e570 (EAX the bug): chooses the direction and starts the flight there
};

} // namespace Sexy
