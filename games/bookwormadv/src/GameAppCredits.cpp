// GameApp's side of the credits (Credits): the options' button that starts them, the screen made over the others,
// and the way out of it. The end of the game starts them from the hall of fame (GameAppHallOfFame.cpp).
#include "Board.h"
#include "BookManager.h"
#include "ConversationPanel.h"
#include "Credits.h"
#include "CurtainTransition.h"
#include "GameApp.h"
#include "MainMenu.h"
#include "SongPlayer.h"
#include "TooltipWidget.h"
#include "TreasureScreen.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace {

// what ShowCredits stops in the screens under the credits, and CloseCredits lets go on again (the update, the
// drawing and the mouse)
const int kCoveredFlags = WIDGETFLAGS_UPDATE | WIDGETFLAGS_DRAW | WIDGETFLAGS_ALLOW_MOUSE;

void ShowCreditsCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->ShowCredits();
}

}  // namespace

void GameApp::StartCredits()
{
	// 0x450fe0: the curtain closes over everything, then the credits come; the main menu is not the title screen's
	DoFullCurtainTransition("");
	mCurtainCallback = ShowCreditsCallback;
	mCurtainCallbackObject = this;
	if (mMainMenu != nullptr)
		mMainMenu->mFromTitle = false;
}

void GameApp::ShowCredits()
{
	// 0x450d80: the options go (and the battle they paused goes on), the screens under the credits stop
	if (GetDialog(DIALOG_OPTIONS) != nullptr) {
		KillDialog(DIALOG_OPTIONS);
		if (mBoard != nullptr)
			mBoard->Pause(false, true);
	}
	mCredits = new Credits();
	mWidgetManager->AddWidget(mCredits);
	mCreditsCount++;
	if (mBoard != nullptr)
		mBoard->mWidgetFlagsMod.mRemoveFlags |= kCoveredFlags;
	if (mBookManager != nullptr)
		mBookManager->mWidgetFlagsMod.mRemoveFlags |= kCoveredFlags;
	if (mTreasureScreen != nullptr)
		mTreasureScreen->mWidgetFlagsMod.mRemoveFlags |= kCoveredFlags;
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
}

void GameApp::CloseCredits()
{
	// 0x450e90: the credits go; the main menu comes back after the end of the game
	mCreditsCount--;
	SafeDeleteWidget(mCredits);
	mWidgetManager->RemoveWidget(mCredits);
	if (mCredits->mEndOfGame)
		ShowMainMenu(false, false);
	mCredits = nullptr;
	if (mBoard != nullptr)
		mBoard->mWidgetFlagsMod.mRemoveFlags &= ~kCoveredFlags;
	if (mBookManager != nullptr)
		mBookManager->mWidgetFlagsMod.mRemoveFlags &= ~kCoveredFlags;
	if (mTreasureScreen != nullptr)
		mTreasureScreen->mWidgetFlagsMod.mRemoveFlags &= ~kCoveredFlags;
	if (mNoMusic && mConversationPanel != nullptr) {
		// the end of the game (DoEndGame) cut the music: the conversation panel is shown again over the main menu,
		// which has its song
		mNoMusic = false;
		mConversationPanel->Activate(false);
		mWidgetManager->PutInfront(mConversationPanel, mMainMenu);
		if (mSongPlayer != nullptr)
			mSongPlayer->PlaySong(0x8c, 0.005f, true);
		if (mCurtain != nullptr)
			mWidgetManager->BringToFront(mCurtain);
	} else if (mSongPlayer != nullptr) {
		// the song from before the credits
		mSongPlayer->PlaySong(mSongPlayer->mLastOrder, 0.005f, true);
	}
	if (mHoverTooltip != nullptr)
		mHoverTooltip->MouseLeave();
}

void GameApp::CloseCreditsCallback(void* theApp)
{
	static_cast<GameApp*>(theApp)->CloseCredits();
}
