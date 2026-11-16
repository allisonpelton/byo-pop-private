#include "SplashState.h"

#include "Engine.h"
#include "ScreenMgr.h"
#include "Theme.h"
#include "Transition.h"

using namespace engine;

namespace
{

// The splash screen's fade, in and out (the float at 0x5668b0 (feedingfrenzy2))
const float SPLASH_FADE_SECONDS = 0.25f;

} // namespace

SplashState::SplashState(FeedingFrenzyTwo* theApp, bool theSaverMode)
	: AppState(theApp), mTimer(0.0f), mLoadBarWidth(0), mLoadBarHeight(0), mSaverMode(false)
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

	// The bubble text (BubbleText, 0x128 bytes, its constructor 0x488190 (feedingfrenzy2)) at (400, 280) in the
	// theme's bubbleTextFont, bubbleTextBubbleSplash, bubbleTextFx and splashTextColor, saying "LOADING", added to the
	// screen, then 0x409dd0 (feedingfrenzy2): not yet translated
	getEngine()->log("SplashState: the bubble text not yet translated\n");
}

void SplashState::exit()
{
	mApp->mSplashScreen = nullptr;
}

void SplashState::update(UpdateContext* theContext)
{
	mTimer = (float)((double)mTimer - theContext->mElapsed);
	// The phases (waiting, loading, failed, loaded, bursting) and the next state: not yet translated
}
