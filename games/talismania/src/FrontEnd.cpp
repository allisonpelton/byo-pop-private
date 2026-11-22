#include "FrontEnd.h"

#include "AtlantisApp.h"
#include "Engine.h"
#include "Transition.h"

namespace
{

// The fades of the front-end screens (the float 0.3 0x3ecccccd the original passes at 0x403fe0 and after)
const float MENU_FADE_SECONDS = 0.4f;

// The credits' layout (a string of 0x4039b0)
const char* const SKY_BACKGROUND_LAYOUT = "config/screens/skybg.xml";

const char* const CREDITS_LAYOUT = "config/screens/menus/storyCredits.xml";

// A rectangle over the whole screen that tells its owner when either mouse button goes down in it. The credits screen has
// no button, and the original's mouse handler (its strings "leftmousedown" and "rightmousedown") ends the credits
class ClickCatcher : public engine::Component
{
public:
	ClickCatcher()
	{
		addOwnState(engine::STATE_VISIBLE);
		setTakesInput(true);
		computeBounds();
	}
	std::string getClassName() override { return "ClickCatcher"; }
	void paint(engine::DrawContext*) override {}
	void computeBounds() override { mBounds = engine::RectF(0, 0, 800, 600); }
	void mouseDown(engine::MouseEvent&) override { mClicked.emit(); }
	void rightDown(engine::MouseEvent&) override { mClicked.emit(); }

	sigslot::signal0 mClicked;
};

} // namespace

// 0x4039b0 (the stacks and the screens): the sky stack's layers by their names
FrontEnd::FrontEnd(AtlantisApp* theApp)
	: mApp(theApp), mSkyStack(theApp->mScene.get(), "skymenu", "skymenupopup", "darken_sky", "darken_skypopup")
{
	// The sky's own background, in the scene's "skybg" layer under every screen (config/screens/skybg.xml)
	engine::GenericScreen* aSkyBackground = new engine::GenericScreen();
	aSkyBackground->load(SKY_BACKGROUND_LAYOUT, mApp->mTheme.get());
	engine::Container* aSkyLayer = dynamic_cast<engine::Container*>(mApp->mScene->getLayer("skybg"));
	if (aSkyLayer != nullptr)
	{
		aSkyLayer->addChild(aSkyBackground);
		aSkyBackground->show();
	}
	mMainMenu = makeScreen(mApp->mOptions.mainMenuLayout);
	mOptionsMenu = makeScreen(mApp->mOptions.optionsMenuLayout);
	mHighScores = makeScreen(mApp->mOptions.highScoreMenuLayout);
	mCredits = makeScreen(CREDITS_LAYOUT);
	ClickCatcher* aCatcher = new ClickCatcher();
	mCredits->addChild(aCatcher);
	aCatcher->mClicked.connect(this, &FrontEnd::onCreditsClicked);
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
	{
		// 0x407ab4: the main menu leaves and the credits take its place
		mSkyStack.pop(mMainMenu.get());
		mSkyStack.push(mCredits.get());
	}
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

void FrontEnd::onCreditsClicked()
{
	if (!mSkyStack.contains(mCredits.get()))
		return;
	mSkyStack.pop(mCredits.get());
	mSkyStack.push(mMainMenu.get());
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
