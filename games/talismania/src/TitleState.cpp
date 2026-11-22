#include "TitleState.h"

#include "AtlantisApp.h"
#include "Engine.h"
#include "Transition.h"

namespace
{

// The fades of the front-end screens (the float 0.3 0x3ecccccd the original passes at 0x403fe0 and after)
const float MENU_FADE_SECONDS = 0.4f;

} // namespace

void TitleState::enter()
{
	mMainMenu = new engine::GenericScreen();
	mMainMenu->load(mApp->mOptions.mainMenuLayout, mApp->mTheme.get());
	mMainMenu->setTransitions(new engine::FadeTransition(MENU_FADE_SECONDS), new engine::FadeTransition(MENU_FADE_SECONDS));
	engine::Container* aLayer = dynamic_cast<engine::Container*>(mApp->mScene->getLayer("skymenu"));
	if (aLayer != nullptr)
		aLayer->addChild(mMainMenu.get());
	mMainMenu->show();
}
