// BugImgSkeeter: the skeeter's body (0x424ca0..0x425150 in the original, "Skeeter" there; 0x2ac bytes). It is the picture
// of the skeeter, a child of its brain (BugBrainSkeeter): it hovers in place, drifting 12 pixels up and to the left and
// back over 70 frames each way, its wings beating through the 30 cels, while the brain moves the brain's own place. The
// brain sends it commands (BugObj::SetCommand's, through slot 13): 100 starts a dance of as many frames as the second
// argument says, which is what a frozen skeeter does, and 0x65 ends it. Its notify (0x4237d0) and update (0x4219c0) are
// functions the bee's body names too: they are in BugShared.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The events the body's keys end with (the key's mEventId), which Notify (shared) tells apart
enum SkeeterBodyEvent
{
	SKEETER_EVENT_HOVER_DONE = 1,   // the hover's first key (Notify does nothing)
	SKEETER_EVENT_DANCE_DONE = 2,   // the dance's last key: it hovers again
};

class BugImgSkeeter : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused294;               // +0x294: set to 0 by the constructor; nothing reads it
	AnimKey* mHoverFirstKey;       // +0x298 (the name is a guess): the hover's first key: 70 frames from 6 left and 12 up, the cels counting up
	AnimKey* mHoverSecondKey;      // +0x29c (the name is a guess): the second: 70 frames from 6 right and 12 down, whose next is the first
	AnimKey* mDanceKey;            // +0x2a0: the dance track's first key, whose length the dance asked for sets
	int mHoverTrack;               // +0x2a4 (the name is a guess): the hover (AddTrack's number; the first track, which Activate starts)
	int mDanceTrack;               // +0x2a8: the dance: the dance cels play back and forth, then the hover starts again (event 2)

	BugImgSkeeter();               // 0x424ca0
	~BugImgSkeeter() override;     // 0x424f10 (the body 0x424f30)
	void Notify(ObjBase* theObj) override;   // 0x4237d0 (slot 4; shared)
	bool Update() override;                  // 0x4219c0 (slot 5; shared)
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x424f40 (calls BugImg's only for other commands)

	static Anim* sFlyAnim;         // 0x60e2a4: "images/skeeter_fly"
	static Anim* sDanceAnim;       // 0x60e2a8: "images/skeeter_dance"

private:
	void BuildDanceTrack();        // 0x424f90 (the body 0x424ca0 builds the hover itself)
};

// 0x425110 (GameWidget's preloads): the pictures of the flight and the dance (96 by 96, 30 cels each)
void PreloadSkeeter();

} // namespace Sexy
