// GameApp's mini-game methods: a mini-game's board, the floating texts of its points, and the way back to the
// main menu. The mini-games themselves are scripts (scripts/minigames/: gMiniGame).
#include "GameApp.h"

#include "BonusText.h"
#include "Board.h"
#include "CurtainTransition.h"
#include "LuaLinkedClass.h"
#include "MinigameMenu.h"
#include "PopAnimManager.h"
#include "Profile.h"
#include "SongPlayer.h"
#include "WidgetManager.h"

using namespace Sexy;

void GameApp::StartMiniGame(std::string theName, int x, int y, int theXGap, int theYGap, int theCols, int theRows,
                            std::string theLetters, bool theNoTilesWhilePaused)
{
	// 0x44b780: a mini-game's board, over everything but the curtain; theName is the mini-game's class
	mMiniGameName = theName;  // port: for the test command state
	mBoard = new Board(this, true);
	mBoard->CreateLinkedObject(mBoard);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	if (mCurtain != nullptr) {
		mWidgetManager->BringToFront(mCurtain);
		mCurtain->MarkDirtyFull();
	}
	mBoard->InitMiniGame(theName, x, y, theXGap, theYGap, theCols, theRows, theLetters, theNoTilesWhilePaused);
	mWidgetManager->SetFocus(mBoard);
	// port: the original then freed the images' extra buffers (0x5492a0, the resource manager's 0x575cd0), as
	// StartBattle did; the port's images keep none
	if (!mNoMusic && mSongPlayer != nullptr)
		mSongPlayer->PlaySong(0x96, 0.005f, true);  // the mini-games' song
}

void GameApp::ReturnToMMFromMiniGame()
{
	// 0x44b9d0: the mini-game's board and resources freed, and the main menu
	mProfile->Save();
	LuaLib::CallLua("PAMAnimator.CleanUp", 0);
	FreeChapterResources();
	mPopAnimManager->DeleteClones();
	CleanSharedImages();
	RemoveAllTooltips();
	RemoveBoard();
	Unmute(true);
	gBoard = nullptr;
	ShowMainMenu(false, false);
	mMiniGameFromMenu = false;
}

static void MiniGameDoneCallback(void* theApp)
{
	// 0x44d290: once the curtain is closed
	((GameApp*)theApp)->ReturnToMMFromMiniGame();
	LuaLib::CallLua("gMiniGame:Cleanup", 0);
}

void GameApp::UnlockedMiniGameComplete()
{
	// 0x44d1e0: the whole curtain closes over the mini-game, then the main menu
	DoFullCurtainTransition("");
	mCurtainCallback = MiniGameDoneCallback;
	mCurtainCallbackObject = this;
	mCurtain->mPriority = 999999;
}

void GameApp::ShowMiniGameMenu()
{
	// 0x44f9d0: the mini-games' menu, over the screens but under the curtain. Its buttons (MinigameMenu) close the
	// whole curtain again and then start the mini-game picked, or bring the main menu back.
	mMiniGameMenu = new MinigameMenu(this);
	mWidgetManager->AddWidget(mMiniGameMenu);
	if (mCurtain != nullptr)
		mWidgetManager->BringToFront(mCurtain);
}

void GameApp::AddBonusText(const std::string& theText, float x, float y, float theSpeed, int theFadeDelay,
                           Font* theFont)
{
	// 0x44fab0
	mBonusTexts.push_back(new JeffLib::BonusText(theText, theFont, x, y, theSpeed, theFadeDelay));
}

void GameApp::UpdateBonusText()
{
	// 0x44fbd0: the dead ones stay until RemoveAllBonusText
	for (size_t i = 0; i < mBonusTexts.size(); i++)
		mBonusTexts[i]->Update();
}

void GameApp::DrawBonusText(Graphics* g)
{
	// 0x44fb70
	for (size_t i = 0; i < mBonusTexts.size(); i++)
		mBonusTexts[i]->Draw(g);
}

void GameApp::RemoveAllBonusText()
{
	// 0x44fd00
	for (size_t i = 0; i < mBonusTexts.size(); i++)
		delete mBonusTexts[i];
	mBonusTexts.clear();
}
