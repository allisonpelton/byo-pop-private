#include "PopCap.h"

#include "Anim.h"
#include "GameApp.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

// The keys' event ids (AnimKey::mEventId)
enum PopCapEvent
{
	POPCAP_EVENT_HELD = 1,
	POPCAP_EVENT_POP = 2,
	POPCAP_EVENT_GONE = 3,
};

// 0x45e8d0
PopCap::PopCap()
{
	SetParent(nullptr, nullptr);
	mHasPartner = 1 < strlen(gApp->mPartnerImage);
	mPhase = POPCAP_SHOWING;
	BuildLogoTracks();
	BuildPartnerTrack();
}

// 0x45e980
PopCap::~PopCap()
{
	if (mAnim != nullptr)
		delete mAnim;
	mAnim = nullptr;
}

// 0x45ea00: the logo's keys report here. After the logo, the partner's logo (scaled 1:1) fades in on the partner
// track; after that, or with no partner, the screen is done.
void PopCap::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case POPCAP_EVENT_HELD:
		StartTrack(1);
		mPhase = POPCAP_FADING;
		break;
	case POPCAP_EVENT_POP:
		gApp->mSoundQueue->Request(SOUND_POPCORN, 0, false);
		break;
	case POPCAP_EVENT_GONE:
		if (mHasPartner) {
			if (mAnim != nullptr)
				delete mAnim;
			// in 3D the original scales the partner's logo by the desktop's height / 600 instead
			SetStateInt(STATE_SCALE_X, 1);
			SetStateInt(STATE_SCALE_Y, 1);
			mAnim = Anim::Get(gApp->mPartnerImage, ANIM_SCALE_SMOOTH, true, 0, 0, 1, true);
			StartTrack(mPartnerTrack);
			mHasPartner = false;
		} else {
			mPhase = POPCAP_DONE;
		}
		break;
	}
}

// 0x45eb30: the partner's logo fades in over 45 frames, holds 100, fades out over 45
void PopCap::BuildPartnerTrack()
{
	AnimKey* aFadeIn = new AnimKey();
	aFadeIn->SetDuration(45);
	aFadeIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aHold = new AnimKey();
	aHold->SetDuration(100);
	aHold->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aFadeOut = new AnimKey();
	aFadeOut->SetDuration(45);
	aFadeOut->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->mEventId = POPCAP_EVENT_GONE;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFadeIn, nullptr);
	aTrack->AddKey(aHold, nullptr);
	aTrack->AddKey(aFadeOut, nullptr);
	aTrack->AddKey(aGone, nullptr);
	mPartnerTrack = AddTrack(aTrack, nullptr);
}

// 0x45ecc0: PopCap's logo grows from nothing over 35 frames, swells on a sine over the next 35, holds for 200 (the
// pop sound as the hold starts), then fades out over 30 on the second track
void PopCap::BuildLogoTracks()
{
	mAnim = Anim::Get("images/init/PopCap_Logo", ANIM_SCALE_SMOOTH, false, 0, 0, 1, true);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_ALPHA, 0);

	AnimKey* aGrow = new AnimKey();
	aGrow->SetDuration(35);
	aGrow->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwell = new AnimKey();
	aSwell->SetDuration(35);
	aSwell->AddValue(STATE_SCALE_X, 0.5f, MOD_TOWARD_NEXT);
	aSwell->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSwell->AddValue(STATE_SCALE_Y, 0.5f, MOD_TOWARD_NEXT);
	aSwell->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSwell->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	AnimKey* aHold = new AnimKey();
	aHold->mEventId = POPCAP_EVENT_POP;
	aHold->SetDuration(200);
	aHold->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aHold->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aHold->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aHeld = new AnimKey();
	aHeld->mEventId = POPCAP_EVENT_HELD;
	aHeld->SetDuration(1);
	aHeld->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aHeld->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aHeld->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aShow = new AnimTrack();
	aShow->AddKey(aGrow, nullptr);
	aShow->AddKey(aSwell, nullptr);
	aShow->AddKey(aHold, nullptr);
	aShow->AddKey(aHeld, aHeld);
	AddTrack(aShow, nullptr);

	AnimKey* aFade = new AnimKey();
	aFade->SetDuration(30);
	aFade->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->mEventId = POPCAP_EVENT_GONE;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aFadeTrack = new AnimTrack();
	aFadeTrack->AddKey(aFade, nullptr);
	aFadeTrack->AddKey(aGone, aGone);
	AddTrack(aFadeTrack, nullptr);
	Activate(this, false);
}
