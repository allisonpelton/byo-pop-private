#include "MainMenu.h"

#include "Anim.h"
#include "AskWin.h"
#include "Cursor.h"
#include "Emitter.h"
#include "GameApp.h"
#include "Help.h"
#include "HighScore.h"
#include "Map.h"
#include "NewUser.h"
#include "ObjString.h"
#include "Options.h"
#include "Pending.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "Transition.h"
#include "UserSelect.h"

using namespace Sexy;

bool MainMenu::sShowHighScores = false;

// The object type of a button that takes the mouse (the same value BBWindow gives its buttons)
const int OBJ_TYPE_MENU_BUTTON = 0xf;

// The buttons' ids (ObjBase::mButtonId): the five buttons count from 100, the sign has its own
enum MenuButtonId
{
	MENU_ID_PLAY = 100,
	MENU_ID_OPTIONS = 101,
	MENU_ID_HIGH_SCORES = 102,
	MENU_ID_HELP = 103,
	MENU_ID_EXIT = 104,
	MENU_ID_SIGN = 500,
};

// The key event that tells the menu a button has landed (AnimKey::mEventId)
const int MENU_EVENT_BUTTON_LANDED = 100;

// The tracks of a button's group
enum ButtonGroupTrack
{
	GROUP_TRACK_LAND = 0,
	GROUP_TRACK_SWELL = 1,
	GROUP_TRACK_SHRINK = 2,
};

// The tree's tracks
enum TreeTrack
{
	TREE_TRACK_HOLD = 0,
	TREE_TRACK_JOLT = 1,
};

const int SIGN_TRACK_DROP = 0;

// How long the menu waits after the doors open before it takes the mouse
const int MENU_START_DELAY = 50;

// 0x43fc90
MainMenu::MainMenu()
{
	SetParent(nullptr, nullptr);
	mMap = nullptr;
	mNewUser = nullptr;
	mMapTimer = 0;
	mHighScore = nullptr;
	mAskWin = nullptr;
	mHelp = nullptr;
	mOptions = nullptr;
	mUserSelect = nullptr;
	mChoice = MENU_CHOICE_NONE;
	mMenuState = MENU_STATE_IDLE;
	mReady = false;
	mStartDelay = MENU_START_DELAY;
	SetState(STATE_Z, 0.01f);
	mAnim = Anim::Get("images/mainmenu_background", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mHut01Anim = Anim::Get("images/mainmenu_hut_01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mHut02Anim = Anim::Get("images/mainmenu_hut_02", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mTreeAnim = Anim::Get("images/mainmenu_tree", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mPlantsAnim = Anim::Get("images/mainmenu_plants", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mPlant01Anim = Anim::Get("images/mainmenu_plant_01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mPlant02Anim = Anim::Get("images/mainmenu_plant_02", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mPlant03Anim = Anim::Get("images/mainmenu_plant_03", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mRaysAnim = Anim::Get("images/mainmenu_rays", ANIM_SCALE_FAST, false, 0, 0, 1, false);
	mVines01Anim = Anim::Get("images/mainmenu_vines_01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mVines02Anim = Anim::Get("images/mainmenu_vines_02", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mFogAnim = Anim::Get("images/mainmenu_fog", ANIM_SCALE_FAST, false, 0, 0, 1, false);
	mSmokeAnim = Anim::Get("images/mainmenu_chimney_smoke", ANIM_SCALE_FAST, false, 221, 362, 3, false);
	mLogoAnim = Anim::Get("images/mainmenu_logo", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mButtonsAnim = Anim::Get("images/mainmenu_buttons", ANIM_SCALE_SMOOTH, false, 265, 130, 5, false);
	BuildHuts();
	BuildSmoke1();
	BuildSmoke2();
	BuildSmoke3();
	BuildRayRight();
	BuildRayLeft();
	BuildRayCentre();
	BuildVines02Left();
	BuildVines02Right();
	BuildVines01Left();
	BuildVines01Middle();
	BuildVines01Right();
	BuildFogUpper();
	BuildFogLower();
	BuildTree();
	BuildLogo();
	BuildButtons();
	BuildPlant01();
	BuildPlant02();
	BuildPlant03First();
	BuildPlant03Second();
	BuildPlant03Third();
	BuildPlant03Fourth();
	BuildPlantsStrip();
	BuildHoldTrack();
	mFadeTrack = BuildFadeTrack(this);
	mLeaveTrack = BuildLeaveTrack(this);
	BuildSign();
	Activate(this, false);
	Cursor::Show();
	mButtonsArrived = 0;
	if (sShowHighScores) {
		mHighScore = new HighScore();
		mHighScore->SetParent(this, nullptr);
		mMenuState = MENU_STATE_HIGH_SCORES;
		mChoice = MENU_CHOICE_HIGH_SCORES;
		sShowHighScores = false;
	}
}

// 0x440000 (body 0x440020): the objects go with the ObjBase's children. The original does not release the menu's
// pictures (they stay loaded) and never frees the buttons' emitter sets; the port frees the sets, which takes their
// emitters (children of the button groups) out first.
MainMenu::~MainMenu()
{
	for (int i = 0; i < MENU_BUTTONS; i++)
		delete mButtonGroups[i]->mEmitters;
}

// 0x440030: a button has landed (the key that plays the drum sound ended): the tree jolts and the landing sprays
// slow swell particles over the button's picture. The fifth landing shows the sign, or asks for a new player's name
// when there is no profile yet.
void MainMenu::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || theObj->mCurrentKey->mEventId != MENU_EVENT_BUTTON_LANDED)
		return;
	ObjBase* aTree = mTree;
	mButtonsArrived++;
	aTree->StartTrack(TREE_TRACK_JOLT);
	float aCelWidth = mButtonsAnim->GetSourceCelWidth();
	float aHalfWidth = (float)((double)aCelWidth * 0.5);
	float aCelHeight = mButtonsAnim->GetSourceCelHeight();
	float aHalfHeight = (float)((double)aCelHeight * 0.5);
	float aLimitX = (float)((double)aCelWidth - 32.0);
	float aLimitY = (float)((double)aCelHeight - 32.0);
	int aMaxY = (int)aLimitY;
	if (aMaxY >= 32) {
		int aMaxX = (int)aLimitX;
		for (int aY = 32; aY <= aMaxY; aY += 32) {
			for (int aX = 32; aX <= aMaxX; aX += 32) {
				theObj->mEmitters->BurstSlowSwell((float)((double)aX - (double)aHalfWidth + 8.0),
					(float)((double)aY - (double)aHalfHeight + 8.0));
			}
		}
	}
	if (mButtonsArrived == MENU_BUTTONS) {
		const char* aName = GetProfileName();
		if (aName == nullptr) {
			mNewUser = new NewUser(true);
			mNewUser->Open(this);
			mMenuState = MENU_STATE_NEW_USER;
			gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
		} else {
			mSignText->SetText(aName);
			mSign->StartTrack(SIGN_TRACK_DROP);
			Activate(mSign, false);
		}
	}
}

// 0x440240: while the doors are open and the start delay has run out, the menu takes the mouse and watches the window
// or screen it opened last; once it has closed, the menu cleans up and goes on. In the idle state the buttons swell
// for the mouse.
bool MainMenu::Update()
{
	if (!ObjBase::Update())
		return false;
	mReady = false;
	if (Transition::GetPhase() != TRANSITION_OPEN)
		return true;
	if (mStartDelay > 0) {
		mStartDelay--;
		return true;
	}
	mReady = true;
	switch (mMenuState) {
	case MENU_STATE_CONTINUE_ASK:
		if (mAskWin != nullptr) {
			int aChoice = mAskWin->TakeChoice();
			if (aChoice == ASK_CHOICE_YES) {
				mMenuState = MENU_STATE_CONTINUE_CLOSING;
				if (Player::sCurrent->mMetaGameStarted)
					MetaGame::ClearFirstVisit();
			} else if (aChoice == ASK_CHOICE_NO) {
				ResetProfile();
				SaveProfiles();
				mMenuState = MENU_STATE_CONTINUE_CLOSING;
			} else if (aChoice == ASK_CHOICE_OTHER) {
				mMenuState = MENU_STATE_IDLE;
				mAskWin->Delete();
				mAskWin = nullptr;
			}
		}
		break;
	case MENU_STATE_CONTINUE_CLOSING:
		if (mAskWin->IsClosed()) {
			mMenuState = MENU_STATE_START_MAP;
			mAskWin->Delete();
			mAskWin = nullptr;
		}
		break;
	case MENU_STATE_START_MAP:
		if (mMap == nullptr) {
			mMap = new Map();
			mMap->Start(Player::sCurrent->mMapLevel - 1, Player::sCurrent->mMapLevel);
			mMapTimer = 2000;
			mMenuState = MENU_STATE_MAP;
		}
		break;
	case MENU_STATE_MAP:
		if (mMap->IsDone()) {
			mChoice = MENU_CHOICE_PLAY;
			mMap->Delete();
			mMap = nullptr;
			mMenuState = MENU_STATE_LEFT;
		}
		break;
	case MENU_STATE_OPTIONS:
		if (mOptions != nullptr && mOptions->GetState() == BBWINDOW_CLOSED) {
			mOptions->Delete();
			mOptions = nullptr;
			mMenuState = MENU_STATE_IDLE;
		}
		break;
	case MENU_STATE_HIGH_SCORES:
		if (mHighScore != nullptr && mHighScore->GetState() == BBWINDOW_CLOSED) {
			mHighScore->Delete();
			mHighScore = nullptr;
			mMenuState = MENU_STATE_IDLE;
		}
		break;
	case MENU_STATE_HELP:
		// the help screen deletes itself
		if (mHelp != nullptr && mHelp->IsDone()) {
			mHelp = nullptr;
			mMenuState = MENU_STATE_IDLE;
		}
		break;
	case MENU_STATE_NEW_USER:
		if (mNewUser != nullptr && mNewUser->GetState() == BBWINDOW_CLOSED) {
			mNewUser->Delete();
			mNewUser = nullptr;
			ShowSignName();
			mMenuState = MENU_STATE_IDLE;
		}
		break;
	case MENU_STATE_USER_SELECT:
		if (mUserSelect != nullptr && mUserSelect->GetState() == BBWINDOW_CLOSED) {
			mUserSelect->Delete();
			mUserSelect = nullptr;
			ShowSignName();
			mMenuState = MENU_STATE_IDLE;
		}
		break;
	case MENU_STATE_EXIT_ASK:
		if (mAskWin != nullptr) {
			int aChoice = mAskWin->TakeChoice();
			if (aChoice == ASK_CHOICE_YES)
				mMenuState = MENU_STATE_EXIT_YES;
			else if (aChoice == ASK_CHOICE_NO)
				mMenuState = MENU_STATE_EXIT_NO;
		}
		break;
	case MENU_STATE_EXIT_NO:
		if (mAskWin->IsClosed()) {
			mMenuState = MENU_STATE_IDLE;
			mAskWin->Delete();
			mAskWin = nullptr;
		}
		break;
	case MENU_STATE_EXIT_YES:
		if (mAskWin != nullptr && mAskWin->IsClosed()) {
			mChoice = MENU_CHOICE_QUIT;
			mAskWin->Delete();
			mAskWin = nullptr;
		}
		break;
	default:
		if (mState[STATE_ROTATION] == 0.0f && mButtonsArrived >= MENU_BUTTONS) {
			for (int i = 0; i < MENU_BUTTONS; i++) {
				ObjBase* aGroup = mButtonGroups[i];
				if (mButtons[i] == sHover) {
					if (aGroup->mHoverState == 0) {
						aGroup->StartTrack(GROUP_TRACK_SWELL);
						aGroup->mHoverState = 1;
						gApp->mSoundQueue->Request(SOUND_DRUM_C, 0, false);
					}
				} else if (aGroup->mHoverState == 1) {
					aGroup->StartTrack(GROUP_TRACK_SHRINK);
					aGroup->mHoverState = 0;
				}
			}
		}
		break;
	}
	return true;
}

// The sign takes the current profile's name and drops in if it was not showing (the end of 0x440240's two cases that
// close the name windows)
void MainMenu::ShowSignName()
{
	mSignText->SetText(GetProfileName());
	if (!mSign->IsActive()) {
		mSign->StartTrack(SIGN_TRACK_DROP);
		Activate(mSign, false);
	}
}

// 0x440720: a button press opens what it stands for, if nothing else is open: play asks about continuing the last
// game when there is one, else starts the map; the others open the options, the high scores, the help, an "exit
// game?" question and the player list (the sign). Every press that gets this far ends the mouse hold.
bool MainMenu::MouseUp(ObjBase* theObj)
{
	if (!mReady)
		return false;
	int anId = theObj->mButtonId;
	bool aClick = false;
	switch (anId) {
	case MENU_ID_PLAY:
		if (mAskWin == nullptr) {
			gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
			if (Player::sCurrent->mSavedProgress > 0) {
				mAskWin = new AskWin();
				mAskWin->SetParent(this, nullptr);
				mAskWin->SetText("CONTINUE LAST", ASK_MODE_YES_NO_CANCEL, "GAME ?");
				mMenuState = MENU_STATE_CONTINUE_ASK;
			} else if (mMap == nullptr) {
				mMenuState = MENU_STATE_START_MAP;
			}
			mReady = false;
		}
		break;
	case MENU_ID_OPTIONS:
		if (mOptions == nullptr) {
			mOptions = new Options();
			mOptions->SetParent(this, nullptr);
			mMenuState = MENU_STATE_OPTIONS;
			mChoice = MENU_CHOICE_OPTIONS;
			aClick = true;
		}
		break;
	case MENU_ID_HIGH_SCORES:
		if (mHighScore == nullptr) {
			mHighScore = new HighScore();
			mHighScore->SetParent(this, nullptr);
			mMenuState = MENU_STATE_HIGH_SCORES;
			mChoice = MENU_CHOICE_HIGH_SCORES;
			aClick = true;
		}
		break;
	case MENU_ID_HELP:
		if (mHelp == nullptr) {
			mHelp = new Help();
			mHelp->SetParent(this, nullptr);
			mHelp->Start();
			mMenuState = MENU_STATE_HELP;
			mChoice = MENU_CHOICE_HELP;
			aClick = true;
		}
		break;
	case MENU_ID_EXIT:
		if (mAskWin == nullptr) {
			mAskWin = new AskWin();
			mAskWin->SetParent(this, nullptr);
			mAskWin->SetText("EXIT GAME", ASK_MODE_YES_NO, "ARE YOU SURE ?");
			mMenuState = MENU_STATE_EXIT_ASK;
			aClick = true;
		}
		break;
	case MENU_ID_SIGN:
		// the original does not make the window a child of the menu
		if (mUserSelect == nullptr) {
			mUserSelect = new UserSelect();
			mMenuState = MENU_STATE_USER_SELECT;
			mChoice = MENU_CHOICE_USER_SELECT;
			aClick = true;
		}
		break;
	default:
		break;
	}
	if (aClick)
		gApp->mSoundQueue->Request(SOUND_MOUSECLICK, 0, false);
	sMouseHeld = false;
	return true;
}

// 0x4059c0: the object shrinks to nothing over 120 frames, then swells to 1.5 as it fades to half over 10 more, and
// goes (the keys end it)
int Sexy::BuildLeaveTrack(ObjBase* theObj)
{
	AnimKey* aShrinkKey = new AnimKey();
	aShrinkKey->mEventId = 0;
	aShrinkKey->SetDuration(120);
	aShrinkKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aShrinkKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aShrinkKey->AddValue(STATE_ALPHA, 0.9f, MOD_TOWARD_NEXT);
	aShrinkKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aShrinkKey->mEasing = EASE_IN_SQUARE;
	AnimKey* aSwellKey = new AnimKey();
	aSwellKey->mEventId = 0;
	aSwellKey->SetRemoveAtEnd();
	aSwellKey->SetDuration(10);
	aSwellKey->AddValue(STATE_SCALE_X, 1.5f, MOD_TOWARD_NEXT);
	aSwellKey->AddValue(STATE_SCALE_Y, 1.5f, MOD_TOWARD_NEXT);
	aSwellKey->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	aSwellKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aLastKey = new AnimKey();
	aLastKey->mEventId = 0;
	aLastKey->SetRemoveAtEnd();
	aLastKey->SetDuration(1);
	aLastKey->AddValue(STATE_SCALE_X, 1.5f, MOD_TOWARD_NEXT);
	aLastKey->AddValue(STATE_SCALE_Y, 1.5f, MOD_TOWARD_NEXT);
	aLastKey->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	aLastKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aShrinkKey, nullptr);
	aTrack->AddKey(aSwellKey, nullptr);
	aTrack->AddKey(aLastKey, aLastKey);
	return theObj->AddTrack(aTrack, nullptr);
}

// 0x405c80: the object starts at 1.5 times its size and falls to nothing over 120 frames, fading from clear to 0.8
// opaque over 10 more, then holds opaque and goes
int Sexy::BuildFadeTrack(ObjBase* theObj)
{
	AnimKey* aFallKey = new AnimKey();
	aFallKey->mEventId = 0;
	aFallKey->SetDuration(120);
	aFallKey->AddValue(STATE_SCALE_X, 1.5f, MOD_TOWARD_NEXT);
	aFallKey->AddValue(STATE_SCALE_Y, 1.5f, MOD_TOWARD_NEXT);
	aFallKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aFallKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	aFallKey->mEasing = EASE_IN_SQUARE;
	AnimKey* aFadeKey = new AnimKey();
	aFadeKey->mEventId = 1;
	aFadeKey->SetDuration(10);
	aFadeKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aFadeKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aFadeKey->AddValue(STATE_ALPHA, 0.8f, MOD_TOWARD_NEXT);
	aFadeKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimKey* aLastKey = new AnimKey();
	aLastKey->mEventId = 3;
	aLastKey->SetDuration(1);
	aLastKey->SetRemoveAtEnd();
	aLastKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
	aLastKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
	aLastKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
	aLastKey->AddValue(STATE_ROTATION, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFallKey, nullptr);
	aTrack->AddKey(aFadeKey, nullptr);
	aTrack->AddKey(aLastKey, aLastKey);
	return theObj->AddTrack(aTrack, nullptr);
}

// 0x440a70: a track that holds the menu's scale, alpha and rotation at 0; nothing starts it
void MainMenu::BuildHoldTrack()
{
	AnimKey* aKey = new AnimKey();
	aKey->mEventId = 0;
	aKey->SetDuration(1);
	aKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey, aKey);
	mHoldTrack = AddTrack(aTrack, nullptr);
}

// 0x444090: the five buttons, down the tree's right side 133 apart. Each is a group (an object with the burst
// emitter, which swells and shrinks for the mouse) holding the button itself, a cel of the buttons' picture that takes
// the mouse. The group's first track lands it: after a delay of 10 frames and 20 for each button down, it spins
// and shrinks into place over 20 frames, plays the drum, and tells the menu (event 100, which sprays the particles).
void MainMenu::BuildButtons()
{
	for (int i = 0, aDelay = 10, anOffset = 0; aDelay < 110; i++, aDelay += 20, anOffset += 133) {
		ObjBase* aGroup = new ObjBase();
		mButtonGroups[i] = aGroup;
		aGroup->mType = OBJ_TYPE_ROOT;
		aGroup->SetParent(mTree, nullptr);
		aGroup->SetEventTarget(this);
		aGroup->mButtonId = i;
		aGroup->mHoverState = 0;
		Activate(aGroup, false);
		aGroup->mEmitters = new EmitterSet(aGroup);
		aGroup->mEmitters->MakeSlowSwell(0.0f);

		ObjBase* aButton = new ObjBase();
		mButtons[i] = aButton;
		aButton->mType = OBJ_TYPE_MENU_BUTTON;
		aButton->SetMouseable(32.0f, 32.0f);
		mButtonsAnim->AddRef();
		aButton->mAnim = mButtonsAnim;
		aButton->SetParent(aGroup, nullptr);
		aButton->mButtonId = MENU_ID_PLAY + i;
		aButton->SetStateInt(STATE_Z, 1);
		aButton->SetStateInt(STATE_FRAME, i);
		aButton->SetMouseListener(this);
		Activate(aButton, false);

		aGroup->SetStateInt(STATE_ALPHA, 0);
		aGroup->SetStateInt(STATE_X, -96);
		aGroup->SetState(STATE_Y, (float)((double)anOffset - 180.0));
		aGroup->SetStateInt(STATE_Z, 5);

		AnimKey* aSpinKey = new AnimKey();
		aSpinKey->mEventId = 0;
		aSpinKey->SetDuration(20);
		aSpinKey->AddValue(STATE_SCALE_X, 2.0f, MOD_TOWARD_NEXT);
		aSpinKey->AddValue(STATE_SCALE_Y, 2.0f, MOD_TOWARD_NEXT);
		aSpinKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aSpinKey->AddValue(STATE_ROTATION, (i & 1) != 0 ? 1.3962635f : -1.3962635f, MOD_TOWARD_NEXT);
		aSpinKey->mEasing = EASE_IN_CUBE;
		AnimKey* aDrumKey = new AnimKey();
		aDrumKey->mEndCondition = KEY_SOUND;
		aDrumKey->mEndChannel = SOUND_DRUM_C;
		aDrumKey->mEndValue = 2.0f;
		aDrumKey->mEventId = MENU_EVENT_BUTTON_LANDED;
		aDrumKey->SetDuration(1);
		aDrumKey->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aDrumKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
		aDrumKey->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aDrumKey->AddValue(STATE_ROTATION, (i & 1) != 0 ? 0.08726647f : -0.08726647f, MOD_HOLD);
		AnimKey* aRestKey = new AnimKey();
		aRestKey->mEventId = 0;
		aRestKey->SetDuration(1);
		AnimTrack* aLandTrack = new AnimTrack();
		aLandTrack->SetDelay(aDelay, false);
		aLandTrack->AddKey(aSpinKey, nullptr);
		aLandTrack->AddKey(aDrumKey, nullptr);
		aLandTrack->AddKey(aRestKey, aRestKey);
		aGroup->AddTrack(aLandTrack, nullptr);

		AnimKey* aSwellStart = new AnimKey();
		aSwellStart->SetDuration(5);
		aSwellStart->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aSwellStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimKey* aSwellEnd = new AnimKey();
		aSwellEnd->SetDuration(2);
		aSwellEnd->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
		aSwellEnd->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
		AnimTrack* aSwellTrack = new AnimTrack();
		aSwellTrack->AddKey(aSwellStart, nullptr);
		aSwellTrack->AddKey(aSwellEnd, aSwellEnd);
		aGroup->AddTrack(aSwellTrack, nullptr);

		AnimKey* aShrinkStart = new AnimKey();
		aShrinkStart->SetDuration(5);
		aShrinkStart->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
		aShrinkStart->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
		AnimKey* aShrinkEnd = new AnimKey();
		aShrinkEnd->SetDuration(2);
		aShrinkEnd->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aShrinkEnd->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		AnimTrack* aShrinkTrack = new AnimTrack();
		aShrinkTrack->AddKey(aShrinkStart, nullptr);
		aShrinkTrack->AddKey(aShrinkEnd, aShrinkEnd);
		aGroup->AddTrack(aShrinkTrack, nullptr);
	}
}

// 0x4446d0: the sign at the tree's foot, which takes the mouse (button 500) and shows the player's name in yellow
// over it. Its one track drops it 300 pixels into place over 20 frames; it stays out of the scene until the last
// button has landed (Notify) or a name is chosen.
void MainMenu::BuildSign()
{
	mSign = new ObjBase();
	mSign->mAnim = Anim::Get("images/mainmenu/mainmenu_sign", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	mSign->SetParent(this, nullptr);
	mSign->SetStateInt(STATE_X, 650);
	mSign->SetStateInt(STATE_Y, 450);
	mSign->SetStateInt(STATE_Z, 50);
	mSign->SetMouseable(32.0f, 32.0f);
	mSign->SetMouseListener(this);
	mSign->mButtonId = MENU_ID_SIGN;
	mSign->mHoverState = 0;
	mSignText = new ObjString();
	mSignText->SetParent(mSign, nullptr);
	mSignText->SetStateInt(STATE_X, 0);
	mSignText->SetStateInt(STATE_Y, -34);
	mSignText->SetStateInt(STATE_Z, 1);
	mSignText->SetState(STATE_SCALE_X, 0.5f);
	mSignText->SetState(STATE_SCALE_Y, 0.75f);
	mSignText->SetFont(gApp->mFont);
	mSignText->SetStateInt(STATE_RED, 1);
	mSignText->SetState(STATE_GREEN, 0.8f);
	mSignText->SetState(STATE_BLUE, 0.3f);
	mSignText->SetStateInt(STATE_ALPHA, 1);
	mSignText->Create("000000000000000", false, 0);
	mSignText->SetText(" ");
	AnimKey* aDropKey = new AnimKey();
	aDropKey->SetDuration(20);
	aDropKey->AddValue(STATE_Y, 300.0f, MOD_TOWARD_NEXT);
	aDropKey->mEasing = EASE_IN_CUBE;
	AnimKey* aRestKey = new AnimKey();
	aRestKey->SetDuration(10);
	aRestKey->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aDropKey, nullptr);
	aTrack->AddKey(aRestKey, aRestKey);
	mSign->AddTrack(aTrack, nullptr);
	Activate(mSignText, true);
}

// 0x440b70: the two huts, each at its place with its picture centred on it
void MainMenu::BuildHuts()
{
	mHut01 = new ObjBase();
	mHut01Anim->AddRef();
	mHut01->mAnim = mHut01Anim;
	mHut01->SetParent(this, nullptr);
	mHut01->SetState(STATE_OFFSET_X, (float)((double)mHut01Anim->GetSourceCelWidth() * 0.5));
	mHut01->SetState(STATE_OFFSET_Y, (float)((double)mHut01Anim->GetSourceCelHeight() * 0.5));
	mHut01->SetState(STATE_X, FromCorner(111.0f, 1920.0f));
	mHut01->SetState(STATE_Y, FromCorner(337.0f, 1200.0f));
	mHut01->SetStateInt(STATE_Z, 5);
	mHut01->Activate(mHut01, false);
	mHut02 = new ObjBase();
	mHut02Anim->AddRef();
	mHut02->mAnim = mHut02Anim;
	mHut02->SetParent(this, nullptr);
	mHut02->SetState(STATE_OFFSET_X, (float)((double)mHut02Anim->GetSourceCelWidth() * 0.5));
	mHut02->SetState(STATE_OFFSET_Y, (float)((double)mHut02Anim->GetSourceCelHeight() * 0.5));
	mHut02->SetState(STATE_X, FromCorner(1233.0f, 1920.0f));
	mHut02->SetState(STATE_Y, FromCorner(260.0f, 1200.0f));
	mHut02->SetStateInt(STATE_Z, 5);
	mHut02->Activate(mHut02, false);
}

// 0x440db0: the first puff of the chimney's smoke (cel 0): four keys of 150 frames that widen and thin it, looping
void MainMenu::BuildSmoke1()
{
	mSmoke[0] = new ObjBase();
	mSmoke[0]->SetParent(this, nullptr);
	mSmoke[0]->SetState(STATE_X, FromCorner(1770.0f, 1920.0f));
	mSmoke[0]->SetState(STATE_Y, FromCorner(160.0f, 1200.0f));
	mSmoke[0]->SetStateInt(STATE_Z, 4);
	mSmoke[0]->Activate(mSmoke[0], false);
	AnimKey* aFirstKey = new AnimKey();
	mSmokeAnim->AddRef();
	aFirstKey->mAnim = mSmokeAnim;
	aFirstKey->SetDuration(150);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimKey* aSecondKey = new AnimKey();
	mSmokeAnim->AddRef();
	aSecondKey->mAnim = mSmokeAnim;
	aSecondKey->SetDuration(150);
	aSecondKey->AddValue(STATE_SCALE_X, -0.02f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_ALPHA, -0.8f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimKey* aThirdKey = new AnimKey();
	mSmokeAnim->AddRef();
	aThirdKey->mAnim = mSmokeAnim;
	aThirdKey->SetDuration(150);
	aThirdKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimKey* aFourthKey = new AnimKey();
	mSmokeAnim->AddRef();
	aFourthKey->mAnim = mSmokeAnim;
	aFourthKey->SetDuration(150);
	aFourthKey->AddValue(STATE_SCALE_X, 0.05f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_ALPHA, -0.8f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_FRAME, 0.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	aTrack->AddKey(aFourthKey, aFirstKey);
	mSmoke[0]->AddTrack(aTrack, nullptr);
}

// 0x441180: the second puff (cel 1), out of step with the first: four keys of 150 frames, looping
void MainMenu::BuildSmoke2()
{
	mSmoke[1] = new ObjBase();
	mSmoke[1]->SetParent(this, nullptr);
	mSmoke[1]->SetState(STATE_X, FromCorner(1770.0f, 1920.0f));
	mSmoke[1]->SetState(STATE_Y, FromCorner(160.0f, 1200.0f));
	mSmoke[1]->SetStateInt(STATE_Z, 4);
	mSmoke[1]->Activate(mSmoke[1], false);
	AnimKey* aFirstKey = new AnimKey();
	mSmokeAnim->AddRef();
	aFirstKey->mAnim = mSmokeAnim;
	aFirstKey->SetDuration(150);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, -0.8f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
	AnimKey* aSecondKey = new AnimKey();
	mSmokeAnim->AddRef();
	aSecondKey->mAnim = mSmokeAnim;
	aSecondKey->SetDuration(150);
	aSecondKey->AddValue(STATE_SCALE_X, 0.03f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
	AnimKey* aThirdKey = new AnimKey();
	mSmokeAnim->AddRef();
	aThirdKey->mAnim = mSmokeAnim;
	aThirdKey->SetDuration(150);
	aThirdKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_ALPHA, -0.8f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
	AnimKey* aFourthKey = new AnimKey();
	mSmokeAnim->AddRef();
	aFourthKey->mAnim = mSmokeAnim;
	aFourthKey->SetDuration(150);
	aFourthKey->AddValue(STATE_SCALE_X, -0.02f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_FRAME, 1.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	aTrack->AddKey(aFourthKey, aFirstKey);
	mSmoke[1]->AddTrack(aTrack, nullptr);
}

// 0x441560: the third puff (cel 2): four keys of 75 frames, looping
void MainMenu::BuildSmoke3()
{
	mSmoke[2] = new ObjBase();
	mSmoke[2]->SetParent(this, nullptr);
	mSmoke[2]->SetState(STATE_X, FromCorner(1770.0f, 1920.0f));
	mSmoke[2]->SetState(STATE_Y, FromCorner(160.0f, 1200.0f));
	mSmoke[2]->SetStateInt(STATE_Z, 4);
	mSmoke[2]->Activate(mSmoke[2], false);
	AnimKey* aFirstKey = new AnimKey();
	mSmokeAnim->AddRef();
	aFirstKey->mAnim = mSmokeAnim;
	aFirstKey->SetDuration(75);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, -0.7f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_FRAME, 2.0f, MOD_HOLD);
	AnimKey* aSecondKey = new AnimKey();
	mSmokeAnim->AddRef();
	aSecondKey->mAnim = mSmokeAnim;
	aSecondKey->SetDuration(75);
	aSecondKey->AddValue(STATE_SCALE_X, 0.03f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_ALPHA, -0.3f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_FRAME, 2.0f, MOD_HOLD);
	AnimKey* aThirdKey = new AnimKey();
	mSmokeAnim->AddRef();
	aThirdKey->mAnim = mSmokeAnim;
	aThirdKey->SetDuration(75);
	aThirdKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_ALPHA, -0.7f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_FRAME, 2.0f, MOD_HOLD);
	AnimKey* aFourthKey = new AnimKey();
	mSmokeAnim->AddRef();
	aFourthKey->mAnim = mSmokeAnim;
	aFourthKey->SetDuration(75);
	aFourthKey->AddValue(STATE_SCALE_X, -0.02f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_ALPHA, -0.3f, MOD_TOWARD_NEXT);
	aFourthKey->AddValue(STATE_FRAME, 2.0f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	aTrack->AddKey(aFourthKey, aFirstKey);
	mSmoke[2]->AddTrack(aTrack, nullptr);
}

// 0x441940: the tree, which the buttons hang from: a track that holds it at 1% scale and one that jolts it for 10
// frames as each button lands (Notify)
void MainMenu::BuildTree()
{
	mTree = new ObjBase();
	mTreeAnim->AddRef();
	mTree->mAnim = mTreeAnim;
	mTree->SetParent(this, nullptr);
	mTree->SetState(STATE_X, FromCorner(1100.0f, 1920.0f));
	mTree->SetState(STATE_Y, FromCorner(600.0f, 1200.0f));
	mTree->SetStateInt(STATE_Z, 0xa);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(1);
	aFirstKey->AddValue(STATE_SCALE_X, 0.01f, MOD_HOLD);
	aFirstKey->AddValue(STATE_SCALE_Y, 0.01f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, aFirstKey);
	mTree->AddTrack(aTrack, nullptr);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(5);
	aSecondKey->AddValue(STATE_SCALE_X, 0.01f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_Y, 0.01f, MOD_TOWARD_NEXT);
	AnimKey* aThirdKey = new AnimKey();
	aThirdKey->SetDuration(5);
	aThirdKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aThirdKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aFourthKey = new AnimKey();
	aFourthKey->SetDuration(1);
	aFourthKey->AddValue(STATE_SCALE_X, 0.01f, MOD_HOLD);
	aFourthKey->AddValue(STATE_SCALE_Y, 0.01f, MOD_HOLD);
	AnimTrack* aSecondTrack = new AnimTrack();
	aSecondTrack->AddKey(aSecondKey, nullptr);
	aSecondTrack->AddKey(aThirdKey, nullptr);
	aSecondTrack->AddKey(aFourthKey, nullptr);
	mTree->AddTrack(aSecondTrack, aTrack);
	mTree->Activate(mTree, false);
}

// 0x441c80: the strip of plants along the foot of the screen: a track of two keys with no values (220 and 260 frames)
// that loops, so it only shows its picture
void MainMenu::BuildPlantsStrip()
{
	mPlantsStrip = new ObjBase();
	mPlantsAnim->AddRef();
	mPlantsStrip->mAnim = mPlantsAnim;
	mPlantsStrip->SetParent(this, nullptr);
	mPlantsStrip->SetState(STATE_OFFSET_Y, (float)((double)mPlantsAnim->GetSourceCelHeight() * 0.5));
	mPlantsStrip->SetState(STATE_X, FromCorner(960.0f, 1920.0f));
	mPlantsStrip->SetState(STATE_Y, FromCorner(838.0f, 1200.0f));
	mPlantsStrip->SetStateInt(STATE_Z, 0x14);
	mPlantsStrip->Activate(mPlantsStrip, false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(220);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(260);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlantsStrip->AddTrack(aTrack, nullptr);
}

// 0x441e00: the first plant, on the bottom edge at x 1700: it sways half a degree to either side over about 280
// frames, looping
void MainMenu::BuildPlant01()
{
	mPlant01 = new ObjBase();
	mPlant01Anim->AddRef();
	mPlant01->mAnim = mPlant01Anim;
	mPlant01->SetParent(this, nullptr);
	mPlant01->SetState(STATE_OFFSET_Y, (float)(-(double)mPlant01Anim->GetSourceCelHeight() * 0.5));
	mPlant01->SetState(STATE_X, FromCorner(1700.0f, 1920.0f));
	mPlant01->SetState(STATE_Y, FromCorner(1200.0f, 1200.0f));
	mPlant01->SetStateInt(STATE_Z, 0x13);
	mPlant01->Activate(mPlant01, false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(280);
	aFirstKey->AddValue(STATE_ROTATION, -0.008726646f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(250);
	aSecondKey->AddValue(STATE_ROTATION, 0.008726646f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlant01->AddTrack(aTrack, nullptr);
}

// 0x441ff0: the second plant, on the bottom edge at x 400: a sway of under a degree over about 300 frames
void MainMenu::BuildPlant02()
{
	mPlant02 = new ObjBase();
	mPlant02Anim->AddRef();
	mPlant02->mAnim = mPlant02Anim;
	mPlant02->SetParent(this, nullptr);
	mPlant02->SetState(STATE_OFFSET_Y, (float)(-(double)mPlant02Anim->GetSourceCelHeight() * 0.5));
	mPlant02->SetState(STATE_X, FromCorner(400.0f, 1920.0f));
	mPlant02->SetState(STATE_Y, FromCorner(1200.0f, 1200.0f));
	mPlant02->SetStateInt(STATE_Z, 0x13);
	mPlant02->Activate(mPlant02, false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(320);
	aFirstKey->AddValue(STATE_ROTATION, -0.0052359877f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(280);
	aSecondKey->AddValue(STATE_ROTATION, 0.008726646f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlant02->AddTrack(aTrack, nullptr);
}

// 0x4421e0: the third plant, the first of four on the bottom edge (x 1520): a sway of under a degree over about 290
// frames
void MainMenu::BuildPlant03First()
{
	mPlant03[0] = new ObjBase();
	mPlant03Anim->AddRef();
	mPlant03[0]->mAnim = mPlant03Anim;
	mPlant03[0]->SetParent(this, nullptr);
	mPlant03[0]->SetState(STATE_OFFSET_Y, (float)(-(double)mPlant03Anim->GetSourceCelHeight() * 0.5));
	mPlant03[0]->SetState(STATE_X, FromCorner(1520.0f, 1920.0f));
	mPlant03[0]->SetState(STATE_Y, FromCorner(1208.0f, 1200.0f));
	mPlant03[0]->SetStateInt(STATE_Z, 0x13);
	mPlant03[0]->Activate(mPlant03[0], false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(290);
	aFirstKey->AddValue(STATE_ROTATION, 0.013962635f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(230);
	aSecondKey->AddValue(STATE_ROTATION, -0.008726646f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlant03[0]->AddTrack(aTrack, nullptr);
}

// 0x4423d0: the third plant, the second of four (x 1400): a sway of under a degree over about 340 frames
void MainMenu::BuildPlant03Second()
{
	mPlant03[1] = new ObjBase();
	mPlant03Anim->AddRef();
	mPlant03[1]->mAnim = mPlant03Anim;
	mPlant03[1]->SetParent(this, nullptr);
	mPlant03[1]->SetState(STATE_OFFSET_Y, (float)(-(double)mPlant03Anim->GetSourceCelHeight() * 0.5));
	mPlant03[1]->SetState(STATE_X, FromCorner(1400.0f, 1920.0f));
	mPlant03[1]->SetState(STATE_Y, FromCorner(1200.0f, 1200.0f));
	mPlant03[1]->SetStateInt(STATE_Z, 0x13);
	mPlant03[1]->Activate(mPlant03[1], false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(350);
	aFirstKey->AddValue(STATE_ROTATION, -0.010471975f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(310);
	aSecondKey->AddValue(STATE_ROTATION, 0.009773844f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlant03[1]->AddTrack(aTrack, nullptr);
}

// 0x4425c0: the third plant, the third of four (x 64): a sway of under a degree over about 230 frames
void MainMenu::BuildPlant03Third()
{
	mPlant03[2] = new ObjBase();
	mPlant03Anim->AddRef();
	mPlant03[2]->mAnim = mPlant03Anim;
	mPlant03[2]->SetParent(this, nullptr);
	mPlant03[2]->SetState(STATE_OFFSET_Y, (float)(-(double)mPlant03Anim->GetSourceCelHeight() * 0.5));
	mPlant03[2]->SetState(STATE_X, FromCorner(64.0f, 1920.0f));
	mPlant03[2]->SetState(STATE_Y, FromCorner(1200.0f, 1200.0f));
	mPlant03[2]->SetStateInt(STATE_Z, 0x13);
	mPlant03[2]->Activate(mPlant03[2], false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(220);
	aFirstKey->AddValue(STATE_ROTATION, -0.012217305f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(230);
	aSecondKey->AddValue(STATE_ROTATION, 0.008726646f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlant03[2]->AddTrack(aTrack, nullptr);
}

// 0x4427b0: the third plant, the fourth of four (x 700): a sway of under a degree over about 240 frames
void MainMenu::BuildPlant03Fourth()
{
	mPlant03[3] = new ObjBase();
	mPlant03Anim->AddRef();
	mPlant03[3]->mAnim = mPlant03Anim;
	mPlant03[3]->SetParent(this, nullptr);
	mPlant03[3]->SetState(STATE_OFFSET_Y, (float)(-(double)mPlant03Anim->GetSourceCelHeight() * 0.5));
	mPlant03[3]->SetState(STATE_X, FromCorner(700.0f, 1920.0f));
	mPlant03[3]->SetState(STATE_Y, FromCorner(1250.0f, 1200.0f));
	mPlant03[3]->SetStateInt(STATE_Z, 0x13);
	mPlant03[3]->Activate(mPlant03[3], false);
	AnimKey* aFirstKey = new AnimKey();
	aFirstKey->SetDuration(240);
	aFirstKey->AddValue(STATE_ROTATION, -0.008726646f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->SetDuration(230);
	aSecondKey->AddValue(STATE_ROTATION, 0.010471975f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mPlant03[3]->AddTrack(aTrack, nullptr);
}

// 0x4429a0: the upper fog band, a picture 6 wide and 1 high at 0.3 alpha that drifts across 1200 frames
void MainMenu::BuildFogUpper()
{
	mFog[0] = new ObjBase();
	mFog[0]->SetParent(this, nullptr);
	mFog[0]->SetStateInt(STATE_X, 0);
	mFog[0]->SetStateInt(STATE_Y, 0x100);
	mFog[0]->SetStateInt(STATE_Z, 7);
	mFog[0]->SetStateInt(STATE_SCALE_X, 6);
	mFog[0]->SetStateInt(STATE_SCALE_Y, 1);
	mFog[0]->SetState(STATE_ALPHA, 0.3f);
	mFog[0]->Activate(mFog[0], false);
	AnimKey* aFirstKey = new AnimKey();
	mFogAnim->AddRef();
	aFirstKey->mAnim = mFogAnim;
	aFirstKey->SetDuration(1200);
	aFirstKey->AddValue(STATE_X, 64.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	mFogAnim->AddRef();
	aSecondKey->mAnim = mFogAnim;
	aSecondKey->SetDuration(1200);
	aSecondKey->AddValue(STATE_X, -64.0f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_X, 0.28f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_Y, 0.55f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mFog[0]->AddTrack(aTrack, nullptr);
}

// 0x442bf0: the lower fog band, wider and clearer than the upper one, drifting across 9000 frames
void MainMenu::BuildFogLower()
{
	mFog[1] = new ObjBase();
	mFog[1]->SetParent(this, nullptr);
	mFog[1]->SetStateInt(STATE_X, 0);
	mFog[1]->SetStateInt(STATE_Y, 0x1ea);
	mFog[1]->SetStateInt(STATE_Z, 8);
	mFog[1]->SetStateInt(STATE_SCALE_X, 0xa);
	mFog[1]->SetState(STATE_SCALE_Y, 0.8f);
	mFog[1]->SetState(STATE_ALPHA, 0.8f);
	mFog[1]->Activate(mFog[1], false);
	AnimKey* aFirstKey = new AnimKey();
	mFogAnim->AddRef();
	aFirstKey->mAnim = mFogAnim;
	aFirstKey->SetDuration(9000);
	aFirstKey->AddValue(STATE_X, 1537.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	mFogAnim->AddRef();
	aSecondKey->mAnim = mFogAnim;
	aSecondKey->SetDuration(9000);
	aSecondKey->AddValue(STATE_X, -1537.0f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_SCALE_X, 0.18f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mFog[1]->AddTrack(aTrack, nullptr);
}

// 0x442e00: a light ray to the right of the middle, added to the picture behind it (additive): widens and fades over
// 800 frames, looping
void MainMenu::BuildRayRight()
{
	mRays[0] = new ObjBase();
	mRays[0]->SetParent(this, nullptr);
	mRays[0]->SetStateInt(STATE_X, 0x208);
	mRays[0]->SetStateInt(STATE_Y, 0);
	mRays[0]->SetStateInt(STATE_Z, 7);
	mRays[0]->Activate(mRays[0], false);
	AnimKey* aFirstKey = new AnimKey();
	mRaysAnim->AddRef();
	aFirstKey->mAnim = mRaysAnim;
	aFirstKey->mDrawMode = 1;  // additive;
	aFirstKey->SetDuration(800);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, -0.25f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->mDrawMode = 1;  // additive;
	mRaysAnim->AddRef();
	aSecondKey->mAnim = mRaysAnim;
	aSecondKey->SetDuration(800);
	aSecondKey->AddValue(STATE_SCALE_X, 0.28f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mRays[0]->AddTrack(aTrack, nullptr);
}

// 0x442fe0: a light ray to the left of the middle: as the right one over 740 frames
void MainMenu::BuildRayLeft()
{
	mRays[1] = new ObjBase();
	mRays[1]->SetParent(this, nullptr);
	mRays[1]->SetStateInt(STATE_X, -490);
	mRays[1]->SetStateInt(STATE_Y, 0);
	mRays[1]->SetStateInt(STATE_Z, 7);
	mRays[1]->Activate(mRays[1], false);
	AnimKey* aFirstKey = new AnimKey();
	mRaysAnim->AddRef();
	aFirstKey->mAnim = mRaysAnim;
	aFirstKey->mDrawMode = 1;  // additive;
	aFirstKey->SetDuration(740);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, -0.25f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->mDrawMode = 1;  // additive;
	mRaysAnim->AddRef();
	aSecondKey->mAnim = mRaysAnim;
	aSecondKey->SetDuration(740);
	aSecondKey->AddValue(STATE_SCALE_X, 0.25f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mRays[1]->AddTrack(aTrack, nullptr);
}

// 0x4431c0: a light ray in the middle, behind the rest (depth 2), widening as it fades out over 680 frames
void MainMenu::BuildRayCentre()
{
	mRays[2] = new ObjBase();
	mRays[2]->SetParent(this, nullptr);
	mRays[2]->SetStateInt(STATE_X, 0);
	mRays[2]->SetStateInt(STATE_Y, 0);
	mRays[2]->SetStateInt(STATE_Z, 2);
	mRays[2]->Activate(mRays[2], false);
	AnimKey* aFirstKey = new AnimKey();
	mRaysAnim->AddRef();
	aFirstKey->mAnim = mRaysAnim;
	aFirstKey->mDrawMode = 1;  // additive;
	aFirstKey->SetDuration(680);
	aFirstKey->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aFirstKey->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aSecondKey = new AnimKey();
	aSecondKey->mDrawMode = 1;  // additive;
	mRaysAnim->AddRef();
	aSecondKey->mAnim = mRaysAnim;
	aSecondKey->SetDuration(680);
	aSecondKey->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aSecondKey->AddValue(STATE_ALPHA, -1.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mRays[2]->AddTrack(aTrack, nullptr);
}

// 0x443390: the left hanging vines (the second picture, behind the first set), swinging a degree to either side over
// 480 frames
void MainMenu::BuildVines02Left()
{
	mVines02[0] = new ObjBase();
	mVines02[0]->SetParent(this, nullptr);
	mVines02[0]->SetStateInt(STATE_OFFSET_Y, 0x200);
	mVines02[0]->SetState(STATE_X, FromCorner(190.0f, 1920.0f));
	mVines02[0]->SetState(STATE_Y, FromCorner(-400.0f, 1200.0f));
	mVines02[0]->SetState(STATE_Z, 5.9f);
	mVines02[0]->Activate(mVines02[0], false);
	AnimKey* aFirstKey = new AnimKey();
	mVines02Anim->AddRef();
	aFirstKey->mAnim = mVines02Anim;
	aFirstKey->SetDuration(480);
	aFirstKey->AddValue(STATE_ROTATION, 0.017453292f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	mVines02Anim->AddRef();
	aSecondKey->mAnim = mVines02Anim;
	aSecondKey->SetDuration(480);
	aSecondKey->AddValue(STATE_ROTATION, -0.017453292f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mVines02[0]->AddTrack(aTrack, nullptr);
}

// 0x443580: the right hanging vines of the second picture, swinging against the left ones
void MainMenu::BuildVines02Right()
{
	mVines02[1] = new ObjBase();
	mVines02[1]->SetParent(this, nullptr);
	mVines02[1]->SetStateInt(STATE_OFFSET_Y, 0x200);
	mVines02[1]->SetState(STATE_X, FromCorner(1790.0f, 1920.0f));
	mVines02[1]->SetState(STATE_Y, FromCorner(-460.0f, 1200.0f));
	mVines02[1]->SetState(STATE_Z, 5.9f);
	mVines02[1]->Activate(mVines02[1], false);
	AnimKey* aFirstKey = new AnimKey();
	mVines02Anim->AddRef();
	aFirstKey->mAnim = mVines02Anim;
	aFirstKey->SetDuration(480);
	aFirstKey->AddValue(STATE_ROTATION, -0.017453292f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	mVines02Anim->AddRef();
	aSecondKey->mAnim = mVines02Anim;
	aSecondKey->SetDuration(480);
	aSecondKey->AddValue(STATE_ROTATION, 0.017453292f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mVines02[1]->AddTrack(aTrack, nullptr);
}

// 0x443770: the first set of vines, on the left, hung from their top: a swing of 4 degrees
void MainMenu::BuildVines01Left()
{
	mVines01[0] = new ObjBase();
	mVines01[0]->SetParent(this, nullptr);
	mVines01[0]->SetState(STATE_OFFSET_Y, mVines01Anim->GetSourceCelHeight());
	mVines01[0]->SetState(STATE_X, FromCorner(150.0f, 1920.0f));
	mVines01[0]->SetState(STATE_Y, FromCorner(-310.0f, 1200.0f));
	mVines01[0]->SetStateInt(STATE_Z, 6);
	mVines01[0]->Activate(mVines01[0], false);
	AnimKey* aFirstKey = new AnimKey();
	mVines01Anim->AddRef();
	aFirstKey->mAnim = mVines01Anim;
	aFirstKey->SetDuration(280);
	aFirstKey->AddValue(STATE_ROTATION, 0.06981317f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	mVines01Anim->AddRef();
	aSecondKey->mAnim = mVines01Anim;
	aSecondKey->SetDuration(280);
	aSecondKey->AddValue(STATE_ROTATION, -0.06981317f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mVines01[0]->AddTrack(aTrack, nullptr);
}

// 0x443960: the first set's middle vines, hung from their top at x 400: a swing of 6 degrees
void MainMenu::BuildVines01Middle()
{
	mVines01[1] = new ObjBase();
	mVines01[1]->SetParent(this, nullptr);
	mVines01[1]->SetState(STATE_OFFSET_Y, (float)((double)mVines01Anim->GetSourceCelHeight() * 0.5));
	mVines01[1]->SetState(STATE_X, FromCorner(400.0f, 1920.0f));
	mVines01[1]->SetState(STATE_Y, FromCorner(-138.0f, 1200.0f));
	mVines01[1]->SetStateInt(STATE_Z, 6);
	mVines01[1]->Activate(mVines01[1], false);
	AnimKey* aFirstKey = new AnimKey();
	mVines01Anim->AddRef();
	aFirstKey->mAnim = mVines01Anim;
	aFirstKey->SetDuration(300);
	aFirstKey->AddValue(STATE_ROTATION, -0.10471976f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	mVines01Anim->AddRef();
	aSecondKey->mAnim = mVines01Anim;
	aSecondKey->SetDuration(300);
	aSecondKey->AddValue(STATE_ROTATION, 0.10471976f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mVines01[1]->AddTrack(aTrack, nullptr);
}

// 0x443b60: the first set's right vines, at x 1650: a swing of 7 degrees. The original stores the object in the
// middle vines' field (it writes mVines01[1] again, not mVines01[2]), so the middle vines are still in the scene but
// nothing points to them.
void MainMenu::BuildVines01Right()
{
	mVines01[1] = new ObjBase();
	mVines01[1]->SetParent(this, nullptr);
	mVines01[1]->SetState(STATE_OFFSET_Y, (float)((double)mVines01Anim->GetSourceCelHeight() * 0.5));
	mVines01[1]->SetState(STATE_X, FromCorner(1650.0f, 1920.0f));
	mVines01[1]->SetState(STATE_Y, FromCorner(-168.0f, 1200.0f));
	mVines01[1]->SetStateInt(STATE_Z, 6);
	mVines01[1]->Activate(mVines01[1], false);
	AnimKey* aFirstKey = new AnimKey();
	mVines01Anim->AddRef();
	aFirstKey->mAnim = mVines01Anim;
	aFirstKey->SetDuration(350);
	aFirstKey->AddValue(STATE_ROTATION, -0.122173056f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	mVines01Anim->AddRef();
	aSecondKey->mAnim = mVines01Anim;
	aSecondKey->SetDuration(340);
	aSecondKey->AddValue(STATE_ROTATION, 0.122173056f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, aFirstKey);
	mVines01[1]->AddTrack(aTrack, nullptr);
}

// 0x443d60: the logo: it swings in from 170 degrees (the object is set to that, drawn from its top) to 0, with a
// delay of 9 frames, then rocks gently to either side, looping. The original takes the logo's offset from the first
// vines' picture, not its own.
void MainMenu::BuildLogo()
{
	mLogo = new ObjBase();
	mLogo->SetParent(this, nullptr);
	mLogo->SetState(STATE_OFFSET_Y, (float)((double)mVines01Anim->GetSourceCelHeight() * 0.5));
	mLogo->SetState(STATE_X, FromCorner(1024.0f, 1920.0f));
	mLogo->SetState(STATE_Y, FromCorner(-230.0f, 1200.0f));
	mLogo->SetStateInt(STATE_Z, 0x14);
	mLogo->mState[STATE_ROTATION] = 2.9670596f;
	mLogo->ComputeDrawn();
	AnimKey* aFirstKey = new AnimKey();
	mLogoAnim->AddRef();
	aFirstKey->mAnim = mLogoAnim;
	aFirstKey->SetDuration(300);
	aFirstKey->AddValue(STATE_ROTATION, 2.792527f, MOD_TOWARD_NEXT);
	aFirstKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aSecondKey = new AnimKey();
	mLogoAnim->AddRef();
	aSecondKey->mAnim = mLogoAnim;
	aSecondKey->SetDuration(400);
	aSecondKey->AddValue(STATE_ROTATION, -0.2617994f, MOD_TOWARD_NEXT);
	aSecondKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aThirdKey = new AnimKey();
	mLogoAnim->AddRef();
	aThirdKey->mAnim = mLogoAnim;
	aThirdKey->SetDuration(700);
	aThirdKey->AddValue(STATE_ROTATION, 0.05235988f, MOD_TOWARD_NEXT);
	aThirdKey->mEasing = EASE_COSINE_TOO;
	AnimKey* aFourthKey = new AnimKey();
	mLogoAnim->AddRef();
	aFourthKey->mAnim = mLogoAnim;
	aFourthKey->SetDuration(700);
	aFourthKey->AddValue(STATE_ROTATION, -0.05235988f, MOD_TOWARD_NEXT);
	aFourthKey->mEasing = EASE_COSINE_TOO;
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFirstKey, nullptr);
	aTrack->AddKey(aSecondKey, nullptr);
	aTrack->AddKey(aThirdKey, nullptr);
	aTrack->AddKey(aFourthKey, aThirdKey);
	aTrack->SetDelay(9, false);
	mLogo->AddTrack(aTrack, nullptr);
	Activate(mLogo, false);
}
