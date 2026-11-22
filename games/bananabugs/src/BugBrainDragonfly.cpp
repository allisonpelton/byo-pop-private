#include "BugBrainDragonfly.h"

#include "BugImg.h"
#include "BugImgDragonfly.h"
#include "BugShared.h"
#include "FillObj.h"
#include "GameApp.h"
#include "PipeObj.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The places the dragonfly's box covers each way from its own (SetKind's half-width and half-height)
static const int DRAGONFLY_HALF_WIDTH = 4;
static const int DRAGONFLY_HALF_HEIGHT = 2;

static const double GRID_CELL = 31.0;                  // the grid's places are 31 pixels apart (BUG_GRID_CELL)

// The look ahead: 31 steps of 2 pixels along the row (the loop counts down from 30 to 0)
static const int LOOK_AHEAD_STEPS = 30;
static const float LOOK_AHEAD_PIXELS = 2.0f;
// The body is told to turn round (the command 2, which the body does not answer) for this many frames
static const int TURN_COMMAND = 2;
static const int TURN_COMMAND_FRAMES = 0x1e;

// The scan along the row: at most this many places; a target is chosen only when more than this many were free, and it is
// this many places short of the wall; one nearer than this many columns to the bug is no target
static const int SCAN_MAX_PLACES = 0x28;
static const int TARGET_MIN_FREE_PLACES = 6;
static const int TARGET_SHORT_OF_WALL = 3;
static const int TARGET_MIN_DISTANCE = 3;

// The flight's keys: 100 frames of hover, 71 of wind-up, a frame at the end; the dart's length is a seventh of its pixels
static const int IDLE_FRAMES = 100;
static const int WINDUP_FRAMES = 0x47;
static const int END_FRAMES = 1;
static const double DART_PIXELS_PER_FRAME = 7.0;
// The dart is "far" when its length in frames, times the scale of the picture and 1.8, is over 5 (nothing reads that)
static const double FAR_SCALE = 1.8;
static const float FAR_THRESHOLD = 5.0f;

// The deaths and the fade
static const int FADE_FRAMES_EVENT = DRAGONFLY_BRAIN_EVENT_GONE;
static const double FLING_SPEED_PLUS = 8.0;            // the fling's speed is the orbit radius plus this (0x5e6ed0)
static const int FLING_ANGLE_RANGE = 21;
static const int FLING_ANGLE_FIRST = 80;
static const int FLING_FRAMES = 0x226;
static const float FLING_START = 4.8f;                 // the decays' start, which Zap replaces
static const float FLING_FALL_RATE = -0.17f;
static const float FLING_END_Y = 1040.0f;
static const float FLING_SHRINK_SCALE = 0.2f;          // the picture shrinks to a fifth in the fall
static const int TUMBLE_FRAMES = 100;
static const float TUMBLE_TURN = (float)(12 * TWO_PI_F);   // twelve turns (0x4296cbe4): the tumble spins this far either way
static const int FADE_OUT_FRAMES = 1;
static const float FADE_OUT_ALPHA = -1.0f;
static const float FADE_OUT_SCALE_Y = -0.6f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's (0x5e6ff0)

// 0x418880: SetKind (the kind, and the places its box covers each way), no body yet, no column scanned, a heading at random
BugBrainDragonfly::BugBrainDragonfly()
{
	mBody = nullptr;
	mLastColumn = -1;
	SetKind(BUG_DRAGONFLY, DRAGONFLY_HALF_WIDTH, DRAGONFLY_HALF_HEIGHT);
	mUnused2A4 = 2;
	mHoldFrames = 0;
	mAirborne = true;
	mHeading = (int)(GameRandomNext() & 3);
	// port: the original leaves these as the memory had them until Setup and PlaceAt set them
	mTargetCell = nullptr;
	mFarFlight = false;
	mMoveTrack = 0;
	mIdleKey = nullptr;
	mWindupKey = nullptr;
	mDartKey = nullptr;
	mEndKey = nullptr;
	mFlingKey = nullptr;
	mTumbleKey = nullptr;
	mFadeOutKey = nullptr;
}

// 0x418920: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this
// brain's child, and the flight's track built and started (its keys hold the bug's own place until PlaceAt chooses a
// target). The dragonfly is shown at once
void BugBrainDragonfly::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgDragonfly* aBody = new BugImgDragonfly();
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildMoveTrack();
	StartTrack(mMoveTrack);
	Activate(this, false);
}

// 0x4189f0: the base's update; a dragonfly in play (not flung off) whose place has become a wall or lies outside the grid
// dies; when a wall is within 62 pixels ahead the body is told to turn round and a new flight starts
bool BugBrainDragonfly::Update()
{
	if (!BugObj::Update())
		return false;
	if (mAlive && !mFlying) {
		int aColumn = (int)(mState[STATE_X] / GRID_CELL);
		int aRow = (int)(mState[STATE_Y] / GRID_CELL);
		if ((unsigned)aColumn < FILL_GRID_COLUMNS && (unsigned)aRow < FILL_GRID_ROWS && !gFillGrid[aColumn][aRow].mPipe->IsActive()) {
			if (!IsBlockedAhead())
				return true;
			mBody->Slot13(TURN_COMMAND, TURN_COMMAND_FRAMES, 0);
			BeginFlight();
			return true;
		}
		Zap(true, true, false);
	}
	return true;
}

// 0x418aa0: the ends of the flight's keys (a bug not on the grid hears none): each tells the body what the key is, with its
// length, and the wind-up and the dart sound; the last key starts the next flight; the bug goes when its fade is done, and
// the tumble's end puts the last key where the dragonfly lies
void BugBrainDragonfly::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	int aFrames = (int)theObj->mCurrentKey->mDuration;
	switch (theObj->mCurrentKey->mEventId) {
	case DRAGONFLY_BRAIN_EVENT_IDLE:
		mBody->Slot13(DRAGONFLY_CMD_IDLE, aFrames, 0);
		return;
	case DRAGONFLY_BRAIN_EVENT_WINDUP:
		mBody->Slot13(DRAGONFLY_CMD_WINDUP, aFrames, 0);
		sApp->mSoundQueue->Request(SOUND_WINDUP, 0, false);
		return;
	case DRAGONFLY_BRAIN_EVENT_DART:
		mBody->Slot13(DRAGONFLY_CMD_DART, aFrames, 0);
		sApp->mSoundQueue->Request(SOUND_DART, 0, false);
		return;
	case DRAGONFLY_BRAIN_EVENT_UNUSED:
		mBody->Slot13(DRAGONFLY_CMD_DART, aFrames, 0);
		return;
	case DRAGONFLY_BRAIN_EVENT_END:
		mBody->Slot13(DRAGONFLY_CMD_END, aFrames, 0);
		BeginFlight();
		return;
	case DRAGONFLY_BRAIN_EVENT_GONE:
		Delete();
		return;
	case DRAGONFLY_BRAIN_EVENT_TUMBLED:
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
void BugBrainDragonfly::Vanish()
{
	BugSharedVanish(this);
}

// 0x418c50 (slot 18): the bug and its column are held (by the base) and the body is told what for by the command the bug
// holds: a freeze is the dance; the stars and the ring only when the body is not frozen. Returns false always
bool BugBrainDragonfly::SetCommand(int theCommand, int theFrames)
{
	if (BugObj::SetCommand(theCommand, theFrames) && mBody != nullptr) {
		switch (mCommand) {
		case COMMAND_FREEZE:
			mBody->Slot13(BUG_CMD_FREEZE_ON, theFrames, 0);
			break;
		case COMMAND_DAZE:
			if (!mBody->mFrozen)
				mBody->Slot13(BUG_CMD_STARS_ON, theFrames, 0);
			break;
		case COMMAND_SHOCK:
			if (!mBody->mFrozen)
				mBody->Slot13(BUG_CMD_RING_ON, theFrames, 0);
			break;
		}
	}
	return false;
}

// 0x418cd0 (slot 19; shared): see BugSharedTakeCommandNoRing
int BugBrainDragonfly::TakeCommand()
{
	return BugSharedTakeCommandNoRing(this);
}

// 0x419070 (slot 20): the bug dies (by the base). The dragonfly is flung off: the death track's first key throws it at an
// angle of 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame, and the turn of that key starts from the
// way the dragonfly faces now. The fall's sound is asked for, pitched up a step or not at random
bool BugBrainDragonfly::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
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

// 0x418d20 (slot 21): placed, it chooses its first flight
void BugBrainDragonfly::PlaceAt(int theColumn, int theRow)
{
	BugObj::PlaceAt(theColumn, theRow);
	BeginFlight();
}

// 0x419170 (slot 22): the fade track, ending with event 7 (the same code as 0x415b60)
void BugBrainDragonfly::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, FADE_FRAMES_EVENT);
}

// 0x4194d0 (slot 23): the death track: flung off and falling, shrinking to a fifth (its decays start from where the bug
// is when it is built, and Zap sets the fling), a tumble that fades it in over 100 frames (event 8 at its end), and a last
// frame that fades it out, flattens it and removes it. The tumble's turn is a random side
void BugBrainDragonfly::BuildDeathTrack()
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
	mTumbleKey->mEventId = DRAGONFLY_BRAIN_EVENT_TUMBLED;
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

// 0x419300: the flight's track: the hover, the wind-up, the dart and a last frame, each moving only along the screen toward
// the next key's (set by each flight; the dart's length too)
void BugBrainDragonfly::BuildMoveTrack()
{
	mIdleKey = new AnimKey();
	mIdleKey->mEventId = DRAGONFLY_BRAIN_EVENT_IDLE;
	mIdleKey->SetDuration(IDLE_FRAMES);
	mIdleKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);

	mWindupKey = new AnimKey();
	mWindupKey->mEventId = DRAGONFLY_BRAIN_EVENT_WINDUP;
	mWindupKey->SetDuration(WINDUP_FRAMES);
	mWindupKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);

	mDartKey = new AnimKey();
	mDartKey->mEventId = DRAGONFLY_BRAIN_EVENT_DART;
	mDartKey->SetDuration(END_FRAMES);
	mDartKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);

	mEndKey = new AnimKey();
	mEndKey->mEventId = DRAGONFLY_BRAIN_EVENT_END;
	mEndKey->SetDuration(END_FRAMES);
	mEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mIdleKey, nullptr);
	aTrack->AddKey(mWindupKey, nullptr);
	aTrack->AddKey(mDartKey, nullptr);
	aTrack->AddKey(mEndKey, nullptr);
	mMoveTrack = AddTrack(aTrack, nullptr);
}

// 0x418d40 (EAX the bug): whether the way ahead is blocked: along the row, 2 pixels a step (to the right when the heading
// is right, otherwise to the left, whatever the heading) for 31 steps, a place outside the grid or whose pipe is part of a
// wall stops it
bool BugBrainDragonfly::IsBlockedAhead()
{
	float aX = mState[STATE_X];
	float aY = mState[STATE_Y];
	float aStep;
	if (mHeading == HEADING_RIGHT)
		aStep = LOOK_AHEAD_PIXELS;
	else
		aStep = -LOOK_AHEAD_PIXELS;
	int aRow = (int)((double)aY / GRID_CELL);
	int aStepsLeft = LOOK_AHEAD_STEPS;
	for (;;) {
		aX = aX + aStep;
		int aColumn = (int)((double)aX / GRID_CELL);
		if (aColumn < 0 || aRow < 0 || aColumn >= FILL_GRID_COLUMNS || aRow >= FILL_GRID_ROWS)
			return true;
		if (gFillGrid[aColumn][aRow].mPipe->IsActive())
			return true;
		aStepsLeft--;
		if (aStepsLeft < 0)
			return false;
	}
}

// 0x418df0 (the bug pushed): the heading is left when the picture's scale is not over 0 and right when it is, and the row
// is scanned from the bug's own place that way, up to 40 places, until a place outside the grid or with a pipe that is part
// of a wall. When more than 6 places were free the place is 3 short of the one that stopped it (mLastColumn is where the scan
// ended); with fewer, the last free place. A place within 3 columns of the bug is no target (none)
void BugBrainDragonfly::ChooseTarget()
{
	int aStep;
	if (mState[STATE_SCALE_X] <= 0.0f) {
		mHeading = HEADING_LEFT;
		aStep = -1;
	} else {
		mHeading = HEADING_RIGHT;
		aStep = 1;
	}
	int aColumn = mColumn;
	int aRow = mRow;
	int aFreePlaces = 0;
	int aEndColumn = aColumn;      // the column the scan stopped at, or went one past
	int aProbe = aColumn;
	FillGridCell* aCell;
	for (;;) {
		aCell = &gFillGrid[aEndColumn][aRow];
		aProbe += aStep;
		aEndColumn += aStep;
		if ((unsigned)aProbe >= FILL_GRID_COLUMNS)
			break;
		if (gFillGrid[aProbe][aRow].mPipe->IsActive())
			break;
		aFreePlaces++;
		if (aFreePlaces >= SCAN_MAX_PLACES)
			break;
	}
	// The scan's own column is never the bug's (it has moved at least one step), so this never happens
	if (aEndColumn == mColumn && aEndColumn == mLastColumn)
		Zap(true, true, false);
	mLastColumn = aEndColumn;
	if (aFreePlaces > TARGET_MIN_FREE_PLACES) {
		if (aStep > 0)
			aCell = &gFillGrid[aEndColumn - TARGET_SHORT_OF_WALL][aRow];
		else
			aCell = &gFillGrid[aEndColumn + TARGET_SHORT_OF_WALL][aRow];
	}
	int aDistance = aCell->mColumn - mColumn;
	if (aDistance < 0)
		aDistance = -aDistance;
	mTargetCell = (aDistance <= TARGET_MIN_DISTANCE) ? nullptr : aCell;
}

// 0x418f10 (EAX the bug): the picture is turned round (its scale's sign flips), the heading and target are chosen, and the
// flight starts: the first three keys hold where the dragonfly is and the last the target's place, the dart (the third) lasting a
// seventh of the pixels in frames. With no target all four keys hold its place
void BugBrainDragonfly::BeginFlight()
{
	if (0.0f <= mState[STATE_SCALE_X])
		SetStateInt(STATE_SCALE_X, -1);
	else
		SetStateInt(STATE_SCALE_X, 1);
	ComputeDrawn();
	ChooseTarget();
	mFarFlight = false;
	if (mTargetCell != nullptr) {
		float aTargetX = (float)((double)mTargetCell->mColumn * GRID_CELL);
		float aDistance = FloatAbs(aTargetX - mState[STATE_X]);
		float aFrames = (float)((double)aDistance / DART_PIXELS_PER_FRAME);
		float aFarness = (float)(((double)mState[STATE_SCALE_X] * FAR_SCALE) * (double)aFrames);
		mIdleKey->mValue[STATE_X] = mState[STATE_X];
		mWindupKey->mValue[STATE_X] = mState[STATE_X];
		mDartKey->mValue[STATE_X] = mState[STATE_X];
		mEndKey->mValue[STATE_X] = aTargetX;
		mDartKey->SetDuration((int)aFrames);
		if (FAR_THRESHOLD < aFarness) {
			mFarFlight = true;
			StartTrack(mMoveTrack);
			return;
		}
	} else {
		mIdleKey->mValue[STATE_X] = mState[STATE_X];
		mWindupKey->mValue[STATE_X] = mState[STATE_X];
		mDartKey->mValue[STATE_X] = mState[STATE_X];
		mEndKey->mValue[STATE_X] = mState[STATE_X];
	}
	StartTrack(mMoveTrack);
}
