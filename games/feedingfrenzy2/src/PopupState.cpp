#include "PopupState.h"

#include "DrawContext.h"
#include "EditBox.h"
#include "Engine.h"
#include "GamePlayState.h"
#include "LevelLoadState.h"
#include "MenuState.h"
#include "Stage.h"
#include "Scene.h"
#include "ScreenMgr.h"
#include "TextItem.h"
#include "Theme.h"
#include "UserInfo.h"
#include "UserManager.h"

using namespace engine;

namespace
{

// 0x5acd1c (feedingfrenzy2): the popups' screen manager, made by the first popup and kept until the shutdown
Ref<ScreenMgr> gPopupScreenMgr;

// 0x488250 (feedingfrenzy2): a key of a straight line
ColorKey makeKey(float theTime, const Color& theColor)
{
	ColorKey aKey;
	aKey.mTime = theTime;
	aKey.mValue = theColor;
	return aKey;
}

// 0x49b880 (feedingfrenzy2): whether a player other than the first has the strings' alienUser name
bool hasAlienUser(FeedingFrenzyTwo* theApp)
{
	return UserManager::instance()->findUser(theApp->mStrings->getString("alienUser")) > 0;
}

} // namespace

PopupState::PopupState(FeedingFrenzyTwo* theApp, GenericScreen* theScreen, AppState* thePreviousState)
	: AppState(theApp), mClosing(false), mOptionsFromPause(false)
{
	if (gPopupScreenMgr == nullptr)
		gPopupScreenMgr = new ScreenMgr();
	mScreen = theScreen;
	mPreviousState = thePreviousState;
	mNextState = nullptr;
}

void PopupState::releaseScreenMgr()
{
	gPopupScreenMgr = nullptr;
}

void PopupState::enter()
{
	Theme* aTheme = mApp->mShellTheme.get();
	mApp->mScreenMgr->setColor(aTheme->getColor("white"));
	mApp->mScreenMgr->setColorMode(COLOR_MODE_MULTIPLY);
	mApp->mContainerSlot->setColor(aTheme->getColor("white"));
	mApp->mContainerSlot->setColorMode(COLOR_MODE_MULTIPLY);
	mApp->mScreenMgr->disable();
	mApp->mScene->addChild(gPopupScreenMgr.get());
	mDarken.clear();
	mDarken.addKey(makeKey(0.0f, aTheme->getColor("white")));
	mDarken.addKey(makeKey(1.0f, aTheme->getColor("popupDarkenColor")));
	mDarkenTime.setTime(0.0f);
	mDarkenTime.start(mScreen->getInDuration(), 1.0f);
	show(mScreen.get());
	mScreen = nullptr;
	mOptionsFromPause = false;
}

void PopupState::exit()
{
	mApp->mScreenMgr->setColor(0.0f, 0.0f, 0.0f);
	mApp->mScreenMgr->setColorMode(COLOR_MODE_NONE);
	mApp->mContainerSlot->setColor(0.0f, 0.0f, 0.0f);
	mApp->mContainerSlot->setColorMode(COLOR_MODE_NONE);
	mDarkenTime.setTime(0.0f);
}

void PopupState::fade(UpdateContext* theContext)
{
	mDarkenTime.setDirection(mClosing ? TIME_BACKWARD : TIME_FORWARD);
	mDarkenTime.advance(theContext->mElapsed);
	Color aColor = mDarken.getValue((float)mDarkenTime.getValue());
	mApp->mScreenMgr->setColor(aColor);
	mApp->mContainerSlot->setColor(aColor);
}

void PopupState::update(UpdateContext* theContext)
{
	// The sounds' update (0x434c80 (feedingfrenzy2)): not yet translated
	fade(theContext);
	// While the menu runs under the popup, its stage's update (0x43a570 (feedingfrenzy2), the application's +0x40):
	// not yet translated
	if (!mClosing)
		return;
	if (gPopupScreenMgr->isBusy())
		return;
	mApp->mScene->removeChild(gPopupScreenMgr.get());
	mApp->mScreenMgr->enable();
	mClosing = false;
	if (mNextState != nullptr)
		mApp->mStateMgr.changeState(mNextState.get());
	else
		mApp->mStateMgr.popState(mPreviousState.get());
}

void PopupState::close()
{
	gPopupScreenMgr->showScreen(nullptr, 0.0f);
	mClosing = true;
}

void PopupState::show(GenericScreen* theScreen)
{
	gPopupScreenMgr->showScreen(theScreen, 0.0f);
}

void PopupState::action(const std::string& theAction)
{
	UserManager* aUsers = UserManager::instance();
	if (theAction == "pauseResume")
	{
		if (mOptionsFromPause)
		{
			mOptionsFromPause = false;
			show(mApp->mPauseScreen.get());
		}
		else
			close();
	}
	else if (theAction == "appQuitNo" || theAction == "saveGameCancel" || theAction == "userSelectCancel")
		close();
	else if (theAction == "userEntryDone")
	{
		EditBox* aNameEntry = static_cast<EditBox*>(mApp->mUserEntry->findComponent("nameEntry"));
		std::string aName = aNameEntry->getText();
		if (aName.empty())
			return;
		if (aUsers->findUser(aName) >= 0)
		{
			TextItem* aNameText = static_cast<TextItem*>(mApp->mUserDuplicateConfirm->findComponent("userName"));
			aNameText->setText(aName);
			show(mApp->mUserDuplicateConfirm.get());
			return;
		}
		Ref<UserInfo> aUser = new UserInfo();
		aUser->setName(aName);
		aUsers->addUser(aUser.get());
		if (aUsers->getUserCount() > 1)
			show(mApp->mUserSelect.get());
		else
			close();
	}
	else if (theAction == "userEntryCancel")
	{
		if (aUsers->getUserCount() > 0)
			show(mApp->mUserSelect.get());
	}
	else if (theAction == "userSelectDone")
	{
		mApp->mUserSelect->selectUser();
		close();
	}
	else if (theAction == "userSelectNew")
	{
		// A row fewer while no player has the strings' alienUser name
		int aRoom = mApp->mUserSelect->getRows();
		if (!hasAlienUser(mApp))
			aRoom--;
		if (aUsers->getUserCount() < aRoom)
			showUserEntry();
		else
			show(mApp->mUserListFullConfirm.get());
	}
	else if (theAction == "userSelectDelete")
	{
		mApp->mUserSelect->selectUser();
		TextItem* aNameText = static_cast<TextItem*>(mApp->mUserDeleteConfirm->findComponent("userName"));
		aNameText->setText(mApp->mUserData->getName());
		show(mApp->mUserDeleteConfirm.get());
	}
	else if (theAction == "deleteUserYes")
	{
		mApp->mUserSelect->deleteUser();
		mApp->mUserSelect->selectUser();
		if (aUsers->getUserCount() == 0)
			showUserEntry();
		else
			show(mApp->mUserSelect.get());
	}
	else if (theAction == "deleteUserNo")
	{
		mApp->mUserSelect->selectUser();
		show(mApp->mUserSelect.get());
	}
	else if (theAction == "continueYes")
	{
		// The game taken up again on the stage lost, with a continue fewer, three lives and the stage's clock
		GameData* aData = mApp->mGameData.get();
		aData->setContinues(aData->getContinues() - 1);
		aData->setLives(3);
		const StageManifest::Stage* anEntry = aData->getStageEntry();
		mApp->mStage->setStageTime((float)anEntry->mStageTime);
		aData->setTimeRollover(anEntry->mNewGameTime);
		mNextState = new GamePlayState(mApp);
		close();
	}
	else if (theAction == "continueNo" || theAction == "gameOverDone")
	{
		// The high scores first (0x49b030 (feedingfrenzy2), the application's +0x8c, given the mode, score and
		// stage): a score they take sends the menu to them (MENU_SCREEN_HIGH_SCORES); not yet translated, as the
		// high scores are not. (continueNo also builds a name it does not use.)
		getEngine()->log("PopupState: the high scores not yet translated\n");
		mApp->mUserData->clearSavedGame(mApp->mGameData->getMode());
		const StageManifest::Stage* aMenuEntry = &mApp->mGameData->getMenuStages().getStages()[0];
		mNextState = new LevelLoadState(mApp, aMenuEntry, new MenuState(mApp));
		// The application's sound at +0x38 stopped (0x490370 (feedingfrenzy2)): not yet translated
		close();
	}
	else
		// appQuitYes, saveGameContinue, saveGameNoContinue, pauseOptions, optionsDone, pauseQuit,
		// gameExitSave and the rest: not yet translated. (The play's pause, sent by the focus lost, is none of the
		// popups' actions and does nothing.)
		if (theAction != "pause")
			getEngine()->log("PopupState: the action %s not yet translated\n", theAction.c_str());
}

void PopupState::showUserEntry()
{
	static_cast<EditBox*>(mApp->mUserEntry->findComponent("nameEntry"))->setText("");
	Component* aCancel = mApp->mUserEntry->findComponent("cancelBtn");
	if (UserManager::instance()->getUserCount() != 0)
	{
		aCancel->setVisible(true);
		aCancel->addOwnState(STATE_INPUT);
	}
	else
	{
		aCancel->setVisible(false);
		aCancel->removeOwnState(STATE_INPUT);
	}
	show(mApp->mUserEntry.get());
}
