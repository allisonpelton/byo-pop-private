#include "MenuState.h"

#include "Engine.h"
#include "PopupState.h"
#include "ScreenMgr.h"
#include "TextItem.h"

using namespace engine;

void MenuState::enter()
{
	if (mApp->mMenuScreen == MENU_SCREEN_HIGH_SCORES)
	{
		// The game data's high scores brought up to date (0x498940 (feedingfrenzy2), on its +0x0c) and the high
		// score screen (+0x8c) shown: not yet translated
		getEngine()->log("MenuState: the high score screen not yet translated\n");
		mApp->mMenuScreen = MENU_SCREEN_MAIN;
	}
	else if (mApp->mMenuScreen == MENU_SCREEN_MAP)
		// The map (+0x84) shown: not yet translated
		getEngine()->log("MenuState: the map not yet translated\n");
	else
		mApp->mScreenMgr->showScreen(mApp->mMainMenu.get(), 0.0f);
	showPlayer();
	mApp->mMenuRunning = true;
	// The stage's music player (its +0x2d4) playing menuMusic, unless it already is (0x489670 and 0x48d420
	// (feedingfrenzy2)), the application's sound at +0x38 started unless its +0x321 is set (0x4929a0
	// (feedingfrenzy2)), and the stage started (its slot 0x14): not yet translated, as the stage and the sounds are not
	getEngine()->log("MenuState: the menu's stage and music not yet translated\n");
}

void MenuState::exit()
{
	// The screen hidden unless it is the map (+0x84), which is not yet translated
	mApp->mScreenMgr->showScreen(nullptr, 0.0f);
	mApp->mMenuRunning = false;
	// The application's sound at +0x38 stopped (0x490370 (feedingfrenzy2)): not yet translated
	mApp->mMenuScreen = MENU_SCREEN_MAIN;
}

void MenuState::update(UpdateContext* theContext)
{
	// The sounds' update (0x434c80 (feedingfrenzy2)) and the stage's (0x43a570 (feedingfrenzy2)): not yet translated
	if (mPlayerChecked)
		return;
	if (mApp->mMainMenu->getPhase() != SCREEN_SHOWN)
		return;
	if (mApp->mUserData->getName().empty())
		mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mUserEntry.get(), this));
	mPlayerChecked = true;
}

void MenuState::action(const std::string& theAction)
{
	if (theAction == "credits")
		mApp->mScreenMgr->showScreen(mApp->mCredits.get(), 0.0f);
	else if (theAction == "gameMapCancel" || theAction == "creditsDone" || theAction == "highScoreDone")
		mApp->mScreenMgr->showScreen(mApp->mMainMenu.get(), 0.0f);
	else if (theAction == "switchUser")
	{
		mApp->mUserSelect->init();
		mApp->mMenuScreen = MENU_SCREEN_MAIN;
		mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mUserSelect.get(), this));
	}
	else
		// newGame, newTimedGame, quit, mainMenuOptions, debugSelectLevel, highScores, scoreModeRight,
		// scoreModeLeft, highScoreReset and an action holding levelStart: not yet translated. (The original's
		// gameLinkClick opened the distributor's link; it goes with its button)
		getEngine()->log("MenuState: the action %s not yet translated\n", theAction.c_str());
}

void MenuState::suspend()
{
	mApp->mScreenMgr->showScreen(nullptr, 0.0f);
}

void MenuState::resume()
{
	if (mApp->mMenuScreen == MENU_SCREEN_MAP)
		// The map (+0x84): not yet translated
		getEngine()->log("MenuState: the map not yet translated\n");
	else if (mApp->mMenuScreen == MENU_SCREEN_HIGH_SCORES)
		// The high scores (+0x8c): not yet translated
		getEngine()->log("MenuState: the high score screen not yet translated\n");
	else
		mApp->mScreenMgr->showScreen(mApp->mMainMenu.get(), 0.0f);
	showPlayer();
}

void MenuState::showPlayer()
{
	TextItem* aNameText = static_cast<TextItem*>(mApp->mMainMenu->findComponent("userNameText"));
	aNameText->setText(mApp->mUserData->getName());

	TextItem* aWelcomeText = static_cast<TextItem*>(mApp->mMainMenu->findComponent("welcomeText"));
	bool aNewPlayer = true;
	for (int aMode = 0; aMode < GAME_MODE_COUNT; aMode++)
	{
		if (mApp->mUserData->getMode(aMode).mMaxStage > 0)
			aNewPlayer = false;
	}
	aWelcomeText->setText(mApp->mStrings->getString(aNewPlayer ? "welcomeNew" : "welcomeBack"));

	int aStageCount = (int)mApp->mGameData->getStages().getStages().size();
	bool aShowTrophy = mApp->mUserData->getMode(GAME_MODE_NORMAL).mMaxStage >= aStageCount ||
	                   mApp->mUserData->getMode(GAME_MODE_TIMED).mMaxStage >= aStageCount;
	Component* aTrophy = mApp->mMainMenu->findComponent("menuTrophyHigh");
	if (aTrophy != nullptr)
		aTrophy->setVisible(aShowTrophy);
	aTrophy = mApp->mMainMenu->findComponent("menuTrophyLow");
	if (aTrophy != nullptr)
		aTrophy->setVisible(aShowTrophy);
}
