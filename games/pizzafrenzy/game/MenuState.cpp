#include "MenuState.h"
#include "DialogState.h"
#include "Host.h"
#include "Players.h"
#include "ScreenNodes.h"

#include <cstdio>

using namespace pizzafrenzy;

namespace
{
// The main menu fades in over this long when the state starts
const float MAIN_MENU_DELAY = 0.2f;

// The modes' numbers (the application's +0x58)
const int SPEED_MODE = 0;
const int MEMORY_MODE = 1;
const int CONCENTRATION_MODE = 2;

// The music of the menu, in the tile manifest
const char* const MENU_MUSIC = "music_mainMenu";

// The pizza designer is locked while the player has fewer toppings than this
const int DESIGNER_TOPPINGS = 5;
}

// 0x405f90. The original's slot 0x184 of the main menu (0x43b0c0) is called here with the settings' link; the port
// has no link (MainMenu::setup).
void MenuState::enter()
{
	if (engine::Players::instance()->current() == nullptr)
		mNeedsPlayer = true;
	else
	{
		PizzaFrenzy::sDesignerUnlockOverride = false;
		mNeedsPlayer = false;
	}
	if (!mApp->mShowHighScores)
	{
		if (!mApp->mShowCitySelect)
			mApp->mScreens->showScreen(mApp->mMainMenu, MAIN_MENU_DELAY);
		else
		{
			mApp->showCitySelect();
			mApp->mShowCitySelect = false;
		}
	}
	else
	{
		mApp->mHighScores->refresh();
		mApp->mScreens->showScreen(mApp->mHighScores, 0);
		mApp->mShowHighScores = false;
	}
	mApp->mMusicPlayer->play(mApp->mTileManifest->getMusic(MENU_MUSIC));
	mApp->mMainMenu->refresh();
	mClearGame = true;
}

// 0x403640
void MenuState::update(engine::UpdateInfo& theInfo)
{
	if (mNeedsPlayer && !mApp->mScreens->isBusy())
		mApp->mStates.changeState(new DialogState(mApp, mApp->mNewUserEntry, this));
	if (mClearGame && !mApp->mScreens->isBusy())
	{
		clearEndedGame();
		mClearGame = false;
	}
}

// 0x4022f0: translated next with the game
void MenuState::clearEndedGame()
{
}

// 0x40c7a0: the dialog state of the actions that open a dialog
void MenuState::showDialog(engine::GenericScreen* theScreen)
{
	mApp->mStates.changeState(new DialogState(mApp, theScreen, this));
}

// 0x40c7a0. The original's "gameLink" action, which opened the game's link, is not here.
void MenuState::action(const std::string& theAction)
{
	if (theAction.compare("newSpeedGame") == 0 || theAction.compare("newMemoryGame") == 0 ||
		theAction.compare("newConcentrationGame") == 0)
	{
		if (theAction.compare("newSpeedGame") == 0)
			mApp->mGameMode = SPEED_MODE;
		else if (theAction.compare("newMemoryGame") == 0)
			mApp->mGameMode = MEMORY_MODE;
		else
			mApp->mGameMode = CONCENTRATION_MODE;
		if (mApp->mUserGameData->hasSavedGame(mApp->mGameMode))
			showDialog(mApp->mRestoreGameConfirm);
		else if (mApp->mUserGameData->mMaxCity > 0)
			mApp->showCitySelect();
		else
			mApp->startGame(mApp->mGameMode, 0);
	}
	else if (theAction.compare("viewToppingBook") == 0)
	{
		mApp->mToppingBook->showPages();
		showDialog(mApp->mToppingBook);
	}
	else if (theAction.compare("nextLevel") == 0)
	{
		mApp->advanceLevel();
		mApp->startLevel();
	}
	else if (theAction.compare("options") == 0)
	{
		mApp->mOptions->setupOptions(mApp, mApp->mSettings);
		showDialog(mApp->mOptions);
	}
	else if (theAction.compare("notme") == 0)
		showDialog(mApp->mUserSelect);
	else if (theAction.compare("showHighScores") == 0)
	{
		mApp->mHighScores->refresh();
		mApp->mScreens->showScreen(mApp->mHighScores, 0);
	}
	else if (theAction.compare("highScoreDone") == 0 || theAction.compare("citySelectCancel") == 0 ||
		theAction.compare("bookCancel") == 0)
		mApp->mScreens->showScreen(mApp->mMainMenu, 0);
	else if (theAction.compare("highScoreReset") == 0)
		showDialog(mApp->mHighScoreResetConfirm);
	else if (theAction.compare(0, 10, "citySelect") == 0)
	{
		// "citySelect" and a city's number start a game in that city
		int aCity = -1;
		sscanf(theAction.c_str(), "citySelect%d", &aCity);
		if (aCity < 0 || (int)mApp->mCities.size() <= aCity)
			return;
		mApp->startGame(mApp->mGameMode, aCity);
	}
	else if (theAction.compare("edit") == 0)
		mApp->startPizzaDesigner();
	else if (theAction.compare("editPizza") == 0)
	{
		int aToppingCount = (int)mApp->mUserGameData->mToppingLevels.size();
		if (aToppingCount < DESIGNER_TOPPINGS && !PizzaFrenzy::sDesignerUnlockOverride)
		{
			engine::TextItem* aNotice = static_cast<engine::TextItem*>(mApp->mUnlockNotice->getNode("notifyText"));
			if (aNotice != nullptr)
				aNotice->setText(formatText(mApp->mUnlockNoticeTemplate, aToppingCount));
			showDialog(mApp->mUnlockNotice);
		}
		else
			mApp->startPizzaEditor();
	}
	else if (theAction.compare("quit") == 0)
		showDialog(mApp->mAppQuitConfirm);
	else if (theAction.compare("loseFullScreen") == 0)
	{
		if (engine::getHost()->isFullScreen())
		{
			action("pauseGame");
			mApp->setFullScreen(false);
		}
	}
}
