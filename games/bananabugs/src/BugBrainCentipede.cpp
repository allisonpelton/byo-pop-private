#include "BugBrainCentipede.h"

#include "BugImg.h"
#include "BugImgCentipede.h"
#include "BugShared.h"
#include "FillObj.h"
#include "LevelObj.h"
#include "PipeObj.h"
#include "RopeObj.h"

using namespace Sexy;

// The places the centipede's box covers each way from its own (SetKind's half-width and half-height)
static const int CENTIPEDE_HALF_SIZE = 4;

// A leg and a cut are walked at this many frames a pixel (and one frame more): 2.35 as a float, widened (0x5e7558)
static const double FRAMES_PER_PIXEL = (double)2.35f;

// The centipede walks to a place this many pixels short of the post, and cuts across to a place this many along the next
// heading from it
static const float CORNER_CUT = 15.0f;

// A leg of more than this many places (a place is 31 pixels) makes the body sway; mRunLength counts it in this many
static const float LONG_LEG_PLACES = 5.0f;
static const double RUN_PLACES = 5.0;

// While a rope is flying the centipede is updated this many times more each frame
static const int HURRY_UPDATES = 3;

// The death: the centipede is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees,
// falls below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = 8.0;
static const int FLING_ANGLE_RANGE = 21;
static const int FLING_ANGLE_FIRST = 80;
static const int FLING_FRAMES = 550;
static const float FLING_END_Y = 1040.0f;
static const double FLING_START_PLUS = 8.0;
static const float FLING_FALL_RATE = -0.17f;
static const int TUMBLE_FRAMES = 100;
static const int FADE_OUT_FRAMES = 1;
static const double TUMBLE_TURN = 50.26548385620117;   // eight turns (0x5e74f8): the tumble spins this far either way
static const float FADE_OUT_ALPHA = -1.0f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's

// 0x417840: SetKind (the kind, and the places its box covers each way), no body yet
BugBrainCentipede::BugBrainCentipede()
{
	SetKind(BUG_CENTIPEDE, CENTIPEDE_HALF_SIZE, CENTIPEDE_HALF_SIZE);
	mBody = nullptr;
	mHoldFrames = 0;
	mRunLength = 0;
	mUnused33C = false;
	mUnused2A4 = 1;
	// port: the original leaves these as the memory had them until Setup and PlaceAt set them
	mNextHeading = HEADING_UP;
	mMoveTrack = 0;
	mLegKey = nullptr;
	mCutKey = nullptr;
	mStepEndKey = nullptr;
	mFlingKey = nullptr;
	mTumbleKey = nullptr;
	mFadeOutKey = nullptr;
}

// 0x4178d0: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made (a head's, a
// body's or a tail's picture by the role) as this brain's child, and the step's track built. The centipede is shown at once
void BugBrainCentipede::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgCentipede* aBody = new BugImgCentipede(theRole);
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildMoveTrack();
	Activate(this, false);
}

// 0x417990: the base's update, and while the rope is flying three more of it, each followed by the body's own: the
// centipede hurries. False when the bug is gone
bool BugBrainCentipede::Update()
{
	if (!BugObj::Update())
		return false;
	// port (crash): the original reads the level's rope unchecked
	RopeObj* aRope = RopeObj::Current();
	if (aRope != nullptr && aRope->GetState() == ROPE_FLYING) {
		for (int anUpdate = 0; anUpdate < HURRY_UPDATES; anUpdate++) {
			if (!BugObj::Update())
				return false;
			// port (crash): the original does not look whether the bug has a body
			if (mBody != nullptr)
				mBody->Update();
		}
	}
	return true;
}

// 0x4179e0: the ends of the step's keys (a bug not on the grid hears none): the last key's end takes the heading the post
// gave and starts the next step, the bug goes when its fade is done, and the tumble's end puts the last key where the
// centipede lies
void BugBrainCentipede::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case CENTIPEDE_BRAIN_EVENT_LEG_DONE:
	case CENTIPEDE_BRAIN_EVENT_CUT_DONE:
		break;
	case CENTIPEDE_BRAIN_EVENT_STEP_END:
		mHeading = mNextHeading;
		BeginStep();
		break;
	case CENTIPEDE_BRAIN_EVENT_GONE:
		Delete();
		return;
	case CENTIPEDE_BRAIN_EVENT_TUMBLED:
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

// 0x41c620 (slot 15; shared): the bug is updated, then its body
void BugBrainCentipede::StepSegment()
{
	BugSharedStepSegment(this);
}

// 0x415810 (slot 17; shared): leaves play, and the fade track starts
void BugBrainCentipede::Vanish()
{
	BugSharedVanish(this);
}

// 0x417aa0 (slot 18; shared): see BugSharedSetCommand
bool BugBrainCentipede::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommand(this, theCommand, theFrames);
}

// 0x417b20: the hold ends (the base clears the command and returns it) and the body is told to stop what it was doing:
// the freeze, the stars or the ring. Unlike 0x41e1f0 it does not look first whether the bug is in play. The original returns
// 0 always, whatever command was waiting
int BugBrainCentipede::TakeCommand()
{
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
	return 0;
}

// 0x417fa0 (slot 20): the bug dies (by the base) and its hold is cleared. The centipede is flung off: the death track's first
// key throws it at an angle of 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame (the orbit radius of
// the brain is read as it is), and the three keys' turns start from the way the centipede faces now; the tumble spins it a
// turn a side at random. Both turn keys are added on top of the ones the death track was built with
bool BugBrainCentipede::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
{
	if (!BugObj::Zap(theFollowers, theStartTrack, thePeaHit))
		return false;
	mHoldFrames = 0;
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

// 0x417bf0 (slot 21): placed on a wall piece that runs across it faces right, on one that runs down it faces down (the
// ant, the beetle and the caterpillar face up there); on a post it keeps the heading it had. It starts its first step and
// is shown
void BugBrainCentipede::PlaceAt(int theColumn, int theRow)
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
			mNextHeading = HEADING_DOWN;
			mHeading = HEADING_DOWN;
		}
	}
	BeginStep();
	Activate(this, false);
}

// 0x418130 (slot 22; shared with the roach and the roly-poly: the code of 0x415b60 with the event 6): the fade track
void BugBrainCentipede::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, CENTIPEDE_BRAIN_EVENT_GONE);
}

// 0x418510 (slot 23): the death track: flung off and falling (its decays start from where the bug is when it is built,
// and Zap sets the fling), a tumble that fades it in over 100 frames (event 7 at its end), and a last frame that fades
// it out and removes it. The tumble's turn is a random side, drawn here as well as in Zap
void BugBrainCentipede::BuildDeathTrack()
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
	mTumbleKey->mEventId = CENTIPEDE_BRAIN_EVENT_TUMBLED;
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

// 0x4182c0: the step's track: three keys that move the centipede's place, its turn (a quarter turn for a corner) and nothing
// else; where they go is set by each step (BeginStep). The keys' own values are all 0, easing toward the next key; the
// first has no length of its own (the step sets it)
void BugBrainCentipede::BuildMoveTrack()
{
	mLegKey = new AnimKey();
	mLegKey->mEventId = CENTIPEDE_BRAIN_EVENT_LEG_DONE;
	mLegKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mCutKey = new AnimKey();
	mCutKey->mEventId = CENTIPEDE_BRAIN_EVENT_CUT_DONE;
	mCutKey->SetDuration(1);
	mCutKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mCutKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mCutKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mStepEndKey = new AnimKey();
	mStepEndKey->mEventId = CENTIPEDE_BRAIN_EVENT_STEP_END;
	mStepEndKey->SetDuration(1);
	mStepEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mStepEndKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mStepEndKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mLegKey, nullptr);
	aTrack->AddKey(mCutKey, nullptr);
	aTrack->AddKey(mStepEndKey, nullptr);
	mMoveTrack = AddTrack(aTrack, nullptr);
}

// The post the heading leads to from the place the centipede is on: the grid's pipe of the place has a link in each heading
PipeObj* BugBrainCentipede::GetNextPost()
{
	// port (crash): the original reads the grid at any place, the 0x7fff of a bug with none included
	if (mColumn < 0 || mColumn >= FILL_GRID_COLUMNS || mRow < 0 || mRow >= FILL_GRID_ROWS)
		return nullptr;
	PipeObj* aPipe = gFillGrid[mColumn][mRow].mPipe;
	if (aPipe == nullptr)
		return nullptr;
	return BugPostLink(aPipe, mHeading);
}

// 0x417c70 (EAX the bug): a step: the heading chosen at the last post is taken, the next is chosen at the post it leads to
// (never at random: the first way on), and the three keys are set and started. The leg key starts where the centipede is,
// facing its heading; the cut key is 15 pixels short of the post (on the side it comes from), facing the same way; the last
// key is 15 pixels along the next heading from the post, facing the quarter turn the next heading is from it. Each of the
// leg and the cut takes one frame more than 2.35 a pixel of its length, cut to whole frames, and the last key one frame.
// A leg over five places makes mRunLength the number of fives (else 0), and then the body is told (0x6e) to sway.
// Nothing happens when the heading leads nowhere
void BugBrainCentipede::BeginStep()
{
	int aWalkedHeading = mHeading;
	PipeObj* aPost = GetNextPost();
	if (aPost == nullptr)
		return;
	mHeading = mNextHeading;
	mNextHeading = ChooseHeading(aPost, false, true);
	float aPostX = (float)(aPost->mColumn * BUG_GRID_CELL);
	float aPostY = (float)(aPost->mRow * BUG_GRID_CELL);
	// (the original leaves these as the stack had them for a heading outside 0 to 3, which none ever is)
	float aCutX = aPostX;
	float aCutY = aPostY;
	float aEndX = aPostX;
	float aEndY = aPostY;
	switch (aWalkedHeading) {
	case HEADING_UP:
		aCutX = aPostX;
		aCutY = aPostY + CORNER_CUT;
		break;
	case HEADING_RIGHT:
		aCutX = aPostX - CORNER_CUT;
		aCutY = aPostY;
		break;
	case HEADING_DOWN:
		aCutX = aPostX;
		aCutY = aPostY - CORNER_CUT;
		break;
	case HEADING_LEFT:
		aCutX = aPostX + CORNER_CUT;
		aCutY = aPostY;
		break;
	}
	switch (mNextHeading) {
	case HEADING_UP:
		aEndY = aPostY - CORNER_CUT;
		break;
	case HEADING_RIGHT:
		aEndX = aPostX + CORNER_CUT;
		break;
	case HEADING_DOWN:
		aEndY = aPostY + CORNER_CUT;
		break;
	case HEADING_LEFT:
		aEndX = aPostX - CORNER_CUT;
		break;
	}
	mLegKey->mValue[STATE_X] = mState[STATE_X];
	mLegKey->mValue[STATE_Y] = mState[STATE_Y];
	mLegKey->mValue[STATE_ROTATION] = BugHeadingAngle(mHeading);
	mCutKey->mValue[STATE_X] = aCutX;
	mCutKey->mValue[STATE_Y] = aCutY;
	mCutKey->mValue[STATE_ROTATION] = BugHeadingAngle(mHeading);
	mStepEndKey->SetDuration(1);
	mStepEndKey->mValue[STATE_X] = aEndX;
	mStepEndKey->mValue[STATE_Y] = aEndY;
	mStepEndKey->mValue[STATE_ROTATION] = BugTurnAngle(mHeading, mNextHeading, mCutKey->mValue[STATE_ROTATION]);
	// The original squares and adds on the FPU stack, rounding only the sum
	double aLegDeltaY = (double)mLegKey->mValue[STATE_Y] - (double)mCutKey->mValue[STATE_Y];
	double aLegDeltaX = (double)mLegKey->mValue[STATE_X] - (double)mCutKey->mValue[STATE_X];
	float aLegDistance = FloatSqrt((float)(aLegDeltaX * aLegDeltaX + aLegDeltaY * aLegDeltaY));
	mLegKey->SetDuration(1 - (int)((double)aLegDistance * -FRAMES_PER_PIXEL));
	float aLegPlaces = (float)((double)aLegDistance / (double)BUG_GRID_CELL);
	if (aLegPlaces <= LONG_LEG_PLACES)
		mRunLength = 0;
	else
		mRunLength = (int)((double)aLegPlaces / RUN_PLACES);
	double aCutDeltaY = (double)mCutKey->mValue[STATE_Y] - (double)mStepEndKey->mValue[STATE_Y];
	double aCutDeltaX = (double)mCutKey->mValue[STATE_X] - (double)mStepEndKey->mValue[STATE_X];
	float aCutDistance = FloatSqrt((float)(aCutDeltaX * aCutDeltaX + aCutDeltaY * aCutDeltaY));
	mCutKey->SetDuration(1 - (int)((double)aCutDistance * -FRAMES_PER_PIXEL));
	StartTrack(mMoveTrack);
	if (mRunLength > 0 && mBody != nullptr)
		mBody->Slot13(BUG_CMD_CENTIPEDE_SWAY_DOWN, 0, 0);
}
