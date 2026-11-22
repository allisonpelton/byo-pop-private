#include "LoadingState.h"

#include "AtlantisApp.h"
#include "Engine.h"
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
	// Then the initial data set ("std") is started loading: not yet translated
}
