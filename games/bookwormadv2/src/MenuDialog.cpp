#include "MenuDialog.h"

#include "ArrowHint.h"
#include "BattleEngine.h"
#include "Board.h"
#include "FadeButton.h"
#include "Font.h"
#include "GameApp.h"
#include "GameText.h"
#include "Image.h"
#include "Res.h"
#include "SoundPlayer.h"

using namespace Sexy;

// 0x47fd00: dialog 0x11 under the menu's banner, its buttons stacked from 80 down, 90 across and 250 wide, the
// dialog 420 wide and as high as they make it, in the middle of the screen
MenuDialog::MenuDialog(bool theHasBoard, bool theFromTreasure, bool theMiniGameFromMenu)
	: GameDialog(GameApp::DIALOG_MENU, _S(""), _S(""), _S(""), BUTTONS_NONE), mHasBoard(theHasBoard),
	  mFromTreasure(theFromTreasure)
{
	SexyString aLabels[NUM_BUTTONS];
	aLabels[BUTTON_OPTIONS] = GT(0x6fd484);
	if (theHasBoard)
		aLabels[BUTTON_MAIN_MENU] = theMiniGameFromMenu ? GT(0x6fd454) : GT(0x6fd464);
	else
		aLabels[BUTTON_MAIN_MENU] = GT(0x6fd478);
	aLabels[BUTTON_ABANDON] = GT(0x6fd48c);
	aLabels[BUTTON_RETURN] = theHasBoard ? GT(0x6fd49c) : GT(0x6fd4ac);

	Board* aBoard = mApp->mBoard;
	int aHeight = 140;
	int aButtonY = 80;
	for (int i = 0; i < NUM_BUTTONS; i++) {
		// "Abandon Game" is for an adventure's mini-game
		if ((aBoard == nullptr || !aBoard->mIsMiniGame || mApp->mMiniGameFromMenu) && i == BUTTON_ABANDON) {
			mButtons[i] = nullptr;
			continue;
		}
		FadeButton* aButton = MakeButton(i, this, aLabels[i]);
		aButton->mFadeOnDisable = false;
		aButton->mFadeOnLeave = false;
		aButton->mFadeOnDown = false;
		aButton->mFadeOnUp = false;
		aButton->mFadeOnEnter = false;
		mButtons[i] = aButton;
		AddWidget(aButton);
		aButton->Resize(90, aButtonY, 250, IMAGE_DIALOG_BUTTON->mHeight);
		aHeight += aButton->mHeight;
		aButtonY += IMAGE_DIALOG_BUTTON->mHeight;
	}
	SetHeaderFont(FONT_LARGE_OUTLINE);
	SetButtonFont(FONT_LARGE);
	SetLinesFont(FONT_MEDIUM);
	SetColor(COLOR_HEADER, Color::Black);
	SetColor(COLOR_LINES, Color::Black);
	mBanner = IMAGE_DIALOG_BANNER_MENU;
	mBannerY = 17;
	GameDialog::Resize(mApp->mWidth / 2 - 210, mApp->mHeight / 2 - aHeight / 2, 420, aHeight);
}

// 0x480180: the buttons are the dialog's widgets, and go with it
MenuDialog::~MenuDialog()
{
	RemoveAllWidgets(true, true);
}

void MenuDialog::ButtonPress(int theId)
{
	// 0x480210
	mApp->PlaySample(SOUND_CLICK1);
}

void MenuDialog::ButtonDepress(int theId)
{
	// 0x480230. Any button but "Main Menu" over a battle ends the menu (so does Escape, as the way back), and the
	// battle goes on; "Main Menu" over a battle asks first, with the menu staying under the question
	if (!mHasBoard || theId != BUTTON_MAIN_MENU) {
		if (theId != BUTTON_OPTIONS)
			mApp->mSoundPlayer->PauseLooping(false);
		mApp->KillDialog(mId);
		if (mApp->mBoard != nullptr)
			mApp->mBoard->Pause(false, true);
	}
	if (theId == BUTTON_OPTIONS) {
		mApp->mSoundPlayer->PauseLooping(false);
		mApp->ShowOptionsDialog();
	} else if (theId == BUTTON_MAIN_MENU) {
		if (!mHasBoard) {
			if (mFromTreasure)
				mApp->StartTreasureToMenuTransition();
			else
				mApp->StartToMenuTransition();
			return;
		}
		// the question (its yes comes back here as ANSWER_QUIT_YES)
		if (mApp->mMiniGameFromMenu)
			mApp->DoConfirmDialog(-1, GT(0x6fd520), GT(0x6fd4b8), this, "");
		else
			mApp->DoConfirmDialog(-1, GT(0x6fd520), GT(0x6fd548), this, "gBookManager:AboutToReturnToMM");
	} else if (theId == BUTTON_ABANDON) {
		mApp->DoConfirmDialog(-1, GT(0x6fd638), GT(0x6fd5b0), nullptr, "gMiniGame:AbandonMiniGame");
	} else if (theId == ANSWER_QUIT_YES) {
		if (!mApp->mMiniGameFromMenu) {
			if (gBattleEngine != nullptr)
				LuaLib::CallLua("gBattleEngine:QuitGame", 0);
			mApp->StartGameToMenuTransition();
		} else {
			// a mini-game of the main menu's list: the curtain closes, then the scripts clean up after it
			if (mApp->mArrowHintManager != nullptr)
				mApp->mArrowHintManager->DeleteAllArrowHints();
			mApp->DoCurtainTransition(true, "gMiniGame:Cleanup", 999999);
		}
	}
}
