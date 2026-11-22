#include "BugBrainRolly.h"

#include "BugImg.h"
#include "BugImgRolly.h"
#include "BugShared.h"
#include "Emitter.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "PipeObj.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The places the roly-poly's box covers each way from its own (SetKind's half-width and half-height)
static const int ROLLY_HALF_SIZE = 1;

// The frames a pixel of a leg takes walking and rolling, and the frames of the turn at a post in each; a leg takes one
// frame more than its length times the rate
static const float WALK_FRAMES_PER_PIXEL = 3.0f;       // 0x5e7180
static const float ROLL_FRAMES_PER_PIXEL = 0.18f;      // 0x5e7304
static const int WALK_TURN_FRAMES = 15;
static const int ROLL_TURN_FRAMES = 3;
// The roll: the legs rolled after the time curled
static const int ROLL_LEGS = 4;
// The time curled: 200 frames
static const int CURL_FRAMES = 200;
// The puff the end of the curl makes (the emitter's depth)
static const float SWELL_DEPTH = -1.0f;                // 0x5e6f60

// The death: the roly-poly is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees,
// falls below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = 8.0;            // 0x5e6ed0
static const int FLING_ANGLE_RANGE = 21;
static const int FLING_ANGLE_FIRST = 80;
static const int FLING_FRAMES = 550;
static const float FLING_END_Y = 1040.0f;
static const double FLING_START_PLUS = (double)3.2f;   // 3.2 as a float, widened (0x5e7508)
static const float FLING_FALL_RATE = -0.18f;           // 0x5e7500
static const int TUMBLE_FRAMES = 100;
static const int FADE_OUT_FRAMES = 1;
static const double TUMBLE_TURN = 50.26548385620117;   // eight turns (0x5e74f8): the tumble spins this far either way
static const float FADE_OUT_ALPHA = -1.0f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's (0x5e6ff0)

// 0x41d0d0: SetKind (the kind, and the places its box covers each way), no body yet, the walking rate, and the emitter of
// the puff (a swell) that the end of the curl bursts
BugBrainRolly::BugBrainRolly()
{
	SetKind(BUG_ROLLY, ROLLY_HALF_SIZE, ROLLY_HALF_SIZE);
	mFramesPerPixel = WALK_FRAMES_PER_PIXEL;
	mRolling = 0;
	mBody = nullptr;
	mUnused2A4 = 1;
	mRollLegsLeft = 0;
	mEmitters = new EmitterSet(this);
	mEmitters->MakeSwell(SWELL_DEPTH);
	// port: the original leaves these as the memory had them until Setup and PlaceAt set them
	mNextHeading = HEADING_UP;
	mUnused340 = 0;
	mMoveTrack = 0;
	mCurlTrack = 0;
	mLegKey = nullptr;
	mTurnKey = nullptr;
	mTurnEndKey = nullptr;
	mFlingKey = nullptr;
	mTumbleKey = nullptr;
	mFadeOutKey = nullptr;
}

// 0x41d1c0: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this
// brain's child, and the step's track and the curl's built. The roly-poly is shown at once
void BugBrainRolly::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgRolly* aBody = new BugImgRolly();
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildMoveTrack();
	BuildCurlTrack();
	Activate(this, false);
}

// 0x41d290: the ends of the step's keys (a bug not on the grid hears none): the turn's end takes the heading the post gave
// and starts the next leg (the next heading is chosen at random); the end of the curl starts the first leg, puffs where the
// body is and sounds; the bug goes when its fade is done, and the tumble's end puts the last key where it lies
void BugBrainRolly::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case ROLLY_BRAIN_EVENT_LEG_DONE:
	case ROLLY_BRAIN_EVENT_TURN_DONE:
		break;
	case ROLLY_BRAIN_EVENT_TURN_END:
		mHeading = mNextHeading;
		BeginStep(true);
		return;
	case ROLLY_BRAIN_EVENT_CURLED:
		BeginStep(true);
		// port (crash): the original takes the body unchecked
		if (mBody != nullptr)
			mEmitters->BurstSwell(mBody->mState[STATE_X], mBody->mState[STATE_Y]);
		gApp->mSoundQueue->Request(SOUND_ROLLYSTART, 0, false);
		return;
	case ROLLY_BRAIN_EVENT_GONE:
		Delete();
		return;
	case ROLLY_BRAIN_EVENT_TUMBLED:
		mTumbleKey->mValue[STATE_X] = mState[STATE_X];
		mTumbleKey->mValue[STATE_Y] = mState[STATE_Y];
		mFadeOutKey->mValue[STATE_X] = mState[STATE_X];
		mFadeOutKey->mValue[STATE_Y] = (float)((double)mState[STATE_Y] + FADE_OUT_DROP);
		return;
	default:
		BugObj::Notify(theObj);
	}
}

// 0x41c640 (slot 5; shared): the base's update
bool BugBrainRolly::Update()
{
	return BugSharedUpdate(this);
}

// 0x41c620 (slot 15; shared): the bug is updated, then its body
void BugBrainRolly::StepSegment()
{
	BugSharedStepSegment(this);
}

// 0x415810 (slot 17; shared): leaves play, and the fade track starts
void BugBrainRolly::Vanish()
{
	BugSharedVanish(this);
}

// 0x416420 (slot 18; shared): see BugSharedSetCommandGiven
bool BugBrainRolly::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommandGiven(this, theCommand, theFrames);
}

// 0x418cd0 (slot 19; shared): see BugSharedTakeCommandNoRing
int BugBrainRolly::TakeCommand()
{
	return BugSharedTakeCommandNoRing(this);
}

// 0x41d700 (slot 20): the bug dies (by the base, passing the pea hit on). The roly-poly is flung off: the death track's first
// key throws it at an angle of 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame, and the three keys'
// turns start from the way it faces now; the tumble spins it a turn a side at random. The turn keys are added on top of
// the ones the death track was built with
bool BugBrainRolly::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
{
	if (!BugObj::Zap(theFollowers, theStartTrack, thePeaHit))
		return false;
	mUnused340 = 0;
	float anAngle = (float)((double)(int)(GameRandomNext() % FLING_ANGLE_RANGE + FLING_ANGLE_FIRST) * PI_F / 180.0);
	float aSpeed = (float)((double)mState[STATE_ORBIT_RADIUS] + FLING_SPEED_PLUS);
	mFlingKey->SetDecay(STATE_X, aSpeed, FloatCos(anAngle));
	aSpeed = (float)((double)mState[STATE_ORBIT_RADIUS] + FLING_SPEED_PLUS);
	mFlingKey->SetDecay(STATE_Y, aSpeed, FloatSin(anAngle));
	mFlingKey->AddModifier(STATE_ROTATION, false, mState[STATE_ROTATION], MOD_TOWARD_NEXT, 0.0f, 0.0f, 0.0f, nullptr);
	float aTumbleTurn;
	if ((GameRandomNext() & 1) == 0)
		aTumbleTurn = (float)((double)mState[STATE_ROTATION] - TUMBLE_TURN);
	else
		aTumbleTurn = (float)((double)mState[STATE_ROTATION] + TUMBLE_TURN);
	mTumbleKey->AddModifier(STATE_ROTATION, false, aTumbleTurn, MOD_HOLD, 0.0f, 0.0f, 0.0f, nullptr);
	mFadeOutKey->AddModifier(STATE_ROTATION, false, mState[STATE_ROTATION], MOD_HOLD, 0.0f, 0.0f, 0.0f, nullptr);
	return true;
}

// 0x41d410 (slot 21): placed on a wall piece that runs across it faces right, on one that runs down it faces up; on a post
// it keeps the heading it had. It starts its first step (which is the curl) and is shown
void BugBrainRolly::PlaceAt(int theColumn, int theRow)
{
	BugObj::PlaceAt(theColumn, theRow);
	// port (crash): the original takes the place's pipe unchecked
	PipeObj* aPipe = nullptr;
	if (mColumn >= 0 && mColumn < FILL_GRID_COLUMNS && mRow >= 0 && mRow < FILL_GRID_ROWS)
		aPipe = gFillGrid[mColumn][mRow].mPipe;
	if (aPipe != nullptr) {
		if (aPipe->mKind == FILL_KIND_WALL_ACROSS) {
			mNextHeading = HEADING_RIGHT;
			mHeading = HEADING_RIGHT;
		} else if (aPipe->mKind == FILL_KIND_WALL_DOWN) {
			mNextHeading = HEADING_UP;
			mHeading = HEADING_UP;
		}
	}
	BeginStep(false);
	Activate(this, false);
}

// 0x418130 (slot 22; the same code as the shared 0x415b60 with the event 6): the fade track, ending with event 6
void BugBrainRolly::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, ROLLY_BRAIN_EVENT_GONE);
}

// 0x41db80 (slot 23): the death track: flung off and falling (its decays start from where the bug is when it is built,
// and Zap sets the fling), a tumble that fades it in over 100 frames (event 7 at its end), and a last frame that fades it
// out and removes it. The tumble's turn is a random side, drawn here as well as in Zap
void BugBrainRolly::BuildDeathTrack()
{
	mFlingKey = new AnimKey();
	mFlingKey->mEventId = 0;
	mFlingKey->SetDuration(FLING_FRAMES);
	mFlingKey->AddDecay(STATE_X, mState[STATE_X], (float)((double)mState[STATE_ORBIT_RADIUS] + FLING_START_PLUS),
		FloatCos(0.0f), 0.0f, 1.0f);
	mFlingKey->AddDecay(STATE_Y, mState[STATE_Y], (float)((double)mState[STATE_ORBIT_RADIUS] + FLING_START_PLUS),
		FloatSin(0.0f), FLING_FALL_RATE, 1.0f);
	mFlingKey->AddModifier(STATE_ROTATION, false, mState[STATE_ROTATION], MOD_TOWARD_NEXT, 0.0f, 0.0f, 0.0f, nullptr);
	mFlingKey->mEasing = EASE_IN_SQUARE;
	mFlingKey->mEndCondition = KEY_END_ABOVE;
	mFlingKey->mEndChannel = STATE_Y;
	mFlingKey->mEndValue = FLING_END_Y;

	mTumbleKey = new AnimKey();
	mTumbleKey->mEventId = ROLLY_BRAIN_EVENT_TUMBLED;
	mTumbleKey->SetDuration(TUMBLE_FRAMES);
	mTumbleKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mTumbleKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mTumbleKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	float aTumbleTurn;
	if ((GameRandomNext() & 1) == 0)
		aTumbleTurn = (float)((double)mState[STATE_ROTATION] - TUMBLE_TURN);
	else
		aTumbleTurn = (float)((double)mState[STATE_ROTATION] + TUMBLE_TURN);
	mTumbleKey->AddValue(STATE_ROTATION, aTumbleTurn, MOD_HOLD);

	mFadeOutKey = new AnimKey();
	mFadeOutKey->SetDuration(FADE_OUT_FRAMES);
	mFadeOutKey->SetRemoveAtEnd();
	mFadeOutKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mFadeOutKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	mFadeOutKey->AddValue(STATE_ALPHA, FADE_OUT_ALPHA, MOD_TOWARD_NEXT);
	mFadeOutKey->AddValue(STATE_ROTATION, mState[STATE_ROTATION], MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlingKey, nullptr);
	aTrack->AddKey(mTumbleKey, nullptr);
	aTrack->AddKey(mFadeOutKey, nullptr);
	mDeathTrack = AddTrack(aTrack, nullptr);
}

// 0x41d890: the step's track: three keys that move the roly-poly's place, its turn and nothing else; where they go is set by
// each step (BeginStep). The keys' own values are all 0, easing toward the next key
void BugBrainRolly::BuildMoveTrack()
{
	mLegKey = new AnimKey();
	mLegKey->mEventId = ROLLY_BRAIN_EVENT_LEG_DONE;
	mLegKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTurnKey = new AnimKey();
	mTurnKey->mEventId = ROLLY_BRAIN_EVENT_TURN_DONE;
	mTurnKey->SetDuration(1);
	mTurnKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mTurnKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mTurnKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTurnEndKey = new AnimKey();
	mTurnEndKey->mEventId = ROLLY_BRAIN_EVENT_TURN_END;
	mTurnEndKey->SetDuration(1);
	mTurnEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mTurnEndKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mTurnEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mLegKey, nullptr);
	aTrack->AddKey(mTurnKey, nullptr);
	aTrack->AddKey(mTurnEndKey, nullptr);
	mMoveTrack = AddTrack(aTrack, nullptr);
}

// 0x41dae0: the time curled: a key of 200 frames and a key of one frame that raises event 5, with no values
void BugBrainRolly::BuildCurlTrack()
{
	AnimKey* aWait = new AnimKey();
	aWait->SetDuration(CURL_FRAMES);

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = ROLLY_BRAIN_EVENT_CURLED;
	aEnd->SetDuration(1);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aWait, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mCurlTrack = AddTrack(aTrack, nullptr);
}

// The post the heading leads to from the place the roly-poly is on: the grid's pipe of the place has a link in each heading
PipeObj* BugBrainRolly::GetNextPost()
{
	// port (crash): the original reads the grid at any place, the 0x7fff of a bug with none included
	if (mColumn < 0 || mColumn >= FILL_GRID_COLUMNS || mRow < 0 || mRow >= FILL_GRID_ROWS)
		return nullptr;
	PipeObj* aPipe = gFillGrid[mColumn][mRow].mPipe;
	if (aPipe == nullptr)
		return nullptr;
	return BugPostLink(aPipe, mHeading);
}

// 0x41d490 (EAX the bug): a step. First the curl and the roll: when it has a body, is walking and has no legs of a roll
// left, the body is told to curl (0x6b), the rate is the rolling one, four legs are to come and the curl track starts: the
// step is not taken now but when the curl ends (event 5). While rolling each step takes one leg off the count; when none is
// left the body is told to uncurl (0x6c), the rate is the walking one and the step goes on.
// Then the step itself: the heading chosen at the last post is taken, the next is chosen at the post it leads to (at
// random when theRandom, and not straight back where it can), and the three keys are set and started: the leg key starts
// where it is, facing its heading; the turn key is at the post, facing the same way, for 3 frames when rolling and
// 15 when not; the last key is at the post facing the quarter turn the next heading is from it. The leg takes one frame
// more than the rate times its length, cut to whole frames, and is eased linearly. Nothing happens when the heading leads
// nowhere
void BugBrainRolly::BeginStep(bool theRandom)
{
	if (mBody != nullptr) {
		if (mRolling == 0) {
			if (mRollLegsLeft == 0) {
				mBody->Slot13(BUG_CMD_ROLLY_CURL, 0, 0);
				mFramesPerPixel = ROLL_FRAMES_PER_PIXEL;
				mRolling = 1;
				mRollLegsLeft = ROLL_LEGS;
				StartTrack(mCurlTrack);
				return;
			}
		} else if (mRollLegsLeft < 1) {
			mBody->Slot13(BUG_CMD_ROLLY_UNCURL, 0, 0);
			mFramesPerPixel = WALK_FRAMES_PER_PIXEL;
			mRolling = 0;
			mRollLegsLeft = 0;
		} else {
			mRollLegsLeft--;
		}
	}
	PipeObj* aPost = GetNextPost();
	if (aPost == nullptr)
		return;
	mHeading = mNextHeading;
	mNextHeading = ChooseHeading(aPost, theRandom, true);
	float aTargetX = (float)(aPost->mColumn * BUG_GRID_CELL);
	float aTargetY = (float)(aPost->mRow * BUG_GRID_CELL);
	mLegKey->mValue[STATE_X] = mState[STATE_X];
	mLegKey->mValue[STATE_Y] = mState[STATE_Y];
	mLegKey->mValue[STATE_ROTATION] = BugHeadingAngle(mHeading);
	mTurnKey->mValue[STATE_X] = aTargetX;
	mTurnKey->mValue[STATE_Y] = aTargetY;
	mTurnKey->mValue[STATE_ROTATION] = BugHeadingAngle(mHeading);
	mTurnEndKey->SetDuration(1);
	mTurnEndKey->mValue[STATE_X] = aTargetX;
	mTurnEndKey->mValue[STATE_Y] = aTargetY;
	mTurnEndKey->mValue[STATE_ROTATION] = BugTurnAngle(mHeading, mNextHeading, mTurnKey->mValue[STATE_ROTATION]);
	// The original squares and adds on the FPU stack, rounding only the sum
	double aDeltaY = (double)mLegKey->mValue[STATE_Y] - (double)mTurnKey->mValue[STATE_Y];
	double aDeltaX = (double)mLegKey->mValue[STATE_X] - (double)mTurnKey->mValue[STATE_X];
	float aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
	mTurnKey->SetDuration(mRolling == 0 ? WALK_TURN_FRAMES : ROLL_TURN_FRAMES);
	mLegKey->SetDuration((int)((double)mFramesPerPixel * (double)aDistance) + 1);
	mLegKey->mEasing = EASE_LINEAR_CLAMPED;
	StartTrack(mMoveTrack);
}
