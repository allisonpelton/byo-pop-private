#include "BugImgCentipede.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgCentipede::sHeadAnim = nullptr;
Anim* BugImgCentipede::sBodyAnim = nullptr;
Anim* BugImgCentipede::sTailAnim = nullptr;

// The pictures: a cel is 65 by 63 for the head, 43 by 49 for a body and 37 by 33 for the tail, 90 cels each
static const int HEAD_CEL_WIDTH = 0x41;
static const int HEAD_CEL_HEIGHT = 0x3f;
static const int BODY_CEL_WIDTH = 0x2b;
static const int BODY_CEL_HEIGHT = 0x31;
static const int TAIL_CEL_WIDTH = 0x25;
static const int TAIL_CEL_HEIGHT = 0x21;
static const int CENTIPEDE_CELS = 0x5a;

// The head's picture is drawn 7 pixels over and 4 up from its place
static const int HEAD_OFFSET_X = 7;
static const int HEAD_OFFSET_Y = -4;

// The idle track: 60 frames, the cel counting up from 0 to 29 every 30 (a pace of 30 and an amount of 29), at depth 2
static const int IDLE_FRAMES = 0x3c;
static const float IDLE_DEPTH = 2.0f;
static const float IDLE_PACE = 30.0f;
static const float IDLE_AMOUNT = 29.0f;

// The sway: each key is this long. The body goes 12, 24 and 12 pixels down (then up) tilting 45 degrees (0.785398 radians)
// each way, and its depth falls from 2 to -12 and comes back; the cels
// run on from 30 to 59 in the first half and from 60 to 90 in the second
static const int SWAY_KEY_FRAMES = 0x2d;
static const float SWAY_DEPTH_TOP = 2.0f;
static const float SWAY_DEPTH_MIDDLE = -6.0f;
static const float SWAY_DEPTH_LOW = -12.0f;
static const float SWAY_STEP = 12.0f;
static const float SWAY_FAR = 24.0f;
static const float SWAY_TILT = 0.7853981852531433f;

// 0x422260: the body on the BugImg base, with the picture of its role (the tail's for the role 2, the head's for 0, and a
// body's for any other; the head is also moved 7 pixels over and 4 up), the idle track, the two halves of the sway built,
// and the idle track started from the first cel. It is shown
BugImgCentipede::BugImgCentipede(int theRole)
{
	mRole = theRole;
	Anim* aAnim;
	if (theRole == CENTIPEDE_ROLE_HEAD) {
		aAnim = sHeadAnim;
		aAnim->AddRef();
		mAnim = aAnim;
		SetStateInt(STATE_OFFSET_X, HEAD_OFFSET_X);
		SetStateInt(STATE_OFFSET_Y, HEAD_OFFSET_Y);
	} else if (theRole == CENTIPEDE_ROLE_TAIL) {
		aAnim = sTailAnim;
		aAnim->AddRef();
		mAnim = aAnim;
	} else {
		aAnim = sBodyAnim;
		aAnim->AddRef();
		mAnim = aAnim;
	}
	BuildIdleTrack();
	BuildSwayDownTrack();
	BuildSwayUpTrack();
	SetStateInt(STATE_FRAME, 0);
	StartTrack(mIdleTrack);
	mUnused2B4 = false;
	Activate(this, false);
}

// 0x422350 (0x422370 is the body)
BugImgCentipede::~BugImgCentipede()
{
}

// 0x422380: the ends of the body's keys: the downward sway's end starts the upward half, and the upward half's end goes back
// to the first cel and the idle track
void BugImgCentipede::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case CENTIPEDE_EVENT_SWAY_DOWN_DONE:
		StartTrack(mSwayUpTrack);
		break;
	case CENTIPEDE_EVENT_SWAY_UP_DONE:
		SetStateInt(STATE_FRAME, 0);
		StartTrack(mIdleTrack);
		break;
	}
}

// 0x4219c0 (slot 5; shared, see BugShared): ObjBase's update
bool BugImgCentipede::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x4223d0: the two sway commands start their halves of the sway; any other is BugImg's (the stars and the ring)
void BugImgCentipede::Slot13(int theCommand, int theFrames, int theUnused)
{
	if (theCommand == BUG_CMD_CENTIPEDE_SWAY_DOWN) {
		StartTrack(mSwayDownTrack);
		return;
	}
	if (theCommand == BUG_CMD_CENTIPEDE_SWAY_UP) {
		StartTrack(mSwayUpTrack);
		return;
	}
	BugImg::Slot13(theCommand, theFrames, theUnused);
}

// 0x422410: the idle track: one key of 60 frames at depth 2 on the spot, counting the cels round (MOD_COUNT_UP), and the key
// is its own next, so it never ends
void BugImgCentipede::BuildIdleTrack()
{
	AnimKey* aKey = new AnimKey();
	aKey->SetDuration(IDLE_FRAMES);
	aKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey->AddValue(STATE_Z, IDLE_DEPTH, MOD_TOWARD_NEXT);
	aKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, IDLE_PACE, IDLE_AMOUNT, 0.0f, nullptr);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	mIdleTrack = AddTrack(aTrack, nullptr);
}

// 0x422510: the first half of the sway: five keys of 45 frames, going down 12, 24, 12 and 0 pixels (tilting 45 degrees
// the one way at the first, the other at the last) with the depth falling from 2 to -12, the cels 30, 38, 46, 53 and 59,
// and the last raising event 1
void BugImgCentipede::BuildSwayDownTrack()
{
	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(SWAY_KEY_FRAMES);
	aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Z, SWAY_DEPTH_TOP, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_FRAME, 30.0f, MOD_TOWARD_NEXT);

	AnimKey* aFirstTilt = new AnimKey();
	aFirstTilt->SetDuration(SWAY_KEY_FRAMES);
	aFirstTilt->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_Y, SWAY_STEP, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_Z, 0.0f, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_ROTATION, -SWAY_TILT, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_FRAME, 38.0f, MOD_TOWARD_NEXT);

	AnimKey* aLow = new AnimKey();
	aLow->SetDuration(SWAY_KEY_FRAMES);
	aLow->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLow->AddValue(STATE_Y, SWAY_FAR, MOD_TOWARD_NEXT);
	aLow->AddValue(STATE_Z, SWAY_DEPTH_MIDDLE, MOD_TOWARD_NEXT);
	aLow->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aLow->AddValue(STATE_FRAME, 46.0f, MOD_TOWARD_NEXT);

	AnimKey* aSecondTilt = new AnimKey();
	aSecondTilt->SetDuration(SWAY_KEY_FRAMES);
	aSecondTilt->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_Y, SWAY_STEP, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_Z, SWAY_DEPTH_LOW, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_ROTATION, SWAY_TILT, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_FRAME, 53.0f, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = CENTIPEDE_EVENT_SWAY_DOWN_DONE;
	aEnd->SetDuration(SWAY_KEY_FRAMES);
	aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Z, SWAY_DEPTH_LOW, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_FRAME, 59.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStart, nullptr);
	aTrack->AddKey(aFirstTilt, nullptr);
	aTrack->AddKey(aLow, nullptr);
	aTrack->AddKey(aSecondTilt, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mSwayDownTrack = AddTrack(aTrack, nullptr);
}

// 0x422a10: the second half of the sway: five keys of 45 frames, going up 12, 24, 12 and 0 pixels (tilting 45 degrees the
// one way at the second, the other at the fourth) with the depth rising from -12 back to 2, the cels 60, 68, 76, 83 and 90,
// and the last raising event 2. The last key is its own next
void BugImgCentipede::BuildSwayUpTrack()
{
	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(SWAY_KEY_FRAMES);
	aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Z, SWAY_DEPTH_LOW, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_FRAME, 60.0f, MOD_TOWARD_NEXT);

	AnimKey* aFirstTilt = new AnimKey();
	aFirstTilt->SetDuration(SWAY_KEY_FRAMES);
	aFirstTilt->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_Y, -SWAY_STEP, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_Z, SWAY_DEPTH_LOW, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_ROTATION, SWAY_TILT, MOD_TOWARD_NEXT);
	aFirstTilt->AddValue(STATE_FRAME, 68.0f, MOD_TOWARD_NEXT);

	AnimKey* aHigh = new AnimKey();
	aHigh->SetDuration(SWAY_KEY_FRAMES);
	aHigh->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aHigh->AddValue(STATE_Y, -SWAY_FAR, MOD_TOWARD_NEXT);
	aHigh->AddValue(STATE_Z, SWAY_DEPTH_MIDDLE, MOD_TOWARD_NEXT);
	aHigh->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aHigh->AddValue(STATE_FRAME, 76.0f, MOD_TOWARD_NEXT);

	AnimKey* aSecondTilt = new AnimKey();
	aSecondTilt->SetDuration(SWAY_KEY_FRAMES);
	aSecondTilt->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_Y, -SWAY_STEP, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_Z, 0.0f, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_ROTATION, -SWAY_TILT, MOD_TOWARD_NEXT);
	aSecondTilt->AddValue(STATE_FRAME, 83.0f, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = CENTIPEDE_EVENT_SWAY_UP_DONE;
	aEnd->SetDuration(SWAY_KEY_FRAMES);
	aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Z, SWAY_DEPTH_TOP, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_FRAME, 90.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStart, nullptr);
	aTrack->AddKey(aFirstTilt, nullptr);
	aTrack->AddKey(aHigh, nullptr);
	aTrack->AddKey(aSecondTilt, nullptr);
	aTrack->AddKey(aEnd, aEnd);
	mSwayUpTrack = AddTrack(aTrack, nullptr);
}

// 0x422f10 (GameWidget's preloads): the head's picture, the body's and the tail's
void Sexy::PreloadCentipede()
{
	BugImgCentipede::sHeadAnim = Anim::Get("images/centipede_head", ANIM_SCALE_SMOOTH, false, HEAD_CEL_WIDTH, HEAD_CEL_HEIGHT,
		CENTIPEDE_CELS, false);
	BugImgCentipede::sBodyAnim = Anim::Get("images/centipede_body", ANIM_SCALE_SMOOTH, false, BODY_CEL_WIDTH, BODY_CEL_HEIGHT,
		CENTIPEDE_CELS, false);
	BugImgCentipede::sTailAnim = Anim::Get("images/centipede_tail", ANIM_SCALE_SMOOTH, false, TAIL_CEL_WIDTH, TAIL_CEL_HEIGHT,
		CENTIPEDE_CELS, false);
}
