#include "BugBrainSkeeter.h"

#include "BugImg.h"
#include "BugImgSkeeter.h"
#include "BugShared.h"
#include "FillObj.h"
#include "GameApp.h"
#include "PipeObj.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The places the skeeter's box covers each way from its own (SetKind's half-width and half-height)
static const int SKEETER_HALF_WIDTH = 2;
static const int SKEETER_HALF_HEIGHT = 1;

// What the skeeter's look and flight are made of: the ray is walked a pixel at a time; the look ahead takes this many
// steps (the loop counts down from 40 to 0), the long look at most this many; the 36 rays are 10 degrees apart
static const int LOOK_AHEAD_STEPS = 40;
static const int RAY_MAX_STEPS = 0x578;
static const int RAY_COUNT_DEGREES = 360;
static const int RAY_STEP_DEGREES = 10;
static const float NO_DISTANCE = -1.0f;
static const double GRID_CELL = 31.0;                  // the grid's places are 31 pixels apart (BUG_GRID_CELL)
// The flight takes this many frames a pixel of the ray plus one; the wait at its end 70
static const double FLIGHT_FRAMES_PER_PIXEL = (double)0.7f;   // 0.7 as a float, widened (0x5e6fc0)
static const int FLIGHT_EXTRA_FRAMES = 1;
static const int WAIT_FRAMES = 0x46;
static const int WAIT_MIN_FRAMES = 10;
static const unsigned int WAIT_RANDOM_FRAMES = 0x33;   // the first key's length is drawn from 10 to 60; BeginMove sets it again
// The facing: the whole part of the speed has been the facing's for more than this many frames
static const int FACING_FRAMES_NEEDED = 20;
// BeginMove: after this many updates a flight (the timer counts them) may start with a buzz, one in two; the timer is
// then drawn again from 0 to 100 in steps of 20
static const int BUZZ_AFTER_FRAMES = 400;
static const unsigned int TIMER_STEPS = 6;
static const int TIMER_STEP_FRAMES = 20;

// The death: the skeeter is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees,
// falls below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = (double)7.6f;   // 7.6 as a float, widened (0x5e7770)
static const int FLING_ANGLE_RANGE = 21;
static const int FLING_ANGLE_FIRST = 80;
static const int FLING_FRAMES = 0x226;
static const float FLING_START = 4.8f;                 // the decays' start, which Zap replaces
static const float FLING_FALL_RATE = -0.16f;
static const float FLING_END_Y = 1040.0f;
static const float FLING_SHRINK_SCALE = 0.2f;          // the picture shrinks to a fifth in the fall
static const int TUMBLE_FRAMES = 100;
static const float TUMBLE_TURN = (float)(12 * TWO_PI_F);   // twelve turns (0x4296cbe4): the tumble spins this far either way
static const int FADE_OUT_FRAMES = 1;
static const float FADE_OUT_ALPHA = -1.0f;
static const float FADE_OUT_SCALE_Y = -0.6f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's

// 0x41def0: SetKind (the kind, and the places its box covers each way), no body yet, facing right, and a heading at random
BugBrainSkeeter::BugBrainSkeeter()
{
	mBody = nullptr;
	SetKind(BUG_SKEETER, SKEETER_HALF_WIDTH, SKEETER_HALF_HEIGHT);
	mUnused33C = 0.0f;
	mHoldFrames = 0;
	mUnused310 = true;
	mFacing = 1.0f;
	mFacingFrames = 0;
	mFacingScale = 1.0f;
	mHeading = (int)(GameRandomNext() & 3);
	// port: the original leaves these as the memory had them until Setup and PlaceAt set them
	mTargetCell = nullptr;
	mMoveTrack = 0;
	mLegKey = nullptr;
	mWaitKey = nullptr;
	mEndKey = nullptr;
	mFlingKey = nullptr;
	mTumbleKey = nullptr;
	mFadeOutKey = nullptr;
	mFlightAngle = 0.0f;
}

// 0x41df90: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this
// brain's child, and the flight's track built and started. The skeeter is shown at once
void BugBrainSkeeter::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgSkeeter* aBody = new BugImgSkeeter();
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildMoveTrack();
	Activate(this, false);
	StartTrack(mMoveTrack);
}

// 0x41e060: which way the skeeter faces is worked out from its speed along the screen (mVelX): the whole part of the speed
// is compared with the facing, and when the same for more than 20 updates in a row the brain's width scale takes the sign
// of the speed (a picture mirrored to the left); when it is not, the facing takes the sign of the speed and the count
// starts again. Then the base's update; a skeeter in play (not flung off) counts the update in its timer, shows the
// facing, looks 41 pixels ahead, and chooses a new flight when a wall or a cut place is there
bool BugBrainSkeeter::Update()
{
	int aSpeed = (int)mVelX;
	int aFacing = (int)mFacing;
	if (aSpeed == aFacing) {
		mFacingFrames++;
		if (mFacingFrames > FACING_FRAMES_NEEDED) {
			if (0.0f <= mVelX)
				mFacingScale = 1.0f;
			else
				mFacingScale = -1.0f;
		}
	} else {
		if (0.0f <= mVelX)
			mFacing = 1.0f;
		else
			mFacing = -1.0f;
		mFacingFrames = 0;
	}
	if (!BugObj::Update())
		return false;
	if (!mFlying) {
		mTimer++;
		if (mAlive) {
			SetState(STATE_SCALE_X, mFacingScale);
			ComputeDrawn();
			if (IsBlockedAhead())
				BeginMove();
		}
	}
	return true;
}

// 0x41e140: the ends of the flight's keys (a bug not on the grid hears none): the track's end chooses a new flight, the bug
// goes when its fade is done, and the tumble's end puts the last key where the skeeter lies
void BugBrainSkeeter::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case SKEETER_BRAIN_EVENT_LEG_DONE:
	case SKEETER_BRAIN_EVENT_WAIT_DONE:
		break;
	case SKEETER_BRAIN_EVENT_CHOOSE:
		BeginMove();
		break;
	case SKEETER_BRAIN_EVENT_GONE:
		Delete();
		return;
	case SKEETER_BRAIN_EVENT_TUMBLED:
		mTumbleKey->mValue[STATE_X] = mState[STATE_X];
		mTumbleKey->mValue[STATE_Y] = mState[STATE_Y];
		mFadeOutKey->mValue[STATE_X] = mState[STATE_X];
		mFadeOutKey->mValue[STATE_Y] = (float)((double)mState[STATE_Y] + FADE_OUT_DROP);
		return;
	default:
		BugObj::Notify(theObj);
		return;
	}
}

// 0x415810 (slot 17; shared): leaves play, and the fade track starts
void BugBrainSkeeter::Vanish()
{
	BugSharedVanish(this);
}

// 0x417aa0 (slot 18; shared): see BugSharedSetCommand
bool BugBrainSkeeter::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommand(this, theCommand, theFrames);
}

// 0x41e1f0 (slot 19; shared): see BugSharedTakeCommand
int BugBrainSkeeter::TakeCommand()
{
	return BugSharedTakeCommand(this);
}

// 0x41e720 (slot 20): the bug dies (by the base). The skeeter is flung off: the death track's first key throws it at an
// angle of 80 to 100 degrees, upwards, at its orbit radius plus 7.6 pixels a frame (the orbit radius of the brain is read
// as it is), and the turn of that key starts from the way the skeeter faces now. The fall's sound is asked for, pitched
// up a step or not at random
bool BugBrainSkeeter::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
{
	if (!BugObj::Zap(theFollowers, theStartTrack, thePeaHit))
		return false;
	float anAngle = (float)((double)(int)(GameRandomNext() % FLING_ANGLE_RANGE + FLING_ANGLE_FIRST) * PI_F / 180.0);
	float aCos = FloatCos(anAngle);
	mFlingKey->SetDecay(STATE_X, (float)((double)mState[STATE_ORBIT_RADIUS] + FLING_SPEED_PLUS), aCos);
	float aSin = FloatSin(anAngle);
	mFlingKey->SetDecay(STATE_Y, (float)((double)mState[STATE_ORBIT_RADIUS] + FLING_SPEED_PLUS), aSin);
	mFlingKey->mValue[STATE_ROTATION] = mState[STATE_ROTATION];
	int aPitchRange = (int)(GameRandomNext() & 1);
	sApp->mSoundQueue->Request(SOUND_FLYFALL1, aPitchRange, false);
	return true;
}

// 0x41e260 (slot 21): placed, it chooses its first flight
void BugBrainSkeeter::PlaceAt(int theColumn, int theRow)
{
	BugObj::PlaceAt(theColumn, theRow);
	BeginMove();
}

// 0x415b60 (slot 22; shared): the fade track, ending with event 5
void BugBrainSkeeter::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, SKEETER_BRAIN_EVENT_GONE);
}

// 0x41ea20 (slot 23): the death track: flung off and falling, shrinking to a fifth (its decays start from where the bug
// is when it is built, and Zap sets the fling), a tumble that fades it in over 100 frames (event 6 at its end), and a last
// frame that fades it out, flattens it and removes it. The tumble's turn is a random side
void BugBrainSkeeter::BuildDeathTrack()
{
	mFlingKey = new AnimKey();
	mFlingKey->SetDuration(FLING_FRAMES);
	mFlingKey->AddDecay(STATE_X, mState[STATE_X], FLING_START, 0.0f, 0.0f, 1.0f);
	mFlingKey->AddDecay(STATE_Y, mState[STATE_Y], FLING_START, 0.0f, FLING_FALL_RATE, 1.0f);
	mFlingKey->AddValue(STATE_SCALE_X, FLING_SHRINK_SCALE, MOD_TOWARD_NEXT);
	mFlingKey->AddValue(STATE_SCALE_Y, FLING_SHRINK_SCALE, MOD_TOWARD_NEXT);
	mFlingKey->mEndCondition = KEY_END_ABOVE;
	mFlingKey->mEndChannel = STATE_Y;
	mFlingKey->mEndValue = FLING_END_Y;
	mFlingKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTumbleKey = new AnimKey();
	mTumbleKey->mEventId = SKEETER_BRAIN_EVENT_TUMBLED;
	mTumbleKey->SetDuration(TUMBLE_FRAMES);
	mTumbleKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mTumbleKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mTumbleKey->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	mTumbleKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	mTumbleKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	if ((GameRandomNext() & 1) == 0)
		mTumbleKey->AddValue(STATE_ROTATION, -TUMBLE_TURN, MOD_HOLD);
	else
		mTumbleKey->AddValue(STATE_ROTATION, TUMBLE_TURN, MOD_HOLD);

	mFadeOutKey = new AnimKey();
	mFadeOutKey->SetDuration(FADE_OUT_FRAMES);
	mFadeOutKey->SetRemoveAtEnd();
	mFadeOutKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mFadeOutKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	mFadeOutKey->AddValue(STATE_ALPHA, FADE_OUT_ALPHA, MOD_TOWARD_NEXT);
	mFadeOutKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	mFadeOutKey->AddValue(STATE_SCALE_Y, FADE_OUT_SCALE_Y, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlingKey, nullptr);
	aTrack->AddKey(mTumbleKey, nullptr);
	aTrack->AddKey(mFadeOutKey, nullptr);
	mDeathTrack = AddTrack(aTrack, nullptr);
}

// 0x41e820: the flight's track: a flight (whose length BeginMove sets; its Y and X move toward the next key, eased
// linearly), a wait of 70 frames, and a last key of one frame that ends with event 4. Where they go is set by each flight
void BugBrainSkeeter::BuildMoveTrack()
{
	mLegKey = new AnimKey();
	mLegKey->mEventId = SKEETER_BRAIN_EVENT_LEG_DONE;
	mLegKey->SetDuration((int)(GameRandomNext() % WAIT_RANDOM_FRAMES + WAIT_MIN_FRAMES));
	mLegKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->mEasing = EASE_LINEAR;

	mWaitKey = new AnimKey();
	mWaitKey->mEventId = SKEETER_BRAIN_EVENT_WAIT_DONE;
	mWaitKey->SetDuration(WAIT_FRAMES);
	mWaitKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWaitKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);

	mEndKey = new AnimKey();
	mEndKey->mEventId = SKEETER_BRAIN_EVENT_CHOOSE;
	mEndKey->SetDuration(1);
	mEndKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mEndKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mLegKey, nullptr);
	aTrack->AddKey(mWaitKey, nullptr);
	aTrack->AddKey(mEndKey, nullptr);
	mMoveTrack = AddTrack(aTrack, nullptr);
}

// 0x41e280 (EAX the bug): whether the skeeter's way ahead is blocked: along the ray at mFlightAngle from where it is, a
// pixel at a time for 41 pixels, a place outside the grid, a place whose pipe is part of a wall, or a place that has been
// cut stops it
bool BugBrainSkeeter::IsBlockedAhead()
{
	float aX = mState[STATE_X];
	float aY = mState[STATE_Y];
	float aStepX = FloatCos(mFlightAngle);
	float aStepY = FloatSin(mFlightAngle);
	int aStepsLeft = LOOK_AHEAD_STEPS;
	for (;;) {
		aX = aX + aStepX;
		aY = aY - aStepY;
		int aColumn = (int)((double)aX / GRID_CELL);
		int aRow = (int)((double)aY / GRID_CELL);
		if (aColumn < 0 || aRow < 0 || aColumn >= FILL_GRID_COLUMNS || aRow >= FILL_GRID_ROWS)
			return true;
		FillGridCell* aCell = &gFillGrid[aColumn][aRow];
		if (aCell->mPipe->IsActive())
			return true;
		if (aCell->mCell->mCut)
			return true;
		aStepsLeft--;
		if (aStepsLeft < 0)
			return false;
	}
}

// 0x41e370 (EDI the bug; theAngle, theX and theY pushed): the ray from the place (a pixel a step, the angle's cosine
// along the screen and its sine up) is followed until a place is outside the grid or has a pipe that is part of a wall, or
// 1400 pixels. mTargetCell is the last place before that (none when the first is already blocked); the answer is the
// distance from the skeeter's own place to the corner of that place (0 when none)
float BugBrainSkeeter::ScanRay(float theAngle, float theX, float theY)
{
	float aDistance = 0.0f;
	mTargetCell = nullptr;
	int aSteps = 0;
	float aStepX = FloatCos(theAngle);
	float aStepY = FloatSin(theAngle);
	for (;;) {
		theX = theX + aStepX;
		theY = theY - aStepY;
		int aColumn = (int)((double)theX / GRID_CELL);
		int aRow = (int)((double)theY / GRID_CELL);
		if (aColumn < 0 || aRow < 0 || aColumn >= FILL_GRID_COLUMNS || aRow >= FILL_GRID_ROWS)
			break;
		FillGridCell* aCell = &gFillGrid[aColumn][aRow];
		if (aCell->mPipe->IsActive())
			break;
		aSteps++;
		mTargetCell = aCell;
		if (aSteps >= RAY_MAX_STEPS)
			break;
	}
	if (mTargetCell != nullptr) {
		// The original squares and adds on the FPU stack, rounding only the sum
		double aDeltaY = (double)mState[STATE_Y] - (double)(mTargetCell->mRow * BUG_GRID_CELL);
		double aDeltaX = (double)mState[STATE_X] - (double)(mTargetCell->mColumn * BUG_GRID_CELL);
		aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
	}
	return aDistance;
}

// 0x41e4a0 (EAX the bug): the rays every ten degrees round the skeeter are measured and the farthest taken (the first of
// equals, and one that ends on a place): mFlightAngle is its angle, mTargetCell its end, and the heading (BugObj's mHeading)
// the one HeadingFromAngle gives. Returns the farthest distance, -1 when no ray ended on a place
float BugBrainSkeeter::ChooseDirection()
{
	float aBestDistance = NO_DISTANCE;
	FillGridCell* aBestCell = nullptr;
	float aBestAngle = 0.0f;
	int aDegrees = 0;
	do {
		float anAngle = (float)((double)aDegrees * PI_F / 180.0);
		float aDistance = ScanRay(anAngle, mState[STATE_X], mState[STATE_Y]);
		if (aBestDistance < aDistance && mTargetCell != nullptr) {
			aBestCell = mTargetCell;
			aBestAngle = anAngle;
			aBestDistance = aDistance;
		}
		aDegrees += RAY_STEP_DEGREES;
	} while (aDegrees < RAY_COUNT_DEGREES);
	// 0x473360 (the angle in radians): turned to degrees the wrong way round (down is positive), the quarter turn the
	// screen's up is from there is taken off, wrapped to 0..359, and the heading counted from the 45 degrees before each
	// quarter: 1 (right) to 4, which no other kind sets (it is the one past the left, which BugObj does not read)
	int aDegreesDown = (int)((double)aBestAngle * -180.0 / PI_F);
	int aTurned = 90 - aDegreesDown;
	if (aTurned >= 360)
		aTurned -= 360;
	if (aTurned < 0)
		aTurned += 360;
	int aHeading = (aTurned - 45) / 90 + 1;
	mFlightAngle = aBestAngle;
	mTargetCell = aBestCell;
	mHeading = aHeading;
	return aBestDistance;
}

// 0x41e570 (EAX the bug): a new flight. Once the timer has run past 400 frames the buzz sounds half the time; the timer
// starts again at 0 to 100 (a multiple of 20 drawn at random). The ray is chosen: when it ends on a place the first key
// goes from where the skeeter is to the place's corner over 0.7 frames a pixel (and a frame), and the other two stay
// there; the skeeter faces left when the place is to its left, else right. With no place the three keys stay where the
// skeeter is. The flight starts
void BugBrainSkeeter::BeginMove()
{
	if (mTimer > BUZZ_AFTER_FRAMES) {
		if ((GameRandomNext() & 1) == 0)
			sApp->mSoundQueue->Request(SOUND_BUGFLY1, 0, false);
	}
	mTimer = (int)(GameRandomNext() % TIMER_STEPS) * TIMER_STEP_FRAMES;
	float aDistance = ChooseDirection();
	if (mHeading >= 0 && mTargetCell != nullptr) {
		float aTargetX = (float)(mTargetCell->mColumn * GRID_CELL);
		float aTargetY = (float)(mTargetCell->mRow * GRID_CELL);
		mLegKey->SetDuration((int)((double)aDistance * FLIGHT_FRAMES_PER_PIXEL + FLIGHT_EXTRA_FRAMES));
		mLegKey->mValue[STATE_X] = mState[STATE_X];
		mLegKey->mValue[STATE_Y] = mState[STATE_Y];
		mWaitKey->mValue[STATE_X] = aTargetX;
		mWaitKey->mValue[STATE_Y] = aTargetY;
		mEndKey->mValue[STATE_X] = aTargetX;
		mEndKey->mValue[STATE_Y] = aTargetY;
		if (aTargetX < mState[STATE_X])
			mHeading = HEADING_LEFT;
		else
			mHeading = HEADING_RIGHT;
		StartTrack(mMoveTrack);
		return;
	}
	mLegKey->mValue[STATE_X] = mState[STATE_X];
	mLegKey->mValue[STATE_Y] = mState[STATE_Y];
	mWaitKey->mValue[STATE_X] = mState[STATE_X];
	mWaitKey->mValue[STATE_Y] = mState[STATE_Y];
	mEndKey->mValue[STATE_X] = mState[STATE_X];
	mEndKey->mValue[STATE_Y] = mState[STATE_Y];
	StartTrack(mMoveTrack);
}
