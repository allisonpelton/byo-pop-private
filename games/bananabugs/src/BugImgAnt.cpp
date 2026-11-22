#include "BugImgAnt.h"

#include "Anim.h"
#include "BugBrainAnt.h"

using namespace Sexy;

Anim* BugImgAnt::sWalkAnim = nullptr;
Anim* BugImgAnt::sTurnAnim = nullptr;
Anim* BugImgAnt::sDanceAnim = nullptr;

// The pictures: a cel is 96 by 73 (the dance's 96 by 96), 30 cels each
static const int ANT_CEL_WIDTH = 0x60;
static const int ANT_CEL_HEIGHT = 0x49;
static const int ANT_DANCE_CEL_HEIGHT = 0x60;
static const int ANT_CELS = 0x1e;

// The body hangs this far to one side of the wall (above is minus)
static const float SIDE_OFFSET = 32.0f;
// The last cel of the walk and the turn (the cels are numbered from 0)
static const float LAST_CEL = 29.0f;
// A step along the wall is this many frames
static const int WALK_FRAMES = 0x50;
// A flip over the wall takes this many frames to the middle, and one for the end
static const int FLIP_FRAMES = 0x32;
// The flip tilts the ant by 25 degrees (0.436332 radians) either way
static const float FLIP_TILT = 0.4363323152065277f;
// The dance: the cels play back and forth from the 11th, 30 of them, at 50 to the cel; it lasts 800 frames unless the
// freeze says how long, and it ends after 75 more
static const int DANCE_FRAMES = 0x320;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x41f9c0: the body on the BugImg base, with the walk, the dance and the two flips built and shown
BugImgAnt::BugImgAnt()
{
	mUnused290 = false;
	mFlipRequested = false;
	mUnused2A8 = false;
	mBrain = nullptr;
	mStepDone = false;
	mDanceFrames = 0;
	BuildWalkTrack();
	BuildDanceTrack();
	BuildFlipToBottomTrack();
	BuildFlipToTopTrack();
	Activate(this, false);
}

// 0x41fa60 (0x41fa80 is the body)
BugImgAnt::~BugImgAnt()
{
}

// 0x41fa90: the ends of the body's keys. The walk's end (event 1) walks on, or flips over when the brain has asked and
// the ant has not yet; the flips' ends put the walk on the side the body is on now and tell the brain it is done; the
// middle of a flip mirrors the picture; the dance's end walks again
void BugImgAnt::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case ANT_EVENT_STEP_DONE:
		SetStateInt(STATE_FRAME, 0);
		if (!mFlipRequested) {
			StartTrack(mWalkTrack);
			mStepDone = true;
			return;
		}
		mStepDone = false;
		mFlipRequested = false;
		if (mState[STATE_Y] < 0.0f)
			StartTrack(mFlipToBottomTrack);
		else
			StartTrack(mFlipToTopTrack);
		return;
	case ANT_EVENT_FLIPPED_BOTTOM:
	case ANT_EVENT_FLIPPED_TOP:
		SetStateInt(STATE_FRAME, 0);
		if (theObj->mCurrentKey->mEventId == ANT_EVENT_FLIPPED_BOTTOM) {
			mWalkKey->mValue[STATE_Y] = SIDE_OFFSET;
			mWalkEndKey->mValue[STATE_Y] = SIDE_OFFSET;
		} else {
			mWalkKey->mValue[STATE_Y] = -SIDE_OFFSET;
			mWalkEndKey->mValue[STATE_Y] = -SIDE_OFFSET;
		}
		StartTrack(mWalkTrack);
		if (mBrain != nullptr)
			mBrain->ClearTurnPhase();
		return;
	case ANT_EVENT_MIRROR:
		SetState(STATE_SCALE_Y, (float)((double)mBase[STATE_SCALE_Y] * -1.0));
		return;
	case ANT_EVENT_DANCE_DONE:
		SetStateInt(STATE_FRAME, 0);
		StartTrack(mWalkTrack);
		return;
	}
}

// 0x41fbe0: a dance that was asked for starts as soon as a step has ended, for as many frames as it was asked
bool BugImgAnt::Update()
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

// 0x41fc30: BugImg's (the stars and the ring), then the ant's own: a freeze asks for a dance of as many frames, its end
// cuts the dance short, and the flip command asks for a flip at the end of the step
void BugImgAnt::Slot13(int theCommand, int theFrames, int theUnused)
{
	BugImg::Slot13(theCommand, theFrames, theUnused);
	if (theCommand == BUG_CMD_FREEZE_ON) {
		mDanceFrames = theFrames;
	} else if (theCommand == BUG_CMD_FREEZE_OFF) {
		mDanceKey->SetDuration(0);
	} else if (theCommand == BUG_CMD_ANT_FLIP) {
		mStepDone = false;
		mFlipRequested = true;
	}
}

// 0x41fc90 (the original takes the brain in EAX): the brain is the body's parent, with no owner
void BugImgAnt::SetBrain(BugBrainAnt* theBrain)
{
	mBrain = theBrain;
	SetParent(theBrain, nullptr);
}

// 0x41fcb0: the first cel again, the walk's two keys swap sides (the sign of their Y) and the walk starts over
void BugImgAnt::FlipWalk()
{
	SetStateInt(STATE_FRAME, 0);
	mWalkKey->mValue[STATE_Y] = -mWalkKey->mValue[STATE_Y];
	mWalkEndKey->mValue[STATE_Y] = -mWalkEndKey->mValue[STATE_Y];
	StartTrack(mWalkTrack);
}

// 0x41fcf0: the walk: 80 frames on the upper side, cels 0 to 29, then a frame at the last cel that raises event 1 (the
// track's end; the original gets its picture by name and not from the preload)
void BugImgAnt::BuildWalkTrack()
{
	mWalkKey = new AnimKey();
	mWalkKey->mAnim = Anim::Get("images/ant_walk", ANIM_SCALE_SMOOTH, false, ANT_CEL_WIDTH, ANT_CEL_HEIGHT, ANT_CELS, false);
	mWalkKey->SetDuration(WALK_FRAMES);
	mWalkKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_Y, -SIDE_OFFSET, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mWalkEndKey = new AnimKey();
	mWalkEndKey->mAnim = Anim::Get("images/ant_walk", ANIM_SCALE_SMOOTH, false, ANT_CEL_WIDTH, ANT_CEL_HEIGHT, ANT_CELS, false);
	mWalkEndKey->mEventId = ANT_EVENT_STEP_DONE;
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

// 0x41ff50: the flip from the upper side to the lower: the turn picture, 50 frames from Y -32 (cel 0), 50 frames
// tilting to -25 degrees and cel 29 with the mirror event in the middle, then a frame at Y +32 (event 2)
void BugImgAnt::BuildFlipToBottomTrack()
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
	aTilt->mEventId = ANT_EVENT_MIRROR;
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
	aLand->mEventId = ANT_EVENT_FLIPPED_BOTTOM;
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

// 0x420210: the flip from the lower side to the upper: the same, from Y +32, tilting to +25 degrees, landing at -32
// with event 3
void BugImgAnt::BuildFlipToTopTrack()
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
	aTilt->mEventId = ANT_EVENT_MIRROR;
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
	aLand->mEventId = ANT_EVENT_FLIPPED_TOP;
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

// 0x4204d0: the dance: the dance cels play back and forth (the frame wave: from cel 11, 30 of them, 50 to the cel) for
// the length the freeze asks for (the key's length is set when the dance starts), then 75 frames returning to the
// first cel from where it was, then a key of no length that raises event 5 and walks again
void BugImgAnt::BuildDanceTrack()
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
	aEnd->mEventId = ANT_EVENT_DANCE_DONE;
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

// 0x420650 (GameWidget's preloads): the three pictures
void Sexy::PreloadAnt()
{
	BugImgAnt::sWalkAnim = Anim::Get("images/ant_walk", ANIM_SCALE_SMOOTH, false, ANT_CEL_WIDTH, ANT_CEL_HEIGHT, ANT_CELS, false);
	BugImgAnt::sTurnAnim = Anim::Get("images/ant_turn", ANIM_SCALE_SMOOTH, false, ANT_CEL_WIDTH, ANT_CEL_HEIGHT, ANT_CELS, false);
	BugImgAnt::sDanceAnim = Anim::Get("images/ant_dance", ANIM_SCALE_SMOOTH, false, ANT_CEL_WIDTH, ANT_DANCE_CEL_HEIGHT, ANT_CELS, false);
}
