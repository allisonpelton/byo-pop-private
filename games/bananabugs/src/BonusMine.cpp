#include "BonusMine.h"

#include "Anim.h"
#include "BugObj.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "StarStream.h"

#include <cstring>

using namespace Sexy;

Anim* BonusMine::sMineAnim = nullptr;

// The key events of the mine's tracks (the base's events 1 and 2 are not used by the mine)
enum MineEvent
{
	MINE_EVENT_ARMED = 5,    // the mine has buried itself: a bug that comes near sets it off
	MINE_EVENT_BURST = 7,    // the mine has swelled: it bursts
	MINE_EVENT_DONE = 8      // the wait after the burst is over: the bonus is put away
};

// Where the held bonus rides, relative to its holder (design pixels)
const int MINE_HELD_X = 64;

// How near a bug must come to set an armed mine off, and how far its burst kills (design pixels)
const float MINE_TRIGGER_REACH = 70.0f;
const float MINE_BLAST_REACH = 256.0f;

// The ring of the burst: 36 stars, 10 degrees apart, of the kind that sparks blue slowly
const int MINE_STAR_DEGREES_APART = 10;
const int MINE_STAR_DEGREES_ROUND = 0x168;

// 0x40d7c0: the fields are as operator new left them in the original (only the armed flag is cleared); the port
// clears them all
BonusMine::BonusMine()
{
	mTriggerTrack = 0;
	mBurstTrack = 0;
	mArmed = false;
	mUnused2ec = 0;
	for (int i = 0; i < MINE_STARS; i++)
		mStars[i] = nullptr;
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x40d810: the tracks, and the 36 stars as children of the mine
void BonusMine::Setup(int theKind)
{
	Bonus::Setup(theKind);
	BuildTracks();
	for (int i = 0; i < MINE_STARS; i++) {
		mStars[i] = new StarStream(STAR_STREAM_SLOW_BLUE);
		mStars[i]->SetParent(this, nullptr);
	}
}

// 0x40d8b0: the mine stands upright whatever it is doing (the turn it was given is taken from its animated value
// (+0x12c) and from the drawn value (+0x1bc)). An armed mine swells as soon as a bug is within reach, and that bug dies.
// (The original starts the track again each frame a bug is still near.)
bool BonusMine::Update()
{
	if (!Bonus::Update())
		return false;
	ObjBase::mState[STATE_ROTATION] = 0.0f;
	mDrawn[STATE_ROTATION] = 0.0f;
	if (mArmed) {
		if (BugObj::ZapFirstNear(mScreenX, mScreenY, MINE_TRIGGER_REACH) != nullptr)
			StartTrack(mTriggerTrack);
	}
	return true;
}

// 0x40d910: the landing arms the mine (event 5); the swell's end (event 7) is the burst: the mine is hidden, every bug
// within reach dies, the ring of stars flies out and the wait starts; the wait's end (event 8) puts the bonus away
void BonusMine::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == MINE_EVENT_ARMED) {
		mArmed = true;
	} else if (anEvent == MINE_EVENT_BURST) {
		SetHidden(true);
		SetStateInt(STATE_SCALE_X, 1);
		SetStateInt(STATE_SCALE_Y, 1);
		BugObj::ZapNear(mScreenX, mScreenY, MINE_BLAST_REACH);
		sApp->mSoundQueue->Request(SOUND_CANNON, 0, false);
		int aDegrees = 0;
		int i = 0;
		do {
			mStars[i]->Burst(0.0f, 0.0f, aDegrees);
			aDegrees += MINE_STAR_DEGREES_APART;
			i++;
		} while (aDegrees < MINE_STAR_DEGREES_ROUND);
		StartTrack(mBurstTrack);
		mArmed = false;
	} else if (anEvent == MINE_EVENT_DONE) {
		Reset();
	} else {
		Bonus::Notify(theObj);
	}
}

// 0x40da10
void BonusMine::Reset()
{
	mArmed = false;
	Player::Drop();
	Bonus::Reset();
}

// 0x40da30: the monkey takes the mine
void BonusMine::Start()
{
	strcpy(mName, "CLICK TO DROP MINE");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, MINE_HELD_X);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_Z, 1);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	Player::PickUp(this, 1);
	BecomeHeld(false);
	Activate(this, false);
	StartTrack(mHeldTrack);
}

// 0x40dc30: the mine's four tracks. Held (every key shows the mine, the alpha and scales added to the object's own):
// it appears growing in, then turns a little each way for ever. Dropped: it flies a short way along its angle (the orbit
// growing to 1 pixel), then takes 0.1 more scale in 5 frames as it buries itself (event 5) and lies there. The trigger
// track (no picture of its own: the mine's stays) swells it, 12 frames and 5 more, and its last key clears it with the
// burst (event 7); the burst track waits 340 frames and ends with event 8
void BonusMine::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sMineAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sMineAnim);
	aSwayKey->SetDuration(0x4b);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sMineAnim);
	aSwayBackKey->SetDuration(0x41);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349065848f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aFlyKey = new AnimKey();
	GiveAnim(aFlyKey, sMineAnim);
	aFlyKey->SetDuration(0x28);
	aFlyKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlyKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlyKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aBuryKey = new AnimKey();
	aBuryKey->mEventId = MINE_EVENT_ARMED;
	GiveAnim(aBuryKey, sMineAnim);
	aBuryKey->SetDuration(5);
	aBuryKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aBuryKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aBuryKey->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aBuryKey->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimKey* aLiesKey = new AnimKey();
	GiveAnim(aLiesKey, sMineAnim);
	aLiesKey->SetDuration(10);
	aLiesKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aLiesKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aLiesKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aDropTrack = new AnimTrack();
	aDropTrack->AddKey(aFlyKey, nullptr);
	aDropTrack->AddKey(aBuryKey, nullptr);
	aDropTrack->AddKey(aLiesKey, aLiesKey);
	mDropTrack = AddTrack(aDropTrack, nullptr);

	AnimKey* aHoldKey = new AnimKey();
	aHoldKey->SetDuration(0xc);
	aHoldKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aHoldKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aHoldKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwellKey = new AnimKey();
	aSwellKey->SetDuration(5);
	aSwellKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aSwellKey->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aSwellKey->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimKey* aVanishKey = new AnimKey();
	aVanishKey->mEventId = MINE_EVENT_BURST;
	aVanishKey->SetDuration(1);
	aVanishKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aVanishKey->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aVanishKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimTrack* aTriggerTrack = new AnimTrack();
	aTriggerTrack->AddKey(aHoldKey, nullptr);
	aTriggerTrack->AddKey(aSwellKey, nullptr);
	aTriggerTrack->AddKey(aVanishKey, nullptr);
	mTriggerTrack = AddTrack(aTriggerTrack, nullptr);

	AnimKey* aWaitKey = new AnimKey();
	aWaitKey->SetDuration(0x154);
	aWaitKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aWaitKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aWaitKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimKey* aDoneKey = new AnimKey();
	aDoneKey->mEventId = MINE_EVENT_DONE;
	aDoneKey->SetDuration(1);
	aDoneKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aDoneKey->AddValue(STATE_SCALE_X, 0.1f, MOD_HOLD);
	aDoneKey->AddValue(STATE_SCALE_Y, 0.1f, MOD_HOLD);
	AnimTrack* aBurstTrack = new AnimTrack();
	aBurstTrack->AddKey(aWaitKey, nullptr);
	aBurstTrack->AddKey(aDoneKey, nullptr);
	mBurstTrack = AddTrack(aBurstTrack, nullptr);
}
