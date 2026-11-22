#include "BonusSwat.h"

#include "Anim.h"
#include "BugObj.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusSwat::sSwatterAnim = nullptr;

// The key events of the swatter's tracks (the base's events 1 and 2 are not used by the swatter)
enum SwatEvent
{
	SWAT_EVENT_BLOW = 5,     // the swatter comes down: the bugs under it die
	SWAT_EVENT_GONE = 6      // the fade out is over: the bonus is put away
};

// How many swings the monkey is given (what Player::PickUp is told it may put down)
const int SWAT_SWINGS = 3;

// The blow falls this far from the swatter's place, in the way the swatter was turned (Bonus::mRotation 1 to 4: right,
// down, left, up), and kills the bugs within this reach of that place (design pixels)
const float SWAT_BLOW_DISTANCE = 110.0f;
const float SWAT_BLOW_REACH = 96.0f;

// 0x410e10: the kind is given by Setup
BonusSwat::BonusSwat()
{
	mUnused2e4 = 0;
	for (int i = 0; i < 6; i++)
		mSwingKeys[i] = nullptr;
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x410e50
void BonusSwat::Setup(int theKind)
{
	Bonus::Setup(theKind);
	BuildTracks();
}

// 0x410e80: the blow (event 5) kills the bugs where the swatter was turned to, and the swings are over once none are
// left to use or a bug has been killed; the fade out's end (event 6) puts the bonus away
void BonusSwat::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEvent = theObj->mCurrentKey->mEventId;
	if (anEvent != SWAT_EVENT_BLOW) {
		if (anEvent != SWAT_EVENT_GONE)
			Bonus::Notify(theObj);
		else
			Reset();
		return;
	}
	float anOffsetX;
	float anOffsetY;
	if (mRotation == 1) {
		anOffsetX = SWAT_BLOW_DISTANCE;
		anOffsetY = 0.0f;
	} else if (mRotation == 2) {
		anOffsetX = 0.0f;
		anOffsetY = SWAT_BLOW_DISTANCE;
	} else if (mRotation == 3) {
		anOffsetX = -SWAT_BLOW_DISTANCE;
		anOffsetY = 0.0f;
	} else {
		anOffsetX = 0.0f;
		anOffsetY = -SWAT_BLOW_DISTANCE;
	}
	bool aKilled = BugObj::ZapNear(mScreenX + anOffsetX, mScreenY + anOffsetY, SWAT_BLOW_REACH);
	if (aKilled)
		sApp->mSoundQueue->Request(SOUND_FLYSWAT, 0, false);
	if (Player::sCurrent->mHeldCount < 1 || aKilled)
		End();
}

// 0x412530 (shared with BonusZapper)
void BonusSwat::Reset()
{
	DropAndReset();
}

// 0x410f90: the monkey takes the swatter. (The original sets the vertical scale and not the horizontal one.)
void BonusSwat::Start()
{
	strcpy(mName, "CLICK TO SWAT BUGS");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, 0);
	SetStateInt(STATE_Y, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_ALPHA, 1);
	SetStateInt(STATE_FRAME, 0);
	SetStateInt(STATE_ROTATION, 0);
	SetStateInt(STATE_OFFSET_X, 0);
	Player::PickUp(this, SWAT_SWINGS);
	BecomeHeld(true);
	Activate(this, false);
	StartTrack(mHeldTrack);
}

// 0x411080: the swing; it is not a drop, the swatter stays where it is held
bool BonusSwat::Place()
{
	if (mState != BONUS_HELD)
		return false;
	SetState(STATE_ROTATION, 0.0f);
	StartTrack(mDropTrack);
	ComputeDrawn();
	return true;
}

// 0x4110c0: the swatter's three tracks (every key shows the swatter; the scales and the turn are added to the object's
// own, the offset puts the swatter beside the monkey). Held: it appears growing in, then sways a little each way for
// ever. The swing: it is raised (10 frames), tilts back (10), comes down fast (15, 6) and strikes at its fifth key
// (event 5: the swatter is 20% bigger and turned flat), stays (5) and shrinks back (1 frame). The fade out: 8 frames,
// and a last key of one frame that is the event 6
void BonusSwat::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sSwatterAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sSwatterAnim);
	aSwayKey->SetDuration(0x4b);
	aSwayKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_X, 0.6f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	aSwayKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sSwatterAnim);
	aSwayBackKey->SetDuration(0x41);
	aSwayBackKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_X, 0.6f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349065848f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	aSwayBackKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_TOWARD_NEXT);
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aRaiseKey = new AnimKey();
	GiveAnim(aRaiseKey, sSwatterAnim);
	aRaiseKey->SetDuration(10);
	aRaiseKey->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aRaiseKey->AddValue(STATE_SCALE_X, 0.6f, MOD_TOWARD_NEXT);
	aRaiseKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_TOWARD_NEXT);
	aRaiseKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_TOWARD_NEXT);
	aRaiseKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aTiltKey = new AnimKey();
	GiveAnim(aTiltKey, sSwatterAnim);
	aTiltKey->SetDuration(10);
	aTiltKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aTiltKey->AddValue(STATE_SCALE_Y, 0.7f, MOD_TOWARD_NEXT);
	aTiltKey->AddValue(STATE_OFFSET_X, 0.0f, MOD_TOWARD_NEXT);
	aTiltKey->AddValue(STATE_ROTATION, -0.0872664675f, MOD_TOWARD_NEXT);
	AnimKey* aLiftKey = new AnimKey();
	GiveAnim(aLiftKey, sSwatterAnim);
	aLiftKey->SetDuration(0xf);
	aLiftKey->AddValue(STATE_SCALE_X, -0.6f, MOD_TOWARD_NEXT);
	aLiftKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_TOWARD_NEXT);
	aLiftKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_TOWARD_NEXT);
	aLiftKey->AddValue(STATE_ROTATION, -0.34906587f, MOD_TOWARD_NEXT);
	AnimKey* aDownKey = new AnimKey();
	GiveAnim(aDownKey, sSwatterAnim);
	aDownKey->SetDuration(6);
	aDownKey->AddValue(STATE_SCALE_X, -0.8f, MOD_TOWARD_NEXT);
	aDownKey->AddValue(STATE_SCALE_Y, 0.8f, MOD_TOWARD_NEXT);
	aDownKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_TOWARD_NEXT);
	aDownKey->AddValue(STATE_ROTATION, -0.34906587f, MOD_TOWARD_NEXT);
	AnimKey* aBlowKey = new AnimKey();
	GiveAnim(aBlowKey, sSwatterAnim);
	aBlowKey->mEventId = SWAT_EVENT_BLOW;
	aBlowKey->SetDuration(6);
	aBlowKey->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aBlowKey->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	aBlowKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_HOLD);
	aBlowKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aStayKey = new AnimKey();
	GiveAnim(aStayKey, sSwatterAnim);
	aStayKey->SetDuration(5);
	aStayKey->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aStayKey->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	aStayKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_HOLD);
	aStayKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aBackKey = new AnimKey();
	GiveAnim(aBackKey, sSwatterAnim);
	aBackKey->SetDuration(1);
	aBackKey->AddValue(STATE_SCALE_X, 0.6f, MOD_TOWARD_NEXT);
	aBackKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_TOWARD_NEXT);
	aBackKey->AddValue(STATE_OFFSET_X, 100.0f, MOD_HOLD);
	aBackKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimTrack* aSwingTrack = new AnimTrack();
	aSwingTrack->AddKey(aRaiseKey, nullptr);
	aSwingTrack->AddKey(aTiltKey, nullptr);
	aSwingTrack->AddKey(aLiftKey, nullptr);
	aSwingTrack->AddKey(aDownKey, nullptr);
	aSwingTrack->AddKey(aBlowKey, nullptr);
	aSwingTrack->AddKey(aStayKey, nullptr);
	aSwingTrack->AddKey(aBackKey, aBackKey);
	mDropTrack = AddTrack(aSwingTrack, nullptr);
	mSwingKeys[0] = aRaiseKey;
	mSwingKeys[1] = aTiltKey;
	mSwingKeys[2] = aLiftKey;
	mSwingKeys[3] = aDownKey;
	mSwingKeys[4] = aBlowKey;
	mSwingKeys[5] = aStayKey;

	AnimKey* aFadeKey = new AnimKey();
	GiveAnim(aFadeKey, sSwatterAnim);
	aFadeKey->SetDuration(8);
	aFadeKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_X, 0.6f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_Y, 0.6f, MOD_TOWARD_NEXT);
	AnimKey* aGoneKey = new AnimKey();
	aGoneKey->mEventId = SWAT_EVENT_GONE;
	GiveAnim(aGoneKey, sSwatterAnim);
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	aGoneKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGoneKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aEndTrack = new AnimTrack();
	aEndTrack->AddKey(aFadeKey, nullptr);
	aEndTrack->AddKey(aGoneKey, nullptr);
	mEndTrack = AddTrack(aEndTrack, nullptr);
}
