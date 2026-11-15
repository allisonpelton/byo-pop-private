#include "NewToppingState.h"

using namespace pizzafrenzy;

namespace
{
const char* const SOUND_NEW_TOPPING = "bonus_newTopping";
const char* const STORY_MUSIC = "music_story";

// The player has this many toppings when the pizza designer opens by itself
const int DESIGNER_OPENS_AT_TOPPINGS = 5;
}

// 0x406630
void NewToppingState::enter()
{
	mApp->mSounds->play(SOUND_NEW_TOPPING, 1.0f, 1.0f);
	mApp->mMusicPlayer->play(mApp->mTileManifest->getMusic(STORY_MUSIC));
	mApp->mScreens->showScreen(mApp->mNewToppingScreen, 0);
}

// 0x4084b0
void NewToppingState::action(const std::string& theAction)
{
	if (theAction.compare("toppingSelected") == 0)
	{
		mApp->saveGame();
		if ((int)mApp->mGameStats->mToppingLevels.size() == DESIGNER_OPENS_AT_TOPPINGS)
		{
			// the pizza designer's screen (the application's +0xe8) is shown: translated with the designer
			return;
		}
	}
	else if (theAction.compare("keepPlaying") != 0)
	{
		if (theAction.compare("designPizza") == 0)
			mApp->startPizzaEditor();
		return;
	}
	mApp->showLevelStart();
}
