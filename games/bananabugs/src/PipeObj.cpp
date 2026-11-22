#include "PipeObj.h"

#include "Anim.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

bool PipeObj::sSoundPlayed = false;
Anim* PipeObj::sPipesAnim = nullptr;

// A whole number as the x87 maths takes it, a double
static double ToDouble(int theValue)
{
	return (double)theValue;
}

// port (crash): the cell the level's picture gives when it has none (the picture was not loaded or was too big), where the
// original reads through a null pointer
static MorphCell sNoMorphCell = {};

// A key's place and colour, as the pipe's keys take them from a cell of a picture: the offset to the cell and its colour
static void SetKeyPlace(AnimKey* theKey, float theX, float theY, const MorphCell* theCell)
{
	theKey->mValue[STATE_X] = theX;
	theKey->mValue[STATE_Y] = theY;
	theKey->mValue[STATE_RED] = theCell->mRed;
	theKey->mValue[STATE_GREEN] = theCell->mGreen;
	theKey->mValue[STATE_BLUE] = theCell->mBlue;
}

// A place four pixels or less each way from theCenter (a whole number from -4 to 4 added to it)
static float Jitter(float theCenter)
{
	return (float)(ToDouble((int)(GameRandomNext() % 9) - 4) + (double)theCenter);
}

// A random angle in radians, from a whole number of degrees: 0 to 360 less theBias degrees (the original's range)
static float RandomAngle(unsigned int theRange, int theBias)
{
	return (float)(ToDouble((int)(GameRandomNext() % theRange) - theBias) * PI_F / 180.0);
}

// 0x455f80: a pipe with the appear track made, hidden by nothing yet (Create sets it up)
PipeObj::PipeObj()
{
	SetStateInt(STATE_Z, 1);
	mShiftSet = 0.0f;
	mUpdateLevel = 2;
	mUnused2A0 = 0;
	mUnused2A4 = 0;
	mUnused2A8 = 0;
	mUnused2AC = 0;
	mKind = PIPE_KIND_UNSET;
	mSettledA = false;
	mSettledB = false;
	// the original writes the name "Pipe" over the object's spare words (+0x1f0), which nothing reads

	// port: the original leaves these as they come from the allocator until the level's setup and links write them
	mColumn = 0;
	mRow = 0;
	mVariant = 0;
	mUpLink = nullptr;
	mRightLink = nullptr;
	mDownLink = nullptr;
	mLeftLink = nullptr;
	mShiftX = 0.0f;
	mShiftY = 0.0f;
	mFirstOffsetX = 0.0f;
	mFirstOffsetY = 0.0f;
	mSecondOffsetX = 0.0f;
	mSecondOffsetY = 0.0f;
	mFirstCell = nullptr;
	mSecondCell = nullptr;

	BuildAppearTrack();
}

// 0x4560a0: the flags and the nudge are cleared (the base class's Init is not called)
void PipeObj::Init()
{
	mShiftSet = 0.0f;
	mSettledA = false;
	mSettledB = false;
}

// 0x4560c0: the nudge a cut gives the pipe goes to this place (from the pipe's own), and is set
void PipeObj::SetShiftTarget(float theX, float theY)
{
	mShiftX = theX;
	mShiftY = theY;
	mShiftSet = 1.0f;
}

// 0x4560e0: the nudge starts, but only on a pipe at its full size (the original compares the float for equal)
void PipeObj::StartShift()
{
	if (mState[STATE_SCALE_X] == 1.0f) {
		mNudgeKey->mValue[STATE_X] = mShiftX;
		mNudgeKey->mValue[STATE_Y] = mShiftY;
		StartTrack(mNudgeTrack);
	}
}

// 0x456120: the cell of each of the level's two pictures that the pipe takes, and the way to it. A pipe with a wall's or a
// post's look takes middle tones, an empty one the dark and the bright
void PipeObj::PickMorphCells()
{
	mFirstCell = LevelObj::PickMorphCell(mVariant != PIPE_VARIANT_EMPTY, 0);
	if (mFirstCell == nullptr)
		mFirstCell = &sNoMorphCell;
	mFirstOffsetX = (float)(((double)mFirstCell->mX - (double)mColumn) * (double)FILL_CELL_SPACING);
	mFirstOffsetY = (float)(((double)mFirstCell->mY - (double)mRow) * (double)FILL_CELL_SPACING);
	mSecondCell = LevelObj::PickMorphCell(mVariant != PIPE_VARIANT_EMPTY, 1);
	if (mSecondCell == nullptr)
		mSecondCell = &sNoMorphCell;
	mSecondOffsetX = (float)(((double)mSecondCell->mX - (double)mColumn) * (double)FILL_CELL_SPACING);
	mSecondOffsetY = (float)(((double)mSecondCell->mY - (double)mRow) * (double)FILL_CELL_SPACING);
}

// 0x4561b0: the drift in starts
void PipeObj::StartDriftTrack()
{
	StartTrack(mDriftTrack);
}

// 0x4561c0: a new look is chosen (the level's, which makes its draws), the spin key for it goes to the second picture's
// cell and its track starts, and the pipe is shown. A wall or a post finds its links
void PipeObj::Refresh()
{
	mUnused2A0 = 0;
	mUnused2A4 = 0;
	mUnused2A8 = 0;
	mUnused2AC = 0;
	LevelObj::ChoosePipeLook(this);
	if (mVariant == PIPE_VARIANT_EMPTY) {
		SetKeyPlace(mSpinEmptyKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
		StartTrack(mSpinEmptyTrack);
	} else {
		SetKeyPlace(mSpinKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
		StartTrack(mSpinTrack);
	}
	if (mVariant != PIPE_VARIANT_EMPTY)
		LinkByKinds();
	Activate(this, false);
}

// 0x4562d0: the pipe flies off
void PipeObj::Retire()
{
	StartTrack(mFlyOffTrack);
}

// 0x4562f0: the pipe's own keys end. The first (event 1) is the pipe's arrival at the cell of the first picture: it takes
// a look (the level's draws), finds its links when it is a wall or a post, and flies back from the cell to its place at its
// full size, on the return track that goes with the look. Events 2 and 3 set the flags the level waits for; event 4 plays
// the sound of leaves, once for all the pipes until the level clears the flag
void PipeObj::Notify(ObjBase* theObj)
{
	(void)theObj; // the original reads its own current key
	if (mCurrentKey == nullptr)
		return;
	switch (mCurrentKey->mEventId) {
	case PIPE_EVENT_ARRIVED:
		mSettledA = false;
		LevelObj::ChoosePipeLook(this);
		if (mVariant != PIPE_VARIANT_EMPTY)
			LinkByKinds();
		if (mVariant == PIPE_VARIANT_EMPTY) {
			SetKeyPlace(mReturnEmptyKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
			SetStateInt(STATE_SCALE_X, 1);
			SetStateInt(STATE_SCALE_Y, 1);
			SetStateInt(STATE_ALPHA, 1);
			ComputeDrawn();
			StartTrack(mReturnEmptyTrack);
			return;
		}
		SetKeyPlace(mReturnKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
		SetStateInt(STATE_SCALE_X, 1);
		SetStateInt(STATE_SCALE_Y, 1);
		SetStateInt(STATE_ALPHA, 1);
		ComputeDrawn();
		StartTrack(mReturnTrack);
		break;
	case PIPE_EVENT_FLAG_A:
		mSettledA = true;
		return;
	case PIPE_EVENT_FLAG_B:
		mSettledB = true;
		return;
	case PIPE_EVENT_LEAVES:
		if (!sSoundPlayed) {
			ObjBase::sApp->mSoundQueue->Request(SOUND_LEAVES, 0, false);
			sSoundPlayed = true;
			return;
		}
	}
}

// 0x4564c0: the pipe flies back from the second picture's cell to its place, at full size, on the return track of its look
void PipeObj::Rebuild()
{
	if (mVariant == PIPE_VARIANT_EMPTY) {
		SetKeyPlace(mReturnEmptyKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
		SetStateInt(STATE_SCALE_X, 1);
		SetStateInt(STATE_SCALE_Y, 1);
		SetStateInt(STATE_ALPHA, 1);
		ComputeDrawn();
		StartTrack(mReturnEmptyTrack);
		return;
	}
	SetKeyPlace(mReturnKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	ComputeDrawn();
	StartTrack(mReturnTrack);
}

// 0x4565f0: how long the drift waits at the bottom of the screen, 300 to 332 frames
void PipeObj::RandomizeDuration()
{
	mDriftKey->SetDuration((int)(GameRandomNext() % 0x21) + 300);
}

// 0x43c220 (it is in LevelObj's range, with EBX the pipe): a pipe that is part of a wall finds the nearest post on each
// side from the grid's kinds, looking along the places that have ground (a kind of 10 or more is the kind less 10; 0
// or less ends the look). The pipes' own LinkPipes does the same by the pipes and when a cut makes the walls
void PipeObj::LinkByKinds()
{
	int aRow = mRow;
	int aColumn = mColumn;
	mUpLink = nullptr;
	mRightLink = nullptr;
	mDownLink = nullptr;
	mLeftLink = nullptr;
	if (mKind == 0)
		return;
	int aNear = aRow - 1;
	if (aNear >= 0) {
		do {
			int aKind = gFillGrid[aColumn][aNear].mKind;
			if (aKind > 9)
				aKind -= 10;
			if (aKind < 1)
				break;
			if (aKind == 1) {
				mUpLink = gFillGrid[aColumn][aNear].mPipe;
				break;
			}
			aNear--;
		} while (aNear >= 0);
	}
	aNear = aColumn + 1;
	if (aNear < FILL_GRID_COLUMNS) {
		do {
			int aKind = gFillGrid[aNear][aRow].mKind;
			if (aKind > 9)
				aKind -= 10;
			if (aKind < 1)
				break;
			if (aKind == 1) {
				mRightLink = gFillGrid[aNear][aRow].mPipe;
				break;
			}
			aNear++;
		} while (aNear < FILL_GRID_COLUMNS);
	}
	aNear = aRow + 1;
	if (aNear < FILL_GRID_ROWS) {
		do {
			int aKind = gFillGrid[aColumn][aNear].mKind;
			// the original ends the look on a raw kind of 10 here too, which the next test would do anyway
			if (aKind < 1 || aKind == 10)
				break;
			if (aKind > 9)
				aKind -= 10;
			if (aKind < 1)
				break;
			if (aKind == 1) {
				mDownLink = gFillGrid[aColumn][aNear].mPipe;
				break;
			}
			aNear++;
		} while (aNear < FILL_GRID_ROWS);
	}
	aNear = aColumn - 1;
	if (aNear >= 0) {
		while (true) {
			int aKind = gFillGrid[aNear][aRow].mKind;
			if (aKind > 9)
				aKind -= 10;
			if (aKind < 1)
				return;
			if (aKind == 1)
				break;
			aNear--;
			if (aNear < 0)
				return;
		}
		mLeftLink = gFillGrid[aNear][aRow].mPipe;
	}
}

// 0x458dc0: the track a cut starts a pipe on: it grows in from 3.6 times its size, turned up to 45 degrees either way and
// clear, with the sound of a pipe as it starts, over 15 frames, and settles over 100 more. The level sets its values for
// each cut (LevelObj's StartWallPipes)
void PipeObj::BuildAppearTrack()
{
	mAppearKey = new AnimKey();
	mAppearKey->SetDuration(0xf);
	mAppearKey->AddValue(STATE_SCALE_X, 3.6f, MOD_TOWARD_NEXT);
	mAppearKey->AddValue(STATE_SCALE_Y, 3.6f, MOD_TOWARD_NEXT);
	mAppearKey->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	mAppearKey->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	mAppearKey->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	mAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	mAppearKey->AddValue(STATE_ROTATION, RandomAngle(0x5b, 0x2d), MOD_TOWARD_NEXT);
	mAppearKey->mEndCondition = KEY_SOUND;
	mAppearKey->mEndChannel = SOUND_PIPE;
	mAppearKey->mEndValue = 2.0f;

	mSettleKey = new AnimKey();
	mSettleKey->SetDuration(100);
	mSettleKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	mSettleKey->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	mSettleKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	mSettleKey->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	mSettleKey->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	mSettleKey->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	mSettleKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mAppearTrack = new AnimTrack();
	mAppearTrack->AddKey(mAppearKey, nullptr);
	mAppearTrack->AddKey(mSettleKey, mSettleKey);
	mAppearTrackIndex = AddTrack(mAppearTrack, nullptr);
}

// 0x456620: the drift in: the pipe waits at the bottom of the screen (1150 down, up to 64 across, dark green and turned
// a random way) for 800 to 840 frames (RandomizeDuration shortens it), flies to its place in 100 frames with a wobble
// (event 4, the sound of leaves), flies to its cell of the first picture in 60 (taking its colour) and arrives with
// event 1, a few pixels off
void PipeObj::BuildDriftTrack()
{
	AnimKey* aWait = new AnimKey();
	aWait->SetDuration((int)(GameRandomNext() % 0x29) + 0x320);
	aWait->AddValue(STATE_X, (float)((int)(GameRandomNext() % 0x81) - 0x40), MOD_TOWARD_NEXT);
	aWait->AddValue(STATE_Y, (float)((1150.0 - (double)mBase[STATE_Y]) + (double)(GameRandomNext() % 0x11)), MOD_TOWARD_NEXT);
	aWait->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aWait->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aWait->AddValue(STATE_RED, 0.05f, MOD_TOWARD_NEXT);
	aWait->AddValue(STATE_GREEN, 0.1f, MOD_TOWARD_NEXT);
	aWait->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aWait->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aWait->AddValue(STATE_ROTATION, RandomAngle(0x169, 0xb4), MOD_TOWARD_NEXT);
	mDriftKey = aWait;

	AnimKey* aFlyIn = new AnimKey();
	aFlyIn->mEventId = PIPE_EVENT_LEAVES;
	aFlyIn->SetDuration(100);
	aFlyIn->AddValue(STATE_X, aWait->mValue[STATE_X], MOD_TOWARD_NEXT);
	aFlyIn->AddModifier(STATE_X, false, 0.0f, MOD_SINE, 4.0f, 2.0f, 0.0f, nullptr);
	aFlyIn->AddValue(STATE_Y, aWait->mValue[STATE_Y], MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_RED, 0.1f, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_GREEN, 0.12f, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_ROTATION, aWait->mValue[STATE_ROTATION], MOD_TOWARD_NEXT);

	AnimKey* aGather = new AnimKey();
	aGather->SetDuration(0x3c);
	aGather->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aGather->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGather->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aGather->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aGather->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aGather->AddValue(STATE_ROTATION, aWait->mValue[STATE_ROTATION], MOD_TOWARD_NEXT);
	aGather->mEasing = EASE_IN_SQUARE;
	mGatherKey = aGather;

	AnimKey* aArrived = new AnimKey();
	aArrived->mEventId = PIPE_EVENT_ARRIVED;
	aArrived->SetDuration(1);
	aArrived->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aArrived->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aArrived->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aArrived->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aArrived->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aArrived->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aArrived->mEasing = EASE_IN_SQUARE;
	mGatherEndKey = aArrived;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aWait, nullptr);
	aTrack->AddKey(aFlyIn, nullptr);
	aTrack->AddKey(aGather, nullptr);
	aTrack->AddKey(aArrived, nullptr);
	mDriftTrack = AddTrack(aTrack, nullptr);
}

// 0x456cc0: the return of a wall or a post: from the cell it is at (the key's place, set before it starts) to its place in
// 60 frames, then colours itself in one frame with event 2, and holds the rest key
void PipeObj::BuildReturnTrack()
{
	AnimKey* aLeave = new AnimKey();
	aLeave->SetDuration(0x3c);
	aLeave->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aLeave->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aLeave->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLeave->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aLeave->mEasing = EASE_IN_SQUARE;
	mReturnKey = aLeave;

	AnimKey* aLanded = new AnimKey();
	aLanded->SetDuration(1);
	aLanded->mEventId = PIPE_EVENT_FLAG_A;
	aLanded->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLanded->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aLanded->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aLanded->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aLanded->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aLanded->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLanded->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aLeave, nullptr);
	aTrack->AddKey(aLanded, nullptr);
	aTrack->AddKey(mRestKey, mRestKey);
	mReturnTrack = AddTrack(aTrack, nullptr);
}

// 0x457020: the return of an empty place: 50 frames from the cell to the place, and one frame of event 2 that turns it
// over (a random angle, scale -1) and holds
void PipeObj::BuildReturnEmptyTrack()
{
	AnimKey* aLeave = new AnimKey();
	aLeave->SetDuration(0x32);
	aLeave->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aLeave->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	mReturnEmptyKey = aLeave;

	AnimKey* aLanded = new AnimKey();
	aLanded->SetDuration(1);
	aLanded->mEventId = PIPE_EVENT_FLAG_A;
	aLanded->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aLanded->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aLanded->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aLanded->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	aLanded->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aLanded->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aLanded->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aLanded->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aLanded->AddValue(STATE_ROTATION, RandomAngle(0x169, 0xb4), MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aLeave, nullptr);
	aTrack->AddKey(aLanded, nullptr);
	mReturnEmptyTrack = AddTrack(aTrack, nullptr);
}

// 0x4573f0: the spin of a wall or a post, started by Refresh: 100 to 125 frames from the second picture's cell to its place,
// coloured in, then 360 frames (event 3, the second settled flag) of turning a random angle on, and holds
void PipeObj::BuildSpinTrack()
{
	AnimKey* aCome = new AnimKey();
	aCome->SetDuration((int)(GameRandomNext() % 0x1a) + 100);
	aCome->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aCome->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aCome->mEasing = EASE_IN_SQUARE;

	AnimKey* aSpin = new AnimKey();
	aSpin->mEventId = PIPE_EVENT_FLAG_B;
	aSpin->SetDuration(0x168);
	aSpin->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSpin->AddModifier(STATE_ROTATION, false, RandomAngle(0x169, 0xb4), MOD_WRAP_DEGREES, 1.0f, 1.0f, 0.0f, nullptr);
	mSpinKey = aSpin;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aCome, nullptr);
	aTrack->AddKey(aSpin, aSpin);
	mSpinTrack = AddTrack(aTrack, nullptr);
}

// 0x457740: a second spin of a wall or a post (50 to 75 frames in, 50 turning to a random angle, 20 more ending with
// event 1). Nothing in the class starts it; the original builds it twice, the second over the first's fields (the first
// track stays on the object, never started)
void PipeObj::BuildSecondSpinTrack()
{
	AnimKey* aCome = new AnimKey();
	aCome->SetDuration((int)(GameRandomNext() % 0x1a) + 0x32);
	aCome->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aCome->mEasing = EASE_IN_SQUARE;

	AnimKey* aTurn = new AnimKey();
	aTurn->SetDuration(0x32);
	aTurn->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_ROTATION, RandomAngle(0x169, 0xb4), MOD_TOWARD_NEXT);
	mSecondSpinMidKey = aTurn;

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = PIPE_EVENT_ARRIVED;
	aEnd->SetDuration(0x14);
	aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_ROTATION, aTurn->mValue[STATE_ROTATION], MOD_TOWARD_NEXT);
	mSecondSpinEndKey = aEnd;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aCome, nullptr);
	aTrack->AddKey(aTurn, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mSecondSpinTrack = AddTrack(aTrack, nullptr);
}

// 0x457bb0: the spin of an empty place, started by Refresh: like the wall's, but it comes in turned over (scale and alpha
// -1)
void PipeObj::BuildSpinEmptyTrack()
{
	AnimKey* aCome = new AnimKey();
	aCome->SetDuration((int)(GameRandomNext() % 0x1a) + 100);
	aCome->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aCome->mEasing = EASE_IN_SQUARE;

	AnimKey* aSpin = new AnimKey();
	aSpin->mEventId = PIPE_EVENT_FLAG_B;
	aSpin->SetDuration(0x168);
	aSpin->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aSpin->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSpin->AddModifier(STATE_ROTATION, false, RandomAngle(0x169, 0xb4), MOD_WRAP_DEGREES, 1.0f, 1.0f, 0.0f, nullptr);
	mSpinEmptyKey = aSpin;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aCome, nullptr);
	aTrack->AddKey(aSpin, aSpin);
	mSpinEmptyTrack = AddTrack(aTrack, nullptr);
}

// 0x457fa0: the second spin of an empty place (the pair of BuildSecondSpinTrack's, turned over); nothing starts it
void PipeObj::BuildSecondSpinEmptyTrack()
{
	AnimKey* aCome = new AnimKey();
	aCome->SetDuration((int)(GameRandomNext() % 0x1a) + 0x32);
	aCome->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aCome->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aCome->mEasing = EASE_IN_SQUARE;

	AnimKey* aTurn = new AnimKey();
	aTurn->SetDuration(0x32);
	aTurn->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aTurn->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aTurn->AddValue(STATE_ROTATION, RandomAngle(0x169, 0xb4), MOD_TOWARD_NEXT);
	mSecondSpinEmptyMidKey = aTurn;

	AnimKey* aEnd = new AnimKey();
	aEnd->mEventId = PIPE_EVENT_ARRIVED;
	aEnd->SetDuration(0x14);
	aEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aEnd->AddValue(STATE_ROTATION, aTurn->mValue[STATE_ROTATION], MOD_TOWARD_NEXT);
	mSecondSpinEmptyEndKey = aEnd;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aCome, nullptr);
	aTrack->AddKey(aTurn, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mSecondSpinEmptyTrack = AddTrack(aTrack, nullptr);
}

// 0x458570: the fly-off (Retire): the pipe is thrown at a random angle at a speed of 4.2 to 7.8 with a pull of 0.28 down,
// slowing by 1% a frame, for 1900 frames or until it passes 1040 to 1056 down; it fades to black while it spins (the
// second key, event 4: the sound of leaves) and clears
void PipeObj::BuildFlyOffTrack()
{
	unsigned int aSpeedRandom = GameRandomNext();
	// the speed is held as a float before the 3 is added, as the original does
	float aSpeedFraction = (float)((double)(int)(aSpeedRandom % 0x17d + 0x78) / 100.0);
	unsigned int aAngleRandom = GameRandomNext();
	float anAngle = (float)((double)(int)(aAngleRandom % 0x169) * PI_F / 180.0);

	AnimKey* aThrow = new AnimKey();
	aThrow->SetDuration(0x76c);
	float aSpeed = (float)((double)aSpeedFraction + 3.0);
	aThrow->AddDecay(STATE_X, 0.0f, aSpeed, FloatCos(anAngle), 0.0f, 0.99f);
	aThrow->AddDecay(STATE_Y, 0.0f, aSpeed, FloatSin(anAngle), -0.28f, 0.99f);
	aThrow->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aThrow->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aThrow->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aThrow->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aThrow->mEndCondition = KEY_END_ABOVE;
	aThrow->mEndChannel = STATE_Y;
	aThrow->mEndValue = (float)(int)(GameRandomNext() % 0x11 + 0x410);
	mFlyOffKey = aThrow;

	AnimKey* aFade = new AnimKey();
	aFade->mEventId = PIPE_EVENT_LEAVES;
	aFade->SetDuration(0x32);
	aFade->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_ROTATION, (float)(ToDouble((int)(GameRandomNext() % 0x939) - 0x49c) * PI_F / 180.0),
		MOD_TOWARD_NEXT);
	mFlyOffTurnKey = aFade;

	AnimKey* aClear = new AnimKey();
	aClear->SetDuration(100);
	aClear->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aClear->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aClear->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	aClear->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aClear->AddValue(STATE_ROTATION, aFade->mValue[STATE_ROTATION], MOD_TOWARD_NEXT);
	mFlyOffFadeKey = aClear;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aThrow, nullptr);
	aTrack->AddKey(aFade, nullptr);
	aTrack->AddKey(aClear, nullptr);
	mFlyOffTrack = AddTrack(aTrack, nullptr);
}

// 0x458b80: the nudge of a cut (StartShift): 6 frames still, 6 frames to the place the cut sets (turned up to 2 degrees
// either way), 6 frames back, and the rest key
void PipeObj::BuildNudgeTrack()
{
	AnimKey* aStill = new AnimKey();
	aStill->SetDuration(6);
	aStill->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aStill->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aStill->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

	AnimKey* aPush = new AnimKey();
	aPush->SetDuration(6);
	aPush->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aPush->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aPush->AddValue(STATE_ROTATION, RandomAngle(5, 2), MOD_TOWARD_NEXT);
	mNudgeKey = aPush;

	AnimKey* aBack = new AnimKey();
	aBack->SetDuration(6);
	aBack->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aBack->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aBack->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aStill, nullptr);
	aTrack->AddKey(aPush, nullptr);
	aTrack->AddKey(aBack, nullptr);
	aTrack->AddKey(mRestKey, mRestKey);
	mNudgeTrack = AddTrack(aTrack, nullptr);
}

// 0x458aa0: the pipe's tracks, in the order that numbers them. The second spin's builder is called twice (once where
// the empty one's was probably meant), the second over the first's fields; the random draws of both are made
void PipeObj::BuildTracks()
{
	BuildDriftTrack();
	BuildReturnTrack();
	BuildReturnEmptyTrack();
	BuildSpinTrack();
	BuildSecondSpinTrack();
	BuildSpinEmptyTrack();
	BuildSecondSpinEmptyTrack();
	BuildSecondSpinTrack();
	BuildFlyOffTrack();
}

// 0x456bf0: the drift track's last two keys go to the first picture's cell, taking its colour; the last is a few pixels off
// in height
void PipeObj::PlaceGatherKeys()
{
	SetKeyPlace(mGatherKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
	mGatherEndKey->mValue[STATE_X] = mFirstOffsetX;
	mGatherEndKey->mValue[STATE_Y] = Jitter(mFirstOffsetY);
	mGatherEndKey->mValue[STATE_RED] = mFirstCell->mRed;
	mGatherEndKey->mValue[STATE_GREEN] = mFirstCell->mGreen;
	mGatherEndKey->mValue[STATE_BLUE] = mFirstCell->mBlue;
}

// 0x456fc0
void PipeObj::PlaceReturnKey()
{
	SetKeyPlace(mReturnKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
}

// 0x457390
void PipeObj::PlaceReturnEmptyKey()
{
	SetKeyPlace(mReturnEmptyKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
}

// 0x4576e0: the spin of a wall or a post turns about the second picture's cell
void PipeObj::PlaceSpinKey()
{
	SetKeyPlace(mSpinKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
}

// 0x457b00: the second spin's keys go to the first picture's cell (the last a few pixels off each way, with no colour)
void PipeObj::PlaceSecondSpinKeys()
{
	SetKeyPlace(mSecondSpinMidKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
	mSecondSpinEndKey->mValue[STATE_X] = Jitter(mFirstOffsetX);
	mSecondSpinEndKey->mValue[STATE_Y] = Jitter(mFirstOffsetY);
}

// 0x457f40
void PipeObj::PlaceSpinEmptyKey()
{
	SetKeyPlace(mSpinEmptyKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
}

// 0x458480: the same for the empty place's second spin, the last key with the colour
void PipeObj::PlaceSecondSpinEmptyKeys()
{
	SetKeyPlace(mSecondSpinEmptyMidKey, mFirstOffsetX, mFirstOffsetY, mFirstCell);
	mSecondSpinEmptyEndKey->mValue[STATE_X] = Jitter(mFirstOffsetX);
	mSecondSpinEmptyEndKey->mValue[STATE_Y] = Jitter(mFirstOffsetY);
	mSecondSpinEmptyEndKey->mValue[STATE_RED] = mFirstCell->mRed;
	mSecondSpinEmptyEndKey->mValue[STATE_GREEN] = mFirstCell->mGreen;
	mSecondSpinEmptyEndKey->mValue[STATE_BLUE] = mFirstCell->mBlue;
}

// 0x4589a0: the fly-off's three keys are at the second picture's cell, in its colour
void PipeObj::PlaceFlyOffKeys()
{
	SetKeyPlace(mFlyOffKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
	SetKeyPlace(mFlyOffTurnKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
	SetKeyPlace(mFlyOffFadeKey, mSecondOffsetX, mSecondOffsetY, mSecondCell);
}

// 0x458b00: the pipe is set up for the level (after the level's look for it): no wall, no flags, a cell picked of each
// picture, every key put at its cell, and the drift in started
void PipeObj::SetupMorph()
{
	mShiftSet = 0.0f;
	mUnused2A0 = 0;
	mUnused2A4 = 0;
	mUnused2A8 = 0;
	mUnused2AC = 0;
	mKind = PIPE_KIND_UNSET;
	mSettledA = false;
	mSettledB = false;
	PickMorphCells();
	PlaceGatherKeys();
	PlaceReturnKey();
	PlaceReturnEmptyKey();
	PlaceSpinKey();
	PlaceSecondSpinKeys();
	PlaceSpinEmptyKey();
	PlaceSecondSpinEmptyKeys();
	PlaceFlyOffKeys();
	StartDriftTrack();
}

// 0x459120: a pipe for the place: its picture (the pipes' cels), its place and depth (the columns times the rows, so the
// later places draw over the earlier), black and clear, the rest key, and every track
PipeObj* PipeObj::Create(int theColumn, int theRow)
{
	PipeObj* aPipe = new PipeObj();
	aPipe->SetParent(nullptr, nullptr);
	sPipesAnim->AddRef();
	aPipe->mAnim = sPipesAnim;
	aPipe->mColumn = theColumn;
	aPipe->mRow = theRow;
	aPipe->SetStateInt(STATE_X, theColumn * FILL_CELL_SPACING);
	aPipe->SetStateInt(STATE_Y, theRow * FILL_CELL_SPACING);
	aPipe->SetState(STATE_Z, (float)((double)(theColumn * theRow) / 2000.0));
	aPipe->SetStateInt(STATE_RED, 0);
	aPipe->SetStateInt(STATE_GREEN, 0);
	aPipe->SetStateInt(STATE_BLUE, 0);
	aPipe->SetStateInt(STATE_ALPHA, 1);
	aPipe->mRestKey = new AnimKey();
	aPipe->mRestKey->SetDuration(10);
	aPipe->mRestKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aPipe->mRestKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aPipe->mRestKey->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aPipe->mRestKey->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aPipe->mRestKey->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aPipe->mRestKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	aPipe->Deactivate(aPipe);
	aPipe->BuildTracks();
	aPipe->BuildNudgeTrack();
	return aPipe;
}

// 0x459360
void Sexy::PreloadPipes()
{
	PipeObj::sPipesAnim = Anim::Get("images/pipes01", ANIM_SCALE_SMOOTH, false, 0x21, 0x21, 0x10, false);
}
