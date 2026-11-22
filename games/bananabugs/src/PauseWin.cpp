#include "PauseWin.h"

#include "Anim.h"
#include "Cursor.h"
#include "GameApp.h"
#include "GameWidget.h"
#include "LevelObj.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

PauseWin* PauseWin::sInstance = nullptr;
bool PauseWin::sWaiting = false;
bool PauseWin::sEnabled = false;
Anim* PauseWin::sButtonAnim = nullptr;

// The window's tracks
enum PauseTrack
{
	PAUSE_TRACK_OPEN = 0,
	PAUSE_TRACK_CLOSE = 1,
};

// The keys' event ids (AnimKey::mEventId)
const int PAUSE_EVENT_GROWN = 1;      // the open track's second key: the window waits for the release
const int PAUSE_EVENT_BUTTON_BACK = 2; // the picture's third track's first key; nothing answers it
const int PAUSE_EVENT_CLOSED = 3;     // the close track's last key: the window ends

const int PAUSE_BUTTON_ID = 100;      // the window's and its picture's (neither is answered)

const int PAUSE_COLUMNS = 7;          // the panel, in 100-pixel tiles
const int PAUSE_ROWS = 5;
const int PAUSE_FIRST_Z = 0x257;      // set before the panel is built; the second, after it, is the one that stays
const int PAUSE_Z = 0x7af;
const int PAUSE_LINE_SPACING = 64;
const int PAUSE_BUTTON_Y = 0x40;
const int PAUSE_OPEN_FRAMES = 3;
const int PAUSE_CLOSE_FRAMES = 10;
const int PAUSE_SHOT_COOLDOWN = 0x32; // frames before the monkey may shoot once the game goes on
const int PAUSE_UPDATE_LEVEL_WAITING = 1; // ObjBase's update level while the window waits for the release

// 0x454ef0: a window at no alpha and no scale, with a line of text in yellow, the picture, and its tracks
PauseWin::PauseWin()
{
	mUnused339 = false;
	sWaiting = false;
	SetStateInt(STATE_Z, PAUSE_FIRST_Z);
	BuildPanel(PAUSE_COLUMNS, PAUSE_ROWS, false);
	SetStateInt(STATE_Z, PAUSE_Z);
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetMouseable(32.0f, 32.0f);
	mButtonId = PAUSE_BUTTON_ID;
	ComputeDrawn();
	BuildLines(gApp->mFont, false);
	MoveLines(0, (float)PAUSE_LINE_SPACING, true);
	SetLineColor(0, 1.0f, 0.7f, 0.0f, 1.0f);
	SetLineScale(0, 1.2f, 1.3f);
	SetLineText(0, "Paused");
	BuildTracks();
	BuildButton();
	ObjBase::Activate(this, false);
}

// 0x455050 (body 0x455070): nothing of its own to let go (MenuObj's destructor follows)
PauseWin::~PauseWin()
{
}

// 0x455080 (theObj is the window itself: an object is its own event target unless given another): the open track's
// second key makes the game wait for the release (the update level goes to 1); the close track's last key ends the
// window, hides the system pointer again, lowers the update level, and, with a level on, hides the game's own pointer
// and keeps the monkey from shooting for a moment
void PauseWin::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr)
		return;
	int anEventId = theObj->mCurrentKey->mEventId;
	if (anEventId == PAUSE_EVENT_GROWN) {
		sWaiting = true;
		SetUpdateLevel(PAUSE_UPDATE_LEVEL_WAITING);
	} else if (anEventId == PAUSE_EVENT_CLOSED) {
		Delete();
		sInstance = nullptr;
		sWaiting = false;
		SetUpdateLevel(0);
		GameApp::HideCursor();
		if (sLevel != nullptr) {
			Cursor::Hide();
			Player::SetShotCooldown(PAUSE_SHOT_COOLDOWN);
		}
	}
}

// 0x455570: the open track (0) grows the window in 3 frames from nothing to its own size, then a key of one frame that
// tells the game it may wait, then holds; the close track (1) shrinks it away in 10 frames and ends with a key that
// tells the window to go
void PauseWin::BuildTracks()
{
	AnimKey* aGrow = new AnimKey();
	aGrow->SetDuration(PAUSE_OPEN_FRAMES);
	aGrow->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aGrow->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aGrown = new AnimKey();
	aGrown->mEventId = PAUSE_EVENT_GROWN;
	aGrown->SetDuration(1);
	aGrown->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aGrown->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aGrown->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimKey* aHold = new AnimKey();
	aHold->SetDuration(2);
	aHold->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aHold->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aHold->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* anOpenTrack = new AnimTrack();
	anOpenTrack->AddKey(aGrow, nullptr);
	anOpenTrack->AddKey(aGrown, nullptr);
	anOpenTrack->AddKey(aHold, aHold);
	AddTrack(anOpenTrack, nullptr);

	AnimKey* aShrink = new AnimKey();
	aShrink->SetDuration(PAUSE_CLOSE_FRAMES);
	aShrink->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aShrink->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aGone = new AnimKey();
	aGone->mEventId = PAUSE_EVENT_CLOSED;
	aGone->SetDuration(1);
	aGone->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aCloseTrack = new AnimTrack();
	aCloseTrack->AddKey(aShrink, nullptr);
	aCloseTrack->AddKey(aGone, nullptr);
	AddTrack(aCloseTrack, nullptr);
}

// 0x4550f0: the picture (the first cel of the in-level menu's buttons) below the line of text, at no alpha and no
// scale, with three tracks that nothing starts (the original never calls StartTrack on it): one that brings it in from
// double size, one that swells it by a fifth and one that puts it back, as a button that answered the mouse would
void PauseWin::BuildButton()
{
	ObjBase* aButton = new ObjBase();
	mButton = aButton;
	sButtonAnim->AddRef();
	aButton->mAnim = sButtonAnim;
	aButton->SetParent(this, nullptr);
	aButton->SetMouseable(32.0f, 32.0f);
	aButton->SetMouseListener(this);
	aButton->SetStateInt(STATE_ALPHA, 0);
	aButton->SetStateInt(STATE_X, 0);
	aButton->SetStateInt(STATE_Y, PAUSE_BUTTON_Y);
	aButton->SetStateInt(STATE_Z, 1); // the original passes the register the call before left, the channel 1
	aButton->SetStateInt(STATE_SCALE_X, 0);
	aButton->SetStateInt(STATE_SCALE_Y, 0);
	aButton->SetStateInt(STATE_FRAME, 0);
	aButton->mButtonId = PAUSE_BUTTON_ID;
	aButton->mHoverState = 0;

	AnimKey* aAppear = new AnimKey();
	aAppear->SetDuration(2);
	aAppear->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aAppear->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
	aAppear->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
	AnimKey* aShown = new AnimKey();
	aShown->SetDuration(2);
	aShown->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aShown->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
	aShown->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
	AnimTrack* aShowTrack = new AnimTrack();
	aShowTrack->AddKey(aAppear, nullptr);
	aShowTrack->AddKey(aShown, aShown);
	aButton->AddTrack(aShowTrack, nullptr);

	AnimKey* aSwellStart = new AnimKey();
	aSwellStart->SetDuration(5);
	aSwellStart->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aSwellStart->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aSwellEnd = new AnimKey();
	aSwellEnd->SetDuration(2);
	aSwellEnd->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aSwellEnd->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimTrack* aSwellTrack = new AnimTrack();
	aSwellTrack->AddKey(aSwellStart, nullptr);
	aSwellTrack->AddKey(aSwellEnd, aSwellEnd);
	aButton->AddTrack(aSwellTrack, nullptr);

	AnimKey* aShrinkStart = new AnimKey();
	aShrinkStart->mEventId = PAUSE_EVENT_BUTTON_BACK;
	aShrinkStart->SetDuration(5);
	aShrinkStart->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
	aShrinkStart->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
	AnimKey* aShrinkEnd = new AnimKey();
	aShrinkEnd->SetDuration(2);
	aShrinkEnd->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aShrinkEnd->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aBackTrack = new AnimTrack();
	aBackTrack->AddKey(aShrinkStart, nullptr);
	aBackTrack->AddKey(aShrinkEnd, aShrinkEnd);
	aButton->AddTrack(aBackTrack, nullptr);

	Activate(aButton, false);
}

// 0x4558f0
void Sexy::PreloadInGameButtons()
{
	PauseWin::sButtonAnim = Anim::Get("images/menu/ingame_buttons", ANIM_SCALE_SMOOTH, false, 0x167, 0x67, 3, false);
}

// 0x455920: with no pause window, the game loaded, no wait for a release and no pictures being made again, the window
// opens: an open in-level menu is closed first (LevelObj::CloseMenuForPause, with a level on), the window starts to
// grow, the system pointer shows, and the scene is updated one frame on (the widget's frame, counted up) so the window
// has begun to grow
void PauseWin::PauseForFocusLoss()
{
	if (sInstance == nullptr && sEnabled && !sWaiting && !AnimReload::sActive) {
		if (sLevel != nullptr)
			LevelObj::CloseMenuForPause();
		PauseWin* aWindow = new PauseWin();
		sInstance = aWindow;
		aWindow->StartTrack(PAUSE_TRACK_OPEN);
		aWindow->Activate(aWindow, false);
		GameApp::ShowCursor();
		gApp->mGameWidget->mFrame++;
		ObjBase::UpdateAll(gApp->mGameWidget->mFrame);
	}
}

// 0x455a10
bool PauseWin::IsBlocking()
{
	if (sWaiting)
		return true;
	return sInstance != nullptr;
}

// 0x455a40: a release while the window waits: a click, the shrinking track, the update level back, and the widget
// ignores the mouse for a while
void PauseWin::ResumeOnRelease()
{
	if (sInstance != nullptr && sWaiting) {
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		sInstance->StartTrack(PAUSE_TRACK_CLOSE);
		sWaiting = false;
		SetUpdateLevel(0);
		gApp->mGameWidget->LockInput();
	}
}
