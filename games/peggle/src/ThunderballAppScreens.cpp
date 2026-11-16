// ThunderballAppScreens.cpp: the part of ThunderballApp that puts the game's screens up and takes them down.

#include "LoadingScreen.h"
#include "MusicInterface.h"
#include "NotTranslated.h"
#include "SoundManager.h"
#include "ThunderballApp.h"
#include "WidgetManager.h"

using namespace Sexy;

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

// The screens that are not translated yet
void ThunderballApp::ShowMainMenu()
{
	NOT_TRANSLATED("ThunderballApp::ShowMainMenu");
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

void ThunderballApp::DoCreateUserDialog()
{
	NOT_TRANSLATED("ThunderballApp::DoCreateUserDialog");
}
