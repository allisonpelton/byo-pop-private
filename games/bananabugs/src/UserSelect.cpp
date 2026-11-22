#include "UserSelect.h"

#include "Anim.h"
#include "AskWin.h"
#include "GameApp.h"
#include "NewUser.h"
#include "ObjString.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

#include <cstring>

using namespace Sexy;

// The controls' ids (ObjBase::mButtonId): BBWindow's OK and Cancel, the rows, and the window's own buttons
enum UserSelectControl
{
	USERSELECT_OK = 100,
	USERSELECT_CANCEL = 101,
	USERSELECT_FIRST_ROW = 102,
	USERSELECT_LAST_ROW = 109,
	USERSELECT_RENAME = 110,
	USERSELECT_DELETE = 111,
};

// A row's cels
enum UserSelectRowCel
{
	ROW_CEL_PLAIN = 0,
	ROW_CEL_NEW_PLAYER = 1,
	ROW_CEL_PICKED = 2,
};

// The buttons' tracks
enum UserSelectButtonTrack
{
	USERSELECT_TRACK_REST = 0,
	USERSELECT_TRACK_HOVER = 1,
	USERSELECT_TRACK_LEAVE = 2,
};

// The window's buttons are of the menus' button type
const int OBJ_TYPE_WINDOW_BUTTON = 0xf;

// The panel's size, in 100-pixel tiles
const int USERSELECT_COLUMNS = 11;
const int USERSELECT_PANEL_ROWS = 11;

// The rows run down from 300 pixels above the middle, 65 apart
const int USERSELECT_FIRST_ROW_Y = -300;
const int USERSELECT_ROW_STEP = 65;
const int USERSELECT_ROWS_END_Y = 220;

// The shortest name a rename takes
const int USERSELECT_MIN_NAME = 2;

// What a name box starts with, before it is emptied: fifteen characters, the longest a name may be
static const char* USERSELECT_NAME_ROOM = "000000000000000";

// 0x4721e0: the rows, the buttons below them, and the profiles' names
UserSelect::UserSelect()
{
	mFreeRow = 0;
	mSelected = -1;
	mSelectedName = nullptr;
	mNewUser = nullptr;
	mDeleteAsk = nullptr;
	mAskWin = nullptr;
	mRenaming = false;
	SetStateInt(STATE_Z, 300);
	Open(USERSELECT_COLUMNS, USERSELECT_PANEL_ROWS, 2, false);
	BuildLines(gApp->mFont, false);
	MoveLines(0, 48.0f, true);
	SetLineColor(0, 1.0f, 1.0f, 0.0f, 1.0f);
	SetLineScale(0, 1.0f, 1.0f);
	SetLineText(0, "WHO ARE YOU ?");
	Player::FindFreeSlot();
	mFreeRow = Player::sFreeSlot;
	BuildRows();
	BuildButtons(1100.0f);
	Refresh();
}

// A window with a message and an OK button
void UserSelect::Tell(const char* theFirstText, const char* theSecondText)
{
	mAskWin = new AskWin();
	mAskWin->SetParent(this, nullptr);
	mAskWin->SetText(theFirstText, ASK_MODE_OK, theSecondText);
}

// 0x472340: an open message, delete question or new player window takes the update until it closes; otherwise the
// buttons swell under the mouse and a name being renamed takes the keys
bool UserSelect::Update()
{
	if (!BBWindow::Update())
		return false;
	if (mAskWin != nullptr) {
		mAskWin->TakeChoice();
		if (mAskWin->IsClosed()) {
			mAskWin->Delete();
			mAskWin = nullptr;
		}
		return true;
	}
	if (mDeleteAsk != nullptr) {
		int aChoice = mDeleteAsk->TakeChoice();
		if (aChoice == ASK_CHOICE_YES) {
			if (mDeleteAsk->IsClosed()) {
				mDeleteAsk->Delete();
				mDeleteAsk = nullptr;
				Deselect();
				if (mFreeRow >= 0)
					mRows[mFreeRow]->SetStateInt(STATE_FRAME, ROW_CEL_PLAIN);
				Player::DeleteProfile(mSelected);
				Refresh();
				if (mFreeRow >= 0)
					mRows[mFreeRow]->SetStateInt(STATE_FRAME, ROW_CEL_NEW_PLAYER);
				if (mFreeRow != mSelected && mFreeRow != 0)
					Select(mSelected);
			}
		} else if (aChoice == ASK_CHOICE_NO) {
			if (mDeleteAsk->IsClosed()) {
				mDeleteAsk->Delete();
				mDeleteAsk = nullptr;
			}
		}
		return true;
	}
	if (mNewUser != nullptr) {
		if (mNewUser->GetState() == BBWINDOW_CLOSED) {
			mNewUser->Delete();
			mNewUser = nullptr;
			Refresh();
			if (mFreeRow >= 0)
				mRows[mFreeRow]->SetStateInt(STATE_FRAME, ROW_CEL_NEW_PLAYER);
			Select(mSelected);
		}
		return true;
	}
	for (int i = 0; i < USERSELECT_BUTTONS; i++) {
		ObjBase* aButton = mButtons[i];
		if (aButton == nullptr)
			continue;
		if (aButton == sHover) {
			if (aButton->mHoverState == 0) {
				aButton->StartTrack(USERSELECT_TRACK_HOVER);
				aButton->mHoverState = 1;
				gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
			}
		} else if (aButton->mHoverState == 1) {
			aButton->StartTrack(USERSELECT_TRACK_LEAVE);
			aButton->mHoverState = 0;
		}
	}
	if (!mRenaming || mSelectedName == nullptr)
		return true;
	switch (TakeKeyPressed()) {
	case KEY_BACKSPACE:
		mSelectedName->DeleteBeforeCursor();
		return true;
	case KEY_LEFT:
		mSelectedName->MoveCursor(-1);
		return true;
	case KEY_RIGHT:
		mSelectedName->MoveCursor(1);
		return true;
	case KEY_DELETE:
		mSelectedName->DeleteAtCursor();
		return true;
	}
	if (sKeyHeld == 0)
		return true;
	if (sKeyHeld == KEY_ENTER) {
		// unlike OK, Enter checks the name against every profile, the player's own among them, and takes a short one
		mSelectedName->Trim();
		if (!Player::NameIsFree(mSelectedName->mText, -1)) {
			Tell("NAME IS ALREADY", "USED!");
			mSelectedName->SetText(mOldName);
		} else {
			Player::AddProfile(mSelected, mNames[mSelected]->mText);
		}
		if (mSelectedName->GetTextLength() < 1)
			mSelectedName->SetText(mOldName);
		Select(mSelected);
		mRenaming = false;
		sKeyHeld = 0;
		return true;
	}
	mSelectedName->InsertTypedKey();
	sKeyHeld = 0;
	return true;
}

// 0x472730
bool UserSelect::MouseUp(ObjBase* theObj)
{
	if (mState != BBWINDOW_OPEN)
		return false;
	int anId = theObj->mButtonId;
	switch (anId) {
	case USERSELECT_OK:
		if (mRenaming) {
			mSelectedName->Trim();
			if (strlen(mSelectedName->mText) < USERSELECT_MIN_NAME) {
				Tell("NAME IS TOO", "SHORT!");
				return true;
			}
			if (!Player::NameIsFree(mSelectedName->mText, mSelected)) {
				Tell("NAME IS ALREADY", "USED!");
				mSelectedName->SetText(mOldName);
				return true;
			}
			Player::AddProfile(mSelected, mNames[mSelected]->mText);
			if (mSelectedName->GetTextLength() < 1)
				mSelectedName->SetText(mOldName);
			Select(mSelected);
			mRenaming = false;
		}
		if (mSelected < 0) {
			if (mAskWin == nullptr)
				Tell("YOU MUST SELECT", "A PLAYER");
			return true;
		}
		Player::AddProfile(mSelected, mNames[mSelected]->mText);
		break;
	case USERSELECT_CANCEL:
		if (mSelected < 0) {
			if (mAskWin == nullptr)
				Tell("YOU MUST SELECT", "A PLAYER");
			return true;
		}
		break;
	case USERSELECT_RENAME:
		if (mSelectedName == nullptr) {
			if (mAskWin == nullptr)
				Tell("YOU MUST SELECT", "A PLAYER");
			break;
		}
		strncpy(mOldName, mSelectedName->mText, USERSELECT_NAME_SIZE - 1);
		mSelectedName->SetCursor(mSelectedName->GetTextLength());
		mRenaming = true;
		TakeKeyPressed();
		sKeyHeld = 0;
		break;
	case USERSELECT_DELETE:
		if (mSelected < 0 || mSelected >= USERSELECT_ROWS) {
			if (mAskWin == nullptr)
				Tell("YOU MUST SELECT", "A PLAYER");
			break;
		}
		mDeleteAsk = new AskWin();
		mDeleteAsk->SetParent(this, nullptr);
		mDeleteAsk->SetText("DELETE PLAYER", ASK_MODE_YES_NO, "ARE YOU SURE?");
		break;
	default:
		if (anId < USERSELECT_FIRST_ROW || anId > USERSELECT_LAST_ROW)
			break;
		// a row: a rename in progress is dropped, and the new player's row opens the new player window
		sKeyHeld = 0;
		if (mRenaming) {
			mRenaming = false;
			mSelectedName->SetText(mOldName);
		}
		if (mFreeRow == anId - USERSELECT_FIRST_ROW) {
			if (mNewUser == nullptr) {
				Deselect();
				mSelected = mFreeRow;
				mNewUser = new NewUser(2);
				mNewUser->Open(this);
				if (mFreeRow >= 0)
					mRows[mFreeRow]->SetStateInt(STATE_FRAME, ROW_CEL_PLAIN);
			}
		} else {
			Select(anId - USERSELECT_FIRST_ROW);
		}
		break;
	}
	BBWindow::MouseUp(theObj);
	return true;
}

// 0x472b50: a row that holds a player, or the new player's, is picked: its cel shows it, and its name becomes the one a
// rename works on
void UserSelect::Select(int theRow)
{
	if (mFreeRow < 0) {
		if (theRow < 0 || theRow >= USERSELECT_ROWS)
			return;
	} else {
		if (theRow < 0 || theRow > mFreeRow)
			return;
	}
	Deselect();
	mSelected = theRow;
	mRenaming = false;
	mSelectedName = mNames[theRow];
	mSelectedName->SetCursor(-1);
	mRows[mSelected]->SetStateInt(STATE_FRAME, ROW_CEL_PICKED);
}

// 0x472bc0: the picked row goes back to plain, the new player's row too
void UserSelect::Deselect()
{
	if (mSelectedName != nullptr)
		mSelectedName->SetCursor(-1);
	if (mSelected >= 0) {
		mRows[mSelected]->SetStateInt(STATE_FRAME, ROW_CEL_PLAIN);
		mNames[mSelected]->SetCursor(-1);
	}
	mSelectedName = nullptr;
}

// 0x472c20: the profiles' names fill the rows from the top, gaps closed up, and the current player is picked
void UserSelect::Refresh()
{
	int aFilled = 0;
	for (int i = 0; i < USERSELECT_ROWS; i++) {
		const char* aName = Player::GetNameAt(i);
		mNames[i]->SetText(nullptr);
		if (aName != nullptr && aName[0] != '\0') {
			mNames[aFilled]->SetText(aName);
			aFilled++;
		}
	}
	Player::FindFreeSlot();
	mFreeRow = Player::sFreeSlot;
	mSelected = Player::ChooseFirstInUse();
	Select(mSelected);
}

// 0x472ca0
void UserSelect::BuildRows()
{
	ComputeDrawn();
	int aRow = 0;
	for (int aY = USERSELECT_FIRST_ROW_Y; aY < USERSELECT_ROWS_END_Y; aY += USERSELECT_ROW_STEP) {
		ObjBase* aBack = new ObjBase();
		mRows[aRow] = aBack;
		aBack->mAnim = Anim::Get("images/menu/inputback", ANIM_SCALE_SMOOTH, false, 800, 65, 3, false);
		aBack->SetParent(this, nullptr);
		aBack->SetMouseable(32.0f, 32.0f);
		aBack->SetMouseListener(this);
		aBack->SetStateInt(STATE_X, 0);
		aBack->SetStateInt(STATE_Y, aY);
		aBack->SetStateInt(STATE_Z, 1);
		aBack->SetStateInt(STATE_ALPHA, 1);
		aBack->SetStateInt(STATE_SCALE_X, 1);
		aBack->SetStateInt(STATE_SCALE_Y, 1);
		aBack->mButtonId = USERSELECT_FIRST_ROW + aRow;
		if (aRow == mFreeRow)
			aBack->SetStateInt(STATE_FRAME, ROW_CEL_NEW_PLAYER);
		aBack->ComputeDrawn();

		ObjString* aName = new ObjString();
		mNames[aRow] = aName;
		aName->SetParent(aBack, nullptr);
		aName->SetFont(gApp->mFont);
		aName->SetStateInt(STATE_Z, 3);
		aName->Create(USERSELECT_NAME_ROOM, false, 0);
		aName->SetText(nullptr);
		aName->SetCursor(-1);
		Activate(aName, false);
		Activate(aBack, false);
		aRow++;
	}
}

// 0x472e70: rename and delete, side by side under the rows; they swell under the mouse like BBWindow's
void UserSelect::BuildButtons(float theHeight)
{
	const int aButtonIds[USERSELECT_BUTTONS] = { USERSELECT_RENAME, USERSELECT_DELETE };
	for (int i = 0; i < USERSELECT_BUTTONS; i++) {
		ObjBase* aButton = new ObjBase();
		mButtons[i] = aButton;
		aButton->mType = OBJ_TYPE_WINDOW_BUTTON;
		aButton->mAnim = Anim::Get("images/menu/usrsel_buttons", ANIM_SCALE_SMOOTH, false, 0x167, 0x67, 4, false);
		aButton->SetParent(this, nullptr);
		aButton->SetStateInt(STATE_Z, 1);
		aButton->SetStateInt(STATE_FRAME, i);
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
	mButtons[0]->SetStateInt(STATE_X, -247);
	float aY = (float)((double)theHeight * 0.5 - 256.0);
	mButtons[0]->SetState(STATE_Y, aY);
	mButtons[1]->SetStateInt(STATE_X, 247);
	mButtons[1]->SetState(STATE_Y, aY);
}
