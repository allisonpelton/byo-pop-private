#include "GamePlayState.h"

#include "Engine.h"
#include "LevelLoadState.h"
#include "Mouse.h"
#include "NumberText.h"
#include "PlayerRanks.h"
#include "PopupState.h"
#include "StageCompleteState.h"
#include "ScreenMgr.h"

using namespace engine;

namespace
{

// The keys the play answers (their virtual-key codes)
const int KEY_ESCAPE = 0x1b;
const int KEY_SPACE = 0x20;
const int KEY_F = 0x46;
const int KEY_P = 0x50;

} // namespace

void GamePlayState::enter()
{
	mApp->mStage->start();
	mSpeed = NORMAL_SPEED;
	mApp->mScreenMgr->showScreen(nullptr, 0.0f);
	mApp->mStageInPlay = true;
	acquireMouse();
}

void GamePlayState::exit()
{
	getEngine()->getMouse()->release();
}

void GamePlayState::acquireMouse()
{
	getEngine()->getMouse()->acquire();
}

void GamePlayState::update(UpdateContext* theContext)
{
	if (!mApp->mActive)
	{
		showPause();
		// Then the sound system's update (0x434c80 (feedingfrenzy2)): not yet translated
		return;
	}
	// The sound system's update (0x434c80 (feedingfrenzy2)): not yet translated
	theContext->mElapsed = theContext->mElapsed / mSpeed;
	mApp->mStage->update(theContext);
	if (mApp->mStage->getRunState() != nullptr)
		return;

	if (mApp->mStage->isCompleted() || mApp->mStage->isBonus())
	{
		// The stage's end in its place
		mApp->mStateMgr.changeState(new StageCompleteState(mApp));
		return;
	}
	// The stage lost: with continues left the offer to go on, else the game over
	if (mApp->mGameData->getContinues() > 0)
		showContinue();
	else
		showGameOver();
}

void GamePlayState::showContinue()
{
	// The stage theme's gameOver music first: not yet translated
	TextItem* aContinuesText = static_cast<TextItem*>(mApp->mGameOverContinue->findComponent("contLeftNum"));
	aContinuesText->setText(localeNumber(mApp->mGameData->getContinues()));
	mApp->mStateMgr.changeState(new PopupState(mApp, mApp->mGameOverContinue.get(), nullptr));
}

void GamePlayState::showGameOver()
{
	// The stage theme's gameOver music first: not yet translated
	GameData* aData = mApp->mGameData.get();
	mApp->mUserData->clearSavedGame(aData->getMode());
	GenericScreen* aScreen = mApp->mGameOver.get();
	static_cast<TextItem*>(aScreen->findComponent("scoreText"))->setText(groupedNumber(aData->getScore()));
	// ("%s%s" in the original)
	std::string aHeader = mApp->mUserData->getName() + mApp->mStrings->getString("rankHdrSuffix");
	static_cast<TextItem*>(aScreen->findComponent("rankHdrText"))->setText(aHeader);
	static_cast<TextItem*>(aScreen->findComponent("rankText"))
		->setText(mApp->mPlayerRanks->getTitle(mApp->mUserData->getFoodBank()));
	mApp->mStateMgr.changeState(new PopupState(mApp, aScreen, nullptr));
}

void GamePlayState::action(const std::string& theAction)
{
	if (theAction == "pause")
		showPause();
	else if (theAction == "reload")
	{
		// The application's sound at +0x38 stopped first (0x490370 (feedingfrenzy2)): not yet translated
		mApp->mStateMgr.changeState(new LevelLoadState(mApp, mApp->mGameData->getStageEntry(), nullptr));
	}
	else if (theAction == "slowmo")
	{
		// 1, 2, 4, 8, 16 and back to 1; any other speed stays
		if (mSpeed == SLOWEST_SPEED)
			mSpeed = NORMAL_SPEED;
		else if (mSpeed == 8.0f || mSpeed == 4.0f || mSpeed == 2.0f || mSpeed == NORMAL_SPEED)
			mSpeed = mSpeed * 2.0f;
	}
}

void GamePlayState::suspend()
{
	exit();
	mApp->mStage->pause(true);
}

void GamePlayState::resume()
{
	acquireMouse();
	mApp->mStage->pause(false);
	mApp->mStageInPlay = true;
}

void GamePlayState::keyDown(int theKey)
{
	if (theKey == KEY_P || theKey == KEY_ESCAPE || theKey == KEY_SPACE)
		mApp->mStateMgr.action("pause");
	else if (theKey == KEY_F)
	{
		mApp->mStateMgr.action("pause");
		// Then the window switched to the full screen or back (the engine's slots 0x1c and 0x18), the options told
		// (+0xb8, its slot 4) and the options screen set from them (0x487da0 (feedingfrenzy2)): not yet translated
	}
}

void GamePlayState::showPause()
{
	mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mPauseScreen.get(), this));
}
