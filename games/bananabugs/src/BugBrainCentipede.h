// BugBrainCentipede: the centipede's brain (0x417840..0x418880 in the original; BugBrainCentipede). A centipede is a column of
// sixteen bugs (a head, fourteen bodies and a tail, each its own brain with its own body, BugImgCentipede): the level
// makes the head, then each of the others at the head's place after the bugs made before it have walked on for 60 frames.
// Every one walks the walls of the level's grid post to post, like the caterpillar (BugBrainCaterpillar), but it cuts
// each corner: it walks to a place 15 pixels short of the post, then straight across to a place 15 pixels along the next
// heading, at 2.35 frames a pixel (three keys: the walk, the cut across, and its end). At a post it takes the first way on
// (not straight back) that the post has a wall for, from the one after its heading, and never a random one, so every bug
// of the column takes the way the one before it took. A leg longer than five places also makes its body sway (a wave that
// runs down the column as the bugs start the leg one after another). While a rope is flying the centipede is updated four
// times a frame, so it hurries. It does not look at the rope otherwise. Killing the head kills the column (BugObj::Zap);
// killing a body holds the ones behind it, daze-held for good. Each one that dies is flung off and falls, tumbling, as the
// death track.
// Slots this class overrides: Notify (4), Update (5), Setup (14), StepSegment (15), Vanish (17), SetCommand (18),
// TakeCommand (19), Zap (20), PlaceAt (21), BuildFadeTrack (22) and BuildDeathTrack (23). StepSegment, Vanish, SetCommand
// and BuildFadeTrack are one function in the original that several kinds' vtables name: they are in BugShared, and each
// override here calls the shared copy.
#pragma once

#include "BugObj.h"

namespace Sexy
{

class BugImgCentipede;
class PipeObj;

// The events the brain's keys end with (the key's mEventId), which Notify tells apart (event 1 kills the bug in BugObj)
enum CentipedeBrainEvent
{
	CENTIPEDE_BRAIN_EVENT_LEG_DONE = 3,   // the leg to the place short of the post is walked (Notify does nothing)
	CENTIPEDE_BRAIN_EVENT_CUT_DONE = 4,   // the cut across the corner is done (Notify does nothing)
	CENTIPEDE_BRAIN_EVENT_STEP_END = 5,   // the last key: the next step starts
	CENTIPEDE_BRAIN_EVENT_GONE = 6,       // the fade track's end: the bug goes
	CENTIPEDE_BRAIN_EVENT_TUMBLED = 7,    // the death track's tumble key is done
};

class BugBrainCentipede : public BugObj
{
public:
	// The offsets in the comments are the original's, from the object
	int mNextHeading;              // +0x338 (the name is a guess): the heading the centipede takes at the post it is walking to
	bool mUnused33C;               // +0x33c: set to 0 by the constructor; nothing reads it
	int mRunLength;                // +0x340 (the name is a guess): the leg's length in fives of places (0 when it is five places or less); the body sways while it is over 0
	int mMoveTrack;                // +0x344: the step's track (AddTrack's number)
	AnimKey* mLegKey;              // +0x34c (the name is a guess): the walk from where the centipede is to the place short of the post (event 3)
	AnimKey* mCutKey;              // +0x350 (the name is a guess): the cut across to the place along the next heading (event 4)
	AnimKey* mStepEndKey;          // +0x354 (the name is a guess): the step's last frame (event 5)
	AnimKey* mFlingKey;            // +0x358 (the name is a guess): the death track's first key: it is flung off and falls, until it is below the screen
	AnimKey* mTumbleKey;           // +0x35c (the name is a guess): its spin and fade-in (event 7)
	AnimKey* mFadeOutKey;          // +0x360 (the name is a guess): the last key, which fades it out and removes it

	BugBrainCentipede();           // 0x417840
	void Notify(ObjBase* theObj) override;   // 0x4179e0 (slot 4)
	bool Update() override;                  // 0x417990 (slot 5)
	void Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious) override; // 0x4178d0 (slot 14)
	void StepSegment() override;             // 0x41c620 (slot 15): updates the bug, then its body
	void Vanish() override;                  // 0x415810 (slot 17)
	bool SetCommand(int theCommand, int theFrames) override; // 0x417aa0 (slot 18; shared)
	int TakeCommand() override;              // 0x417b20 (slot 19)
	bool Zap(bool theFollowers, bool theStartTrack, bool thePeaHit) override; // 0x417fa0 (slot 20)
	void PlaceAt(int theColumn, int theRow) override; // 0x417bf0 (slot 21)
	void BuildFadeTrack() override;          // 0x418130 (slot 22; shared with the roach and the roly-poly)
	void BuildDeathTrack() override;         // 0x418510 (slot 23)

private:
	void BuildMoveTrack();         // 0x4182c0 (EBX the bug)
	void BeginStep();              // 0x417c70 (EAX the bug)
	PipeObj* GetNextPost();        // the post the heading leads to from the place the centipede is on (the start of 0x417c70), or none
};

} // namespace Sexy
