#include "FillObj.h"

#include "Anim.h"
#include "Emitter.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstdio>

using namespace Sexy;

FillGridCell Sexy::gFillGrid[FILL_GRID_COLUMNS][FILL_GRID_ROWS];

int FillObj::sFillWave = FILL_WAVE_FIRST;
float FillObj::sNextDepth = 1.0f;
int FillObj::sCellsShown = 0;
int FillObj::sCellsSettled = 0;
int FillObj::sCellsCut = 0;
int FillObj::sCellsArrived = 0;
int FillObj::sFarthestDistance = 0;
FillObj* FillObj::sFarthestCell = nullptr;
int FillObj::sPendingWave = 0;
int FillObj::sBusy = 0;
Anim* FillObj::sFillSheet = nullptr;
Anim* FillObj::sNetFlyAnim = nullptr;

// How many frames the level's picture waits per cell of distance from where the cut began
static const double FRAMES_PER_CELL = 5.0;
// The part of a design-space pixel a 31-pixel cel of the picture is scaled by
static const float FILL_PICTURE_SCALE = 0.031f;
// The original's 2.7 is a float widened to a double
static const double NET_FLY_SHRINK = 2.700000047683716;
// The depth step between cells (0.0005 as a float widened to a double)
static const double DEPTH_STEP = 0.0005000000237487257;

// A pipe that may be missing as the port's stand-in grid has none; the original reads the grid unchecked
static bool IsActivePipe(PipeObj* thePipe)
{
	return thePipe != nullptr && thePipe->IsActive();
}

// A whole number as the x87 maths takes it, a double
static double ToDouble(int theValue)
{
	return (double)theValue;
}

// 0x428230: a cell no one knows yet: hidden, uncut, with nothing queued. Create builds the rest
FillObj::FillObj()
{
	SetStateInt(STATE_Z, 1);
	// the original writes the name "FillCell" over the object's spare words (+0x1f0), which nothing reads
	mColumn = 0;
	mRow = 0;
	mUnused284 = 0;
	mUnused288 = 0;
	mSide = 0;
	mCut = false;
	mCutShown = false;
	mMark = 0;
	mHighlighted = false;
	mFillDone = false;
	mShiftPending = 0;
	mShiftCount = 0;
	mLastCut = false;
	mBonus = nullptr;
	mWave = 0;
	for (int i = 0; i < FILL_SHIFT_MAX; i++)
		mShiftPipes[i] = nullptr;
	mNetFly = nullptr;
	mFillPicture = nullptr;
	mWaveKey = nullptr;
	mNetFlyKey = nullptr;
	mNetFlyKeyAfter = nullptr;
	mHighlightTrack = nullptr;
	mType = OBJ_TYPE_FILL_CELL;
	mUpdateLevel = 2;
}

// 0x4282d0 and 0x4282f0
FillObj::~FillObj()
{
}

// 0x428300: the cell and its picture stop and start their first tracks again, and the cell is whole and uncut
void FillObj::Init()
{
	Deactivate(this);
	Deactivate(mFillPicture);
	StartTrack(FILL_CELL_TRACK_APPEAR);
	mFillPicture->StartTrack(0);
	mCut = false;
	mCutShown = false;
	mShiftCount = 0;
	mLastCut = false;
	mFillDone = false;
	mHighlighted = false;
}

// 0x428360: the events of the cell's own keys, of its picture's and of the net fly's. A key whose event is the cut's
// wave number counts the cell as reached by the wave
void FillObj::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == sFillWave) {
		sCellsArrived = sCellsArrived + 1;
		return;
	}
	switch (anEvent) {
	case FILL_EVENT_LANDED:
		if (mCut) {
			if (!mCutShown) {
				if (sLevel != nullptr)
					LevelObj::OnCellCut();
				SetHidden(true);
				mCutShown = true;
				if (mBonus != nullptr && mBonus->mButtonId == 0) {
					if (mBonus->mEmitters != nullptr)
						mBonus->mEmitters->BurstStarRing(0.0f, 0.0f);
					mBonus->mButtonId = 99;
					mBonus->Uncover();
				}
			}
			if (mLastCut && sBusy > 0)
				sBusy = sBusy - 1;
		}
		break;
	case FILL_EVENT_RESET_SHOWN:
		mCutShown = false;
		break;
	case FILL_EVENT_SETTLED:
		sCellsSettled = sCellsSettled + 1;
		break;
	case FILL_EVENT_SHIFT_PIPES:
		mShiftPending = 0;
		QueueCornerPipes();
		if (mShiftPending != 0)
			ReleaseQueuedPipes();
		if (mLastCut) {
			sPendingWave = mWave;
			LevelObj::OnFillDone();
		}
		break;
	case FILL_EVENT_FILL_END:
		mFillDone = true;
		break;
	case FILL_EVENT_GLOW_REPEAT:
		if (!mCut && mHighlighted) {
			if (!mNetFly->IsActive()) {
				mNetFlyKey->SetDuration((int)(GameRandomNext() % 21 + 0x28));
				mNetFly->StartTrack(0);
				Activate(mNetFly, false);
				mNetFly->mCurrentTrack->SetDelay((int)(GameRandomNext() % 21) * 4, false);
				mNetFly->SetStateInt(STATE_RED, 1);
				mNetFly->SetStateInt(STATE_GREEN, 1);
				mNetFly->SetStateInt(STATE_BLUE, 1);
				mNetFlyKey->mValue[STATE_SCALE_X] = (float)(ToDouble((int)(GameRandomNext() % 26 + 25)) / 100.0);
				mNetFlyKey->mValue[STATE_SCALE_Y] = mNetFlyKey->mValue[STATE_SCALE_X];
				mNetFlyKey->mValue[STATE_ORBIT_ANGLE] =
					(float)(ToDouble((int)(GameRandomNext() % 361) - 180) * PI_F / 180.0);
				mNetFlyKey->mValue[STATE_ORBIT_RADIUS] = (float)(ToDouble((int)(GameRandomNext() % 33 + 8)) / 100.0);
			} else {
				mNetFly->SetStateInt(STATE_RED, 1);
				mNetFly->SetStateInt(STATE_GREEN, 1);
				mNetFly->SetStateInt(STATE_BLUE, 1);
				mNetFlyKey->mValue[STATE_ORBIT_RADIUS] = (float)(ToDouble((int)(GameRandomNext() % 33 + 8)) / 100.0);
			}
		}
		break;
	case FILL_EVENT_GLOW_FADE:
		if (!mHighlighted && mNetFly->IsActive()) {
			float aDuration = mNetFlyKey->mDuration;
			float aTime = mNetFly->mKeyTime;
			if ((double)aDuration - (double)aTime > 7.0) {
				// the flash is cut short: its time and length shrink, the orbit widens and the colour reddens
				mNetFly->mKeyTime = (float)((double)aTime / NET_FLY_SHRINK + 3.0);
				mNetFlyKey->SetDuration((int)((double)aDuration / NET_FLY_SHRINK + 3.0));
				mNetFlyKey->mValue[STATE_ORBIT_RADIUS] = (float)((double)mNetFlyKey->mValue[STATE_ORBIT_RADIUS] * 4.0);
				mNetFly->SetState(STATE_RED, 0.8f);
				mNetFly->SetState(STATE_GREEN, 0.7f);
				mNetFly->SetState(STATE_BLUE, 0.5f);
			}
		}
		break;
	default:
		break;
	}
}

// 0x4287a0: a pipe at one of the cell's corners is to move by this much once the wave has reached the cell
void FillObj::QueuePipe(PipeObj* thePipe, float theX, float theY)
{
	if (mShiftCount < FILL_SHIFT_MAX) {
		mShiftPending = 1;
		mShiftPipes[mShiftCount] = thePipe;
		mShiftCount = mShiftCount + 1;
		thePipe->SetShiftTarget(theX, theY);
	}
}

// 0x4287e0: the pipes at the cell's four corners (the grid's cell, the next column, the next row and both) are queued
// to move by 8 pixels. The pipe at the cell's own place moves up and left, by what its kind says (1: both ways,
// 2: up, 3: left); the other three move 8 pixels to the right, down or both, away from it
void FillObj::QueueCornerPipes()
{
	PipeObj* aPipe = (PipeObj*)gFillGrid[mColumn][mRow].mPipe;
	if (IsActivePipe(aPipe)) {
		int aKind = aPipe->mKind;
		if (aKind == 1)
			QueuePipe(aPipe, -8.0f, -8.0f);
		else if (aKind == 2)
			QueuePipe(aPipe, 0.0f, -8.0f);
		else if (aKind == 3)
			QueuePipe(aPipe, -8.0f, 0.0f);
	}
	if (mColumn < FILL_GRID_COLUMNS - 1) {
		aPipe = (PipeObj*)gFillGrid[mColumn + 1][mRow].mPipe;
		if (IsActivePipe(aPipe))
			QueuePipe(aPipe, 8.0f, 0.0f);
	}
	if (mRow < FILL_GRID_ROWS - 1) {
		aPipe = (PipeObj*)gFillGrid[mColumn][mRow + 1].mPipe;
		if (IsActivePipe(aPipe))
			QueuePipe(aPipe, 0.0f, 8.0f);
	}
	if (mColumn < FILL_GRID_COLUMNS - 1 && mRow < FILL_GRID_ROWS - 1) {
		aPipe = (PipeObj*)gFillGrid[mColumn + 1][mRow + 1].mPipe;
		if (IsActivePipe(aPipe))
			QueuePipe(aPipe, 8.0f, 8.0f);
	}
}

// 0x428940: the queued pipes start their move
void FillObj::ReleaseQueuedPipes()
{
	for (int i = 0; i < mShiftCount; i++)
		mShiftPipes[i]->StartShift();
}

// 0x428970: the bonus lying under the cell, which keeps a pointer back to it
void FillObj::SetBonus(BonusCoin* theBonus)
{
	mBonus = theBonus;
	if (theBonus != nullptr)
		theBonus->mCell = this;
}

// 0x428990: the picture shows the cel of the level's sheet that belongs to this place, the sheet's width repeating
void FillObj::ShowPicture()
{
	mFillPicture->mAnim = sFillSheet;
	int aCelsAcross = sFillSheet->mCelsAcross;
	int aCel = (mRow % aCelsAcross) * aCelsAcross + mColumn % aCelsAcross;
	mFillPicture->SetState(STATE_FRAME, (float)aCel);
}

// 0x4289e0: the cell is cut away: its picture starts its first track after as many frames as the cell is cells from
// the one the cut began at (five a cell); the cell farthest from there is the one the cut ends at
void FillObj::Cut(int theFromColumn, int theFromRow)
{
	if (mCut)
		return;
	if (!IsActive())
		return;
	mWaveKey->mEventId = sFillWave;
	mWave = sFillWave;
	mFillPicture->StartTrack(0);
	mFillPicture->SetStateInt(STATE_ALPHA, 0);
	double aDx = (double)theFromColumn - (double)mColumn;
	double aDy = (double)theFromRow - (double)mRow;
	float aDistanceSquared = (float)(aDy * aDy + aDx * aDx);
	int aFrames = (int)((double)FloatSqrt(aDistanceSquared) * FRAMES_PER_CELL);
	mFillPicture->mCurrentTrack->SetDelay(aFrames, false);
	Activate(mFillPicture, false);
	mCut = true;
	if (aFrames > sFarthestDistance) {
		sFarthestDistance = aFrames;
		sFarthestCell = this;
	}
	sCellsCut = sCellsCut + 1;
}

// 0x428ac0: the cell under the rope's preview glows, after as many frames as it is cells from the preview's start,
// and its net fly begins to flash
void FillObj::Highlight(int theFromColumn, int theFromRow)
{
	if (!mHighlighted && mHighlightTrack != mCurrentTrack) {
		double aDx = (double)theFromColumn - (double)mColumn;
		double aDy = (double)theFromRow - (double)mRow;
		float aDistanceSquared = (float)(aDy * aDy + aDx * aDx);
		int aFrames = (int)((double)FloatSqrt(aDistanceSquared) * FRAMES_PER_CELL);
		StartTrack(FILL_CELL_TRACK_HIGHLIGHT);
		mCurrentTrack->SetDelay(aFrames + 1, false);
	}
	if (!mNetFly->IsActive()) {
		mNetFlyKey->SetDuration((int)(GameRandomNext() % 26 + 0x1e));
		mNetFly->StartTrack(0);
		Activate(mNetFly, false);
		mNetFly->mCurrentTrack->SetDelay((int)(GameRandomNext() % 26) * 3, false);
		mNetFlyKey->mValue[STATE_SCALE_X] = (float)(ToDouble((int)(GameRandomNext() % 26 + 25)) / 100.0);
		mNetFlyKey->mValue[STATE_SCALE_Y] = mNetFlyKey->mValue[STATE_SCALE_X];
		mNetFlyKey->mValue[STATE_ORBIT_ANGLE] =
			(float)(ToDouble((int)(GameRandomNext() % 361) - 180) * PI_F / 180.0);
		mNetFlyKey->mValue[STATE_ORBIT_RADIUS] = (float)(ToDouble((int)(GameRandomNext() % 33 + 8)) / 100.0);
	}
	mNetFly->SetStateInt(STATE_RED, 1);
	mNetFly->SetStateInt(STATE_GREEN, 1);
	mNetFly->SetStateInt(STATE_BLUE, 1);
	mHighlighted = true;
}

// 0x428c90: the glow goes
void FillObj::Unhighlight()
{
	if (mHighlighted) {
		StartTrack(FILL_CELL_TRACK_UNHIGHLIGHT);
		mCurrentTrack->SetDelay(0, false);
		mHighlighted = false;
	}
}

// 0x428cc0: the level gives the cell its mark as it lays the grid out and hides the cell until Settle; the net fly
// is made ready for its second track, which starts at once
void FillObj::SetMark(int theMark)
{
	mMark = theMark;
	SetHidden(true);
	mNetFly->SetStateInt(STATE_RED, 1);
	mNetFly->SetStateInt(STATE_GREEN, 1);
	mNetFly->SetStateInt(STATE_BLUE, 1);
	mNetFly->SetStateInt(STATE_ALPHA, 0);
	mNetFlyKeyAfter->SetDuration((int)(GameRandomNext() % 41 + 0x32));
	mNetFlyKeyAfter->mValue[STATE_SCALE_X] = (float)(ToDouble((int)(GameRandomNext() % 26 + 25)) / 100.0);
	// the original copies the first flash key's scale into the second key's height, not the second key's own width
	mNetFlyKeyAfter->mValue[STATE_SCALE_Y] = mNetFlyKey->mValue[STATE_SCALE_X];
	mNetFlyKeyAfter->mValue[STATE_ORBIT_ANGLE] = (float)(ToDouble((int)(GameRandomNext() % 361) - 180) * PI_F / 180.0);
	mNetFlyKeyAfter->mValue[STATE_ORBIT_RADIUS] = (float)(ToDouble((int)(GameRandomNext() % 33 + 8)) / 100.0);
	mNetFly->StartTrack(1);
}

// 0x428e00: after the level has given the grid its marks: a cell inside the field is shown and counted, and the net
// fly goes; any other cell is put away
void FillObj::Settle()
{
	Init();
	if (mMark < 0) {
		SetHidden(false);
		Activate(this, false);
		Deactivate(mNetFly);
		sCellsShown = sCellsShown + 1;
		mMark = 0;
		return;
	}
	Deactivate(this);
	mMark = 0;
}

// 0x428e60: the cell and its picture pop out again
void FillObj::Replay()
{
	if (IsActive()) {
		StartTrack(FILL_CELL_TRACK_REPLAY);
		mFillPicture->StartTrack(1);
	}
}

// 0x428e90: pipes queued by a cut move once the wave number of the cell has been released
bool FillObj::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mShiftPending != 0 && mWave <= sPendingWave) {
		ReleaseQueuedPipes();
		mWave = 0;
		mShiftPending = 0;
	}
	return true;
}

// 0x42a0d0: the net fly that flashes on a cell under the rope's preview. A child of the cell, drawn above it and
// added to the picture's light (the keys draw additively), with two tracks: the flash, which runs round a small
// orbit while it swells and fades in and out (the key kept in mNetFlyKey, whose length and orbit change each time),
// and the same for the grid's laying out, with its key in mNetFlyKeyAfter
void FillObj::BuildNetFly()
{
	mNetFly = new ObjBase();
	mNetFly->mUpdateLevel = 2;
	mNetFly->SetParent(this, nullptr);
	sNetFlyAnim->AddRef();
	mNetFly->mAnim = sNetFlyAnim;
	mNetFly->SetStateInt(STATE_Z, 2);
	mNetFly->SetStateInt(STATE_SCALE_X, 0);
	mNetFly->SetStateInt(STATE_SCALE_Y, 0);
	mNetFly->SetStateInt(STATE_RED, 1);
	mNetFly->SetStateInt(STATE_GREEN, 1);
	mNetFly->SetStateInt(STATE_BLUE, 1);
	mNetFly->SetStateInt(STATE_ALPHA, 0);
	mNetFly->SetStateInt(STATE_FRAME, 1);

	AnimKey* aStart = new AnimKey();
	aStart->SetDuration(1);
	aStart->mDrawMode = 1; // 0x4535c0: drawn additively
	aStart->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aStart->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aStart->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aStart->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);

	AnimKey* aFlash = new AnimKey();
	aFlash->SetDuration(0x41);
	aFlash->mDrawMode = 1;
	aFlash->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlash->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlash->AddValue(STATE_ALPHA, 0.55f, MOD_TOWARD_NEXT);
	aFlash->AddValue(STATE_SCALE_X, 0.45f, MOD_HOLD);
	aFlash->AddModifier(STATE_SCALE_X, false, 1.0f, MOD_SINE, 1.0f, 1.0f, 0.0f, nullptr);
	aFlash->AddValue(STATE_SCALE_Y, 0.45f, MOD_HOLD);
	aFlash->AddModifier(STATE_SCALE_Y, false, 1.0f, MOD_SINE, 1.0f, 1.0f, 0.0f, nullptr);
	aFlash->AddValue(STATE_ORBIT_RADIUS, 0.1f, MOD_TOWARD_NEXT);
	aFlash->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_TOWARD_NEXT);
	mNetFlyKey = aFlash;
	// the original makes a third key here (additive, one frame, the orbit's radius 0.02) and never uses it; the port
	// leaves it out (a leak that changes nothing)

	AnimKey* aFlashEnd = new AnimKey(); // the track ends on a key of the defaults
	AnimTrack* aFlashTrack = new AnimTrack();
	aFlashTrack->AddKey(aStart, nullptr);
	aFlashTrack->AddKey(aFlash, nullptr);
	aFlashTrack->AddKey(aFlashEnd, nullptr);
	mNetFly->AddTrack(aFlashTrack, nullptr);
	mNetFly->SetEventTarget(this);
	mNetFly->Deactivate(mNetFly);

	AnimKey* aStartAfter = new AnimKey();
	aStartAfter->SetDuration(1);
	aStartAfter->mDrawMode = 1;
	aStartAfter->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aStartAfter->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aStartAfter->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aStartAfter->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aStartAfter->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);

	AnimKey* aFlashAfter = new AnimKey();
	aFlashAfter->SetDuration(0x4b);
	aFlashAfter->mDrawMode = 1;
	aFlashAfter->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlashAfter->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlashAfter->AddValue(STATE_ALPHA, 0.55f, MOD_TOWARD_NEXT);
	aFlashAfter->AddValue(STATE_SCALE_X, 0.45f, MOD_HOLD);
	aFlashAfter->AddModifier(STATE_SCALE_X, false, 1.0f, MOD_SINE, 1.0f, 1.0f, 0.0f, nullptr);
	aFlashAfter->AddValue(STATE_SCALE_Y, 0.45f, MOD_HOLD);
	aFlashAfter->AddModifier(STATE_SCALE_Y, false, 1.0f, MOD_SINE, 1.0f, 1.0f, 0.0f, nullptr);
	aFlashAfter->AddValue(STATE_ORBIT_RADIUS, (float)(ToDouble((int)(GameRandomNext() % 33 + 8)) / 100.0), MOD_TOWARD_NEXT);
	aFlashAfter->AddValue(STATE_ORBIT_ANGLE, (float)(ToDouble((int)(GameRandomNext() % 361) - 180) * PI_F / 180.0),
		MOD_TOWARD_NEXT);
	mNetFlyKeyAfter = aFlashAfter;

	AnimKey* aEndAfter = new AnimKey();
	aEndAfter->SetDuration(1);
	aEndAfter->mDrawMode = 1;
	aEndAfter->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aEndAfter->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aEndAfter->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aEndAfter->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aEndAfter->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aEndAfter->AddValue(STATE_ORBIT_RADIUS, 0.02f, MOD_HOLD);
	aEndAfter->AddValue(STATE_ORBIT_ANGLE, 0.0f, MOD_HOLD);
	AnimTrack* aAfterTrack = new AnimTrack();
	aAfterTrack->AddKey(aStartAfter, nullptr);
	aAfterTrack->AddKey(aFlashAfter, nullptr);
	aAfterTrack->AddKey(aEndAfter, nullptr);
	mNetFly->AddTrack(aAfterTrack, nullptr);
	mNetFly->Deactivate(mNetFly);
}

// 0x4298f0: the cell's own four tracks. 0 grows the cell in, spinning, over 30 frames and ends with the event
// "settled"; 1 pops it out; 2 and 3 are the glow, which swells and shrinks, and its end. On even rows the tracks that
// grow and glow wait five frames, so the rows come in turn
void FillObj::BuildCellTracks(int theRow)
{
	bool anEvenRow = (theRow % 2) == 0;

	AnimKey* aGrowStart = new AnimKey();
	aGrowStart->SetDuration(1);
	aGrowStart->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aGrowStart->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrowStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGrowStart->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGrowSpin = new AnimKey();
	aGrowSpin->SetDuration(0x14);
	aGrowSpin->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGrowSpin->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrowSpin->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGrowSpin->AddValue(STATE_ROTATION, (float)(ToDouble((int)(GameRandomNext() % 0x5b + 0x5a)) * PI_F / 180.0),
		MOD_TOWARD_NEXT);
	AnimKey* aGrowDone = new AnimKey();
	aGrowDone->mEventId = FILL_EVENT_SETTLED;
	aGrowDone->SetDuration(10);
	aGrowDone->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aGrowDone->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aGrowDone->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aGrowDone->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aGrowEnd = new AnimKey();
	aGrowEnd->mEventId = FILL_EVENT_GLOW_REPEAT;
	aGrowEnd->SetDuration(10);
	AnimTrack* aGrowTrack = new AnimTrack();
	aGrowTrack->AddKey(aGrowStart, nullptr);
	aGrowTrack->AddKey(aGrowSpin, nullptr);
	aGrowTrack->AddKey(aGrowDone, nullptr);
	aGrowTrack->AddKey(aGrowEnd, aGrowEnd);
	if (anEvenRow)
		aGrowTrack->SetDelay(5, false);
	AddTrack(aGrowTrack, nullptr);

	AnimKey* aPopSwell = new AnimKey();
	aPopSwell->SetDuration(0x19);
	aPopSwell->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aPopSwell->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aPopSwell->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPopSwell->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aPopOut = new AnimKey();
	aPopOut->mEventId = FILL_EVENT_UNUSED_POP;
	aPopOut->SetDuration(1);
	aPopOut->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aPopOut->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aPopOut->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	// the turn is a number of degrees the original gives as it is, where the rest of the keys use radians
	aPopOut->AddValue(STATE_ROTATION, (float)((int)(GameRandomNext() % 0x5b) - 0x2d), MOD_HOLD);
	AnimTrack* aPopTrack = new AnimTrack();
	aPopTrack->AddKey(aPopSwell, nullptr);
	aPopTrack->AddKey(aPopOut, nullptr);
	AddTrack(aPopTrack, nullptr);

	AnimKey* aGlowHold = new AnimKey();
	aGlowHold->SetDuration(0xf);
	aGlowHold->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aGlowHold->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aGlowHold->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aGlowHold->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aGlowHoldMore = new AnimKey();
	aGlowHoldMore->SetDuration(0xf);
	aGlowHoldMore->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aGlowHoldMore->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aGlowHoldMore->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimKey* aGlowEnd = new AnimKey();
	aGlowEnd->mEventId = FILL_EVENT_GLOW_REPEAT;
	aGlowEnd->SetDuration(10);
	AnimTrack* aGlowTrack = new AnimTrack();
	aGlowTrack->AddKey(aGlowHold, nullptr);
	aGlowTrack->AddKey(aGlowHoldMore, nullptr);
	aGlowTrack->AddKey(aGlowEnd, aGlowEnd);
	if (anEvenRow)
		aGlowTrack->SetDelay(5, false);
	mHighlightTrack = aGlowTrack;
	AddTrack(aGlowTrack, nullptr);

	AnimKey* aFadeStart = new AnimKey();
	aFadeStart->SetDuration(10);
	aFadeStart->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aFadeStart->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aFadeStart->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aFadeStart->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aFadeLoop = new AnimKey();
	aFadeLoop->mEventId = FILL_EVENT_GLOW_REPEAT;
	aFadeLoop->SetDuration(10);
	AnimTrack* aFadeTrack = new AnimTrack();
	aFadeTrack->AddKey(aFadeStart, nullptr);
	aFadeTrack->AddKey(aFadeLoop, aFadeStart); // the end of the track goes back to its start
	if (anEvenRow)
		aFadeTrack->SetDelay(5, false);
	AddTrack(aFadeTrack, nullptr);
}

// 0x428ee0: the picture's two tracks. 0 is the fill: the picture flies in from 32 pixels off, swollen, spinning and
// clear over 15 frames (with the fill sound), then runs through the events 99 (the wave's number goes here), 4, 1, 2
// and 5; 1 swells it out and throws it away. The first key's side depends on the column's evenness
void FillObj::BuildPictureTracks(int theColumn)
{
	AnimKey* aFlyIn = new AnimKey();
	aFlyIn->mEventId = FILL_EVENT_PICTURE_START;
	aFlyIn->SetDuration(0xf);
	float aSide = (theColumn % 2) == 0 ? -32.0f : 32.0f;
	aFlyIn->AddValue(STATE_X, aSide, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_Y, aSide, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_SCALE_X, 2.5f, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_SCALE_Y, 2.5f, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFlyIn->AddValue(STATE_ROTATION, (float)(ToDouble((int)(GameRandomNext() % 0x5b + 0x5a)) * PI_F / 180.0),
		MOD_TOWARD_NEXT);
	// the key plays the fill sound, which the queue holds back five frames between plays
	aFlyIn->mEndCondition = KEY_SOUND;
	aFlyIn->mEndChannel = SOUND_FILL1;
	aFlyIn->mEndValue = 4.0f;
	ObjBase::sApp->mSoundQueue->DelayFillSound();

	AnimKey* aWaveKey = new AnimKey();
	aWaveKey->SetDuration(1);
	aWaveKey->mEventId = FILL_EVENT_WAVE_PLACEHOLDER;
	aWaveKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aWaveKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aWaveKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aWaveKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aWaveKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aWaveKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	mWaveKey = aWaveKey;

	AnimKey* aShiftKey = new AnimKey();
	aShiftKey->SetDuration(1);
	aShiftKey->mEventId = FILL_EVENT_SHIFT_PIPES;
	aShiftKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aShiftKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aShiftKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aShiftKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aShiftKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aShiftKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimKey* aLandKey = new AnimKey();
	aLandKey->mEventId = FILL_EVENT_LANDED;
	aLandKey->SetDuration(6);
	aLandKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aLandKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aLandKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aLandKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aLandKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aLandKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);

	AnimKey* aResetKey = new AnimKey();
	aResetKey->mEventId = FILL_EVENT_RESET_SHOWN;
	aResetKey->SetDuration(0x32);
	aResetKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aResetKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aResetKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aResetKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);

	AnimKey* aEndKey = new AnimKey();
	aEndKey->mEventId = FILL_EVENT_FILL_END;
	aEndKey->SetDuration(1);
	aEndKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aEndKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aEndKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aEndKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);

	AnimKey* aHoldKey = new AnimKey();
	aHoldKey->SetDuration(1);
	aHoldKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aHoldKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	aHoldKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aHoldKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);

	AnimTrack* aFillTrack = new AnimTrack();
	aFillTrack->AddKey(aFlyIn, nullptr);
	aFillTrack->AddKey(aWaveKey, nullptr);
	aFillTrack->AddKey(aShiftKey, nullptr);
	aFillTrack->AddKey(aLandKey, nullptr);
	aFillTrack->AddKey(aResetKey, nullptr);
	aFillTrack->AddKey(aEndKey, nullptr);
	aFillTrack->AddKey(aHoldKey, aHoldKey);
	mFillPicture->AddTrack(aFillTrack, nullptr);

	AnimKey* aSwell = new AnimKey();
	aSwell->SetDuration(0x64);
	aSwell->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aSwell->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aThrown = new AnimKey();
	aThrown->SetDuration(1);
	aThrown->AddValue(STATE_X, (float)((int)(GameRandomNext() % 0x81) - 0x40), MOD_HOLD);
	aThrown->AddValue(STATE_Y, (float)((int)(GameRandomNext() % 0x81) - 0x40), MOD_HOLD);
	aThrown->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aThrown->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aThrown->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	// degrees again, as the pop-out of the cell's own
	aThrown->AddValue(STATE_ROTATION, (float)((int)(GameRandomNext() % 0xb5) - 0x5a), MOD_HOLD);
	AnimTrack* aThrowTrack = new AnimTrack();
	aThrowTrack->AddKey(aSwell, nullptr);
	aThrowTrack->AddKey(aThrown, nullptr);
	mFillPicture->AddTrack(aThrowTrack, nullptr);
}

// 0x42a9c0: a cell at a place of the grid, hidden. Each cell sits a little above the one made before it
FillObj* FillObj::Create(int theColumn, int theRow)
{
	FillObj* aCell = new FillObj();
	aCell->SetParent(nullptr, nullptr);
	aCell->SetState(STATE_Z, sNextDepth);
	aCell->SetStateInt(STATE_ALPHA, 0);
	sNextDepth = (float)((double)sNextDepth + DEPTH_STEP);
	aCell->BuildNetFly();
	sNetFlyAnim->AddRef();
	aCell->mAnim = sNetFlyAnim;
	aCell->mColumn = theColumn;
	aCell->mRow = theRow;
	aCell->SetStateInt(STATE_X, theColumn * FILL_CELL_SPACING);
	aCell->SetStateInt(STATE_Y, theRow * FILL_CELL_SPACING);
	aCell->SetStateInt(STATE_SCALE_X, 0);
	aCell->SetStateInt(STATE_SCALE_Y, 0);
	aCell->SetStateInt(STATE_FRAME, 0);
	aCell->ComputeDrawn();
	aCell->BuildCellTracks(theRow);
	// the original also formats "images/fills/fill_01" into a buffer here and never reads it

	ObjBase* aPicture = new ObjBase();
	aCell->mFillPicture = aPicture;
	aPicture->mUpdateLevel = 2;
	aPicture->SetParent(aCell, nullptr);
	aPicture->mAnim = sFillSheet; // the sheet is shared: no reference is taken
	aPicture->SetState(STATE_Z, 1.0f);
	aPicture->SetState(STATE_SCALE_X, FILL_PICTURE_SCALE);
	aPicture->SetState(STATE_SCALE_Y, FILL_PICTURE_SCALE);
	aPicture->SetStateInt(STATE_ALPHA, 0);
	aPicture->SetStateInt(STATE_FRAME, 0);
	aCell->BuildPictureTracks(theColumn);
	aPicture->SetEventTarget(aCell);
	aCell->Deactivate(aCell);
	return aCell;
}

// 0x42aba0: no cell has a bonus under it, and none is counted
void FillObj::ClearBonuses()
{
	sCellsShown = 0;
	sCellsSettled = 0;
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			// the original reads the grid unchecked; the port's stand-in grid may hold no cell
			FillObj* aCell = (FillObj*)gFillGrid[aColumn][aRow].mCell;
			if (aCell != nullptr)
				aCell->SetBonus(nullptr);
		}
	}
}

// 0x42abf0: the next cut begins: its counts are cleared and it takes the next wave number
void FillObj::StartWave()
{
	sFarthestDistance = 0;
	sCellsCut = 0;
	sCellsArrived = 0;
	sFarthestCell = nullptr;
	sFillWave = sFillWave + 1;
	if (sFillWave > FILL_WAVE_LAST)
		sFillWave = FILL_WAVE_FIRST;
}

// 0x42ac50: the cell the wave reaches last is the one that ends the cut
void FillObj::MarkFarthest()
{
	if (sFarthestCell != nullptr)
		sFarthestCell->mLastCut = true;
}

// 0x42ac70: every cell of the field has settled
bool FillObj::IsReady()
{
	return sCellsShown > 0 && sCellsSettled >= sCellsShown;
}

// 0x42ac90: the level's picture, cut in 31-pixel cels, by the world the player is in
void FillObj::LoadSheet()
{
	FreeSheet();
	if (sFillSheet == nullptr) {
		char aName[92];
		snprintf(aName, sizeof(aName), "images/fills/fill_%02d", Player::sCurrent->mMapLevel + 1);
		sFillSheet = new Anim();
		sFillSheet->LoadFillSheet(aName);
	}
}

// 0x42ad10
void FillObj::FreeSheet()
{
	if (sFillSheet != nullptr) {
		delete sFillSheet;
		sFillSheet = nullptr;
	}
}

// the grid's pipe at a place (the original reads the table with no check of the place)
PipeObj* FillObj::GetWall(int theColumn, int theRow)
{
	return (PipeObj*)gFillGrid[theColumn][theRow].mPipe;
}

// the grid's cell at a place
FillObj* FillObj::GetTile(int theColumn, int theRow)
{
	return (FillObj*)gFillGrid[theColumn][theRow].mCell;
}

// 0x42ad40: the net fly's picture, two cels of 33 by 33
void Sexy::PreloadNetFly()
{
	FillObj::sNetFlyAnim = Anim::Get("images/netfly", ANIM_SCALE_SMOOTH, false, 0x21, 0x21, 2, false);
}
