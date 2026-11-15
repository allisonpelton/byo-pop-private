#include "LevelStartState.h"

using namespace pizzafrenzy;

namespace
{
const char* const STORY_MUSIC = "music_story";
const char* const ACTION_SELECTION_DONE = "selectionDone";
const char* const ACTION_MENU = "menu";  // 0x4f99f0

// The vans' toppings are picked from this many toppings of the player's or more
const int PICKS_FROM_TOPPINGS = 3;
}

// 0x407850
void LevelStartState::enter()
{
	if ((int)mApp->mGameStats->mToppingLevels.size() < PICKS_FROM_TOPPINGS)
	{
		mApp->setUpLevel();
		return;
	}
	mApp->mMusicPlayer->play(mApp->mTileManifest->getMusic(STORY_MUSIC));
	mApp->mScreens->showScreen(mApp->mLevelStartScreen, 0);
	mApp->mLevelStartScreen->connect();
}

// 0x407940
void LevelStartState::action(const std::string& theAction)
{
	if (theAction.compare(ACTION_SELECTION_DONE) == 0)
	{
		mApp->setUpLevel();
		return;
	}
	if (theAction.compare(ACTION_MENU) == 0)
		mApp->pauseGame();
}
