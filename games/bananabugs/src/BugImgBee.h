// BugImgBee: the bee's body (0x4206b0..0x420b30 in the original, "Bee" there; 0x2ac bytes). It is the picture of the bee (the
// "wasp" picture, 30 cels of 95 by 95), a child of its brain (BugBrainBee): it shakes a little (two pixels either way across
// and down, over 70 frames each way) while its cels count round, while the brain moves the brain's own place. The brain
// sends it commands (BugObj::SetCommand's, through slot 13): it answers the end of a freeze (0x65) by cutting the dance
// short and the start (100) by nothing (the brain never sends either); the stars and the ring are BugImg's. It has the
// skeeter's fields: its notify (0x4237d0) and update (0x4219c0) are in BugShared.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The events the body's keys end with (the key's mEventId), which Notify (shared) tells apart
enum BeeBodyEvent
{
	BEE_EVENT_SHAKE = 1,        // the shake's keys (Notify does nothing)
	BEE_EVENT_DANCE_DONE = 2,   // the dance's last key: the shake starts again
};

class BugImgBee : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mUnused294;               // +0x294: set to 0 by the constructor; nothing reads it
	AnimKey* mShakeFirstKey;       // +0x298 (the name is a guess): the shake's first key: 70 frames, the place toward two pixels up and left of the next, the cels counting up
	AnimKey* mShakeSecondKey;      // +0x29c (the name is a guess): the second: toward two pixels down and right of the first, whose next it is
	AnimKey* mDanceKey;            // +0x2a0: the dance track's first key: 800 frames, the cels counting up from the 12th, the turn eased to none
	int mShakeTrack;               // +0x2a4 (the name is a guess): the shake (AddTrack's number; the first track, which Activate starts)
	int mDanceTrack;               // +0x2a8: the dance: the dance key, 75 frames holding the first cel, then a key of no length that raises event 2 (nothing starts it)

	BugImgBee();                   // 0x4206b0
	~BugImgBee() override;         // 0x420920 (the body 0x420940)
	void Notify(ObjBase* theObj) override;   // 0x4237d0 (slot 4; shared)
	bool Update() override;                  // 0x4219c0 (slot 5; shared)
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x420950

	static Anim* sWaspAnim;        // 0x60e268: "images/wasp"

private:
	void BuildDanceTrack();        // 0x420980
};

// 0x420b30 (GameWidget's preloads): the picture (95 by 95, 30 cels)
void PreloadWasp();

} // namespace Sexy
