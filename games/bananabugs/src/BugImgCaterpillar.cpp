#include "BugImgCaterpillar.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgCaterpillar::sHeadAnim = nullptr;
Anim* BugImgCaterpillar::sBodyAnim = nullptr;

// The pictures: a cel is 36 by 45 for the head and 36 by 36 for a body, 30 cels each
static const int CATERPILLAR_CEL_WIDTH = 0x24;
static const int CATERPILLAR_HEAD_CEL_HEIGHT = 0x2d;
static const int CATERPILLAR_BODY_CEL_HEIGHT = 0x24;
static const int CATERPILLAR_CELS = 0x1e;

// Setup's role for the first bug of a column, which has the head's picture
static const int ROLE_HEAD = 0;

// The body hangs this far to one side of the wall while it walks (above is minus), and this far while it turns
static const float WALK_SIDE_OFFSET = 28.0f;
static const float FLIP_SIDE_OFFSET = 25.0f;
// A step along the wall: 25 frames, then 55 swinging the body 8 pixels back to cel 15, then a frame at the first cel
static const int WALK_FIRST_FRAMES = 0x19;
static const int WALK_SECOND_FRAMES = 0x37;
static const float WALK_SWING = -8.0f;
static const float WALK_MIDDLE_CEL = 15.0f;
// A flip over the wall takes this many frames to the middle and this many to the end, and one frame to land; the
// cels play from the 17th to the 30th and back to the 17th
static const int FLIP_FRAMES = 0x40;
static const float FLIP_FIRST_CEL = 16.0f;
static const float FLIP_LAST_CEL = 29.0f;
// The flip swings the body 8 pixels and tilts it by 25 degrees (0.436332 radians) in the middle
static const float FLIP_SWING = 8.0f;
static const float FLIP_TILT = 0.4363323152065277f;

// 0x4217b0: the body on the BugImg base, with the head's picture for the role 0 and the body's for any other, and the walk
// and the two flips built. It is shown
BugImgCaterpillar::BugImgCaterpillar(int theRole)
{
	mUnused290 = false;
	mFlipRequested = false;
	mUnused2A4 = false;
	Anim* aAnim = (theRole == ROLE_HEAD) ? sHeadAnim : sBodyAnim;
	aAnim->AddRef();
	mAnim = aAnim;
	BuildWalkTrack();
	BuildFlipToBottomTrack();
	BuildFlipToTopTrack();
	Activate(this, false);
}

// 0x421850 (0x421870 is the body)
BugImgCaterpillar::~BugImgCaterpillar()
{
}

// 0x421880: the ends of the body's keys. The walk's end (event 1) walks on, or flips over when the brain has asked; the
// flips' ends put the walk on the side the body is on now (the three keys' Y: 25, 28 and 25 below the wall, or minus
// those above it) and walk; the middle of a flip mirrors the picture
void BugImgCaterpillar::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case CATERPILLAR_EVENT_STEP_DONE:
		SetStateInt(STATE_FRAME, 0);
		if (mFlipRequested) {
			mFlipRequested = false;
			if (mState[STATE_Y] < 0.0f)
				StartTrack(mFlipToBottomTrack);
			else
				StartTrack(mFlipToTopTrack);
			return;
		}
		StartTrack(mWalkTrack);
		return;
	case CATERPILLAR_EVENT_FLIPPED_BOTTOM:
		SetStateInt(STATE_FRAME, 0);
		mWalkKey->mValue[STATE_Y] = FLIP_SIDE_OFFSET;
		mWalkMiddleKey->mValue[STATE_Y] = WALK_SIDE_OFFSET;
		mWalkEndKey->mValue[STATE_Y] = FLIP_SIDE_OFFSET;
		StartTrack(mWalkTrack);
		return;
	case CATERPILLAR_EVENT_FLIPPED_TOP:
		SetStateInt(STATE_FRAME, 0);
		mWalkKey->mValue[STATE_Y] = -FLIP_SIDE_OFFSET;
		mWalkMiddleKey->mValue[STATE_Y] = -WALK_SIDE_OFFSET;
		mWalkEndKey->mValue[STATE_Y] = -FLIP_SIDE_OFFSET;
		StartTrack(mWalkTrack);
		return;
	case CATERPILLAR_EVENT_MIRROR:
		SetState(STATE_SCALE_Y, (float)((double)mBase[STATE_SCALE_Y] * -1.0));
		return;
	}
}

// 0x4219c0 (slot 5; shared, see BugShared): ObjBase's update
bool BugImgCaterpillar::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x4219d0: the flip command asks for a flip at the end of the step; a freeze and its end are not answered (the original
// does not even tell BugImg's), and any other command is BugImg's (the stars and the ring)
void BugImgCaterpillar::Slot13(int theCommand, int theFrames, int theUnused)
{
	if (theCommand == BUG_CMD_FREEZE_ON || theCommand == BUG_CMD_FREEZE_OFF)
		return;
	if (theCommand != BUG_CMD_CATERPILLAR_FLIP) {
		BugImg::Slot13(theCommand, theFrames, theUnused);
		return;
	}
	mFlipRequested = true;
}

// 0x421a00: the walk: 25 frames on the upper side (cel 0), 55 frames swinging 8 pixels back to cel 15, then a frame at
// cel 0 that raises event 1 (the track's end; the keys carry no picture, the body's own is shown)
void BugImgCaterpillar::BuildWalkTrack()
{
	mWalkKey = new AnimKey();
	mWalkKey->SetDuration(WALK_FIRST_FRAMES);
	mWalkKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_Y, -WALK_SIDE_OFFSET, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mWalkMiddleKey = new AnimKey();
	mWalkMiddleKey->SetDuration(WALK_SECOND_FRAMES);
	mWalkMiddleKey->AddValue(STATE_X, WALK_SWING, MOD_TOWARD_NEXT);
	mWalkMiddleKey->AddValue(STATE_Y, -WALK_SIDE_OFFSET, MOD_TOWARD_NEXT);
	mWalkMiddleKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mWalkMiddleKey->AddValue(STATE_FRAME, WALK_MIDDLE_CEL, MOD_TOWARD_NEXT);

	mWalkEndKey = new AnimKey();
	mWalkEndKey->mEventId = CATERPILLAR_EVENT_STEP_DONE;
	mWalkEndKey->SetDuration(1);
	mWalkEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_Y, -WALK_SIDE_OFFSET, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mWalkKey, nullptr);
	aTrack->AddKey(mWalkMiddleKey, nullptr);
	aTrack->AddKey(mWalkEndKey, nullptr);
	mWalkTrack = AddTrack(aTrack, nullptr);
}

// 0x421ce0: the flip from the upper side to the lower: 64 frames from Y -25 (cel 16), 64 frames swinging 8 pixels and
// tilting to -25 degrees through cel 29 with the mirror event in the middle, then a frame at Y +25 (event 2)
void BugImgCaterpillar::BuildFlipToBottomTrack()
{
	AnimKey* aRise = new AnimKey();
	aRise->SetDuration(FLIP_FRAMES);
	aRise->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_Y, -FLIP_SIDE_OFFSET, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_FRAME, FLIP_FIRST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aTilt = new AnimKey();
	aTilt->mEventId = CATERPILLAR_EVENT_MIRROR;
	aTilt->SetDuration(FLIP_FRAMES);
	aTilt->AddValue(STATE_X, FLIP_SWING, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_ROTATION, -FLIP_TILT, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_FRAME, FLIP_LAST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aLand = new AnimKey();
	aLand->mEventId = CATERPILLAR_EVENT_FLIPPED_BOTTOM;
	aLand->SetDuration(1);
	aLand->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_Y, FLIP_SIDE_OFFSET, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_FRAME, FLIP_FIRST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aRise, nullptr);
	aTrack->AddKey(aTilt, nullptr);
	aTrack->AddKey(aLand, nullptr);
	mFlipToBottomTrack = AddTrack(aTrack, nullptr);
}

// 0x421f80: the flip from the lower side to the upper: the same, from Y +25, swinging 8 pixels the other way and tilting to
// +25 degrees, landing at -25 with event 3
void BugImgCaterpillar::BuildFlipToTopTrack()
{
	AnimKey* aRise = new AnimKey();
	aRise->SetDuration(FLIP_FRAMES);
	aRise->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_Y, FLIP_SIDE_OFFSET, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_FRAME, FLIP_FIRST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aTilt = new AnimKey();
	aTilt->mEventId = CATERPILLAR_EVENT_MIRROR;
	aTilt->SetDuration(FLIP_FRAMES);
	aTilt->AddValue(STATE_X, -FLIP_SWING, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_ROTATION, FLIP_TILT, MOD_TOWARD_NEXT);
	aTilt->AddValue(STATE_FRAME, FLIP_LAST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aLand = new AnimKey();
	aLand->mEventId = CATERPILLAR_EVENT_FLIPPED_TOP;
	aLand->SetDuration(1);
	aLand->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_Y, -FLIP_SIDE_OFFSET, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aLand->AddValue(STATE_FRAME, FLIP_FIRST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aRise, nullptr);
	aTrack->AddKey(aTilt, nullptr);
	aTrack->AddKey(aLand, nullptr);
	mFlipToTopTrack = AddTrack(aTrack, nullptr);
}

// 0x422220 (GameWidget's preloads): the head's picture and the body's
void Sexy::PreloadCaterpillar()
{
	BugImgCaterpillar::sHeadAnim = Anim::Get("images/caterpillar_head", ANIM_SCALE_SMOOTH, false, CATERPILLAR_CEL_WIDTH,
		CATERPILLAR_HEAD_CEL_HEIGHT, CATERPILLAR_CELS, false);
	BugImgCaterpillar::sBodyAnim = Anim::Get("images/caterpillar_body", ANIM_SCALE_SMOOTH, false, CATERPILLAR_CEL_WIDTH,
		CATERPILLAR_BODY_CEL_HEIGHT, CATERPILLAR_CELS, false);
}
