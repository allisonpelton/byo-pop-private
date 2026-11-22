// BugImgBeetle: the beetle's body (0x420b50..0x421750 in the original, "Beetle" there; 0x2d0 bytes). It is the picture of
// the beetle, a child of its brain (BugBrainBeetle), and walks, turns and dances on its own tracks while the brain moves
// the brain's own place. The body hangs 32 pixels to one side of the wall the beetle walks along (its Y is -32 above the
// line or +32 below it); when the beetle turns round a post the body flips over to the other side with a turn track. The
// brain sends it commands (BugObj::SetCommand's, through slot 13): 100 and 0x65 start and end a dance, which is what a
// frozen beetle does, and 0x6a tells it to flip at the end of the step it is on. The same shape as the ant's body
// (BugImgAnt), with its own pictures and times; it does not know its brain.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The command the brain sends the body that the beetle alone answers (BugImg's BUG_CMD_ run from 100 to 0x69)
const int BUG_CMD_BEETLE_FLIP = 0x6a;

// The events the body's keys end with (the key's mEventId), which Notify tells apart
enum BeetleBodyEvent
{
	BEETLE_EVENT_STEP_DONE = 1,       // the walk track's last key: one step is walked
	BEETLE_EVENT_FLIPPED_BOTTOM = 2,  // the flip down to the lower side is done
	BEETLE_EVENT_FLIPPED_TOP = 3,     // the flip up to the upper side is done
	BEETLE_EVENT_MIRROR = 4,          // the middle of a flip: the picture is mirrored
	BEETLE_EVENT_DANCE_DONE = 5,      // the dance's last key: it walks again
};

class BugImgBeetle : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused290;               // +0x290: set to 0 by the constructor; nothing reads it
	bool mFlipRequested;           // +0x298 (the name is a guess): the brain has sent 0x6a; the next step's end flips the body over
	int mDanceFrames;              // +0x2a0 (the name is a guess): how long the dance asked for lasts, in frames; 0 for none
	bool mUnused2A4;               // +0x2a4: set to 0 by the constructor; nothing reads it
	int mWalkTrack;                // +0x2ac: one step along the wall (AddTrack's number)
	int mFlipToBottomTrack;        // +0x2b0: flips the body from the upper side to the lower (starts at Y -32, tilts, ends at +32 with event 2)
	int mFlipToTopTrack;           // +0x2b4: the other way (ends with event 3)
	int mDanceTrack;               // +0x2b8: the dance: the dance cels play back and forth, then it walks again (event 5)
	bool mStepDone;                // +0x2c0 (the name is a guess): a step has just ended without a flip, so a dance may start now
	AnimKey* mWalkKey;             // +0x2c4: the walk track's first key (a step of 80 frames)
	AnimKey* mWalkEndKey;          // +0x2c8: its second key, which raises event 1
	AnimKey* mDanceKey;            // +0x2cc: the dance track's first key, whose length the dance asked for sets

	BugImgBeetle();                // 0x420b50
	~BugImgBeetle() override;      // 0x420be0 (the body 0x420c00)
	void Notify(ObjBase* theObj) override;   // 0x420c10 (slot 4)
	bool Update() override;                  // 0x420d60 (slot 5): starts the dance that was asked for once a step has ended
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x420db0 (calls BugImg's first)

	static Anim* sWalkAnim;        // 0x60e26c: "images/beetle_walk"
	static Anim* sTurnAnim;        // 0x60e270: "images/beetle_turn"
	static Anim* sDanceAnim;       // 0x60e274: "images/beetle_dance"

private:
	void BuildWalkTrack();         // 0x420e10 (EBX the body)
	void BuildFlipToBottomTrack(); // 0x421050
	void BuildFlipToTopTrack();    // 0x421310
	void BuildDanceTrack();        // 0x4215d0
};

// 0x421750 (GameWidget's preloads): the pictures of the walk (96 by 79, 30 cels), the turn (96 by 87) and the dance (96 by 96)
void PreloadBeetle();

} // namespace Sexy
