#include "InGameMenu.h"

#include "Anim.h"
#include "AskWin.h"
#include "Cursor.h"
#include "GameApp.h"
#include "Options.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The buttons' ids (ObjBase::mButtonId), each also the cel of "images/menu/ingame_buttons" it shows (the id less one)
enum InGameMenuButton
{
	IN_GAME_BUTTON_RESUME = 1,
	IN_GAME_BUTTON_OPTIONS = 2,
	IN_GAME_BUTTON_QUIT = 3,
};

// The buttons are of the type the windows' buttons have (BBWindow.cpp)
const int OBJ_TYPE_MENU_BUTTON = 0xf;

// The buttons' tracks
enum InGameButtonTrack
{
	IN_GAME_BUTTON_TRACK_REST = 0,
	IN_GAME_BUTTON_TRACK_HOVER = 1,
	IN_GAME_BUTTON_TRACK_LEAVE = 2,
};

// The panel, in 100-pixel tiles
const int IN_GAME_MENU_COLUMNS = 7;
const int IN_GAME_MENU_ROWS = 5;

const int IN_GAME_MENU_Z = 0x209;
const int IN_GAME_MENU_START_DELAY = 0x32;     // frames before the buttons answer, from the start
const int IN_GAME_MENU_OPTIONS_DELAY = 100;    // after the options window closes
const int IN_GAME_MENU_ASK_DELAY = 0x19;       // after the question window closes
const int IN_GAME_MENU_SHOT_COOLDOWN = 0x32;   // frames before the monkey may shoot, once the menu is gone
const int IN_GAME_MENU_LEVEL_UPDATES = 2;      // ObjBase's update level while a level is on
const int IN_GAME_BUTTON_CEL_WIDTH = 0x167;
const int IN_GAME_BUTTON_CEL_HEIGHT = 0x67;
const int IN_GAME_BUTTON_CELS = 3;
const int IN_GAME_BUTTON_REST_FRAMES = 0x32;
const int IN_GAME_BUTTON_SWELL_FRAMES = 5;
const int IN_GAME_BUTTON_SETTLE_FRAMES = 2;
static const int IN_GAME_BUTTON_Y[IN_GAME_MENU_BUTTONS] = { -128, 0, 128 };

// The hover track swells a button by a tenth (its scale values are relative to its own); the leave track puts it back
const float IN_GAME_BUTTON_SWELL = 0.1f;

// 0x435c00: a window with no buttons of its own (BBWindow's), which then builds its three. While a level is on, the
// game's cursor shows and the update level is raised to 2.
InGameMenu::InGameMenu()
{
	mChoice = IN_GAME_MENU_OPEN;
	mOptions = nullptr;
	mAskWin = nullptr;
	mDelay = IN_GAME_MENU_START_DELAY;
	SetStateInt(STATE_Z, IN_GAME_MENU_Z);
	Open(IN_GAME_MENU_COLUMNS, IN_GAME_MENU_ROWS, 0, true);
	BuildButtons();
	ObjBase::Activate(this, false);
	if (sLevel != nullptr) {
		Cursor::Show();
		SetUpdateLevel(IN_GAME_MENU_LEVEL_UPDATES);
	}
}

// 0x435cb0 (body 0x435cd0): the update level is lowered again, the monkey may not shoot at once, the options window
// goes with the menu. The question window, a child, goes with it as any child does.
InGameMenu::~InGameMenu()
{
	if (sLevel != nullptr) {
		Player::SetShotCooldown(IN_GAME_MENU_SHOT_COOLDOWN);
		Cursor::Hide();
		SetUpdateLevel(0);
	}
	if (mOptions != nullptr) {
		mOptions->Delete();
		mOptions = nullptr;
	}
}

// 0x436030: three buttons, each its cel of the picture, at their places down the middle, at half depth, with a rest
// track and the swell and shrink the mouse starts
void InGameMenu::BuildButtons()
{
	const int aButtonIds[IN_GAME_MENU_BUTTONS] = { IN_GAME_BUTTON_RESUME, IN_GAME_BUTTON_OPTIONS, IN_GAME_BUTTON_QUIT };
	for (int i = 0; i < IN_GAME_MENU_BUTTONS; i++) {
		ObjBase* aButton = new ObjBase();
		mMenuButtons[i] = aButton;
		aButton->mType = OBJ_TYPE_MENU_BUTTON;
		aButton->mAnim = Anim::Get("images/menu/ingame_buttons", ANIM_SCALE_SMOOTH, false, IN_GAME_BUTTON_CEL_WIDTH,
			IN_GAME_BUTTON_CEL_HEIGHT, IN_GAME_BUTTON_CELS, false);
		aButton->SetParent(this, nullptr);
		aButton->SetState(STATE_Z, 0.5f);
		aButton->SetStateInt(STATE_FRAME, i);
		aButton->SetMouseable(32.0f, 32.0f);
		aButton->SetMouseListener(this);
		aButton->mButtonId = aButtonIds[i];
		Activate(aButton, false);

		// track 0, rest: the scale held
		AnimKey* aRestKey = new AnimKey();
		aRestKey->mEventId = 0;
		aRestKey->SetDuration(IN_GAME_BUTTON_REST_FRAMES);
		aRestKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aRestKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
		AnimTrack* aRestTrack = new AnimTrack();
		aRestTrack->AddKey(aRestKey, aRestKey);
		aButton->AddTrack(aRestTrack, nullptr);

		// track 1, hover: swells in 5 frames, then 2 more, held
		AnimKey* aSwellStart = new AnimKey();
		aSwellStart->SetDuration(IN_GAME_BUTTON_SWELL_FRAMES);
		aSwellStart->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aSwellStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aSwellEnd = new AnimKey();
		aSwellEnd->SetDuration(IN_GAME_BUTTON_SETTLE_FRAMES);
		aSwellEnd->AddValue(STATE_SCALE_X, IN_GAME_BUTTON_SWELL, MOD_TOWARD_NEXT);
		aSwellEnd->AddValue(STATE_SCALE_Y, IN_GAME_BUTTON_SWELL, MOD_TOWARD_NEXT);
		AnimTrack* aHoverTrack = new AnimTrack();
		aHoverTrack->AddKey(aSwellStart, nullptr);
		aHoverTrack->AddKey(aSwellEnd, aSwellEnd);
		aButton->AddTrack(aHoverTrack, nullptr);

		// track 2, leave: back from the swell in 5 frames, then 2 more, held
		AnimKey* aShrinkStart = new AnimKey();
		aShrinkStart->SetDuration(IN_GAME_BUTTON_SWELL_FRAMES);
		aShrinkStart->AddValue(STATE_SCALE_X, IN_GAME_BUTTON_SWELL, MOD_TOWARD_NEXT);
		aShrinkStart->AddValue(STATE_SCALE_Y, IN_GAME_BUTTON_SWELL, MOD_TOWARD_NEXT);
		AnimKey* aShrinkEnd = new AnimKey();
		aShrinkEnd->SetDuration(IN_GAME_BUTTON_SETTLE_FRAMES);
		aShrinkEnd->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aShrinkEnd->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimTrack* aLeaveTrack = new AnimTrack();
		aLeaveTrack->AddKey(aShrinkStart, nullptr);
		aLeaveTrack->AddKey(aShrinkEnd, aShrinkEnd);
		aButton->AddTrack(aLeaveTrack, nullptr);
	}
	for (int i = 0; i < IN_GAME_MENU_BUTTONS; i++)
		mMenuButtons[i]->SetStateInt(STATE_Y, IN_GAME_BUTTON_Y[i]);
}

// 0x435d60: while the delay runs nothing answers. With no question window open, the button the mouse is on swells (the
// first time with a drum) and shrinks when it leaves, and a closed options window is let go (the delay starts over).
// With the question window open, the answer yes sets the result to quit and no puts the window away, each once the
// window has closed.
bool InGameMenu::Update()
{
	if (!BBWindow::Update())
		return false;
	if (mDelay > 0) {
		mDelay--;
		return true;
	}
	if (mAskWin == nullptr) {
		for (int i = 0; i < IN_GAME_MENU_BUTTONS; i++) {
			ObjBase* aButton = mMenuButtons[i];
			if (aButton != nullptr) {
				if (aButton == sHover) {
					if (aButton->mHoverState == 0) {
						aButton->StartTrack(IN_GAME_BUTTON_TRACK_HOVER);
						aButton->mHoverState = 1;
						gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
					}
				} else if (aButton->mHoverState == 1) {
					aButton->StartTrack(IN_GAME_BUTTON_TRACK_LEAVE);
					aButton->mHoverState = 0;
				}
			}
		}
		if (mChoice == IN_GAME_MENU_OPTIONS && mOptions != nullptr) {
			if (mOptions->GetState() == BBWINDOW_CLOSED) {
				mOptions->Delete();
				mOptions = nullptr;
				mChoice = IN_GAME_MENU_OPEN;
				mDelay = IN_GAME_MENU_OPTIONS_DELAY;
			}
		}
	} else {
		int anAnswer = mAskWin->TakeChoice();
		if (anAnswer == ASK_CHOICE_YES) {
			if (mAskWin->IsClosed()) {
				mAskWin->Delete();
				mChoice = IN_GAME_MENU_QUIT;
				mAskWin = nullptr;
				mDelay = IN_GAME_MENU_ASK_DELAY;
				return true;
			}
		} else if (anAnswer == ASK_CHOICE_NO) {
			if (mAskWin->IsClosed()) {
				mAskWin->Delete();
				mAskWin = nullptr;
				mDelay = IN_GAME_MENU_ASK_DELAY;
				return true;
			}
		}
	}
	return true;
}

// 0x435ed0: while a result stands (not open), nothing is taken. Resume sets the result; options opens the options
// window; quit opens the question window ("EXIT GAME ?" over "ARE YOU SURE?"). Each button clicks. The window's own
// buttons (ids 100 and 101) are not among them, so BBWindow's call only matters for ids it knows.
bool InGameMenu::MouseUp(ObjBase* theObj)
{
	if (mChoice != IN_GAME_MENU_OPEN)
		return false;
	switch (theObj->mButtonId) {
	case IN_GAME_BUTTON_QUIT:
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		if (mAskWin == nullptr) {
			mAskWin = new AskWin();
			mAskWin->SetParent(this, nullptr);
			mAskWin->SetText("EXIT GAME ?", ASK_MODE_YES_NO, "ARE YOU SURE?");
		}
		break;
	case IN_GAME_BUTTON_OPTIONS:
		mChoice = IN_GAME_MENU_OPTIONS;
		mOptions = new Options();
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		break;
	case IN_GAME_BUTTON_RESUME:
		mChoice = IN_GAME_MENU_RESUME;
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		break;
	default:
		break;
	}
	BBWindow::MouseUp(theObj);
	return true;
}

// 0x436010 (EAX the menu): the result, once the delay is over
int InGameMenu::GetResult()
{
	if (mDelay != 0)
		return IN_GAME_MENU_OPEN;
	return mChoice;
}
