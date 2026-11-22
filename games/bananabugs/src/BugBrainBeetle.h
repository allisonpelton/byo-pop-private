// BugBrainBeetle: the beetle's brain (0x416220..0x416ea0 in the original; BugBrainBeetle). The beetle walks along the
// walls of the level's grid, post to post, like the ant (BugBrainAnt) but alone: each step is a track of three keys (the
// walk to the next post, a turn in place there, and the turn's end). A long walk (over 128 pixels) is walked at 2.7
// frames a pixel and eased like a cosine; a short one, or one on which the body flips to the other side of the wall,
// at 3 frames a pixel and linearly. It does not look at the rope. It is killed by anything that kills a bug; it flies
// off and falls, tumbling, as the death track.
// Slots this class overrides: Notify (4), Setup (14), StepSegment (15), Vanish (17), SetCommand (18), TakeCommand (19),
// Zap (20), PlaceAt (21), BuildFadeTrack (22) and BuildDeathTrack (23); Update (5) is the base's. StepSegment, Vanish,
// SetCommand, TakeCommand and BuildFadeTrack are one function in the original that several kinds' vtables name: they are in
// BugShared, and each override here calls the shared copy.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgBeetle;
class PipeObj;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum BeetleBrainEvent
{
	BEETLE_BRAIN_EVENT_LEG_DONE = 2,   // the leg to the next post is walked (Notify does nothing)
	BEETLE_BRAIN_EVENT_TURN_DONE = 3,  // the turn in place is done (Notify does nothing)
	BEETLE_BRAIN_EVENT_TURN_END = 4,   // the turn's last key: the next leg starts
	BEETLE_BRAIN_EVENT_GONE = 5,       // the fade track's end: the bug goes
	BEETLE_BRAIN_EVENT_TUMBLED = 6,    // the death track's tumble key is done
};

class BugBrainBeetle : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mNextHeading;              // +0x338 (the name is a guess): the heading the beetle takes at the post it is walking to
	int mUnused33C;                // +0x33c: Zap sets it to 0; nothing reads it
	int mMoveTrack;                // +0x340: the step's track (AddTrack's number)
	AnimKey* mLegKey;              // +0x34c (the name is a guess): the walk from where the beetle is to the next post (event 2)
	AnimKey* mTurnKey;             // +0x350 (the name is a guess): the turn in place at the post (event 3)
	AnimKey* mTurnEndKey;          // +0x354 (the name is a guess): the turn's last frame (event 4)
	AnimKey* mFlingKey;            // +0x358 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x35c (the name is a guess): its spin and fade-in (event 6)
	AnimKey* mFadeOutKey;          // +0x360 (the name is a guess): the last key, which fades it out and removes it

	BugBrainBeetle();              // 0x416220
	void Notify(ObjBase* theObj) override;   // 0x416360 (slot 4)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x4162a0 (slot 14)
	void StepSegment() override;             // 0x41c620 (slot 15): updates the bug, then its body
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x416420 (slot 18; shared)
	int TakeCommand() override;              // 0x41e1f0 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x416750 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x416480 (slot 21)
	void BuildFadeTrack() override;          // 0x415b60 (slot 22)
	void BuildDeathTrack() override;         // 0x416b30 (slot 23)

private:
	void BuildMoveTrack();         // 0x4168e0 (EBX the bug)
	void BeginStep(bool theRandom);// 0x416500 (EAX the bug, theRandom pushed)
	PipeObj* GetNextPost();        // the post the heading leads to from the place the beetle is on (the start of 0x416500), or none
};

} // namespace Sexy
