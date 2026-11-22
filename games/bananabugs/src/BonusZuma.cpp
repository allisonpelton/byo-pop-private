#include "BonusZuma.h"

#include "Anim.h"
#include "BugImg.h"
#include "BugObj.h"
#include "FillObj.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusZuma::sZumaAnim = nullptr;
Anim* BonusZuma::sZumaBallAnim = nullptr;

// The key events of the frog's tracks and the ball's (the base's events 1 and 2 are not used by the frog)
enum ZumaEvent
{
	ZUMA_EVENT_LANDED = 5,   // the frog has landed (its drop track's second key): it starts to hunt
	ZUMA_EVENT_FLOWN = 7,    // the ball has flown the whole way (its flight track's last key)
	ZUMA_EVENT_GONE = 8      // the fade out is over: the bonus is put away
};

const int ZUMA_HELD_X = 0x40;                  // where the monkey holds it
const int ZUMA_FROG_Z = 2;                     // the frog's own depth; the ball's is 1
const int ZUMA_BALL_Z = 1;
const int ZUMA_FIRST_COUNTDOWN = 0x78;         // what the countdown is set to by Setup and when the reload's ends (the aiming does not count)
const int ZUMA_RELOAD_FRAMES = 0x40;           // frames the frog waits with the ball back before it turns toward a bug
const int ZUMA_SHOT_FRAMES = 200;              // the ball flies this long when it is shot
const float ZUMA_HIT_REACH = 40.0f;            // the ball kills a bug this near
const double ZUMA_TURN_WHILE_WAITING = 30.0;   // the angle moves 1/31 of the way to the bug's while the ball is coming back
const double ZUMA_TURN_WAITING_DIVISOR = 31.0;
const double ZUMA_TURN_WHILE_AIMING = 29.0;    // ... and 1/30 of the way while it aims
const double ZUMA_TURN_AIMING_DIVISOR = 30.0;
const float ZUMA_RANGE_NEAR = 64.0f;           // a bug is shot at when it is between these (pixels)
const float ZUMA_RANGE_FAR = 216.0f;
const double ZUMA_HALF_PLACE = 15.5;           // the grid: a place's number is its pixel position plus half a place, over the spacing
const double ZUMA_PLACE_SIZE = 31.0;
const float ZUMA_BALL_START_X = 24.0f;         // the ball flies from 24 pixels along the frog's angle to 1920
const float ZUMA_BALL_END_X = 1920.0f;
const int ZUMA_BALL_TRACK_BACK = 0;            // the ball's tracks: back to the frog, and away
const int ZUMA_BALL_TRACK_FLY = 1;

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// The place of the grid a position is over
static int GridPlace(float thePosition)
{
	return (int)(((double)thePosition + ZUMA_HALF_PLACE) / ZUMA_PLACE_SIZE);
}

// 0x412f20: the kind is given by Setup; the fields are as operator new left them in the original, the port clears them
BonusZuma::BonusZuma()
{
	mMode = ZUMA_HELD;
	mBall = nullptr;
	mFrog = nullptr;
	mFlightKey = nullptr;
	mHeldTrack = 0;
	mDropTrack = 0;
	mEndTrack = 0;
	mCountdown = 0;
	mAngle = 0.0f;
	mUnused2fc = 0.0f;
	mUnused300 = 0;
	mEnding = false;
}

// 0x412f70
void BonusZuma::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mCountdown = ZUMA_FIRST_COUNTDOWN;
	BuildFrog();
	BuildTracks();
	BuildBall();
}

// 0x412fa0: while the ball is back the frog turns toward the nearest bug (a thirty-first of the way each frame) and counts
// down; then it aims (a thirtieth of the way) and, when the nearest bug is alive and between 64 and 216 pixels off, snaps
// to face it and shoots. While the ball flies it kills the first bug within 40 pixels, and is taken back. The frog's
// place on the grid is kept; when that place has been cut away the bonus ends
bool BonusZuma::Update()
{
	if (!Bonus::Update())
		return false;
	if (mMode == ZUMA_RELOADING) {
		mCountdown--;
		if (mCountdown < 1) {
			mCountdown = ZUMA_FIRST_COUNTDOWN;
			mMode = ZUMA_AIMING;
		}
		float aDistance;
		BugObj* aBug = BugObj::FindNearest(mScreenX, mScreenY, &aDistance);
		if (aBug != nullptr) {
			float aBehind = mScreenY - aBug->mBody->mScreenY;
			float aAhead = aBug->mBody->mScreenX - mScreenX;
			float aToBug = FloatAtan2(aBehind, aAhead);
			mAngle = (float)(((double)mAngle * ZUMA_TURN_WHILE_WAITING + (double)aToBug) / ZUMA_TURN_WAITING_DIVISOR);
		}
	} else if (mMode == ZUMA_AIMING) {
		float aDistance;
		BugObj* aBug = BugObj::FindNearest(mScreenX, mScreenY, &aDistance);
		if (aBug != nullptr) {
			float aBehind = mScreenY - aBug->mBody->mScreenY;
			float aAhead = aBug->mBody->mScreenX - mScreenX;
			float aToBug = FloatAtan2(aBehind, aAhead);
			mAngle = (float)(((double)mAngle * ZUMA_TURN_WHILE_AIMING + (double)aToBug) / ZUMA_TURN_AIMING_DIVISOR);
			if (aDistance < ZUMA_RANGE_FAR && ZUMA_RANGE_NEAR < aDistance && aBug->mAlive) {
				mAngle = FloatAtan2(mScreenY - aBug->mBody->mScreenY, aBug->mBody->mScreenX - mScreenX);
				Shoot();
				mMode = ZUMA_SHOT;
			}
		}
		int aColumn = GridPlace(ObjBase::mState[STATE_X]);
		int aRow = GridPlace(ObjBase::mState[STATE_Y]);
		mGridColumn = aColumn;
		mGridRow = aRow;
		if (aRow >= 0 && aRow < FILL_GRID_ROWS && aColumn >= 0 && aColumn < FILL_GRID_COLUMNS) {
			FillObj* aCell = gFillGrid[aColumn][aRow].mCell;
			// port: the original does not look at whether the place has a cell, and it starts the fade again every frame
			// the place stays cut, so the fade never ends and the bonus never goes
			if (aCell != nullptr && aCell->mCut && !mEnding)
				End();
		}
	} else if (mMode == ZUMA_SHOT) {
		if (BugObj::ZapFirstNear(mBall->mScreenX, mBall->mScreenY, ZUMA_HIT_REACH) != nullptr) {
			sApp->mSoundQueue->Request(SOUND_ZUMACLICK, 0, false);
			mCountdown = ZUMA_RELOAD_FRAMES;
			mMode = ZUMA_RELOADING;
			mBall->StartTrack(ZUMA_BALL_TRACK_BACK);
			Activate(mBall, false);
		}
	} else {
		return true;
	}
	SetState(STATE_ROTATION, mAngle);
	return true;
}

// 0x4132b0: the frog has landed (event 5): it stands upright and starts to hunt. The ball has flown the whole way (event
// 7): it is taken back, and the frog waits. The fade's end (event 8) puts the bonus away
void BonusZuma::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == ZUMA_EVENT_LANDED) {
		SetState(STATE_ROTATION, 0.0f);
	} else if (anEvent == ZUMA_EVENT_FLOWN) {
		mCountdown = ZUMA_RELOAD_FRAMES;
	} else if (anEvent == ZUMA_EVENT_GONE) {
		Reset();
		return;
	} else {
		Bonus::Notify(theObj);
		return;
	}
	mMode = ZUMA_RELOADING;
	mBall->StartTrack(ZUMA_BALL_TRACK_BACK);
	Activate(mBall, false);
}

// 0x413340: put away: the ball goes back, the frog is let go of and hidden
void BonusZuma::Reset()
{
	ResetBall();
	Player::Drop();
	Deactivate(mFrog);
	Bonus::Reset();
}

// 0x413370: the frog fades out
void BonusZuma::End()
{
	mEnding = true;
	StartTrack(mEndTrack);
}

// 0x412570
bool BonusZuma::EndWhenBugsGone()
{
	return EndOnLastBug();
}

// 0x413380: the monkey takes the frog
void BonusZuma::Start()
{
	strcpy(mName, "ZUMA");
	Bonus::Start();
	mUnused2fc = 0.0f;
	mAngle = 0.0f;
	mUnused300 = 0;
	mMode = ZUMA_HELD;
	mEnding = false;
	sApp->mSoundQueue->Request(SOUND_ZUMA, 0, false);
	SetStateInt(STATE_X, ZUMA_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, 1);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	Player::PickUp(this, 1);
	BecomeHeld(false);
	Activate(this, false);
	Activate(mFrog, false);
	StartTrack(mHeldTrack);
}

// 0x413460: the dropper's Place with Zuma's own sound and tracks
bool BonusZuma::Place()
{
	if (mState != BONUS_HELD)
		return false;
	sApp->mSoundQueue->Request(SOUND_ZUMALETGO, 0, false);
	return PutDown(mDropTrack);
}

// 0x413570: the ball's flight is 200 frames long, its sound plays, and the ball starts to fly
void BonusZuma::Shoot()
{
	mFlightKey->SetDuration(ZUMA_SHOT_FRAMES);
	sApp->mSoundQueue->Request(SOUND_ZUMASHOOT, 0, false);
	mBall->StartTrack(ZUMA_BALL_TRACK_FLY);
}

// 0x4135b0: when there is a ball, it goes back to the frog's place and is hidden
void BonusZuma::ResetBall()
{
	if (mBall != nullptr) {
		mBall->SetStateInt(STATE_X, 0);
		mBall->SetStateInt(STATE_Y, 0);
		Deactivate(mBall);
	}
}

// 0x4135f0: the ball, a child of the bonus (it sits at the bonus's place and turns with it), with two tracks. Back: it
// returns to the frog (10 frames, then a frame that takes it to 24). Away: it flies from 24 to 1920 along the frog's
// angle (the key's length is set by Shoot); the last key's event 7 says it has flown the whole way
void BonusZuma::BuildBall()
{
	mBall = new ObjBase();
	sZumaBallAnim->AddRef();
	mBall->mAnim = sZumaBallAnim;
	mBall->SetParent(this, nullptr);
	mBall->SetEventTarget(this);
	mBall->SetStateInt(STATE_X, 0);
	mBall->SetStateInt(STATE_Y, 0);
	mBall->SetStateInt(STATE_Z, ZUMA_BALL_Z);
	ResetBall();

	AnimKey* aBackKey = new AnimKey();
	aBackKey->SetDuration(10);
	aBackKey->AddValue(STATE_X, 0.0f, MOD_HOLD);
	AnimKey* aBackEndKey = new AnimKey();
	aBackEndKey->SetDuration(1);
	aBackEndKey->AddValue(STATE_X, ZUMA_BALL_START_X, MOD_TOWARD_NEXT);
	AnimTrack* aBackTrack = new AnimTrack();
	aBackTrack->AddKey(aBackKey, nullptr);
	aBackTrack->AddKey(aBackEndKey, aBackEndKey);
	mBall->AddTrack(aBackTrack, nullptr);

	mFlightKey = new AnimKey();
	mFlightKey->SetDuration(300);
	mFlightKey->AddValue(STATE_X, ZUMA_BALL_START_X, MOD_TOWARD_NEXT);
	AnimKey* aFlownKey = new AnimKey();
	aFlownKey->SetDuration(1);
	aFlownKey->mEventId = ZUMA_EVENT_FLOWN;
	aFlownKey->AddValue(STATE_X, ZUMA_BALL_END_X, MOD_HOLD);
	AnimTrack* aFlyTrack = new AnimTrack();
	aFlyTrack->AddKey(mFlightKey, nullptr);
	aFlyTrack->AddKey(aFlownKey, nullptr);
	mBall->AddTrack(aFlyTrack, nullptr);
}

// 0x413840: the frog's picture, a child of the bonus, in front of the ball, on its second cel
void BonusZuma::BuildFrog()
{
	mFrog = new ObjBase();
	sZumaAnim->AddRef();
	mFrog->mAnim = sZumaAnim;
	mFrog->SetParent(this, nullptr);
	mFrog->SetStateInt(STATE_X, 0);
	mFrog->SetStateInt(STATE_Y, 0);
	mFrog->SetStateInt(STATE_Z, ZUMA_FROG_Z);
	mFrog->SetStateInt(STATE_FRAME, 1);
}

// 0x4138d0: the bonus's three tracks (every key but the end's shows the frog; the alpha and scales of a key are added to
// the object's own, so -1 clears and 0 leaves them as they are). Held: it appears growing in and then sways a little
// each way for ever. Dropped: it flies a short way along its angle (the orbit's radius is 1), shrinks, and its second
// key (event 5) says it has landed; its last key holds (it points to itself). The fade out: 20 frames, and a last key of
// one frame that is the event 8
void BonusZuma::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sZumaAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sZumaAnim);
	aSwayKey->SetDuration(0x4b);
	aSwayKey->AddValue(STATE_ALPHA, -0.25f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sZumaAnim);
	aSwayBackKey->SetDuration(0x41);
	aSwayBackKey->AddValue(STATE_ALPHA, -0.25f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aFlyKey = new AnimKey();
	GiveAnim(aFlyKey, sZumaAnim);
	aFlyKey->SetDuration(0x28);
	aFlyKey->AddValue(STATE_ALPHA, -0.25f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlyKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlyKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aLandKey = new AnimKey();
	aLandKey->mEventId = ZUMA_EVENT_LANDED;
	GiveAnim(aLandKey, sZumaAnim);
	aLandKey->SetDuration(5);
	aLandKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_SCALE_X, 0.02f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_SCALE_Y, 0.02f, MOD_TOWARD_NEXT);
	AnimKey* aRestKey = new AnimKey();
	GiveAnim(aRestKey, sZumaAnim);
	aRestKey->SetDuration(10);
	aRestKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aRestKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aRestKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aDropTrack = new AnimTrack();
	aDropTrack->AddKey(aFlyKey, nullptr);
	aDropTrack->AddKey(aLandKey, nullptr);
	aDropTrack->AddKey(aRestKey, aRestKey);
	mDropTrack = AddTrack(aDropTrack, nullptr);

	AnimKey* aFadeKey = new AnimKey();
	aFadeKey->SetDuration(0x14);
	aFadeKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->mEventId = ZUMA_EVENT_GONE;
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimTrack* anEndTrack = new AnimTrack();
	anEndTrack->AddKey(aFadeKey, nullptr);
	anEndTrack->AddKey(aGoneKey, nullptr);
	mEndTrack = AddTrack(anEndTrack, nullptr);
}
