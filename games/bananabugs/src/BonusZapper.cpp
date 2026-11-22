#include "BonusZapper.h"

#include "Anim.h"
#include "BugObj.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusZapper::sZapperAnim = nullptr;

// The key events of the zapper's tracks (the base's events 1 and 2 are not used by the zapper)
enum ZapperEvent
{
	ZAPPER_EVENT_LANDED = 5, // the zapper has landed: the zap area is set
	ZAPPER_EVENT_GONE = 7    // the fade out is over: the bonus is put away
};

const int ZAPPER_ZAP_KILL = 2;                  // BugObj::SetZapArea's mode: the bugs in the area die
const double ZAPPER_BOX_HALF_WIDTH = 40.0;      // the zap area, around the zapper
const double ZAPPER_BOX_HALF_HEIGHT = 50.0;
const int ZAPPER_HELD_X = 0x40;                 // where the monkey holds it

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x4123e0: the kind is given by Setup
BonusZapper::BonusZapper()
{
	mEndTrack = 0;
}

// 0x412420
void BonusZapper::Setup(int theKind)
{
	Bonus::Setup(theKind);
	BuildTracks();
}

// 0x412440: a bonus that is not waiting stands upright, and the light flickers (the frame is picked at random, one of two)
bool BonusZapper::Update()
{
	if (!Bonus::Update())
		return false;
	if (Bonus::mState != BONUS_WAITING) {
		ObjBase::mState[STATE_ROTATION] = 0.0f;
		mDrawn[STATE_ROTATION] = 0.0f;
	}
	SetStateInt(STATE_FRAME, GameRandomNext() & 1);
	return true;
}

// 0x412480: the landing (event 5) sets the area the bugs are zapped in; the end of the fade (event 7) puts the bonus away
void BonusZapper::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent == ZAPPER_EVENT_LANDED) {
		BugObj::SetZapArea(ZAPPER_ZAP_KILL,
			(float)((double)mScreenX - ZAPPER_BOX_HALF_WIDTH),
			(float)((double)mScreenY - ZAPPER_BOX_HALF_HEIGHT),
			(float)((double)mScreenX + ZAPPER_BOX_HALF_WIDTH),
			(float)((double)mScreenY + ZAPPER_BOX_HALF_HEIGHT));
	} else if (anEvent == ZAPPER_EVENT_GONE) {
		Reset();
	} else {
		Bonus::Notify(theObj);
	}
}

// 0x412530 (shared with BonusSwat)
void BonusZapper::Reset()
{
	DropAndReset();
}

// 0x412550: it is not the fly paper's End: only a running bonus fades out
void BonusZapper::End()
{
	if (IsActive())
		StartTrack(mEndTrack);
}

// 0x412570
bool BonusZapper::EndWhenBugsGone()
{
	return EndOnLastBug();
}

// 0x412580: the monkey takes the zapper
void BonusZapper::Start()
{
	strcpy(mName, "CLICK TO DROP ZAPPER");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, ZAPPER_HELD_X);
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

// 0x412670: the dropper's Place without its sound
bool BonusZapper::Place()
{
	if (mState != BONUS_HELD)
		return false;
	return PutDown(mDropTrack);
}

// 0x412770: the zapper's three tracks (every key shows the zapper; the alpha and scales of a key are added to the
// object's own, so -1 clears and 0 leaves them as they are). Held: it appears growing in and then sways a little each
// way for ever. Dropped: it flies a short way along its angle (the orbit's radius is 1), shrinks, and its second key
// (event 5) sets the zap area; its last key holds (it points to itself). The fade out: 20 frames, and a last key of one
// frame that is the event 7
void BonusZapper::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sZapperAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sZapperAnim);
	aSwayKey->SetDuration(0x4b);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_X, -0.4f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_Y, -0.4f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sZapperAnim);
	aSwayBackKey->SetDuration(0x41);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_X, -0.4f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_Y, -0.4f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349065848f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aFlyKey = new AnimKey();
	GiveAnim(aFlyKey, sZapperAnim);
	aFlyKey->SetDuration(0x28);
	aFlyKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlyKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlyKey->AddValue(STATE_SCALE_X, -0.4f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_SCALE_Y, -0.4f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_ORBIT_RADIUS, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aLandKey = new AnimKey();
	aLandKey->mEventId = ZAPPER_EVENT_LANDED;
	GiveAnim(aLandKey, sZapperAnim);
	aLandKey->SetDuration(5);
	aLandKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_SCALE_X, 0.01f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_SCALE_Y, 0.01f, MOD_TOWARD_NEXT);
	AnimKey* aRestKey = new AnimKey();
	GiveAnim(aRestKey, sZapperAnim);
	aRestKey->SetDuration(10);
	aRestKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aRestKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aRestKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aRestKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aDropTrack = new AnimTrack();
	aDropTrack->AddKey(aFlyKey, nullptr);
	aDropTrack->AddKey(aLandKey, nullptr);
	aDropTrack->AddKey(aRestKey, aRestKey);
	mDropTrack = AddTrack(aDropTrack, nullptr);

	AnimKey* aFadeKey = new AnimKey();
	GiveAnim(aFadeKey, sZapperAnim);
	aFadeKey->SetDuration(0x14);
	aFadeKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_ORBIT_RADIUS, 3.0f, MOD_TOWARD_NEXT);
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->mEventId = ZAPPER_EVENT_GONE;
	GiveAnim(aGoneKey, sZapperAnim);
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aGoneKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aGoneKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	aGoneKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimTrack* anEndTrack = new AnimTrack();
	anEndTrack->AddKey(aFadeKey, nullptr);
	anEndTrack->AddKey(aGoneKey, nullptr);
	mEndTrack = AddTrack(anEndTrack, nullptr);
}
