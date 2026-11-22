#include "BugImgDragonfly.h"

#include "Anim.h"
#include "BugShared.h"
#include "Emitter.h"

using namespace Sexy;

// The swell emitter's depth (the original passes -1)
static const float SWELL_DEPTH = -1.0f;

// The pictures (each Get, as the original makes them in each body's constructor, not in a preload): the cel's size and count
static const int IDLE_CEL_WIDTH = 0x6e;
static const int IDLE_CEL_HEIGHT = 0x4c;
static const int IDLE_CELS = 0x1e;
static const int WINDUP_CEL_WIDTH = 0xa0;
static const int WINDUP_CEL_HEIGHT = 0x5a;
static const int WINDUP_CELS = 0x14;
static const int DART_CEL_WIDTH = 0x72;
static const int DART_CEL_HEIGHT = 0x42;
static const int DART_CELS = 6;
static const int DANCE_CEL_WIDTH = 0xa0;
static const int DANCE_CEL_HEIGHT = 0x56;
static const int DANCE_CELS = 0x1e;

// The cels count up at this rate over this many (the idle's 30, the dart's 6) before they begin again
static const float COUNT_RATE = 100.0f;
static const float IDLE_COUNT_AMOUNT = 30.0f;
static const float DART_COUNT_AMOUNT = 6.0f;
// The hover key is one cycle of the cels; the wind-up goes up 19 cels in 60 frames and back in 10 and holds one frame
static const int IDLE_FRAMES = 0x1e;
static const int WINDUP_UP_FRAMES = 0x3c;
static const int WINDUP_DOWN_FRAMES = 10;
static const float WINDUP_LAST_CEL = 19.0f;
static const int DART_FRAMES = 6;
// The dance: the cels play back and forth from the 11th, 30 of them, at 50 to the cel; it lasts 800 frames unless the
// freeze says how long, and it ends after 75 more
static const int DANCE_FRAMES = 800;
static const float DANCE_RATE = 50.0f;
static const float DANCE_AMOUNT = 30.0f;
static const float DANCE_BASE = 11.0f;
static const int DANCE_END_FRAMES = 0x4b;

// 0x422f70: the body on the BugImg base, not frozen, with a swell emitter, the four pictures and the four tracks; the
// hover (the first track) is started, and the body shown
BugImgDragonfly::BugImgDragonfly()
{
	mUnused29C = -99;
	mFrozen = false;
	mUnused2A0 = false;
	mEmitters = new EmitterSet(this);
	mEmitters->MakeSwell(SWELL_DEPTH);
	mIdleAnim = Anim::Get("images/dragonfly_idle", ANIM_SCALE_SMOOTH, false, IDLE_CEL_WIDTH, IDLE_CEL_HEIGHT, IDLE_CELS, false);
	mWindupAnim = Anim::Get("images/dragonfly_windup", ANIM_SCALE_SMOOTH, false, WINDUP_CEL_WIDTH, WINDUP_CEL_HEIGHT, WINDUP_CELS, false);
	mDartAnim = Anim::Get("images/dragonfly_dart", ANIM_SCALE_SMOOTH, false, DART_CEL_WIDTH, DART_CEL_HEIGHT, DART_CELS, false);
	mDanceAnim = Anim::Get("images/dragonfly_dance", ANIM_SCALE_SMOOTH, false, DANCE_CEL_WIDTH, DANCE_CEL_HEIGHT, DANCE_CELS, false);
	BuildIdleTrack();
	BuildWindupTrack();
	BuildDartTrack();
	BuildDanceTrack();
	StartTrack(0);
	Activate(this, false);
}

// 0x4230b0 (0x4230d0 is the body)
BugImgDragonfly::~BugImgDragonfly()
{
}

// 0x4237d0 (slot 4; shared in the original, with the skeeter's, the bee's and the flies' bodies): the end of a key with event 2
// starts the track whose number is in the object's +0x2a4. Those bodies keep their hover track there; this body keeps the
// idle picture's pointer there, so the original starts the track numbered by an address (the wind-up's last key has the
// event 2), reading far off the object's track list, a bug of the original. Here the event is not heard (port: no crash);
// the brain starts the dart (and later the hover) itself
void BugImgDragonfly::Notify(ObjBase* theObj)
{
	(void)theObj;
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgDragonfly::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x4230e0: the brain's commands: a freeze begins the dance (for as many frames as it asks) and its end the hover, 0x6e and
// 0x71 the hover, 0x6f the wind-up and 0x70 the dart; any other is BugImg's (the stars and the ring)
void BugImgDragonfly::Slot13(int theCommand, int theFrames, int theUnused)
{
	switch (theCommand) {
	case BUG_CMD_FREEZE_ON:
		mFrozen = true;
		mDanceKey->SetDuration(theFrames);
		StartTrack(mDanceTrack);
		return;
	case BUG_CMD_FREEZE_OFF:
		mFrozen = false;
		StartTrack(mIdleTrack);
		return;
	case DRAGONFLY_CMD_IDLE:
	case DRAGONFLY_CMD_END:
		StartTrack(mIdleTrack);
		return;
	case DRAGONFLY_CMD_WINDUP:
		StartTrack(mWindupTrack);
		return;
	case DRAGONFLY_CMD_DART:
		StartTrack(mDartTrack);
		return;
	default:
		BugImg::Slot13(theCommand, theFrames, theUnused);
		return;
	}
}

// 0x4231b0: the hover: one key of the idle cels counting up over 30 frames, which holds itself (its next is itself)
void BugImgDragonfly::BuildIdleTrack()
{
	AnimKey* aKey = new AnimKey();
	aKey->mEventId = DRAGONFLY_EVENT_IDLE;
	mIdleAnim->AddRef();
	aKey->mAnim = mIdleAnim;
	aKey->SetDuration(IDLE_FRAMES);
	aKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, COUNT_RATE, IDLE_COUNT_AMOUNT, 0.0f, nullptr);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	mIdleTrack = AddTrack(aTrack, nullptr);
}

// 0x423260: the wind-up: the cels go from the first to the 20th over 60 frames and back over 10, then a frame that holds the
// first with the event 2
void BugImgDragonfly::BuildWindupTrack()
{
	AnimKey* aUp = new AnimKey();
	mWindupAnim->AddRef();
	aUp->mAnim = mWindupAnim;
	aUp->SetDuration(WINDUP_UP_FRAMES);
	aUp->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aDown = new AnimKey();
	mWindupAnim->AddRef();
	aDown->mAnim = mWindupAnim;
	aDown->SetDuration(WINDUP_DOWN_FRAMES);
	aDown->AddValue(STATE_FRAME, WINDUP_LAST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = DRAGONFLY_EVENT_WINDUP_DONE;
	mWindupAnim->AddRef();
	aEnd->mAnim = mWindupAnim;
	aEnd->SetDuration(1);
	aEnd->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aUp, nullptr);
	aTrack->AddKey(aDown, nullptr);
	aTrack->AddKey(aEnd, aEnd);
	mWindupTrack = AddTrack(aTrack, nullptr);
}

// 0x4233d0: the dart: the six cels counting up over six frames, held by itself
void BugImgDragonfly::BuildDartTrack()
{
	AnimKey* aKey = new AnimKey();
	aKey->mEventId = DRAGONFLY_EVENT_DART;
	mDartAnim->AddRef();
	aKey->mAnim = mDartAnim;
	aKey->SetDuration(DART_FRAMES);
	aKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, COUNT_RATE, DART_COUNT_AMOUNT, 0.0f, nullptr);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	mDartTrack = AddTrack(aTrack, nullptr);
}

// 0x423480: the dance: the dance cels play back and forth for the length the freeze asks for (set when the dance starts),
// then 75 frames returning to the first cel from where they were, then a key of no length with the event 4
void BugImgDragonfly::BuildDanceTrack()
{
	mDanceKey = new AnimKey();
	mDanceAnim->AddRef();
	mDanceKey->mAnim = mDanceAnim;
	mDanceKey->SetDuration(DANCE_FRAMES);
	mDanceKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_PING_PONG, DANCE_RATE, DANCE_AMOUNT, DANCE_BASE, nullptr);

	AnimKey* aSettle = new AnimKey();
	mDanceAnim->AddRef();
	aSettle->mAnim = mDanceAnim;
	aSettle->SetDuration(DANCE_END_FRAMES);
	aSettle->AddModifier(STATE_FRAME, true, 0.0f, MOD_TOWARD_NEXT, 0.0f, 0.0f, 0.0f, nullptr);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = DRAGONFLY_EVENT_DANCE_DONE;
	mDanceAnim->AddRef();
	aEnd->mAnim = mDanceAnim;
	aEnd->SetDuration(0);
	aEnd->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mDanceKey, nullptr);
	aTrack->AddKey(aSettle, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mDanceTrack = AddTrack(aTrack, nullptr);
}
