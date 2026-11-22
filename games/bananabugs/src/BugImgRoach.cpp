#include "BugImgRoach.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgRoach::sWalkAnim = nullptr;

// The command 0x6a is the ant's and the beetle's flip: the roach (which never flips) ignores it
static const int BUG_CMD_FLIP_UNUSED = 0x6a;

// The picture: a cel is 96 by 64, 28 cels
static const int ROACH_CEL_WIDTH = 0x60;
static const int ROACH_CEL_HEIGHT = 0x40;
static const int ROACH_CELS = 0x1c;

// A step is this many frames, in which the cels run from the first to the 28th (a cel past the picture's last), and the
// roach wobbles by 6 degrees (0.10472 radians, 0x5e72bc) to a side and then the other
static const int WALK_FRAMES = 10;
static const float WALK_TURN = 0.10471975803375244f;
static const float WALK_LAST_CEL = 28.0f;
// The spin: it lasts as many frames as the freeze asks (800 until it does), three degrees for every frame, and ends after 75
// frames settling back to none; a turn of a whole circle (6.28319) is what the settling key starts from
static const int SPIN_FRAMES = 0x320;
static const int SPIN_DEGREES_A_FRAME = 3;
static const int SPIN_END_FRAMES = 0x4b;
static const float SPIN_START_TURN = 6.2831854820251465f;   // 0x5e7450

// 0x423d50: the body on the BugImg base, with the picture of the walk and the walk and the spin built and shown (the walk
// is the first track, which Activate starts)
BugImgRoach::BugImgRoach()
{
	mUnused290 = false;
	mUnused2A4 = false;
	sWalkAnim->AddRef();
	mAnim = sWalkAnim;
	BuildWalkTrack();
	BuildSpinTrack();
	Activate(this, false);
}

// 0x423dd0 (0x423df0 is the body)
BugImgRoach::~BugImgRoach()
{
}

// 0x423e00: the ends of the body's keys: the walk's end (event 1) starts the next step at the first cel, wobbling the way
// opposite to the turn the roach has now (a roach turned one way or not at all turns that way first); the spin's end (event
// 5) walks again
void BugImgRoach::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == ROACH_EVENT_STEP_DONE) {
		SetStateInt(STATE_FRAME, 0);
		float aOtherTurn;
		if (0.0f <= mState[STATE_ROTATION]) {
			mWalkKey->mValue[STATE_ROTATION] = WALK_TURN;
			aOtherTurn = -WALK_TURN;
		} else {
			mWalkKey->mValue[STATE_ROTATION] = -WALK_TURN;
			aOtherTurn = WALK_TURN;
		}
		mWalkEndKey->mValue[STATE_ROTATION] = aOtherTurn;
		StartTrack(mWalkTrack);
	} else if (anEvent == ROACH_EVENT_SPIN_DONE) {
		SetStateInt(STATE_FRAME, 0);
		StartTrack(mWalkTrack);
	}
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgRoach::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x423ea0: BugImg's (the stars and the ring), then the roach's own: a freeze asks for a spin of as many frames and a turn of
// three degrees a frame, its end cuts the spin short and the roach walks again. The commands 0x66, 0x67 and 0x6a are
// left at that. A bug of the original: the others (the ring's 0x68 and 0x69 and any other number) are handed to BugImg's
// a second time
void BugImgRoach::Slot13(int theCommand, int theFrames, int theUnused)
{
	BugImg::Slot13(theCommand, theFrames, theUnused);
	switch (theCommand) {
	case BUG_CMD_FREEZE_ON:
		mSpinKey->SetDuration(theFrames);
		mSpinTurnKey->mValue[STATE_ROTATION] = (float)((double)(theFrames * SPIN_DEGREES_A_FRAME) * PI_F / 180.0);
		StartTrack(mSpinTrack);
		return;
	case BUG_CMD_FREEZE_OFF:
		mSpinKey->SetDuration(0);
		mSpinTurnKey->SetDuration(0);
		SetStateInt(STATE_FRAME, 0);
		StartTrack(mWalkTrack);
		return;
	case BUG_CMD_STARS_ON:
	case BUG_CMD_STARS_OFF:
	case BUG_CMD_FLIP_UNUSED:
		break;
	default:
		BugImg::Slot13(theCommand, theFrames, theUnused);
	}
}

// 0x423f80: the walk: ten frames of the cels counting up from the first (to a 28th), the roach turning 6 degrees one way,
// then a frame at the 28th cel (the picture has no such cel; the original sets the number all the same) and 6 degrees the
// other way, that raises event 1 (the track's end)
void BugImgRoach::BuildWalkTrack()
{
	mWalkKey = new AnimKey();
	mWalkKey->SetDuration(WALK_FRAMES);
	mWalkKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_ROTATION, -WALK_TURN, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mWalkEndKey = new AnimKey();
	mWalkEndKey->mEventId = ROACH_EVENT_STEP_DONE;
	mWalkEndKey->SetDuration(1);
	mWalkEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_ROTATION, WALK_TURN, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_FRAME, WALK_LAST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mWalkKey, nullptr);
	aTrack->AddKey(mWalkEndKey, nullptr);
	mWalkTrack = AddTrack(aTrack, nullptr);
}

// 0x424180: the spin: the turn from none to the one the freeze asks for over the length it asks (the key's length is set when
// the spin starts), then 75 frames back to none, eased in as a square (the asked turn is set when the spin starts; the
// whole circle here is only a start), then a key of no length that raises event 5 and walks again
void BugImgRoach::BuildSpinTrack()
{
	mSpinKey = new AnimKey();
	mSpinKey->SetDuration(SPIN_FRAMES);
	mSpinKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mSpinTurnKey = new AnimKey();
	mSpinTurnKey->SetDuration(SPIN_END_FRAMES);
	mSpinTurnKey->AddValue(STATE_ROTATION, SPIN_START_TURN, MOD_TOWARD_NEXT);
	mSpinTurnKey->mEasing = EASE_IN_SQUARE;

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = ROACH_EVENT_SPIN_DONE;
	aEnd->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aEnd->SetDuration(0);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mSpinKey, nullptr);
	aTrack->AddKey(mSpinTurnKey, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mSpinTrack = AddTrack(aTrack, nullptr);
}

// 0x4242e0 (GameWidget's preloads): the picture of the walk
void Sexy::PreloadCockroach()
{
	BugImgRoach::sWalkAnim = Anim::Get("images/cockroach_walk", ANIM_SCALE_SMOOTH, false, ROACH_CEL_WIDTH, ROACH_CEL_HEIGHT, ROACH_CELS, false);
}
