#include "LoadingState.h"

#include "AtlantisApp.h"
#include "Engine.h"
#include "TitleState.h"
#include "Transition.h"

namespace
{

// The loading screen's fade out (the float 1.0 the original passes at 0x4060a0)
const float LOADING_FADE_OUT_SECONDS = 1.0f;

} // namespace

// 0x4060a0
void LoadingState::enter()
{
	engine::Engine* anEngine = engine::getEngine();
	// The screen and its few images are loose files, read with loose files switched on while the archive is in use
	if (gUsingArchive)
		anEngine->setUseLooseFiles(true);
	mScreen = new engine::GenericScreen();
	mScreen->load(mApp->mOptions.loadingScreenLayout);
	// Without the distributor's logo among the files the screen shows the three-logo group and not the four
	if (!anEngine->fileExists("splash/distributor.png"))
	{
		engine::Component* aThreeLogos = mScreen->findComponent("3LogoGrp");
		if (aThreeLogos != nullptr)
			aThreeLogos->setOpacity(1.0f);
		engine::Component* aFourLogos = mScreen->findComponent("4LogoGrp");
		if (aFourLogos != nullptr)
			aFourLogos->setOpacity(0.0f);
	}
	if (gUsingArchive)
		anEngine->setUseLooseFiles(false);
	mScreen->setTransitions(nullptr, new engine::FadeTransition(LOADING_FADE_OUT_SECONDS));
	// The original sets the flag 0x1000 on the screen here: not yet translated
	engine::Container* aLayer = dynamic_cast<engine::Container*>(mApp->mScene->getLayer("skymenu"));
	if (aLayer != nullptr)
		aLayer->addChild(mScreen.get());
	mScreen->show();
	// The initial data set is read a file a frame by the update; the original reads it on a thread. The kinds of data
	// not yet translated are logged as they are met
	mApp->mData.beginDataSet(mApp->mOptions.initialDataSet);
}

// 0x4064e0
void LoadingState::update(engine::UpdateContext* theContext)
{
	AtlantisData& aData = mApp->mData;
	if (aData.isDataSetDone())
	{
		if (!mInitializingShown)
		{
			// The screen's text (a component of the loading screen) is set to "Initializing Game": not yet translated
			mInitializingShown = true;
			return;
		}
		// The original hides the loading screen and enters the title state at once (0x4039b0 first makes the front end's
		// screens)
		mScreen->hide();
		mApp->mStateMgr.changeState(new TitleState(mApp));
		return;
	}
	engine::Component* aBar = mScreen->findComponent("loadbar");
	aData.loadNextFile();
	if (aBar != nullptr)
		aBar->setScale(aData.getDataSetProgress(), 1.0f);
}

// 0x4067d0
void LoadingState::exit()
{
	mScreen = nullptr;
}
