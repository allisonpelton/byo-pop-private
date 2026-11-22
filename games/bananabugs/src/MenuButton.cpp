#include "MenuButton.h"

#include "Anim.h"
#include "Cursor.h"
#include "GameApp.h"
#include "Pending.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

const int MENU_BUTTON_COOLDOWN = 100;

// 0x446850: two cels of 123 by 75 design pixels
MenuButton::MenuButton()
{
	mAnim = Anim::Get("images/menu_button", ANIM_SCALE_SMOOTH, false, 0x7b, 0x4b, 2, false);
	SetStateInt(STATE_X, -900);
	SetStateInt(STATE_Y, -564);
	SetStateInt(STATE_Z, 190);
	SetStateInt(STATE_FRAME, 0);
	SetStateInt(STATE_ALPHA, 1);
	SetMouseable(32.0f, 32.0f);
	mClicked = false;
	mClickCooldown = MENU_BUTTON_COOLDOWN;
	mKeepCursor = false;
	mHovered = false;
}

// 0x446930
MenuButton::~MenuButton()
{
}

// 0x446960
void MenuButton::Deactivate(ObjBase* theObj)
{
	ObjBase::Deactivate(theObj);
	mHovered = false;
}

// 0x42c290
void MenuButton::Notify(ObjBase* theObj)
{
}

// 0x446980: the cooldown runs down
bool MenuButton::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mClickCooldown > 0)
		mClickCooldown--;
	return true;
}

// 0x4469b0: a click counts when the cooldown is over
bool MenuButton::MouseUp(ObjBase* theObj)
{
	if (mClickCooldown < 1) {
		mClicked = true;
		mClickCooldown = MENU_BUTTON_COOLDOWN;
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
	}
	return true;
}

// 0x4469f0: the second cel shows, and the game's pointer if a level is running
void MenuButton::MouseEnter(ObjBase* theObj, bool isNew)
{
	if (isNew && (sLevel == nullptr || LevelAcceptsMouse())) {
		mHovered = true;
		if (sLevel != nullptr)
			Cursor::Show();
		SetStateInt(STATE_FRAME, 1);
	}
}

// 0x446a30
void MenuButton::MouseLeave(ObjBase* theObj)
{
	mHovered = false;
	if (sLevel == nullptr || LevelAcceptsMouse()) {
		SetStateInt(STATE_FRAME, 0);
		if (sLevel != nullptr && !mKeepCursor)
			Cursor::Hide();
	}
}

// 0x446a70
bool MenuButton::TakeClicked()
{
	bool aClicked = mClicked;
	mClicked = false;
	return aClicked;
}

// 0x446a80: the first cel again, and shown
void MenuButton::Reset()
{
	SetStateInt(STATE_FRAME, 0);
	Activate(this, false);
}

// 0x446aa0: the game's pointer shows or hides at once
void MenuButton::SetKeepCursor(bool keep)
{
	mKeepCursor = keep;
	if (keep)
		Cursor::Show();
	else
		Cursor::Hide();
}
