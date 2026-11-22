#include "BugImg.h"

#include "Anim.h"

using namespace Sexy;

Anim* BugImg::sSpinStarsAnim = nullptr;
Anim* BugImg::sShockRingAnim = nullptr;

// The stars' cel, as the counter modifier turns it: 24 cels, a new one every 30 steps of the rate
static const float STARS_CEL_RATE = 30.0f;
static const float STARS_CEL_COUNT = 24.0f;
// The stars hang above the body (a child's place is the body's), in front of it
static const int STARS_OFFSET_Y = -32;
static const int EFFECT_DEPTH = 5;
// The shock ring's turn over its 200 frames (as a float widened: 268.7807 radians)
static const float RING_TURN = 268.7807f;

// 0x41f3d0: the two effect children, made hidden
BugImg::BugImg()
{
	mFrozen = false; // port: the original never sets the byte (see the header)
	BuildStars();
	BuildRing();
}

// 0x41f430 (0x41f450 is the body): the children belong to this body's scene and go with it
BugImg::~BugImg()
{
}

// 0x41f460: the stars or the ring start their track and show. Every other command is the kind's
void BugImg::Slot13(int theCommand, int theFrames, int theUnused)
{
	(void)theFrames;
	(void)theUnused;
	if (theCommand == BUG_CMD_STARS_ON) {
		mSpinStars->Activate(mSpinStars, false);
		mSpinStars->StartTrack(mStarsTrack);
	} else if (theCommand == BUG_CMD_RING_ON) {
		mShockRing->Activate(mShockRing, false);
		mShockRing->StartTrack(mRingTrack);
	}
}

// 0x41f4c0: the stars: 24 cels turning on, fading in over 5 frames, shown for 235, then fading out over 10
void BugImg::BuildStars()
{
	mSpinStars = new ObjBase();
	sSpinStarsAnim->AddRef();
	mSpinStars->mAnim = sSpinStarsAnim;
	mSpinStars->SetParent(this, nullptr);
	mSpinStars->SetStateInt(STATE_ALPHA, 0);
	mSpinStars->SetStateInt(STATE_Y, STARS_OFFSET_Y);
	mSpinStars->SetStateInt(STATE_Z, EFFECT_DEPTH);

	AnimKey* aFadeIn = new AnimKey();
	aFadeIn->SetDuration(5);
	aFadeIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFadeIn->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, STARS_CEL_RATE, STARS_CEL_COUNT, 0.0f, nullptr);

	AnimKey* aShown = new AnimKey();
	aShown->SetDuration(0xeb);
	aShown->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aShown->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, STARS_CEL_RATE, STARS_CEL_COUNT, 0.0f, nullptr);

	AnimKey* aFadeOut = new AnimKey();
	aFadeOut->SetDuration(10);
	aFadeOut->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aFadeOut->AddModifier(STATE_FRAME, false, 0.0f, MOD_COUNT_UP, STARS_CEL_RATE, STARS_CEL_COUNT, 0.0f, nullptr);

	// port (leak): the original makes a fourth key here (1 frame, alpha 0, the same counter) that no track holds, and
	// never frees it; it is not made
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFadeIn, nullptr);
	aTrack->AddKey(aShown, nullptr);
	aTrack->AddKey(aFadeOut, nullptr);
	mStarsTrack = mSpinStars->AddTrack(aTrack, nullptr);
	mSpinStars->Deactivate(mSpinStars);
}

// 0x41f7a0: the shock ring: a half-size ring, mostly clear, that jitters at random and turns for 200 frames
void BugImg::BuildRing()
{
	mShockRing = new ObjBase();
	sShockRingAnim->AddRef();
	mShockRing->mAnim = sShockRingAnim;
	mShockRing->SetParent(this, nullptr);
	mShockRing->SetState(STATE_ALPHA, 0.8f);
	mShockRing->SetStateInt(STATE_Z, EFFECT_DEPTH);
	mShockRing->SetState(STATE_SCALE_X, 0.5f);
	mShockRing->SetState(STATE_SCALE_Y, 0.5f);

	AnimKey* aTurn = new AnimKey();
	aTurn->SetDuration(200);
	aTurn->AddModifier(STATE_OFFSET_X, false, 0.0f, MOD_RANDOM, 2.0f, -1.0f, 1.0f, nullptr);
	aTurn->AddModifier(STATE_OFFSET_Y, false, 0.0f, MOD_RANDOM, 2.0f, -1.0f, 1.0f, nullptr);
	aTurn->AddValue(STATE_ROTATION, RING_TURN, MOD_TOWARD_NEXT);

	AnimKey* aEnd = new AnimKey();
	aEnd->SetDuration(1);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aTurn, nullptr);
	aTrack->AddKey(aEnd, nullptr);
	mRingTrack = mShockRing->AddTrack(aTrack, nullptr);
	mShockRing->Deactivate(mShockRing);
}

// 0x41f980: the pictures of the stars (60 by 28, 24 cels) and of the ring
void Sexy::PreloadSpinStars()
{
	BugImg::sSpinStarsAnim = Anim::Get("images/spinstars", ANIM_SCALE_SMOOTH, false, 0x3c, 0x1c, 0x18, false);
	BugImg::sShockRingAnim = Anim::Get("images/monkey_shock_ring", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
}
