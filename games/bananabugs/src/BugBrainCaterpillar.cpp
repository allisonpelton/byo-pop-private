#include "BugBrainCaterpillar.h"

#include "BugImg.h"
#include "BugImgCaterpillar.h"
#include "BugShared.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "PipeObj.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The places the caterpillar's box covers each way from its own (SetKind's half-width and half-height)
static const int CATERPILLAR_HALF_SIZE = 3;

// A leg is walked at this many frames a pixel (and one frame more), and the turn at a post takes TURN_FRAMES
static const double LEG_FRAMES_PER_PIXEL = 3.5;       // 0x5e7568
static const int TURN_FRAMES = 0x5d;

// The death: the caterpillar is flung off at (its orbit radius plus this) pixels a frame at an angle of 80 to 100 degrees,
// falls below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = 8.0;
static const int FLING_ANGLE_RANGE = 21;
static const int FLING_ANGLE_FIRST = 80;
static const int FLING_FRAMES = 550;
static const float FLING_END_Y = 1040.0f;
static const double FLING_START_PLUS = (double)3.2f;   // 3.2 as a float, widened (0x5e7508)
static const float FLING_FALL_RATE = -0.16f;
static const int TUMBLE_FRAMES = 100;
static const int FADE_OUT_FRAMES = 1;
static const double TUMBLE_TURN = 50.26548385620117;   // eight turns (0x5e74f8): the tumble spins this far either way
static const float FADE_OUT_ALPHA = -1.0f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's

// The sound the fall asks for is pitched one of this many ways at random
static const unsigned int SPLAT_PITCHES = 3;

// 0x416ea0: SetKind (the kind, and the places its box covers each way), no body yet
BugBrainCaterpillar::BugBrainCaterpillar()
{
	SetKind(BUG_CATERPILLAR, CATERPILLAR_HALF_SIZE, CATERPILLAR_HALF_SIZE);
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

// 0x416f10: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made (a head's
// or a body's picture by the role) as this brain's child, and the step's track built. The caterpillar is shown at once
void BugBrainCaterpillar::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgCaterpillar* aBody = new BugImgCaterpillar(theRole);
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	mMoveTrack = BugSharedBuildMoveTrack(this, mLegKey, mTurnKey, mTurnEndKey);   // 0x4168e0 (shared with the beetle)
	Activate(this, false);
}

// 0x416fd0: the ends of the step's keys (a bug not on the grid hears none): the turn's end takes the heading the post
// gave and starts the next leg, the bug goes when its fade is done, and the tumble's end puts the last key where the
// caterpillar lies
void BugBrainCaterpillar::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case CATERPILLAR_BRAIN_EVENT_LEG_DONE:
	case CATERPILLAR_BRAIN_EVENT_TURN_DONE:
		break;
	case CATERPILLAR_BRAIN_EVENT_TURN_END:
		mHeading = mNextHeading;
		BeginStep();
		break;
	case CATERPILLAR_BRAIN_EVENT_GONE:
		Delete();
		return;
	case CATERPILLAR_BRAIN_EVENT_TUMBLED:
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
void BugBrainCaterpillar::StepSegment()
{
	BugSharedStepSegment(this);
}

// 0x415810 (slot 17; shared): leaves play, and the fade track starts
void BugBrainCaterpillar::Vanish()
{
	BugSharedVanish(this);
}

// 0x417aa0 (slot 18; shared): see BugSharedSetCommand
bool BugBrainCaterpillar::SetCommand(int theCommand, int theFrames)
{
	return BugSharedSetCommand(this, theCommand, theFrames);
}

// 0x41e1f0 (slot 19; shared): see BugSharedTakeCommand
int BugBrainCaterpillar::TakeCommand()
{
	return BugSharedTakeCommand(this);
}

// 0x417320 (slot 20): the bug dies (by the base). The caterpillar is flung off: the death track's first key throws it at an
// angle of 80 to 100 degrees, upwards, at its orbit radius plus 8 pixels a frame (the orbit radius of the brain is read as
// it is), and the three keys' turns start from the way the caterpillar faces now; the tumble spins it a turn a side at
// random. Both turn keys are added on top of the ones the death track was built with. The fall's sound (a splat) is asked
// for, pitched one of three ways at random
bool BugBrainCaterpillar::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
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
	int aPitchRange = (int)(GameRandomNext() % SPLAT_PITCHES);
	sApp->mSoundQueue->Request(SOUND_SPLAT1, aPitchRange, false);
	return true;
}

// 0x417090 (slot 21): placed on a wall piece that runs across it faces right, on one that runs down it faces up; on a post
// it keeps the heading it had. It starts its first step and is shown
void BugBrainCaterpillar::PlaceAt(int theColumn, int theRow)
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
	BeginStep();
	Activate(this, false);
}

// 0x415b60 (slot 22; shared): the fade track, ending with event 5
void BugBrainCaterpillar::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, CATERPILLAR_BRAIN_EVENT_GONE);
}

// 0x4174d0 (slot 23): the death track: flung off and falling (its decays start from where the bug is when it is built,
// and Zap sets the fling), a tumble that fades it in over 100 frames (event 6 at its end), and a last frame that fades
// it out and removes it. The tumble's turn is a random side, drawn here as well as in Zap
void BugBrainCaterpillar::BuildDeathTrack()
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
	mTumbleKey->mEventId = CATERPILLAR_BRAIN_EVENT_TUMBLED;
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

// The post the heading leads to from the place the caterpillar is on: the grid's pipe of the place has a link in each heading
PipeObj* BugBrainCaterpillar::GetNextPost()
{
	// port (crash): the original reads the grid at any place, the 0x7fff of a bug with none included
	if (mColumn < 0 || mColumn >= FILL_GRID_COLUMNS || mRow < 0 || mRow >= FILL_GRID_ROWS)
		return nullptr;
	PipeObj* aPipe = gFillGrid[mColumn][mRow].mPipe;
	if (aPipe == nullptr)
		return nullptr;
	return BugPostLink(aPipe, mHeading);
}

// 0x417110 (EAX the bug): a step: the heading chosen at the last post is taken, the next is chosen at the post it leads to
// (never at random: the first way on), and the three keys are set and started: the leg key starts where the caterpillar
// is, facing its heading; the turn key is at the post, facing the same way, for 93 frames; the last key is at the post
// facing the quarter turn the next heading is from it. The leg takes one frame more than 3.5 a pixel of its length, cut
// to whole frames. When the body would be on the wrong side of the wall for the turn (above the line and turning one way,
// below it and turning the other) the body is told to flip (0x6a). Nothing happens when the heading leads nowhere
void BugBrainCaterpillar::BeginStep()
{
	PipeObj* aPost = GetNextPost();
	if (aPost == nullptr)
		return;
	mHeading = mNextHeading;
	mNextHeading = ChooseHeading(aPost, false, true);
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
	mLegKey->SetDuration(1 - (int)((double)aDistance * -LEG_FRAMES_PER_PIXEL));
	mTurnKey->SetDuration(TURN_FRAMES);
	BugImgCaterpillar* aBody = static_cast<BugImgCaterpillar*>(mBody);
	if (aBody != nullptr) {
		int aTurn = TurnDirection(mNextHeading, mHeading);
		bool aAbove = aBody->mState[STATE_Y] < 0.0f;
		if (aAbove ? aTurn > 0 : aTurn < 0)
			aBody->Slot13(BUG_CMD_CATERPILLAR_FLIP, 0, 0);
	}
	StartTrack(mMoveTrack);
}
