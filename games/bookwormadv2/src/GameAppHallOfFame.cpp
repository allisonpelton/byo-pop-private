// GameApp's openers of the hall of fame (HallOfFame), the way out of it, and the credits that follow the end of the
// game.
#include <SDL.h>

#include "Board.h"
#include "BookManager.h"
#include "Credits.h"
#include "CurtainTransition.h"
#include "GameApp.h"
#include "HallOfFame.h"
#include "LuaLinkedClass.h"
#include "PopLua.h"
#include "TreasureScreen.h"
#include "WidgetManager.h"
#include "lua.h"

using namespace Sexy;

void GameApp::ShowHallOfFame()
{
	// 0x450710
	mWidgetManager->AddWidget(mHallOfFame);
	mHallOfFame->Resize(0, 0, mWidth, mHeight);
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
}

void GameApp::OpenHallOfFame()
{
	// 0x450760: the adventure's tab
	mHallOfFame = new HallOfFame(this, HallOfFame::MODE_ADVENTURE, false, false, std::string(), false);
	ShowHallOfFame();
	SetCursor(CURSOR_POINTER);
}

void GameApp::ShowArenaHallOfFame()
{
	// 0x450850: the way back to the main menu from a battle (as CurtainDone's), then the arena's tab
	KillDialog(1);
	ReturnToChapterSelect(false, true, true, true);
	mWidgetManager->RemoveWidget(mBookManager);
	if (mTreasureScreen != nullptr)
		mWidgetManager->RemoveWidget(mTreasureScreen);
	SafeDeleteWidget(mTreasureScreen);
	if (mBookScreen != nullptr)
		RemoveStatsBook();
	RemoveBookManager();
	mTreasureScreen = nullptr;
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
	if (mTimedMode) {
		mTimedMode = false;
		lua_pushboolean(mLua->L, 0);
		mLua->Call("main.SetTimedMode", 1, 0);
	}
	ClearUpdateBacklog(true);
	HallOfFame* aHall = new HallOfFame(this, HallOfFame::MODE_ARENA, true, true, std::string(), false);
	mHallOfFame = aHall;
	aHall->mFromArena = true;
	ShowHallOfFame();
	SetCursor(CURSOR_POINTER);
}

void GameApp::DoHighScoreScreenFromMG(std::string theName)
{
	// 0x450aa0: the mini-games' first page; the board of the mini-game that was just played goes (it is kept until
	// the app is done with it)
	mHallOfFame = new HallOfFame(this, HallOfFame::MODE_MINI_GAMES, true, false, theName, false);
	ShowHallOfFame();
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard->SetDisabled(true);
		mBoard->SetVisible(false);
		mBoard = nullptr;
		Unmute(true);
	}
	SetCursor(CURSOR_POINTER);
}

// 0x446860: state 12's closed-curtain case at once (the book's removals, the Hall of Fame) but the replay flag stays as it
// is, and the curtain opens over it
void GameApp::DoHighScoreScreenFromReplay()
{
	KillDialog(DIALOG_OPTIONS);
	mWidgetManager->RemoveWidget(mBookManager);
	if (mTreasureScreen != nullptr)
		mWidgetManager->RemoveWidget(mTreasureScreen);
	SafeDeleteWidget(mTreasureScreen);
	if (mBookScreen != nullptr)
		RemoveStatsBook();
	RemoveBookManager();
	mTreasureScreen = nullptr;
	RemoveBoard();
	if (mTimedMode) {
		mTimedMode = false;
		lua_pushboolean(mLua->L, 0);
		mLua->Call("main.SetTimedMode", 1, 0);
	}
	ClearUpdateBacklog(false);
	HallOfFame* aHall = new HallOfFame(this, HallOfFame::MODE_ADVENTURE, true, false, std::string(), false);
	mHallOfFame = aHall;
	aHall->mFromArena = true;
	ShowHallOfFame();
	SetCursor(CURSOR_POINTER);
	DoCurtainTransition(false, "", 999999);
	mWidgetManager->BringToBack(mHallOfFame);
	mWidgetManager->BringToFront(mCurtain);
}

void GameApp::ShowEndOfGameHallOfFame()
{
	// 0x450c30: "Continue" in place of the main menu's button
	mHallOfFame = new HallOfFame(this, HallOfFame::MODE_ADVENTURE, false, false, std::string(), true);
	ShowHallOfFame();
}

static void EndOfGameHallOfFameCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->ShowEndOfGameHallOfFame();
}

void GameApp::DoEndGame()
{
	// 0x450c00: the end of the adventure: no music, and the hall of fame once the curtain is closed
	mNoMusic = true;
	StartToMenuTransition();
	mCurtainCallback = EndOfGameHallOfFameCallback;
	mCurtainCallbackObject = this;
}

void GameApp::CloseHallOfFame()
{
	// 0x450bb0
	SafeDeleteWidget(mHallOfFame);
	mWidgetManager->RemoveWidget(mHallOfFame);
	mHallOfFame = nullptr;
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
}

static void EndOfGameCreditsCallback(void* theApp)
{
	// 0x451130: the credits come (0x450d80), with the finale of the end of the game (their +0x157, which also makes
	// them end on the main menu), and the hall of fame goes
	GameApp* anApp = static_cast<GameApp*>(theApp);
	anApp->ShowCredits();
	anApp->mCredits->mEndOfGame = true;
	if (anApp->mHallOfFame != nullptr)
		anApp->CloseHallOfFame();
}

void GameApp::StartEndOfGameCredits()
{
	// 0x451090
	DoFullCurtainTransition("");
	mCurtainCallback = EndOfGameCreditsCallback;
	mCurtainCallbackObject = this;
}
