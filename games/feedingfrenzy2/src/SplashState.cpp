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
	// The phases (waiting, loading, failed, loaded, bursting) and the next state: not yet translated
}
