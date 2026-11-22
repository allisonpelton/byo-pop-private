// BugBrainMoth: the moth's brain (0x41af60..0x41c470 in the original, with the flight paths 0x41c470; BugBrainMoth). The
// moth is drawn to the monkey. It flies a short flight at a time: it takes the direction to the monkey, turned by up to
// three degrees either way at random, and when the monkey is 32 pixels away or more flies toward it over 20 to 45 frames,
// its speed easing from 0.7 to 0.5 a frame (its picture turned to face the way); closer than that it stays where it is,
// turning to face the way, for 40 frames. Each frame it looks 51 pixels ahead along its last flight; when a wall or a cut
// place is in the way it stops heeding the monkey for 120 frames, and takes the way it last went (turned a few degrees at
// random) instead, so it hovers, turning a little each time, until the way is clear. When the monkey moves fast and comes
// within 124 pixels it flees: it flies off along the path read from "images/path_moth" (its place when it begins and the
// path's points, over as many frames as 0.155 of the points and some at random), lands where the path ends, and flies
// again. It is killed by anything that kills a bug; it is flung off, falls and tumbles as the death track. A moth held
// by a freeze whose place is cut, or on a wall, dies on the spot.
// The class is the fly's (BugBrainFly) with its own flight: its tracks, death and Zap are the same code at other addresses.
// Slots this class overrides: Notify (4), Update (5), Setup (14), TakeCommand (19), Zap (20), PlaceAt (21) and BuildDeathTrack
// (23); Vanish (17), SetCommand (18) and BuildFadeTrack (22; 0x41b990, the fly's too) are one function in the original that
// several kinds' vtables name: they are in BugShared and each override here calls the shared copy. StepSegment (15) and
// AvoidRope (16) are the base's.
// The constructor gives the kind 7 (BUG_FLY, the fly's number), though LevelObj's switch makes the moth for case 9
// (BUG_MOTH); nothing reads the difference.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgMoth;
class PathTrace;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum MothBrainEvent
{
	MOTH_BRAIN_EVENT_PATH_BEGUN = 3,    // the leave track's first key has begun (nothing hears it)
	MOTH_BRAIN_EVENT_LANDING = 4,       // the leave track's second key has begun: the moth may be hit again
	MOTH_BRAIN_EVENT_LEFT = 5,          // the leave track's end: it has landed, and flies again
	MOTH_BRAIN_EVENT_UNUSED_6 = 6,      // no key has it (nothing hears it)
	MOTH_BRAIN_EVENT_FLIGHT_DONE = 7,   // the flight's end: it chooses again
	MOTH_BRAIN_EVENT_GONE = 8,          // the fade track's end: the bug goes
	MOTH_BRAIN_EVENT_TUMBLED = 9,       // the death track's tumble key is done
};

// What the moth is doing (mMothState; 1 is never set)
enum MothState
{
	MOTH_STATE_FLYING = 0,              // flies short flights
	MOTH_STATE_FLEEING = 2,             // on the path of its flight from the monkey
	MOTH_STATE_LANDED = 3,              // the path is done: the next frame it is flying
};

class BugBrainMoth : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mWanderFrames;             // +0x338 (the name is a guess): frames left in which a new flight ignores the monkey (set to 120 when a wall is ahead)
	int mPathIndex;                // +0x33c (the name is a guess): which of the two paths it flees along (0 always: nothing else sets it)
	float mLeaveFromX;             // +0x340 (the name is a guess): where it was when it began to flee
	float mLeaveFromY;             // +0x344 (the name is a guess)
	float mLeaveFromRotation;      // +0x348 (the name is a guess): and how it was turned
	bool mUnused34C;               // +0x34c: set to 1 by the constructor; nothing reads it
	int mMothState;                // +0x350 (the name is a guess): a MothState
	float mUnused354;              // +0x354: set to 0 by the constructor; nothing reads it
	int mFlightTrack;              // +0x358: the flight's track (AddTrack's number)
	int mLeaveTrack;               // +0x35c (the name is a guess): the fleeing along the path
	AnimKey* mFlightKey;           // +0x360 (the name is a guess): the flight: 20 to 45 frames (40 when it stays), from where the moth is, easing its speed from 0.7 to 0.5 a frame along the angle
	AnimKey* mFlightEndKey;        // +0x364 (the name is a guess): the last, a frame (event 7), which holds the place the flight goes to
	AnimKey* mPathKey;             // +0x36c (the name is a guess): the path's first key: the path's points, as the PATH channel moves along them (event 3)
	AnimKey* mLandKey;             // +0x370 (the name is a guess): from the path's last point to where the moth began, 20 frames (event 4)
	AnimKey* mLeaveEndKey;         // +0x374 (the name is a guess): the last, a frame (event 5)
	AnimKey* mFlingKey;            // +0x378 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x37c (the name is a guess): its spin and fade-in (event 9)
	AnimKey* mFadeOutKey;          // +0x380 (the name is a guess): the last key, which fades it out and removes it
	float mFlightAngle;            // +0x384 (the name is a guess): the angle (in radians, up is a quarter turn) of the last flight, which the look ahead goes along

	BugBrainMoth();                // 0x41af60
	void Notify(ObjBase* theObj) override;   // 0x41b220 (slot 4)
	bool Update() override;                  // 0x41b0d0 (slot 5)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x41b000 (slot 14)
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x417aa0 (slot 18)
	int TakeCommand() override;              // 0x41b380 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x41b890 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x41b440 (slot 21)
	void BuildFadeTrack() override;          // 0x41b990 (slot 22)
	void BuildDeathTrack() override;         // 0x41c080 (slot 23)

private:
	void BuildFlightTrack();       // 0x41bb20 (EBX the bug)
	void BuildLeaveTrack();        // 0x41bd70 (EBX the bug)
	bool IsBlockedAhead();         // 0x41b460 (EAX the bug)
	void BeginFlight(bool theIgnoreMonkey); // 0x41b660 (EAX the bug, theIgnoreMonkey pushed)
	void BeginLeave();             // 0x41b550 (EAX the bug)
};

// 0x41c470 (GameWidget's preloads): the flight paths, the picture "images/path_moth" read for each of the two
void PreloadMothPaths();

} // namespace Sexy
