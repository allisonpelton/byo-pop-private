// BugImgDragonfly: the dragonfly's body (0x422f70..0x423600 in the original, "DragonFly" there; 0x2d8 bytes). It is the
// picture of the dragonfly, a child of its brain (BugBrainDragonfly), which tells it by BugObj::SetCommand's commands and its
// own: 0x6e and 0x71 hover (the idle picture, 30 cels), 0x6f winds up (20 cels over 60 frames and back over 10), 0x70
// darts (6 cels, counting up), 100 starts a dance of as many frames as the second argument says (what a frozen dragonfly
// does; it sets mFrozen, which the brain reads) and 0x65 ends it. A swell emitter is made for the body (the original keeps
// it in a field; the port in mEmitters like the roly-poly's) and never used by the body's own code.
// Its update (0x4219c0) is a function the other bodies name too: it is in BugShared.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The commands the brain sends the body, beside BugImg's (BUG_CMD_FREEZE_ON, BUG_CMD_FREEZE_OFF and the stars and ring)
enum DragonflyCommand
{
	DRAGONFLY_CMD_IDLE = 0x6e,     // hover
	DRAGONFLY_CMD_WINDUP = 0x6f,
	DRAGONFLY_CMD_DART = 0x70,
	DRAGONFLY_CMD_END = 0x71,      // hover again
};

// The events the body's keys end with (the key's mEventId)
enum DragonflyBodyEvent
{
	DRAGONFLY_EVENT_IDLE = 1,        // the hover key (nothing hears it)
	DRAGONFLY_EVENT_WINDUP_DONE = 2, // the wind-up's last key (see Notify)
	DRAGONFLY_EVENT_DART = 3,        // the dart key (nothing hears it)
	DRAGONFLY_EVENT_DANCE_DONE = 4,  // the dance's last key (nothing hears it)
};

class BugImgDragonfly : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	int mUnused29C;                // +0x29c: set to -99 by the constructor; nothing reads it
	bool mUnused2A0;               // +0x2a0: set to 0 by the constructor; nothing reads it
	Anim* mIdleAnim;               // +0x2a4: "images/dragonfly_idle" (110 by 76, 30 cels); the original's shared notify reads this field as a track number (see Notify)
	Anim* mWindupAnim;             // +0x2a8: "images/dragonfly_windup" (160 by 90, 20 cels)
	Anim* mDartAnim;               // +0x2ac: "images/dragonfly_dart" (114 by 66, 6 cels)
	Anim* mDanceAnim;              // +0x2b0: "images/dragonfly_dance" (160 by 86, 30 cels)
	AnimKey* mDanceKey;            // +0x2c4: the dance's first key, whose length the freeze asked for sets
	int mIdleTrack;                // +0x2c8: the hover: the idle cels count up, over and over
	int mWindupTrack;              // +0x2cc
	int mDartTrack;                // +0x2d0
	int mDanceTrack;               // +0x2d4

	BugImgDragonfly();             // 0x422f70
	~BugImgDragonfly() override;   // 0x4230b0 (the body 0x4230d0)
	void Notify(ObjBase* theObj) override;   // 0x4237d0 (slot 4)
	bool Update() override;                  // 0x4219c0 (slot 5; shared)
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x4230e0

private:
	void BuildIdleTrack();         // 0x4231b0
	void BuildWindupTrack();       // 0x423260
	void BuildDartTrack();         // 0x4233d0
	void BuildDanceTrack();        // 0x423480
};

} // namespace Sexy
