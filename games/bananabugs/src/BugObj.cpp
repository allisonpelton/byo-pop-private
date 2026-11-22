#include "BugObj.h"

#include "Anim.h"
#include "BananaBonus.h"
#include "BugImg.h"
#include "Compat2004.h"
#include "Emitter.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Pending.h"
#include "WalkThru.h"
#include "PipeObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "platform/crt/CrtQsort.h"

#include <cstring>

using namespace Sexy;

int BugObj::sCount = 0;
int BugObj::sAlive = 0;
int BugObj::sKillStreak = 0;
float BugObj::sNextDepth = 0.0f;
int BugObj::sSlowFrames = 0;
float BugObj::sUnused2C0 = 0.0f;
float BugObj::sZapLeft = 0.0f;
float BugObj::sZapTop = 0.0f;
float BugObj::sZapRight = 0.0f;
float BugObj::sZapBottom = 0.0f;
int BugObj::sZapMode = 0;
bool BugObj::sAlwaysPeaHit = false;

// The list of bugs, 0x60e2d8
static BugObj* sList[BUG_LIST_MAX];

// What the zap area's modes are (BugObj::sZapMode)
enum ZapMode
{
	ZAP_NONE = 0,
	ZAP_HOLD = 1,                                  // a bug in the area is held ten frames more, every frame
	ZAP_KILL = 2,                                  // a bug in the area dies (with the sound of the zapper)
};

// The places the grid has
static const int GRID_COLUMNS = FILL_GRID_COLUMNS;
static const int GRID_ROWS = FILL_GRID_ROWS;

// The size of the box the rope is asked about when a bug has no body to give one: 54 design pixels
static const float BOX_SIZE_WITHOUT_BODY = 54.0f;
// The bugs a rope shot along a line warns, from the line, in design pixels
static const double ALERT_REACH = 248.0;
// How far from the screen's edge the nearest-bug search counts a bug; a bug farther (or none) reports 0
static const float NEAREST_LIMIT = 1920.0f;
static const float NEAREST_START = 9999.0f;
// A bug flung off the screen travels this far right
static const double FLY_OFF_DISTANCE = 1900.0;
// A quarter turn, as the original's float widened to a double
static const double QUARTER_TURN = 1.5707963705062866;
// The freeze FreezeAll puts every bug in, in frames, and the daze and shock the rope's touch holds a bug for
static const int FREEZE_FRAMES = 1500;
static const int DAZE_FRAMES_RIPE = 250;
static const int DAZE_FRAMES_TOUCH = 200;
// A held bug that is held for good (the ones behind a bug that was killed in the middle of its column)
static const int HOLD_FOR_GOOD = 100000;
// The commands SetCommand takes (BugImg's BUG_CMD_ start at 100 and are what the body is told)
static const int COMMAND_FREEZE = 1;
static const int COMMAND_DAZE = 2;
// The hold the score of a kill streak is worth: hundreds a kill
static const int STREAK_SCORE = 100;
// How far a hold lengthens, in frames, every frame a bug is inside the hold area
static const int ZAP_HOLD_EXTRA = 10;
// The direction a heading turned back faces, and the "none" ChooseHeading gives when going back is not to be avoided
static const int HEADING_COUNT = 4;
static const int HEADING_NONE = 999;

// 0x425150's grid corner values: a bug with no place stands far outside the grid
static const int NO_PLACE = 0x7fff;

// The position a bug is at on the screen: its body's, or its own when it has none
static ObjBase* GetPositionOwner(BugObj* theBug)
{
	if (theBug->mBody != nullptr)
		return (ObjBase*)theBug->mBody;
	return theBug;
}

// The distance from the bug to the place, as the original's x87 maths takes it (the squared distance is rounded to a
// float before the square root)
static float DistanceToBug(BugObj* theBug, float theX, float theY)
{
	ObjBase* anOwner = GetPositionOwner(theBug);
	double aDiffX = (double)theX - (double)anOwner->mScreenX;
	double aDiffY = (double)theY - (double)anOwner->mScreenY;
	return FloatSqrt((float)(aDiffX * aDiffX + aDiffY * aDiffY));
}

// The post's link in a heading (PipeObj keeps them in the order up, right, down, left)
static PipeObj* GetLink(PipeObj* thePost, int theHeading)
{
	switch (theHeading) {
	case 0:
		return thePost->mUpLink;
	case 1:
		return thePost->mRightLink;
	case 2:
		return thePost->mDownLink;
	case 3:
		return thePost->mLeftLink;
	}
	// port (crash): the original reads the memory beside the links for a heading outside 0 to 3, which none ever is
	return nullptr;
}

// The dust --------------------------------------------------------------------------------------------------------

// 0x41ee60: the puff's emitter: pale grey sparks that ring out from the place, grow, and fade
static void BuildDustEmitter(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 0);
	theEmitter->SetAdditive();

	AnimKey* aRise = new AnimKey();
	aRise->SetDuration(4);
	aRise->mDrawMode = 1;
	aRise->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aRise->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aRise->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_RED, 0.5f, MOD_HOLD);
	aRise->AddValue(STATE_GREEN, 0.5f, MOD_HOLD);
	aRise->AddValue(STATE_BLUE, 0.5f, MOD_HOLD);
	aRise->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aRise->AddValue(STATE_ORBIT_RADIUS, 0.1f, MOD_TOWARD_NEXT);

	AnimKey* aSpread = new AnimKey();
	aSpread->SetDuration(0x32);
	aSpread->mDrawMode = 1;
	aSpread->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aSpread->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aSpread->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSpread->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aSpread->AddValue(STATE_RED, 0.7f, MOD_HOLD);
	aSpread->AddValue(STATE_GREEN, 0.7f, MOD_HOLD);
	aSpread->AddValue(STATE_BLUE, 0.7f, MOD_HOLD);
	aSpread->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aSpread->AddValue(STATE_ORBIT_RADIUS, 2.5f, MOD_TOWARD_NEXT);

	AnimKey* aFade = new AnimKey();
	aFade->SetDuration(10);
	aFade->mDrawMode = 1;
	aFade->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFade->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFade->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_RED, 0.3f, MOD_HOLD);
	aFade->AddValue(STATE_GREEN, 0.1f, MOD_HOLD);
	aFade->AddValue(STATE_BLUE, 0.1f, MOD_HOLD);
	aFade->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aRise, nullptr);
	aTrack->AddKey(aSpread, nullptr);
	aTrack->AddKey(aFade, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(20, 1, 20, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x41ee10: an emitter under the bug, two deep, that bursts where it is told
BugDust::BugDust(ObjBase* theOwner)
{
	mOwner = theOwner;
	mUnused = 0; // port: the original leaves the word as the memory had it
	mEmitter = new Emitter(mOwner, true);
	Emitter::SetDepth(mEmitter, 2.0f);
	BuildDustEmitter(mEmitter);
}

// 0x41f3a0
void BugDust::Burst(float theX, float theY)
{
	mEmitter->Emit(theX, theY, 0.0f);
}

// The bug -----------------------------------------------------------------------------------------------------------

// 0x425150: not on the grid, alive, in no column, with the depth after the last bug's. It joins the level's bugs and the
// list, and the zap area of the bonuses is cleared
BugObj::BugObj()
{
	Player::CountBug();
	mUnused324 = 32.0f;
	mUnused328 = 32.0f;
	mAlive = true;
	mIsGroupLeader = true;
	mDead = false;
	mUnused2A4 = 0;
	mUnused2D5 = false;
	mNextInCell = nullptr;
	mPrevInCell = nullptr;
	mPlaced = false;
	mGroupSize = 0;
	mHalfWidth = 0;
	mHalfHeight = 0;
	mSegmentSteps = 0;
	mHoldFrames = 0;
	mCommand = 0;
	mSetupBody = nullptr;
	mNext = nullptr;
	mPrev = nullptr;
	mPeaTarget = false;
	mWaitCounter = 0;
	mUnused310 = false;
	mFlying = false;
	mPeaHit = false;
	mType = 3;
	mColumn = NO_PLACE;
	mRow = NO_PLACE;
	mListIndex = -1;
	// port: the original leaves these as the memory had them until a kind's Setup sets them
	mBody = nullptr;
	mHead = nullptr;
	mHeading = 0;
	mTimer = 0;
	mFadeTrack = 0;
	mDeathTrack = 0;
	mSpinTrack = 0;
	mHideTrack = 0;
	mSpinKey = nullptr;
	mSpinEndKey = nullptr;
	mHideKey = nullptr;
	mRole = 0;
	SetState(STATE_Z, (float)((double)sNextDepth + 5.0));
	sNextDepth = (float)((double)sNextDepth - 0.005);
	// port (crash): the original reads the level through sLevel without looking whether there is one
	ObjBase* aParent = nullptr;
	if (sLevel != nullptr)
		aParent = static_cast<LevelObj*>(sLevel)->mObjectLayer;
	SetParent(aParent, nullptr);
	mDust = new BugDust(this);
	SetZapArea(ZAP_NONE, 0.0f, 0.0f, 0.0f, 0.0f);
	AddToList(this);
	sAlive++;
}

// 0x425300 (0x419950 deletes): out of its place on the grid, its column and the list; a bug that dies by being deleted, not
// zapped, stops being counted alive here, and the bonuses are told when none is left
BugObj::~BugObj()
{
	LevelObj::UnlinkBug(this);
	delete mDust;
	UnlinkFromColumn();
	RemoveFromList(mListIndex);
	mListIndex = -1;
	if (!mDead) {
		sAlive--;
		if (sAlive == 0)
			Bonus::OnAllBugsDead();
	}
	mTimer = 0;
}

// 0x4253b0: the head of the column (itself when none), the role, and the update level that lets it run while the level
// waits; then the tracks the kind and the base make
void BugObj::Setup(int theRole, int theWaitFrames, BugObj* theHead, BugObj* thePrevious)
{
	(void)theWaitFrames;
	if (theHead == nullptr)
		theHead = this;
	mHead = theHead;
	mRole = theRole;
	mUpdateLevel = 2;
	mPrev = thePrevious;
	if (thePrevious != nullptr)
		thePrevious->mNext = this;
	BuildFadeTrack();
	BuildDeathTrack();
	BuildSpinTrack();
	BuildHideTrack();
}

// 0x4d6be0
void BugObj::StepSegment()
{
}

// 0x5656b0
void BugObj::AvoidRope(int theVertical, float theX, float theY)
{
	(void)theVertical;
	(void)theX;
	(void)theY;
}

// 0x425a70
void BugObj::Vanish()
{
	mAlive = false;
}

// 0x425a80
bool BugObj::SetCommand(int theCommand, int theFrames)
{
	if (!mAlive)
		return false;
	for (BugObj* aBug = mHead; aBug != nullptr; aBug = aBug->mNext) {
		aBug->mCommand = theCommand;
		aBug->mHoldFrames = theFrames;
	}
	return true;
}

// 0x425ac0
int BugObj::TakeCommand()
{
	int aCommand = mCommand;
	if (mAlive) {
		mPeaHit = false;
		mCommand = 0;
		mHoldFrames = 0;
	}
	return aCommand;
}

// 0x425cd0: the bug is at the place, whose middle is 31 times each number from the top left
void BugObj::PlaceAt(int theColumn, int theRow)
{
	mColumn = theColumn;
	mState[STATE_X] = (float)(theColumn * BUG_GRID_CELL);
	mPlaced = true;
	mRow = theRow;
	mState[STATE_Y] = (float)(theRow * BUG_GRID_CELL);
}

// 0x4d6be0
void BugObj::BuildFadeTrack()
{
}

// 0x4d6be0
void BugObj::BuildDeathTrack()
{
}

// 0x425d20: the spin's keys take the bug's place and turn, the last a quarter turn less and 1900 pixels along, and
// the track starts: the bug turns once round, flies off to the right and (event 1) is zapped
void BugObj::FlyOff()
{
	MarkPeaHit();
	mSpinKey->mValue[STATE_X] = mState[STATE_X];
	mFlying = true;
	mSpinEndKey->mValue[STATE_X] = (float)((double)mState[STATE_X] + FLY_OFF_DISTANCE);
	mSpinKey->mValue[STATE_ROTATION] = mState[STATE_ROTATION];
	mSpinEndKey->mValue[STATE_ROTATION] = (float)((double)mState[STATE_ROTATION] - QUARTER_TURN);
	StartTrack(mSpinTrack);
}

// 0x4258c0
void BugObj::SetKind(int theKind, int theHalfWidth, int theHalfHeight)
{
	mKind = theKind;
	mHalfWidth = theHalfWidth;
	mHalfHeight = theHalfHeight;
}

// 0x4258f0: out of the column's list. The bug behind a head becomes the head of what is left, with no head of its own
// (the original sets its head to none and every other bug's to it: a bug that is its own head is not found by
// PickPeaTarget's second pass)
void BugObj::UnlinkFromColumn()
{
	if (mPrev == nullptr) {
		if (mNext != nullptr) {
			mNext->mPrev = nullptr;
			for (BugObj* aBug = mNext; aBug != nullptr; aBug = aBug->mNext)
				aBug->mHead = mNext;
			mNext->mHead = nullptr;
		}
	} else {
		mPrev->mNext = mNext;
	}
	if (mNext != nullptr)
		mNext->mPrev = mPrev;
	mHead = nullptr;
	mNext = nullptr;
	mPrev = nullptr;
}

// 0x4259b0
int BugObj::TurnDirection(int theTarget, int theHeading)
{
	if ((theHeading + 1) % HEADING_COUNT == theTarget)
		return -1;
	if ((theHeading + 3) % HEADING_COUNT == theTarget)
		return 1;
	return 0;
}

// 0x4259f0
int BugObj::ChooseHeading(PipeObj* thePost, bool theRandom, bool theAvoidBack)
{
	int aBack = HEADING_NONE;
	if (theAvoidBack)
		aBack = (mHeading + 2) % HEADING_COUNT;
	if (thePost != nullptr) {
		int aStart;
		if (theRandom)
			aStart = mHeading + (int)(GameRandomNext() % 3) + 1;
		else
			aStart = mHeading + 1;
		for (int aTry = 0; aTry < HEADING_COUNT; aTry++) {
			int aHeading = (aTry + aStart) % HEADING_COUNT;
			if (GetLink(thePost, aHeading) != nullptr && aHeading != aBack)
				return aHeading;
		}
	}
	return aBack;
}

// 0x425880: event 1 of a key of theObj kills the bug, with the death track and the column
void BugObj::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == 1)
		Zap(true, true, false);
}

// 0x425410: one frame. Waiting bugs (a bonus slows them) skip frames. The place and drawn place are kept before the base
// moves the bug along its track: a held bug is put back where it was. The cell it is over is worked out and the bug
// moved in the grid's lists when it changed; the bonuses' area holds or kills it; a cut cell beside it (the bug is in
// the 3 by 3 round its place) kills it. The rope is asked whether it touches the bug's box (see gBugBox)
bool BugObj::Update()
{
	if (mAlive) {
		if (mWaitCounter < sSlowFrames) {
			mWaitCounter++;
			return true;
		}
		mWaitCounter = 0;
	}
	// the original keeps these in the globals it later gives the rope the box in
	gBugBoxX = mState[STATE_X];
	gBugBoxY = mState[STATE_Y];
	gBugBoxWidth = mDrawn[STATE_X];
	gBugBoxHeight = mDrawn[STATE_Y];
	if (!ObjBase::Update())
		return false;
	if (!mAlive)
		return true;
	if (!mFlying) {
		if (mHoldFrames > 0) {
			mState[STATE_X] = gBugBoxX;
			mState[STATE_Y] = gBugBoxY;
			mDrawn[STATE_X] = gBugBoxWidth;
			mDrawn[STATE_Y] = gBugBoxHeight;
			if (mHoldFrames == 1)
				TakeCommand();
			mHoldFrames--;
			mKeyTime = (float)((double)mKeyTime - 1.0);
		}
		// the place it is over: the middle of a cell is 31 pixels on from the one before, and half a cell rounds up
		int aColumn = (int)(((double)mState[STATE_X] + 15.5) / (double)BUG_GRID_CELL);
		int aRow = (int)(((double)mState[STATE_Y] + 15.5) / (double)BUG_GRID_CELL);
		if (aColumn >= GRID_COLUMNS)
			aColumn = GRID_COLUMNS - 1;
		else if (aColumn < 0)
			aColumn = 0;
		if (aRow >= GRID_ROWS)
			aRow = GRID_ROWS - 1;
		else if (aRow < 0)
			aRow = 0;
		if (aColumn != mColumn || aRow != mRow) {
			LevelObj::UnlinkBug(this);
			mColumn = aColumn;
			mRow = aRow;
			LevelObj::LinkBug(this);
		}
		ObjBase* anOwner = GetPositionOwner(this);
		float aPlaceX = anOwner->mScreenX;
		float aPlaceY = anOwner->mScreenY;
		if (sZapMode == ZAP_HOLD) {
			if (sZapLeft <= aPlaceX && aPlaceX <= sZapRight && sZapTop <= aPlaceY && aPlaceY <= sZapBottom) {
				MarkPeaHit();
				mHoldFrames += ZAP_HOLD_EXTRA;
			}
		} else if (sZapMode == ZAP_KILL) {
			if (sZapLeft <= aPlaceX && aPlaceX <= sZapRight && sZapTop <= aPlaceY && aPlaceY <= sZapBottom) {
				sApp->mSoundQueue->Request(SOUND_ZAPPER, 0, false);
				MarkPeaHit();
				Zap(true, true, false);
			}
		}
		if (mRow >= 0 && mRow < GRID_ROWS && mColumn >= 0 && mColumn < GRID_COLUMNS) {
			int aFirstColumn = mColumn - 1;
			int aLastColumn = mColumn + 1;
			int aFirstRow = mRow - 1;
			int aLastRow = mRow + 1;
			if (aFirstColumn < 0)
				aFirstColumn = 0;
			if (aLastColumn >= GRID_COLUMNS)
				aLastColumn = GRID_COLUMNS - 1;
			if (aFirstRow < 0)
				aFirstRow = 0;
			if (aLastRow >= GRID_ROWS)
				aLastRow = GRID_ROWS - 1;
			for (int aCheckRow = aFirstRow; aCheckRow <= aLastRow; aCheckRow++) {
				for (int aCheckColumn = aFirstColumn; aCheckColumn <= aLastColumn; aCheckColumn++) {
					FillObj* aCell = gFillGrid[aCheckColumn][aCheckRow].mCell;
					// port (crash): the original reads the cell unchecked
					if (aCell != nullptr && aCell->mCutShown)
						Zap(true, true, false);
				}
			}
		}
	}
	if (mAlive) {
		// the box the rope is asked about: the body's place and size, or 54 square at the bug's own place
		if (mBody != nullptr) {
			gBugBoxX = mBody->mScreenX;
			gBugBoxY = mBody->mScreenY;
			gBugBoxWidth = mBody->mHitWidth;
			gBugBoxHeight = mBody->mHitHeight;
		} else {
			gBugBoxX = mScreenX;
			gBugBoxY = mScreenY;
			gBugBoxWidth = BOX_SIZE_WITHOUT_BODY;
			gBugBoxHeight = BOX_SIZE_WITHOUT_BODY;
		}
		if (!mFlying) {
			// the original's rope answer 1 (RopeObj::HitBox never gives it) holds the bug like the answer 3 does, for 250 frames
			int aHit = Player::HitBugBox();
			if (aHit == 1) {
				if (mBody == nullptr || !mBody->mFrozen)
					SetCommand(COMMAND_DAZE, DAZE_FRAMES_RIPE);
			} else if (aHit == ROPE_HIT_BURN) {
				MarkPeaHit();
				Zap(true, true, false);
				return true;
			} else if (aHit == ROPE_HIT_TOUCH) {
				// port (crash): a stand-in bug has no body to ask
				if (mBody == nullptr || !mBody->mFrozen)
					SetCommand(COMMAND_DAZE, DAZE_FRAMES_TOUCH);
				return true;
			}
		}
	}
	return true;
}

// 0x425b00: a bug in play dies: the dust, its place on the grid, the count; the death track starts, or the bug is deleted;
// the column dies or is held; the head of a column drops its bananas (when hit by a pea) and pays the streak
bool BugObj::Zap(bool theFollowers, bool theStartTrack, bool thePeaHit)
{
	if (!mAlive)
		return false;
	if (thePeaHit)
		mPeaHit = true;
	if (sAlwaysPeaHit)
		mPeaHit = true;
	WalkThru::OnBugZapped();
	BugImg* aBody = mBody;
	mHoldFrames = 0;
	mAlive = false;
	// port (crash): a stand-in bug has no body to take the dust's place from
	if (aBody != nullptr) {
		mDust->Burst(aBody->mState[STATE_SCALE_X] * aBody->mState[STATE_X],
			aBody->mState[STATE_SCALE_Y] * aBody->mState[STATE_Y]);
	}
	LevelObj::UnlinkBug(this);
	Player::CountKill();
	if (theStartTrack)
		StartTrack(mDeathTrack);
	else
		Delete();
	sAlive--;
	mDead = true;
	if (sAlive == 0)
		Bonus::OnAllBugsDead();
	if (theFollowers) {
		BugObj* aFollower = mNext;
		if (mPrev == nullptr) {
			for (; aFollower != nullptr; aFollower = aFollower->mNext)
				aFollower->Zap(false, true, false);
		} else {
			for (; aFollower != nullptr; aFollower = aFollower->mNext) {
				aFollower->mCommand = COMMAND_DAZE;
				aFollower->mHoldFrames = HOLD_FOR_GOOD;
			}
		}
	}
	if (mHead == this) {
		if (mPeaHit) {
			mPeaHit = false;
			static_cast<LevelObj*>(sLevel)->mBananaBonus->Spawn(mScreenX, mScreenY);
		}
		sKillStreak++;
		LevelObj::ShowBugScore(sKillStreak * STREAK_SCORE, mState[STATE_X], mState[STATE_Y]);
		sApp->mSoundQueue->Request(GetComboSound(), 0, false);
		Player::AddScore(sKillStreak * STREAK_SCORE);
	}
	return true;
}

// 0x425da0
void BugObj::LeaveCopy(ObjBase* theCopy)
{
	MarkPeaHit();
	Zap(false, false, false);
	// port (crash): a stand-in bug has no body to copy
	if (mBody != nullptr) {
		mBody->mAnim->AddRef();
		theCopy->mAnim = mBody->mAnim;
		theCopy->SetState(STATE_ROTATION, (float)((double)mState[STATE_ROTATION] + QUARTER_TURN));
		theCopy->SetState(STATE_SCALE_X, mBody->mState[STATE_SCALE_X]);
		theCopy->SetState(STATE_SCALE_Y, mBody->mState[STATE_SCALE_Y]);
	}
	SetStateInt(STATE_ALPHA, 0);
}

// 0x425e40: the spin: 80 frames turning from 0 to a whole turn, easing in, then a key that ends with event 1 and keeps
// three quarters of a turn
void BugObj::BuildSpinTrack()
{
	mSpinKey = new AnimKey();
	mSpinKey->SetDuration(0x50);
	mSpinKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mSpinKey->AddValue(STATE_ROTATION, 6.2831855f, MOD_TOWARD_NEXT);
	mSpinKey->mEasing = EASE_IN_SQUARE;
	mSpinEndKey = new AnimKey();
	mSpinEndKey->mEventId = 1;
	mSpinEndKey->SetDuration(1);
	mSpinEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mSpinEndKey->AddValue(STATE_ROTATION, 4.712389f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mSpinKey, nullptr);
	aTrack->AddKey(mSpinEndKey, nullptr);
	mSpinTrack = AddTrack(aTrack, nullptr);
}

// 0x425fa0: the hide: one frame, clear, that ends with event 1 (the bug is zapped)
void BugObj::BuildHideTrack()
{
	mHideKey = new AnimKey();
	mHideKey->mEventId = 1;
	mHideKey->SetDuration(1);
	mHideKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mHideKey, mHideKey);
	mHideTrack = AddTrack(aTrack, nullptr);
}

// The list and the bonuses' settings -------------------------------------------------------------------------------

// 0x426950
void BugObj::SetZapArea(int theMode, float theLeft, float theTop, float theRight, float theBottom)
{
	sZapMode = theMode;
	sZapLeft = theLeft;
	sZapTop = theTop;
	sZapRight = theRight;
	sZapBottom = theBottom;
}

// 0x426980
void BugObj::SetSlowFrames(int theFrames)
{
	sSlowFrames = theFrames;
	sUnused2C0 = 0.0f;
}

// 0x426940
void BugObj::SetAlwaysPeaHit(bool theValue)
{
	sAlwaysPeaHit = theValue;
}

// 0x426040
int BugObj::GetComboSound()
{
	int aStep = sKillStreak - 1;
	if (aStep > 8)
		aStep = 8;
	return SOUND_BUGKILL1 + aStep;
}

// 0x426060
void BugObj::ResetKillStreak()
{
	sKillStreak = 0;
}

// 0x426070
void BugObj::ResetCounters()
{
	sNextDepth = 0.0f;
	sSlowFrames = 0;
	sKillStreak = 0;
}

// 0x426090: the biggest box first
static int CompareBugBoxes(const void* theFirst, const void* theSecond)
{
	const BugObj* aFirst = *(BugObj* const*)theFirst;
	const BugObj* aSecond = *(BugObj* const*)theSecond;
	return aSecond->mHalfHeight * aSecond->mHalfWidth - aFirst->mHalfHeight * aFirst->mHalfWidth;
}

// 0x4260c0
void BugObj::SortList()
{
	Platform::CrtQsort(sList, (size_t)sCount, sizeof(BugObj*), CompareBugBoxes);
	for (int aPlace = 0; aPlace < sCount; aPlace++)
		sList[aPlace]->mListIndex = aPlace;
}

// 0x426110: a bug past the 128th is made and not listed
void BugObj::AddToList(BugObj* theBug)
{
	if (sCount < BUG_LIST_MAX) {
		sList[sCount] = theBug;
		theBug->mListIndex = sCount;
		sCount++;
	}
}

// 0x426140: the ones after it move up one place
void BugObj::RemoveFromList(int theIndex)
{
	if (sCount > 0) {
		sCount--;
		// port (crash): the original, given -1 (a bug that was never listed), writes before the list
		for (int aPlace = theIndex < 0 ? sCount : theIndex; aPlace < sCount; aPlace++) {
			sList[aPlace] = sList[aPlace + 1];
			sList[aPlace]->mListIndex--;
		}
	}
}

// 0x426180: the list is forgotten; the bugs themselves are not touched
void BugObj::Reset()
{
	memset(sList, 0, sizeof(sList));
	sCount = 0;
	sAlive = 0;
}

// 0x4261b0 (the original does not look for a negative place)
BugObj* BugObj::Get(int theIndex)
{
	if (sCount > 0 && theIndex >= 0 && theIndex < sCount && theIndex < BUG_LIST_MAX)
		return sList[theIndex];
	return nullptr;
}

// 0x4261e0: each place swaps with a place drawn with the C library's rand, then every bug is numbered again
void BugObj::Shuffle()
{
	int aCount = sCount;
	for (int aPlace = 0; aPlace < aCount; aPlace++) {
		int aOther = CrtRand() % sCount;
		BugObj* aTemp = sList[aPlace];
		sList[aPlace] = sList[aOther];
		sList[aOther] = aTemp;
	}
	for (int aPlace = 0; aPlace < aCount; aPlace++)
		sList[aPlace]->mListIndex = aPlace;
}

// 0x426250
bool BugObj::Overlaps(int theColumn, int theRow, BugObj* theBug)
{
	int aHalfWidth = theBug->mHalfWidth;
	int aHalfHeight = theBug->mHalfHeight;
	int aLeft = theColumn - aHalfWidth;
	int aRight = aHalfWidth + theColumn;
	int aTop = theRow - aHalfHeight;
	int aBottom = aHalfHeight + theRow;
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return false;
		if (aBug != theBug && aBug->mPlaced) {
			if (aLeft <= aBug->mColumn + aBug->mHalfWidth && aRight >= aBug->mColumn - aBug->mHalfWidth &&
				aTop <= aBug->mRow + aBug->mHalfHeight && aBottom >= aBug->mRow - aBug->mHalfHeight)
				return true;
		}
	}
}

// 0x426300
BugObj* BugObj::FindNear(float theX, float theY, float theReach)
{
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return nullptr;
		if (aBug->mAlive) {
			float aDistance = DistanceToBug(aBug, theX, theY);
			if (aDistance <= theReach && !aBug->mPeaTarget)
				return aBug;
		}
	}
}

// 0x4263d0: every bug in reach (alive or not: Zap leaves the dead ones as they are)
bool BugObj::ZapNear(float theX, float theY, float theReach)
{
	bool aZapped = false;
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return aZapped;
		float aDistance = DistanceToBug(aBug, theX, theY);
		if (aDistance <= theReach) {
			aBug->MarkPeaHit();
			aBug->Zap(true, true, false);
			aZapped = true;
		}
	}
}

// 0x4264a0
BugObj* BugObj::FindNearest(float theX, float theY, float* theDistance)
{
	BugObj* aNearest = nullptr;
	float aBest = NEAREST_START;
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			break;
		if (aBug->mAlive) {
			float aDistance = DistanceToBug(aBug, theX, theY);
			if (aDistance <= aBest) {
				aNearest = aBug;
				aBest = aDistance;
			}
		}
	}
	if (aBest >= NEAREST_LIMIT)
		aBest = 0.0f;
	// port (crash): the original writes the distance through a pointer that may be none; its callers give one
	if (theDistance != nullptr)
		*theDistance = aBest;
	return aNearest;
}

// 0x4265a0
BugObj* BugObj::ZapFirstNear(float theX, float theY, float theReach)
{
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return nullptr;
		if (aBug->mAlive) {
			float aDistance = DistanceToBug(aBug, theX, theY);
			if (aDistance <= theReach) {
				aBug->MarkPeaHit();
				aBug->Zap(true, true, false);
				return aBug;
			}
		}
	}
}

// 0x426680: from a random place in the list on round: a bug in play that leads and is not a caterpillar or a centipede and
// has no pea on its way; else, from another random place, a bug in play that leads its column of its own. The bug found
// is marked as a target
BugObj* BugObj::PickPeaTarget()
{
	int aCount = sCount;
	if (aCount < 1)
		return nullptr;
	int aPlace = (int)(GameRandomNext() % (unsigned int)aCount);
	for (int aTry = 0; aTry < aCount; aTry++) {
		BugObj* aBug = Get(aPlace);
		if (aBug != nullptr && aBug->mAlive && aBug->mIsGroupLeader && aBug->mKind != BUG_CENTIPEDE &&
			aBug->mKind != BUG_CATERPILLAR && !aBug->mPeaTarget) {
			aBug->mPeaTarget = true;
			return aBug;
		}
		aPlace = (aPlace + 1) % aCount;
	}
	aPlace = (int)(GameRandomNext() % (unsigned int)aCount);
	for (int aTry = 0; aTry < aCount; aTry++) {
		BugObj* aBug = Get(aPlace);
		if (aBug != nullptr && aBug->mAlive && aBug->mIsGroupLeader && aBug->mHead == aBug && !aBug->mPeaTarget) {
			aBug->mPeaTarget = true;
			return aBug;
		}
		aPlace = (aPlace + 1) % aCount;
	}
	return nullptr;
}

// 0x426750
BugObj* BugObj::PickAbove(float theY)
{
	int aCount = sCount;
	if (aCount < 1)
		return nullptr;
	int aPlace = (int)(GameRandomNext() % (unsigned int)aCount);
	for (int aTry = 0; aTry < aCount; aTry++) {
		BugObj* aBug = Get(aPlace);
		if (aBug->mScreenY < theY && aBug->mAlive && aBug->mIsGroupLeader)
			return aBug;
		aPlace = (aPlace + 1) % aCount;
	}
	return nullptr;
}

// 0x4267c0: the line the rope runs along is the start's column (vertical) or row, and the bugs within 248 of it are told
// the rope's start and the middle of the line
void BugObj::AlertAlongRope(PipeObj* theStart, PipeObj* theEnd, int theVertical)
{
	float aLine;
	float aPlaceX;
	float aPlaceY;
	int anAxis;
	if (theVertical == 0) {
		aLine = theStart->mState[STATE_Y];
		aPlaceY = aLine;
		aPlaceX = (float)(((double)theStart->mState[STATE_X] + (double)theEnd->mState[STATE_X]) * 0.5);
		anAxis = STATE_Y;
	} else {
		aLine = theStart->mState[STATE_X];
		aPlaceX = aLine;
		aPlaceY = (float)(((double)theStart->mState[STATE_Y] + (double)theEnd->mState[STATE_Y]) * 0.5);
		anAxis = STATE_X;
	}
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return;
		float aDifference = (float)((double)aLine - (double)aBug->mState[anAxis]);
		if ((double)FloatAbs(aDifference) <= ALERT_REACH)
			aBug->AvoidRope(theVertical, aPlaceX, aPlaceY);
	}
}

// 0x4268a0
void BugObj::KillAll()
{
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return;
		if (!aBug->IsActive())
			aBug->Delete();
		else
			aBug->Vanish();
	}
}

// 0x4268e0
void BugObj::FreezeAll()
{
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return;
		aBug->SetCommand(COMMAND_FREEZE, FREEZE_FRAMES);
	}
}

// 0x426910
void BugObj::ReleaseAll()
{
	for (int aPlace = 0;; aPlace++) {
		BugObj* aBug = Get(aPlace);
		if (aBug == nullptr)
			return;
		aBug->TakeCommand();
	}
}
