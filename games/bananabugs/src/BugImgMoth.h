// BugImgMoth: the moth's body (0x4239c0..0x423d10 in the original, "MothObj" there; 0x2ac bytes). It is the picture of the moth,
// a child of its brain (BugBrainMoth): its wings beat (2 cels, one frame each) while the brain moves the brain's own place.
// The brain sends it commands (BugObj::SetCommand's, through slot 13): 100 starts a dance of as many frames as the second
// argument says, which is what a frozen moth does, and 0x65 ends it. It is the fly's body (BugImgFly) with other pictures
// and a one-frame wing key: it has the skeeter's fields and the same functions: its notify (0x4237d0), update (0x4219c0)
// and slot 13 (0x424f40) are in BugShared.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The events the body's keys end with (the key's mEventId), which Notify (shared) tells apart
enum MothBodyEvent
{
	MOTH_EVENT_FLAP = 1,         // the wings' keys (Notify does nothing)
	MOTH_EVENT_DANCE_DONE = 2,   // the dance's last key: the wings beat again
};

class BugImgMoth : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused294;               // +0x294: set to 0 by the constructor; nothing reads it
	AnimKey* mFlapFirstKey;        // +0x298 (the name is a guess): the wings' first key: a frame, the cel 0 (toward the next)
	AnimKey* mFlapSecondKey;       // +0x29c (the name is a guess): the second: a frame, the cel 1 (toward the first, whose next it is)
	AnimKey* mDanceKey;            // +0x2a0: the dance track's first key, whose length the dance asked for sets
	int mFlapTrack;                // +0x2a4 (the name is a guess): the wings (AddTrack's number; the first track, which Activate starts)
	int mDanceTrack;               // +0x2a8: the dance: the dance cels play back and forth, then the wings beat again (event 2)

	BugImgMoth();                   // 0x4239c0
	~BugImgMoth() override;         // 0x423b60 (the body 0x423b80)
	void Notify(ObjBase* theObj) override;   // 0x4237d0 (slot 4; shared)
	bool Update() override;                  // 0x4219c0 (slot 5; shared)
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x424f40 (shared)

	static Anim* sDanceAnim;       // 0x60e294: "images/moth_dance"
	static Anim* sMothAnim;         // 0x60e298: "images/moth"

private:
	void BuildDanceTrack();        // 0x423b90
};

// 0x423d10 (GameWidget's preloads): the pictures of the dance (96 by 96, 30 cels) and of the wings (96 by 96, 2 cels)
void PreloadMoth();

} // namespace Sexy
