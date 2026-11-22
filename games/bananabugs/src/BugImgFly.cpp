#include "BugImgFly.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgFly::sDanceAnim = nullptr;
Anim* BugImgFly::sFlyAnim = nullptr;

// The pictures: a cel of the dance is 96 by 96 (30 of them), of the wings 64 by 64 (6)
static const int DANCE_CEL_SIZE = 0x60;
static const int DANCE_CELS = 0x1e;
static const int FLY_CEL_SIZE = 0x40;
static const int FLY_CELS = 6;

// The wings: each key is this many frames; the cels go from the first to this one and back
static const int FLAP_FRAMES = 5;
static const float FLAP_WIDE_CEL = 3.0f;
// The dance: the cels play back and forth from the 11th, 30 of them, at 50 to the cel; it lasts 800 frames unless the
// freeze says how long, and it ends after 75 more
static const int DANCE_FRAMES = 800;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x423600: the body on the BugImg base, with the wings built (the first track, which Activate starts) and the dance built,
// and shown
BugImgFly::BugImgFly()
{
	mUnused294 = false;
	mFlapFirstKey = new AnimKey();
	mFlapFirstKey->mEventId = FLY_EVENT_FLAP;
	sFlyAnim->AddRef();
	mFlapFirstKey->mAnim = sFlyAnim;
	mFlapFirstKey->SetDuration(FLAP_FRAMES);
	mFlapFirstKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mFlapSecondKey = new AnimKey();
	mFlapSecondKey->mEventId = FLY_EVENT_FLAP;
	sFlyAnim->AddRef();
	mFlapSecondKey->mAnim = sFlyAnim;
	mFlapSecondKey->SetDuration(FLAP_FRAMES);
	mFlapSecondKey->AddValue(STATE_FRAME, FLAP_WIDE_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlapFirstKey, nullptr);
	aTrack->AddKey(mFlapSecondKey, mFlapFirstKey);
	mFlapTrack = AddTrack(aTrack, nullptr);
	BuildDanceTrack();
	Activate(this, false);
}

// 0x4237a0 (0x4237c0 is the body)
BugImgFly::~BugImgFly()
{
}

// 0x4237d0 (slot 4; shared): the end of the dance (event 2) starts the wings again
void BugImgFly::Notify(ObjBase* theObj)
{
	BugSharedImgNotify(this, theObj, mFlapTrack);
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgFly::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x424f40 (slot 13; shared): see BugSharedImgSlot13
void BugImgFly::Slot13(int theCommand, int theFrames, int theUnused)
{
	BugSharedImgSlot13(this, mDanceKey, mDanceTrack, theCommand, theFrames, theUnused);
}

// 0x423800: the dance: the dance cels play back and forth for the length the freeze asks for (set when the dance starts),
// then 75 frames returning to the first cel from where they were, then a key of no length that raises event 2
void BugImgFly::BuildDanceTrack()
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
	aEnd->mEventId = FLY_EVENT_DANCE_DONE;
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

// 0x423980 (GameWidget's preloads): the two pictures
void Sexy::PreloadFly()
{
	BugImgFly::sDanceAnim = Anim::Get("images/fly_dance", ANIM_SCALE_SMOOTH, false, DANCE_CEL_SIZE, DANCE_CEL_SIZE, DANCE_CELS, false);
	BugImgFly::sFlyAnim = Anim::Get("images/fly_fly", ANIM_SCALE_SMOOTH, false, FLY_CEL_SIZE, FLY_CEL_SIZE, FLY_CELS, false);
}
