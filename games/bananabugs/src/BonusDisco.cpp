#include "BonusDisco.h"

#include "Anim.h"
#include "BackDrop.h"
#include "Board.h"
#include "BugObj.h"
#include "GameApp.h"
#include "GameWidget.h"
#include "LevelObj.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusDisco::sDiscoBallAnim = nullptr;

// The key events of the ball's tracks (the base's events 1 and 2 are not used by the disco)
enum DiscoEvent
{
	DISCO_EVENT_LANDED = 4,   // the lowering is over: the ball turns
	DISCO_EVENT_GONE = 5      // the rising is over: the bonus is put away
};

// 0x40b0d0 sets this field of the object (+0xf0, one of the ObjBase's that nothing reads) to 550256; the port keeps it
const int DISCO_UNUSED_F0 = 0x86570;

const int DISCO_NO_COUNTDOWN = -99;
const int DISCO_FRAMES = 600;              // how long it lasts
const int DISCO_Z = 20;                    // the depth the ball is drawn at, and its size (twice the picture's)
const float DISCO_SCALE = 2.0f;

// The ball's heights above its place (design pixels), how long it takes to lower, to turn and to rise, and how its cel
// counts: up one every 33 frames, round six cels (the key's rate and amount)
const float DISCO_HIGH_Y = -300.0f;
const float DISCO_LOW_Y = -20.0f;
const int DISCO_LOWER_FRAMES = 200;
const int DISCO_TURN_FRAMES = 33;
const int DISCO_RISE_FRAMES = 120;
const float DISCO_CEL_RATE = 33.0f;
const float DISCO_CEL_COUNT = 6.0f;

// 0x40b0d0
BonusDisco::BonusDisco()
{
	mUnusedF0 = DISCO_UNUSED_F0;
	mEnded = false;
	mCountdown = 0;
	mLowerTrack = 0;
	mSpinTrack = 0;
	mRiseTrack = 0;
	mDarkened = false;
	mLanded = false;
	mMusicOn = false;
	mEndRequested = false;
}

// 0x40b0f0
void BonusDisco::Setup(int theKind)
{
	Bonus::Setup(theKind);
	mDarkened = false;
	mCountdown = DISCO_NO_COUNTDOWN;
	mEnded = false;
	mEndRequested = false;
	mLanded = false;
	mMusicOn = false;
	BuildTracks();
}

// 0x40b140: the countdown ends the bonus
bool BonusDisco::Update()
{
	if (!Bonus::Update())
		return false;
	if (mCountdown != DISCO_NO_COUNTDOWN) {
		if (mCountdown > 0) {
			mCountdown--;
			return true;
		}
		if (mCountdown == 0) {
			mCountdown = DISCO_NO_COUNTDOWN;
			End();
		}
	}
	return true;
}

// 0x40b190: the ball has landed (it turns, and the bonus may now be ended by the last bug) or has risen (put away). A
// bonus that was asked to end before the ball landed ends as soon as any event comes in
void BonusDisco::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == DISCO_EVENT_LANDED) {
		StartTrack(mSpinTrack);
		mLanded = true;
	} else if (anEvent == DISCO_EVENT_GONE) {
		Reset();
	} else {
		Bonus::Notify(theObj);
	}
	if (mEndRequested) {
		End();
		mEndRequested = false;
	}
}

// 0x40b200: put away; the backdrop and the sound are put back
void BonusDisco::Reset()
{
	BugObj::SetAlwaysPeaHit(false);
	Bonus::Reset();
	mLanded = false;
	if (mDarkened) {
		mDarkened = false;
		sApp->mGameWidget->mBoard->mBackDrop->Undarken();
	}
	if (mMusicOn) {
		sApp->mSoundQueue->StopLoop(SOUND_DISCOSONG);
		mMusicOn = false;
	}
	mEnded = false;
	mCountdown = DISCO_NO_COUNTDOWN;
}

// 0x40b280: the first End: the bugs are let go (each takes the command it was held for) and the ball rises
void BonusDisco::End()
{
	if (mEnded)
		return;
	BugObj::SetAlwaysPeaHit(false);
	mCountdown = DISCO_NO_COUNTDOWN;
	mEnded = true;
	sApp->mSoundQueue->StopLoop(SOUND_DISCOSONG);
	mMusicOn = false;
	if (mDarkened) {
		mDarkened = false;
		sApp->mGameWidget->mBoard->mBackDrop->Undarken();
	}
	BugObj::ReleaseAll();
	StartTrack(mRiseTrack);
	mLanded = false;
}

// 0x40b310: the last bug is gone: the bonus ends if the ball has landed, otherwise as soon as it does
bool BonusDisco::EndWhenBugsGone()
{
	if (mLanded) {
		End();
		return true;
	}
	mEndRequested = true;
	return false;
}

// 0x40b330: every kill counts as a pea's; the song starts; the ball (the picture of the bonus) is put on the level's
// object layer and starts to lower; the backdrop darkens; the bugs are frozen
void BonusDisco::Start()
{
	BugObj::SetAlwaysPeaHit(true);
	mCountdown = DISCO_FRAMES;
	mLanded = false;
	mEndRequested = false;
	mEnded = false;
	strcpy(mName, "DISCO");
	Bonus::Start();
	sApp->mSoundQueue->Request(SOUND_DISCOSONG, 0, true);
	mMusicOn = true;
	sDiscoBallAnim->AddRef();
	mAnim = sDiscoBallAnim;
	SetParent(static_cast<LevelObj*>(sLevel)->mObjectLayer, nullptr);
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, DISCO_Z);
	SetStateInt(STATE_ALPHA, 1);
	SetState(STATE_SCALE_X, DISCO_SCALE);
	SetState(STATE_SCALE_Y, DISCO_SCALE);
	SetStateInt(STATE_FRAME, 0);
	SetStateInt(STATE_ROTATION, 0);
	sApp->mGameWidget->mBoard->mBackDrop->Darken();
	mDarkened = true;
	StartTrack(mLowerTrack);
	BugObj::FreezeAll();
}

// 0x40b450: the ball's three tracks. Each key's cel counts up, one every 33 frames, round the six cels of the picture.
// The first lowers it (from 300 above its place to 20 above, in 200 frames) and its event 4 says it has landed. The
// second holds it there, turning. The third lifts it (back to 300 above, in 120 frames) and its event 5 puts the bonus
// away
void BonusDisco::BuildTracks()
{
	AnimKey* aLowerKey = new AnimKey();
	aLowerKey->SetDuration(DISCO_LOWER_FRAMES);
	aLowerKey->AddValue(STATE_Y, DISCO_HIGH_Y, MOD_TOWARD_NEXT);
	aLowerKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DISCO_CEL_RATE, DISCO_CEL_COUNT, 0.0f, nullptr);
	aLowerKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aLandedKey = new AnimKey();
	aLandedKey->SetDuration(1);
	aLandedKey->mEventId = DISCO_EVENT_LANDED;
	aLandedKey->AddValue(STATE_Y, DISCO_LOW_Y, MOD_TOWARD_NEXT);
	aLandedKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DISCO_CEL_RATE, DISCO_CEL_COUNT, 0.0f, nullptr);
	AnimTrack* aLowerTrack = new AnimTrack();
	aLowerTrack->AddKey(aLowerKey, nullptr);
	aLowerTrack->AddKey(aLandedKey, nullptr);
	mLowerTrack = AddTrack(aLowerTrack, nullptr);

	AnimKey* aTurnKey = new AnimKey();
	aTurnKey->SetDuration(DISCO_TURN_FRAMES);
	aTurnKey->AddValue(STATE_Y, DISCO_LOW_Y, MOD_TOWARD_NEXT);
	aTurnKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DISCO_CEL_RATE, DISCO_CEL_COUNT, 0.0f, nullptr);
	AnimTrack* aSpinTrack = new AnimTrack();
	aSpinTrack->AddKey(aTurnKey, aTurnKey);
	mSpinTrack = AddTrack(aSpinTrack, nullptr);

	AnimKey* aRiseKey = new AnimKey();
	aRiseKey->SetDuration(DISCO_RISE_FRAMES);
	aRiseKey->AddValue(STATE_Y, DISCO_LOW_Y, MOD_TOWARD_NEXT);
	aRiseKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DISCO_CEL_RATE, DISCO_CEL_COUNT, 0.0f, nullptr);
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->SetDuration(1);
	aGoneKey->mEventId = DISCO_EVENT_GONE;
	aGoneKey->AddValue(STATE_Y, DISCO_HIGH_Y, MOD_TOWARD_NEXT);
	aGoneKey->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, DISCO_CEL_RATE, DISCO_CEL_COUNT, 0.0f, nullptr);
	AnimTrack* aRiseTrack = new AnimTrack();
	aRiseTrack->AddKey(aRiseKey, nullptr);
	aRiseTrack->AddKey(aGoneKey, nullptr);
	mRiseTrack = AddTrack(aRiseTrack, nullptr);
}
