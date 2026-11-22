#include "BugImgBee.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgBee::sWaspAnim = nullptr;

// The picture: a cel is 95 by 95 (30 of them)
static const int WASP_CEL_SIZE = 0x5f;
static const int WASP_CELS = 0x1e;

// The shake: each key is this many frames and eases the place toward the next key's: the first key's values are -2 pixels
// across and down, the second's 2; the cels count up (a cel each 70 frames, wrapping at 30)
static const int SHAKE_FRAMES = 0x46;
static const float SHAKE_FIRST_OFFSET = -2.0f;
static const float SHAKE_SECOND_OFFSET = 2.0f;
static const float SHAKE_CEL_RATE = 70.0f;
static const float SHAKE_CELS = 30.0f;
// The dance: the cels count up from the 12th, 30 of them, at 50 to the cel; it lasts 800 frames unless the end of a freeze
// cuts it short, and then it holds the first cel 75 frames
static const int DANCE_FRAMES = 800;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x4206b0: the body on the BugImg base, with the shake built (the first track, which Activate starts) and the dance built,
// and shown
BugImgBee::BugImgBee()
{
	mUnused294 = false;
	mShakeFirstKey = new AnimKey();
	mShakeFirstKey->mEventId = BEE_EVENT_SHAKE;
	sWaspAnim->AddRef();
	mShakeFirstKey->mAnim = sWaspAnim;
	mShakeFirstKey->SetDuration(SHAKE_FRAMES);
	mShakeFirstKey->AddValue(STATE_X, SHAKE_FIRST_OFFSET, MOD_TOWARD_NEXT);
	mShakeFirstKey->AddValue(STATE_Y, SHAKE_FIRST_OFFSET, MOD_TOWARD_NEXT);
	mShakeFirstKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, SHAKE_CEL_RATE, SHAKE_CELS, 0.0f, nullptr);

	mShakeSecondKey = new AnimKey();
	mShakeSecondKey->mEventId = BEE_EVENT_SHAKE;
	sWaspAnim->AddRef();
	mShakeSecondKey->mAnim = sWaspAnim;
	mShakeSecondKey->SetDuration(SHAKE_FRAMES);
	mShakeSecondKey->AddValue(STATE_X, SHAKE_SECOND_OFFSET, MOD_TOWARD_NEXT);
	mShakeSecondKey->AddValue(STATE_Y, SHAKE_SECOND_OFFSET, MOD_TOWARD_NEXT);
	mShakeSecondKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, SHAKE_CEL_RATE, SHAKE_CELS, 0.0f, nullptr);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mShakeFirstKey, nullptr);
	aTrack->AddKey(mShakeSecondKey, mShakeFirstKey);
	mShakeTrack = AddTrack(aTrack, nullptr);
	BuildDanceTrack();
	Activate(this, false);
}

// 0x420920 (0x420940 is the body)
BugImgBee::~BugImgBee()
{
}

// 0x4237d0 (slot 4; shared): the end of the dance (event 2) starts the shake again
void BugImgBee::Notify(ObjBase* theObj)
{
	BugSharedImgNotify(this, theObj, mShakeTrack);
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgBee::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x420950 (slot 13): the start of a freeze (100) does nothing, the end of it (0x65) cuts the dance short; any other command
// is BugImg's (the stars and the ring)
void BugImgBee::Slot13(int theCommand, int theFrames, int theUnused)
{
	if (theCommand == BUG_CMD_FREEZE_ON)
		return;
	if (theCommand != BUG_CMD_FREEZE_OFF) {
		BugImg::Slot13(theCommand, theFrames, theUnused);
		return;
	}
	mDanceKey->SetDuration(0);
}

// 0x420980: the dance (which nothing starts: the brain never sends the freeze's start): the cels count up for 800 frames
// while the turn eases toward the next key's, then 75 frames holding the first cel, then a key of no length that raises event 2
void BugImgBee::BuildDanceTrack()
{
	mDanceKey = new AnimKey();
	sWaspAnim->AddRef();
	mDanceKey->mAnim = sWaspAnim;
	mDanceKey->SetDuration(DANCE_FRAMES);
	mDanceKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DANCE_RATE, DANCE_AMOUNT, DANCE_BASE, nullptr);
	mDanceKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aHold = new AnimKey();
	sWaspAnim->AddRef();
	aHold->mAnim = sWaspAnim;
	aHold->SetDuration(DANCE_END_FRAMES);
	aHold->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = BEE_EVENT_DANCE_DONE;
	sWaspAnim->AddRef();
	aEnd->mAnim = sWaspAnim;
	aEnd->SetDuration(0);
	aEnd->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mDanceKey, nullptr);
	aTrack->AddKey(aHold, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mDanceTrack = AddTrack(aTrack, nullptr);
}

// 0x420b30 (GameWidget's preloads): the picture
void Sexy::PreloadWasp()
{
	BugImgBee::sWaspAnim = Anim::Get("images/wasp", ANIM_SCALE_SMOOTH, false, WASP_CEL_SIZE, WASP_CEL_SIZE, WASP_CELS, false);
}
