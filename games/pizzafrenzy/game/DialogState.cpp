#include "DialogState.h"
#include "EditBox.h"
#include "MenuState.h"
#include "Players.h"
#include "Scene.h"
#include "ScreenNodes.h"
#include "UserInfo.h"

using namespace pizzafrenzy;

engine::Ref<engine::ScreenMgr> DialogState::sDialogScreens;  // 0x532904

namespace
{
// The sounds the dialogs make
const char* const SOUND_DIALOG_IN = "menu_in";
const char* const SOUND_DIALOG_OUT = "menu_out";

// The screens beneath a dialog are dimmed by this much (each colour channel), and take their colour with a mode that
// adds it
const float DIM_AMOUNT = -0.2f;
const int COLOR_MODE_ADD = 1;
const int COLOR_MODE_NORMAL = 0;

// What a new player starts with in the food bank
const int NEW_PLAYER_FOOD_BANK = 0;

// With more players than this, a new player is followed by the choice of a player
const int PLAYERS_TO_CHOOSE_FROM = 1;
}

// 0x402420
DialogState::DialogState(PizzaFrenzy* theApp, engine::GenericScreen* theScreen, engine::State* theNextState) :
	mApp(theApp), mScreen(theScreen), mNextState(theNextState), mClosing(false), mResume(false), mCity(0),
	mFromPause(false)
{
	if (sDialogScreens == nullptr)
		sDialogScreens = new engine::ScreenMgr();
}

// 0x406170
void DialogState::enter()
{
	mApp->mScreens->setColor(DIM_AMOUNT, DIM_AMOUNT, DIM_AMOUNT);
	mApp->mScreens->setColorMode(COLOR_MODE_ADD);
	mApp->mScreens->disconnect();
	mApp->mScene->addChild(sDialogScreens);
	show(mScreen);
	mScreen = nullptr;
	mApp->mSounds->play(SOUND_DIALOG_IN, 1.0f, 1.0f);
	mResume = false;
	mCity = 0;
}

// 0x402670
void DialogState::exit()
{
	mApp->mScreens->setColor(0, 0, 0);
	mApp->mScreens->setColorMode(COLOR_MODE_NORMAL);
}

// 0x40cf30
void DialogState::update(engine::UpdateInfo& theInfo)
{
	if (!mClosing)
		return;
	if (sDialogScreens->isBusy())
		return;
	mApp->mScene->removeChild(sDialogScreens);
	mApp->mScreens->connect();
	mClosing = false;
	if (!mResume)
	{
		mApp->mStates.changeState(mNextState);
		return;
	}
	if (!mApp->mUserGameData->hasSavedGame(mApp->mGameMode))
	{
		if (mApp->mUserGameData->mMaxCity != 0)
		{
			mApp->mShowCitySelect = true;
			mApp->mStates.changeState(mNextState);
			return;
		}
	}
	mApp->startGame(mApp->mGameMode, mCity);
}

// 0x406270
void DialogState::show(engine::GenericScreen* theScreen)
{
	sDialogScreens->showScreen(theScreen, 0);
	mApp->mSounds->play(SOUND_DIALOG_IN, 1.0f, 1.0f);
}

// 0x406310
void DialogState::close()
{
	sDialogScreens->showScreen(nullptr, 0);
	mClosing = true;
	mApp->mSounds->play(SOUND_DIALOG_OUT, 1.0f, 1.0f);
}

// 0x40d3e0. The original's actions for posting a score online ("postGlobalScore" and "cancelGlobalScore"), and the
// dialog for it that the end of a game showed after the high scores, are not here: the port has no online scores.
void DialogState::action(const std::string& theAction)
{
	if (theAction.compare("done") == 0 || theAction.compare("bookCancel") == 0)
	{
		if (mFromPause)
		{
			mFromPause = false;
			show(mApp->mPauseMenu);
			return;
		}
		close();
	}
	else if (theAction.compare("quit") == 0)
	{
		mFromPause = true;
		show(mApp->mAppQuitConfirm);
	}
	else if (theAction.compare("appQuitCancel") == 0)
		close();
	else if (theAction.compare("appQuitFinal") == 0)
		mApp->quitApplication();
	else if (theAction.compare("saveGameContinue") == 0)
	{
		close();
		mResume = true;
	}
	else if (theAction.compare("saveGameNoContinue") == 0)
	{
		mApp->mUserGameData->clearSavedGame(mApp->mGameMode);
		close();
		mResume = true;
	}
	else if (theAction.compare("nameEntryDone") == 0)
	{
		engine::EditBox* aNameBox = static_cast<engine::EditBox*>(mApp->mNewUserEntry->getNode("nameEntry"));
		const std::string& aName = aNameBox->getText();
		if (aName.empty())
			return;
		engine::Ref<engine::UserInfo> aUser = new engine::UserInfo();
		aUser->setName(aName);
		aUser->getParams().setInt("foodBank", NEW_PLAYER_FOOD_BANK);
		engine::Players* aPlayers = engine::Players::instance();
		aPlayers->add(aUser);
		if (aPlayers->count() > PLAYERS_TO_CHOOSE_FROM)
		{
			show(mApp->mUserSelect);
			return;
		}
		close();
	}
	else if (theAction.compare("nameEntryCancel") == 0)
		show(mApp->mUserSelect);
	else if (theAction.compare("newUser") == 0)
	{
		if (mApp->mUserSelect->isListFull())
			return;
		show(mApp->mNewUserEntry);
	}
	else if (theAction.compare("deleteUser") == 0)
	{
		std::string aName = mApp->mUserSelect->getPickedName();
		mApp->mDeleteUserText->setText(formatText(mApp->mDeleteUserTemplate, aName));
		show(mApp->mDeleteUserConfirm);
	}
	else if (theAction.compare("deleteUserYes") == 0)
	{
		show(mApp->mUserSelect);
		mApp->mUserSelect->removePlayer();
	}
	else if (theAction.compare("deleteUserNo") == 0)
		show(mApp->mUserSelect);
	else if (theAction.compare("pauseOptions") == 0)
	{
		mFromPause = true;
		show(mApp->mOptions);
	}
	else if (theAction.compare("pauseQuit") == 0)
	{
		mFromPause = true;
		show(mApp->mGameExitConfirm);
	}
	else if (theAction.compare("toppingScreen") == 0)
	{
		// the topping book (the application's +0xe0): its slot 0x180, and then this state shows it; translated next
		// with the topping screens
		mFromPause = true;
	}
	else if (theAction.compare("gameExitSave") == 0)
	{
		mApp->saveGame();
		// the game's +0x110 object's slot 0x20 stops the game when there is one; translated next
		mNextState = new MenuState(mApp);
		close();
	}
	else if (theAction.compare("gameExitQuit") == 0)
	{
		// the game's +0x110 object's slot 0x20 stops the game when there is one; translated next
		mApp->mUserGameData->clearSavedGame(mApp->mGameMode);
		if (mApp->mHighScores->addCurrentScore())
			mApp->mShowHighScores = true;
		mNextState = new MenuState(mApp);
		close();
	}
	else if (theAction.compare("gameExitCancel") == 0)
		close();
	else if (theAction.compare("userSelectCancel") == 0)
		close();
	else if (theAction.compare("userSelectOk") == 0)
	{
		mApp->mUserSelect->choosePlayer();
		close();
	}
	else if (theAction.compare("highScoreResetConfirm") == 0)
	{
		mApp->mHighScores->clear();
		mApp->mShowHighScores = true;
		close();
	}
	else if (theAction.compare("highScoreResetCancel") == 0)
	{
		mApp->mShowHighScores = true;
		close();
	}
}
