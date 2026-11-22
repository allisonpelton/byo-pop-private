#include "BonusMissile.h"

#include "Anim.h"
#include "BugObj.h"
#include "Emitter.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "StarStream.h"

#include <cstring>

using namespace Sexy;

Anim* BonusMissile::sMissileAnim = nullptr;

// Where the held bonus rides, relative to its holder (design pixels)
const int MISSILE_HELD_X = 64;

// The reach of the missile: how near a bug must be for it to burst, how far the burst kills, and how close in front of it
// a bug may stand for it not to be fired (design pixels): the place looked at is 49 from the missile's, in the way it
// points
const float MISSILE_TRIGGER_REACH = 75.0f;
const float MISSILE_BLAST_REACH = 128.0f;
const float MISSILE_MUZZLE_REACH = 33.0f;
const double MISSILE_MUZZLE_DISTANCE = 49.0;

// How long the ended missile waits before it is put away, and the burst one (the original puts it away when the count
// is 1)
const int MISSILE_END_FRAMES = 400;
const int MISSILE_BURST_FRAMES = 800;

// The trail: a swell every fourth frame of the flight (the frame number's low bits)
const int MISSILE_TRAIL_MASK = 3;

// The ring of the burst: 10 stars, 36 degrees apart, of the kind that sparks blue
const int MISSILE_STAR_DEGREES_APART = 0x24;
const int MISSILE_STAR_DEGREES_ROUND = 0x168;

// The emitter the missile makes in place of the base's: a short swell of 64, at depth -0.5
const float MISSILE_EMITTER_DEPTH = -0.5f;
const int MISSILE_SWELL_COUNT = 0x40;

// The angle of the orbit for each way the missile points (Bonus::mRotation 0 to 3: up, right, down, left), in degrees
// (0x600b24; the same four as BonusCannon's)
const int MISSILE_ROTATION_DEGREES[4] = {90, 0, 270, 180};

// How long the flight's first key may last (it ends sooner when the missile leaves the screen); its last key raises the
// base's BONUS_EVENT_END
const int MISSILE_FLIGHT_FRAMES = 500;

// 0x40e550: the fields are as operator new left them in the original; the port clears them
BonusMissile::BonusMissile()
{
	mUnused2d8 = 0;
	mFired = false;
	mBurstTrack = 0;
	mHeldTrack = 0;
	mFlightTrack = 0;
	mTimer = 0;
	for (int i = 0; i < MISSILE_STARS; i++)
		mStars[i] = nullptr;
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x40e590 (ObjBase::Init is 0x450dd0): put back as it was made, with nothing held
void BonusMissile::Init()
{
	ObjBase::Init();
	mFired = false;
	SetHidden(false);
	mTimer = 0;
	if (Player::sCurrent != nullptr)
		Player::Drop();
	Deactivate(this);
}

// 0x40e5d0: the tracks, the swell of the trail and the 10 stars as children of the missile. The emitters are made again
// (the original lets go of the ones Bonus::Setup made, which are never freed)
void BonusMissile::Setup(int theKind)
{
	mTimer = 0;
	mFired = false;
	Bonus::Setup(theKind);
	BuildTracks();
	mEmitters = new EmitterSet(this);
	mEmitters->MakeShortSwell(MISSILE_EMITTER_DEPTH, MISSILE_SWELL_COUNT);
	for (int i = 0; i < MISSILE_STARS; i++) {
		mStars[i] = new StarStream(STAR_STREAM_BLUE);
		mStars[i]->SetParent(this, nullptr);
	}
}

// 0x40e6b0: in flight (state 4) the missile trails a swell every fourth frame, and bursts as soon as a bug is near
// enough (that bug dies, and so does every one within the blast); once it has burst or been ended (state 5) it counts
// down and is put away
bool BonusMissile::Update()
{
	if (!Bonus::Update())
		return false;
	if (mState == BONUS_PLACED) {
		if ((ObjBase::sFrame & MISSILE_TRAIL_MASK) == 0)
			mEmitters->BurstShortSwell(0.0f, 0.0f);
		if (BugObj::ZapFirstNear(mScreenX, mScreenY, MISSILE_TRIGGER_REACH) != nullptr) {
			int aDegrees = 0;
			int i = 0;
			do {
				mStars[i]->Burst(0.0f, 0.0f, aDegrees);
				aDegrees += MISSILE_STAR_DEGREES_APART;
				i++;
			} while (aDegrees < MISSILE_STAR_DEGREES_ROUND);
			BugObj::ZapNear(mScreenX, mScreenY, MISSILE_BLAST_REACH);
			mTimer = MISSILE_BURST_FRAMES;
			mState = BONUS_ENDING;
			StartTrack(mBurstTrack);
			SetState(STATE_ROTATION, 0.0f);
			SetState(STATE_ORBIT_ANGLE, 0.0f);
			SetHidden(true);
			sApp->mSoundQueue->Request(SOUND_SPLAT1, 0, false);
		}
	} else if (mState == BONUS_ENDING) {
		mTimer--;
		if (mTimer == 1) {
			Reset();
			return true;
		}
	}
	return true;
}

// 0x40e810
void BonusMissile::Reset()
{
	Player::Drop();
	Bonus::Reset();
	mFired = false;
}

// 0x40e830: ended while it is still on the board (active) and not yet fired: it is put away after 400 frames
void BonusMissile::End()
{
	if (IsActive() && !mFired) {
		mFired = true;
		mState = BONUS_ENDING;
		mTimer = MISSILE_END_FRAMES;
	}
}

// 0x40e860: the monkey takes the missile
void BonusMissile::Start()
{
	strcpy(mName, "CLICK TO SHOOT MISSILE");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, MISSILE_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, 1);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	Player::PickUp(this, 1);
	BecomeHeld(true);
	Activate(this, false);
	StartTrack(mHeldTrack);
}

// 0x40e920: the missile is fired, unless a bug stands in front of it (49 pixels from it, in the way it points, within 33):
// it goes to the level's object layer without moving on the screen, takes the angle of the way it points, and starts its
// flight
bool BonusMissile::Place()
{
	if (mState != BONUS_HELD)
		return false;
	BugObj* aBugInFront;
	if (mRotation == 0)
		aBugInFront = BugObj::FindNear(mScreenX, (float)((double)mScreenY - MISSILE_MUZZLE_DISTANCE), MISSILE_MUZZLE_REACH);
	else if (mRotation == 1)
		aBugInFront = BugObj::FindNear((float)((double)mScreenX + MISSILE_MUZZLE_DISTANCE), mScreenY, MISSILE_MUZZLE_REACH);
	else if (mRotation == 2)
		aBugInFront = BugObj::FindNear(mScreenX, (float)((double)mScreenY + MISSILE_MUZZLE_DISTANCE), MISSILE_MUZZLE_REACH);
	else
		aBugInFront = BugObj::FindNear((float)((double)mScreenX - MISSILE_MUZZLE_DISTANCE), mScreenY, MISSILE_MUZZLE_REACH);
	if (aBugInFront != nullptr)
		return false;
	mFired = true;
	sApp->mSoundQueue->Request(SOUND_MISSILE, 0, false);
	Player::Drop();
	mState = BONUS_PLACED;
	float anAngle = (float)((double)MISSILE_ROTATION_DEGREES[mRotation] * PI_F / 180.0);
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	float aY = (float)(((double)mDrawn[STATE_Y] - (double)mParent->mDrawn[STATE_Y]) * (double)sApp->mWindowToDesignY);
	float aX = (float)(((double)mDrawn[STATE_X] - (double)mParent->mDrawn[STATE_X]) * (double)sApp->mWindowToDesignX);
	SetState(STATE_X, aX);
	SetState(STATE_Y, aY);
	SetState(STATE_ROTATION, anAngle);
	SetState(STATE_ORBIT_ANGLE, anAngle);
	ComputeDrawn();
	StartTrack(mFlightTrack);
	return true;
}

// 0x40eaf0: the missile's three tracks (every key but the burst track's shows the missile). Held, it appears growing in
// and then turns a little each way for ever. In flight it flies along its angle, the orbit's radius (its speed a frame)
// growing from 2 to 64 pixels, until it is off the screen, and then its event 2 puts the bonus away. The burst track holds
// the missile still, for 10 frames (the original's key holds itself)
void BonusMissile::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sMissileAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sMissileAnim);
	aSwayKey->SetDuration(0x4b);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sMissileAnim);
	aSwayBackKey->SetDuration(0x41);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349065848f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aFlyKey = new AnimKey();
	GiveAnim(aFlyKey, sMissileAnim);
	aFlyKey->SetDuration(MISSILE_FLIGHT_FRAMES);
	aFlyKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlyKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlyKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aFlyKey->AddValue(STATE_ORBIT_RADIUS, 2.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFlyKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aFlyKey->mEndCondition = KEY_END_OFF_SCREEN;
	aFlyKey->mEndChannel = STATE_X;
	aFlyKey->mEndValue = 0.0f;
	AnimKey* aFlyOutKey = new AnimKey();
	aFlyOutKey->mEventId = BONUS_EVENT_END;
	GiveAnim(aFlyOutKey, sMissileAnim);
	aFlyOutKey->SetDuration(1);
	aFlyOutKey->AddValue(STATE_ORBIT_RADIUS, 64.0f, MOD_HOLD);
	AnimTrack* aFlightTrack = new AnimTrack();
	aFlightTrack->AddKey(aFlyKey, nullptr);
	aFlightTrack->AddKey(aFlyOutKey, nullptr);
	mFlightTrack = AddTrack(aFlightTrack, nullptr);

	AnimKey* aStillKey = new AnimKey();
	aStillKey->SetDuration(10);
	aStillKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	AnimTrack* aBurstTrack = new AnimTrack();
	aBurstTrack->AddKey(aStillKey, aStillKey);
	mBurstTrack = AddTrack(aBurstTrack, nullptr);
}
