// BugImgAnt: the ant's body (0x41f9c0..0x420650 in the original, "Ant" there; 0x2d4 bytes). It is the picture of the
// ant, a child of its brain (BugBrainAnt), and walks, turns and dances on its own tracks while the brain moves the
// brain's own place. The body hangs 32 pixels to one side of the wall the ant walks along (its Y is -32 above the line
// or +32 below it); when the ant turns round a post the body flips over to the other side with a turn track. The brain
// sends it commands (BugObj::SetCommand's, through slot 13): 100 and 0x65 start and end a dance, which is what a frozen
// ant does, and 0x6a tells it to flip at the end of the step it is on.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;
class BugBrainAnt;

// The command the brain sends the body that the ant alone answers (BugImg's BUG_CMD_ run from 100 to 0x69)
const int BUG_CMD_ANT_FLIP = 0x6a;

// The events the body's keys end with (the key's mEventId), which Notify tells apart
enum AntBodyEvent
{
	ANT_EVENT_STEP_DONE = 1,       // the walk track's last key: one step is walked
	ANT_EVENT_FLIPPED_BOTTOM = 2,  // the flip down to the lower side is done
	ANT_EVENT_FLIPPED_TOP = 3,     // the flip up to the upper side is done
	ANT_EVENT_MIRROR = 4,          // the middle of a flip: the picture is mirrored
	ANT_EVENT_DANCE_DONE = 5,      // the dance's last key: it walks again
};

class BugImgAnt : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused290;               // +0x290: set to 0 by the constructor; nothing reads it
	bool mFlipRequested;           // +0x298 (the name is a guess): the brain has sent 0x6a; the next step's end flips the body over
	bool mStepDone;                // +0x2a0 (the name is a guess): a step has just ended without a flip, so a dance may start now
	int mDanceFrames;              // +0x2a4 (the name is a guess): how long the dance asked for lasts, in frames; 0 for none
	bool mUnused2A8;               // +0x2a8: set to 0 by the constructor; nothing reads it
	int mWalkTrack;                // +0x2b0: one step along the wall (AddTrack's number)
	int mFlipToBottomTrack;        // +0x2b4: flips the body from the upper side to the lower (starts at Y -32, tilts, ends at +32 with event 2)
	int mFlipToTopTrack;           // +0x2b8: the other way (ends with event 3)
	int mDanceTrack;               // +0x2bc: the dance: the dance cels play back and forth, then it walks again (event 5)
	AnimKey* mWalkKey;             // +0x2c4: the walk track's first key (a step of 80 frames)
	AnimKey* mWalkEndKey;          // +0x2c8: its second key, which raises event 1
	AnimKey* mDanceKey;            // +0x2cc: the dance track's first key, whose length the dance asked for sets
	BugBrainAnt* mBrain;           // +0x2d0: the brain that owns this body, whose turn state the flips end

	BugImgAnt();                   // 0x41f9c0
	~BugImgAnt() override;         // 0x41fa60 (the body 0x41fa80)
	void Notify(ObjBase* theObj) override;   // 0x41fa90 (slot 4)
	bool Update() override;                  // 0x41fbe0 (slot 5): starts the dance that was asked for once a step has ended
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x41fc30 (calls BugImg's first)

	void SetBrain(BugBrainAnt* theBrain);    // 0x41fc90 (EAX the brain, ECX the body): the brain is the body's parent
	void FlipWalk();               // 0x41fcb0 (ECX the body): the walk's Y sides swap and the walk starts again, from the first cel

	static Anim* sWalkAnim;        // 0x60e25c: "images/ant_walk"
	static Anim* sTurnAnim;        // 0x60e260: "images/ant_turn"
	static Anim* sDanceAnim;       // 0x60e264: "images/ant_dance"

private:
	void BuildWalkTrack();         // 0x41fcf0 (EBX the body)
	void BuildFlipToBottomTrack(); // 0x41ff50
	void BuildFlipToTopTrack();    // 0x420210
	void BuildDanceTrack();        // 0x4204d0
};

// 0x420650 (GameWidget's preloads): the pictures of the walk (96 by 73, 30 cels), the turn (the same) and the dance (96 by 96)
void PreloadAnt();

} // namespace Sexy
