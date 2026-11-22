#include "BonusBomb.h"

#include "Anim.h"
#include "BugObj.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusBomb::sCocoBombAnim = nullptr;

// The key events of the bomb's drop track (the base's events 1 and 2 are not used by the bomb)
enum BombEvent
{
	BOMB_EVENT_BURST = 5,    // the bomb has landed: it bursts
	BOMB_EVENT_SETTLED = 6,  // nothing listens for it
	BOMB_EVENT_DONE = 8      // the bomb is put away
};

// Where the held bonus rides, relative to its holder (design pixels)
const int BOMB_HELD_X = 64;

// The reach of the burst (design pixels)
const float BOMB_BLAST_REACH = 138.0f;

// The angle the bomb is thrown at, for each way it was turned (Bonus::mRotation 1 to 4), in radians
const float BOMB_ANGLE_UP = 0.0f;
const float BOMB_ANGLE_RIGHT = 4.71238899f; // 0x4096cbe4
const float BOMB_ANGLE_DOWN = 3.14159274f;  // 0x40490fdb
const float BOMB_ANGLE_LEFT = 1.57079637f;  // 0x3fc90fdb

// A drop of the spray: a whole number of degrees from 0 to 359 for its direction, a speed of 1.8 to 2.9 a frame for its
// start, a life of 40 to 70 frames; it flies along its direction, slowing and falling a little, shrinking
const int BOMB_ANGLE_DEGREES = 0x168;
const int BOMB_SPEED_STEPS = 0xd;
const int BOMB_SPEED_LEAST = 0x12;
const int BOMB_LIFE_RANGE = 0x1f;
const int BOMB_LIFE_LEAST = 0x28;
const float BOMB_DROP_FALL_RATE = -0.05f;

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x4080c0: the fields are as operator new left them in the original; the port clears them
BonusBomb::BonusBomb()
{
	mDropped = false;
	mHeldTrack = 0;
	mDropTrack = 0;
	for (int i = 0; i < BOMB_DROPS; i++)
		mDrops[i] = nullptr;
}

// 0x408100
void BonusBomb::Setup(int theKind)
{
	mDropped = false;
	Bonus::Setup(theKind);
	BuildTracks();
	BuildDrops();
}

// 0x40c760 (shared with BonusFlyPaper): a bonus that is not waiting stands upright
bool BonusBomb::Update()
{
	return UpdateUpright();
}

// 0x408130: the bomb bursts when it lands, and is put away at its last key
void BonusBomb::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr) {
		int anEvent = theObj->mCurrentKey->mEventId;
		if (anEvent == BOMB_EVENT_BURST) {
			SetHidden(true);
			BugObj::ZapNear(mScreenX, mScreenY, BOMB_BLAST_REACH);
			Burst();
			sApp->mSoundQueue->Request(SOUND_SPLAT1, 0, false);
			return;
		}
		if (anEvent != BOMB_EVENT_SETTLED) {
			if (anEvent == BOMB_EVENT_DONE)
				Reset();
			else
				Bonus::Notify(theObj);
		}
	}
}

// 0x4081d0
void BonusBomb::Reset()
{
	Player::Drop();
	Bonus::Reset();
	mDropped = false;
}

// 0x4081f0: the first End (the bonus is over before the bomb was dropped) puts it away
void BonusBomb::End()
{
	if (!mDropped) {
		mDropped = true;
		Reset();
	}
}

// 0x408210: the monkey takes the bomb
void BonusBomb::Start()
{
	strcpy(mName, "CLICK TO DROP COCOBOMB");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, BOMB_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	Player::PickUp(this, 1);
	BecomeHeld(false);
	Activate(this, false);
	StartTrack(mHeldTrack);
}

// 0x4082c0: the bomb is dropped: it goes to the level's object layer without moving on the screen, takes the angle of
// the way it was turned, and starts its track
bool BonusBomb::Place()
{
	if (mState != BONUS_HELD)
		return false;
	mDropped = true;
	sApp->mSoundQueue->Request(SOUND_COCOLETGO, 0, false);
	Player::Drop();
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	float aY = (float)(((double)mDrawn[STATE_Y] - (double)mParent->mDrawn[STATE_Y]) * (double)sApp->mWindowToDesignY);
	float aX = (float)(((double)mDrawn[STATE_X] - (double)mParent->mDrawn[STATE_X]) * (double)sApp->mWindowToDesignX);
	SetState(STATE_X, aX);
	SetState(STATE_Y, aY);
	float anAngle;
	if (mRotation == 1)
		anAngle = BOMB_ANGLE_UP;
	else if (mRotation == 2)
		anAngle = BOMB_ANGLE_RIGHT;
	else if (mRotation == 3)
		anAngle = BOMB_ANGLE_DOWN;
	else
		anAngle = BOMB_ANGLE_LEFT;
	SetState(STATE_ORBIT_ANGLE, anAngle);
	ComputeDrawn();
	StartTrack(mDropTrack);
	return true;
}

// 0x4083c0 (the bomb on the stack): each drop is put at the bomb's centre and starts its track
void BonusBomb::Burst()
{
	for (int i = 0; i < BOMB_DROPS; i++) {
		ObjBase* aDrop = mDrops[i];
		aDrop->SetParent(this, nullptr);
		aDrop->SetStateInt(STATE_X, 0);
		aDrop->SetStateInt(STATE_Y, 0);
		aDrop->StartTrack(0);
		aDrop->Activate(aDrop, false);
	}
}

// 0x408420: the spray, 32 drops, each with a random direction, speed and life (drawn in that order)
void BonusBomb::BuildDrops()
{
	for (int i = 0; i < BOMB_DROPS; i++) {
		float anAngle = (float)((double)(int)(GameRandomNext() % BOMB_ANGLE_DEGREES) * PI_F / 180.0);
		ObjBase* aDrop = new ObjBase();
		mDrops[i] = aDrop;
		gMilkDropAnim->AddRef();
		aDrop->mAnim = gMilkDropAnim;
		aDrop->SetParent(this, nullptr);
		aDrop->SetStateInt(STATE_ALPHA, 0);
		aDrop->SetStateInt(STATE_SCALE_X, 0);
		aDrop->SetStateInt(STATE_SCALE_Y, 0);
		float aSpeed = (float)((double)(int)(GameRandomNext() % BOMB_SPEED_STEPS + BOMB_SPEED_LEAST) / 10.0);

		AnimKey* aFlyKey = new AnimKey();
		aFlyKey->SetDuration((int)(GameRandomNext() % BOMB_LIFE_RANGE + BOMB_LIFE_LEAST));
		aFlyKey->AddDecay(STATE_X, 0.0f, aSpeed, FloatCos(anAngle), 0.0f, 1.0f);
		aFlyKey->AddDecay(STATE_Y, 0.0f, aSpeed, FloatSin(anAngle), BOMB_DROP_FALL_RATE, 1.0f);
		aFlyKey->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
		aFlyKey->AddValue(STATE_SCALE_X, 0.7f, MOD_TOWARD_NEXT);
		aFlyKey->AddValue(STATE_SCALE_Y, 0.7f, MOD_TOWARD_NEXT);
		aFlyKey->AddValue(STATE_ROTATION, anAngle, MOD_FACE_MOTION);
		AnimKey* aFadeKey = new AnimKey();
		aFadeKey->SetDuration(20);
		aFadeKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aFadeKey->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
		aFadeKey->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
		aFadeKey->AddValue(STATE_ROTATION, anAngle, MOD_COPY_ORBIT_ANGLE);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aFlyKey, nullptr);
		aTrack->AddKey(aFadeKey, nullptr);
		aDrop->AddTrack(aTrack, nullptr);
		aDrop->Deactivate(aDrop);
	}
}

// 0x408760: the bomb's two tracks (every key shows the coconut). Held, it appears growing in and then turns a little
// each way for ever. Dropped, it flies a short way along its angle (the orbit growing to 1 pixel), swells, shows the
// burst (event 5: the bomb is hidden then), waits 150 frames and is put away (event 8)
void BonusBomb::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sCocoBombAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sCocoBombAnim);
	aSwayKey->SetDuration(75);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sCocoBombAnim);
	aSwayBackKey->SetDuration(65);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349066f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aThrowKey = new AnimKey();
	GiveAnim(aThrowKey, sCocoBombAnim);
	aThrowKey->SetDuration(40);
	aThrowKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aThrowKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aThrowKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aThrowKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aThrowKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aThrowKey->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aFallKey = new AnimKey();
	GiveAnim(aFallKey, sCocoBombAnim);
	aFallKey->SetDuration(40);
	aFallKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aFallKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aFallKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFallKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwellKey = new AnimKey();
	aSwellKey->mEventId = BOMB_EVENT_SETTLED;
	GiveAnim(aSwellKey, sCocoBombAnim);
	aSwellKey->SetDuration(5);
	aSwellKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aSwellKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aSwellKey->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aSwellKey->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	AnimKey* aBurstKey = new AnimKey();
	aBurstKey->mEventId = BOMB_EVENT_BURST;
	GiveAnim(aBurstKey, sCocoBombAnim);
	aBurstKey->SetDuration(1);
	aBurstKey->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aBurstKey->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aBurstKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimKey* aWaitKey = new AnimKey();
	GiveAnim(aWaitKey, sCocoBombAnim);
	aWaitKey->SetDuration(150);
	aWaitKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aWaitKey->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aWaitKey->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimKey* aDoneKey = new AnimKey();
	aDoneKey->mEventId = BOMB_EVENT_DONE;
	GiveAnim(aDoneKey, sCocoBombAnim);
	aDoneKey->SetDuration(1);
	AnimTrack* aDropTrack = new AnimTrack();
	aDropTrack->AddKey(aThrowKey, nullptr);
	aDropTrack->AddKey(aFallKey, nullptr);
	aDropTrack->AddKey(aSwellKey, nullptr);
	aDropTrack->AddKey(aBurstKey, nullptr);
	aDropTrack->AddKey(aWaitKey, nullptr);
	aDropTrack->AddKey(aDoneKey, nullptr);
	mDropTrack = AddTrack(aDropTrack, nullptr);
}
