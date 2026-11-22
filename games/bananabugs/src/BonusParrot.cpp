#include "BonusParrot.h"

#include "Anim.h"
#include "BugObj.h"
#include "FillObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "PipeObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusParrot::sFlyAnim = nullptr;
Anim* BonusParrot::sWalkAnim = nullptr;
Anim* BonusParrot::sPeckAnim = nullptr;

// The key events of the parrot's tracks (the base's events 1 and 2 are not used by the parrot; the first key of the
// flight in has the event 4, which nothing answers; 6 and 7 are not used by any key)
enum ParrotEvent
{
	PARROT_EVENT_LANDED = 5,       // the flight in is over: the parrot is on the wall
	PARROT_EVENT_UNUSED = 7,       // answered by doing nothing
	PARROT_EVENT_WALKED = 8,       // a walk along the wall is over
	PARROT_EVENT_PECK_HIT = 9,     // the beak lands: the bug dies
	PARROT_EVENT_PECK_DONE = 10,   // the peck is over: the walk starts again
	PARROT_EVENT_FLOWN = 11        // the flight off the screen is over: the bonus is put away
};

// Where the held bonus rides, relative to its holder (design pixels), and the depth it is drawn at when it is put down
const int PARROT_HELD_X = 64;
const int PARROT_Z = 5;

// How many times the monkey may put it down
const int PARROT_USES = 1;

// The grid (FILL_CELL_SPACING pixels a place): a place's number is its pixel position plus half a place, over the
// spacing; the parrot looks for a bug in the places from 3 above to 2 below, and from 2 behind it to 1 ahead (when it
// faces left) or 1 behind to 2 ahead (facing right)
const double PARROT_HALF_PLACE = 15.5;
const double PARROT_PLACE_SIZE = 31.0;
const int PARROT_LOOK_ABOVE = 3;
const int PARROT_LOOK_BELOW = 2;
const int PARROT_LOOK_BEHIND_LEFT = 2;
const int PARROT_LOOK_AHEAD_LEFT = 1;
const int PARROT_LOOK_BEHIND_RIGHT = 1;
const int PARROT_LOOK_AHEAD_RIGHT = 2;

// What a piece of wall that the parrot may land on is (PipeObj::mKind: a wall that runs across), and a post (a corner
// of the walls, where a wall ends)
const int PARROT_WALL_KIND = FILL_KIND_WALL_ACROSS;
const int PARROT_POST_KIND = FILL_KIND_CORNER;

// The flights: they take a quarter frame a pixel and 32 frames more; a walk takes two frames a pixel; the parrot leaves
// to 300 left or 2000 right of the screen, 64 higher than it was
const double PARROT_FLY_FRAMES_PER_PIXEL = 0.25;
const double PARROT_FLY_FRAMES_MORE = 32.0;
const int PARROT_WALK_FRAMES_PER_PIXEL = 2;
const float PARROT_LEFT_EXIT_X = -300.0f;
const float PARROT_RIGHT_EXIT_X = 2000.0f;
const double PARROT_EXIT_RISE = 64.0;
const float PARROT_NO_PIPE_DISTANCE = 9999.0f; // the first distance a piece of wall has to beat

// The tracks' keys: the lengths and the cels' counters (a cel every 50 frames a hundredth: the key's rate and amount, 11
// cels of the flight, 20 of the walk, 17 of the peck)
const int PARROT_APPEAR_FRAMES = 5;
const int PARROT_SWAY_FRAMES = 0x16;
const float PARROT_SWAY_CELS = 10.0f;
const float PARROT_SWAY_TURN = 0.0349065848f;
const int PARROT_FLY_IN_FRAMES = 0x14;
const int PARROT_FLY_AWAY_FRAMES = 300;
const int PARROT_PECK_FRAMES = 0x11;
const float PARROT_PECK_LAST_CEL = 16.0f;
const float PARROT_OFFSET_Y = -42.0f;         // the parrot is drawn this far above its place while it flies in or walks
const float PARROT_FLY_CEL_RATE = 50.0f;
const float PARROT_FLY_CEL_COUNT = 11.0f;
const float PARROT_WALK_CEL_RATE = 33.0f;
const float PARROT_WALK_CEL_COUNT = 20.0f;

// 0x40eff0: the kind is given by Setup; the fields are as operator new left them in the original, the port clears them
BonusParrot::BonusParrot()
{
	mEnded = false;
	mHasTarget = false;
	mLanded = false;
	mPipe = nullptr;
	mNextPipe = nullptr;
	mTarget = nullptr;
	mFlyInKey = nullptr;
	mFlyInEndKey = nullptr;
	mWalkKey = nullptr;
	mWalkNextKey = nullptr;
	mPeckKey = nullptr;
	mPeckHitKey = nullptr;
	mPeckEndKey = nullptr;
	mFlyAwayKey = nullptr;
	mFlyAwayEndKey = nullptr;
	mHeldTrack = 0;
	mFlyInTrack = 0;
	mWalkTrack = 0;
	mPeckTrack = 0;
	mFlyAwayTrack = 0;
	mColumn = 0;
	mRow = 0;
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// The place of the grid a position is over
static int GridPlace(float thePosition)
{
	return (int)(((double)thePosition + PARROT_HALF_PLACE) / PARROT_PLACE_SIZE);
}

// 0x40f030
void BonusParrot::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mLanded = false;
	mHasTarget = false;
	mTarget = nullptr;
	mEnded = false;
	BuildTracks();
}

// 0x40f060: the parrot stands upright; its place of the grid is kept; once it has landed (and until the bonus ends) it
// looks for a bug, and a bug it has found is pecked
bool BonusParrot::Update()
{
	if (!Bonus::Update())
		return false;
	ObjBase::mState[STATE_ROTATION] = 0.0f;
	mDrawn[STATE_ROTATION] = 0.0f;
	mColumn = GridPlace(ObjBase::mState[STATE_X]);
	mRow = GridPlace(ObjBase::mState[STATE_Y]);
	if (!mEnded && mLanded) {
		FindTarget();
		if (mHasTarget) {
			mHasTarget = false;
			SetStateInt(STATE_FRAME, 0);
			StartTrack(mPeckTrack);
		}
	}
	return true;
}

// 0x40f100: the flight in is over (event 5): the parrot stands on the wall, faces along it and walks to the end ahead
// of it. A walk's end (event 8) takes it on to the end of the wall. The beak's landing (event 9) kills the bug, and
// the peck's end (event 10) sets it walking again from where it is. The flight off the screen (event 11) puts the
// bonus away
void BonusParrot::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case PARROT_EVENT_LANDED:
		mLanded = true;
		SetStateInt(STATE_FRAME, 0);
		sApp->mSoundQueue->Request(SOUND_PARROTCAW, 0, false);
		mTarget = nullptr;
		if (0.0f <= mBase[STATE_SCALE_X]) {
			SetStateInt(STATE_SCALE_X, -1);
			mNextPipe = mPipe->mLeftLink;
		} else {
			SetStateInt(STATE_SCALE_X, 1);
			mNextPipe = mPipe->mRightLink;
		}
		mWalkKey->mValue[STATE_X] = mPipe->mState[STATE_X];
		mWalkKey->mValue[STATE_Y] = mPipe->mState[STATE_Y];
		mWalkNextKey->mValue[STATE_X] = mNextPipe->mState[STATE_X];
		mWalkNextKey->mValue[STATE_Y] = mNextPipe->mState[STATE_Y];
		StartTrack(mWalkTrack);
		mCurrentKey->SetDuration((int)FloatAbs(mPipe->mState[STATE_X] - mNextPipe->mState[STATE_X]) * PARROT_WALK_FRAMES_PER_PIXEL);
		return;
	case PARROT_EVENT_UNUSED:
		return;
	case PARROT_EVENT_WALKED:
		mPipe = mNextPipe;
		mTarget = nullptr;
		sApp->mSoundQueue->Request(SOUND_PARROTCAW, 0, false);
		// The original walks on to the end of the wall; at the end (a post, which has no link ahead) it reads the link of
		// nothing and the game crashes: the port ends the bonus there, and the parrot flies off (port)
		if (0.0f <= mBase[STATE_SCALE_X]) {
			SetStateInt(STATE_SCALE_X, -1);
			if (mNextPipe->mLeftLink == nullptr) {
				End();
				return;
			}
			mNextPipe = mNextPipe->mLeftLink;
			while (mNextPipe->mLeftLink != nullptr)
				mNextPipe = mNextPipe->mLeftLink;
		} else {
			SetStateInt(STATE_SCALE_X, 1);
			if (mNextPipe->mRightLink == nullptr) {
				End();
				return;
			}
			mNextPipe = mNextPipe->mRightLink;
			while (mNextPipe->mRightLink != nullptr)
				mNextPipe = mNextPipe->mRightLink;
		}
		mCurrentKey->mValue[STATE_X] = mPipe->mState[STATE_X];
		mCurrentKey->mValue[STATE_Y] = mPipe->mState[STATE_Y];
		mCurrentKey->mNext->mValue[STATE_X] = mNextPipe->mState[STATE_X];
		mCurrentKey->mNext->mValue[STATE_Y] = mNextPipe->mState[STATE_Y];
		mCurrentKey->SetDuration((int)FloatAbs(mPipe->mState[STATE_X] - mNextPipe->mState[STATE_X]) * PARROT_WALK_FRAMES_PER_PIXEL);
		return;
	case PARROT_EVENT_PECK_HIT:
		if (mTarget != nullptr && mTarget->mAlive) {
			mTarget->MarkPeaHit();
			mTarget->Zap(true, true, false);
			mTarget = nullptr;
			sApp->mSoundQueue->Request(SOUND_PARROTHIT, 0, false);
		}
		return;
	case PARROT_EVENT_PECK_DONE: {
		FillGridCell& aCell = gFillGrid[mColumn][mRow];
		if (aCell.mPipe->mKind != PARROT_WALL_KIND) {
			mPipe = aCell.mPipe;
			if (0.0f <= mBase[STATE_SCALE_X])
				mNextPipe = aCell.mPipe->mLeftLink;
			else
				mNextPipe = aCell.mPipe->mRightLink;
		} else {
			mPipe = mNextPipe;
			if (mBase[STATE_SCALE_X] <= 0.0f)
				mNextPipe = aCell.mPipe->mLeftLink;
			else
				mNextPipe = aCell.mPipe->mRightLink;
		}
		// A place with no wall has no link to walk to: the original reads the link of nothing and crashes; the port ends
		// the bonus there (port)
		if (mNextPipe == nullptr) {
			End();
			return;
		}
		mWalkKey->mValue[STATE_X] = ObjBase::mState[STATE_X];
		mWalkKey->mValue[STATE_Y] = ObjBase::mState[STATE_Y];
		mWalkNextKey->mValue[STATE_X] = mNextPipe->mState[STATE_X];
		mWalkNextKey->mValue[STATE_Y] = mNextPipe->mState[STATE_Y];
		mWalkKey->SetDuration((int)FloatAbs(ObjBase::mState[STATE_X] - mNextPipe->mState[STATE_X]) * PARROT_WALK_FRAMES_PER_PIXEL);
		StartTrack(mWalkTrack);
		mTarget = nullptr;
		return;
	}
	case PARROT_EVENT_FLOWN:
		Reset();
		return;
	default:
		Bonus::Notify(theObj);
		return;
	}
}

// 0x40f550
void BonusParrot::Reset()
{
	Bonus::Reset();
	mLanded = false;
	mHasTarget = false;
	mTarget = nullptr;
	mEnded = false;
}

// 0x40f580: the first End: the parrot flies off the screen, to the left when it faces left and to the right otherwise,
// a little higher than it is
void BonusParrot::End()
{
	if (mEnded)
		return;
	mEnded = true;
	float anExitX = (mBase[STATE_SCALE_X] <= 0.0f) ? PARROT_LEFT_EXIT_X : PARROT_RIGHT_EXIT_X;
	float anExitY = (float)((double)ObjBase::mState[STATE_Y] - PARROT_EXIT_RISE);
	sApp->mSoundQueue->Request(SOUND_PARROTHIT, 0, false);
	mFlyAwayKey->mValue[STATE_X] = ObjBase::mState[STATE_X];
	mFlyAwayKey->mValue[STATE_Y] = ObjBase::mState[STATE_Y];
	mFlyAwayEndKey->mValue[STATE_X] = anExitX;
	mFlyAwayEndKey->mValue[STATE_Y] = anExitY;
	StartTrack(mFlyAwayTrack);
}

// 0x412570
bool BonusParrot::EndWhenBugsGone()
{
	return EndOnLastBug();
}

// 0x40f690: the monkey takes the parrot
void BonusParrot::Start()
{
	strcpy(mName, "PARROT HELPER");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, PARROT_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	Player::PickUp(this, PARROT_USES);
	BecomeHeld(false);
	Activate(this, false);
	StartTrack(mHeldTrack);
}

// 0x40f630: the held parrot faces left when it has been turned to the way that is 2 or 3, and right otherwise
bool BonusParrot::Rotate()
{
	if (!Bonus::Rotate())
		return false;
	if (mRotation == 1)
		SetStateInt(STATE_SCALE_X, 1);
	else if (mRotation == 2)
		SetStateInt(STATE_SCALE_X, -1);
	else if (mRotation == 3)
		SetStateInt(STATE_SCALE_X, -1);
	else
		SetStateInt(STATE_SCALE_X, 1);
	return true;
}

// 0x40f760: held only (state 2): the parrot goes to the level's object layer without moving on the screen, its place
// is kept as the start of its flight, and the nearest piece of wall that runs across is where it flies to (it takes a
// quarter frame a pixel and 32 frames more); it faces the way it flies. With no wall to land on the bonus is not
// started (the original has put it down already)
bool BonusParrot::Place()
{
	if (mState != BONUS_HELD)
		return false;
	sApp->mSoundQueue->Request(SOUND_PARROTCAW, 0, false);
	mState = BONUS_PLACED;
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	float aX = (float)(((double)mDrawn[STATE_X] - (double)mParent->mDrawn[STATE_X]) * (double)sApp->mWindowToDesignX);
	float aY = (float)(((double)mDrawn[STATE_Y] - (double)mParent->mDrawn[STATE_Y]) * (double)sApp->mWindowToDesignY);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, PARROT_Z);
	FindNearestPipe(aX, aY);
	if (mPipe == nullptr)
		return false;
	mFlyInKey->mValue[STATE_X] = aX;
	mFlyInKey->mValue[STATE_Y] = aY;
	double aDeltaY = (double)aY - (double)mPipe->mState[STATE_Y];
	double aDeltaX = (double)aX - (double)mPipe->mState[STATE_X];
	float aSquares = (float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY);
	mFlyInKey->SetDuration((int)((double)FloatSqrt(aSquares) * PARROT_FLY_FRAMES_PER_PIXEL + PARROT_FLY_FRAMES_MORE));
	mFlyInEndKey->mValue[STATE_X] = mPipe->mState[STATE_X];
	mFlyInEndKey->mValue[STATE_Y] = mPipe->mState[STATE_Y];
	if (mFlyInKey->mValue[STATE_X] <= mFlyInEndKey->mValue[STATE_X])
		SetStateInt(STATE_SCALE_X, 1);
	else
		SetStateInt(STATE_SCALE_X, -1);
	ComputeDrawn();
	StartTrack(mFlyInTrack);
	return true;
}

// 0x40f900: mPipe is the piece of wall that runs across nearest to the place, the first of them if two are as near (the
// grid is searched along its rows); the distance is returned
float BonusParrot::FindNearestPipe(float theX, float theY)
{
	float aNearest = PARROT_NO_PIPE_DISTANCE;
	PipeObj* aNearestPipe = nullptr;
	for (int aRow = 0; aRow < FILL_GRID_ROWS; aRow++) {
		for (int aColumn = 0; aColumn < FILL_GRID_COLUMNS; aColumn++) {
			PipeObj* aPipe = gFillGrid[aColumn][aRow].mPipe;
			if (aPipe->mKind == PARROT_WALL_KIND) {
				float aDeltaX = aPipe->mState[STATE_X] - theX;
				float aDeltaY = aPipe->mState[STATE_Y] - theY;
				float aDistance = FloatSqrt((float)((double)aDeltaY * aDeltaY + (double)aDeltaX * aDeltaX));
				if (aDistance < aNearest) {
					aNearest = aDistance;
					aNearestPipe = aPipe;
				}
			}
		}
	}
	mPipe = aNearestPipe;
	return aNearest;
}

// 0x40f9d0: once the parrot has landed and has no bug in its sights, the places of the grid from 3 above to 2 below it
// and (facing left) from 2 behind to 1 ahead, or (facing right) from 1 behind to 2 ahead, are searched, row by row from
// the top, for the first bug in play; it is not looked for while the parrot stands at a post
void BonusParrot::FindTarget()
{
	if (!mLanded || mTarget != nullptr)
		return;
	int aColumn = GridPlace(ObjBase::mState[STATE_X]);
	int aRow = GridPlace(ObjBase::mState[STATE_Y]);
	mHasTarget = false;
	int aTop = aRow - PARROT_LOOK_ABOVE;
	int aLeft;
	int aRight;
	if (mBase[STATE_SCALE_X] <= 0.0f) {
		aLeft = aColumn - PARROT_LOOK_BEHIND_LEFT;
		aRight = aColumn + PARROT_LOOK_AHEAD_LEFT;
	} else {
		aLeft = aColumn - PARROT_LOOK_BEHIND_RIGHT;
		aRight = aColumn + PARROT_LOOK_AHEAD_RIGHT;
	}
	int aBottom = aRow + PARROT_LOOK_BELOW;
	if (aLeft < 0)
		aLeft = 0;
	if (aRight >= FILL_GRID_COLUMNS)
		aRight = FILL_GRID_COLUMNS - 1;
	if (aTop < 0)
		aTop = 0;
	if (aBottom >= FILL_GRID_ROWS)
		aBottom = FILL_GRID_ROWS - 1;
	if (gFillGrid[mColumn][mRow].mPipe->mKind == PARROT_POST_KIND)
		return;
	if (aTop > aBottom)
		return;
	for (int aScanRow = aTop;; ) {
		if (aLeft <= aRight) {
			int aScanColumn = aLeft;
			do {
				for (BugObj* aBug = gFillGrid[aScanColumn][aScanRow].mFirstBug; aBug != nullptr; aBug = aBug->mNextInCell) {
					if (aBug->mAlive) {
						mTarget = aBug;
						mHasTarget = true;
						return;
					}
				}
				if (mHasTarget)
					return;
				aScanColumn++;
			} while (aScanColumn <= aRight);
		}
		if (mHasTarget)
			return;
		aScanRow++;
		if (aBottom < aScanRow)
			return;
	}
}

// 0x40fb10: the parrot's five tracks (every key shows the parrot's picture for what it is doing). Held: it appears
// growing in and then sways a little each way for ever. The flight in: from where it was put to the wall (Place sets the
// ends), the picture 50 frames a hundredth through 11 cels, drawn 42 above its place; the last key's event 5 is the
// landing. The walk: from one end to the other along the wall, 33 frames a hundredth through 20 cels; each key ends in
// event 8 and is followed by the other (the second's next is the first). The peck: 17 cels; the beak lands at the
// second key (event 9) and the third's event 10 ends it. The flight off the screen: 300 frames at most (End sets the
// ends), its last key's event 11 puts the bonus away
void BonusParrot::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sFlyAnim);
	anAppearKey->SetDuration(PARROT_APPEAR_FRAMES);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sFlyAnim);
	aSwayKey->SetDuration(PARROT_SWAY_FRAMES);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_FRAME, PARROT_SWAY_CELS, MOD_TOWARD_NEXT);
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sFlyAnim);
	aSwayBackKey->SetDuration(PARROT_SWAY_FRAMES);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_ROTATION, PARROT_SWAY_TURN, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	mFlyInKey = new AnimKey();
	GiveAnim(mFlyInKey, sFlyAnim);
	mFlyInKey->mEventId = 4;
	mFlyInKey->SetDuration(PARROT_FLY_IN_FRAMES);
	mFlyInKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mFlyInKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mFlyInKey->AddValue(STATE_OFFSET_Y, 0.0f, MOD_HOLD);
	mFlyInKey->mEasing = EASE_IN_SQUARE;
	mFlyInKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, PARROT_FLY_CEL_RATE, PARROT_FLY_CEL_COUNT, 0.0f, nullptr);
	mFlyInEndKey = new AnimKey();
	GiveAnim(mFlyInEndKey, sFlyAnim);
	mFlyInEndKey->mEventId = PARROT_EVENT_LANDED;
	mFlyInEndKey->SetDuration(0);
	mFlyInEndKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	mFlyInEndKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	mFlyInEndKey->AddValue(STATE_OFFSET_Y, PARROT_OFFSET_Y, MOD_HOLD);
	mFlyInEndKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, PARROT_FLY_CEL_RATE, PARROT_FLY_CEL_COUNT, 0.0f, nullptr);
	AnimTrack* aFlyInTrack = new AnimTrack();
	aFlyInTrack->AddKey(mFlyInKey, nullptr);
	aFlyInTrack->AddKey(mFlyInEndKey, nullptr);
	mFlyInTrack = AddTrack(aFlyInTrack, nullptr);

	mWalkKey = new AnimKey();
	GiveAnim(mWalkKey, sWalkAnim);
	mWalkKey->mEventId = PARROT_EVENT_WALKED;
	mWalkKey->SetDuration(PARROT_FLY_IN_FRAMES);
	mWalkKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mWalkKey->AddValue(STATE_OFFSET_Y, PARROT_OFFSET_Y, MOD_HOLD);
	mWalkKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, PARROT_WALK_CEL_RATE, PARROT_WALK_CEL_COUNT, 0.0f, nullptr);
	mWalkNextKey = new AnimKey();
	GiveAnim(mWalkNextKey, sWalkAnim);
	mWalkNextKey->mEventId = PARROT_EVENT_WALKED;
	mWalkNextKey->SetDuration(PARROT_FLY_IN_FRAMES);
	mWalkNextKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mWalkNextKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mWalkNextKey->AddValue(STATE_OFFSET_Y, PARROT_OFFSET_Y, MOD_HOLD);
	mWalkNextKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, PARROT_WALK_CEL_RATE, PARROT_WALK_CEL_COUNT, 0.0f, nullptr);
	AnimTrack* aWalkTrack = new AnimTrack();
	aWalkTrack->AddKey(mWalkKey, nullptr);
	aWalkTrack->AddKey(mWalkNextKey, mWalkKey);
	mWalkTrack = AddTrack(aWalkTrack, nullptr);

	mPeckKey = new AnimKey();
	GiveAnim(mPeckKey, sPeckAnim);
	mPeckKey->SetDuration(PARROT_PECK_FRAMES);
	mPeckKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	mPeckHitKey = new AnimKey();
	GiveAnim(mPeckHitKey, sPeckAnim);
	mPeckHitKey->mEventId = PARROT_EVENT_PECK_HIT;
	mPeckHitKey->SetDuration(PARROT_PECK_FRAMES);
	mPeckHitKey->AddValue(STATE_FRAME, PARROT_PECK_LAST_CEL, MOD_TOWARD_NEXT);
	mPeckEndKey = new AnimKey();
	GiveAnim(mPeckEndKey, sPeckAnim);
	mPeckEndKey->mEventId = PARROT_EVENT_PECK_DONE;
	mPeckEndKey->SetDuration(0);
	mPeckEndKey->AddValue(STATE_FRAME, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aPeckTrack = new AnimTrack();
	aPeckTrack->AddKey(mPeckKey, nullptr);
	aPeckTrack->AddKey(mPeckHitKey, nullptr);
	aPeckTrack->AddKey(mPeckEndKey, nullptr);
	mPeckTrack = AddTrack(aPeckTrack, nullptr);

	mFlyAwayKey = new AnimKey();
	GiveAnim(mFlyAwayKey, sFlyAnim);
	mFlyAwayKey->SetDuration(PARROT_FLY_AWAY_FRAMES);
	mFlyAwayKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mFlyAwayKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mFlyAwayKey->mEasing = EASE_IN_SQUARE;
	mFlyAwayKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, PARROT_FLY_CEL_RATE, PARROT_FLY_CEL_COUNT, 0.0f, nullptr);
	mFlyAwayEndKey = new AnimKey();
	GiveAnim(mFlyAwayEndKey, sFlyAnim);
	mFlyAwayEndKey->mEventId = PARROT_EVENT_FLOWN;
	mFlyAwayEndKey->SetDuration(0);
	mFlyAwayEndKey->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	mFlyAwayEndKey->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mFlyAwayEndKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, PARROT_FLY_CEL_RATE, PARROT_FLY_CEL_COUNT, 0.0f, nullptr);
	AnimTrack* aFlyAwayTrack = new AnimTrack();
	aFlyAwayTrack->AddKey(mFlyAwayKey, nullptr);
	aFlyAwayTrack->AddKey(mFlyAwayEndKey, nullptr);
	mFlyAwayTrack = AddTrack(aFlyAwayTrack, nullptr);
}
