#include "BBWindow.h"

#include "Anim.h"
#include "GameApp.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

int BBWindow::sOpenCount = 0;

// The window's buttons are of a type of their own, and their ids tell them apart
const int OBJ_TYPE_WINDOW_BUTTON = 0xf;
const int BUTTON_ID_FIRST = 100;
const int BUTTON_ID_SECOND = 0x65;

// The keys' event ids (AnimKey::mEventId)
const int BBWINDOW_EVENT_CLOSED = 1;

// The buttons' tracks
enum ButtonTrack
{
	BUTTON_TRACK_REST = 0,
	BUTTON_TRACK_HOVER = 1,
	BUTTON_TRACK_LEAVE = 2,
};

// 0x4270e0
BBWindow::BBWindow()
{
	sOpenCount++;
	mSavedHoverMinZ = 0.0f;
	mState = BBWINDOW_NOT_OPEN;
	mButtons[0] = nullptr;
	mButtons[1] = nullptr;
}

// 0x427120: the mouse takes what is under the window again
BBWindow::~BBWindow()
{
	SetHoverMinZ(mSavedHoverMinZ);
	if (sOpenCount > 0)
		sOpenCount--;
	else
		sOpenCount = 0;
}

// 0x427170 (ECX the columns): the panel, the tracks and the buttons; the window raises the mouse's lowest depth to its
// own and starts to grow
void BBWindow::Open(int theColumns, int theRows, int theButtons, bool deleteWhenClosed)
{
	BuildPanel(theColumns, theRows, false);
	ComputeDrawn();
	mSavedHoverMinZ = GetHoverMinZ();
	SetHoverMinZ(mDrawn[STATE_Z]);
	BuildTracks(deleteWhenClosed);
	if (theButtons == 0) {
		mButtonCount = 0;
	} else if (theButtons == 1) {
		mButtonCount = 1;
		BuildButtons((float)(theRows * 100));
	} else {
		mButtonCount = 2;
		BuildButtons((float)(theRows * 100));
	}
	mState = BBWINDOW_OPEN;
	StartTrack(mOpenTrack);
	SetEventTarget(this);
}

// 0x427230: the close track's last key ended
void BBWindow::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == BBWINDOW_EVENT_CLOSED)
		mState = BBWINDOW_CLOSED;
}

// 0x427260: a button the mouse is on swells (with a drum), and shrinks back when it leaves
bool BBWindow::Update()
{
	if (!ObjBase::Update())
		return false;
	for (int i = 0; i < BBWINDOW_MAX_BUTTONS; i++) {
		ObjBase* aButton = mButtons[i];
		if (aButton != nullptr) {
			if (aButton == sHover) {
				if (aButton->mHoverState == 0) {
					aButton->StartTrack(BUTTON_TRACK_HOVER);
					aButton->mHoverState = 1;
					gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
				}
			} else if (aButton->mHoverState == 1) {
				aButton->StartTrack(BUTTON_TRACK_LEAVE);
				aButton->mHoverState = 0;
			}
		}
	}
	return true;
}

// 0x4272f0: either button closes the window. The id is 0 on every object but a button.
bool BBWindow::MouseUp(ObjBase* theObj)
{
	if (mState == BBWINDOW_OPEN) {
		int aButtonId = theObj->mButtonId;
		if (aButtonId == BUTTON_ID_FIRST || aButtonId == BUTTON_ID_SECOND) {
			StartTrack(mCloseTrack);
			return true;
		}
	}
	return false;
}

// 0x427340
void BBWindow::Close()
{
	StartTrack(mCloseTrack);
}

// 0x427350: the open track grows the window (it swells in 25 frames on a sine wave, fading in) and holds; the close
// track shrinks it in 10 frames, then a last key tells the window it is closed (and with deleteWhenClosed ends the
// object)
void BBWindow::BuildTracks(bool deleteWhenClosed)
{
	AnimKey* aGrow = new AnimKey();
	aGrow->SetDuration(25);
	aGrow->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aGrow->AddModifier(STATE_SCALE_X, false, -1.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aGrow->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aGrow->AddModifier(STATE_SCALE_Y, false, -1.0f, MOD_SINE, 1.0f, 1.1f, 0.0f, nullptr);
	aGrow->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	AnimKey* aHold = new AnimKey();
	aHold->SetDuration(200);
	aHold->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aHold->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aHold->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	AnimTrack* anOpenTrack = new AnimTrack();
	anOpenTrack->AddKey(aGrow, nullptr);
	anOpenTrack->AddKey(aHold, aHold);
	mOpenTrack = AddTrack(anOpenTrack, nullptr);

	AnimKey* aShrink = new AnimKey();
	aShrink->SetDuration(10);
	aShrink->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->mEventId = BBWINDOW_EVENT_CLOSED;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_SCALE_X, -1.0f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_Y, -1.0f, MOD_HOLD);
	aGone->AddValue(STATE_ALPHA, -1.0f, MOD_HOLD);
	if (deleteWhenClosed)
		aGone->SetRemoveAtEnd();
	AnimTrack* aCloseTrack = new AnimTrack();
	aCloseTrack->AddKey(aShrink, nullptr);
	aCloseTrack->AddKey(aGone, nullptr);
	mCloseTrack = AddTrack(aCloseTrack, nullptr);
}

// 0x4276c0: the buttons (cels 2 and 3 of the picture) sit 127 above the panel's middle, one in the centre or two 247
// either side of it. Each has a rest track, one that swells it a little for the mouse on it and one that shrinks it
// back.
void BBWindow::BuildButtons(float theHeight)
{
	const int aButtonIds[BBWINDOW_MAX_BUTTONS] = {BUTTON_ID_FIRST, BUTTON_ID_SECOND};
	for (int i = 0; i < mButtonCount; i++) {
		ObjBase* aButton = new ObjBase();
		mButtons[i] = aButton;
		aButton->mType = OBJ_TYPE_WINDOW_BUTTON;
		aButton->mAnim = Anim::Get("images/menu/usrsel_buttons", ANIM_SCALE_SMOOTH, false, 0x16f, 0x5f, 4, false);
		aButton->SetParent(this, nullptr);
		aButton->SetStateInt(STATE_Z, 1);
		aButton->SetStateInt(STATE_FRAME, i + 2);
		aButton->SetMouseable(32.0f, 32.0f);
		aButton->SetMouseListener(this);
		aButton->mButtonId = aButtonIds[i];
		Activate(aButton, false);

		AnimKey* aRest = new AnimKey();
		aRest->mEventId = 0;
		aRest->SetDuration(50);
		aRest->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aRest->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
		AnimTrack* aRestTrack = new AnimTrack();
		aRestTrack->AddKey(aRest, aRest);
		aButton->AddTrack(aRestTrack, nullptr);

		AnimKey* aSwellStart = new AnimKey();
		aSwellStart->SetDuration(5);
		aSwellStart->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aSwellStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aSwelled = new AnimKey();
		aSwelled->SetDuration(2);
		aSwelled->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
		aSwelled->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
		AnimTrack* aSwellTrack = new AnimTrack();
		aSwellTrack->AddKey(aSwellStart, nullptr);
		aSwellTrack->AddKey(aSwelled, aSwelled);
		aButton->AddTrack(aSwellTrack, nullptr);

		AnimKey* aShrinkStart = new AnimKey();
		aShrinkStart->SetDuration(5);
		aShrinkStart->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
		aShrinkStart->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
		AnimKey* aShrunk = new AnimKey();
		aShrunk->SetDuration(2);
		aShrunk->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aShrunk->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimTrack* aShrinkTrack = new AnimTrack();
		aShrinkTrack->AddKey(aShrinkStart, nullptr);
		aShrinkTrack->AddKey(aShrunk, aShrunk);
		aButton->AddTrack(aShrinkTrack, nullptr);
	}
	if (mButtonCount == 2) {
		mButtons[0]->SetStateInt(STATE_X, -247);
		float aY = (float)((double)theHeight * 0.5 - 127.0);
		mButtons[0]->SetState(STATE_Y, aY);
		mButtons[1]->SetStateInt(STATE_X, 247);
		mButtons[1]->SetState(STATE_Y, aY);
	} else {
		mButtons[0]->SetStateInt(STATE_X, 0);
		mButtons[0]->SetState(STATE_Y, (float)((double)theHeight * 0.5 - 127.0));
	}
}
