#include "BugBrainMoth.h"

#include "BugImg.h"
#include "BugImgMoth.h"
#include "BugShared.h"
#include "FillObj.h"
#include "GameApp.h"
#include "PathTrace.h"
#include "PipeObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The places the moth's box covers each way from its own (SetKind's half-width and half-height)
static const int MOTH_HALF_WIDTH = 2;
static const int MOTH_HALF_HEIGHT = 1;
// The constructor stores the fly's kind, 7 (a bug of the original: LevelObj makes the moth for BUG_MOTH, 9, and nothing
// reads the difference)
static const int MOTH_STORED_KIND = BUG_FLY;

static const double GRID_CELL = 31.0;                  // the grid's places are 31 pixels apart (BUG_GRID_CELL)

// The flight paths, the original's two tables at 0x60e24c and 0x60e250 (the fly's are its own)
static const int MOTH_PATH_COUNT = 2;
static PathTrace* sMothPaths[MOTH_PATH_COUNT];

// The look ahead: the loop counts down from 50 to 0 (51 steps of a pixel along the flight's angle)
static const int LOOK_AHEAD_STEPS = 0x32;
// After a wall is seen ahead, the next flights ignore the monkey this many frames
static const int WANDER_FRAMES = 0x78;
// The flight's angle is the monkey's, or the last flight's when it ignores the monkey, turned by a draw of 0 to 6 less 3
// degrees
static const unsigned int TURN_DRAW_RANGE = 7;
static const int TURN_DRAW_MIDDLE = 3;
static const double DEGREES_HALF_TURN = 180.0;
// The moth flies toward the monkey when it is this far from it or farther; closer it stays
static const float FLY_TO_MIN_DISTANCE = 32.0f;
// A flight toward the monkey lasts 20 frames and a draw of 26 more; a stay lasts 40
static const int FLIGHT_MIN_FRAMES = 0x14;
static const unsigned int FLIGHT_FRAMES_RANGE = 0x1a;
static const int FLIGHT_STAY_FRAMES = 0x28;
// The speed (the orbit radius) at the start and end of a flight toward the monkey
static const float FLIGHT_START_RADIUS = 0.7f;
static const float FLIGHT_END_RADIUS = 0.5f;
// The flight track's own keys start at these (BuildFlightTrack); every flight replaces them
static const float TRACK_START_RADIUS = 0.8f;
static const float TRACK_END_RADIUS = 0.6f;

// The moth flees from the monkey when its speed (averaged) is at least this and it is within this reach
static const double FLEE_MIN_SPEED = 1.0;
static const float FLEE_REACH = 124.0f;
// Its flight along the path takes 0.155 of the points in frames, and a draw of 41 and 5 more
static const double PATH_FRAMES_PER_POINT = 0.155;
static const unsigned int PATH_EXTRA_FRAMES_RANGE = 0x29;
static const int PATH_EXTRA_FRAMES_FIRST = 5;
static const int PATH_LAND_FRAMES = 0x14;
static const int PATH_FRAMES = 0x64;

// The flight's keys: 50 frames, the end a frame
static const int FLIGHT_KEY_FRAMES = 0x32;

// The death: the moth is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees, falls
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

// 0x41af60: SetKind (the kind, and the places its box covers each way), no body yet, nothing placed, flying, a heading at random
BugBrainMoth::BugBrainMoth()
{
	mBody = nullptr;
	mUnused2A4 = MOTH_HALF_WIDTH;
	SetKind(MOTH_STORED_KIND, MOTH_HALF_WIDTH, MOTH_HALF_HEIGHT);
	mUnused354 = 0.0f;
	mHoldFrames = 0;
	mUnused34C = true;
	mMothState = MOTH_STATE_FLYING;
	mPlaced = false;
	mPathIndex = 0;
	mWanderFrames = 0;
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

// 0x41b000: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this brain's
// child, the flight's and the fleeing tracks built, and the first flight chosen (toward the monkey). The moth is shown at once
void BugBrainMoth::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgMoth* aBody = new BugImgMoth();
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildFlightTrack();
	BuildLeaveTrack();
	BeginFlight(false);
	Activate(this, false);
}

// 0x41b0d0: the base's update; a moth in play (not fleeing along the path) counts its wander frames down; when the monkey is
// fast and close and it is not already fleeing nor held, it flees (it cannot be hit while it does: mFlying); while
// flying it looks ahead and, when a wall or a cut place is in the way, takes the way it last went and ignores the monkey for
// a while; a moth that has landed flies again; a moth held by a freeze whose place is cut or on a wall dies
bool BugBrainMoth::Update()
{
	if (!BugObj::Update())
		return false;
	if (!mFlying) {
		if (mWanderFrames > 0)
			mWanderFrames--;
		if (mAlive) {
			int aState = mMothState;
			if (aState != MOTH_STATE_FLEEING && mCommand == 0 && Player::GetSpeedAverage() >= FLEE_MIN_SPEED && Player::IsNear(this, FLEE_REACH)) {
				mFlying = true;
				mAirborne = true;
				mMothState = MOTH_STATE_FLEEING;
				BeginLeave();
				return true;
			}
			if (aState == MOTH_STATE_FLYING) {
				if (IsBlockedAhead()) {
					mWanderFrames = WANDER_FRAMES;
					BeginFlight(true);
				}
			} else if (aState == MOTH_STATE_LANDED) {
				mAirborne = false;
				mMothState = MOTH_STATE_FLYING;
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

// 0x41b220: the ends of the keys (a bug not on the grid hears none): the leave track's second key lets it be hit again, its
// end lands the moth (it dies if its place is cut or a wall, else it flies on, toward the monkey), the flight's end chooses
// a new flight, the bug goes when its fade is done, and the tumble's end puts the last key where the moth lies
void BugBrainMoth::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case MOTH_BRAIN_EVENT_PATH_BEGUN:
	case MOTH_BRAIN_EVENT_UNUSED_6:
		break;
	case MOTH_BRAIN_EVENT_LANDING:
		mFlying = false;
		return;
	case MOTH_BRAIN_EVENT_LEFT: {
		int aColumn = (int)(mState[STATE_X] / GRID_CELL);
		int aRow = (int)(mState[STATE_Y] / GRID_CELL);
		FillGridCell* aCell = nullptr;
		if ((unsigned)aColumn <= 0x27u && (unsigned)aRow <= 0x27u)
			aCell = BugFlatGridCell(aColumn, aRow);
		if (aCell == nullptr || aCell->mCell->mCut) {
			Zap(true, true, false);
			return;
		}
		if (!aCell->mPipe->IsActive()) {
			BeginFlight(false);
			mMothState = MOTH_STATE_LANDED;
			return;
		}
		Zap(true, true, false);
		break;
	}
	case MOTH_BRAIN_EVENT_FLIGHT_DONE:
		BeginFlight(false);
		return;
	case MOTH_BRAIN_EVENT_GONE:
		Delete();
		return;
	case MOTH_BRAIN_EVENT_TUMBLED:
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
void BugBrainMoth::Vanish()
{
	BugSharedVanish(this);
}

// 0x417aa0 (slot 18; shared): see BugSharedSetCommand
bool BugBrainMoth::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommand(this, theCommand, theFrames);
}

// 0x41b380 (slot 19): the hold ends (by the base) and the body is told to stop what it was doing (the dance, the stars, the
// ring); a moth whose place is cut or a wall then dies. A bug not in play returns 0 (the fly's returns the command it holds).
// Returns the command that was waiting
int BugBrainMoth::TakeCommand()
{
	if (!mAlive)
		return 0;
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

// 0x41b890 (slot 20): the bug dies (by the base). The moth is flung off: the death track's first key throws it at an angle
// of 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame, and the turn of that key starts from the way the
// moth faces now. The fall's sound is asked for, pitched up a step or not at random
bool BugBrainMoth::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
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

// 0x41b440 (slot 21): placed, it chooses its first flight (toward the monkey)
void BugBrainMoth::PlaceAt(int theColumn, int theRow)
{
	BugObj::PlaceAt(theColumn, theRow);
	BeginFlight(false);
}

// 0x41b990 (slot 22; shared with the fly): the fade track, ending with event 8 (the same code as 0x415b60)
void BugBrainMoth::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, MOTH_BRAIN_EVENT_GONE);
}

// 0x41c080 (slot 23): the death track: flung off and falling, shrinking to a fifth (its decays start from where the bug
// is when it is built, and Zap sets the fling), a tumble that fades it in over 100 frames (event 9 at its end), and a last
// frame that fades it out, flattens it and removes it. The tumble's turn is a random side (the same code as the fly's,
// 0x41aaf0)
void BugBrainMoth::BuildDeathTrack()
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
	mTumbleKey->mEventId = MOTH_BRAIN_EVENT_TUMBLED;
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

// 0x41bb20 (EBX the bug): the flight's track: a flight of 50 frames (whose length BeginFlight sets) that moves the place
// round the orbit (the orbit radius is the speed, easing from the flight's start to the next key's, along the orbit angle,
// which is the flight's angle) and turns the picture to face the angle, and a last frame (event 7) that holds the place the
// flight ends at (the same code as the fly's, 0x41a590)
void BugBrainMoth::BuildFlightTrack()
{
	mFlightKey = new AnimKey();
	mFlightKey->SetDuration(FLIGHT_KEY_FRAMES);
	mFlightKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	mFlightKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	mFlightKey->AddValue(STATE_ORBIT_RADIUS, TRACK_START_RADIUS, MOD_TOWARD_NEXT);
	mFlightKey->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_TOWARD_NEXT);
	mFlightKey->AddValue(STATE_ROTATION, 0.0f, MOD_COPY_ORBIT_ANGLE);

	mFlightEndKey = new AnimKey();
	mFlightEndKey->mEventId = MOTH_BRAIN_EVENT_FLIGHT_DONE;
	mFlightEndKey->SetDuration(1);
	mFlightEndKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	mFlightEndKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	mFlightEndKey->AddValue(STATE_ORBIT_RADIUS, TRACK_END_RADIUS, MOD_HOLD);
	mFlightEndKey->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_TOWARD_NEXT);
	mFlightEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_COPY_ORBIT_ANGLE);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mFlightKey, nullptr);
	aTrack->AddKey(mFlightEndKey, nullptr);
	mFlightTrack = AddTrack(aTrack, nullptr);
}

// 0x41bd70 (EBX the bug): the fleeing track: the path's points (the place and the turn read from its tables, by the PATH
// channel, which BeginLeave's key length carries along them), then 20 frames back to where the moth began (the turn to the
// path's last angle, held with the PATH channel at the last point, event 4), and a last frame (event 5) that holds its place
// (the same code as the fly's, 0x41a7e0, but for the path's tables and the first key's 100 frames)
void BugBrainMoth::BuildLeaveTrack()
{
	PathTrace* aPath = sMothPaths[mPathIndex];

	mPathKey = new AnimKey();
	mPathKey->mEventId = MOTH_BRAIN_EVENT_PATH_BEGUN;
	mPathKey->SetDuration(PATH_FRAMES);
	mPathKey->AddModifier(STATE_X, false, 0.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, aPath->mX);
	mPathKey->AddModifier(STATE_Y, false, 0.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, aPath->mY);
	mPathKey->AddModifier(STATE_ROTATION, false, 0.0f, MOD_PATH, 0.0f, 0.0f, 0.0f, aPath->mAngle);
	mPathKey->AddValue(STATE_PATH, 0.0f, MOD_TOWARD_NEXT);

	mLandKey = new AnimKey();
	mLandKey->SetDuration(PATH_LAND_FRAMES);
	mLandKey->mEventId = MOTH_BRAIN_EVENT_LANDING;
	mLandKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLandKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLandKey->AddValue(STATE_PATH, (float)(aPath->mCount - 1), MOD_HOLD);
	mLandKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mLeaveEndKey = new AnimKey();
	mLeaveEndKey->mEventId = MOTH_BRAIN_EVENT_LEFT;
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

// 0x41b460 (EAX the bug): whether the way ahead is blocked. Along the ray at mFlightAngle from where it is, a pixel at a time
// for 51 pixels, a place outside the grid, a place whose pipe is part of a wall, or one that has been cut stops it. (The
// fly's look, 0x419e00, first kills a fly whose place is outside the grid; the moth's does not)
bool BugBrainMoth::IsBlockedAhead()
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

// 0x41b660 (EAX the bug, theIgnoreMonkey pushed): a new flight. With the monkey in play and not ignored (theIgnoreMonkey, or
// wander frames left) the flight's angle is the way to the monkey (from the places the two are drawn at) and its distance the
// distance between their places; otherwise the angle is the last flight's and the distance 0. The angle is turned by a draw
// of 0 to 6 less 3 degrees. From 32 pixels away the flight key goes from where the moth is over 20 to 45 frames, the
// picture turning to the angle, the speed (the orbit radius) easing from 0.7 to 0.5 along it; closer, the key lasts 40
// frames and the speed is 0. The flight starts
void BugBrainMoth::BeginFlight(bool theIgnoreMonkey)
{
	float aDistance;
	float anAngle;
	Player* aPlayer = Player::sCurrent;
	if (aPlayer == nullptr || theIgnoreMonkey || mWanderFrames != 0) {
		aDistance = 0.0f;
		anAngle = mState[STATE_ORBIT_ANGLE];
	} else {
		double aDeltaY = (double)mState[STATE_Y] - (double)aPlayer->mState[STATE_Y];
		double aDeltaX = (double)mState[STATE_X] - (double)aPlayer->mState[STATE_X];
		aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
		float aUp = (float)((double)mDrawn[STATE_Y] - (double)aPlayer->mDrawn[STATE_Y]);
		float anAcross = (float)((double)aPlayer->mDrawn[STATE_X] - (double)mDrawn[STATE_X]);
		anAngle = FloatAtan2(aUp, anAcross);
	}
	int aDegrees = (int)(GameRandomNext() % TURN_DRAW_RANGE) - TURN_DRAW_MIDDLE;
	anAngle = (float)((double)aDegrees * PI_F / DEGREES_HALF_TURN + (double)anAngle);
	if (FLY_TO_MIN_DISTANCE <= aDistance) {
		mFlightKey->SetDuration((int)(GameRandomNext() % FLIGHT_FRAMES_RANGE + FLIGHT_MIN_FRAMES));
		mFlightKey->mValue[STATE_X] = mState[STATE_X];
		mFlightKey->mValue[STATE_Y] = mState[STATE_Y];
		mFlightKey->mValue[STATE_ROTATION] = mState[STATE_ORBIT_ANGLE];
		mFlightKey->mValue[STATE_ORBIT_ANGLE] = anAngle;
		mFlightKey->mValue[STATE_ORBIT_RADIUS] = FLIGHT_START_RADIUS;
		mFlightEndKey->mValue[STATE_X] = mState[STATE_X];
		mFlightEndKey->mValue[STATE_Y] = mState[STATE_Y];
		mFlightEndKey->mValue[STATE_ORBIT_ANGLE] = anAngle;
		mFlightEndKey->mValue[STATE_ORBIT_RADIUS] = FLIGHT_END_RADIUS;
	} else {
		mFlightKey->SetDuration(FLIGHT_STAY_FRAMES);
		mFlightKey->mValue[STATE_X] = mState[STATE_X];
		mFlightKey->mValue[STATE_Y] = mState[STATE_Y];
		mFlightKey->mValue[STATE_ROTATION] = mState[STATE_ORBIT_ANGLE];
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

// 0x41b550 (EAX the bug): the moth flees: its place and turn are kept; the path key lasts 0.155 of the path's points in
// frames and a draw of 5 to 45 more, from where the moth is (the path's points are relative to it); the last two keys
// hold it, the turn of the second that of the path's last point, the last its own turn from before. The track starts
void BugBrainMoth::BeginLeave()
{
	mLeaveFromX = mState[STATE_X];
	mLeaveFromY = mState[STATE_Y];
	mLeaveFromRotation = mState[STATE_ROTATION];
	PathTrace* aPath = sMothPaths[mPathIndex];
	int aExtraFrames = (int)(GameRandomNext() % PATH_EXTRA_FRAMES_RANGE) + PATH_EXTRA_FRAMES_FIRST;
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

// 0x41c470 (GameWidget's preloads): a path for each of the two (the picture's name has no number in it: both read the same)
void Sexy::PreloadMothPaths()
{
	for (int i = 0; i < MOTH_PATH_COUNT; i++) {
		sMothPaths[i] = new PathTrace();
		sMothPaths[i]->Load("images/path_moth");
	}
}
