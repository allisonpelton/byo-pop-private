#include "TextFade.h"

#include "GameApp.h"

using namespace Sexy;

// The mode each way of playing has
enum TextFadeMode
{
	TEXTFADE_MODE_GROW = 0,
	TEXTFADE_MODE_SPIN_SHORT = 1,
	TEXTFADE_MODE_SPIN_LONG = 2,
};

const int TEXTFADE_HOLD_FRAMES = 100;
const float TEXTFADE_SPIN_TURNS = 9.424778f; // three pi, the spin-in keys' rotation

// 0x46f5a0: the placeholder text is 32 zeros, for the cels each of its letters gets
TextFade::TextFade()
{
	SetFont(gApp->mFont);
	SetStateInt(STATE_Z, 200);
	Create("00000000000000000000000000000000", false, 0);
	mClickSkips = false;
	mSkipping = false;
	mMode = TEXTFADE_MODE_GROW;
	mExtraHold = 0;
	mClickQuickOut = 0; // the original leaves it as the allocator gave it until SetMode sets it
	BuildMode0Keys();
	BuildMode1Keys();
	BuildMode2Keys();
	Deactivate(this);
}

// 0x46f650 (body 0x46f670)
TextFade::~TextFade()
{
}

// 0x46f680: once the click has set mSkipping, each key that starts is cut to one frame
void TextFade::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && mSkipping)
		theObj->mCurrentKey->SetDuration(1);
}

// 0x46f6b0: a click (while mClickSkips) sets mSkipping, and a mode 2 text starts its quick track out
bool TextFade::Update()
{
	if (!ObjBase::Update())
		return false;
	if (sMouseHeld) {
		if (mClickSkips)
			mSkipping = true;
		if (mClickQuickOut != 0)
			StartTrack(mQuickOutTrackIndex);
	}
	return true;
}

// 0x46f6f0
void TextFade::Hide()
{
	if (IsActive())
		Deactivate(this);
}

// 0x46f710 (EAX the text, ECX the mode)
void TextFade::SetMode(int theMode)
{
	mClickSkips = false;
	mMode = theMode;
	if (theMode == TEXTFADE_MODE_SPIN_LONG)
		mClickQuickOut = 1;
}

// 0x46f730
void TextFade::SetExtraHold(int theFrames)
{
	mExtraHold = theFrames;
}

// 0x46f740 (EAX the text): the colour and place, the scale each pulse of the mode swells to (a little more than
// theScale, then theScale, in turn; the mode 0 hold and the key after it are set the same way), and the delay before
// the mode's track starts
void TextFade::Start(const char* theText, float theX, float theY, float theScale, float theRed, float theGreen,
	float theBlue, int theDelay)
{
	SetState(STATE_X, theX);
	SetState(STATE_Y, theY);
	mSkipping = false;
	sMouseHeld = false;
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetState(STATE_RED, theRed);
	SetState(STATE_GREEN, theGreen);
	SetState(STATE_BLUE, theBlue);
	SetStateInt(STATE_ALPHA, 0);
	if (mMode == TEXTFADE_MODE_SPIN_SHORT) {
		float aBigScale = (float)(0.2 + (double)theScale);
		mMode1Keys[1]->mValue[STATE_SCALE_X] = aBigScale;
		mMode1Keys[1]->mValue[STATE_SCALE_Y] = aBigScale;
		mMode1Keys[2]->mValue[STATE_SCALE_X] = theScale;
		mMode1Keys[2]->mValue[STATE_SCALE_Y] = theScale;
		mMode1Keys[3]->mValue[STATE_SCALE_X] = aBigScale;
		mMode1Keys[3]->mValue[STATE_SCALE_Y] = aBigScale;
		mMode1Keys[4]->mValue[STATE_SCALE_X] = theScale;
		mMode1Keys[4]->mValue[STATE_SCALE_Y] = theScale;
		mMode1Keys[5]->mValue[STATE_SCALE_X] = aBigScale;
		mMode1Keys[5]->mValue[STATE_SCALE_Y] = aBigScale;
	} else if (mMode == TEXTFADE_MODE_SPIN_LONG) {
		float aBigScale = (float)(0.1 + (double)theScale);
		mMode2Keys[1]->mValue[STATE_SCALE_X] = aBigScale;
		mMode2Keys[1]->mValue[STATE_SCALE_Y] = aBigScale;
		mMode2Keys[2]->mValue[STATE_SCALE_X] = theScale;
		mMode2Keys[2]->mValue[STATE_SCALE_Y] = theScale;
		mMode2Keys[3]->mValue[STATE_SCALE_X] = aBigScale;
		mMode2Keys[3]->mValue[STATE_SCALE_Y] = aBigScale;
		mMode2Keys[4]->mValue[STATE_SCALE_X] = theScale;
		mMode2Keys[4]->mValue[STATE_SCALE_Y] = theScale;
		mMode2Keys[5]->mValue[STATE_SCALE_X] = aBigScale;
		mMode2Keys[5]->mValue[STATE_SCALE_Y] = aBigScale;
	} else {
		mMode0Keys[1]->SetDuration(mExtraHold + TEXTFADE_HOLD_FRAMES);
		mExtraHold = 0;
		mMode0Keys[1]->mValue[STATE_SCALE_X] = theScale;
		mMode0Keys[1]->mValue[STATE_SCALE_Y] = theScale;
		float aSmallScale = (float)((double)theScale - 0.1);
		mMode0Keys[2]->mValue[STATE_SCALE_X] = aSmallScale;
		mMode0Keys[2]->mValue[STATE_SCALE_Y] = aSmallScale;
	}
	mModeTracks[mMode]->SetDelay(theDelay, false);
	Activate(this, true);
	SetText(theText);
	StartTrack(mModeTrackIndex[mMode]);
}

// 0x46f960: mode 0. It grows from nothing over 40 frames (the scale to 0.3 of the way, wobbling on a sine wave, easing
// in), holds 100 frames, takes 20 to shrink a little and fade, and ends in a key of no time.
void TextFade::BuildMode0Keys()
{
	AnimKey* aGrow = new AnimKey();
	mMode0Keys[0] = aGrow;
	aGrow->SetDuration(0x28);
	aGrow->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aGrow->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aGrow->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aGrow->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aGrow->mEasing = EASE_IN_CUBE;
	AnimKey* aHold = new AnimKey();
	mMode0Keys[1] = aHold;
	aHold->SetDuration(100);
	aHold->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aHold->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aHold->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aShrink = new AnimKey();
	mMode0Keys[2] = aShrink;
	aShrink->SetDuration(0x14);
	aShrink->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_X, 0.9f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_Y, 0.9f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	mMode0Keys[3] = aGone;
	aGone->SetDuration(0);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	mModeTracks[0] = aTrack;
	aTrack->AddKey(mMode0Keys[0], nullptr);
	aTrack->AddKey(mMode0Keys[1], nullptr);
	aTrack->AddKey(mMode0Keys[2], nullptr);
	aTrack->AddKey(mMode0Keys[3], nullptr);
	mModeTrackIndex[0] = AddTrack(aTrack, nullptr);
}

// 0x46fd10: mode 1. It spins in over 60 frames (three pi of rotation, the same wobbling grow), then pulses between the
// scale Start sets and a little more, each pulse of 30 and 20 frames, 25 for the last, and ends in a key of no time.
void TextFade::BuildMode1Keys()
{
	AnimKey* aSpinIn = new AnimKey();
	mMode1Keys[0] = aSpinIn;
	aSpinIn->SetDuration(0x3c);
	aSpinIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSpinIn->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aSpinIn->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSpinIn->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aSpinIn->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSpinIn->AddValue(STATE_ROTATION, TEXTFADE_SPIN_TURNS, MOD_TOWARD_NEXT);
	aSpinIn->mEasing = EASE_IN_CUBE;
	AnimKey* aPulseUp = new AnimKey();
	mMode1Keys[1] = aPulseUp;
	aPulseUp->SetDuration(0x1e);
	aPulseUp->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseUp->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aPulseUp->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	aPulseUp->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aPulseDown = new AnimKey();
	mMode1Keys[2] = aPulseDown;
	aPulseDown->SetDuration(0x14);
	aPulseDown->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aPulseUp2 = new AnimKey();
	mMode1Keys[3] = aPulseUp2;
	aPulseUp2->SetDuration(0x1e);
	aPulseUp2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseUp2->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aPulseUp2->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aPulseDown2 = new AnimKey();
	mMode1Keys[4] = aPulseDown2;
	aPulseDown2->SetDuration(0x14);
	aPulseDown2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown2->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown2->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aPulseUp3 = new AnimKey();
	mMode1Keys[5] = aPulseUp3;
	aPulseUp3->SetDuration(0x19);
	aPulseUp3->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseUp3->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aPulseUp3->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	mMode1Keys[6] = aGone;
	aGone->SetDuration(0);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	mModeTracks[1] = aTrack;
	aTrack->AddKey(mMode1Keys[0], nullptr);
	aTrack->AddKey(mMode1Keys[1], nullptr);
	aTrack->AddKey(mMode1Keys[2], nullptr);
	aTrack->AddKey(mMode1Keys[3], nullptr);
	aTrack->AddKey(mMode1Keys[4], nullptr);
	aTrack->AddKey(mMode1Keys[5], nullptr);
	aTrack->AddKey(mMode1Keys[6], nullptr);
	mModeTrackIndex[1] = AddTrack(aTrack, nullptr);
}

// 0x470360: mode 2. It spins in over 30 frames, then pulses on and on (the last pulse's next key is the one before
// it, so the last two keys repeat). The track out, for a click, takes the scale and alpha from where they are (the
// scale kept) down to nothing over 30 frames.
void TextFade::BuildMode2Keys()
{
	AnimKey* aSpinIn = new AnimKey();
	mMode2Keys[0] = aSpinIn;
	aSpinIn->SetDuration(0x1e);
	aSpinIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSpinIn->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aSpinIn->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSpinIn->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aSpinIn->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSpinIn->AddValue(STATE_ROTATION, TEXTFADE_SPIN_TURNS, MOD_TOWARD_NEXT);
	aSpinIn->mEasing = EASE_IN_CUBE;
	AnimKey* aPulseUp = new AnimKey();
	mMode2Keys[1] = aPulseUp;
	aPulseUp->SetDuration(0x32);
	aPulseUp->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseUp->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aPulseUp->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	aPulseUp->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aPulseDown = new AnimKey();
	mMode2Keys[2] = aPulseDown;
	aPulseDown->SetDuration(0x2d);
	aPulseDown->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aPulseUp2 = new AnimKey();
	mMode2Keys[3] = aPulseUp2;
	aPulseUp2->SetDuration(0x32);
	aPulseUp2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseUp2->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aPulseUp2->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	AnimKey* aPulseDown2 = new AnimKey();
	mMode2Keys[4] = aPulseDown2;
	aPulseDown2->SetDuration(0x2d);
	aPulseDown2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown2->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aPulseDown2->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aPulseUp3 = new AnimKey();
	mMode2Keys[5] = aPulseUp3;
	aPulseUp3->SetDuration(0x32);
	aPulseUp3->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aPulseUp3->AddValue(STATE_SCALE_X, 1.1f, MOD_TOWARD_NEXT);
	aPulseUp3->AddValue(STATE_SCALE_Y, 1.1f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	mModeTracks[2] = aTrack;
	aTrack->AddKey(mMode2Keys[0], nullptr);
	aTrack->AddKey(mMode2Keys[1], nullptr);
	aTrack->AddKey(mMode2Keys[2], nullptr);
	aTrack->AddKey(mMode2Keys[3], nullptr);
	aTrack->AddKey(mMode2Keys[4], nullptr);
	aTrack->AddKey(mMode2Keys[5], mMode2Keys[4]);
	mModeTrackIndex[2] = AddTrack(aTrack, nullptr);

	AnimKey* aQuickOut = new AnimKey();
	aQuickOut->SetDuration(0x1e);
	aQuickOut->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aQuickOut->AddModifier(STATE_SCALE_X, true, 1.0f, MOD_TOWARD_NEXT, 0.0f, 0.0f, 0.0f, nullptr);
	aQuickOut->AddModifier(STATE_SCALE_Y, true, 1.0f, MOD_TOWARD_NEXT, 0.0f, 0.0f, 0.0f, nullptr);
	AnimKey* aQuickGone = new AnimKey();
	aQuickGone->SetDuration(0);
	aQuickGone->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aQuickGone->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aQuickGone->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	AnimTrack* aQuickTrack = new AnimTrack();
	aQuickTrack->AddKey(aQuickOut, nullptr);
	aQuickTrack->AddKey(aQuickGone, nullptr);
	mQuickOutTrackIndex = AddTrack(aQuickTrack, nullptr);
}
