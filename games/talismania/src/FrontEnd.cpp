#include "FrontEnd.h"

#include "AtlantisApp.h"
#include "Engine.h"
#include "Transition.h"

namespace
{

// The fades of the front-end screens (the float 0.3 0x3ecccccd the original passes at 0x403fe0 and after)
const float MENU_FADE_SECONDS = 0.4f;

// The credits' layout (a string of 0x4039b0)
const char* const CREDITS_LAYOUT = "config/screens/menus/storyCredits.xml";

} // namespace

// 0x4039b0 (the stacks and the screens): the sky stack's layers by their names
FrontEnd::FrontEnd(AtlantisApp* theApp)
	: mApp(theApp), mSkyStack(theApp->mScene.get(), "skymenu", "skymenupopup", "darken_sky", "darken_skypopup")
{
	mMainMenu = makeScreen(mApp->mOptions.mainMenuLayout);
	mOptionsMenu = makeScreen(mApp->mOptions.optionsMenuLayout);
	mHighScores = makeScreen(mApp->mOptions.highScoreMenuLayout);
	mCredits = makeScreen(CREDITS_LAYOUT);
	mQuitConfirm = makeScreen(mApp->mOptions.quitConfirmDialog);

	// The registration badge and the buttons that lead out of the game (the trial badge, the page of reasons to buy and
	// the page of more games) are removed, not hidden
	static const char* const REMOVED[] = {"trialbadge", "whybuybtn", "moregamesbtn"};
	for (size_t i = 0; i < sizeof(REMOVED) / sizeof(REMOVED[0]); i++)
	{
		engine::Component* aRemoved = mMainMenu->findComponent(REMOVED[i]);
		if (aRemoved != nullptr && aRemoved->getParent() != nullptr)
			aRemoved->getParent()->removeChild(aRemoved);
	}

	mMainMenu->mActionSignal.connect(this, &FrontEnd::onMainMenuAction);
	mOptionsMenu->mActionSignal.connect(this, &FrontEnd::onOptionsAction);
	mHighScores->mActionSignal.connect(this, &FrontEnd::onHighScoresAction);
	mCredits->mActionSignal.connect(this, &FrontEnd::onCreditsAction);
	mQuitConfirm->mActionSignal.connect(this, &FrontEnd::onQuitAction);
}

engine::GenericScreen* FrontEnd::makeScreen(const std::string& theLayout)
{
	engine::GenericScreen* aScreen = new engine::GenericScreen();
	aScreen->load(theLayout, mApp->mTheme.get());
	aScreen->setTransitions(new engine::FadeTransition(MENU_FADE_SECONDS), new engine::FadeTransition(MENU_FADE_SECONDS));
	return aScreen;
}

void FrontEnd::showMainMenu()
{
	mSkyStack.push(mMainMenu.get());
}

// 0x407510: the main menu's buttons. The main menu stays on the stack under the screens it opens
void FrontEnd::onMainMenuAction(const std::string& theAction)
{
	if (theAction == "optionsmenu")
		mSkyStack.push(mOptionsMenu.get(), true);
	else if (theAction == "highscores")
		mSkyStack.push(mHighScores.get(), true);
	else if (theAction == "credits")
		mSkyStack.push(mCredits.get(), true);
	else if (theAction == "quit")
		mSkyStack.push(mQuitConfirm.get(), true);
	else
		engine::getEngine()->log("Main menu action \"%s\" not yet translated\n", theAction.c_str());
}

void FrontEnd::onOptionsAction(const std::string& theAction)
{
	if (theAction == "optionsDone")
		mSkyStack.pop(mOptionsMenu.get());
	else
		engine::getEngine()->log("Options action \"%s\" not yet translated\n", theAction.c_str());
}

void FrontEnd::onHighScoresAction(const std::string& theAction)
{
	if (theAction == "highscoresDone")
		mSkyStack.pop(mHighScores.get());
	else
		engine::getEngine()->log("High scores action \"%s\" not yet translated\n", theAction.c_str());
}

void FrontEnd::onCreditsAction(const std::string& theAction)
{
	if (theAction == "creditsdone")
		mSkyStack.pop(mCredits.get());
	else
		engine::getEngine()->log("Credits action \"%s\" not yet translated\n", theAction.c_str());
}

void FrontEnd::onQuitAction(const std::string& theAction)
{
	if (theAction == "quityes")
		engine::getEngine()->quit();
	else if (theAction == "quitno")
		mSkyStack.pop(mQuitConfirm.get());
	else
		engine::getEngine()->log("Quit action \"%s\" not yet translated\n", theAction.c_str());
}
