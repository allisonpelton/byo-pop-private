#include "BugImgMoth.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgMoth::sDanceAnim = nullptr;
Anim* BugImgMoth::sMothAnim = nullptr;

// The pictures: a cel of the dance is 96 by 96 (30 of them), of the wings 96 by 96 (2)
static const int DANCE_CEL_SIZE = 0x60;
static const int DANCE_CELS = 0x1e;
static const int MOTH_CEL_SIZE = 0x60;
static const int MOTH_CELS = 2;

// The wings: each key is this many frames; the cels go from the first to this one and back
static const int FLAP_FRAMES = 1;
static const float FLAP_WIDE_CEL = 1.0f;
// The dance: the cels play back and forth from the 11th, 30 of them, at 50 to the cel; it lasts 800 frames unless the
// freeze says how long, and it ends after 75 more
static const int DANCE_FRAMES = 800;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x4239c0: the body on the BugImg base, with the wings built (the first track, which Activate starts) and the dance built,
// and shown
BugImgMoth::BugImgMoth()
{
	mUnused294 = false;
	mFlapFirstKey = new AnimKey();
	mFlapFirstKey->mEventId = MOTH_EVENT_FLAP;
	sMothAnim->AddRef();
	mFlapFirstKey->mAnim = sMothAnim;
	mFlapFirstKey->SetDuration(FLAP_FRAMES);
	mFlapFirstKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mFlapSecondKey = new AnimKey();
	mFlapSecondKey->mEventId = MOTH_EVENT_FLAP;
	sMothAnim->AddRef();
	mFlapSecondKey->mAnim = sMothAnim;
	mFlapSecondKey->SetDuration(FLAP_FRAMES);
	mFlapSecondKey->AddValue(STATE_FRAME, FLAP_WIDE_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlapFirstKey, nullptr);
	aTrack->AddKey(mFlapSecondKey, mFlapFirstKey);
	mFlapTrack = AddTrack(aTrack, nullptr);
	BuildDanceTrack();
	Activate(this, false);
}

// 0x423b60 (0x423b80 is the body)
BugImgMoth::~BugImgMoth()
{
}

// 0x4237d0 (slot 4; shared): the end of the dance (event 2) starts the wings again
void BugImgMoth::Notify(ObjBase* theObj)
{
	BugSharedImgNotify(this, theObj, mFlapTrack);
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgMoth::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x424f40 (slot 13; shared): see BugSharedImgSlot13
void BugImgMoth::Slot13(int theCommand, int theFrames, int theUnused)
{
	BugSharedImgSlot13(this, mDanceKey, mDanceTrack, theCommand, theFrames, theUnused);
}

// 0x423b90: the dance: the dance cels play back and forth for the length the freeze asks for (set when the dance starts),
// then 75 frames returning to the first cel from where they were, then a key of no length that raises event 2
void BugImgMoth::BuildDanceTrack()
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
	aEnd->mEventId = MOTH_EVENT_DANCE_DONE;
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

// 0x423d10 (GameWidget's preloads): the two pictures
void Sexy::PreloadMoth()
{
	BugImgMoth::sDanceAnim = Anim::Get("images/moth_dance", ANIM_SCALE_SMOOTH, false, DANCE_CEL_SIZE, DANCE_CEL_SIZE, DANCE_CELS, false);
	BugImgMoth::sMothAnim = Anim::Get("images/moth", ANIM_SCALE_SMOOTH, false, MOTH_CEL_SIZE, MOTH_CEL_SIZE, MOTH_CELS, false);
}
