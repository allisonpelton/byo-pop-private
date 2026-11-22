#include "BonusFlyPaper.h"

#include "Anim.h"
#include "BugObj.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* BonusFlyPaper::sPaperAnim = nullptr;

// The key events of the paper's tracks (the base's events 1 and 2 are not used by the paper)
enum PaperEvent
{
	PAPER_EVENT_LANDED = 5,  // the paper has landed: bugs over it are held
	PAPER_EVENT_GONE = 7     // the paper has faded out: the bonus is put away
};

// Where the held bonus rides, relative to its holder (design pixels)
const int PAPER_HELD_X = 64;

// The zap area's mode that holds the bugs in it (BugObj's ZAP_HOLD), and the box the landed paper holds them in: 40
// either side of its place, 45 above and 43 below
const int PAPER_ZAP_HOLD = 1;
const double PAPER_BOX_HALF_WIDTH = 40.0;
const double PAPER_BOX_ABOVE = 45.0;
const double PAPER_BOX_BELOW = 43.0;

// 0x40c700: the fields are as operator new left them in the original; the port clears them
BonusFlyPaper::BonusFlyPaper()
{
	mHeldTrack = 0;
	mDropTrack = 0;
	mEndTrack = 0;
	mUnused2e4 = 0;
}

static void GiveAnim(AnimKey* theKey, Anim* theAnim)
{
	theAnim->AddRef();
	theKey->mAnim = theAnim;
}

// 0x40c740
void BonusFlyPaper::Setup(int theKind)
{
	Bonus::Setup(theKind);
	BuildTracks();
}

// 0x40c760 (shared with BonusBomb): a bonus that is not waiting stands upright
bool BonusFlyPaper::Update()
{
	return UpdateUpright();
}

// 0x40c790: the landing (event 5) sets the area bugs are held in; the end of the fade (event 7) puts the bonus away
void BonusFlyPaper::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr) {
		int anEvent = theObj->mCurrentKey->mEventId;
		if (anEvent == PAPER_EVENT_LANDED) {
			BugObj::SetZapArea(PAPER_ZAP_HOLD,
				(float)((double)mScreenX - PAPER_BOX_HALF_WIDTH),
				(float)((double)mScreenY - PAPER_BOX_ABOVE),
				(float)((double)mScreenX + PAPER_BOX_HALF_WIDTH),
				(float)((double)mScreenY + PAPER_BOX_BELOW));
		} else if (anEvent == PAPER_EVENT_GONE) {
			Reset();
		} else {
			Bonus::Notify(theObj);
		}
	}
}

// 0x40c840: the bugs take the commands they were held for
void BonusFlyPaper::Reset()
{
	BugObj::ReleaseAll();
	Player::Drop();
	Bonus::Reset();
}

// 0x40c860: the paper fades out
void BonusFlyPaper::End()
{
	StartTrack(mEndTrack);
}

// 0x412570
bool BonusFlyPaper::EndWhenBugsGone()
{
	return EndOnLastBug();
}

// 0x40c870: the monkey takes the paper
void BonusFlyPaper::Start()
{
	strcpy(mName, "CLICK TO DROP FLYPAPER");
	sApp->mSoundQueue->Request(SOUND_BONUSGET, 0, false);
	Bonus::Start();
	SetStateInt(STATE_X, PAPER_HELD_X);
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

// 0x40c930: the paper's three tracks (every key shows the paper; the alpha and scales of a key are added to the
// object's own, so -1 clears and 0 leaves them as they are). Held, it appears growing in and then sways a little each
// way for ever. Dropped, it flies a short way along its angle (the orbit growing to 0.3 pixels), shrinks as it lands
// (event 5: the zap area is set then) and lies there. The end track fades it out (event 7 puts the bonus away)
void BonusFlyPaper::BuildTracks()
{
	AnimKey* anAppearKey = new AnimKey();
	GiveAnim(anAppearKey, sPaperAnim);
	anAppearKey->SetDuration(5);
	anAppearKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_X, -1.0f, MOD_TOWARD_NEXT);
	anAppearKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayKey = new AnimKey();
	GiveAnim(aSwayKey, sPaperAnim);
	aSwayKey->SetDuration(0x4b);
	aSwayKey->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_X, -0.51f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_SCALE_Y, -0.5f, MOD_TOWARD_NEXT);
	aSwayKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aSwayKey->mEasing = EASE_COSINE;
	AnimKey* aSwayBackKey = new AnimKey();
	GiveAnim(aSwayBackKey, sPaperAnim);
	aSwayBackKey->SetDuration(0x41);
	aSwayBackKey->AddValue(STATE_ALPHA, -0.25f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_X, -0.5f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_SCALE_Y, -0.51f, MOD_TOWARD_NEXT);
	aSwayBackKey->AddValue(STATE_ROTATION, 0.0349065848f, MOD_TOWARD_NEXT);
	aSwayBackKey->mEasing = EASE_COSINE;
	AnimTrack* aHeldTrack = new AnimTrack();
	aHeldTrack->AddKey(anAppearKey, nullptr);
	aHeldTrack->AddKey(aSwayKey, nullptr);
	aHeldTrack->AddKey(aSwayBackKey, aSwayKey);
	mHeldTrack = AddTrack(aHeldTrack, nullptr);

	AnimKey* aFlyKey = new AnimKey();
	GiveAnim(aFlyKey, sPaperAnim);
	aFlyKey->SetDuration(0x32);
	aFlyKey->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aFlyKey->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aFlyKey->AddValue(STATE_SCALE_X, -0.5f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_SCALE_Y, -0.5f, MOD_TOWARD_NEXT);
	aFlyKey->AddValue(STATE_ORBIT_RADIUS, 0.3f, MOD_TOWARD_NEXT);
	AnimKey* aLandKey = new AnimKey();
	aLandKey->mEventId = PAPER_EVENT_LANDED;
	GiveAnim(aLandKey, sPaperAnim);
	aLandKey->SetDuration(5);
	aLandKey->AddValue(STATE_ALPHA, -0.53f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_SCALE_X, 0.01f, MOD_TOWARD_NEXT);
	aLandKey->AddValue(STATE_SCALE_Y, 0.01f, MOD_TOWARD_NEXT);
	AnimKey* aLiesKey = new AnimKey();
	GiveAnim(aLiesKey, sPaperAnim);
	aLiesKey->SetDuration(10);
	aLiesKey->AddValue(STATE_ALPHA, -0.5f, MOD_HOLD);
	aLiesKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aLiesKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aDropTrack = new AnimTrack();
	aDropTrack->AddKey(aFlyKey, nullptr);
	aDropTrack->AddKey(aLandKey, nullptr);
	aDropTrack->AddKey(aLiesKey, aLiesKey);
	mDropTrack = AddTrack(aDropTrack, nullptr);

	AnimKey* aFadeKey = new AnimKey();
	GiveAnim(aFadeKey, sPaperAnim);
	aFadeKey->SetDuration(0xc);
	aFadeKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGoneKey = new AnimKey();
	GiveAnim(aGoneKey, sPaperAnim);
	aGoneKey->mEventId = PAPER_EVENT_GONE;
	aGoneKey->SetDuration(1);
	aGoneKey->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aGoneKey->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	AnimTrack* anEndTrack = new AnimTrack();
	anEndTrack->AddKey(aFadeKey, nullptr);
	anEndTrack->AddKey(aGoneKey, nullptr);
	mEndTrack = AddTrack(anEndTrack, nullptr);
}
