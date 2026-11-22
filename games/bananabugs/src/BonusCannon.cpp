#include "BonusCannon.h"

#include "Anim.h"
#include "BugObj.h"
#include "Emitter.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusCannon::sCannonAnim = nullptr;
Anim* BonusCannon::sCannonBallAnim = nullptr;

// The key events of the cannon's tracks (the ball's last key raises BONUS_EVENT_END)
enum CannonEvent
{
	CANNON_EVENT_FIRE = 5,   // the cannon has recoiled: the ball goes
	CANNON_EVENT_DONE = 6    // the cannon has finished (its look is put back)
};

// Where the held bonus rides, relative to its holder (design pixels), and the depth and offset of the cannon
const int CANNON_HELD_X = 64;
const int CANNON_Z = 17;
const float CANNON_OFFSET_Y = -8.0f;

// How long the bonus lasts after End, and the reach of the ball
const int CANNON_END_FRAMES = 400;
const float CANNON_BLAST_REACH = 32.0f;

// The angle of the object for each way the cannon may lie (Bonus::mRotation 0 to 3), in degrees (0x6009c4)
const int CANNON_ROTATION_DEGREES[4] = {90, 0, 270, 180};

// The emitters the bonus makes in place of the base's: a short swell of 8 and the gold burst, at depth 0.5
const float CANNON_EMITTER_DEPTH = 0.5f;
const int CANNON_SWELL_COUNT = 8;

// The effect of the shot: six swells along the barrel
const float CANNON_MUZZLE_X[6] = {32.0f, 38.0f, 48.0f, 24.0f, 38.0f, 32.0f};
const float CANNON_MUZZLE_Y[6] = {0.0f, -8.0f, -16.0f, -16.0f, -8.0f, 0.0f};

// The tracks of the cannon's picture
const int CANNON_TRACK_HIDDEN = 0;
const int CANNON_TRACK_RECOIL = 1;

// 0x407170: the fields are as operator new left them in the original; the port clears them
BonusCannon::BonusCannon()
{
	mCannon = nullptr;
	mEnded = false;
	mEndCountdown = 0;
	mBallTrack = 0;
	mFireTrack = 0;
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x4071b0 (ObjBase::Init is 0x450dd0): put back as it was made, with nothing held
void BonusCannon::Init()
{
	ObjBase::Init();
	mEndCountdown = 0;
	mEnded = false;
	if (Player::sCurrent != nullptr)
		Player::Drop();
	ResetLook();
	Deactivate(this);
}

// 0x4071f0: the bonus is back to its held place (at 64, 0, its scale nothing, its cel the kind's) and the cannon, if it
// has been made, is hidden at its centre, a child of the bonus
void BonusCannon::ResetLook()
{
	SetStateInt(STATE_X, CANNON_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, mKind);
	if (mCannon != nullptr) {
		mCannon->SetParent(this, nullptr);
		mCannon->SetStateInt(STATE_X, 0);
		mCannon->SetStateInt(STATE_Y, 0);
		mCannon->SetState(STATE_OFFSET_Y, CANNON_OFFSET_Y);
		mCannon->SetStateInt(STATE_ROTATION, 0);
		mCannon->SetStateInt(STATE_ORBIT_ANGLE, 0);
		mCannon->SetStateInt(STATE_Z, CANNON_Z);
		mCannon->SetStateInt(STATE_SCALE_X, 1);
		mCannon->SetStateInt(STATE_SCALE_Y, 1);
		mCannon->SetStateInt(STATE_ALPHA, 1);
		mCannon->SetStateInt(STATE_FRAME, 0);
		mCannon->Deactivate(mCannon);
	}
}

// 0x407300: the message, the ball's tracks and the cannon. The emitters are made again (the original lets go of the
// ones Bonus::Setup made, which are never freed)
void BonusCannon::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mEndCountdown = 0;
	mEnded = false;
	strcpy(mName, "CLICK TO FIRE CANNON");
	BuildBall();
	BuildCannon();
	mEmitters = new EmitterSet(this);
	mEmitters->MakeShortSwell(CANNON_EMITTER_DEPTH, CANNON_SWELL_COUNT);
	mEmitters->MakeGoldBurst(CANNON_EMITTER_DEPTH);
}

// 0x4073b0: the ball in flight (state 4) kills the first bug it meets each frame; an ended bonus (state 5) counts down
// and goes
bool BonusCannon::Update()
{
	if (!Bonus::Update())
		return false;
	if (mState == BONUS_PLACED) {
		if (BugObj::ZapFirstNear(mScreenX, mScreenY, CANNON_BLAST_REACH) != nullptr)
			mEmitters->BurstGoldBurst(0.0f, 0.0f);
	} else if (mState == BONUS_ENDING) {
		mEndCountdown = mEndCountdown - 1;
		if (mEndCountdown == 1) {
			Reset();
			return true;
		}
	}
	return true;
}

// 0x407440: the recoil fires the ball, the cannon's last key puts its look back
void BonusCannon::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr) {
		int anEvent = theObj->mCurrentKey->mEventId;
		if (anEvent == CANNON_EVENT_FIRE)
			Fire();
		else if (anEvent == CANNON_EVENT_DONE)
			ResetLook();
		else
			Bonus::Notify(theObj);
	}
}

// 0x407480
void BonusCannon::Reset()
{
	ResetLook();
	Player::Drop();
	Bonus::Reset();
}

// 0x4074a0: the first End (of an active bonus) starts the count of its last frames
void BonusCannon::End()
{
	if (IsActive() && !mEnded) {
		mEnded = true;
		mState = BONUS_ENDING;
		mEndCountdown = CANNON_END_FRAMES;
	}
}

// 0x4074d0: the monkey takes the bonus (held, its cannon waiting hidden and the ball resting)
void BonusCannon::Start()
{
	Bonus::Start();
	SetStateInt(STATE_X, CANNON_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	Player::PickUp(this, 1);
	BecomeHeld(true);
	Activate(this, false);
	Activate(mCannon, false);
	StartTrack(mBallTrack);
	mCannon->StartTrack(CANNON_TRACK_HIDDEN);
}

// 0x407570 (and 0x407586): the cannon is put down: the cannon and the ball go to the level's object layer without
// moving on the screen, turned the way the bonus was held; the cannon recoils and its sound plays
bool BonusCannon::Place()
{
	if (mState != BONUS_HELD)
		return false;
	mState = BONUS_PLACED;
	Player::Drop();
	float anAngle = (float)((double)CANNON_ROTATION_DEGREES[mRotation] * PI_F / 180.0);
	ObjBase* aLayer = static_cast<LevelObj*>(sLevel)->mObjectLayer;
	mCannon->SetParent(aLayer, nullptr);
	float aCannonY = (float)(((double)mCannon->mDrawn[STATE_Y] - (double)mCannon->mParent->mDrawn[STATE_Y]) *
		(double)sApp->mWindowToDesignY);
	float aCannonX = (float)(((double)mCannon->mDrawn[STATE_X] - (double)mCannon->mParent->mDrawn[STATE_X]) *
		(double)sApp->mWindowToDesignX);
	mCannon->SetState(STATE_X, aCannonX);
	mCannon->SetState(STATE_Y, aCannonY);
	mCannon->SetState(STATE_ROTATION, anAngle);
	mCannon->ComputeDrawn();
	mCannon->StartTrack(CANNON_TRACK_RECOIL);
	SetParent(aLayer, nullptr);
	float aBallY = (float)(((double)mDrawn[STATE_Y] - (double)mParent->mDrawn[STATE_Y]) *
		(double)sApp->mWindowToDesignY);
	float aBallX = (float)(((double)mDrawn[STATE_X] - (double)mParent->mDrawn[STATE_X]) *
		(double)sApp->mWindowToDesignX);
	SetState(STATE_X, aBallX);
	SetState(STATE_Y, aBallY);
	SetState(STATE_ROTATION, anAngle);
	SetState(STATE_ORBIT_ANGLE, anAngle);
	ComputeDrawn();
	sApp->mSoundQueue->Request(SOUND_CANNON, 0, false);
	return true;
}

// 0x407710: the muzzle's swells, and the ball takes off
void BonusCannon::Fire()
{
	for (int i = 0; i < 6; i++)
		mEmitters->BurstShortSwell(CANNON_MUZZLE_X[i], CANNON_MUZZLE_Y[i]);
	StartTrack(mFireTrack);
}

// 0x4077f0: the picture of the cannon (a child of the bonus) and its two tracks: the first holds it hidden (scale 0),
// the second is the recoil, which raises the event that fires the ball and, at its end, the one that puts the look back
void BonusCannon::BuildCannon()
{
	mCannon = new ObjBase();
	sCannonAnim->AddRef();
	mCannon->mAnim = sCannonAnim;
	mCannon->SetParent(this, nullptr);
	mCannon->SetEventTarget(this);
	ResetLook();

	AnimKey* aHiddenKey = new AnimKey();
	aHiddenKey->SetDuration(10);
	aHiddenKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aHiddenKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aHiddenTrack = new AnimTrack();
	aHiddenTrack->AddKey(aHiddenKey, aHiddenKey);
	mCannon->AddTrack(aHiddenTrack, nullptr);

	AnimKey* aGrowKey = new AnimKey();
	aGrowKey->SetDuration(20);
	aGrowKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrowKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aRecoilKey = new AnimKey();
	aRecoilKey->SetDuration(4);
	aRecoilKey->AddValue(STATE_SCALE_X, 0.35f, MOD_TOWARD_NEXT);
	aRecoilKey->AddValue(STATE_SCALE_Y, 0.35f, MOD_TOWARD_NEXT);
	AnimKey* aFireKey = new AnimKey();
	aFireKey->mEventId = CANNON_EVENT_FIRE;
	aFireKey->SetDuration(50);
	aFireKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFireKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSettleKey = new AnimKey();
	aSettleKey->SetDuration(10);
	aSettleKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSettleKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aWaitKey = new AnimKey();
	aWaitKey->SetDuration(500);
	aWaitKey->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aWaitKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimKey* aDoneKey = new AnimKey();
	aDoneKey->mEventId = CANNON_EVENT_DONE;
	aDoneKey->SetDuration(0);
	aDoneKey->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aDoneKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimTrack* aRecoilTrack = new AnimTrack();
	aRecoilTrack->AddKey(aGrowKey, nullptr);
	aRecoilTrack->AddKey(aRecoilKey, nullptr);
	aRecoilTrack->AddKey(aFireKey, nullptr);
	aRecoilTrack->AddKey(aSettleKey, nullptr);
	aRecoilTrack->AddKey(aWaitKey, nullptr);
	aRecoilTrack->AddKey(aDoneKey, nullptr);
	mCannon->AddTrack(aRecoilTrack, nullptr);
}

// 0x407c40: the ball's tracks. Held, it appears (growing in) and then turns a little each way for ever. Fired, it
// leaves the cannon in a straight line (an orbit about the cannon, its radius growing from 8 pixels) until it is off the
// screen or 500 frames have passed, and then the event 2 puts the bonus away
void BonusCannon::BuildBall()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sCannonBallAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sCannonBallAnim);
	aSwayKey->SetDuration(75);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sCannonBallAnim);
	aSwayBackKey->SetDuration(65);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349065848f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aBallTrack = new AnimTrack();
	aBallTrack->AddKey(anAppearKey, nullptr);
	aBallTrack->AddKey(aSwayKey, nullptr);
	aBallTrack->AddKey(aSwayBackKey, aSwayKey);
	mBallTrack = AddTrack(aBallTrack, nullptr);

	AnimKey* aFlightKey = new AnimKey();
	GiveAnim(aFlightKey, sCannonBallAnim);
	aFlightKey->SetDuration(500);
	aFlightKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlightKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlightKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aFlightKey->AddValue(STATE_ORBIT_RADIUS, 8.0f, MOD_TOWARD_NEXT);
	aFlightKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFlightKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aFlightKey->mEndCondition = KEY_END_OFF_SCREEN;
	aFlightKey->mEndChannel = STATE_X;
	aFlightKey->mEndValue = 0.0f;
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->mEventId = BONUS_EVENT_END;
	GiveAnim(aGoneKey, sCannonBallAnim);
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_ORBIT_RADIUS, 16.0f, MOD_HOLD);
	AnimTrack* aFlightTrack = new AnimTrack();
	aFlightTrack->AddKey(aFlightKey, nullptr);
	aFlightTrack->AddKey(aGoneKey, nullptr);
	mFireTrack = AddTrack(aFlightTrack, nullptr);
}
