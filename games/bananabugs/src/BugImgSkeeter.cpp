#include "BugImgSkeeter.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgSkeeter::sFlyAnim = nullptr;
Anim* BugImgSkeeter::sDanceAnim = nullptr;

// The pictures: a cel is 96 by 96, 30 cels each
static const int SKEETER_CEL_WIDTH = 0x60;
static const int SKEETER_CEL_HEIGHT = 0x60;
static const int SKEETER_CELS = 0x1e;

// The hover: each key is this many frames; the body drifts this far from the brain's place (the first key to the left and
// up, the second to the right and down); the cels count up 30 in 70 frames
static const int HOVER_FRAMES = 0x46;
static const float HOVER_DRIFT_X = 6.0f;
static const float HOVER_DRIFT_Y = 12.0f;
static const float HOVER_CEL_RATE = 70.0f;
static const float HOVER_CEL_AMOUNT = 30.0f;
// The dance: the cels play back and forth from the 11th, 30 of them, at 50 to the cel; it lasts 800 frames unless the
// freeze says how long, and it ends after 75 more
static const int DANCE_FRAMES = 0x320;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x424ca0: the body on the BugImg base, with the hover and the dance built and shown (the hover is the first track, which
// Activate starts)
BugImgSkeeter::BugImgSkeeter()
{
	mUnused294 = false;
	mHoverFirstKey = new AnimKey();
	mHoverFirstKey->mEventId = SKEETER_EVENT_HOVER_DONE;
	sFlyAnim->AddRef();
	mHoverFirstKey->mAnim = sFlyAnim;
	mHoverFirstKey->SetDuration(HOVER_FRAMES);
	mHoverFirstKey->AddValue(STATE_X, -HOVER_DRIFT_X, MOD_TOWARD_NEXT);
	mHoverFirstKey->AddValue(STATE_Y, -HOVER_DRIFT_Y, MOD_TOWARD_NEXT);
	mHoverFirstKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, HOVER_CEL_RATE, HOVER_CEL_AMOUNT, 0.0f, nullptr);

	mHoverSecondKey = new AnimKey();
	sFlyAnim->AddRef();
	mHoverSecondKey->mAnim = sFlyAnim;
	mHoverSecondKey->SetDuration(HOVER_FRAMES);
	mHoverSecondKey->AddValue(STATE_X, HOVER_DRIFT_X, MOD_TOWARD_NEXT);
	mHoverSecondKey->AddValue(STATE_Y, HOVER_DRIFT_Y, MOD_TOWARD_NEXT);
	mHoverSecondKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, HOVER_CEL_RATE, HOVER_CEL_AMOUNT, 0.0f, nullptr);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mHoverFirstKey, nullptr);
	aTrack->AddKey(mHoverSecondKey, mHoverFirstKey);
	mHoverTrack = AddTrack(aTrack, nullptr);
	BuildDanceTrack();
	Activate(this, false);
}

// 0x424f10 (0x424f30 is the body)
BugImgSkeeter::~BugImgSkeeter()
{
}

// 0x4237d0 (slot 4; shared): the end of the dance (event 2) starts the hover again
void BugImgSkeeter::Notify(ObjBase* theObj)
{
	BugSharedImgNotify(this, theObj, mHoverTrack);
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgSkeeter::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x424f40: a freeze starts the dance at once, for as many frames as it asks, and its end cuts the dance short; any other
// command is BugImg's (the stars and the ring)
void BugImgSkeeter::Slot13(int theCommand, int theFrames, int theUnused)
{
	if (theCommand == BUG_CMD_FREEZE_ON) {
		mDanceKey->SetDuration(theFrames);
		StartTrack(mDanceTrack);
		return;
	}
	if (theCommand != BUG_CMD_FREEZE_OFF) {
		BugImg::Slot13(theCommand, theFrames, theUnused);
		return;
	}
	mDanceKey->SetDuration(0);
}

// 0x424f90: the dance: the dance cels play back and forth (the frame wave: from cel 11, 30 of them, 50 to the cel) for
// the length the freeze asks for (the key's length is set when the dance starts), then 75 frames returning to the
// first cel from where it was, then a key of no length that raises event 2 and hovers again
void BugImgSkeeter::BuildDanceTrack()
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
	aEnd->mEventId = SKEETER_EVENT_DANCE_DONE;
	sDanceAnim->AddRef();
	aEnd->mAnim = sDanceAnim;
	aEnd->SetDuration(0);
	aEnd->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mDanceKey, nullptr);
	aTrack->AddKey(aSettle, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mDanceTrack = AddTrack(aTrack, nullptr);
}

// 0x425110 (GameWidget's preloads): the two pictures
void Sexy::PreloadSkeeter()
{
	BugImgSkeeter::sFlyAnim = Anim::Get("images/skeeter_fly", ANIM_SCALE_SMOOTH, false, SKEETER_CEL_WIDTH, SKEETER_CEL_HEIGHT, SKEETER_CELS, false);
	BugImgSkeeter::sDanceAnim = Anim::Get("images/skeeter_dance", ANIM_SCALE_SMOOTH, false, SKEETER_CEL_WIDTH, SKEETER_CEL_HEIGHT, SKEETER_CELS, false);
}
