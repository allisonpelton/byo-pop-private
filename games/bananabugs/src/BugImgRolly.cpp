#include "BugImgRolly.h"

#include "Anim.h"
#include "BugShared.h"

using namespace Sexy;

Anim* BugImgRolly::sRollyAnim = nullptr;

// The picture: a cel is 96 by 79, 60 cels
static const int ROLLY_CEL_WIDTH = 0x60;
static const int ROLLY_CEL_HEIGHT = 0x4f;
static const int ROLLY_CELS = 0x3c;

// The body hangs this far above the wall's line (above is minus), and a little lower as it curls up
static const float WALK_Y = -32.0f;
static const float CURL_MIDDLE_Y = -28.0f;
static const float CURL_END_Y = -26.0f;
static const float UNCURL_MIDDLE_Y = -31.0f;
static const float BALL_Y = -30.0f;
// A step along the wall is this many frames, the cels 0 to 29
static const int WALK_FRAMES = 0x50;
static const float WALK_LAST_CEL = 29.0f;
// The curl and the uncurl are two keys of this many frames each; the cels they run through (each key's own, the next's
// being where it goes)
static const int CURL_KEY_FRAMES = 0x32;
static const float CURL_FIRST_CEL = 30.0f;
static const float CURL_MIDDLE_CEL = 40.0f;
static const float CURL_LAST_CEL = 50.0f;
static const float UNCURL_FIRST_CEL = 20.0f;
static const float UNCURL_MIDDLE_CEL = 10.0f;
static const float UNCURL_LAST_CEL = 0.0f;
// The ball: 20 frames at the cel 0 going to the cel 8
static const int BALL_FRAMES = 0x14;
static const float BALL_LAST_CEL = 8.0f;
// The cel the ball is shown at when the curl ends, and the one the uncurl starts from (the pictures' cels 51 and 30)
static const int BALL_CEL = 0x33;
static const int UNCURL_START_CEL = 0x1e;

// 0x424300: the body on the BugImg base, with the picture and the four tracks built and shown (the walk is the first
// track, which Activate starts)
BugImgRolly::BugImgRolly()
{
	mCurled = false;
	mUnused2A4 = false;
	sRollyAnim->AddRef();
	mAnim = sRollyAnim;
	BuildWalkTrack();
	BuildCurlTrack();
	BuildUncurlTrack();
	BuildBallTrack();
	Activate(this, false);
}

// 0x424390 (0x4243b0 is the body)
BugImgRolly::~BugImgRolly()
{
}

// 0x4243c0: the ends of the body's keys: the walk's end (event 1) walks on from the first cel, as does the uncurl's end
// (event 4); the curl's end (event 2) shows the ball and starts the ball track, and the ball track's end (event 3) starts
// it again
void BugImgRolly::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case ROLLY_EVENT_STEP_DONE:
	case ROLLY_EVENT_UNCURLED:
	case ROLLY_EVENT_UNUSED_5:
		SetStateInt(STATE_FRAME, 0);
		StartTrack(mWalkTrack);
		break;
	case ROLLY_EVENT_CURLED:
		SetStateInt(STATE_FRAME, BALL_CEL);
		StartTrack(mBallTrack);
		mCurled = true;
		return;
	case ROLLY_EVENT_BALL_DONE:
		SetStateInt(STATE_FRAME, BALL_CEL);
		StartTrack(mBallTrack);
		return;
	}
}

// 0x4219c0 (slot 5; shared): ObjBase's update
bool BugImgRolly::Update()
{
	return BugSharedImgUpdate(this);
}

// 0x424450: the curl starts its track; the uncurl starts from the cel 30 (the curl's first) on its own. The commands 100 to
// 0x67 are not answered at all: a bug of the original, since the daze's stars (0x66) are never shown on a roly-poly (the
// shock's ring, 0x68, is: it goes to BugImg's, as does any other)
void BugImgRolly::Slot13(int theCommand, int theFrames, int theUnused)
{
	switch (theCommand) {
	case BUG_CMD_FREEZE_ON:
	case BUG_CMD_FREEZE_OFF:
	case BUG_CMD_STARS_ON:
	case BUG_CMD_STARS_OFF:
		return;
	case BUG_CMD_ROLLY_CURL:
		StartTrack(mCurlTrack);
		return;
	case BUG_CMD_ROLLY_UNCURL:
		SetStateInt(STATE_FRAME, UNCURL_START_CEL);
		StartTrack(mUncurlTrack);
		return;
	default:
		BugImg::Slot13(theCommand, theFrames, theUnused);
	}
}

// 0x4244c0: the walk: 80 frames on the upper side, cels 0 to 29, then a frame at the last cel that raises event 1 (the
// track's end)
void BugImgRolly::BuildWalkTrack()
{
	mWalkKey = new AnimKey();
	mWalkKey->SetDuration(WALK_FRAMES);
	mWalkKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_Y, WALK_Y, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mWalkEndKey = new AnimKey();
	mWalkEndKey->mEventId = ROLLY_EVENT_STEP_DONE;
	mWalkEndKey->SetDuration(1);
	mWalkEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_Y, WALK_Y, MOD_TOWARD_NEXT);
	mWalkEndKey->AddValue(STATE_FRAME, WALK_LAST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mWalkKey, nullptr);
	aTrack->AddKey(mWalkEndKey, nullptr);
	mWalkTrack = AddTrack(aTrack, nullptr);
}

// 0x424670: the curl: two keys of 50 frames, the cels 30 to 40 and 40 to 50 and the body sinking from Y -32 to -28 and -26,
// then a frame at the cel 50 and Y -26 that raises event 2
void BugImgRolly::BuildCurlTrack()
{
	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(CURL_KEY_FRAMES);
	aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Y, WALK_Y, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_FRAME, CURL_FIRST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aMiddle = new AnimKey();
	aMiddle->SetDuration(CURL_KEY_FRAMES);
	aMiddle->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aMiddle->AddValue(STATE_Y, CURL_MIDDLE_Y, MOD_TOWARD_NEXT);
	aMiddle->AddValue(STATE_FRAME, CURL_MIDDLE_CEL, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = ROLLY_EVENT_CURLED;
	aEnd->SetDuration(1);
	aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Y, CURL_END_Y, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_FRAME, CURL_LAST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStart, nullptr);
	aTrack->AddKey(aMiddle, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mCurlTrack = AddTrack(aTrack, nullptr);
}

// 0x4248a0: the uncurl: two keys of 50 frames, the cels 20 to 10 and 10 to 0 and the body rising from Y -32 to -31 and -30,
// then a frame at the cel 0 and Y -30 that raises event 4
void BugImgRolly::BuildUncurlTrack()
{
	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(CURL_KEY_FRAMES);
	aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_Y, WALK_Y, MOD_TOWARD_NEXT);
	aStart->AddValue(STATE_FRAME, UNCURL_FIRST_CEL, MOD_TOWARD_NEXT);

	AnimKey* aMiddle = new AnimKey();
	aMiddle->SetDuration(CURL_KEY_FRAMES);
	aMiddle->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aMiddle->AddValue(STATE_Y, UNCURL_MIDDLE_Y, MOD_TOWARD_NEXT);
	aMiddle->AddValue(STATE_FRAME, UNCURL_MIDDLE_CEL, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = ROLLY_EVENT_UNCURLED;
	aEnd->SetDuration(1);
	aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Y, BALL_Y, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_FRAME, UNCURL_LAST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStart, nullptr);
	aTrack->AddKey(aMiddle, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mUncurlTrack = AddTrack(aTrack, nullptr);
}

// 0x424ad0: the ball: 20 frames at the cel 0 going to the cel 8, then a frame at the cel 8 that raises event 3 (the track
// starts again after the notify sets the cel back to the ball's); the body is at Y -30
void BugImgRolly::BuildBallTrack()
{
	mBallKey = new AnimKey();
	mBallKey->SetDuration(BALL_FRAMES);
	mBallKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mBallKey->AddValue(STATE_Y, BALL_Y, MOD_TOWARD_NEXT);
	mBallKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);

	mBallEndKey = new AnimKey();
	mBallEndKey->mEventId = ROLLY_EVENT_BALL_DONE;
	mBallEndKey->SetDuration(1);
	mBallEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mBallEndKey->AddValue(STATE_Y, BALL_Y, MOD_TOWARD_NEXT);
	mBallEndKey->AddValue(STATE_FRAME, BALL_LAST_CEL, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mBallKey, nullptr);
	aTrack->AddKey(mBallEndKey, nullptr);
	mBallTrack = AddTrack(aTrack, nullptr);
}

// 0x424c80 (GameWidget's preloads): the picture
void Sexy::PreloadRolyPoly()
{
	BugImgRolly::sRollyAnim = Anim::Get("images/rolypoly", ANIM_SCALE_SMOOTH, false, ROLLY_CEL_WIDTH, ROLLY_CEL_HEIGHT, ROLLY_CELS, false);
}
