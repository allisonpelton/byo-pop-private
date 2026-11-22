#include "Transition.h"

#include "Anim.h"
#include "Cursor.h"
#include "GameApp.h"
#include "Res.h"

using namespace Sexy;

int Transition::sPhase = TRANSITION_OPEN;
int Transition::sLoadingCel = 1;

// The keys' event ids (AnimKey::mEventId)
enum TransitionEvent
{
	TRANSITION_EVENT_DOOR_OPEN = 0xca,
	TRANSITION_EVENT_DOOR_SHUT = 0xcb,
	TRANSITION_EVENT_MONKEY_DONE = 300,
};

// The doors' tracks
enum DoorTrack
{
	DOOR_TRACK_SHUT = 0,
	DOOR_TRACK_CLOSE = 1,
	DOOR_TRACK_OPEN = 2,
};

// 0x470a60
Transition::Transition()
{
	SetParent(nullptr, nullptr);
	SetState(STATE_Z, 500.0f);
	mMonkeyAnim = Anim::Get("images/monkey_trans", ANIM_SCALE_SMOOTH, true, 0, 0, 1, false);
	mDoorAnim = Anim::Get("images/door_trans", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mLoadingTextAnim = Anim::Get("images/loading_text", ANIM_SCALE_SMOOTH, false, 0x528, 0xaf, 2, false);
	sPhase = TRANSITION_OPEN;
	mDoorsArrived = 0;
	mShowingLoading = false;
	mPattern = 0;
	mMonkeyFullScale = (float)gApp->mWidth / 100.0f;
	mKeyTime = 0.0f;
	mTopOpenY = -728.0f;
	mBottomOpenY = 728.0f;
	BuildMonkey();
	BuildDoors();
	BuildLoadingText();
	Activate(this, true);
}

// 0x470bb0. The original keeps its count on the loading text's Anim; the port lets it go too.
Transition::~Transition()
{
	mMonkeyAnim->Release();
	mDoorAnim->Release();
	mLoadingTextAnim->Release();
	for (int i = 0; i < TRANSITION_DOORS; i++) {
		mTopDoors[i]->Delete();
		mBottomDoors[i]->Delete();
	}
}

// 0x470c50: the move ends once all ten doors have arrived and the monkey, if shown, is done
void Transition::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	switch (theObj->mCurrentKey->mEventId) {
	case TRANSITION_EVENT_DOOR_OPEN:
		mDoorsArrived++;
		break;
	case TRANSITION_EVENT_DOOR_SHUT:
		mDoorsArrived++;
		theObj->StartTrack(DOOR_TRACK_SHUT);
		break;
	case TRANSITION_EVENT_MONKEY_DONE:
		mMonkeyDone = true;
		break;
	}
	if (sPhase == TRANSITION_OPENING) {
		if (9 < mDoorsArrived && mMonkeyDone)
			sPhase = TRANSITION_OPEN;
	} else if (sPhase == TRANSITION_CLOSING && 9 < mDoorsArrived && mMonkeyDone) {
		sPhase = TRANSITION_CLOSED;
	}
}

// One top door and the bottom door under it start their move after theDelay frames; closing doors start from the
// open place
void Transition::StartDoorPair(int theIndex, bool opening, int theDelay)
{
	int aTrack = opening ? DOOR_TRACK_OPEN : DOOR_TRACK_CLOSE;
	ObjBase* aTop = mTopDoors[theIndex];
	aTop->Activate(aTop, false);
	aTop->mTracks[aTrack]->SetDelay(theDelay, false);
	aTop->StartTrack(aTrack);
	if (!opening) {
		aTop->mState[STATE_Y] = mTopOpenY;
		aTop->ComputeDrawn();
	}
	ObjBase* aBottom = mBottomDoors[theIndex];
	aBottom->Activate(aBottom, false);
	aBottom->mTracks[aTrack]->SetDelay(theDelay, false);
	aBottom->StartTrack(aTrack);
	if (!opening) {
		aBottom->mState[STATE_Y] = mBottomOpenY;
		aBottom->ComputeDrawn();
	}
}

// 0x470cf0: the five patterns stagger the door pairs left to right, right to left, alternately, from the middle out,
// and from the edges in
void Transition::StartDoors(bool opening, int theDelay)
{
	switch (mPattern) {
	case 0:
		for (int i = 0; i < TRANSITION_DOORS; i++)
			StartDoorPair(i, opening, theDelay + i * 10);
		break;
	case 1:
		for (int i = 0; i < TRANSITION_DOORS; i++)
			StartDoorPair(i, opening, theDelay + 50 - i * 10);
		break;
	case 2:
		for (int i = 0; i < TRANSITION_DOORS; i++)
			StartDoorPair(i, opening, theDelay + ((i & 1) + 1) * 25);
		break;
	case 3:
		for (int aFromMiddle = 2, i = 0; -3 < aFromMiddle; aFromMiddle--, i++)
			StartDoorPair(i, opening, theDelay + (aFromMiddle < 0 ? -aFromMiddle : aFromMiddle) * 20 + 10);
		break;
	case 4:
		for (int aFromMiddle = 2, i = 0; -3 < aFromMiddle; aFromMiddle--, i++)
			StartDoorPair(i, opening, theDelay + (4 - (aFromMiddle < 0 ? -aFromMiddle : aFromMiddle)) * 25);
		break;
	}
	mPattern = (mPattern + 1) % 5;
}

// 0x471130: the doors close; the monkey grows over them (the doors wait 20 frames for it), and the loading text
// spins in
void Transition::Close(bool withMonkey, bool showLoading)
{
	Cursor::Hide();
	sPhase = TRANSITION_CLOSING;
	mDoorsArrived = 0;
	mWithMonkey = withMonkey;
	mShowingLoading = showLoading;
	if (showLoading) {
		mLoadingText->Activate(mLoadingText, true);
		mLoadingText->StartTrack(0);
		mLoadingText->SetStateInt(STATE_FRAME, sLoadingCel);
		Activate(mLoadingText, false);
		sLoadingCel = 1;
	}
	if (withMonkey) {
		mMonkey->Activate(mMonkey, false);
		mMonkey->StartTrack(0);
		mMonkeyDone = false;
		StartDoors(false, 20);
		return;
	}
	mMonkeyDone = true;
	StartDoors(false, 0);
}

// 0x471200: the loading text fades, the doors part (after 10 frames with the monkey, which shrinks away)
void Transition::Open(bool withMonkey)
{
	if (mShowingLoading) {
		mShowingLoading = false;
		mLoadingText->StartTrack(1);
	}
	sPhase = TRANSITION_OPENING;
	mDoorsArrived = 0;
	mWithMonkey = withMonkey;
	if (withMonkey) {
		mMonkey->Activate(mMonkey, false);
		mMonkey->StartTrack(1);
		mMonkeyDone = false;
		StartDoors(true, 10);
		return;
	}
	mMonkeyDone = true;
	StartDoors(true, 0);
}

// 0x4712a0: the monkey grows from 0.18 to the window's width over 170 frames as the doors close, and shrinks back
// over 140 as they open
void Transition::BuildMonkey()
{
	mMonkey = new ObjBase();
	mMonkey->mAnim = mMonkeyAnim;
	mMonkeyAnim->AddRef();
	mMonkey->SetEventTarget(this);
	mMonkey->SetParent(this, nullptr);
	mMonkey->SetStateInt(STATE_Z, 1);
	mMonkey->SetStateInt(STATE_SCALE_X, 0);
	mMonkey->SetStateInt(STATE_SCALE_Y, 0);

	AnimKey* aGrow = new AnimKey();
	aGrow->mEventId = 0;
	aGrow->SetDuration(170);
	aGrow->AddValue(STATE_SCALE_X, 0.18f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 0.18f, MOD_TOWARD_NEXT);
	aGrow->mEasing = EASE_IN_SQUARE;
	AnimKey* aGrown = new AnimKey();
	aGrown->mEventId = TRANSITION_EVENT_MONKEY_DONE;
	aGrown->SetDuration(0);
	aGrown->AddValue(STATE_SCALE_X, mMonkeyFullScale, MOD_TOWARD_NEXT);
	aGrown->AddValue(STATE_SCALE_Y, mMonkeyFullScale, MOD_TOWARD_NEXT);
	AnimTrack* aGrowTrack = new AnimTrack();
	aGrowTrack->AddKey(aGrow, nullptr);
	aGrowTrack->AddKey(aGrown, nullptr);
	mMonkey->AddTrack(aGrowTrack, nullptr);

	AnimKey* aShrink = new AnimKey();
	aShrink->mEventId = 0;
	aShrink->SetDuration(140);
	aShrink->AddValue(STATE_SCALE_X, mMonkeyFullScale, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_Y, mMonkeyFullScale, MOD_TOWARD_NEXT);
	aShrink->mEasing = EASE_IN_SQUARE;
	AnimKey* aShrunk = new AnimKey();
	aShrunk->mEventId = TRANSITION_EVENT_MONKEY_DONE;
	aShrunk->SetDuration(0);
	aShrunk->AddValue(STATE_SCALE_X, 0.18f, MOD_TOWARD_NEXT);
	aShrunk->AddValue(STATE_SCALE_Y, 0.18f, MOD_TOWARD_NEXT);
	AnimTrack* aShrinkTrack = new AnimTrack();
	aShrinkTrack->AddKey(aShrink, nullptr);
	aShrinkTrack->AddKey(aShrunk, nullptr);
	mMonkey->AddTrack(aShrinkTrack, nullptr);
}

// 0x4715d0: the doors, one cel wide each side by side from the screen's left edge, toothed up and down by turns.
// Each has a track that holds it shut, one that closes it over 40 frames (the top ones with the door's sound) and one
// that opens it over 40.
void Transition::BuildDoors()
{
	for (int i = 0; i < TRANSITION_DOORS; i++) {
		ObjBase* aDoor = new ObjBase();
		mTopDoors[i] = aDoor;
		aDoor->SetParent(nullptr, nullptr);
		aDoor->SetEventTarget(this);
		aDoor->mAnim = mDoorAnim;
		mDoorAnim->AddRef();
		aDoor->SetState(STATE_OFFSET_X, aDoor->mAnim->GetSourceCelWidth() * 0.5f);
		aDoor->SetState(STATE_OFFSET_Y, 33.0f - aDoor->mAnim->GetSourceCelHeight() * 0.5f);
		aDoor->SetState(STATE_X, aDoor->mAnim->GetSourceCelWidth() * (float)i - 960.0f);
		aDoor->SetStateInt(STATE_Y, (i & 1) == 0 ? 0x40 : -0x40);
		aDoor->SetState(STATE_Z, (float)((double)i * 0.01 + 503.0));

		AnimKey* aShut = new AnimKey();
		aShut->mEventId = 200;
		aShut->SetDuration(1);
		aShut->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aShut->mEasing = EASE_IN_SQUARE;
		AnimTrack* aShutTrack = new AnimTrack();
		aShutTrack->AddKey(aShut, aShut);
		aDoor->AddTrack(aShutTrack, nullptr);

		AnimKey* aClose = new AnimKey();
		aClose->SetDuration(40);
		aClose->AddValue(STATE_Y, mTopOpenY, MOD_TOWARD_NEXT);
		aClose->mEasing = EASE_IN_SQUARE;
		AnimKey* aBang = new AnimKey();
		aBang->SetDuration(2);
		aBang->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		aBang->mEndCondition = KEY_SOUND;
		aBang->mEndChannel = SOUND_DOORSHUT;
		aBang->mEndValue = 1.0f;
		AnimKey* aClosed = new AnimKey();
		aClosed->mEventId = TRANSITION_EVENT_DOOR_SHUT;
		aClosed->SetDuration(1);
		aClosed->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		AnimTrack* aCloseTrack = new AnimTrack();
		aCloseTrack->AddKey(aClose, nullptr);
		aCloseTrack->AddKey(aBang, nullptr);
		aCloseTrack->AddKey(aClosed, nullptr);
		aDoor->AddTrack(aCloseTrack, nullptr);

		AnimKey* anOpen = new AnimKey();
		anOpen->SetDuration(40);
		anOpen->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* anOpened = new AnimKey();
		anOpened->mEventId = TRANSITION_EVENT_DOOR_OPEN;
		anOpened->SetDuration(1);
		anOpened->AddValue(STATE_Y, mTopOpenY, MOD_TOWARD_NEXT);
		AnimTrack* anOpenTrack = new AnimTrack();
		anOpenTrack->AddKey(anOpen, nullptr);
		anOpenTrack->AddKey(anOpened, nullptr);
		aDoor->AddTrack(anOpenTrack, nullptr);
	}

	for (int i = 0; i < TRANSITION_DOORS; i++) {
		ObjBase* aDoor = new ObjBase();
		mBottomDoors[i] = aDoor;
		aDoor->SetParent(nullptr, nullptr);
		aDoor->SetEventTarget(this);
		aDoor->mAnim = mDoorAnim;
		mDoorAnim->AddRef();
		aDoor->SetState(STATE_OFFSET_X, aDoor->mAnim->GetSourceCelWidth() * 0.5f);
		aDoor->SetState(STATE_OFFSET_Y, aDoor->mAnim->GetSourceCelHeight() * 0.5f - 1.0f);
		aDoor->SetState(STATE_X, aDoor->mAnim->GetSourceCelWidth() * (float)i - 960.0f);
		aDoor->SetStateInt(STATE_Y, (i & 1) == 0 ? 0x3f : -0x3f);
		aDoor->SetState(STATE_Z, (float)((double)i * 0.01 + 502.0));

		AnimKey* aShut = new AnimKey();
		aShut->mEventId = 200;
		aShut->SetDuration(0);
		aShut->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimTrack* aShutTrack = new AnimTrack();
		aShutTrack->AddKey(aShut, aShut);
		aDoor->AddTrack(aShutTrack, nullptr);

		AnimKey* aClose = new AnimKey();
		aClose->SetDuration(40);
		aClose->AddValue(STATE_Y, mBottomOpenY, MOD_TOWARD_NEXT);
		aClose->mEasing = EASE_IN_SQUARE;
		AnimKey* aSettle = new AnimKey();
		aSettle->SetDuration(10);
		aSettle->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		AnimKey* aClosed = new AnimKey();
		aClosed->mEventId = TRANSITION_EVENT_DOOR_SHUT;
		aClosed->SetDuration(1);
		aClosed->AddValue(STATE_Y, 0.0f, MOD_HOLD);
		AnimTrack* aCloseTrack = new AnimTrack();
		aCloseTrack->AddKey(aClose, nullptr);
		aCloseTrack->AddKey(aSettle, nullptr);
		aCloseTrack->AddKey(aClosed, nullptr);
		aDoor->AddTrack(aCloseTrack, nullptr);

		AnimKey* anOpen = new AnimKey();
		anOpen->SetDuration(40);
		anOpen->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		anOpen->mEasing = EASE_IN_SQUARE;
		AnimKey* anOpened = new AnimKey();
		anOpened->mEventId = TRANSITION_EVENT_DOOR_OPEN;
		anOpened->SetDuration(1);
		anOpened->AddValue(STATE_Y, mBottomOpenY, MOD_TOWARD_NEXT);
		AnimTrack* anOpenTrack = new AnimTrack();
		anOpenTrack->AddKey(anOpen, nullptr);
		anOpenTrack->AddKey(anOpened, nullptr);
		aDoor->AddTrack(anOpenTrack, nullptr);
	}
}

// 0x471d80: "loading" spins a full turn in as it grows over 55 frames (after 15), settles over 10 and holds; its
// second track fades it out over 60
void Transition::BuildLoadingText()
{
	mLoadingText = new ObjBase();
	mLoadingText->SetParent(this, nullptr);
	mLoadingText->mAnim = mLoadingTextAnim;
	mLoadingTextAnim->AddRef();
	mLoadingText->SetState(STATE_X, 0.0f);
	mLoadingText->SetState(STATE_Y, 0.0f);
	mLoadingText->SetStateInt(STATE_Z, 200);
	mLoadingText->SetStateInt(STATE_SCALE_X, 0);
	mLoadingText->SetStateInt(STATE_SCALE_Y, 0);
	mLoadingText->SetStateInt(STATE_ALPHA, 0);

	AnimKey* aSpinIn = new AnimKey();
	aSpinIn->SetDuration(55);
	aSpinIn->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aSpinIn->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aSpinIn->AddModifier(STATE_SCALE_X, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSpinIn->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aSpinIn->AddModifier(STATE_SCALE_Y, false, 0.0f, MOD_SINE, 1.0f, 1.3f, 0.0f, nullptr);
	aSpinIn->AddValue(STATE_ROTATION, -6.2831855f, MOD_TOWARD_NEXT);
	AnimKey* aSettled = new AnimKey();
	aSettled->SetDuration(10);
	aSettled->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aSettled->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aSettled->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	aSettled->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimTrack* aShowTrack = new AnimTrack();
	aShowTrack->AddKey(aSpinIn, nullptr);
	aShowTrack->AddKey(aSettled, aSettled);
	aShowTrack->SetDelay(15, false);
	mLoadingText->AddTrack(aShowTrack, nullptr);

	AnimKey* aFade = new AnimKey();
	aFade->SetDuration(60);
	aFade->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aFade->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->SetDuration(0);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aFadeTrack = new AnimTrack();
	aFadeTrack->AddKey(aFade, nullptr);
	aFadeTrack->AddKey(aGone, nullptr);
	mLoadingText->AddTrack(aFadeTrack, nullptr);
	mLoadingText->Deactivate(mLoadingText);
}
