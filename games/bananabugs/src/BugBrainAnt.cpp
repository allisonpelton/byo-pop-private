#include "BugBrainAnt.h"

#include "BugImg.h"
#include "BugImgAnt.h"
#include "BugShared.h"
#include "FillObj.h"
#include "LevelObj.h"
#include "PipeObj.h"

using namespace Sexy;

// The places the ant's box covers each way from its own (SetKind's half-width and half-height)
static const int ANT_HALF_SIZE = 2;

// A leg is walked at this many frames a pixel, and the turn at its end takes this many frames; a rope hurries the ant,
// which walks the leg in half the time (a frame and a half a pixel) and turns in less
static const double LEG_FRAMES_PER_PIXEL = 3.0;
static const int TURN_FRAMES = 88;
static const double RUSH_FRAMES_PER_PIXEL = 1.5;
static const int RUSH_TURN_FRAMES = 69;

// The death: the ant is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees, falls
// below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = 8.0;
static const int FLING_ANGLE_RANGE = 21;
static const int FLING_ANGLE_FIRST = 80;
static const int FLING_FRAMES = 550;
static const float FLING_END_Y = 1040.0f;
static const double FLING_START_PLUS = (double)3.2f;   // 3.2 as a float, widened (0x5e7508)
static const float FLING_FALL_RATE = -0.17f;
static const int TUMBLE_FRAMES = 100;
static const int FADE_OUT_FRAMES = 1;
static const double TUMBLE_TURN = 50.26548385620117;   // eight turns (0x5e74f8): the tumble spins this far either way
static const float FADE_OUT_ALPHA = -1.0f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's

// 0x413f90: SetKind (the kind, and the places its box covers each way), no body yet
BugBrainAnt::BugBrainAnt()
{
	SetKind(BUG_ANT, ANT_HALF_SIZE, ANT_HALF_SIZE);
	mTurnPhase = ANT_TURN_NONE;
	mBody = nullptr;
	mUnused2A4 = 1;
	// port: the original leaves these as the memory had them until Setup and PlaceAt set them
	mNextHeading = HEADING_UP;
	mUnused33C = 0;
	mMoveTrack = 0;
	mLegKey = nullptr;
	mTurnKey = nullptr;
	mTurnEndKey = nullptr;
	mFlingKey = nullptr;
	mTumbleKey = nullptr;
	mFadeOutKey = nullptr;
}

// 0x413ff0: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this
// brain's child, and the step's track built. The ant is shown at once
void BugBrainAnt::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgAnt* aBody = new BugImgAnt();
	aBody->SetBrain(this);
	SetSetupBody(aBody);
	mBody = aBody;
	BuildMoveTrack();
	Activate(this, false);
}

// 0x4140b0: the ends of the step's keys (a bug not on the grid hears none): the turn done, the next leg (the heading
// the post gave is taken now, and the next is chosen at random), the bug going when its fade is done, and the tumble's
// end, which puts the last key where the ant lies
void BugBrainAnt::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case ANT_BRAIN_EVENT_LEG_DONE:
		break;
	case ANT_BRAIN_EVENT_TURN_DONE:
		mTurnPhase = ANT_TURN_AT_POST;
		return;
	case ANT_BRAIN_EVENT_TURN_END:
		mHeading = mNextHeading;
		BeginStep(true);
		mTurnPhase = ANT_TURN_NONE;
		return;
	case ANT_BRAIN_EVENT_GONE:
		Delete();
		return;
	case ANT_BRAIN_EVENT_TUMBLED:
		mTumbleKey->mValue[STATE_X] = mState[STATE_X];
		mTumbleKey->mValue[STATE_Y] = mState[STATE_Y];
		mFadeOutKey->mValue[STATE_X] = mState[STATE_X];
		mFadeOutKey->mValue[STATE_Y] = (float)((double)mState[STATE_Y] + FADE_OUT_DROP);
		return;
	default:
		BugObj::Notify(theObj);
	}
}

// 0x41c640 (slot 5; shared, see BugShared): the base's update
bool BugBrainAnt::Update()
{
	return BugSharedUpdate(this);
}

// 0x41c620 (slot 15; shared): the bug is updated, then its body
void BugBrainAnt::StepSegment()
{
	BugSharedStepSegment(this);
}

// 0x4141a0 (slot 16): a rope has been shot along a line near the ant. It acts only when it is in play, not in the middle
// of a turn, and its body has just ended a step. The rope is ahead of it when the line is in the way it faces: then it
// turns round (the heading two on, the walk's sides swapped and the picture mirrored), else it hurries to the next post
void BugBrainAnt::AvoidRope(int theVertical, float theX, float theY)
{
	if (!mAlive)
		return;
	if (mTurnPhase != ANT_TURN_NONE)
		return;
	BugImgAnt* aBody = static_cast<BugImgAnt*>(mBody);
	if (aBody != nullptr && !aBody->mStepDone)
		return;
	bool aAhead = false;
	if (theVertical == 0) {
		if (mHeading == HEADING_UP)
			aAhead = theY < mState[STATE_Y];
		else if (mHeading == HEADING_DOWN)
			aAhead = mState[STATE_Y] < theY;
	} else {
		if (mHeading == HEADING_RIGHT)
			aAhead = mState[STATE_X] < theX;
		else if (mHeading == HEADING_LEFT)
			aAhead = theX < mState[STATE_X];
	}
	if (aAhead)
		mHeading = (mHeading + 2) % HEADING_COUNT;
	PipeObj* aPost = GetNextPost();
	if (aPost == nullptr)
		return;
	mNextHeading = ChooseHeading(aPost, false, true);
	SetLegKeys(aPost, RUSH_FRAMES_PER_PIXEL, RUSH_TURN_FRAMES);
	if (aAhead && aBody != nullptr) {
		aBody->FlipWalk();
		aBody->SetState(STATE_SCALE_Y, (float)((double)aBody->mBase[STATE_SCALE_Y] * -1.0));
	}
	StartTrack(mMoveTrack);
}

// 0x415810 (slot 17; shared): leaves play, and the fade track starts
void BugBrainAnt::Vanish()
{
	BugSharedVanish(this);
}

// 0x417aa0 (slot 18; shared): see BugSharedSetCommand
bool BugBrainAnt::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommand(this, theCommand, theFrames);
}

// 0x41e1f0 (slot 19; shared): see BugSharedTakeCommand
int BugBrainAnt::TakeCommand()
{
	return BugSharedTakeCommand(this);
}

// 0x414850 (slot 20): the bug dies (by the base). The ant is flung off: the death track's first key throws it at an
// angle of 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame (the orbit radius of the brain is
// read as it is), and the three keys' turns start from the way the ant faces now; the tumble spins it a turn a
// side at random. Both turn keys are added on top of the ones the death track was built with
bool BugBrainAnt::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
{
	if (!BugObj::Zap(theFollowers, theStartTrack, thePeaHit))
		return false;
	mUnused33C = 0;
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

// 0x4145b0 (slot 21): placed on a wall piece that runs across it faces right, on one that runs down it faces up; on a post
// it keeps the heading it had. It starts its first step and is shown
void BugBrainAnt::PlaceAt(int theColumn, int theRow)
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

// 0x415b60 (slot 22; shared): the fade track, ending with event 5
void BugBrainAnt::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, ANT_BRAIN_EVENT_GONE);
}

// 0x414c30 (slot 23): the death track: flung off and falling (its decays start from where the bug is when it is built,
// and Zap sets the fling), a tumble that fades it in over 100 frames (event 6 at its end), and a last frame that fades
// it out and removes it. The tumble's turn is a random side, drawn here as well as in Zap
void BugBrainAnt::BuildDeathTrack()
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
	mTumbleKey->mEventId = ANT_BRAIN_EVENT_TUMBLED;
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

// 0x4149e0: the step's track: three keys that move the ant's place, its turn (a quarter turn for a corner) and nothing
// else; Where they go is set by each step (BeginStep). The keys' own values are all 0, easing toward the next key
void BugBrainAnt::BuildMoveTrack()
{
	mLegKey = new AnimKey();
	mLegKey->mEventId = ANT_BRAIN_EVENT_LEG_DONE;
	mLegKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTurnKey = new AnimKey();
	mTurnKey->mEventId = ANT_BRAIN_EVENT_TURN_DONE;
	mTurnKey->SetDuration(1);
	mTurnKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mTurnKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mTurnKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTurnEndKey = new AnimKey();
	mTurnEndKey->mEventId = ANT_BRAIN_EVENT_TURN_END;
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

// The post the heading leads to from the place the ant is on: the grid's pipe of the place has a link in each heading
PipeObj* BugBrainAnt::GetNextPost()
{
	// port (crash): the original reads the grid at any place, the 0x7fff of a bug with none included
	if (mColumn < 0 || mColumn >= FILL_GRID_COLUMNS || mRow < 0 || mRow >= FILL_GRID_ROWS)
		return nullptr;
	PipeObj* aPipe = gFillGrid[mColumn][mRow].mPipe;
	if (aPipe == nullptr)
		return nullptr;
	return BugPostLink(aPipe, mHeading);
}

// The keys of a leg to thePost (the code of 0x414630 and of the two copies in 0x4141a0): the leg key starts where the
// ant is, facing its heading; the turn key is at the post, facing the same way; the last key is at the post facing the
// quarter turn the next heading is from it. The leg takes one frame more than theFramesPerPixel a pixel of its length
// (cut to whole frames), the turn theTurnFrames
void BugBrainAnt::SetLegKeys(PipeObj* thePost, double theFramesPerPixel, int theTurnFrames)
{
	float aTargetX = (float)(thePost->mColumn * BUG_GRID_CELL);
	float aTargetY = (float)(thePost->mRow * BUG_GRID_CELL);
	mLegKey->mValue[STATE_X] = mState[STATE_X];
	mLegKey->mValue[STATE_Y] = mState[STATE_Y];
	mLegKey->mValue[STATE_ROTATION] = BugHeadingAngle(mHeading);
	mLegKey->mEasing = EASE_LINEAR_CLAMPED;
	mTurnKey->mValue[STATE_X] = aTargetX;
	mTurnKey->mValue[STATE_Y] = aTargetY;
	mTurnKey->mValue[STATE_ROTATION] = BugHeadingAngle(mHeading);
	mTurnKey->mEasing = EASE_LINEAR_CLAMPED;
	mTurnEndKey->SetDuration(1);
	mTurnEndKey->mValue[STATE_X] = aTargetX;
	mTurnEndKey->mValue[STATE_Y] = aTargetY;
	mTurnEndKey->mValue[STATE_ROTATION] = BugTurnAngle(mHeading, mNextHeading, mTurnKey->mValue[STATE_ROTATION]);
	mTurnEndKey->mEasing = EASE_LINEAR_CLAMPED;
	// The original squares and adds on the FPU stack, rounding only the sum
	double aDeltaY = (double)mLegKey->mValue[STATE_Y] - (double)mTurnKey->mValue[STATE_Y];
	double aDeltaX = (double)mLegKey->mValue[STATE_X] - (double)mTurnKey->mValue[STATE_X];
	float aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
	mLegKey->SetDuration(1 - (int)((double)aDistance * -theFramesPerPixel));
	mTurnKey->SetDuration(theTurnFrames);
}

// 0x414630 (EAX the bug): a step: the heading chosen at the last post is taken, the next is chosen at the post it leads to
// (at random when theRandom), and the three keys are set (a leg at 3 frames a pixel, a turn of 88 frames) and
// started. When the body would be on the wrong side of the wall for the turn (above the line and turning one way, below
// it and turning the other) the body is told to flip (0x6a) and the turn phase waits for it. Nothing happens when
// the heading leads nowhere
void BugBrainAnt::BeginStep(bool theRandom)
{
	PipeObj* aPost = GetNextPost();
	if (aPost == nullptr)
		return;
	mHeading = mNextHeading;
	mNextHeading = ChooseHeading(aPost, theRandom, true);
	SetLegKeys(aPost, LEG_FRAMES_PER_PIXEL, TURN_FRAMES);
	BugImgAnt* aBody = static_cast<BugImgAnt*>(mBody);
	if (aBody != nullptr) {
		int aTurn = TurnDirection(mNextHeading, mHeading);
		bool aAbove = aBody->mState[STATE_Y] < 0.0f;
		if (aAbove ? aTurn > 0 : aTurn < 0) {
			aBody->Slot13(BUG_CMD_ANT_FLIP, 0, 0);
			mTurnPhase = ANT_TURN_FLIPPING;
		}
	}
	StartTrack(mMoveTrack);
}
