// BugImgCaterpillar: the caterpillar's body (0x4217b0..0x422220 in the original, "Caterpillar" there; 0x2d0 bytes). It is the
// picture of one bug of the column (the head's picture or a body's, by the role the brain was set up with), a child of its
// brain (BugBrainCaterpillar), and walks and turns on its own tracks while the brain moves the brain's own place. The body
// hangs 28 pixels to one side of the wall the bug walks along (its Y is -28 above the line or +28 below it); when the bug
// turns round a post the body flips over to the other side with a turn track. The brain sends it commands
// (BugObj::SetCommand's, through slot 13): 0x6a tells it to flip at the end of the step it is on; a freeze is not
// answered (the caterpillar has no dance), nor is its end. The same shape as the ant's body (BugImgAnt), with its own pictures
// and times, without the dance and without knowing its brain.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The command the brain sends the body that the caterpillar alone answers (BugImg's BUG_CMD_ run from 100 to 0x69)
const int BUG_CMD_CATERPILLAR_FLIP = 0x6a;

// The events the body's keys end with (the key's mEventId), which Notify tells apart
enum CaterpillarBodyEvent
{
	CATERPILLAR_EVENT_STEP_DONE = 1,       // the walk track's last key: one step is walked
	CATERPILLAR_EVENT_FLIPPED_BOTTOM = 2,  // the flip down to the lower side is done
	CATERPILLAR_EVENT_FLIPPED_TOP = 3,     // the flip up to the upper side is done
	CATERPILLAR_EVENT_MIRROR = 4,          // the middle of a flip: the picture is mirrored
};

class BugImgCaterpillar : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused290;               // +0x290: set to 0 by the constructor; nothing reads it
	bool mFlipRequested;           // +0x298 (the name is a guess): the brain has sent 0x6a; the next step's end flips the body over
	bool mUnused2A4;               // +0x2a4: set to 0 by the constructor; nothing reads it
	int mWalkTrack;                // +0x2ac: one step along the wall (AddTrack's number)
	int mFlipToBottomTrack;        // +0x2b0: flips the body from the upper side to the lower (starts at Y -25, tilts, ends at +25 with event 2)
	int mFlipToTopTrack;           // +0x2b4: the other way (ends with event 3)
	AnimKey* mWalkKey;             // +0x2c0: the walk track's first key (25 frames)
	AnimKey* mWalkMiddleKey;       // +0x2c4: its second key, 55 frames, which swings the body 8 pixels back and shows cel 15
	AnimKey* mWalkEndKey;          // +0x2c8: its third key, which raises event 1

	explicit BugImgCaterpillar(int theRole); // 0x4217b0 (the role Setup was given: 0 is the head, which has its own picture)
	~BugImgCaterpillar() override; // 0x421850 (the body 0x421870)
	void Notify(ObjBase* theObj) override;   // 0x421880 (slot 4)
	bool Update() override;                  // 0x4219c0 (slot 5; shared): ObjBase's
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x4219d0 (calls BugImg's for the commands it does not answer)

	static Anim* sHeadAnim;        // 0x60e278: "images/caterpillar_head"
	static Anim* sBodyAnim;        // 0x60e27c: "images/caterpillar_body"

private:
	void BuildWalkTrack();         // 0x421a00 (EBX the body)
	void BuildFlipToBottomTrack(); // 0x421ce0
	void BuildFlipToTopTrack();    // 0x421f80
};

// 0x422220 (GameWidget's preloads): the head's picture (36 by 45, 30 cels) and the body's (36 by 36)
void PreloadCaterpillar();

} // namespace Sexy
