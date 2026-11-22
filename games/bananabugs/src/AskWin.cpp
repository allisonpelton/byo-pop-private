#include "AskWin.h"

#include "Anim.h"
#include "GameApp.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

Anim* AskWin::sYesNoAnim = nullptr;
Anim* AskWin::sButtonsAnim = nullptr;
Anim* AskWin::sCancelAnim = nullptr;

// The buttons' ids (ObjBase::mButtonId): the first two button ids count from 100, the OK and cancel buttons have 120
const int ASK_BUTTON_YES = 100;
const int ASK_BUTTON_NO = 101;
const int ASK_BUTTON_UNUSED = 110;  // answers yes, as the first button does; no button has it
const int ASK_BUTTON_OK = 120;

// The keys' event ids (AnimKey::mEventId)
const int ASK_EVENT_BUTTON_SHRINK = 1;  // a button's shrinking key; nothing answers it
const int ASK_EVENT_CLOSED = 2;

// The window's tracks, and the buttons'
enum AskWindowTrack
{
	ASK_TRACK_OPEN = 0,
	ASK_TRACK_CLOSE = 1,
};
enum AskButtonTrack
{
	ASK_BUTTON_TRACK_REST = 0,
	ASK_BUTTON_TRACK_HOVER = 1,
	ASK_BUTTON_TRACK_LEAVE = 2,
};

const int ASK_PANEL_COLUMNS = 9;
const int ASK_TEXT_LENGTH = 0x50;
const int ASK_BUTTON_Y = 0x70;
const int ASK_CANCEL_BUTTON_Y = 0xd2;
const int ASK_RAISED_BUTTON_Y = 0x2c;
const int ASK_YES_BUTTON_X = -212;
const int ASK_NO_BUTTON_X = 212;

// 0x402920
AskWin::AskWin()
{
	mSavedHoverMinZ = 0.0f;
	mButtonCount = 0;
	mChoiceTaken = false;
	SetStateInt(STATE_Z, 0x33);
	SetMouseable(32.0f, 32.0f);
	memset(mFirstText, 0, ASK_TEXT_SIZE);
	memset(mSecondText, 0, ASK_TEXT_SIZE);
	mButtons[0] = nullptr;
	mButtons[1] = nullptr;
	mChoice = ASK_CHOICE_NONE;
	mClosed = false;
	Deactivate(this);
}

// 0x4029b0 (body 0x4029d0): the mouse takes what is under the window again
AskWin::~AskWin()
{
	SetHoverMinZ(mSavedHoverMinZ);
}

// 0x4029f0 (the second text in EAX): the window gets its two lines, rows to fit them (a third text row with the cancel
// button), its buttons, and starts to open. The window raises the mouse's lowest depth to its own.
void AskWin::SetText(const char* theFirstText, int theMode, const char* theSecondText)
{
	ComputeDrawn();
	mSavedHoverMinZ = GetHoverMinZ();
	SetHoverMinZ(mDrawn[STATE_Z]);
	strncpy(mFirstText, theFirstText, ASK_TEXT_LENGTH);
	strncpy(mSecondText, theSecondText, ASK_TEXT_LENGTH);
	int aRows = theSecondText != nullptr ? 1 : 0;
	if (theMode == ASK_MODE_YES_NO_CANCEL)
		aRows = 3;
	BuildPanel(ASK_PANEL_COLUMNS, aRows + 4, false);
	SetStateInt(STATE_Z, 0x32);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	BuildLines(gApp->mFont, false);
	MoveLines(0, 64.0f, true);
	SetLineColor(0, 1.0f, 0.7f, 0.0f, 1.0f);
	SetLineScale(0, 1.2f, 1.3f);
	SetLineText(0, mFirstText);
	MoveLines(1, 64.0f, true);
	SetLineColor(1, 1.0f, 0.7f, 0.0f, 1.0f);
	SetLineScale(1, 1.2f, 1.3f);
	SetLineText(1, mSecondText);
	BuildWindowTracks();
	if (theMode == ASK_MODE_OK) {
		BuildOkButton();
	} else if (theMode == ASK_MODE_YES_NO_CANCEL) {
		BuildYesNoButtons();
		BuildCancelButton();
		mButtons[0]->SetStateInt(STATE_Y, ASK_RAISED_BUTTON_Y);
		mButtons[1]->SetStateInt(STATE_Y, ASK_RAISED_BUTTON_Y);
	} else {
		BuildYesNoButtons();
	}
	StartTrack(ASK_TRACK_OPEN);
	Activate(this, false);
}

// 0x402be0: the close track ended
void AskWin::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == ASK_EVENT_CLOSED) {
		mClosed = true;
		SetHoverMinZ(mSavedHoverMinZ);
	}
}

// 0x402c10: until an answer is given, a button the mouse is on swells (with a drum), and shrinks back when it leaves
bool AskWin::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mChoice == ASK_CHOICE_NONE) {
		for (int i = 0; i < mButtonCount; i++) {
			ObjBase* aButton = mButtons[i];
			if (aButton == sHover) {
				if (aButton->mHoverState == 0) {
					aButton->StartTrack(ASK_BUTTON_TRACK_HOVER);
					aButton->mHoverState = 1;
					gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
				}
			} else if (aButton->mHoverState == 1) {
				aButton->StartTrack(ASK_BUTTON_TRACK_LEAVE);
				aButton->mHoverState = 0;
			}
		}
	}
	return true;
}

// 0x402cc0: a button gives the answer, with a click. The id is 0 on every object but a button.
bool AskWin::MouseUp(ObjBase* theObj)
{
	int aButtonId = theObj->mButtonId;
	switch (aButtonId) {
	case ASK_BUTTON_YES:
	case ASK_BUTTON_UNUSED:
		mChoice = ASK_CHOICE_YES;
		break;
	case ASK_BUTTON_NO:
		mChoice = ASK_CHOICE_NO;
		break;
	case ASK_BUTTON_OK:
		mChoice = ASK_CHOICE_OTHER;
		break;
	default:
		return true;
	}
	gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
	return true;
}

// 0x402d50: the answer, or none yet; the first time it gives one, the window starts to close
int AskWin::TakeChoice()
{
	if (mChoice != ASK_CHOICE_NONE && !mChoiceTaken) {
		mChoiceTaken = true;
		StartTrack(ASK_TRACK_CLOSE);
	}
	return mChoice;
}

// 0x402d90: a yes (the first cel of the picture, left of the middle) and a no button. Each has a rest track, one that
// swells it for the mouse on it and one that shrinks it back.
void AskWin::BuildYesNoButtons()
{
	mButtonCount = 2;
	const int aButtonX[2] = {ASK_YES_BUTTON_X, ASK_NO_BUTTON_X};
	for (int i = 0; i < 2; i++) {
		ObjBase* aButton = new ObjBase();
		mButtons[i] = aButton;
		sYesNoAnim->AddRef();
		aButton->mAnim = sYesNoAnim;
		aButton->SetParent(this, nullptr);
		aButton->SetMouseable(32.0f, 32.0f);
		aButton->SetMouseListener(this);
		aButton->SetStateInt(STATE_ALPHA, 0);
		aButton->SetStateInt(STATE_X, aButtonX[i]);
		aButton->SetStateInt(STATE_Y, ASK_BUTTON_Y);
		aButton->SetStateInt(STATE_Z, 1); // the original passes the register the call before left, the channel 1
		aButton->SetStateInt(STATE_SCALE_X, 0);
		aButton->SetStateInt(STATE_SCALE_Y, 0);
		aButton->SetStateInt(STATE_FRAME, i);
		aButton->mButtonId = i + ASK_BUTTON_YES;
		aButton->mHoverState = 0;

		AnimKey* aAppear = new AnimKey();
		aAppear->SetDuration(8);
		aAppear->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aAppear->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
		aAppear->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
		AnimKey* aRest = new AnimKey();
		aRest->SetDuration(1);
		aRest->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aRest->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
		aRest->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
		AnimTrack* aRestTrack = new AnimTrack();
		aRestTrack->AddKey(aAppear, nullptr);
		aRestTrack->AddKey(aRest, aRest);
		aButton->AddTrack(aRestTrack, nullptr);

		AnimKey* aSwellStart = new AnimKey();
		aSwellStart->SetDuration(5);
		aSwellStart->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aSwellStart->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aSwellStart->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		AnimKey* aSwelled = new AnimKey();
		aSwelled->SetDuration(2);
		aSwelled->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aSwelled->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
		aSwelled->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
		AnimTrack* aSwellTrack = new AnimTrack();
		aSwellTrack->AddKey(aSwellStart, nullptr);
		aSwellTrack->AddKey(aSwelled, aSwelled);
		aButton->AddTrack(aSwellTrack, nullptr);

		AnimKey* aShrinkStart = new AnimKey();
		aShrinkStart->SetDuration(5);
		aShrinkStart->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aShrinkStart->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
		aShrinkStart->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
		AnimKey* aShrunk = new AnimKey();
		aShrunk->mEventId = ASK_EVENT_BUTTON_SHRINK;
		aShrunk->SetDuration(2);
		aShrunk->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aShrunk->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aShrunk->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		AnimTrack* aShrinkTrack = new AnimTrack();
		aShrinkTrack->AddKey(aShrinkStart, nullptr);
		aShrinkTrack->AddKey(aShrunk, aShrunk);
		aButton->AddTrack(aShrinkTrack, nullptr);
		Activate(aButton, false);
	}
}

// 0x403310: one OK button (the third cel of "usrsel_buttons"), with the same three tracks
void AskWin::BuildOkButton()
{
	mButtonCount = 1;
	ObjBase* aButton = new ObjBase();
	mButtons[0] = aButton;
	sButtonsAnim->AddRef();
	aButton->mAnim = sButtonsAnim;
	aButton->SetParent(this, nullptr);
	aButton->SetMouseable(32.0f, 32.0f);
	aButton->SetMouseListener(this);
	aButton->SetStateInt(STATE_ALPHA, 0);
	aButton->SetStateInt(STATE_X, 0);
	aButton->SetStateInt(STATE_Y, ASK_BUTTON_Y);
	aButton->SetStateInt(STATE_Z, 1); // the channel the call before left in the register
	aButton->SetStateInt(STATE_SCALE_X, 0);
	aButton->SetStateInt(STATE_SCALE_Y, 0);
	aButton->SetStateInt(STATE_FRAME, 2);
	aButton->mButtonId = ASK_BUTTON_OK;
	aButton->mHoverState = 0;

	AnimKey* aAppear = new AnimKey();
	aAppear->SetDuration(8);
	aAppear->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aAppear->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
	aAppear->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
	AnimKey* aRest = new AnimKey();
	aRest->SetDuration(1);
	aRest->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aRest->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aRest->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aRestTrack = new AnimTrack();
	aRestTrack->AddKey(aAppear, nullptr);
	aRestTrack->AddKey(aRest, aRest);
	aButton->AddTrack(aRestTrack, nullptr);

	AnimKey* aSwellStart = new AnimKey();
	aSwellStart->SetDuration(5);
	aSwellStart->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSwellStart->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwelled = new AnimKey();
	aSwelled->SetDuration(2);
	aSwelled->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aSwelled->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimTrack* aSwellTrack = new AnimTrack();
	aSwellTrack->AddKey(aSwellStart, nullptr);
	aSwellTrack->AddKey(aSwelled, aSwelled);
	aButton->AddTrack(aSwellTrack, nullptr);

	AnimKey* aShrinkStart = new AnimKey();
	aShrinkStart->mEventId = ASK_EVENT_BUTTON_SHRINK;
	aShrinkStart->SetDuration(5);
	aShrinkStart->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aShrinkStart->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aShrunk = new AnimKey();
	aShrunk->SetDuration(2);
	aShrunk->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aShrunk->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aShrinkTrack = new AnimTrack();
	aShrinkTrack->AddKey(aShrinkStart, nullptr);
	aShrinkTrack->AddKey(aShrunk, aShrunk);
	aButton->AddTrack(aShrinkTrack, nullptr);
	Activate(aButton, false);
}

// 0x4037a0: a cancel button (its own picture) lower than the others, with the OK button's three tracks
void AskWin::BuildCancelButton()
{
	mButtonCount = 3;
	ObjBase* aButton = new ObjBase();
	mButtons[2] = aButton;
	sCancelAnim->AddRef();
	aButton->mAnim = sCancelAnim;
	aButton->SetParent(this, nullptr);
	aButton->SetMouseable(32.0f, 32.0f);
	aButton->SetMouseListener(this);
	aButton->SetStateInt(STATE_ALPHA, 0);
	aButton->SetStateInt(STATE_X, 0);
	aButton->SetStateInt(STATE_Y, ASK_CANCEL_BUTTON_Y);
	aButton->SetStateInt(STATE_Z, 1); // the channel the call before left in the register
	aButton->SetStateInt(STATE_SCALE_X, 0);
	aButton->SetStateInt(STATE_SCALE_Y, 0);
	aButton->SetStateInt(STATE_FRAME, 0);
	aButton->mButtonId = ASK_BUTTON_OK;
	aButton->mHoverState = 0;

	AnimKey* aAppear = new AnimKey();
	aAppear->SetDuration(8);
	aAppear->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aAppear->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
	aAppear->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
	AnimKey* aRest = new AnimKey();
	aRest->SetDuration(1);
	aRest->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aRest->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aRest->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aRestTrack = new AnimTrack();
	aRestTrack->AddKey(aAppear, nullptr);
	aRestTrack->AddKey(aRest, aRest);
	aButton->AddTrack(aRestTrack, nullptr);

	AnimKey* aSwellStart = new AnimKey();
	aSwellStart->SetDuration(5);
	aSwellStart->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSwellStart->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwelled = new AnimKey();
	aSwelled->SetDuration(2);
	aSwelled->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aSwelled->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimTrack* aSwellTrack = new AnimTrack();
	aSwellTrack->AddKey(aSwellStart, nullptr);
	aSwellTrack->AddKey(aSwelled, aSwelled);
	aButton->AddTrack(aSwellTrack, nullptr);

	AnimKey* aShrinkStart = new AnimKey();
	aShrinkStart->mEventId = ASK_EVENT_BUTTON_SHRINK;
	aShrinkStart->SetDuration(5);
	aShrinkStart->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aShrinkStart->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aShrunk = new AnimKey();
	aShrunk->SetDuration(2);
	aShrunk->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aShrunk->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aShrinkTrack = new AnimTrack();
	aShrinkTrack->AddKey(aShrinkStart, nullptr);
	aShrinkTrack->AddKey(aShrunk, aShrunk);
	aButton->AddTrack(aShrinkTrack, nullptr);
	Activate(aButton, false);
}

// 0x403c30: the open track fades and scales the window in over 10 frames and holds; the close track takes it out over
// 10 frames and a last key reports it closed
void AskWin::BuildWindowTracks()
{
	AnimKey* aOpening = new AnimKey();
	aOpening->SetDuration(10);
	aOpening->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aOpening->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aOpening->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aOpen = new AnimKey();
	aOpen->SetDuration(1);
	aOpen->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aOpen->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aOpen->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* anOpenTrack = new AnimTrack();
	anOpenTrack->AddKey(aOpening, nullptr);
	anOpenTrack->AddKey(aOpen, aOpen);
	AddTrack(anOpenTrack, nullptr);

	AnimKey* aClosing = new AnimKey();
	aClosing->SetDuration(10);
	aClosing->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aClosing->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aClosing->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aClosed = new AnimKey();
	aClosed->mEventId = ASK_EVENT_CLOSED;
	aClosed->SetDuration(1);
	aClosed->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aClosed->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aClosed->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aCloseTrack = new AnimTrack();
	aCloseTrack->AddKey(aClosing, nullptr);
	aCloseTrack->AddKey(aClosed, nullptr);
	AddTrack(aCloseTrack, nullptr);
}

// 0x403f10
void Sexy::PreloadAskWin()
{
	AskWin::sYesNoAnim = Anim::Get("images/menu/ask_yesno", ANIM_SCALE_SMOOTH, false, 0xb3, 0x83, 2, false);
	AskWin::sButtonsAnim = Anim::Get("images/menu/usrsel_buttons", ANIM_SCALE_SMOOTH, false, 0x167, 0x67, 4, false);
	AskWin::sCancelAnim = Anim::Get("images/menu/ask_cancel", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
}
