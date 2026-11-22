#include "BugBrainBeetle.h"

#include "BugImg.h"
#include "BugImgBeetle.h"
#include "BugShared.h"
#include "FillObj.h"
#include "LevelObj.h"
#include "PipeObj.h"

#include <cstring>

using namespace Sexy;

// The places the beetle's box covers each way from its own (SetKind's half-width and half-height)
static const int BEETLE_HALF_SIZE = 2;

// The constructor leaves the command at 2 (the register that held the half-size): see the constructor
static const int BEETLE_START_COMMAND = 2;

// A leg longer than this many pixels is walked at LONG_FRAMES_PER_PIXEL and eased like a cosine; a shorter one, or one
// on which the body flips over, at SHORT_FRAMES_PER_PIXEL and linearly. The turn at a post takes TURN_FRAMES
static const float LONG_LEG_PIXELS = 128.0f;
static const double LONG_FRAMES_PER_PIXEL = (double)2.7f;   // 2.7 as a float, widened (0x5e7578)
static const double SHORT_FRAMES_PER_PIXEL = 3.0;           // 0x5e7570
static const int TURN_FRAMES = 88;

// The death: the beetle is flung off at (its orbit radius plus this) pixels a frame at an angle of 75 to 105 degrees, falls
// below the screen (Y over 1040), tumbles and fades
static const double FLING_SPEED_PLUS = (double)8.6f;   // 8.6 as a float, widened (0x5e7778)
static const int FLING_ANGLE_RANGE = 31;
static const int FLING_ANGLE_FIRST = 75;
static const int FLING_FRAMES = 550;
static const float FLING_END_Y = 1040.0f;
static const double FLING_START_PLUS = (double)3.2f;   // 3.2 as a float, widened (0x5e7508)
static const float FLING_FALL_RATE = -0.18f;
static const int TUMBLE_FRAMES = 100;
static const int FADE_OUT_FRAMES = 1;
static const double TUMBLE_TURN = 50.26548385620117;   // eight turns (0x5e74f8): the tumble spins this far either way
static const float FADE_OUT_ALPHA = -1.0f;
static const double FADE_OUT_DROP = 20.0;              // the last key is 20 pixels lower than the tumble's

// 0x416220: SetKind (the kind, and the places its box covers each way), no body yet
BugBrainBeetle::BugBrainBeetle()
{
	SetKind(BUG_BEETLE, BEETLE_HALF_SIZE, BEETLE_HALF_SIZE);
	mBody = nullptr;
	mUnused2A4 = 1;
	// The original stores the register that held the half-size (2) in the command: a beetle that was never held starts
	// with the command 2 (a daze), where the other kinds have what BugObj leaves
	mCommand = BEETLE_START_COMMAND;
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

// 0x4162a0: the base's setup (which makes the fade and death tracks), the waiting steps kept, the body made as this
// brain's child, and the step's track built. The beetle is shown at once
void BugBrainBeetle::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	BugObj::Setup(theRole, theWaitFrames, theHead, thePrevious);
	SetSegmentSteps(theWaitFrames);
	BugImgBeetle* aBody = new BugImgBeetle();
	mBody = aBody;
	aBody->SetParent(this, nullptr);
	SetSetupBody(aBody);
	BuildMoveTrack();
	Activate(this, false);
}

// 0x416360: the ends of the step's keys (a bug not on the grid hears none): the turn's end takes the heading the post
// gave and starts the next leg (the next heading is chosen at random), the bug goes when its fade is done, and the
// tumble's end puts the last key where the beetle lies
void BugBrainBeetle::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || !mPlaced)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case BEETLE_BRAIN_EVENT_LEG_DONE:
	case BEETLE_BRAIN_EVENT_TURN_DONE:
		break;
	case BEETLE_BRAIN_EVENT_TURN_END:
		mHeading = mNextHeading;
		BeginStep(true);
		return;
	case BEETLE_BRAIN_EVENT_GONE:
		Delete();
		break;
	case BEETLE_BRAIN_EVENT_TUMBLED:
		mTumbleKey->mValue[STATE_X] = mState[STATE_X];
		mTumbleKey->mValue[STATE_Y] = mState[STATE_Y];
		mFadeOutKey->mValue[STATE_X] = mState[STATE_X];
		mFadeOutKey->mValue[STATE_Y] = (float)((double)mState[STATE_Y] + FADE_OUT_DROP);
		return;
	default:
		BugObj::Notify(theObj);
	}
}

// 0x41c620 (slot 15; shared): the bug is updated, then its body
void BugBrainBeetle::StepSegment()
{
	BugSharedStepSegment(this);
}

// 0x415810 (slot 17; shared): leaves play, and the fade track starts
void BugBrainBeetle::Vanish()
{
	BugSharedVanish(this);
}

// 0x416420 (slot 18): the bug and its column are held and the body is told what for: freezing is a dance, dazing shows
// the stars, shock the ring. Unlike the shared one (0x417aa0) it goes by the command it was given, not the one the bug
// holds. The original returns false always, whatever the base said
bool BugBrainBeetle::SetCommand(int theCommand, int theFrames)
{
	if (BugObj::SetCommand(theCommand, theFrames) && mBody != nullptr) {
		switch (theCommand) {
		case COMMAND_FREEZE:
			mBody->Slot13(BUG_CMD_FREEZE_ON, theFrames, 0);
			break;
		case COMMAND_DAZE:
			mBody->Slot13(BUG_CMD_STARS_ON, theFrames, 0);
			break;
		case COMMAND_SHOCK:
			mBody->Slot13(BUG_CMD_RING_ON, theFrames, 0);
			break;
		}
	}
	return false;
}

// 0x41e1f0 (slot 19; shared): see BugSharedTakeCommand
int BugBrainBeetle::TakeCommand()
{
	return BugSharedTakeCommand(this);
}

// 0x416750 (slot 20): the bug dies (by the base). A bug of the original: the base is told the bug was not hit by a pea
// whatever thePeaHit says (the ant passes it on). The beetle is flung off: the death track's first key throws it at an angle of 75 to
// 105 degrees, upwards, at its orbit radius plus 8.6 pixels a frame (the orbit radius of the brain is read as it is), its
// orbit angle is cleared, and the three keys' turns start from the way the beetle faces now; the tumble spins it a turn a
// side at random. The turn keys are added on top of the ones the death track was built with
bool BugBrainBeetle::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
{
	(void)thePeaHit;
	if (!BugObj::Zap(theFollowers, theStartTrack, false))
		return false;
	mUnused33C = 0;
	float anAngle = (float)((double)(int)(GameRandomNext() % FLING_ANGLE_RANGE + FLING_ANGLE_FIRST) * PI_F / 180.0);
	SetStateInt(STATE_ORBIT_ANGLE, 0);
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
	mFadeOutKey->AddModifier(STATE_ROTATION, false, mTumbleKey->mValue[STATE_ROTATION], MOD_HOLD, 0.0f, 0.0f, 0.0f, nullptr);
	return true;
}

// 0x416480 (slot 21): placed on a wall piece that runs across it faces right, on one that runs down it faces up; on a post
// it keeps the heading it had. It starts its first step and is shown
void BugBrainBeetle::PlaceAt(int theColumn, int theRow)
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
void BugBrainBeetle::BuildFadeTrack()
{
	BugSharedBuildFadeTrack(this, BEETLE_BRAIN_EVENT_GONE);
}

// 0x416b30 (slot 23): the death track: flung off and falling (its decays start from where the bug is when it is built,
// and Zap sets the fling), a tumble that fades it in over 100 frames (event 6 at its end), and a last frame that fades
// it out and removes it. The tumble's turn is a random side, drawn here as well as in Zap
void BugBrainBeetle::BuildDeathTrack()
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
	mTumbleKey->mEventId = BEETLE_BRAIN_EVENT_TUMBLED;
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

// 0x4168e0: the step's track: three keys that move the beetle's place, its turn (a quarter turn for a corner) and nothing
// else; where they go is set by each step (BeginStep). The keys' own values are all 0, easing toward the next key
void BugBrainBeetle::BuildMoveTrack()
{
	mLegKey = new AnimKey();
	mLegKey->mEventId = BEETLE_BRAIN_EVENT_LEG_DONE;
	mLegKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mLegKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTurnKey = new AnimKey();
	mTurnKey->mEventId = BEETLE_BRAIN_EVENT_TURN_DONE;
	mTurnKey->SetDuration(1);
	mTurnKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mTurnKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mTurnKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	mTurnEndKey = new AnimKey();
	mTurnEndKey->mEventId = BEETLE_BRAIN_EVENT_TURN_END;
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

// The post the heading leads to from the place the beetle is on: the grid's pipe of the place has a link in each heading
PipeObj* BugBrainBeetle::GetNextPost()
{
	// port (crash): the original reads the grid at any place, the 0x7fff of a bug with none included
	if (mColumn < 0 || mColumn >= FILL_GRID_COLUMNS || mRow < 0 || mRow >= FILL_GRID_ROWS)
		return nullptr;
	PipeObj* aPipe = gFillGrid[mColumn][mRow].mPipe;
	if (aPipe == nullptr)
		return nullptr;
	return BugPostLink(aPipe, mHeading);
}

// 0x416500 (EAX the bug): a step: the heading chosen at the last post is taken, the next is chosen at the post it leads to
// (at random when theRandom), and the three keys are set and started: the leg key starts where the beetle is, facing its
// heading; the turn key is at the post, facing the same way, for 88 frames; the last key is at the post facing the
// quarter turn the next heading is from it. When the body would be on the wrong side of the wall for the turn (above the
// line and turning one way, below it and turning the other) the body is told to flip (0x6a). The leg takes one frame
// more than 2.7 (or 3, when the body flips or the leg is 128 pixels or less) a pixel of its length, cut to whole frames,
// and is eased like a cosine only on a long leg that does not flip. Nothing happens when the heading leads nowhere
void BugBrainBeetle::BeginStep(bool theRandom)
{
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
	mTurnKey->SetDuration(TURN_FRAMES);
	bool aFlipped;
	BugImgBeetle* aBody = static_cast<BugImgBeetle*>(mBody);
	if (aBody == nullptr) {
		// The original keeps the low byte of the distance's bits in the flag when there is no body (there always is one)
		unsigned int aBits;
		memcpy(&aBits, &aDistance, sizeof(aBits));
		aFlipped = (aBits & 0xff) != 0;
	} else {
		aFlipped = false;
		int aTurn = TurnDirection(mNextHeading, mHeading);
		bool aAbove = aBody->mState[STATE_Y] < 0.0f;
		if (aAbove ? aTurn > 0 : aTurn < 0) {
			aBody->Slot13(BUG_CMD_BEETLE_FLIP, 0, 0);
			aFlipped = true;
		}
	}
	if (aDistance <= LONG_LEG_PIXELS || aFlipped) {
		mLegKey->SetDuration(1 - (int)((double)aDistance * -SHORT_FRAMES_PER_PIXEL));
		mLegKey->mEasing = EASE_LINEAR_CLAMPED;
	} else {
		mLegKey->SetDuration(1 - (int)((double)aDistance * -LONG_FRAMES_PER_PIXEL));
		mLegKey->mEasing = EASE_COSINE;
	}
	StartTrack(mMoveTrack);
}
