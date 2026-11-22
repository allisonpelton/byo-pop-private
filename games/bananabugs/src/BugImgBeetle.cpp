#include "BugImgBeetle.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgBeetle::sWalkAnim = nullptr;
Anim* BugImgBeetle::sTurnAnim = nullptr;
Anim* BugImgBeetle::sDanceAnim = nullptr;

// The pictures: a cel is 96 by 79 (the turn's 96 by 87, the dance's 96 by 96), 30 cels each
static const int BEETLE_CEL_WIDTH = 0x60;
static const int BEETLE_WALK_CEL_HEIGHT = 0x4f;
static const int BEETLE_TURN_CEL_HEIGHT = 0x57;
static const int BEETLE_DANCE_CEL_HEIGHT = 0x60;
static const int BEETLE_CELS = 0x1e;

// The body hangs this far to one side of the wall (above is minus)
static const float SIDE_OFFSET = 32.0f;
// The last cel of the walk and the turn (the cels are numbered from 0)
static const float LAST_CEL = 29.0f;
// A step along the wall is this many frames
static const int WALK_FRAMES = 0x50;
// A flip over the wall takes this many frames to the middle, and as many again to the end
static const int FLIP_FRAMES = 0x46;
// The flip tilts the beetle by 25 degrees (0.436332 radians) either way
static const float FLIP_TILT = 0.4363323152065277f;
// The dance: the cels play back and forth from the 11th, 30 of them, at 50 to the cel; it lasts 800 frames unless the
// freeze says how long, and it ends after 75 more
static const int DANCE_FRAMES = 0x320;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x420b50: the body on the BugImg base, with the walk, the dance and the two flips built and shown
BugImgBeetle::BugImgBeetle()
{
	mUnused290 = false;
	mFlipRequested = false;
	mUnused2A4 = false;
	mStepDone = false;
	mDanceFrames = 0;
	BuildWalkTrack();
	BuildDanceTrack();
	BuildFlipToBottomTrack();
	BuildFlipToTopTrack();
	Activate(this, false);
}

// 0x420be0 (0x420c00 is the body)
BugImgBeetle::~BugImgBeetle()
{
}

// 0x420c10: the ends of the body's keys. The walk's end (event 1) walks on, or flips over when the brain has asked; the
// flips' ends put the walk on the side the body is on now; the middle of a flip mirrors the picture; the dance's end walks
// again
void BugImgBeetle::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case BEETLE_EVENT_STEP_DONE:
		SetStateInt(STATE_FRAME, 0);
		if (!mFlipRequested) {
			StartTrack(mWalkTrack);
			mStepDone = true;
			return;
		}
		mStepDone = false;
		mFlipRequested = false;
		if (0.0f <= mState[STATE_Y])
			StartTrack(mFlipToTopTrack);
		else
			StartTrack(mFlipToBottomTrack);
		return;
	case BEETLE_EVENT_FLIPPED_BOTTOM:
		SetStateInt(STATE_FRAME, 0);
		mWalkKey->mValue[STATE_Y] = SIDE_OFFSET;
		mWalkEndKey->mValue[STATE_Y] = SIDE_OFFSET;
		StartTrack(mWalkTrack);
		return;
	case BEETLE_EVENT_FLIPPED_TOP:
		SetStateInt(STATE_FRAME, 0);
		mWalkKey->mValue[STATE_Y] = -SIDE_OFFSET;
		mWalkEndKey->mValue[STATE_Y] = -SIDE_OFFSET;
		StartTrack(mWalkTrack);
		return;
	case BEETLE_EVENT_MIRROR:
		SetState(STATE_SCALE_Y, (float)((double)mBase[STATE_SCALE_Y] * -1.0));
		return;
	case BEETLE_EVENT_DANCE_DONE:
		SetStateInt(STATE_FRAME, 0);
		StartTrack(mWalkTrack);
		return;
	}
}

// 0x420d60: a dance that was asked for starts as soon as a step has ended, for as many frames as it was asked
bool BugImgBeetle::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mDanceFrames != 0 && mStepDone) {
		mDanceKey->SetDuration(mDanceFrames);
		StartTrack(mDanceTrack);
		mStepDone = false;
		mDanceFrames = 0;
	}
	return true;
}

// 0x420db0: BugImg's (the stars and the ring), then the beetle's own: a freeze asks for a dance of as many frames, its end
// cuts the dance short, and the flip command asks for a flip at the end of the step
void BugImgBeetle::Slot13(int theCommand, int theFrames, int theUnused)
{
	BugImg::Slot13(theCommand, theFrames, theUnused);
	if (theCommand == BUG_CMD_FREEZE_ON) {
		mDanceFrames = theFrames;
	} else if (theCommand == BUG_CMD_FREEZE_OFF) {
		mDanceKey->SetDuration(0);
	} else if (theCommand == BUG_CMD_BEETLE_FLIP) {
		mStepDone = false;
		mFlipRequested = true;
	}
}

// 0x420e10: the walk: 80 frames on the upper side, cels 0 to 29, then a frame at the last cel that raises event 1 (the
// track's end)
void BugImgBeetle::BuildWalkTrack()
{
	mWalkKey = new AnimKey();
	sWalkAnim->AddRef();
	mWalkKey->mAnim = sWalkAnim;
	mWalkKey->SetDuration(WALK_FRAMES);
	mWalkKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_Y, -SIDE_OFFSET, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mWalkEndKey = new AnimKey();
	sWalkAnim->AddRef();
	mWalkEndKey->mAnim = sWalkAnim;
	mWalkEndKey->mEventId = BEETLE_EVENT_STEP_DONE;
	mWalkEndKey->SetDuration(1);
	mWalkEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_Y, -SIDE_OFFSET, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_FRAME, LAST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mWalkKey, nullptr);
	aTrack->AddKey(mWalkEndKey, nullptr);
	mWalkTrack = AddTrack(aTrack, nullptr);
}

// 0x421050: the flip from the upper side to the lower: the turn picture, 70 frames from Y -32 (cel 0), 70 frames
// tilting to -25 degrees and cel 29 with the mirror event in the middle, then a frame at Y +32 (event 2)
void BugImgBeetle::BuildFlipToBottomTrack()
{
	AnimKey* aRise = new AnimKey();
	sTurnAnim->AddRef();
	aRise->mAnim = sTurnAnim;
	aRise->SetDuration(FLIP_FRAMES);
	aRise->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_Y, -SIDE_OFFSET, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aTilt = new AnimKey();
	aTilt->mEventId = BEETLE_EVENT_MIRROR;
	sTurnAnim->AddRef();
	aTilt->mAnim = sTurnAnim;
	aTilt->SetDuration(FLIP_FRAMES);
	aTilt->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_ROTATION, -FLIP_TILT, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_FRAME, LAST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aLand = new AnimKey();
	sTurnAnim->AddRef();
	aLand->mAnim = sTurnAnim;
	aLand->mEventId = BEETLE_EVENT_FLIPPED_BOTTOM;
	aLand->SetDuration(1);
	aLand->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_Y, SIDE_OFFSET, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aRise, nullptr);
	aTrack->AddKey(aTilt, nullptr);
	aTrack->AddKey(aLand, nullptr);
	mFlipToBottomTrack = AddTrack(aTrack, nullptr);
}

// 0x421310: the flip from the lower side to the upper: the same, from Y +32, tilting to +25 degrees, landing at -32
// with event 3
void BugImgBeetle::BuildFlipToTopTrack()
{
	AnimKey* aRise = new AnimKey();
	sTurnAnim->AddRef();
	aRise->mAnim = sTurnAnim;
	aRise->SetDuration(FLIP_FRAMES);
	aRise->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_Y, SIDE_OFFSET, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aTilt = new AnimKey();
	aTilt->mEventId = BEETLE_EVENT_MIRROR;
	sTurnAnim->AddRef();
	aTilt->mAnim = sTurnAnim;
	aTilt->SetDuration(FLIP_FRAMES);
	aTilt->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_ROTATION, FLIP_TILT, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_FRAME, LAST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aLand = new AnimKey();
	sTurnAnim->AddRef();
	aLand->mAnim = sTurnAnim;
	aLand->mEventId = BEETLE_EVENT_FLIPPED_TOP;
	aLand->SetDuration(1);
	aLand->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_Y, -SIDE_OFFSET, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aRise, nullptr);
	aTrack->AddKey(aTilt, nullptr);
	aTrack->AddKey(aLand, nullptr);
	mFlipToTopTrack = AddTrack(aTrack, nullptr);
}

// 0x4215d0: the dance: the dance cels play back and forth (the frame wave: from cel 11, 30 of them, 50 to the cel) for
// the length the freeze asks for (the key's length is set when the dance starts), then 75 frames returning to the
// first cel from where it was, then a key of no length that raises event 5 and walks again
void BugImgBeetle::BuildDanceTrack()
{
	mDanceKey = new AnimKey();
	sDanceAnim->AddRef();
	mDanceKey->mAnim = sDanceAnim;
	mDanceKey->SetDuration(DANCE_FRAMES);
	mDanceKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_PING_PONG, DANCE_RATE, DANCE_AMOUNT, DANCE_BASE, nullptr);

	AnimKey* aSettle = new AnimKey();
	sDanceAnim->AddRef();
	aSettle->mAnim = sDanceAnim;
	aSettle->SetDuration(DANCE_END_FRAMES);
	aSettle->AddModifier(STATE_FRAME, true, 0.0f, MOD_TOWARD_NEXT, 0.0f, 0.0f, 0.0f, nullptr);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = BEETLE_EVENT_DANCE_DONE;
	sDanceAnim->AddRef();
	aEnd->mAnim = sDanceAnim;
	aEnd->SetDuration(0);
	aEnd->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mDanceKey, nullptr);
	aTrack->AddKey(aSettle, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mDanceTrack = AddTrack(aTrack, nullptr);
}

// 0x421750 (GameWidget's preloads): the three pictures
void Sexy::PreloadBeetle()
{
	BugImgBeetle::sWalkAnim = Anim::Get("images/beetle_walk", ANIM_SCALE_SMOOTH, false, BEETLE_CEL_WIDTH, BEETLE_WALK_CEL_HEIGHT, BEETLE_CELS, false);
	BugImgBeetle::sTurnAnim = Anim::Get("images/beetle_turn", ANIM_SCALE_SMOOTH, false, BEETLE_CEL_WIDTH, BEETLE_TURN_CEL_HEIGHT, BEETLE_CELS, false);
	BugImgBeetle::sDanceAnim = Anim::Get("images/beetle_dance", ANIM_SCALE_SMOOTH, false, BEETLE_CEL_WIDTH, BEETLE_DANCE_CEL_HEIGHT, BEETLE_CELS, false);
}
