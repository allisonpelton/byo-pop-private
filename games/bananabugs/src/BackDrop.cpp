#include "BackDrop.h"

#include "Anim.h"
#include "Player.h"

#include <cstdio>

using namespace Sexy;

// The background each map level shows; the sixth and seventh levels have each other's
static const int BACKDROP_OF_LEVEL[] = { 1, 2, 3, 4, 5, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };

// How dark the darkened colour is, taken from each channel
const float BACKDROP_DARKNESS = -0.65f;

BackDrop* BackDrop::sCurrent = nullptr;

// 0x403f70: behind everything at depth 0.1, with a colour of its own for its children. The original also names the
// object "BackDrop" in a field nothing reads.
BackDrop::BackDrop()
{
	mBackground = BACKDROP_OF_LEVEL[Player::sCurrent->mMapLevel];
	SetParent(nullptr, nullptr);
	SetState(STATE_Z, 0.1f);
	mChildColorOwn = true;
	mDarkened = false;
	sprintf(mPath, "images/backdrops/background%02d", mBackground);
	mAnim = Anim::Get(mPath, ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	BuildTracks();
	ObjBase::Activate(this, false);
}

// 0x404120
BackDrop::~BackDrop()
{
	sCurrent = nullptr;
	if (mAnim != nullptr)
		delete mAnim;
	mAnim = nullptr;
}

// 0x4041b0
void BackDrop::Darken()
{
	if (!mDarkened) {
		StartTrack(mDarkenTrack);
		mDarkened = true;
	}
}

// 0x4041d0
void BackDrop::Undarken()
{
	if (mDarkened) {
		StartTrack(mUndarkenTrack);
		mDarkened = false;
	}
}

// 0x4041f0: the first track rests; darkening takes 200 frames and holds, and undarkening takes 200 back and rests
void BackDrop::BuildTracks()
{
	AnimKey* aRest = new AnimKey();
	aRest->SetDuration(50);
	aRest->AddValue(STATE_RED, 0.0f, MOD_HOLD);
	aRest->AddValue(STATE_GREEN, 0.0f, MOD_HOLD);
	aRest->AddValue(STATE_BLUE, 0.0f, MOD_HOLD);
	AnimTrack* aRestTrack = new AnimTrack();
	aRestTrack->AddKey(aRest, aRest);
	AddTrack(aRestTrack, nullptr);

	AnimKey* aDarkening = new AnimKey();
	aDarkening->SetDuration(200);
	aDarkening->AddValue(STATE_RED, 0.0f, MOD_TOWARD_NEXT);
	aDarkening->AddValue(STATE_GREEN, 0.0f, MOD_TOWARD_NEXT);
	aDarkening->AddValue(STATE_BLUE, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aDark = new AnimKey();
	aDark->SetDuration(25);
	aDark->AddValue(STATE_RED, BACKDROP_DARKNESS, MOD_HOLD);
	aDark->AddValue(STATE_GREEN, BACKDROP_DARKNESS, MOD_HOLD);
	aDark->AddValue(STATE_BLUE, BACKDROP_DARKNESS, MOD_HOLD);
	AnimTrack* aDarkenTrack = new AnimTrack();
	aDarkenTrack->AddKey(aDarkening, nullptr);
	aDarkenTrack->AddKey(aDark, aDark);
	mDarkenTrack = AddTrack(aDarkenTrack, nullptr);

	AnimKey* aLightening = new AnimKey();
	aLightening->SetDuration(200);
	aLightening->AddValue(STATE_RED, BACKDROP_DARKNESS, MOD_TOWARD_NEXT);
	aLightening->AddValue(STATE_GREEN, BACKDROP_DARKNESS, MOD_TOWARD_NEXT);
	aLightening->AddValue(STATE_BLUE, BACKDROP_DARKNESS, MOD_TOWARD_NEXT);
	AnimKey* aLight = new AnimKey();
	aLight->SetDuration(1);
	aLight->AddValue(STATE_RED, 0.0f, MOD_HOLD);
	aLight->AddValue(STATE_GREEN, 0.0f, MOD_HOLD);
	aLight->AddValue(STATE_BLUE, 0.0f, MOD_HOLD);
	AnimTrack* anUndarkenTrack = new AnimTrack();
	anUndarkenTrack->AddKey(aLightening, nullptr);
	anUndarkenTrack->AddKey(aLight, nullptr);
	mUndarkenTrack = AddTrack(anUndarkenTrack, aRestTrack);
}

// 0x4045b0: the ground stands on the window's bottom edge, 10 pixels low and a little wider than the window, swaying a
// pixel to the side and back as it flattens a little, 200 frames each way
void BackDrop::BuildGround()
{
	char aPath[40];
	sprintf(aPath, "images/backdrops/ground_%02d", mBackground);

	AnimKey* aSettled = new AnimKey();
	aSettled->SetDuration(200);
	aSettled->AddValue(STATE_OFFSET_X, 0.0f, MOD_TOWARD_NEXT);
	aSettled->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwayed = new AnimKey();
	aSwayed->SetDuration(200);
	aSwayed->AddValue(STATE_OFFSET_X, 1.0f, MOD_TOWARD_NEXT);
	aSwayed->AddValue(STATE_SCALE_Y, -0.01f, MOD_TOWARD_NEXT);
	AnimTrack* aSwayTrack = new AnimTrack();
	aSwayTrack->AddKey(aSettled, nullptr);
	aSwayTrack->AddKey(aSwayed, aSettled);

	mGround = new ObjBase();
	mGround->SetParent(this, nullptr);
	mGround->SetStateInt(STATE_Z, 50);
	mGround->AddTrack(aSwayTrack, nullptr);
	mGround->mAnim = Anim::Get(aPath, ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	float aY = (float)((600.0 - mGround->mAnim->GetSourceCelHeight() * 0.5) + 10.0);
	mGround->SetState(STATE_Y, aY);
	mGround->SetState(STATE_SCALE_X, 1.055f);
	mGround->Activate(mGround, false);
}

// 0x4047e0
BackDrop* BackDrop::Create(ObjBase* theParent)
{
	BackDrop* aBackDrop = new BackDrop();
	sCurrent = aBackDrop;
	aBackDrop->SetParent(theParent, nullptr);
	return aBackDrop;
}
