#include "BonusRain.h"

#include "Anim.h"
#include "BackDrop.h"
#include "Board.h"
#include "BonusBomb.h"
#include "BugObj.h"
#include "GameApp.h"
#include "GameWidget.h"
#include "LevelObj.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

// The key event of a drop's last key (the base's events 1 and 2 are not used by the rain)
const int RAIN_EVENT_DROP_LANDED = 4;

const int RAIN_NO_COUNTDOWN = -99;
const int RAIN_FRAMES = 700;               // how long it rains
const int RAIN_DROPS_LEFT_AT_END = 50;     // what the drops that are left are counted from when End is called
const int RAIN_Z = 20;                     // the rain's own object (a picture of nothing) and its size
const float RAIN_SCALE = 2.0f;
const int RAIN_SLOW_FRAMES = 3;            // updates the bugs skip between two they run

// The drops: the first starts 650 above the screen, each next 64 higher; each starts at a place along the top, 2501
// places from 1600 left of the centre (the restart after a landing: 2621 places from 1920 left)
const int RAIN_DROP_START_Y = -650;
const int RAIN_DROP_STEP_Y = -64;
const unsigned int RAIN_DROP_X_RANGE = 0x9c5;
const int RAIN_DROP_X_BIAS = 0x640;
const unsigned int RAIN_RESTART_X_RANGE = 0xa3d;
const int RAIN_RESTART_X_BIAS = 0x780;
const float RAIN_RESTART_Y = -650.0f;
const int RAIN_DROP_STEPS = 8;             // the depth, size and speed go round eight steps
const int RAIN_DROP_Z_STEP = 20;
const float RAIN_DROP_ANGLE = 5.23598814f; // 0x40a78d37: the drop's rotation and the orbit's angle (the way it falls)
const int RAIN_DROP_FIRST_RADIUS = 0x10;   // the orbit's radius, 16 to 30 a frame, in steps of 2
const int RAIN_DROP_FALL_FRAMES = 999;
const float RAIN_DROP_END_Y = 1000.0f;     // the fall ends early when the drop is lower than this
const float RAIN_DROP_FADE_START = 0.2f;
const float RAIN_DROP_LAND_ALPHA = 0.1f;

// 0x410660: the kind is given by Setup; the fields are as operator new left them in the original, the port clears them
BonusRain::BonusRain()
{
	for (int i = 0; i < RAIN_DROPS; i++)
		mDrops[i] = nullptr;
	mEnded = false;
	mCountdown = 0;
	mUnused3a8[0] = 0;
	mUnused3a8[1] = 0;
	mUnused3a8[2] = 0;
	mDarkened = false;
	mDropsLeft = 0;
}

// 0x410690
void BonusRain::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mDarkened = false;
	mCountdown = RAIN_NO_COUNTDOWN;
	mEnded = false;
	mDropsLeft = 0;
	BuildDrops();
}

// 0x4106d0: the countdown ends the bonus
bool BonusRain::Update()
{
	if (!Bonus::Update())
		return false;
	if (mCountdown != RAIN_NO_COUNTDOWN) {
		if (mCountdown > 0) {
			mCountdown--;
			return true;
		}
		if (mCountdown == 0) {
			mCountdown = RAIN_NO_COUNTDOWN;
			End();
		}
	}
	return true;
}

// 0x410720: a drop has landed (its last key, event 4). While it rains it starts again from above the screen at a
// random place along it; once the rain has ended it is put away, and the bonus when only one is left
void BonusRain::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	if (theObj->mCurrentKey->mEventId != RAIN_EVENT_DROP_LANDED) {
		Bonus::Notify(theObj);
		return;
	}
	if (!mEnded) {
		theObj->mState[STATE_X] = (float)((int)(GameRandomNext() % RAIN_RESTART_X_RANGE) - RAIN_RESTART_X_BIAS);
		theObj->mState[STATE_Y] = RAIN_RESTART_Y;
	} else {
		Deactivate(theObj);
		mDropsLeft--;
		if (mDropsLeft < 2)
			Reset();
	}
}

// 0x4107b0: put away. The backdrop and the sound are put back, and the drops are put away
void BonusRain::Reset()
{
	Bonus::Reset();
	mCountdown = RAIN_NO_COUNTDOWN;
	mEnded = false;
	if (mDarkened) {
		mDarkened = false;
		sApp->mGameWidget->mBoard->mBackDrop->Undarken();
	}
	sApp->mSoundQueue->StopLoop(SOUND_RAIN);
	for (int i = 0; i < RAIN_DROPS; i++)
		Deactivate(mDrops[i]);
}

// 0x410830: the rain is over (the countdown is out, or the bonus is cancelled): a bonus that is not running is put away
// at once; otherwise the sound stops, the backdrop is put back and the bugs run at their speed again, and the drops
// fall away
void BonusRain::End()
{
	mCountdown = RAIN_NO_COUNTDOWN;
	mDropsLeft = RAIN_DROPS_LEFT_AT_END;
	if (!IsActive()) {
		Reset();
		return;
	}
	sApp->mSoundQueue->StopLoop(SOUND_RAIN);
	if (!mEnded) {
		mEnded = true;
		if (mDarkened) {
			mDarkened = false;
			sApp->mGameWidget->mBoard->mBackDrop->Undarken();
		}
		BugObj::SetSlowFrames(0);
	}
}

// 0x4108c0: the rain begins: its sound loops, the rain's own object is put on the level's object layer (hidden), the backdrop darkens,
// the drops start to fall and the bugs are slowed
void BonusRain::Start()
{
	strcpy(mName, "RAINSTORM");
	sApp->mSoundQueue->Request(SOUND_RAIN, 0, true);
	mCountdown = RAIN_FRAMES;
	mDropsLeft = RAIN_NO_COUNTDOWN;
	mEnded = false;
	Bonus::Start();
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	SetHidden(true);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, RAIN_Z);
	SetStateInt(STATE_ALPHA, 1);
	SetState(STATE_SCALE_X, RAIN_SCALE);
	SetState(STATE_SCALE_Y, RAIN_SCALE);
	SetStateInt(STATE_FRAME, 0);
	sApp->mGameWidget->mBoard->mBackDrop->Darken();
	mDarkened = true;
	for (int i = 0; i < RAIN_DROPS; i++) {
		mDrops[i]->StartTrack(0);
		Activate(mDrops[i], false);
	}
	BugObj::SetSlowFrames(RAIN_SLOW_FRAMES);
}

// 0x4109f0: the 50 drops, children of the root, each with its own random place along the top, depth, size and speed
// (one random draw each, in order). A drop's one track never ends: it falls (the orbit's radius, 16 to 30, steady: it
// lands in 999 frames at most, or when it is below 1000), and its last key's event 4 tells the rain it has landed (the
// key's next is the first, so the track starts over)
void BonusRain::BuildDrops()
{
	int aStartY = RAIN_DROP_START_Y;
	for (int i = 0; i < RAIN_DROPS; i++) {
		ObjBase* aDrop = new ObjBase();
		mDrops[i] = aDrop;
		aDrop->SetParent(ObjBase::sRoot, nullptr);
		aDrop->SetStateInt(STATE_ALPHA, 0);
		BonusBomb::sMilkDropAnim->AddRef();
		aDrop->mAnim = BonusBomb::sMilkDropAnim;
		int aStep = i & (RAIN_DROP_STEPS - 1);
		aDrop->SetStateInt(STATE_X, (int)(GameRandomNext() % RAIN_DROP_X_RANGE) - RAIN_DROP_X_BIAS);
		aDrop->SetStateInt(STATE_Y, aStartY);
		aDrop->SetStateInt(STATE_Z, aStep * RAIN_DROP_Z_STEP + 1);
		double aHalfSteps = (double)(aStep / 2) * 0.5;
		aDrop->SetState(STATE_SCALE_X, (float)(aHalfSteps + 0.5));
		aDrop->SetState(STATE_SCALE_Y, (float)aHalfSteps);
		aDrop->SetState(STATE_ROTATION, RAIN_DROP_ANGLE);
		aDrop->SetState(STATE_ORBIT_ANGLE, RAIN_DROP_ANGLE);
		aDrop->SetStateInt(STATE_ORBIT_RADIUS, aStep * 2 + RAIN_DROP_FIRST_RADIUS);
		aDrop->SetEventTarget(this);

		AnimKey* aFallKey = new AnimKey();
		aFallKey->SetDuration(RAIN_DROP_FALL_FRAMES);
		aFallKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
		aFallKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
		aFallKey->AddValue(STATE_ALPHA, RAIN_DROP_FADE_START, MOD_TOWARD_NEXT);
		aFallKey->mEndCondition = KEY_END_ABOVE;
		aFallKey->mEndChannel = STATE_Y;
		aFallKey->mEndValue = RAIN_DROP_END_Y;
		AnimKey* aLandedKey = new AnimKey();
		aLandedKey->mEventId = RAIN_EVENT_DROP_LANDED;
		aLandedKey->SetDuration(1);
		aLandedKey->AddValue(STATE_ALPHA, RAIN_DROP_LAND_ALPHA, MOD_TOWARD_NEXT);
		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aFallKey, nullptr);
		aTrack->AddKey(aLandedKey, aFallKey);
		aDrop->AddTrack(aTrack, nullptr);
		Deactivate(aDrop);
		aStartY += RAIN_DROP_STEP_Y;
	}
}
