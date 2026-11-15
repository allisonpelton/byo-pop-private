#include "GameState.h"
#include "MenuState.h"

using namespace pizzafrenzy;

namespace
{
const char* const ACTION_PAUSE_GAME = "pauseGame";
const char* const ACTION_BLUR = "blur";  // 0x4f98d0
const char* const ACTION_MENU = "menu";  // 0x4f99f0
const char* const ACTION_LEVEL_END = "levelEnd";
const char* const ACTION_GAME_OVER = "gameOver";
}

// 0x4063b0: a window that has lost the focus pauses at once
void GameState::enter()
{
	mApp->mScreens->showScreen(mApp->mGameScreen, 0);
	mApp->saveGame();
	mApp->mMode->setPaused(false);
	if (!mApp->isActive())
		action(ACTION_BLUR);
}

// 0x402720
void GameState::update(engine::UpdateInfo& theInfo)
{
	mApp->mMode->update(theInfo);
}

// 0x4075e0
void GameState::action(const std::string& theAction)
{
	if (theAction.compare(ACTION_PAUSE_GAME) == 0 || theAction.compare(ACTION_BLUR) == 0
	    || theAction.compare(ACTION_MENU) == 0)
	{
		mApp->mMode->setPaused(true);
		mApp->pauseGame();
		return;
	}
	if (theAction.compare(ACTION_LEVEL_END) == 0)
	{
		mApp->endLevel();
		return;
	}
	if (theAction.compare(ACTION_GAME_OVER) != 0)
		return;
	// The original posts the score on the global board here, in a dialog when the game scored; the port has no online
	// scores, so the menu comes straight (the same as the story state's end).
	mApp->mUserGameData->clearSavedGame(mApp->mGameMode);
	if (mApp->mHighScores->addCurrentScore())
		mApp->mShowHighScores = true;
	mApp->mStates.changeState(new MenuState(mApp));
}
