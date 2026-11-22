// BugBrainFly: the fly's brain (0x4198c0..0x41af60 in the original, with the flight paths 0x41aee0; BugBrainFly). The fly
// flies free of the walls, a short flight at a time: it takes the way it last went turned a little (a tenth of a turn
// either side at most), looks along that ray for a wall or a cut place, and when the ray is at least 70 pixels long flies
// 62 to 92 pixels along it over 15 to 40 frames (its picture turned to face the way, the speed easing from 0.8 to 0.6 a
// frame); when it is not it turns on the spot. It looks 70 pixels ahead each frame and turns right round (half a turn, and
// the wiggle) when a wall or a cut place is in the way. When the monkey moves fast and comes within 124 pixels it flees: it
// flies off along the path read from "images/path_fly" (its place when it begins and the path's points, over as many frames
// as an eighth of the points and some at random), lands where the path ends, and flies again. It is killed by anything that
// kills a bug; it is flung off, falls and tumbles as the death track. A fly whose place is cut, or on a wall, dies on the spot.
// Slots this class overrides: Notify (4), Update (5), Setup (14), TakeCommand (19), Zap (20), PlaceAt (21) and BuildDeathTrack (23);
// Vanish (17), SetCommand (18) and BuildFadeTrack (22; 0x41b990, the moth's too) are one function in the original that
// several kinds' vtables name: they are in BugShared and each override here calls the shared copy. StepSegment (15) and
// AvoidRope (16) are the base's.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgFly;
class PathTrace;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum FlyBrainEvent
{
	FLY_BRAIN_EVENT_PATH_BEGUN = 3,     // the leave track's first key has begun (nothing hears it)
	FLY_BRAIN_EVENT_LANDING = 4,        // the leave track's second key has begun: the fly may be hit again
	FLY_BRAIN_EVENT_LEFT = 5,           // the leave track's end: it has landed, and flies again
	FLY_BRAIN_EVENT_UNUSED_6 = 6,       // no key has it (nothing hears it)
	FLY_BRAIN_EVENT_FLIGHT_DONE = 7,    // the flight's end: it chooses again
	FLY_BRAIN_EVENT_GONE = 8,           // the fade track's end: the bug goes
	FLY_BRAIN_EVENT_TUMBLED = 9,        // the death track's tumble key is done
};

// What the fly is doing (mFlyState)
enum FlyState
{
	FLY_STATE_FLYING = 0,               // flies short flights
	FLY_STATE_FLEEING = 1,              // on the path of its flight from the monkey
	FLY_STATE_LANDED = 2,               // the path is done: the next frame it is flying
};

class BugBrainFly : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mUnused338;                // +0x338: set to 0 by the constructor and when it flees; nothing reads it
	int mPathIndex;                // +0x33c (the name is a guess): which of the two paths it flees along (0 always: nothing else sets it)
	float mLeaveFromX;             // +0x340 (the name is a guess): where it was when it began to flee
	float mLeaveFromY;             // +0x344 (the name is a guess)
	float mLeaveFromRotation;      // +0x348 (the name is a guess): and how it was turned
	bool mUnused350;               // +0x350: set to 1 by the constructor; nothing reads it
	int mFlyState;                 // +0x354 (the name is a guess): a FlyState
	float mUnused358;              // +0x358: set to 0 by the constructor; nothing reads it
	int mFlightTrack;              // +0x35c: the flight's track (AddTrack's number)
	int mLeaveTrack;               // +0x360 (the name is a guess): the fleeing along the path
	AnimKey* mFlightKey;           // +0x364 (the name is a guess): the flight: 15 to 40 frames, from where the fly is, easing its speed from 0.8 to 0.6 a frame along the angle
	AnimKey* mFlightEndKey;        // +0x368 (the name is a guess): the last, a frame (event 7), which holds the place the flight goes to
	AnimKey* mPathKey;             // +0x370 (the name is a guess): the path's first key: the path's points, as the PATH channel moves along them (event 3)
	AnimKey* mLandKey;             // +0x374 (the name is a guess): from the path's last point to where the fly began, 20 frames (event 4)
	AnimKey* mLeaveEndKey;         // +0x378 (the name is a guess): the last, a frame (event 5)
	AnimKey* mFlingKey;            // +0x37c (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x380 (the name is a guess): its spin and fade-in (event 9)
	AnimKey* mFadeOutKey;          // +0x384 (the name is a guess): the last key, which fades it out and removes it
	float mFlightAngle;            // +0x388 (the name is a guess): the angle (in radians, up is a quarter turn) of the last flight, which the ray looks along

	BugBrainFly();                 // 0x4198c0
	void Notify(ObjBase* theObj) override;   // 0x419bb0 (slot 4)
	bool Update() override;                  // 0x419a40 (slot 5)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x419970 (slot 14)
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x417aa0 (slot 18)
	int TakeCommand() override;              // 0x419d10 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x41a490 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x419de0 (slot 21)
	void BuildFadeTrack() override;          // 0x41b990 (slot 22)
	void BuildDeathTrack() override;         // 0x41aaf0 (slot 23)

private:
	void BuildFlightTrack();       // 0x41a590 (EBX the bug)
	void BuildLeaveTrack();        // 0x41a7e0 (EBX the bug)
	bool IsBlockedAhead();         // 0x419e00 (EDI the bug)
	float CastRay(float theAngle, float theX, float theY, float* theEndX, float* theEndY); // 0x419f60 (EBP the bug, EBX theEndX; the rest pushed)
	float ChooseFlight(float theExtraAngle, float* theEndX, float* theEndY, float* theReach); // 0x41a0a0 (ESI the bug, EAX theEndX, EDI theEndY; theReach and theExtraAngle pushed)
	void BeginFlight(float theExtraAngle);   // 0x41a300 (EAX the bug, theExtraAngle pushed)
	void BeginLeave();             // 0x41a1f0 (EAX the bug)
};

// 0x41aee0 (GameWidget's preloads): the flight paths, the picture "images/path_fly" read for each of the two
void PreloadFlyPaths();

} // namespace Sexy
