#include "Bonus.h"

#include "BonusCoin.h"
#include "Anim.h"
#include "BananaBonus.h"
#include "BonusBomb.h"
#include "BonusCannon.h"
#include "BugObj.h"
#include "Emitter.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Bonus* Bonus::sCurrent = nullptr;
bool Bonus::sRunning = false;

// The kind of bonus whose held thing is turned over, not round (the missile)
const int BONUS_FLIPPED_KIND = BONUS_MISSILE;

// The sway of a bonus waiting: it settles on 0.6, a bit a frame, and is shaken harder while the monkey is near (within
// 124 pixels), the monkey's speed a seventh of its pace
const float SWAY_REST_DEGREES = 0.6f;
const double SWAY_STEP = (double)0.02f; // the exe's constant is the float's value widened (0.01999999955)
const float SWAY_NEAR_REACH = 124.0f;
const double SWAY_SPEED_DIVISOR = 7.0;

// Where the message of a bonus is shown (design pixels), how long it takes, how big and what colour, and which of the
// message places it takes
const float MESSAGE_X = 620.0f;
const float MESSAGE_Y = 496.0f;
const float MESSAGE_SCALE = 1.8f;
const float MESSAGE_RED = 0.1f;
const float MESSAGE_GREEN = 1.0f;
const float MESSAGE_BLUE = 0.2f;
const int MESSAGE_SLOT = 4;

// The emitters of every bonus: a swell, a ring of 40 stars and 100 grains of dust, each drawn in front (depth -1)
const float BONUS_EMITTER_DEPTH = -1.0f;
const int BONUS_STAR_COUNT = 0x28;
const int BONUS_DUST_COUNT = 100;

// 0x4066e0: every value of the object cleared (it has no picture), the depth 5.5
Bonus::Bonus()
{
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetState(STATE_Z, 5.5f);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_OFFSET_X, 0);
	SetStateInt(STATE_OFFSET_Y, 0);
	SetStateInt(STATE_ROTATION, 0);
	mSwayDegrees = SWAY_REST_DEGREES;
	mUnused2c0 = 0;
	mUnused294 = 0;
	mEndsWhenDone = false;
	mState = BONUS_IDLE;
	mRotation = 1;
	mKeyEventOneSeen = false;
	memset(mName, 0, sizeof(mName));
	mHeldCall = false;
	mUnused280 = 0;
	mUnused284 = 0;
	mKind = 0;
	for (int i = 0; i < 4; i++)
		mUnused2c8[i] = 0;
}

// 0x410670 deletes; the body of the base is 0x4067c0 (ObjBase's 0x450bb0)
Bonus::~Bonus()
{
}

// 0x4067d0: this object is the bonus of the kind (the original sets the kind first of all)
void Bonus::Setup(int theKind)
{
	mKind = theKind;
	mEmitters = new EmitterSet(this);
	mEmitters->MakeSwell(BONUS_EMITTER_DEPTH);
	mEmitters->MakeStarRing(BONUS_EMITTER_DEPTH, BONUS_STAR_COUNT);
	mEmitters->MakeDust(BONUS_EMITTER_DEPTH, BONUS_DUST_COUNT);
	if (theKind == BONUS_SHOT_SPEED || theKind == BONUS_SPEAR_CRAFT) {
		mEndsWhenDone = true;
		Deactivate(this);
		return;
	}
	mEndsWhenDone = theKind == BONUS_PARROT;
	Deactivate(this);
}

// 0x406870: while it waits (state 1) it sways, steadily, shaken when the monkey is close
bool Bonus::Update()
{
	if (!ObjBase::Update())
		return false;
	if (sLevel != nullptr && !LevelObj::IsPlaying())
		return false;
	if (mState == BONUS_WAITING) {
		float aSway = SWAY_REST_DEGREES;
		if (mSwayDegrees <= SWAY_REST_DEGREES) {
			if (mSwayDegrees < SWAY_REST_DEGREES)
				aSway = (float)((double)mSwayDegrees + SWAY_STEP);
		} else {
			aSway = (float)((double)mSwayDegrees - SWAY_STEP);
		}
		mSwayDegrees = aSway;
		if (Player::IsNear(this, SWAY_NEAR_REACH))
			mSwayDegrees = (float)(Player::GetSpeedAverage() / SWAY_SPEED_DIVISOR + (double)mSwayDegrees);
		SetState(STATE_ROTATION, (float)((double)mSwayDegrees * PI_F / 180.0 + (double)ObjBase::mState[STATE_ROTATION]));
	}
	return true;
}

// 0x40c760: the base's update, and then a bonus that is not waiting is made to stand upright (the turn it was given is
// taken from its animated value (+0x12c) and from the drawn value (+0x1bc))
bool Bonus::UpdateUpright()
{
	if (!Bonus::Update())
		return false;
	if (mState != BONUS_WAITING) {
		ObjBase::mState[STATE_ROTATION] = 0.0f;
		mDrawn[STATE_ROTATION] = 0.0f;
	}
	return true;
}

// 0x406950: key event 1 is noted, key event 2 puts the bonus away
void Bonus::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr) {
		int anEvent = theObj->mCurrentKey->mEventId;
		if (anEvent == BONUS_EVENT_SEEN)
			mKeyEventOneSeen = true;
		else if (anEvent == BONUS_EVENT_END)
			Reset();
	}
}

// 0x406980: a thing held by a monkey that is hidden is not drawn
void Bonus::Draw(Graphics* g)
{
	Player* aPlayer = Player::sCurrent;
	if (!aPlayer->mHidden || aPlayer->mHolder != mParent)
		ObjBase::Draw(g);
}

// 0x4069b0
void Bonus::SetUnused294(int theValue)
{
	mUnused294 = theValue;
}

// 0x4069c0: everything of the bonus is cleared (as the constructor does) and it is put away
void Bonus::Reset()
{
	sCurrent = nullptr;
	sRunning = false;
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetState(STATE_Z, 5.5f);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_OFFSET_X, 0);
	SetStateInt(STATE_OFFSET_Y, 0);
	SetStateInt(STATE_ROTATION, 0);
	memset(mName, 0, 34);
	mSwayDegrees = SWAY_REST_DEGREES;
	mHeldCall = false;
	mKeyEventOneSeen = false;
	mState = BONUS_IDLE;
	mRotation = 1;
	Deactivate(this);
}

// 0x406aa0: the bonus is over; one that is not just waiting is put away
void Bonus::End()
{
	sCurrent = nullptr;
	sRunning = false;
	if (mState != BONUS_WAITING)
		Reset();
}

// 0x406ac0
bool Bonus::EndWhenBugsGone()
{
	return false;
}

// 0x406ad0: the bonus is won: its sound, and its message. When the level has already reached its target or has no bug
// left, there is nothing for the bonus to do and it pays out a bunch of bananas at its own place and is put away
void Bonus::Start()
{
	mSwayDegrees = SWAY_REST_DEGREES;
	sRunning = true;
	mRotation = 1;
	Activate(this, false);
	mKeyEventOneSeen = false;
	sCurrent = this;
	SetHidden(false);
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	if (sLevel != nullptr && static_cast<LevelObj*>(sLevel)->mState == LEVEL_STATE_PLAYING) {
		if (Player::sCurrent->mPercent < Player::GetTargetPercent() && BugObj::sAlive > 0) {
			LevelObj::ShowMessage(mName, MESSAGE_X, MESSAGE_Y, 0, MESSAGE_SCALE, MESSAGE_RED, MESSAGE_GREEN,
				MESSAGE_BLUE, MESSAGE_SLOT);
		} else {
			static_cast<LevelObj*>(sLevel)->mBananaBonus->Spawn(mScreenX, mScreenY);
			Reset();
			return;
		}
	}
	mState = BONUS_RUNNING;
}

// 0x406be0: the held thing is turned a quarter (the missile is turned over instead)
bool Bonus::Rotate()
{
	if (mState == BONUS_HELD && Player::sCurrent != nullptr) {
		if (mKind == BONUS_FLIPPED_KIND)
			mRotation = Player::FlipHeld();
		else
			mRotation = Player::RotateHeld();
		return true;
	}
	return false;
}

// 0x406c20
bool Bonus::Place()
{
	return true;
}

// 0x406c30 (AL theFlag): the thing is held, lying as it did at first
void Bonus::BecomeHeld(bool theFlag)
{
	mHeldCall = theFlag;
	mState = BONUS_HELD;
	SetStateInt(STATE_ROTATION, 0);
	Rotate();
}

// 0x406c60: as Cancel, with the bonus under the ground (BonusCoin) put back first
void Bonus::ClearAll()
{
	Bonus* aHeld = Player::Drop();
	BonusCoin::ResetCurrent();
	BugObj::SetZapArea(0, 0.0f, 0.0f, 0.0f, 0.0f);
	if (sCurrent != nullptr && sCurrent->IsActive()) {
		if (sCurrent != aHeld) {
			sCurrent->End();
			sCurrent = nullptr;
			return;
		}
		sCurrent->Reset();
		sCurrent = nullptr;
	}
}

// 0x406cd0: what the monkey holds is let go; a bonus that was held is put away, one that was put down is ended
void Bonus::Cancel()
{
	Bonus* aHeld = Player::Drop();
	BugObj::SetZapArea(0, 0.0f, 0.0f, 0.0f, 0.0f);
	if (sCurrent != nullptr && sCurrent->IsActive()) {
		if (sCurrent != aHeld) {
			sCurrent->End();
			sCurrent = nullptr;
			return;
		}
		sCurrent->Reset();
		sCurrent = nullptr;
	}
}

// 0x406d40: the bonus under the ground is reset, and a running bonus that ends with the last bug ends
void Bonus::OnAllBugsDead()
{
	BonusCoin::ResetCurrent();
	BugObj::SetZapArea(0, 0.0f, 0.0f, 0.0f, 0.0f);
	if (sCurrent != nullptr && sCurrent->IsActive()) {
		if (sCurrent->EndWhenBugsGone())
			sCurrent = nullptr;
	}
}

// 0x406d90
void Bonus::CancelIfDone()
{
	if (sCurrent != nullptr && sCurrent->mEndsWhenDone && sCurrent->mState == BONUS_RUNNING) {
		if (sCurrent->IsActive()) {
			sCurrent->End();
			sCurrent = nullptr;
		}
	}
}

// 0x406dd0: the pictures the bonuses use, loaded once and kept (each is in the global of its class or, until the
// class is translated, in Pending)
void Sexy::PreloadBonusPictures()
{
	BonusBomb::sCocoBombAnim = Anim::Get("images/cocobomb", ANIM_SCALE_SMOOTH, false, 0, 0, 1, true);
	gMissileAnim = Anim::Get("images/missile", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	BonusCannon::sCannonBallAnim = Anim::Get("images/cannonball", ANIM_SCALE_SMOOTH, false, 0, 0, 1, true);
	BonusCannon::sCannonAnim = Anim::Get("images/cannon", ANIM_SCALE_SMOOTH, false, 0, 0, 1, true);
	gParrotFlyAnim = Anim::Get("images/parrot_fly", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0xb, false);
	gParrotWalkAnim = Anim::Get("images/parrot_walk", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x14, false);
	gParrotPeckAnim = Anim::Get("images/parrot_peck", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 0x11, false);
	gDiscoBallAnim = Anim::Get("images/bonus_discoball", ANIM_SCALE_SMOOTH, false, 0x60, 0x60, 6, true);
	gMilkDropAnim = Anim::Get("images/bonus_milkdrop", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	gDropAnim = Anim::Get("images/drop", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	gTarzanAnim = Anim::Get("images/bonus_tarzan", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	gPaperAnim = Anim::Get("images/bonus_paper", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	gZapperAnim = Anim::Get("images/bonus_zapper", ANIM_SCALE_SMOOTH, false, 0x3f, 0x55, 2, false);
	gSwatterAnim = Anim::Get("images/bonus_swatter", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	gMineAnim = Anim::Get("images/bonus_mine", ANIM_SCALE_SMOOTH, false, 0, 0, 1, true);
	gZumaAnim = Anim::Get("images/bonus_zuma", ANIM_SCALE_SMOOTH, false, 0x3f, 0x4b, 2, false);
	gZumaBallAnim = Anim::Get("images/bonus_zuma_ball", ANIM_SCALE_SMOOTH, false, 0, 0, 1, true);
}
