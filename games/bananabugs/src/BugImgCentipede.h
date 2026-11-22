// BugImgCentipede: the centipede's body (0x422260..0x422f10 in the original, "Centipede" there; 0x2f4 bytes). It is the
// picture of one bug of the column (the head's, a body's or the tail's, by the role the brain was set up with), a child of
// its brain (BugBrainCentipede). Its legs run on their own: an idle track counts the cels round, and the brain's command 0x6e
// (a long leg is starting) starts a sway: the body moves 24 pixels down with a tilt each way and back, then (event 1) the
// same upwards, then (event 2) the idle track again. The original also answers 0x6f, which starts the upward half on its own;
// nothing translated sends it. Commands that are not these are BugImg's (the stars and the ring).
#pragma once

#include "BugImg.h"

namespace Sexy
{

class Anim;

// The commands the brain sends the body that the centipede alone answers (BugImg's BUG_CMD_ run from 100 to 0x69)
const int BUG_CMD_CENTIPEDE_SWAY_DOWN = 0x6e;  // the name is a guess
const int BUG_CMD_CENTIPEDE_SWAY_UP = 0x6f;    // the name is a guess

// The events the body's keys end with (the key's mEventId), which Notify tells apart
enum CentipedeBodyEvent
{
	CENTIPEDE_EVENT_SWAY_DOWN_DONE = 1,   // the downward sway's last key: the upward sway starts
	CENTIPEDE_EVENT_SWAY_UP_DONE = 2,     // the upward sway's last key: the idle track starts again
};

// What Setup's role means to the body
enum CentipedeRole
{
	CENTIPEDE_ROLE_HEAD = 0,
	CENTIPEDE_ROLE_BODY = 1,
	CENTIPEDE_ROLE_TAIL = 2,
};

class BugImgCentipede : public BugImg
{
public:
	// The offsets in the comments are the original's, from the object
	int mRole;                     // +0x290: the role Setup was given (a CentipedeRole): which picture the body shows
	int mIdleTrack;                // +0x2a4 (the name is a guess): the cels count round (AddTrack's number)
	int mSwayDownTrack;            // +0x2a8 (the name is a guess): the first half of the sway, five keys of 45 frames (event 1 at the end)
	int mSwayUpTrack;              // +0x2ac (the name is a guess): the second half, five keys (event 2 at the end)
	bool mUnused2B4;               // +0x2b4: set to 0 by the constructor; nothing reads it

	explicit BugImgCentipede(int theRole);   // 0x422260 (the role in ESI)
	~BugImgCentipede() override;             // 0x422350 (the body 0x422370)
	void Notify(ObjBase* theObj) override;   // 0x422380 (slot 4)
	bool Update() override;                  // 0x4219c0 (slot 5; shared): ObjBase's
	void Slot13(int theCommand, int theFrames, int theUnused) override; // 0x4223d0 (calls BugImg's for the commands it does not answer)

	static Anim* sHeadAnim;        // 0x60e280: "images/centipede_head"
	static Anim* sBodyAnim;        // 0x60e284: "images/centipede_body"
	static Anim* sTailAnim;        // 0x60e288: "images/centipede_tail"

private:
	void BuildIdleTrack();         // 0x422410 (EBX the body)
	void BuildSwayDownTrack();     // 0x422510
	void BuildSwayUpTrack();       // 0x422a10
};

// 0x422f10 (GameWidget's preloads): the head's picture (65 by 63, 90 cels), the body's (43 by 49) and the tail's (37 by 33)
void PreloadCentipede();

} // namespace Sexy
