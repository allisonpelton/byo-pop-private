#include "MenuState.h"

#include "Engine.h"
#include "LevelLoadState.h"
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
		mApp->mScreenMgr->showScreen(mApp->mGameMap.get(), 0.0f);
	else
		mApp->mScreenMgr->showScreen(mApp->mMainMenu.get(), 0.0f);
	showPlayer();
	mApp->mMenuRunning = true;
	// The stage's music player (its +0x2d4) playing menuMusic, unless it already is (0x489670 and 0x48d420
	// (feedingfrenzy2)), and the application's sound at +0x38 started unless its +0x321 is set (0x4929a0
	// (feedingfrenzy2)): not yet translated, as the sounds are not
	getEngine()->log("MenuState: the menu's music not yet translated\n");
	mApp->mStage->start();
}

void MenuState::exit()
{
	// The map stays, as the load screen of the stage picked on it
	if (mApp->mScreenMgr->getCurrent() != mApp->mGameMap.get())
		mApp->mScreenMgr->showScreen(nullptr, 0.0f);
	mApp->mMenuRunning = false;
	// The application's sound at +0x38 stopped (0x490370 (feedingfrenzy2)): not yet translated
	mApp->mMenuScreen = MENU_SCREEN_MAIN;
}

void MenuState::update(UpdateContext* theContext)
{
	// The sounds' update (0x434c80 (feedingfrenzy2)): not yet translated
	mApp->mStage->update(theContext);
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
	if (theAction == "newGame")
		startGame(GAME_MODE_NORMAL);
	else if (theAction == "newTimedGame")
		startGame(GAME_MODE_TIMED);
	else if (theAction == "credits")
		mApp->mScreenMgr->showScreen(mApp->mCredits.get(), 0.0f);
	else if (theAction == "gameMapCancel" || theAction == "creditsDone" || theAction == "highScoreDone")
		mApp->mScreenMgr->showScreen(mApp->mMainMenu.get(), 0.0f);
	else if (theAction.find("levelStart") != std::string::npos)
		startStage(theAction.substr(theAction.find(':') + 1));
	else if (theAction == "switchUser")
	{
		mApp->mUserSelect->init();
		mApp->mMenuScreen = MENU_SCREEN_MAIN;
		mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mUserSelect.get(), this));
	}
	else
		// quit, mainMenuOptions, debugSelectLevel, highScores, scoreModeRight,
		// scoreModeLeft and highScoreReset: not yet translated. (The original's
		// gameLinkClick opened the distributor's link; it goes with its button)
		getEngine()->log("MenuState: the action %s not yet translated\n", theAction.c_str());
}

void MenuState::startGame(int theMode)
{
	mApp->mGameData->newGame(theMode);
	if (mApp->mUserData->hasSavedGame(theMode))
	{
		mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mRestoreGameScreen.get(), this));
		return;
	}
	mApp->mFreshGame = true;
	// 0x49fa20 (feedingfrenzy2): a stage of the mode reached
	if (mApp->mUserData->getMode(mApp->mGameData->getMode()).mMaxStage > 0)
	{
		mApp->mGameMap->showFor(true, mApp->mGameData->getMode());
		mApp->mScreenMgr->showScreen(mApp->mGameMap.get(), 0.0f);
		return;
	}
	const StageManifest::Stage* anEntry = mApp->mGameData->getStageEntry();
	if (theMode == GAME_MODE_TIMED)
		mApp->mGameData->setTimeRollover(anEntry->mNewGameTime);
	mApp->mStateMgr.changeState(new LevelLoadState(mApp, anEntry, nullptr));
}

void MenuState::startStage(const std::string& theStageId)
{
	// The shell theme's levelChooseSound played at full volume (0x4747a0 (feedingfrenzy2)): not yet translated, as
	// the sound effects are not
	int aMode = mApp->mGameData->getMode();
	mApp->mGameData->newGame(aMode);
	mApp->mUserData->clearSavedGame(aMode);
	mApp->mGameData->setStage(mApp->mGameData->getStages().findStage(theStageId));
	const StageManifest::Stage* anEntry = mApp->mGameData->getStageEntry();
	mApp->mGameData->setTimeRollover(anEntry->mNewGameTime);
	mApp->mStateMgr.changeState(new LevelLoadState(mApp, anEntry, nullptr));
}

void MenuState::suspend()
{
	mApp->mScreenMgr->showScreen(nullptr, 0.0f);
}

void MenuState::resume()
{
	if (mApp->mMenuScreen == MENU_SCREEN_MAP)
		mApp->mScreenMgr->showScreen(mApp->mGameMap.get(), 0.0f);
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
