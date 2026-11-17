#include "PopupState.h"

#include "DrawContext.h"
#include "EditBox.h"
#include "Engine.h"
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

} // namespace

PopupState::PopupState(FeedingFrenzyTwo* theApp, GenericScreen* theScreen, AppState* thePreviousState)
	: AppState(theApp), mClosing(false)
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
	if (theAction == "appQuitNo" || theAction == "saveGameCancel" || theAction == "userSelectCancel")
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
			// The players' list (+0x60) shown: not yet translated
			getEngine()->log("PopupState: the players' screen not yet translated\n");
		else
			close();
	}
	else if (theAction == "userEntryCancel")
	{
		if (aUsers->getUserCount() > 0)
			// The players' list (+0x60) shown: not yet translated
			getEngine()->log("PopupState: the players' screen not yet translated\n");
	}
	else
		// pauseResume, appQuitYes, saveGameContinue, saveGameNoContinue, userSelectDone, userSelectNew,
		// userSelectDelete, deleteUserYes, deleteUserNo, pauseOptions, optionsDone, pauseQuit, gameExitSave and the
		// rest: not yet translated
		getEngine()->log("PopupState: the action %s not yet translated\n", theAction.c_str());
}
