#include "RopeObj.h"

#include "Anim.h"
#include "Emitter.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

Anim* RopeObj::sArrowAnims[ROPE_ARROW_ANIMS];
Anim* RopeObj::sPartAnim = nullptr;
Anim* RopeObj::sArrowAuraAnim = nullptr;
Anim* RopeObj::sSnapAnim = nullptr;

// The sounds the rope asks for
const int SOUND_ARROW_HIT = 25;        // an arrow arrives
const int SOUND_ROPE_SNAP = 26;        // a bug breaks the rope

// The original's float constants that are not whole numbers, as it keeps them (a float or a double as noted)
static const double PI_AS_DOUBLE = 3.1415927410125732;   // the float pi widened to a double
static const float HALF_PI_F = 1.5707964f;               // 1.5708 as a float
static const float THREE_HALF_PI_F = 4.712389f;          // 4.71239 as a float
static const float PI_AS_FLOAT = 3.1415927f;              // 3.14159 as a float

// The aiming loop's first key lies this many pixels, and a margin more, back from its second (SetArrowAimKeys)
static const double AIM_KEY_REACH = 31.0;
static const double AIM_KEY_MARGIN = 8.0;

// How far the aim may move, in pixels, before the preview is laid out again
static const float AIM_TOLERANCE = 1.4f;

// How many frames the sparks a snap leaves last
static const int SNAP_FRAMES = 0x136;
// How many frames a bolt runs for
static const int BOLT_FRAMES = 100;

// The rope's whole number to float as the x87 maths takes it
static double ToDouble(int theValue)
{
	return (double)theValue;
}

// The place an arrow flies to, in design pixels (a cell is 31 of them)
static int CellToPixels(int theCell)
{
	return theCell * FILL_CELL_SPACING;
}

// The original's one-key-after-another helper: a track of the keys given, each ending into the next, the last into
// theLoopTo (none for a track that ends)
static AnimTrack* MakeTrack(AnimKey* theFirst, AnimKey* theSecond, AnimKey* theThird, AnimKey* theLoopTo)
{
	AnimTrack* aTrack = new AnimTrack();
	if (theSecond == nullptr) {
		aTrack->AddKey(theFirst, theLoopTo);
	} else if (theThird == nullptr) {
		aTrack->AddKey(theFirst, nullptr);
		aTrack->AddKey(theSecond, theLoopTo);
	} else {
		aTrack->AddKey(theFirst, nullptr);
		aTrack->AddKey(theSecond, nullptr);
		aTrack->AddKey(theThird, theLoopTo);
	}
	return aTrack;
}

// 0x461320: the arrows, the chain, the sparks and the flash are built, with the emitters and the bolts they use
RopeObj::RopeObj()
{
	SetParent(nullptr, nullptr);
	mState = ROPE_IDLE;
	mAimReady = true;
	mEndAlive[0] = true;
	mEndAlive[1] = true;
	mBoltTimer[0] = 0;
	mBoltTimer[1] = 0;
	mShieldFrames = 0;
	// the original leaves these unset until a rope is shot
	mMode = 0;
	mCellX = 0;
	mCellY = 0;
	mAimX = 0.0f;
	mAimY = 0.0f;
	mFrom = 0;
	mTo = 0;
	mVertical = 0;
	mSnapTimer = 0;
	mStartWall = nullptr;
	mEndWall = nullptr;
	mUnused2a4[0] = 0;
	mUnused2a4[1] = 0;
	mUnused8a4 = false;
	mUnused8b0[0] = 0;
	mUnused8b0[1] = 0;

	BuildArrows();
	BuildParts();
	BuildSparks();
	BuildSnapFlash();

	mArrowEmitters[0] = new EmitterSet(this);
	mArrowEmitters[0]->MakeBlueSparks(100.0f);
	mArrowEmitters[0]->MakeRisingSwell(0.0f, 0x80);
	mArrowEmitters[0]->MakePaleBurst(5.0f);
	mArrowEmitters[1] = new EmitterSet(this);
	mArrowEmitters[1]->MakeBlueSparks(100.0f);
	mArrowEmitters[1]->MakeRisingSwell(0.0f, 0x80);
	mSparkWalls[0] = nullptr;
	mSparkWalls[1] = nullptr;
	mBolts[0] = new BoltObj(5);
	mBolts[1] = new BoltObj(10);
	Activate(this, false);
}

// 0x4614e0 and 0x461500: the original frees nothing it built (the arrows, parts, sparks, emitters and bolts live on)
RopeObj::~RopeObj()
{
}

// 0x461510: nothing is shown: the arrows are hidden at the origin and every part is stopped
void RopeObj::Init()
{
	mArrows[0]->mHoverState = 0;
	mArrows[1]->mHoverState = 0;
	mEndAlive[0] = true;
	mEndAlive[1] = true;
	mState = ROPE_IDLE;
	for (int aPart = 0; aPart < ROPE_PARTS; aPart++) {
		ObjBase* aObj = mParts[aPart];
		aObj->StartTrack(aObj->mButtonId);
		Deactivate(aObj);
	}
	for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++) {
		ObjBase* aObj = mArrows[anArrow];
		aObj->SetStateInt(STATE_X, 0);
		aObj->SetStateInt(STATE_Y, 0);
		aObj->SetHidden(true);
	}
}

// 0x461800: an arrow has reached its end (the bonus rope lays a bolt there, and when both have the ground is cut), or
// an arrow's aiming loop has come round (sparks burst on the pipes at the ends of the bonus ropes)
void RopeObj::Notify(ObjBase* theObj)
{
	AnimKey* aKey = theObj->mCurrentKey;
	if (aKey == nullptr)
		return;
	int anEvent = aKey->mEventId;
	if (anEvent == ROPE_EVENT_ARROW_ARRIVED) {
		mEndAlive[theObj->mButtonId] = false;
		if (mMode == 2)
			LayBolt(theObj->mButtonId);
		sApp->mSoundQueue->Request(SOUND_ARROW_HIT, 0, false);
		theObj->mHoverState = 1;
		if (mArrows[0]->mHoverState == 1 && mArrows[1]->mHoverState == 1) {
			mAimReady = true;
			mState = ROPE_CUTTING;
			mMode = 0;
			mArrows[0]->mHoverState = 0;
			mArrows[1]->mHoverState = 0;
			LevelObj::ShowRopePreview(mCellX, mCellY, mVertical, mFrom, mTo);
			LevelObj::CutGround(mCellX, mCellY, mVertical, mFrom, mTo);
		}
	} else if (anEvent == ROPE_EVENT_ARROW_SPARK) {
		if (mSparkWalls[0] != nullptr) {
			for (int aBurst = 0; aBurst < 6; aBurst++)
				mArrowEmitters[0]->BurstBlueSparks(mSparkWalls[0], 0.0f, 0.0f);
		}
		if (mSparkWalls[1] != nullptr) {
			for (int aBurst = 0; aBurst < 6; aBurst++)
				mArrowEmitters[1]->BurstBlueSparks(mSparkWalls[1], 0.0f, 0.0f);
		}
	}
}

// 0x4615c0: runs the bolts; a snapped rope bursts for a while and then goes; the bonus ropes burn or smoke at the
// arrows; a rope that was shot moves the monkey with it and lays the chain
bool RopeObj::Update()
{
	if (!ObjBase::Update())
		return false;
	if (sLevel == nullptr)
		return true;
	for (int aBolt = 0; aBolt < ROPE_ARROWS; aBolt++) {
		if (mBoltTimer[aBolt] > 0) {
			mBoltTimer[aBolt] = mBoltTimer[aBolt] - 1;
			if (mBoltTimer[aBolt] == 0)
				mBolts[aBolt]->Stop();
			else
				mBolts[aBolt]->UpdateBolt();
		}
	}
	if (mState == ROPE_IDLE)
		return true;
	if (mState == ROPE_SNAPPED) {
		if (mSnapTimer > 0) {
			if (mSnapTimer % 4 == 0) {
				ObjBase* aFirst = mArrows[0];
				if (aFirst->IsActive())
					mArrowEmitters[0]->BurstRisingSwell(aFirst->mState[STATE_X], aFirst->mState[STATE_Y]);
				ObjBase* aSecond = mArrows[1];
				if (aSecond->IsActive())
					mArrowEmitters[1]->BurstRisingSwell(aSecond->mState[STATE_X], aSecond->mState[STATE_Y]);
			}
			mSnapTimer = mSnapTimer - 1;
			return true;
		}
		mState = ROPE_IDLE;
		Init();
		return true;
	}

	int aMode = Player::GetRopeMode();
	if (aMode == 1) {
		for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++)
			mArrows[anArrow]->mEmitters->BurstFlame(0.0f, 0.0f);
	} else if (aMode == 2) {
		for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++)
			mArrows[anArrow]->mEmitters->BurstSmoke(0.0f, 0.0f);
	}
	if (mState == ROPE_FLYING) {
		ObjBase* aLevel = sLevel;
		float aWindowY = (float)(ToDouble(mCellY) * 31.0 * (double)mDrawn[STATE_SCALE_Y] * (double)gImageScaleY
			+ (double)aLevel->mDrawn[STATE_Y] - (double)gScreenScaleY * 24.0);
		int aWindowYInt = (int)aWindowY;
		float aWindowX = (float)(31.0 * ToDouble(mCellX) * (double)mDrawn[STATE_SCALE_X] * (double)gImageScaleX
			+ (double)aLevel->mDrawn[STATE_X]);
		int aWindowXInt = (int)aWindowX;
		Player::MoveTo(aWindowXInt, aWindowYInt);
		LayParts(1);
	}
	if (mState == ROPE_AIMING)
		LayParts(0);
	return true;
}

// 0x461a90: lays a bolt along the pipe at one end of the stretch, from one end of that pipe to the other, and
// startles the bugs standing along it
void RopeObj::LayBolt(int theEnd)
{
	int aColumnFirst;
	int aRowFirst;
	int aColumnLast;
	int aRowLast;
	if (mVertical == 0) {
		PipeObj* aWall = theEnd != 0 ? mStartWall : mEndWall;
		PipeObj* aFirst = aWall;
		if (aWall->mUpLink != nullptr)
			aFirst = aWall->mUpLink;
		aColumnFirst = aFirst->mColumn;
		aRowFirst = aFirst->mRow;
		if (aWall->mDownLink != nullptr) {
			aColumnLast = aWall->mDownLink->mColumn;
			aRowLast = aWall->mDownLink->mRow;
		} else {
			aColumnLast = aWall->mColumn;
			aRowLast = aWall->mRow;
		}
		for (int aRow = aRowFirst; aRow <= aRowLast; aRow++) {
			BugObj* aBug = gFillGrid[aColumnFirst][aRow].mFirstBug;
			while (aBug != nullptr) {
				aBug->Zap(0, 1, 0);
				aBug = aBug->mNextInCell;
			}
		}
	} else {
		PipeObj* aWall = theEnd != 0 ? mStartWall : mEndWall;
		PipeObj* aFirst = aWall;
		if (aWall->mLeftLink != nullptr)
			aFirst = aWall->mLeftLink;
		aColumnFirst = aFirst->mColumn;
		aRowFirst = aFirst->mRow;
		if (aWall->mRightLink != nullptr) {
			aColumnLast = aWall->mRightLink->mColumn;
			aRowLast = aWall->mRightLink->mRow;
		} else {
			aColumnLast = aWall->mColumn;
			aRowLast = aWall->mRow;
		}
		for (int aColumn = aColumnFirst; aColumn <= aColumnLast; aColumn++) {
			BugObj* aBug = gFillGrid[aColumn][aRowFirst].mFirstBug;
			while (aBug != nullptr) {
				aBug->Zap(0, 1, 0);
				aBug = aBug->mNextInCell;
			}
		}
	}
	mBoltTimer[theEnd] = BOLT_FRAMES;
	mBolts[theEnd]->SetEnds(CellToPixels(aColumnFirst), CellToPixels(aRowFirst), CellToPixels(aColumnLast),
		CellToPixels(aRowLast));
}

// 0x461c90: puts the chain between the arrows: from the first arrow to the shot cell facing one way, and from the
// second arrow back to it facing the other, a part every 16 pixels; the parts left over are stopped. Each starts
// theTrack. (The first arrow's place is read for both halves, as the original does.)
void RopeObj::LayParts(int theTrack)
{
	int aShotX = CellToPixels(mCellX);
	int aShotY = CellToPixels(mCellY);
	int aUsed = 0;
	if (mVertical == 0) {
		int aEndX = (int)mArrows[1]->mState[STATE_X] - 32;
		int aX = (int)mArrows[0]->mState[STATE_X] + 32;
		if (aX <= aShotX) {
			unsigned int aCount = ((unsigned int)(aShotX - aX) >> 4) + 1;
			int aIndex = aUsed;
			aUsed = (int)aCount;
			do {
				ObjBase* aPart = mParts[aIndex];
				aPart->SetStateInt(STATE_SCALE_X, 1);
				aPart->SetStateInt(STATE_X, aX);
				aPart->SetState(STATE_Y, mArrows[0]->mState[STATE_Y]);
				aPart->SetState(STATE_ROTATION, 0.0f);
				aPart->Activate(aPart, false);
				aPart->ComputeDrawn();
				aPart->StartTrack(theTrack);
				aIndex++;
				aX += ROPE_PART_SPACING;
			} while (aX <= aShotX);
		}
		if (aEndX >= aShotX) {
			unsigned int aCount = ((unsigned int)(aEndX - aShotX) >> 4) + 1;
			int aIndex = aUsed;
			aUsed = aUsed + (int)aCount;
			int aPartX = aEndX;
			do {
				ObjBase* aPart = mParts[aIndex];
				aPart->SetStateInt(STATE_SCALE_X, -1);
				aPart->SetStateInt(STATE_X, aPartX);
				aPart->SetState(STATE_Y, mArrows[0]->mState[STATE_Y]);
				aPart->SetState(STATE_ROTATION, 0.0f);
				aPart->Activate(aPart, false);
				aPart->ComputeDrawn();
				aPart->StartTrack(theTrack);
				aIndex++;
				aPartX -= ROPE_PART_SPACING;
			} while (aPartX >= aShotX);
		}
	} else {
		int aEndY = (int)mArrows[1]->mState[STATE_Y] - 32;
		int aY = (int)mArrows[0]->mState[STATE_Y] + 32;
		if (aY <= aShotY) {
			unsigned int aCount = ((unsigned int)(aShotY - aY) >> 4) + 1;
			int aIndex = aUsed;
			aUsed = (int)aCount;
			do {
				ObjBase* aPart = mParts[aIndex];
				aPart->SetStateInt(STATE_SCALE_X, 1);
				aPart->SetStateInt(STATE_Y, aY);
				aPart->SetState(STATE_X, mArrows[0]->mState[STATE_X]);
				aPart->SetState(STATE_ROTATION, HALF_PI_F);
				aPart->Activate(aPart, false);
				aPart->ComputeDrawn();
				aPart->StartTrack(theTrack);
				aIndex++;
				aY += ROPE_PART_SPACING;
			} while (aY <= aShotY);
		}
		if (aEndY >= aShotY) {
			unsigned int aCount = ((unsigned int)(aEndY - aShotY) >> 4) + 1;
			int aIndex = aUsed;
			aUsed = aUsed + (int)aCount;
			int aPartY = aEndY;
			do {
				ObjBase* aPart = mParts[aIndex];
				aPart->SetStateInt(STATE_SCALE_X, -1);
				aPart->SetStateInt(STATE_Y, aPartY);
				aPart->SetState(STATE_X, mArrows[0]->mState[STATE_X]);
				aPart->SetState(STATE_ROTATION, HALF_PI_F);
				aPart->Activate(aPart, false);
				aPart->ComputeDrawn();
				aPart->StartTrack(theTrack);
				aIndex++;
				aPartY -= ROPE_PART_SPACING;
			} while (aPartY >= aShotY);
		}
	}
	for (int aPart = aUsed; aPart < ROPE_PARTS; aPart++)
		Deactivate(mParts[aPart]);
}

// 0x461fd0: sends an arrow from the shot cell out to theOtherEnd (a column along a row, a row along a column): its
// flight key starts at the cell, the next one is at the end, the last holds there, and the flight lasts as long as the
// distance takes at the player's shot speed
void RopeObj::SetArrowTarget(int theArrow, int theVertical, int theCellX, int theCellY, int theOtherEnd)
{
	AnimKey* aEndKey = mArrowGatherEndKey[theArrow];
	AnimKey* aFlightKey = mArrowFlightKey[theArrow];
	AnimKey* aArrivedKey = mArrowArrivedKey[theArrow];
	float aStartX = (float)(ToDouble(theCellX) * 31.0);
	float aStartY = (float)(ToDouble(theCellY) * 31.0);
	aFlightKey->mValue[STATE_X] = aStartX;
	aFlightKey->mValue[STATE_Y] = aStartY;
	float aDistance;
	float aRotation;
	if (theVertical == 0) {
		int aSteps = theOtherEnd - theCellX;
		aDistance = (float)(ToDouble(aSteps) * 31.0);
		aArrivedKey->mValue[STATE_X] = (float)((double)aStartX + (double)aDistance);
		aArrivedKey->mValue[STATE_Y] = aStartY;
		aRotation = theArrow == 0 ? HALF_PI_F : THREE_HALF_PI_F;
	} else {
		int aSteps = theOtherEnd - theCellY;
		aDistance = (float)(ToDouble(aSteps) * 31.0);
		aArrivedKey->mValue[STATE_X] = aStartX;
		aArrivedKey->mValue[STATE_Y] = (float)((double)aStartY + (double)aDistance);
		aRotation = theArrow == 0 ? 0.0f : PI_AS_FLOAT;
	}
	mArrows[theArrow]->SetState(STATE_ROTATION, aRotation);
	aFlightKey->mEasing = EASE_LINEAR_CLAMPED;
	float aLength = FloatAbs(aDistance);
	aFlightKey->SetDuration((int)(Player::GetShotInterval() * (double)aLength) + 10);
	aEndKey->mValue[STATE_X] = aArrivedKey->mValue[STATE_X];
	aEndKey->mValue[STATE_Y] = aArrivedKey->mValue[STATE_Y];
	mArrows[theArrow]->SetHidden(false);
	mArrows[theArrow]->StartTrack(mArrowFlightTrack[theArrow]);
	mArrows[theArrow]->Update();
}

// 0x462130: sets an arrow's aiming loop for a preview: the loop's second key is at the far end of the stretch
// theEnd (in design pixels) along the aim's row or column, the first one a cell and a margin back from it
void RopeObj::SetArrowAimKeys(int theArrow, int theVertical, float theAimX, float theAimY, float theEnd)
{
	AnimKey* aFirstKey = mArrowAimKey[theArrow];
	AnimKey* aSecondKey = mArrowAimEventKey[theArrow];
	float aRotation;
	if (theVertical == 0) {
		float aDelta = (float)((double)theEnd - (double)theAimX);
		float aSum = (float)((double)theAimX + (double)aDelta);
		aSecondKey->mValue[STATE_X] = aSum;
		aSecondKey->mValue[STATE_Y] = theAimY;
		double aShifted;
		if (0.0f < aDelta)
			aShifted = (double)aSum - AIM_KEY_REACH - AIM_KEY_MARGIN;
		else
			aShifted = (double)aSum + AIM_KEY_REACH + AIM_KEY_MARGIN;
		aFirstKey->mValue[STATE_X] = (float)aShifted;
		aFirstKey->mValue[STATE_Y] = aSecondKey->mValue[STATE_Y];
		aRotation = theArrow == 0 ? HALF_PI_F : THREE_HALF_PI_F;
	} else {
		float aDelta = (float)((double)theEnd - (double)theAimY);
		aSecondKey->mValue[STATE_X] = theAimX;
		aSecondKey->mValue[STATE_Y] = (float)((double)theAimY + (double)aDelta);
		aFirstKey->mValue[STATE_X] = theAimX;
		double aShifted;
		if (0.0f < aDelta)
			aShifted = (double)aSecondKey->mValue[STATE_Y] - AIM_KEY_REACH - AIM_KEY_MARGIN;
		else
			aShifted = (double)aSecondKey->mValue[STATE_Y] + AIM_KEY_REACH + AIM_KEY_MARGIN;
		aFirstKey->mValue[STATE_Y] = (float)aShifted;
		aRotation = theArrow == 0 ? 0.0f : PI_AS_FLOAT;
	}
	mArrows[theArrow]->SetState(STATE_ROTATION, aRotation);
	Activate(mArrows[theArrow], false);
	mArrows[theArrow]->StartTrack(mArrowAimTrack[theArrow]);
	mArrows[theArrow]->Update();
}

// 0x462270: shoots the rope along the stretch theFrom to theTo (columns of the row theCellY, or rows of the column
// theCellX) from the cell theCellX, theCellY
void RopeObj::Start(int theVertical, int theCellX, int theCellY, int theFrom, int theTo)
{
	mMode = Player::GetRopeMode();
	Init();
	mVertical = theVertical;
	mAimReady = false;
	mState = ROPE_FLYING;
	mCellX = theCellX;
	mCellY = theCellY;
	mFrom = theFrom;
	mTo = theTo;
	if (theVertical == 0) {
		mStartWall = (PipeObj*)gFillGrid[theFrom][theCellY].mPipe;
		mEndWall = (PipeObj*)gFillGrid[theTo][theCellY].mPipe;
	} else {
		mStartWall = (PipeObj*)gFillGrid[theCellX][theFrom].mPipe;
		mEndWall = (PipeObj*)gFillGrid[theCellX][theTo].mPipe;
	}
	SetArrowTarget(0, theVertical, theCellX, theCellY, theFrom);
	SetArrowTarget(1, theVertical, theCellX, theCellY, theTo);
	BugObj::AlertAlongRope(mStartWall, mEndWall, mVertical);
	Activate(this, false);
}

// 0x462360: shows the preview of a shot along the stretch theFrom to theTo (kept as it is while the aim stays within
// a pixel and a half and the stretch is the same; not while a rope runs or has snapped)
void RopeObj::SetAim(int theVertical, int theCellX, int theCellY, int theFrom, int theTo)
{
	if (mState == ROPE_SNAPPED || mState == ROPE_FLYING)
		return;
	ObjBase* aPlayer = Player::sCurrent;
	float aAimX = (float)((double)aPlayer->mState[STATE_X] - (double)sLevel->mState[STATE_X]);
	float aAimY = (float)(((double)aPlayer->mState[STATE_Y] - (double)sLevel->mState[STATE_Y]) + 24.0);
	// the original does not compare the cell, only the direction and the ends
	if (mVertical == theVertical && mState == ROPE_AIMING && mFrom == theFrom && mTo == theTo) {
		float aDeltaX = FloatAbs((float)((double)mAimX - (double)aAimX));
		if (!((double)aDeltaX > (double)AIM_TOLERANCE)) {
			float aDeltaY = FloatAbs((float)((double)mAimY - (double)aAimY));
			if (!((double)aDeltaY > (double)AIM_TOLERANCE))
				return;
		}
	}
	mVertical = theVertical;
	mState = ROPE_AIMING;
	mCellX = theCellX;
	mCellY = theCellY;
	mFrom = theFrom;
	mTo = theTo;
	if (theVertical == 0) {
		mStartWall = (PipeObj*)gFillGrid[theFrom][theCellY].mPipe;
		mEndWall = (PipeObj*)gFillGrid[theTo][theCellY].mPipe;
	} else {
		mStartWall = (PipeObj*)gFillGrid[theCellX][theFrom].mPipe;
		mEndWall = (PipeObj*)gFillGrid[theCellX][theTo].mPipe;
	}
	// the arrows' pictures are swapped with no references taken or let go, as in the original
	mArrows[0]->mAnim = sArrowAnims[Player::GetRopeMode()];
	mArrows[1]->mAnim = sArrowAnims[Player::GetRopeMode()];
	int aMode = Player::GetRopeMode();
	if (aMode == 2 || aMode == 3) {
		mSparkWalls[0] = mStartWall;
		mSparkWalls[1] = mEndWall;
	} else {
		mSparkWalls[0] = nullptr;
		mSparkWalls[1] = nullptr;
	}
	mAimX = aAimX;
	mAimY = aAimY;
	SetArrowAimKeys(0, theVertical, aAimX, aAimY, (float)(theFrom * FILL_CELL_SPACING));
	SetArrowAimKeys(1, theVertical, aAimX, aAimY, (float)(theTo * FILL_CELL_SPACING));
	Activate(this, false);
	LayParts(0);
}

// 0x462600: an aiming preview is put away
void RopeObj::ResetIfFinished()
{
	if (mState == ROPE_AIMING) {
		mSparkWalls[0] = nullptr;
		mSparkWalls[1] = nullptr;
		Init();
	}
}

// 0x462640: a bug has broken the rope at this place. The sound plays, every part runs to the shot point from where it
// is, the arrows too, a flash shows at the part nearest the bug and the emitters burst. False if it had already snapped
bool RopeObj::Snap(float theX, float theY)
{
	if (mState == ROPE_SNAPPED)
		return false;
	sApp->mSoundQueue->Request(SOUND_ROPE_SNAP, 0, false);
	float aShotX = (float)CellToPixels(mCellX);
	float aShotY = (float)CellToPixels(mCellY);
	mState = ROPE_SNAPPED;
	mSnapTimer = SNAP_FRAMES;
	ObjBase* aNearest = nullptr;
	float aNearestX = 0.0f;
	float aNearestY = 0.0f;
	float aNearestDistance = 9999.0f;
	for (int aPartIndex = 0; aPartIndex < ROPE_PARTS; aPartIndex++) {
		ObjBase* aPart = mParts[aPartIndex];
		if (aPart->IsActive()) {
			double aDeltaX = (double)aPart->mScreenX - (double)theX;
			double aDeltaY = (double)aPart->mScreenY - (double)theY;
			float aSquared = (float)(aDeltaY * aDeltaY + aDeltaX * aDeltaX);
			float aDistance = FloatSqrt(aSquared);
			if (aNearestDistance > aDistance) {
				aNearest = aPart;
				aNearestX = aPart->mState[STATE_X];
				aNearestY = aPart->mState[STATE_Y];
				aNearestDistance = aDistance;
			}
			mPartGatherFromKey[aPartIndex]->mValue[STATE_X] = aPart->mState[STATE_X];
			mPartGatherFromKey[aPartIndex]->mValue[STATE_Y] = aPart->mState[STATE_Y];
			mPartGatherToKey[aPartIndex]->mValue[STATE_X] = aShotX;
			mPartGatherToKey[aPartIndex]->mValue[STATE_Y] = aShotY;
			aPart->SetStateInt(STATE_X, 0);
			aPart->SetStateInt(STATE_Y, 0);
			aPart->StartTrack(aPart->mUnusedF0);
			aPart->ComputeDrawn();
		}
	}
	if (aNearest != nullptr) {
		mSnapFlash->SetState(STATE_X, aNearestX);
		mSnapFlash->SetState(STATE_Y, aNearestY);
		mSnapFlash->ComputeDrawn();
		mSnapFlash->StartTrack(0);
		Activate(mSnapFlash, false);
		mArrowEmitters[0]->BurstPaleBurst(aNearestX, aNearestY);
	}
	mArrowEmitters[0]->BurstRisingSwell(mArrows[0]->mState[STATE_X], mArrows[0]->mState[STATE_Y]);
	mArrowEmitters[1]->BurstRisingSwell(mArrows[1]->mState[STATE_X], mArrows[1]->mState[STATE_Y]);
	mArrowEmitters[0]->BurstRisingSwell(mArrows[0]->mState[STATE_X], mArrows[0]->mState[STATE_Y]);
	mArrowEmitters[1]->BurstRisingSwell(mArrows[1]->mState[STATE_X], mArrows[1]->mState[STATE_Y]);
	for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++) {
		ObjBase* aObj = mArrows[anArrow];
		mArrowGatherFromKey[anArrow]->mValue[STATE_X] = aObj->mState[STATE_X];
		mArrowGatherFromKey[anArrow]->mValue[STATE_Y] = aObj->mState[STATE_Y];
		mArrowGatherToKey[anArrow]->mValue[STATE_X] = aShotX;
		mArrowGatherToKey[anArrow]->mValue[STATE_Y] = aShotY;
		aObj->SetStateInt(STATE_X, 0);
		aObj->SetStateInt(STATE_Y, 0);
		aObj->StartTrack(mArrowGatherTrack[anArrow]);
		aObj->ComputeDrawn();
	}
	return true;
}

// 0x462950: whether the rope touches a bug's box (centre and size given). While the rope runs, an arrow that still
// flies counts if it is within the reach the rope mode gives (4 by 6, flame 5 by 8, smoke 6 by 9, the longer side along
// the rope); a usual rope also counts along its whole length. The usual rope costs the monkey a life and snaps, unless
// it is shielded.
int RopeObj::HitBox(float theX, float theY, float theWidth, float theHeight)
{
	int aResult = ROPE_HIT_NONE;
	if (mState != ROPE_FLYING)
		return ROPE_HIT_NONE;
	if (!IsActive())
		return ROPE_HIT_NONE;
	float aHalfWidth = (float)((double)theWidth * 0.5);
	float aHalfHeight = (float)(0.5 * (double)theHeight);
	float aLeft = (float)((double)theX - (double)aHalfWidth);
	float aTop = (float)((double)theY - (double)aHalfHeight);
	float aRight = (float)((double)theX + (double)aHalfWidth);
	float aBottom = (float)((double)theY + (double)aHalfHeight);
	int aMode = Player::GetRopeMode();
	float aShortReach;
	float aLongReach;
	if (aMode == 1) {
		aShortReach = 5.0f;
		aLongReach = 8.0f;
	} else if (aMode == 2) {
		aShortReach = 6.0f;
		aLongReach = 9.0f;
	} else {
		aShortReach = 4.0f;
		aLongReach = 6.0f;
	}
	bool aTouched = false;
	if (mVertical == 0) {
		for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++) {
			if (!mEndAlive[anArrow])
				continue;
			ObjBase* aObj = mArrows[anArrow];
			float aLow = (float)((double)aObj->mScreenX - (double)aLongReach);
			if (aLow > aRight)
				continue;
			float aHigh = (float)((double)aLongReach + (double)aObj->mScreenX);
			if (aHigh < aLeft)
				continue;
			float aLowY = (float)((double)aObj->mScreenY - (double)aShortReach);
			if (aLowY > aBottom)
				continue;
			float aHighY = (float)((double)aObj->mScreenY + (double)aShortReach);
			if (aHighY < aTop)
				continue;
			aTouched = true;
			break;
		}
	} else {
		for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++) {
			if (!mEndAlive[anArrow])
				continue;
			ObjBase* aObj = mArrows[anArrow];
			float aLow = (float)((double)aObj->mScreenX - (double)aShortReach);
			if (aLow > aRight)
				continue;
			float aHigh = (float)((double)aObj->mScreenX + (double)aShortReach);
			if (aHigh < aLeft)
				continue;
			float aLowY = (float)((double)aObj->mScreenY - (double)aLongReach);
			if (aLowY > aBottom)
				continue;
			float aHighY = (float)((double)aObj->mScreenY + (double)aLongReach);
			if (aHighY < aTop)
				continue;
			aTouched = true;
			break;
		}
	}
	if (!aTouched) {
		if (aMode != 0)
			return ROPE_HIT_NONE;
		// the usual rope: its whole length, a pixel thick, between the arrows
		ObjBase* aFirst = mArrows[0];
		float aRopeLeft;
		float aRopeTop;
		float aRopeRight;
		float aRopeBottom;
		if (mVertical == 0) {
			aRopeLeft = aFirst->mScreenX;
			aRopeTop = (float)((double)aFirst->mScreenY - 1.0);
			aRopeRight = mArrows[1]->mScreenX;
			aRopeBottom = (float)(1.0 + (double)mArrows[1]->mScreenY);
		} else {
			aRopeLeft = (float)((double)aFirst->mScreenX - 1.0);
			aRopeTop = aFirst->mScreenY;
			aRopeRight = (float)((double)mArrows[1]->mScreenX + 1.0);
			aRopeBottom = mArrows[1]->mScreenY;
		}
		if (aRopeLeft > aRight)
			return ROPE_HIT_NONE;
		if (aRopeRight < aLeft)
			return ROPE_HIT_NONE;
		if (aRopeTop > aBottom)
			return ROPE_HIT_NONE;
		if (aRopeBottom < aTop)
			return ROPE_HIT_NONE;
		aResult = ROPE_HIT_TOUCH;
	} else {
		if (aMode == 1)
			return ROPE_HIT_BURN;
		aResult = ROPE_HIT_TOUCH;
		if (aMode == 2)
			return aResult;
	}
	if (mShieldFrames < 1) {
		Player::LoseLife();
		Snap(theX, theY);
	}
	return aResult;
}

// 0x462c90: six little objects round each arrow (three angles, two arrows each), each a glow that turns
void RopeObj::BuildSparks()
{
	int aSlot = 0;
	int aIndex = 0;
	for (int anAngle = 0; anAngle < 270; anAngle += 90) {
		int aParity = aIndex % 2;
		for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++) {
			float aRotation = (float)(ToDouble(anAngle) * PI_AS_DOUBLE / 180.0);
			ObjBase* aSpark = new ObjBase();
			mSparks[aSlot] = aSpark;
			aSpark->SetParent(mArrows[anArrow], nullptr);
			aSpark->mButtonId = anArrow;
			aSpark->SetStateInt(STATE_Z, -1);
			aSpark->SetState(STATE_ALPHA, 0.2f);
			aSpark->SetState(STATE_ROTATION, aRotation);

			AnimKey* aFirstKey = new AnimKey();
			sArrowAuraAnim->AddRef();
			aFirstKey->mAnim = sArrowAuraAnim;
			aFirstKey->SetDuration(0x32);
			aFirstKey->mDrawMode = 1;
			AnimKey* aSecondKey = new AnimKey();
			sArrowAuraAnim->AddRef();
			aSecondKey->mAnim = sArrowAuraAnim;
			aSecondKey->SetDuration(0x32);
			aSecondKey->mDrawMode = 1;
			// the angles alternate between a steady turn and a wrap by the same step
			int aKind = aParity == 0 ? MOD_SPIN : MOD_WRAP_DEGREES;
			aFirstKey->AddModifier(STATE_ROTATION, false, 0.0f, aKind, 1.0f, 1.0f, 0.0f, nullptr);
			aSecondKey->AddModifier(STATE_ROTATION, false, 0.0f, aKind, 1.0f, 1.0f, 0.0f, nullptr);

			AnimTrack* aTrack = MakeTrack(aFirstKey, aSecondKey, nullptr, aFirstKey);
			aSpark->AddTrack(aTrack, nullptr);
			Deactivate(aSpark);
			aSlot++;
		}
		aIndex++;
	}
}

// 0x462f00: the two arrows, each with its tracks: the aiming loop, the idle loop, the flight to the end (arrives, waits,
// fades) and the run to the shot point when the rope snaps. (The original builds the flight key twice and keeps the
// second: the first is a leak, left out here.)
void RopeObj::BuildArrows()
{
	for (int anArrow = 0; anArrow < ROPE_ARROWS; anArrow++) {
		ObjBase* aArrow = new ObjBase();
		mArrows[anArrow] = aArrow;
		aArrow->SetParent(this, nullptr);
		sArrowAnims[0]->AddRef();
		aArrow->mAnim = sArrowAnims[0];
		aArrow->mButtonId = anArrow;
		aArrow->SetStateInt(STATE_Z, -1);
		aArrow->SetEventTarget(this);
		aArrow->mEmitters = new EmitterSet(aArrow);
		aArrow->mEmitters->MakeFlame(2.0f);
		aArrow->mEmitters->mFlame->SetEmitOnCall();
		aArrow->mEmitters->MakeSmoke(2.0f);

		// the aiming loop
		AnimKey* aAimKey = new AnimKey();
		mArrowAimKey[anArrow] = aAimKey;
		aAimKey->SetDuration(0x32);
		aAimKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aAimKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aAimKey->AddValue(STATE_ALPHA, -0.4f, MOD_TOWARD_NEXT);
		aAimKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 30.0f, 16.0f, 0.0f, nullptr);
		aAimKey->mEasing = EASE_COSINE;
		AnimKey* aAimEventKey = new AnimKey();
		mArrowAimEventKey[anArrow] = aAimEventKey;
		aAimEventKey->mEventId = ROPE_EVENT_ARROW_SPARK;
		aAimEventKey->SetDuration(0x32);
		aAimEventKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aAimEventKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aAimEventKey->AddValue(STATE_ALPHA, -0.5f, MOD_TOWARD_NEXT);
		aAimEventKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 30.0f, 16.0f, 0.0f, nullptr);
		aAimEventKey->mEasing = EASE_COSINE;
		AnimTrack* aAimTrack = MakeTrack(aAimKey, aAimEventKey, nullptr, aAimKey);
		mArrowAimTrack[anArrow] = aArrow->AddTrack(aAimTrack, nullptr);

		// the idle loop
		AnimKey* aIdleKey = new AnimKey();
		mArrowIdleKey[anArrow] = aIdleKey;
		aIdleKey->SetDuration(0x1e);
		aIdleKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aIdleKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aIdleKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aIdleKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 30.0f, 16.0f, 0.0f, nullptr);
		AnimTrack* aIdleTrack = MakeTrack(aIdleKey, nullptr, nullptr, aIdleKey);
		mArrowIdleTrack[anArrow] = aArrow->AddTrack(aIdleTrack, nullptr);

		// the flight, its arrival and the fade
		AnimKey* aFlightKey = new AnimKey();
		mArrowFlightKey[anArrow] = aFlightKey;
		aFlightKey->mEventId = ROPE_EVENT_ARROW_FLIGHT;
		aFlightKey->SetDuration(100);
		aFlightKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aFlightKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aFlightKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aFlightKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, 30.0f, 16.0f, 0.0f, nullptr);
		AnimKey* aArrivedKey = new AnimKey();
		mArrowArrivedKey[anArrow] = aArrivedKey;
		aArrivedKey->mEventId = ROPE_EVENT_ARROW_ARRIVED;
		aArrivedKey->SetDuration(0xfa);
		aArrivedKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aArrivedKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aArrivedKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aArrivedKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
		AnimKey* aGatherEndKey = new AnimKey();
		mArrowGatherEndKey[anArrow] = aGatherEndKey;
		aGatherEndKey->mEventId = ROPE_EVENT_ARROW_GATHER;
		aGatherEndKey->SetDuration(1);
		aGatherEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGatherEndKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGatherEndKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
		aGatherEndKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
		AnimTrack* aFlightTrack = MakeTrack(aFlightKey, aArrivedKey, aGatherEndKey, nullptr);
		mArrowFlightTrack[anArrow] = aArrow->AddTrack(aFlightTrack, nullptr);

		// the run to the shot point
		AnimKey* aGatherFromKey = new AnimKey();
		mArrowGatherFromKey[anArrow] = aGatherFromKey;
		aGatherFromKey->SetDuration(0x23);
		aGatherFromKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGatherFromKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGatherFromKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
		aGatherFromKey->mEasing = EASE_IN_SQUARE;
		AnimKey* aGatherToKey = new AnimKey();
		mArrowGatherToKey[anArrow] = aGatherToKey;
		aGatherToKey->SetDuration(1);
		aGatherToKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGatherToKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGatherToKey->AddValue(STATE_ROTATION, PI_AS_FLOAT, MOD_TOWARD_NEXT);
		AnimTrack* aGatherTrack = MakeTrack(aGatherFromKey, aGatherToKey, nullptr, nullptr);
		mArrowGatherTrack[anArrow] = aArrow->AddTrack(aGatherTrack, nullptr);
	}
}

// 0x463730: the chain's 120 parts, each with its four tracks: the faint pulse of the aiming preview, the pulse of the
// rope shot, the flying off (a drift whose radius and turn are random, ending when it has fallen off the window) and the
// run to the shot point (a few frames longer for every part in turn)
void RopeObj::BuildParts()
{
	for (int aPartIndex = 0; aPartIndex < ROPE_PARTS; aPartIndex++) {
		ObjBase* aPart = new ObjBase();
		mParts[aPartIndex] = aPart;
		aPart->SetParent(this, nullptr);
		aPart->SetState(STATE_Z, (float)(-ToDouble(aPartIndex) / 100.0));
		sPartAnim->AddRef();
		aPart->mAnim = sPartAnim;
		aPart->SetStateInt(STATE_SCALE_X, 1);
		aPart->SetStateInt(STATE_SCALE_Y, 1);
		Deactivate(aPart);

		// the faint pulse (track 0; AddTrack's number is not kept, the first track is 0)
		AnimKey* aGhostSwell = new AnimKey();
		aGhostSwell->SetDuration(100);
		aGhostSwell->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aGhostSwell->AddValue(STATE_SCALE_Y, 0.05f, MOD_TOWARD_NEXT);
		aGhostSwell->AddValue(STATE_ALPHA, -0.8f, MOD_TOWARD_NEXT);
		aGhostSwell->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
		AnimKey* aGhostShrink = new AnimKey();
		aGhostShrink->SetDuration(100);
		aGhostShrink->AddValue(STATE_SCALE_X, 0.05f, MOD_TOWARD_NEXT);
		aGhostShrink->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGhostShrink->AddValue(STATE_ALPHA, -0.8f, MOD_TOWARD_NEXT);
		aGhostShrink->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
		AnimTrack* aGhostTrack = MakeTrack(aGhostSwell, aGhostShrink, nullptr, aGhostSwell);
		aPart->AddTrack(aGhostTrack, nullptr);

		// the pulse of the rope shot
		AnimKey* aSolidSwell = new AnimKey();
		aSolidSwell->SetDuration(100);
		aSolidSwell->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aSolidSwell->AddValue(STATE_SCALE_Y, 0.05f, MOD_TOWARD_NEXT);
		aSolidSwell->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aSolidSwell->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
		AnimKey* aSolidShrink = new AnimKey();
		aSolidShrink->SetDuration(100);
		aSolidShrink->AddValue(STATE_SCALE_X, 0.05f, MOD_TOWARD_NEXT);
		aSolidShrink->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aSolidShrink->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aSolidShrink->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
		AnimTrack* aSolidTrack = MakeTrack(aSolidSwell, aSolidShrink, nullptr, aSolidSwell);
		aPart->mButtonId = aPart->AddTrack(aSolidTrack, nullptr);

		// the flying off
		AnimKey* aDrift = new AnimKey();
		aDrift->SetDuration(300);
		aDrift->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
		aDrift->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
		aDrift->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aDrift->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aDrift->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		unsigned int aRadiusDraw = GameRandomNext() % 0x65;
		aDrift->AddValue(STATE_ORBIT_RADIUS, (float)(ToDouble((int)aRadiusDraw) / 100.0 + 2.0), MOD_TOWARD_NEXT);
		aDrift->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
		// the drift ends when it has passed the bottom of the window
		aDrift->mEndCondition = KEY_END_ABOVE;
		aDrift->mEndChannel = STATE_Y;
		aDrift->mEndValue = 1800.0f;
		aDrift->mEasing = EASE_IN_SQUARE;
		AnimKey* aDriftEnd = new AnimKey();
		aDriftEnd->mEventId = ROPE_EVENT_PART_FLY_OFF;
		aDriftEnd->SetDuration(1);
		aDriftEnd->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
		aDriftEnd->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
		aDriftEnd->AddValue(STATE_ORBIT_RADIUS, 32.0f, MOD_HOLD);
		unsigned int aTurnDraw = GameRandomNext() % 0x2d1;
		aDriftEnd->AddValue(STATE_ROTATION,
			(float)(ToDouble((int)aTurnDraw - 0x168) * PI_AS_DOUBLE / 180.0), MOD_TOWARD_NEXT);
		AnimTrack* aFlyOffTrack = MakeTrack(aDrift, aDriftEnd, nullptr, nullptr);
		aPart->mHoverState = aPart->AddTrack(aFlyOffTrack, nullptr);

		// the run to the shot point (Snap sets the two keys' places)
		AnimKey* aGatherFrom = new AnimKey();
		aGatherFrom->SetDuration((aPartIndex & 3) + 0x19);
		aGatherFrom->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGatherFrom->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGatherFrom->mEasing = EASE_IN_SQUARE;
		mPartGatherFromKey[aPartIndex] = aGatherFrom;
		AnimKey* aGatherTo = new AnimKey();
		aGatherTo->SetDuration(1);
		aGatherTo->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGatherTo->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		mPartGatherToKey[aPartIndex] = aGatherTo;
		AnimTrack* aGatherTrack = MakeTrack(aGatherFrom, aGatherTo, nullptr, nullptr);
		aPart->mUnusedF0 = aPart->AddTrack(aGatherTrack, nullptr);
	}
}

// 0x463f40: the flash that shows where a snap happened: it swells with a wobble, fades and is gone
void RopeObj::BuildSnapFlash()
{
	ObjBase* aFlash = new ObjBase();
	mSnapFlash = aFlash;
	aFlash->SetParent(this, nullptr);
	aFlash->SetStateInt(STATE_Z, 500);
	sSnapAnim->AddRef();
	aFlash->mAnim = sSnapAnim;
	aFlash->SetStateInt(STATE_SCALE_X, 0);
	aFlash->SetStateInt(STATE_SCALE_Y, 0);
	aFlash->SetStateInt(STATE_ALPHA, 0);

	AnimKey* aSwellKey = new AnimKey();
	aSwellKey->SetDuration(2);
	aSwellKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwellKey->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aSwellKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwellKey->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aSwellKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aFadeKey = new AnimKey();
	aFadeKey->SetDuration(0x28);
	aFadeKey->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_ALPHA, 0.9f, MOD_TOWARD_NEXT);
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = MakeTrack(aSwellKey, aFadeKey, aGoneKey, nullptr);
	aFlash->AddTrack(aTrack, nullptr);
	Deactivate(aFlash);
}

// 0x464250: the pictures the rope uses, loaded once and kept
void Sexy::PreloadRope()
{
	RopeObj::sArrowAnims[0] = Anim::Get("images/rope_arrow_0", ANIM_SCALE_SMOOTH, false, 0x40, 0x40, 0x10, false);
	RopeObj::sArrowAnims[1] = Anim::Get("images/rope_arrow_1", ANIM_SCALE_SMOOTH, false, 0x40, 0x40, 0x10, false);
	RopeObj::sArrowAnims[2] = Anim::Get("images/rope_arrow_2", ANIM_SCALE_SMOOTH, false, 0x40, 0x40, 0x10, false);
	RopeObj::sArrowAnims[3] = Anim::Get("images/rope_arrow_3", ANIM_SCALE_SMOOTH, false, 0x40, 0x40, 0x10, false);
	RopeObj::sArrowAuraAnim = Anim::Get("images/arrow_aura", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	RopeObj::sPartAnim = Anim::Get("images/rope_part", ANIM_SCALE_SMOOTH, false, 0x19, 0x19, 2, false);
	RopeObj::sSnapAnim = Anim::Get("images/rope_snap", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
}
