// BugBrainBee: the bee's brain (0x414fa0..0x416220 in the original; BugBrainBee, its body "Bee" and the picture "wasp").
// The bee flies in straight lines over the whole field, a pixel or more a frame, and bounces off the walls: each frame it
// looks 31 pixels ahead along its speed and, when a wall is in the way, reverses the speed across it (a post: one of the
// two at random); a wall within the first eight pixels crushes it. Its pace is a cycle of states: it cruises (700 to 900 frames
// the first time) at one pixel a frame and a boost that is 3 after each charge and decays by 5% a frame, then buzzes (a sound)
// and winds up for 200 frames, its picture tilting a little more each frame, then charges at five pixels a frame for 450
// frames with the picture held tilted, and cruises again for 900 to 1000 frames.
// A freeze winds it up for 100 frames and holds it still (its speed 0, the picture tilted) for the rest of the freeze (at
// least 50 frames), after which it cruises again for 800 to 1100 frames. A daze and a shock are the body's (the stars and
// the ring). It is killed by anything that kills a bug; it is flung off, falls and tumbles as the death track.
// The bee's picture faces the way it goes along the field (it is flipped across when the bee has gone to the left).
// Slots this class overrides: Notify (4), Update (5), Setup (14), SetCommand (18), TakeCommand (19), Zap (20), PlaceAt (21) and
// BuildDeathTrack (23); Vanish (17) is the one function in the original that several kinds' vtables name, and BuildFadeTrack
// (22) is 0x415b60, the one the roach and others have with their own end event: both are in BugShared and each override here
// calls the shared copy. StepSegment (15) and AvoidRope (16) are the base's.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgBee;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum BeeBrainEvent
{
	BEE_BRAIN_EVENT_GONE = 5,           // the fade track's end: the bug goes
	BEE_BRAIN_EVENT_TUMBLED = 6,        // the death track's tumble key is done
};

// What the bee is doing (mBeeState; the names are guesses)
enum BeeState
{
	BEE_STATE_CRUISE = 0,               // flies at one pixel a frame and the boost; any state not listed is this
	BEE_STATE_WIND_UP = 1,              // the picture tilts a little more each frame
	BEE_STATE_FREEZE_WIND_UP = 2,       // the same, begun by a freeze
	BEE_STATE_CHARGE = 3,               // flies at five pixels a frame, the picture tilted
	BEE_STATE_FROZEN = 4,               // held still by a freeze, the picture tilted
};

// What the look ahead found (CheckAhead's answer), which Bounce acts on
enum BeeBounce
{
	BEE_BOUNCE_NONE = 0,
	BEE_BOUNCE_FLIP_Y = 1,              // reverses mMoveKey's Y speed: the place ahead is a wall across, or past the left or right edge (see CheckAhead)
	BEE_BOUNCE_FLIP_X = 2,              // reverses mMoveKey's X speed: a wall down, or past the top or bottom edge
	BEE_BOUNCE_FLIP_ONE = 3,            // a post: one of the two, at random
};

class BugBrainBee : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mMoveTrack;                // +0x33c: the movement's track (AddTrack's number): restarted at each bounce
	AnimKey* mMoveKey;             // +0x340: 999 frames that add the key's X and Y values to the place every frame: those two values are the bee's speed along each side
	AnimKey* mMoveEndKey;          // +0x344: a frame that adds nothing
	AnimKey* mFlingKey;            // +0x34c: the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x350 (the name is a guess): its spin and fade-in (event 6)
	AnimKey* mFadeOutKey;          // +0x354 (the name is a guess): the last key, which fades it out and removes it
	int mStateFrames;              // +0x35c (the name is a guess): updates left in this state (the cruise's, before the wind-up)
	int mBeeState;                 // +0x360 (the name is a guess): a BeeState
	float mTilt;                   // +0x364 (the name is a guess): how far the picture is tilted, in degrees, growing by 0.12 a frame in a wind-up
	float mBoost;                  // +0x368 (the name is a guess): what is added to a speed of one in a cruise: 3 at the start, 5% less a frame
	float mPrevX;                  // +0x36c (the name is a guess): the place across at the end of the last update, which says which way it is going
	int mFrozenFrames;             // +0x370 (the name is a guess): frames a freeze holds it still after the wind-up: the freeze's less 100, at least 50

	BugBrainBee();                 // 0x414fa0
	void Notify(ObjBase* theObj) override;   // 0x415650 (slot 4)
	bool Update() override;                  // 0x415180 (slot 5)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x4150b0 (slot 14)
	void Vanish() override;                  // 0x415810 (slot 17; shared)
	bool SetCommand(int theCommand, int theFrames) override; // 0x4156d0 (slot 18)
	int TakeCommand() override;              // 0x415760 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x415a60 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x415830 (slot 21)
	void BuildFadeTrack() override;          // 0x415b60 (slot 22; shared)
	void BuildDeathTrack() override;         // 0x415e30 (slot 23)

private:
	void BuildMoveTrack();         // 0x415cf0 (EBX the bug)
	int CheckAhead();              // 0x415860 (EDI the bug): a BeeBounce
	void Bounce(int theSide);      // 0x415990 (EAX a BeeBounce, ECX the bug)
	void SetSpeed(float theSpeed); // 0x415a20 (EAX the bug, theSpeed pushed)
	void ApplyTilt();              // the original's code at each place that turns the picture by mTilt (it is the same each time)
};

} // namespace Sexy
