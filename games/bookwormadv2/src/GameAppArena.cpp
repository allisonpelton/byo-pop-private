// GameApp's arena methods. The arena is the timed mode ("TM" in the scripts' names and the profile's keys,
// GameApp::mTimedMode); its scripts (scripts/arenas, TimedMode) play it. Between its stages the treasure screen is
// the checkpoint, and the arena's win dialog (ArenaWinDialog) ends a won run.
#include "GameApp.h"

#include "ArenaWinDialog.h"
#include "Board.h"
#include "CurtainTransition.h"
#include "LuaLinkedClass.h"
#include "TreasureScreen.h"
#include "WidgetManager.h"

using namespace Sexy;

static void TimedModeTSTransitionCallback(void* theApp)
{
	// V1:0x451450: once the curtain is closed, the treasure screen as the checkpoint, over the hidden board
	GameApp* app = (GameApp*)theApp;
	app->mTreasureScreen->SetVisible(true);
	app->mTreasureScreen->SetDisabled(false);
	app->mWidgetManager->PutBehind(app->mTreasureScreen, app->mCurtain);
	app->mBoard->mVisible = false;
	app->mBoard->mDisabled = true;
	LuaLib::CallLua("TimedMode.CleanupPAMs", 0);
	app->mTreasureScreen->StartFullyDisplayed();
	app->mTreasureScreen->CallLinkedMethod("InitTimedModeCheckpoint", 0);
	app->mWidgetManager->SetFocus(app->mTreasureScreen);
	app->Unmute(true);
}

void GameApp::DoTimedModeTSTransition()
{
	// V1:0x4513b0
	DoFullCurtainTransition("");
	mCurtainCallback = TimedModeTSTransitionCallback;
	mCurtainCallbackObject = this;
}

void GameApp::EnableBoardForTMCheckpointContinuation()
{
	// V1:0x4515a0: the board again, under the curtain, after the checkpoint
	mBoard->mVisible = true;
	mBoard->mDisabled = false;
	mWidgetManager->BringToFront(mBoard);
	mWidgetManager->BringToFront(mCurtain);
	mWidgetManager->SetFocus(mBoard);
}

void GameApp::DoArenaWinDialog(bool theFirstTime, const std::string& theCallback)
{
	// V1:0x4519e0: no pause or muting of the board, as DoOkDialog has
	AddDialog(new ArenaWinDialog(theFirstTime, theCallback));
}

void GameApp::SetArenaWinStats(const std::string& theTime, const std::string& thePotions, const std::string& theWords,
                               const std::string& theLevel, const std::string& theAverageWordLength)
{
	// V1:0x451a80
	ArenaWinDialog* aDialog = dynamic_cast<ArenaWinDialog*>(GetDialog(DIALOG_ARENA_WIN));
	// port: the original did not check for the dialog (the scripts call DoArenaWinDialog first)
	if (aDialog != nullptr)
		aDialog->SetStats(theTime, thePotions, theWords, theLevel, theAverageWordLength);
}
