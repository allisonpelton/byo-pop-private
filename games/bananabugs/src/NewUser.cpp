#include "NewUser.h"

#include "Anim.h"
#include "AskWin.h"
#include "GameApp.h"
#include "ObjString.h"
#include "Player.h"

using namespace Sexy;

// The id the window's button carries (BBWindow's first), given to the box too
const int NEWUSER_BUTTON_OK = 100;

// The panel's size, in 100-pixel tiles
const int NEWUSER_COLUMNS = 11;
const int NEWUSER_ROWS = 7;

// What the name box starts with, before Open empties it: fifteen characters, the longest a name may be
static const char* NEWUSER_NAME_ROOM = "000000000000000";

// 0x4504d0
NewUser::NewUser(int theButtons)
{
	mButtonsWanted = theButtons;
	mAskWin = nullptr;
	mName = new ObjString();
	mName->SetParent(this, nullptr);
	mName->SetFont(gApp->mFont);
	mName->SetStateInt(STATE_Z, 2);
	mName->Create(NEWUSER_NAME_ROOM, false, 0);
	mName->SetCursor(10);
	BuildInputBox();
	mUnused358 = 0;
}

// 0x4505e0: the box is emptied and the window opens over theParent, with its two lines in yellow
void NewUser::Open(ObjBase* theParent)
{
	SetStateInt(STATE_Z, 100);
	SetParent(theParent, nullptr);
	ComputeDrawn();
	mName->SetText(nullptr);
	Activate(mName, false);
	BBWindow::Open(NEWUSER_COLUMNS, NEWUSER_ROWS, mButtonsWanted, false);
	BuildLines(gApp->mFont, false);
	MoveLines(0, 48.0f, true);
	SetLineColor(0, 1.0f, 1.0f, 0.0f, 1.0f);
	SetLineScale(0, 1.0f, 1.0f);
	SetLineText(0, "NEW PLAYER");
	MoveLines(1, 48.0f, true);
	SetLineColor(1, 1.0f, 1.0f, 0.0f, 1.0f);
	SetLineScale(1, 1.0f, 1.0f);
	SetLineText(1, "PLEASE ENTER YOUR NAME");
}

// 0x450710: the keys edit the name, Enter takes it; while the turned-down window is open the keys go to it, and it is
// deleted once it has closed
bool NewUser::Update()
{
	if (!BBWindow::Update())
		return false;
	if (mAskWin != nullptr) {
		mAskWin->TakeChoice();
		if (mAskWin->IsClosed()) {
			mAskWin->Delete();
			mAskWin = nullptr;
			sKeyHeld = 0;
			sKeyPressed = 0;
		}
		return true;
	}
	switch (TakeKeyPressed()) {
	case KEY_BACKSPACE:
		mName->DeleteBeforeCursor();
		return true;
	case KEY_DELETE:
		mName->DeleteAtCursor();
		return true;
	case KEY_RIGHT:
		mName->MoveCursor(1);
		return true;
	case KEY_LEFT:
		mName->MoveCursor(-1);
		return true;
	}
	if (sKeyHeld == 0)
		return true;
	if (sKeyHeld == KEY_ENTER) {
		mInputBox->mButtonId = NEWUSER_BUTTON_OK;
		MouseUp(mInputBox);
		sKeyHeld = 0;
		sKeyPressed = 0;
		return true;
	}
	mName->InsertTypedKey();
	sKeyHeld = 0;
	sKeyPressed = 0;
	return true;
}

// 0x450860: the name, trimmed, becomes a new profile, and the window closes; a name the profiles turn down keeps it open
bool NewUser::MouseUp(ObjBase* theObj)
{
	if (mState != BBWINDOW_OPEN)
		return false;
	if (theObj->mButtonId == NEWUSER_BUTTON_OK) {
		if (mAskWin != nullptr)
			return true;
		mName->Trim();
		if (Player::AddProfile(-1, mName->mText) < 0) {
			mAskWin = new AskWin();
			mAskWin->SetParent(this, nullptr);
			mAskWin->SetText("YOU NEED TO ENTER", ASK_MODE_OK, "A VALID NAME");
			return true;
		}
	}
	BBWindow::MouseUp(theObj);
	return true;
}

// 0x450920: the box, as wide as the screen, under the name
void NewUser::BuildInputBox()
{
	mInputAnim = Anim::Get("images/menu/inputback", ANIM_SCALE_SMOOTH, false, 800, 65, 2, false);
	mInputBox = new ObjBase();
	mInputBox->mAnim = mInputAnim;
	mInputAnim->AddRef();
	mInputBox->SetParent(this, nullptr);
	mInputBox->SetMouseable(32.0f, 32.0f);
	mInputBox->SetMouseListener(this);
	mInputBox->SetStateInt(STATE_X, 0);
	mInputBox->SetStateInt(STATE_Y, 0);
	mInputBox->SetStateInt(STATE_Z, 1);
	mInputBox->SetStateInt(STATE_ALPHA, 1);
	mInputBox->SetStateInt(STATE_SCALE_X, 1);
	mInputBox->SetStateInt(STATE_SCALE_Y, 1);
	mInputBox->mButtonId = NEWUSER_BUTTON_OK;
	Activate(mInputBox, false);
}
