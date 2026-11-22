#include "BugBrainFly.h"

#include "BugImg.h"
#include "BugImgFly.h"
#include "BugShared.h"
#include "FillObj.h"
#include "GameApp.h"
#include "PathTrace.h"
#include "PipeObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The places the fly's box covers each way from its own (SetKind's half-width and half-height)
static const int FLY_HALF_SIZE = 2;

static const double GRID_CELL = 31.0;                  // the grid's places are 31 pixels apart (BUG_GRID_CELL)

// The flight paths, the original's two tables at 0x60e244 and 0x60e248 (the moth's are its own)
static const int FLY_PATH_COUNT = 2;
static PathTrace* sFlyPaths[FLY_PATH_COUNT];

// The look ahead: the loop counts down from 69 to 0 (70 steps of a pixel along the flight's angle)
static const int LOOK_AHEAD_STEPS = 0x45;
// The ray: a pixel at a time, up to 800 pixels; a flight is chosen when the ray is at least 70 pixels long
static const int RAY_MAX_STEPS = 800;
static const float RAY_MIN_REACH = 70.0f;
// The flight goes 62 to 92 pixels (62 and a draw of 31), over 15 to 40 frames (15 and a draw of 26); with no ray it
// waits 40 frames
static const double FLIGHT_MIN_PIXELS = 62.0;
static const unsigned int FLIGHT_PIXELS_RANGE = 0x1f;
static const int FLIGHT_MIN_FRAMES = 0xf;
static const unsigned int FLIGHT_FRAMES_RANGE = 0x1a;
static const int FLIGHT_WAIT_FRAMES = 0x28;
static const float FLIGHT_START_RADIUS = 0.8f;          // the speed (the orbit radius) at the start of the flight
static const float FLIGHT_END_RADIUS = 0.6f;
// The angle of a flight is the last one's, whole radians, and turned by a draw of 0 to 10 less 5: of a tenth of a turn
// (18 degrees) each, or only a degree, when the fly has just turned back (see ChooseFlight)
static const unsigned int TURN_DRAW_RANGE = 0xb;
static const int TURN_DRAW_MIDDLE = 5;
static const int TURN_DEGREES_FREE = 20;
static const double DEGREES_HALF_TURN = 180.0;
static const float TURN_BACK_ANGLE = 3.1415927f;        // a half turn (0x5e6fd0)

// The fly flees from the monkey when its speed (averaged) is at least this and it is within this reach
static const double FLEE_MIN_SPEED = 1.0;
static const float FLEE_REACH = 124.0f;
// Its flight along the path takes an eighth of the points in frames, and a draw of 26 more
static const double PATH_FRAMES_PER_POINT = 0.125;
static const unsigned int PATH_EXTRA_FRAMES_RANGE = 0x1a;
static const int PATH_LAND_FRAMES = 0x14;
static const int PATH_FRAMES = 0x32;
static const int FLEE_SOUND_AFTER_FRAMES = 10;

// The flight's keys: 50 frames, the end a frame
static const int FLIGHT_KEY_FRAMES = 0x32;

// The death: the fly is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees, falls
// below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = 8.0;
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

// 0x4198c0: SetKind (the kind, and the places its box covers each way), no body yet, nothing placed, flying, a heading at random
BugBrainFly::BugBrainFly()
{
	mBody = nullptr;
	SetKind(BUG_FLY, FLY_HALF_SIZE, FLY_HALF_SIZE);
	mUnused358 = 0.0f;
	mUnused2A4 = FLY_HALF_SIZE;
	mHoldFrames = 0;
	mUnused350 = true;
	mFlyState = FLY_STATE_FLYING;
	mPlaced = false;
	mPathIndex = 0;
	mUnused338 = 0;
	mHeading = (int)(GameRandomNext() & 3);
	// port: the original leaves these as the memory had them until Setup and PlaceAt set them
	mLeaveFromX = 0.0f;
	mLeaveFromY = 0.0f;
	mLeaveFromRotation = 0.0f;
	mFlightTrack = 0;
	mLeaveTrack = 0;
	mFlightKey = nullptr;
	mFlightEndKey = nullptr;
	mPathKey = nullptr;
	mLandKey = nullptr;
	mLeaveEndKey = nullptr;
	mFlingKey = nullptr;
	mTumbleKey = nullptr;
	mFadeOutKey = nullptr;
	mFlightAngle = 0.0f;
}

// 0x419970: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this brain's
// child, the flight's and the fleeing tracks built, and the first flight chosen (with no turn). The fly is shown at once
void BugBrainFly::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgFly* aBody = new BugImgFly();
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildFlightTrack();
	BuildLeaveTrack();
	BeginFlight(0.0f);
	Activate(this, false);
}

// 0x419a40: the base's update; a fly in play (not fleeing along the path) counts the update in its timer; when the monkey
// is fast and close and it is not already fleeing nor held, it flees (it cannot be hit while it does: mFlying), with a buzz
// after its first ten updates; while flying it looks 70 pixels ahead and turns right round when a wall or a cut place is in
// the way; a fly that has landed flies again; a fly held by a freeze whose place is cut or on a wall dies
bool BugBrainFly::Update()
{
	if (!BugObj::Update())
		return false;
	if (!mFlying) {
		mTimer++;
		int aTimer = mTimer;
		if (mAlive) {
			int aState = mFlyState;
			if (aState != FLY_STATE_FLEEING && mCommand == 0 && Player::GetSpeedAverage() >= FLEE_MIN_SPEED && Player::IsNear(this, FLEE_REACH)) {
				mFlying = true;
				mFlyState = FLY_STATE_FLEEING;
				mUnused338 = 0;
				mUnused310 = true;
				if (aTimer > FLEE_SOUND_AFTER_FRAMES) {
					int aPitchRange = (int)(GameRandomNext() & 1);
					sApp->mSoundQueue->Request(SOUND_FLYFLY, aPitchRange, false);
				}
				BeginLeave();
				return true;
			}
			if (aState == FLY_STATE_FLYING) {
				if (IsBlockedAhead())
					BeginFlight(TURN_BACK_ANGLE);
			} else if (aState == FLY_STATE_LANDED) {
				mUnused310 = false;
				mFlyState = FLY_STATE_FLYING;
			}
			if (mCommand == COMMAND_FREEZE) {
				int aColumn = (int)(mState[STATE_X] / GRID_CELL);
				int aRow = (int)(mState[STATE_Y] / GRID_CELL);
				FillGridCell* aCell = nullptr;
				if ((unsigned)aColumn <= 0x27u && (unsigned)aRow <= 0x27u)
					aCell = BugFlatGridCell(aColumn, aRow);
				if (aCell == nullptr || aCell->mCell->mCut || aCell->mPipe->IsActive())
					Zap(true, true, false);
			}
		}
	}
	return true;
}

// 0x419bb0: the ends of the keys (a bug not on the grid hears none): the flight's end chooses a new flight, the leave track's
// second key lets it be hit again, its end lands the fly (it dies if its place is cut or a wall, else it flies on, with no turn),
// the bug goes when its fade is done, and the tumble's end puts the last key where the fly lies
void BugBrainFly::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case FLY_BRAIN_EVENT_PATH_BEGUN:
	case FLY_BRAIN_EVENT_UNUSED_6:
		break;
	case FLY_BRAIN_EVENT_LANDING:
		mFlying = false;
		return;
	case FLY_BRAIN_EVENT_LEFT: {
		int aColumn = (int)(mState[STATE_X] / GRID_CELL);
		int aRow = (int)(mState[STATE_Y] / GRID_CELL);
		FillGridCell* aCell = nullptr;
		if ((unsigned)aColumn <= 0x27u && (unsigned)aRow <= 0x27u)
			aCell = BugFlatGridCell(aColumn, aRow);
		if (aCell == nullptr || aCell->mCell->mCut || aCell->mPipe->IsActive()) {
			Zap(true, true, false);
			return;
		}
		BeginFlight(0.0f);
		mFlyState = FLY_STATE_LANDED;
		break;
	}
	case FLY_BRAIN_EVENT_FLIGHT_DONE:
		BeginFlight(0.0f);
		return;
	case FLY_BRAIN_EVENT_GONE:
		Delete();
		return;
	case FLY_BRAIN_EVENT_TUMBLED:
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
void BugBrainFly::Vanish()
{
	BugSharedVanish(this);
}

// 0x417aa0 (slot 18; shared): see BugSharedSetCommand
bool BugBrainFly::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommand(this, theCommand, theFrames);
}

// 0x419d10 (slot 19): the hold ends (by the base) and the body is told to stop what it was doing (the dance, the stars, the
// ring); a fly whose place is cut or a wall then dies. A bug not in play returns the command it holds, unchanged. Returns
// the command that was waiting
int BugBrainFly::TakeCommand()
{
	if (!mAlive)
		return mCommand;
	int aCommand = BugObj::TakeCommand();
	if (aCommand != 0 && mBody != nullptr) {
		switch (aCommand) {
		case COMMAND_FREEZE:
			mBody->Slot13(BUG_CMD_FREEZE_OFF, 0, 0);
			break;
		case COMMAND_DAZE:
			mBody->Slot13(BUG_CMD_STARS_OFF, 0, 0);
			break;
		case COMMAND_SHOCK:
			mBody->Slot13(BUG_CMD_RING_OFF, 0, 0);
			break;
		}
	}
	int aColumn = (int)(mState[STATE_X] / GRID_CELL);
	int aRow = (int)(mState[STATE_Y] / GRID_CELL);
	FillGridCell* aCell = nullptr;
	if ((unsigned)aColumn <= 0x27u && (unsigned)aRow <= 0x27u)
		aCell = BugFlatGridCell(aColumn, aRow);
	if (aCell == nullptr || aCell->mCell->mCut || aCell->mPipe->IsActive())
		Zap(true, true, false);
	return aCommand;
}

// 0x41a490 (slot 20): the bug dies (by the base). The fly is flung off: the death track's first key throws it at an angle of
// 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame, and the turn of that key starts from the way the fly
// faces now. The fall's sound is asked for, pitched up a step or not at random
bool BugBrainFly::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
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

// 0x419de0 (slot 21): placed, it chooses its first flight
void BugBrainFly::PlaceAt(int theColumn, int theRow)
{
	BugObj::PlaceAt(theColumn, theRow);
	BeginFlight(0.0f);
}

// 0x41b990 (slot 22; shared with the moth): the fade track, ending with event 8 (the same code as 0x415b60)
void BugBrainFly::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, FLY_BRAIN_EVENT_GONE);
}

// 0x41aaf0 (slot 23): the death track: flung off and falling, shrinking to a fifth (its decays start from where the bug
// is when it is built, and Zap sets the fling), a tumble that fades it in over 100 frames (event 9 at its end), and a last
// frame that fades it out, flattens it and removes it. The tumble's turn is a random side
void BugBrainFly::BuildDeathTrack()
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
	mTumbleKey->mEventId = FLY_BRAIN_EVENT_TUMBLED;
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

// 0x41a590: the flight's track: a flight of 50 frames (whose length BeginFlight sets) that moves the place round the orbit
// (the orbit radius is the speed, easing from the flight's start to the next key's, along the orbit angle, which is the
// flight's angle) and turns the picture to face the angle, and a last frame (event 7) that holds the place it ends at
void BugBrainFly::BuildFlightTrack()
{
	mFlightKey = new AnimKey();
	mFlightKey->SetDuration(FLIGHT_KEY_FRAMES);
	mFlightKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	mFlightKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	mFlightKey->AddValue(STATE_ORBIT_RADIUS, FLIGHT_START_RADIUS, MOD_TOWARD_NEXT);
	mFlightKey->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_TOWARD_NEXT);
	mFlightKey->AddValue(STATE_ROTATION, 0.0f, MOD_COPY_ORBIT_ANGLE);

	mFlightEndKey = new AnimKey();
	mFlightEndKey->mEventId = FLY_BRAIN_EVENT_FLIGHT_DONE;
	mFlightEndKey->SetDuration(1);
	mFlightEndKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	mFlightEndKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	mFlightEndKey->AddValue(STATE_ORBIT_RADIUS, FLIGHT_END_RADIUS, MOD_HOLD);
	mFlightEndKey->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_TOWARD_NEXT);
	mFlightEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_COPY_ORBIT_ANGLE);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlightKey, nullptr);
	aTrack->AddKey(mFlightEndKey, nullptr);
	mFlightTrack = AddTrack(aTrack, nullptr);
}

// 0x41a7e0: the fleeing track: the path's points (the place and the turn read from its tables, by the PATH channel, which
// BeginLeave's key length carries along them), then 20 frames back to where the fly began (the turn to the path's last
// angle, held with the PATH channel at the last point, event 4), and a last frame (event 5) that holds its place
void BugBrainFly::BuildLeaveTrack()
{
	PathTrace* aPath = sFlyPaths[mPathIndex];

	mPathKey = new AnimKey();
	mPathKey->mEventId = FLY_BRAIN_EVENT_PATH_BEGUN;
	mPathKey->SetDuration(PATH_FRAMES);
	mPathKey->AddModifier(STATE_X, false, 0.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, aPath->mX);
	mPathKey->AddModifier(STATE_Y, false, 0.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, aPath->mY);
	mPathKey->AddModifier(STATE_ROTATION, false, 0.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, aPath->mAngle);
	mPathKey->AddValue(STATE_PATH, 0.0f, MOD_TOWARD_NEXT);

	mLandKey = new AnimKey();
	mLandKey->SetDuration(PATH_LAND_FRAMES);
	mLandKey->mEventId = FLY_BRAIN_EVENT_LANDING;
	mLandKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLandKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLandKey->AddValue(STATE_PATH, (float)(aPath->mCount - 1), MOD_HOLD);
	mLandKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mLeaveEndKey = new AnimKey();
	mLeaveEndKey->mEventId = FLY_BRAIN_EVENT_LEFT;
	mLeaveEndKey->SetDuration(1);
	mLeaveEndKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mLeaveEndKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	mLeaveEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mPathKey, nullptr);
	aTrack->AddKey(mLandKey, nullptr);
	aTrack->AddKey(mLeaveEndKey, nullptr);
	mLeaveTrack = AddTrack(aTrack, nullptr);
}

// 0x419e00 (EDI the bug): whether the way ahead is blocked. A fly whose place is outside the grid (up to the 41st column
// and 33rd row, which are not in it) dies; one on a wall is not blocked (it dies by its other checks). Along the ray at
// mFlightAngle from where it is, a pixel at a time for 70 pixels, a place outside the grid, a place whose pipe is part of
// a wall, or one that has been cut stops it
bool BugBrainFly::IsBlockedAhead()
{
	int aColumn = (int)(mState[STATE_X] / GRID_CELL);
	int aRow = (int)(mState[STATE_Y] / GRID_CELL);
	if ((unsigned)aColumn > 0x28u || (unsigned)aRow > 0x20u) {
		Zap(true, true, false);
		return false;
	}
	FillGridCell* aCell = BugFlatGridCell(aColumn, aRow);
	// port (crash): the original reads the place the table's end gives for the 41st column; here it is taken as not blocked
	if (aCell == nullptr || aCell->mPipe->IsActive())
		return false;
	float aX = mState[STATE_X];
	float aY = mState[STATE_Y];
	float aStepX = FloatCos(mFlightAngle);
	float aStepY = FloatSin(mFlightAngle);
	int aStepsLeft = LOOK_AHEAD_STEPS;
	for (;;) {
		aX = aX + aStepX;
		aY = aY - aStepY;
		int aRayColumn = (int)((double)aX / GRID_CELL);
		int aRayRow = (int)((double)aY / GRID_CELL);
		if (aRayColumn < 0 || aRayRow < 0 || aRayColumn >= FILL_GRID_COLUMNS || aRayRow >= FILL_GRID_ROWS)
			return true;
		FillGridCell* aRayCell = &gFillGrid[aRayColumn][aRayRow];
		if (aRayCell->mPipe->IsActive())
			return true;
		if (aRayCell->mCell->mCut)
			return true;
		aStepsLeft--;
		if (aStepsLeft < 0)
			return false;
	}
}

// 0x419f60 (EBP the bug, EBX theEndX; theAngle, theX, theY and theEndY pushed): the ray from the place, a pixel a step (the
// angle's cosine along the screen and its sine up), until a place is outside the grid, or has a pipe that is part of a wall, or
// has been cut, or 800 steps. The answer is the distance from the bug's own place to the first place that stopped it, 0 when
// the ray left the grid or went the whole way. The place the ray stopped at is given back (and the one it ended at, when it
// went the whole way)
float BugBrainFly::CastRay(float theAngle, float theX, float theY, float* theEndX, float* theEndY)
{
	float aDistance = 0.0f;
	int aSteps = 0;
	float aStepX = FloatCos(theAngle);
	float aStepY = FloatSin(theAngle);
	do {
		theX = theX + aStepX;
		theY = theY - aStepY;
		int aColumn = (int)((double)theX / GRID_CELL);
		int aRow = (int)((double)theY / GRID_CELL);
		if (aColumn < 0 || aRow < 0 || aColumn >= FILL_GRID_COLUMNS || aRow >= FILL_GRID_ROWS) {
			aDistance = 0.0f;
			break;
		}
		FillGridCell* aCell = &gFillGrid[aColumn][aRow];
		if (aCell->mPipe->IsActive() || aCell->mCell->mCut) {
			double aDeltaX = (double)mState[STATE_X] - (double)theX;
			double aDeltaY = (double)mState[STATE_Y] - (double)theY;
			aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
			break;
		}
		aSteps++;
	} while (aSteps < RAY_MAX_STEPS);
	if (theEndX != nullptr)
		*theEndX = theX;
	if (theEndY != nullptr)
		*theEndY = theY;
	return aDistance;
}

// 0x41a0a0 (ESI the bug, EAX theEndX, EDI theEndY; theReach and theExtraAngle pushed): chooses the angle of a flight: the
// angle the fly's orbit angle holds (the last flight's), taken down to a whole number of radians (a bug of the original: it
// is an angle), and turned by a draw of 0 to 10 less 5, in degrees: by 20 degrees each when theExtraAngle is 0, by one each
// when it is not (the turn back). The timer is cleared. The ray at that angle is cast; when it is at least 70 pixels long
// the flight goes 62 to 92 pixels along it (theReach; the end is that far from the fly, the sine taken downwards where the ray
// took it upwards, a bug of the original), otherwise the reach is 0 and the end is where the fly is. Returns the angle
float BugBrainFly::ChooseFlight(float theExtraAngle, float* theEndX, float* theEndY, float* theReach)
{
	int aBaseAngle = (int)((double)mState[STATE_ORBIT_ANGLE] + (double)theExtraAngle);
	mTimer = 0;
	int aDegrees;
	if (theExtraAngle == 0.0f)
		aDegrees = ((int)(GameRandomNext() % TURN_DRAW_RANGE) - TURN_DRAW_MIDDLE) * TURN_DEGREES_FREE;
	else
		aDegrees = (int)(GameRandomNext() % TURN_DRAW_RANGE) - TURN_DRAW_MIDDLE;
	float anAngle = (float)((double)aDegrees * PI_F / DEGREES_HALF_TURN + (double)aBaseAngle);
	float aRayEndX;
	float aRayEndY;
	float aRayLength = CastRay(anAngle, mState[STATE_X], mState[STATE_Y], &aRayEndX, &aRayEndY);
	if (aRayLength >= RAY_MIN_REACH) {
		float aReach = (float)((double)(int)(GameRandomNext() % FLIGHT_PIXELS_RANGE) + FLIGHT_MIN_PIXELS);
		*theReach = aReach;
		*theEndX = (float)((double)FloatCos(anAngle) * (double)aReach + (double)mState[STATE_X]);
		*theEndY = (float)((double)FloatSin(anAngle) * (double)aReach + (double)mState[STATE_Y]);
		return anAngle;
	}
	*theReach = 0.0f;
	*theEndX = mState[STATE_X];
	*theEndY = mState[STATE_Y];
	return anAngle;
}

// 0x41a300 (EAX the bug, theExtraAngle pushed): a new flight. The angle and the reach are chosen; with a reach (it is never
// below 0: the other branch, which holds the fly 40 frames where it is, turned to the angle, is never taken) the flight key
// goes from where the fly is to the end ChooseFlight gave over 15 to 40 frames, the picture turning to the angle, the
// speed (the orbit radius) easing from 0.8 to 0.6 along it; with none the key would hold. The flight starts
void BugBrainFly::BeginFlight(float theExtraAngle)
{
	float aReach;
	float anEndX;
	float anEndY;
	float anAngle = ChooseFlight(theExtraAngle, &anEndX, &anEndY, &aReach);
	if (0.0f <= aReach) {
		mFlightKey->mModifiers[STATE_ROTATION]->Set(MOD_COPY_ORBIT_ANGLE, nullptr, 0.0f, 0.0f, 0.0f);
		mFlightKey->SetDuration((int)(GameRandomNext() % FLIGHT_FRAMES_RANGE + FLIGHT_MIN_FRAMES));
		mFlightKey->mValue[STATE_X] = mState[STATE_X];
		mFlightKey->mValue[STATE_Y] = mState[STATE_Y];
		mFlightKey->mValue[STATE_ORBIT_ANGLE] = anAngle;
		mFlightKey->mValue[STATE_ORBIT_RADIUS] = FLIGHT_START_RADIUS;
		mFlightEndKey->mValue[STATE_X] = anEndX;
		mFlightEndKey->mValue[STATE_Y] = anEndY;
		mFlightEndKey->mValue[STATE_ORBIT_ANGLE] = anAngle;
		mFlightEndKey->mValue[STATE_ORBIT_RADIUS] = FLIGHT_END_RADIUS;
	} else {
		mFlightKey->mModifiers[STATE_ROTATION]->Set(MOD_HOLD, nullptr, 0.0f, 0.0f, 0.0f);
		mFlightKey->SetDuration(FLIGHT_WAIT_FRAMES);
		mFlightKey->mValue[STATE_X] = mState[STATE_X];
		mFlightKey->mValue[STATE_Y] = mState[STATE_Y];
		mFlightKey->mValue[STATE_ROTATION] = anAngle;
		mFlightKey->mValue[STATE_ORBIT_ANGLE] = anAngle;
		mFlightKey->mValue[STATE_ORBIT_RADIUS] = 0.0f;
		mFlightEndKey->mValue[STATE_X] = mState[STATE_X];
		mFlightEndKey->mValue[STATE_Y] = mState[STATE_Y];
		mFlightEndKey->mValue[STATE_ORBIT_ANGLE] = anAngle;
		mFlightEndKey->mValue[STATE_ORBIT_RADIUS] = 0.0f;
	}
	mFlightAngle = anAngle;
	StartTrack(mFlightTrack);
}

// 0x41a1f0 (EAX the bug): the fly flees: its place and turn are kept; the path key lasts an eighth of the path's points in
// frames and a draw of 0 to 25 more, from where the fly is (the path's points are relative to it); the last two keys hold
// it, the turn of the second that of the path's last point, the last its own turn from before. The track starts
void BugBrainFly::BeginLeave()
{
	mTimer = 0;
	mLeaveFromX = mState[STATE_X];
	mLeaveFromY = mState[STATE_Y];
	mLeaveFromRotation = mState[STATE_ROTATION];
	int aExtraFrames = (int)(GameRandomNext() % PATH_EXTRA_FRAMES_RANGE);
	PathTrace* aPath = sFlyPaths[mPathIndex];
	mPathKey->SetDuration((int)((double)aPath->mCount * PATH_FRAMES_PER_POINT + (double)aExtraFrames));
	mPathKey->mValue[STATE_X] = mLeaveFromX;
	mPathKey->mValue[STATE_Y] = mLeaveFromY;
	mLandKey->mValue[STATE_X] = mLeaveFromX;
	mLandKey->mValue[STATE_Y] = mLeaveFromY;
	mLandKey->mValue[STATE_ROTATION] = aPath->mAngle[aPath->mCount - 1];
	mLeaveEndKey->mValue[STATE_X] = mLeaveFromX;
	mLeaveEndKey->mValue[STATE_Y] = mLeaveFromY;
	mLeaveEndKey->mValue[STATE_ROTATION] = mLeaveFromRotation;
	StartTrack(mLeaveTrack);
}

// 0x41aee0 (GameWidget's preloads): a path for each of the two (the picture's name has no number in it: both read the same)
void Sexy::PreloadFlyPaths()
{
	for (int i = 0; i < FLY_PATH_COUNT; i++) {
		sFlyPaths[i] = new PathTrace();
		sFlyPaths[i]->Load("images/path_fly");
	}
}
