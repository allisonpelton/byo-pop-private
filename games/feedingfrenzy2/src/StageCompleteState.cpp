#include "StageCompleteState.h"

#include "Engine.h"
#include "LevelLoadState.h"
#include "MenuState.h"
#include "PopupState.h"
#include "ScreenMgr.h"
#include "Stage.h"
#include "StageCompleteScreen.h"

using namespace engine;

void StageCompleteState::enter()
{
	int aFoodBank = mApp->mUserData->getFoodBank();
	mApp->mFreshGame = false;
	mApp->mStageInPlay = false;
	if (mApp->mStage->isBonus())
		mScreen = mApp->mStageCompleteBonus.get();
	else
		mScreen = mApp->mStageCompleteNormal.get();
	// (The id is the game's stage entry's, 0x403460 (feedingfrenzy2))
	mScreen->setup(mApp->mGameData->getStageEntry()->mId, mApp->mStage.get(), aFoodBank);
	int aTotal = mScreen->getTotal();
	if (aTotal < 1)
		aTotal = 0;
	mApp->mUserData->setFoodBank(aTotal);
	mApp->mUserData->save();
	mApp->mScreenMgr->showScreen(mScreen.get(), 0.0f);

	GameData* aGame = mApp->mGameData.get();
	int aNextStage = aGame->getStage() + 1;
	int aStageCount = (int)aGame->getStages().getStages().size();
	if (aNextStage < aStageCount)
	{
		aGame->setStage(aNextStage);
		// 0x4966d0 (feedingfrenzy2): the game kept in the player's record, and the player saved
		mApp->mUserData->recordGame(*aGame);
		mApp->mUserData->save();
		isGameFinished = false;
	}
	else
	{
		mApp->mUserData->setMaxStage(aGame->getMode(), aStageCount);
		mApp->mUserData->clearSavedGame(aGame->getMode());
		isGameFinished = true;
	}
}

void StageCompleteState::exit()
{
	mScreen = nullptr;
}

void StageCompleteState::update(UpdateContext* theContext)
{
	if (!mApp->mActive)
		return;
	// The sound system's update (0x434c80 (feedingfrenzy2)): not yet translated
	mScreen->updateCount(theContext);
}

void StageCompleteState::action(const std::string& theAction)
{
	if (theAction == "stageCompleteContinue")
	{
		if (!isGameFinished)
		{
			mApp->mStateMgr.changeState(new LevelLoadState(mApp, mApp->mGameData->getStageEntry(), nullptr));
			return;
		}
		// The menu's first stage loaded, then the menu (0x499b50 (feedingfrenzy2)). Before it, the high scores
		// (0x49b030 (feedingfrenzy2), the application's +0x8c, given the mode, score and stage): a score they take
		// sends the menu to them (MENU_SCREEN_HIGH_SCORES): not yet translated, as the high scores are not
		getEngine()->log("StageCompleteState: the high scores not yet translated\n");
		const StageManifest::Stage* aMenuEntry = &mApp->mGameData->getMenuStages().getStages()[0];
		mApp->mStateMgr.changeState(new LevelLoadState(mApp, aMenuEntry, new MenuState(mApp)));
	}
	else if (theAction == "stageCompleteOptions")
		// The options (+0xa4, their 0x50ab13 (feedingfrenzy2) given false first) as a popup over the screen: not yet
		// translated
		getEngine()->log("StageCompleteState: the options not yet translated\n");
	else if (theAction == "stageCompleteQuit")
		mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mGameExitConfirm.get(), this));
	else if (theAction == "debugFunFactNext")
		mScreen->nextFunFact();
	else if (theAction == "debugFunFactPrev")
		mScreen->previousFunFact();
}
