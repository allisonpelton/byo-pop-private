// GameApp's arena methods. The arena is the timed mode ("TM" in the scripts' names and the profile's keys,
// GameApp::mTimedMode); its scripts (scripts/arenas, TimedMode) play it. Between its stages the treasure screen is
// the checkpoint.
#include "GameApp.h"

#include "Board.h"
#include "CurtainTransition.h"
#include "LuaLinkedClass.h"
#include "TreasureScreen.h"
#include "WidgetManager.h"

using namespace Sexy;

static void TimedModeTSTransitionCallback(void* theApp)
{
	// 0x451450: once the curtain is closed, the treasure screen as the checkpoint, over the hidden board
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
	// 0x4513b0
	DoFullCurtainTransition("");
	mCurtainCallback = TimedModeTSTransitionCallback;
	mCurtainCallbackObject = this;
}

void GameApp::EnableBoardForTMCheckpointContinuation()
{
	// 0x4515a0: the board again, under the curtain, after the checkpoint
	mBoard->mVisible = true;
	mBoard->mDisabled = false;
	mWidgetManager->BringToFront(mBoard);
	mWidgetManager->BringToFront(mCurtain);
	mWidgetManager->SetFocus(mBoard);
}
