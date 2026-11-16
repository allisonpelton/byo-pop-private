#include "SplashState.h"

#include "DrawContext.h"
#include "Engine.h"
#include "ScreenMgr.h"
#include "Theme.h"
#include "Transition.h"

using namespace engine;

namespace
{

// The splash screen's fade, in and out (the float at 0x5668b0 (feedingfrenzy2))
const float SPLASH_FADE_SECONDS = 0.25f;
// Where the "LOADING" is centred, and the pixels between its bubbles (the floats at 0x566b4c, 0x566c10 and 0x54dd6c
// (feedingfrenzy2))
const float BUBBLE_TEXT_X = 400.0f;
const float BUBBLE_TEXT_Y = 280.0f;
const float BUBBLE_TEXT_SPACING = 8.0f;
// The frames per second while loading and after
const int LOADING_FRAME_RATE = 30;
const int GAME_FRAME_RATE = 60;
// The most the bar moves a frame towards the loader's progress (the double at 0x566d58 (feedingfrenzy2), the float's
// value)
const float LOAD_BAR_STEP = 0.01f;

} // namespace

SplashState::SplashState(FeedingFrenzyTwo* theApp, bool theSaverMode)
	: AppState(theApp), mTimer(0.0f), mLoadBarWidth(0), mLoadBarHeight(0), mPhase(SPLASH_WAITING),
	  mAbortLoading(false), mLoadProgress(0.0f), mShownProgress(0.0f), mSaverMode(false), mNextStateLogged(false)
{
	if (mApp->mSplashScreen == nullptr)
	{
		Engine* anEngine = getEngine();
		anEngine->setUseLooseFiles(true);
		FadeTransition* aFade = new FadeTransition(SPLASH_FADE_SECONDS);
		mApp->mSplashScreen = new GenericScreen();
		mApp->mSplashScreen->load("splash/splashScreen.xml", mApp->mTheme.get());
		mApp->mSplashScreen->setTransitions(aFade, aFade);
		anEngine->setUseLooseFiles(false);
	}
	mSaverMode = theSaverMode;
}

void SplashState::enter()
{
	mTimer = 0.0f;
	mLoadBar = dynamic_cast<Image*>(mApp->mSplashScreen->findComponent("loadBar"));
	mLoadBar->computeBounds();
	mLoadBarHeight = mLoadBar->getHeight();
	mLoadBarWidth = mLoadBar->getWidth();
	mLoadBar->setSource(RectI(0, 0, 0, mLoadBarHeight));

	mApp->mTheme = new Theme();
	mApp->mTheme->load("resource/themes/minimalTheme.xml");
	mApp->mScreenMgr->showScreen(mApp->mSplashScreen.get(), 0.0f);

	mBubbleText = new BubbleText();
	mBubbleText->setPosition(BUBBLE_TEXT_X, BUBBLE_TEXT_Y);
	mBubbleText->setFont(mApp->mTheme->getFont("bubbleTextFont"));
	mBubbleText->setBubble(mApp->mTheme->getImage("bubbleTextBubbleSplash"));
	mBubbleText->setEffect(mApp->mTheme->getParticleDef("bubbleTextFx"));
	mBubbleText->setLetterColor(mApp->mTheme->getColor("splashTextColor"));
	mBubbleText->setSpacing(BUBBLE_TEXT_SPACING);
	mBubbleText->setFilterMode(FILTER_MODE_SMOOTH);
	mBubbleText->setText("LOADING");
	mApp->mSplashScreen->addChild(mBubbleText);
	mBubbleText->appear(0.0f, 1.0f);
}

void SplashState::exit()
{
	mApp->mSplashScreen = nullptr;
}

void SplashState::update(UpdateContext* theContext)
{
	mTimer = (float)((double)mTimer - theContext->mElapsed);
	switch (mPhase)
	{
	case SPLASH_WAITING:
		if (mApp->mScreenMgr->isBusy())
			return;
		if (!(mTimer < 0.0f))
			return;
		getEngine()->setFrameRate(LOADING_FRAME_RATE);
		startLoading();
		return;
	case SPLASH_LOADING:
		if (mLoadBar == nullptr)
			return;
		if (1.0f <= mLoadProgress)
			mShownProgress = 1.0f;
		else if (mShownProgress < mLoadProgress)
			mShownProgress = (float)(mShownProgress + (double)LOAD_BAR_STEP);
		// _ftol2 truncates
		mLoadBar->setSource(RectI(0, 0, (int)((double)mLoadBarWidth * mShownProgress), mLoadBarHeight));
		return;
	case SPLASH_FAILED:
		getEngine()->quit();
		return;
	case SPLASH_LOADED:
		mLoadBar->setSource(RectI(0, 0, mLoadBarWidth, mLoadBarHeight));
		getEngine()->setFrameRate(GAME_FRAME_RATE);
		mBubbleText->burst();
		// 0x494820 (feedingfrenzy2): the stage shown under the screens
		if (mApp->mStageContainer != nullptr)
			mApp->mContainerSlot->addChild(mApp->mStageContainer);
		// The burst's sound, "bubbleTextBurst" at full volume and normal pitch (the game's sounds, 0x4747a0
		// (feedingfrenzy2)), and the sound system's update (0x434c80 (feedingfrenzy2)): not yet translated
		getEngine()->log("SplashState: the sound bubbleTextBurst not yet translated\n");
		mPhase = SPLASH_BURSTING;
		// (on into the bursting phase in the same update)
	case SPLASH_BURSTING:
		// The sound system's update by the frame's milliseconds (0x434c80 (feedingfrenzy2)) and the game's in
		// hundredth-second steps (0x43a570 (feedingfrenzy2)): not yet translated
		if (!mBubbleText->isDone())
			return;
		// The next state: the screensaver's (SaverState) in saver mode, else the menu (MenuState) or, when the
		// configuration names a stage to start in (the application's +0x338), the game (GamePlayState): not yet
		// translated, so the splash stays
		if (!mNextStateLogged)
			getEngine()->log("SplashState: the next state not yet translated\n");
		mNextStateLogged = true;
		return;
	}
}

void SplashState::startLoading()
{
	mShownProgress = 0.0f;
	// The original makes a thread (the engine's slot 0xc4, at a priority of 2) to run the loader, which sets the
	// phase to loading as it begins; the phase is set to failed when the thread does not start. The splash goes on
	// drawing while the loader runs. The port loads at once, at the machine's speed, so the bar fills in a frame
	mPhase = SPLASH_LOADING;
	load();
}

void SplashState::load()
{
	mLoadProgress = 0.0f;
	mApp->mGameData = new GameData();
	// The command line's stages= and menuStages= (the engine's table of arguments, its slot 0x38, read by 0x402970
	// (feedingfrenzy2)) would name other manifests: not yet translated
	mApp->mGameData->getStages().load("resource/Stages/stageManifest.xml");
	mApp->mGameData->getMenuStages().load(
		mSaverMode ? "resource/Stages/saverManifest.xml" : "resource/Stages/menuManifest.xml");
	// (The original checks after each part whether the state has left, then sets the phase to failed and stops; the
	// port loads at once, so it cannot have)
	// The stages' screen data, fun facts, player ranks, the fish, the menus, the sounds, the stage, the users' data
	// and every screen of the shell: not yet translated
	getEngine()->log("SplashState: the loader not yet translated past the manifests\n");
	mLoadProgress = 1.0f;
	mPhase = SPLASH_LOADED;
}
