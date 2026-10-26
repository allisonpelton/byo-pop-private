// GameApp's stats book: the tome of knowledge, the media player, and the page of statistics that closes a timed
// game (StatsBook), and the way out of it.
#include "Board.h"
#include "CurtainTransition.h"
#include "GameApp.h"
#include "GameText.h"
#include "LuaLinkedClass.h"
#include "MainMenu.h"
#include "StatsBook.h"
#include "WidgetManager.h"

using namespace Sexy;

static void ShowTomeCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->ShowTome();
}

static void ShowMediaPlayerCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->ShowMediaPlayer();
}

void GameApp::AddStatsBook(bool theFromGame, bool theBestiary, bool theMediaPlayer)
{
	// 0x444150
	if (theFromGame || (!theBestiary && !theMediaPlayer)) {
		// at once (port: the original's third flag is always off here)
		StatsBook* aBook = new StatsBook(this, theFromGame, theBestiary, false);
		mBookScreen = aBook;
		aBook->CreateLinkedObject(aBook);
		mWidgetManager->AddWidget(aBook);
		Unmute(true);
		return;
	}
	// the tome or the media player: the whole curtain closes over the main menu first (the tome tells what is
	// coming in the box the curtain shows)
	DoFullCurtainTransition("");
	mCurtainCallback = theMediaPlayer ? ShowMediaPlayerCallback : ShowTomeCallback;
	mCurtainCallbackObject = this;
	mMainMenu->mFromTitle = false;
	if (!theMediaPlayer) {
		mCurtain->mShowLoadingBox = true;
		mCurtain->mLoadingText = GT(0x6fa520);
		mCurtain->mLoadingBoxShown = false;
	}
}

void GameApp::ShowTome()
{
	// 0x444310
	StatsBook* aBook = new StatsBook(this, false, true, false);
	mBookScreen = aBook;
	aBook->CreateLinkedObject(aBook);
	mWidgetManager->AddWidget(aBook);
	mWidgetManager->BringToFront(mCurtain);
}

void GameApp::ShowMediaPlayer()
{
	// 0x4443c0
	StatsBook* aBook = new StatsBook(this, false, false, true);
	mBookScreen = aBook;
	aBook->CreateLinkedObject(aBook);
	mWidgetManager->AddWidget(aBook);
	mWidgetManager->BringToFront(mCurtain);
}

static void RemoveStatsBookCallback(void* theApp)
{
	// 0x444570: the curtain is closed
	GameApp* anApp = static_cast<GameApp*>(theApp);
	anApp->mWidgetManager->RemoveWidget(anApp->mBookScreen);
	anApp->SafeDeleteWidget(anApp->mBookScreen);
	anApp->mBookScreen = nullptr;
}

void GameApp::RemoveStatsBook()
{
	// 0x444470
	StatsBook* aBook = static_cast<StatsBook*>(mBookScreen);
	if (!aBook->mFromGame && (aBook->mBestiary || aBook->mMediaPlayer)) {
		DoFullCurtainTransition("");
		mCurtainCallback = RemoveStatsBookCallback;
		mCurtainCallbackObject = this;
		return;
	}
	RemoveStatsBookCallback(this);
}
