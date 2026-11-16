// ThunderballAppScreens.cpp: the part of ThunderballApp that puts the game's screens up and takes them down.

#include "LoadingScreen.h"
#include "MainMenu.h"
#include "MusicInterface.h"
#include "NotTranslated.h"
#include "SoundManager.h"
#include "ThunderDialog.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

using namespace Sexy;

static const int OPTIONS_DIALOG_SLIDE_SPEED_X = 40; // how fast the options dialog goes off to the side when the menu comes in

// the music of the levels: a tracker module the game ships (the original loads every music file but the two of the menus)
static const char* const LEVEL_MUSIC_FILE = "music/pegglebeats.mo3";

// 0x4a6f90 (the thread procedure of the loading screen) and 0x405740: loads the level music into slot 1 on. The original starts
// it in a thread of its own and the loading screen waits for it; it is done at once here.
void ThunderballApp::LoadLevelMusic()
{
	mLevelMusicPending = false;
	mNextLevelMusicId = 1;
	mLevelMusicParam = 9;
	mMusicInterface->LoadMusic(mNextLevelMusicId, LEVEL_MUSIC_FILE);
	mNextLevelMusicId++;
}

// 0x4296c0
void ThunderballApp::ShowLoadingScreen()
{
	CleanupScreens(true);
	mLoadingScreen = new LoadingScreen(this);
	mLoadingScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mLoadingScreen);
	mWidgetManager->SetFocus(mLoadingScreen);
}

// 0x4234f0
void ThunderballApp::CleanupScreen(Widget* theScreen)
{
	if (theScreen != nullptr)
	{
		if (!mWidgetMover.WillRemove(theScreen))
		{
			mWidgetMover.Forget(theScreen);
			mWidgetManager->RemoveWidget(theScreen);
			SafeDeleteWidget(theScreen);
		}
	}
}

// 0x429390
void ThunderballApp::CleanupScreens(bool theIncludeBoard)
{
	if (theIncludeBoard)
	{
		if (mBoard != nullptr)
			NOT_TRANSLATED("CleanupScreens with a board up: Board::NeedSaveGame, SaveGame, NotifyRemoving");
		CleanupScreen(mBoard);
		mBoard = nullptr;
		// (the upsell screen's slot, +0x7bc, is not in this port)
	}
	CleanupScreen(mHelpScreen);
	mHelpScreen = nullptr;
	CleanupScreen(mLevelScreen);
	mLevelScreen = nullptr;
	CleanupScreen(mLoadingScreen);
	mLoadingScreen = nullptr;
	CleanupScreen(mMainMenu);
	mMainMenu = nullptr;
	CleanupScreen(mTrophyScreen);
	mTrophyScreen = nullptr;
	CleanupScreen(mStoryScreen);
	mStoryScreen = nullptr;
	CleanupScreen(mAdventureScreen);
	mAdventureScreen = nullptr;
	KillDialog(1);
	KillDialog(0x23);
}

// 0x423480
void ThunderballApp::CheckScrollOff(Widget* theScreen, WidgetScroll theTo, bool theRemoveWhenDone)
{
	if (theScreen != nullptr)
	{
		mWidgetMover.ScrollOff(theScreen, theTo, theRemoveWhenDone);
		if (theTo != SCROLL_NONE)
			mScrollOnFrom = theTo;
	}
}

// 0x4234c0
void ThunderballApp::ScrollOn(Widget* theScreen)
{
	if (mScrollOnFrom != SCROLL_NONE)
	{
		mWidgetMover.ScrollOn(theScreen, mScrollOnFrom);
		ClearUpdateBacklog(false);
	}
}

// 0x42d480
void ThunderballApp::ShowMainMenu()
{
	mSoundManager->StopAllSounds();
	CheckPlayMusic(false);
	ResetTwoPlayerStats();

	if (mMainMenu != nullptr)
	{
		// the menu is up already: every other screen goes
		MainMenu* aMenu = mMainMenu;
		mMainMenu = nullptr;
		CleanupScreens(true);
		mMainMenu = aMenu;
		mWidgetManager->SetFocus(aMenu);
		return;
	}

	// the screen that is up leaves, and the menu comes in from the side it left to
	mScrollOnFrom = SCROLL_DOWN;
	CheckScrollOff(mLoadingScreen, SCROLL_LEFT, true);
	CheckScrollOff(mLevelScreen, SCROLL_RIGHT, true);
	CheckScrollOff(mTrophyScreen, SCROLL_RIGHT, true);
	CheckScrollOff(mStoryScreen, SCROLL_RIGHT, true);
	CheckScrollOff(mAdventureScreen, SCROLL_RIGHT, true);
	CheckScrollOff(mBoard, SCROLL_RIGHT, true);

	// an options dialog that is up goes along to the side
	ThunderDialog* anOptionsDialog = static_cast<ThunderDialog*>(GetDialog(DIALOG_OPTIONS));
	if (anOptionsDialog != nullptr)
		anOptionsDialog->mScrollSpeedX = OPTIONS_DIALOG_SLIDE_SPEED_X;

	CleanupScreens(true);
	FinishOptionsDialog(true, true);

	mMainMenu = new MainMenu(this);
	mMainMenu->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mMainMenu);
	mWidgetManager->SetFocus(mMainMenu);
	ScrollOn(mMainMenu);
	BringDialogsToFront();
	if (mCurrentPlayer == nullptr)
		DoCreateUserDialog();
}

// The screens that are not translated yet
void ThunderballApp::ShowStoryScreen(bool theFromMenu, bool theScrollOn)
{
	NOT_TRANSLATED("ThunderballApp::ShowStoryScreen");
}

void ThunderballApp::ShowAdventureScreen()
{
	NOT_TRANSLATED("ThunderballApp::ShowAdventureScreen");
}

void ThunderballApp::ShowLevelScreen(bool theScrollOn)
{
	NOT_TRANSLATED("ThunderballApp::ShowLevelScreen");
}

void ThunderballApp::ShowTrophyScreen()
{
	NOT_TRANSLATED("ThunderballApp::ShowTrophyScreen");
}

void ThunderballApp::StartAdventureGame()
{
	NOT_TRANSLATED("ThunderballApp::StartAdventureGame");
}

void ThunderballApp::ViewReplays()
{
	NOT_TRANSLATED("ThunderballApp::ViewReplays");
}

bool ThunderballApp::CheckSaveGame(bool theAskToContinue)
{
	NOT_TRANSLATED("ThunderballApp::CheckSaveGame");
}

void ThunderballApp::ShowHelpScreen(bool theFromMenu, bool theScrollOn)
{
	NOT_TRANSLATED("ThunderballApp::ShowHelpScreen");
}

void ThunderballApp::ShowBoard(bool theNewGame, bool theScrollOn)
{
	NOT_TRANSLATED("ThunderballApp::ShowBoard");
}

void ThunderballApp::ShowReplay(bool theKeepBoard)
{
	NOT_TRANSLATED("ThunderballApp::ShowReplay");
}
