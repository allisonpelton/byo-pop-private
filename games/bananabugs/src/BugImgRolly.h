// BugImgRolly: the roly-poly's body (0x424300..0x424c80 in the original, "Rolly" there; 0x2d8 bytes). It is the picture of
// the roly-poly, a child of its brain (BugBrainRolly), and walks, curls up into a ball and uncurls on its own tracks while the
// brain moves the brain's own place. The body hangs 32 pixels above the wall's line (Y -32) and draws all its tracks
// from the one picture (set on the body itself, not on its keys), with the cels 0 to 29 the walk, 30 to 50 the curl, 51 the
// ball, and 20 down to 0 the uncurl. The brain sends it commands (BugObj::SetCommand's, through slot 13): 0x6b to curl and
// 0x6c to uncurl; the freeze, the daze (the stars, whose commands it swallows: see Slot13) and the ring are not shown
// as they are on the others.
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The commands the brain sends the body that the roly-poly alone answers (BugImg's BUG_CMD_ run from 100 to 0x69)
const int BUG_CMD_ROLLY_CURL = 0x6b;
const int BUG_CMD_ROLLY_UNCURL = 0x6c;

// The events the body's keys end with (the key's mEventId), which Notify tells apart
enum RollyBodyEvent
{
	ROLLY_EVENT_STEP_DONE = 1,     // the walk track's last key: one step is walked
	ROLLY_EVENT_CURLED = 2,        // the curl's last key: it is a ball
	ROLLY_EVENT_BALL_DONE = 3,     // the ball track's last key: the ball starts again
	ROLLY_EVENT_UNCURLED = 4,      // the uncurl's last key: it walks again
	ROLLY_EVENT_UNUSED_5 = 5,      // Notify treats it like the walk's end, but no key of this body has it
};

class BugImgRolly : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	bool mCurled;                  // +0x290 (the name is a guess): set to 0 by the constructor and to 1 when the curl ends; nothing reads it
	bool mUnused2A4;               // +0x2a4: set to 0 by the constructor; nothing reads it
	int mWalkTrack;                // +0x2ac: one step along the wall (AddTrack's number)
	int mBallTrack;                // +0x2b0 (the name is a guess): the ball: 20 frames and a key that raises event 3, which starts it again
	int mCurlTrack;                // +0x2b4 (the name is a guess): the curl: 100 frames of cels 30 to 50, rising from Y -32 to -26, then a key raising event 2
	int mUncurlTrack;              // +0x2b8 (the name is a guess): the uncurl: 100 frames of cels 20 to 0, then a key raising event 4
	AnimKey* mWalkKey;             // +0x2c4: the walk track's first key (a step of 80 frames, the cels from 0 to 29)
	AnimKey* mWalkEndKey;          // +0x2c8: its second key, which raises event 1
	AnimKey* mBallKey;             // +0x2cc: the ball track's first key (20 frames at cel 0 going to cel 8)
	AnimKey* mBallEndKey;          // +0x2d0: its second key (event 3)

	BugImgRolly();                 // 0x424300
	~BugImgRolly() override;       // 0x424390 (the body 0x4243b0)
	void Notify(ObjBase* theObj) override;   // 0x4243c0 (slot 4)
	bool Update() override;                  // 0x4219c0 (slot 5; shared): ObjBase's update
	// 0x424450 (slot 13): curl and uncurl; the commands 100 to 0x67 do nothing; BugImg's answers every other, but not
	// first (the ant's and the beetle's do that)
	void Slot13(int theCommand, int theFrames, int theUnused) override;

	static Anim* sRollyAnim;       // 0x60e2a0: "images/rolypoly"

private:
	void BuildWalkTrack();         // 0x4244c0 (EBX the body)
	void BuildCurlTrack();         // 0x424670
	void BuildUncurlTrack();       // 0x4248a0
	void BuildBallTrack();         // 0x424ad0 (EBX the body)
};

// 0x424c80 (GameWidget's preloads): the picture (96 by 79, 60 cels)
void PreloadRolyPoly();

} // namespace Sexy
