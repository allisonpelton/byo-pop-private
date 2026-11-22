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
	// The registration badge and the buttons that lead out of the game (the trial badge, the page of reasons to buy and
	// the page of more games) are removed, not hidden
	static const char* const REMOVED[] = {"trialbadge", "whybuybtn", "moregamesbtn"};
	for (size_t i = 0; i < sizeof(REMOVED) / sizeof(REMOVED[0]); i++)
	{
		engine::Component* aRemoved = mMainMenu->findComponent(REMOVED[i]);
		if (aRemoved != nullptr && aRemoved->getParent() != nullptr)
			aRemoved->getParent()->removeChild(aRemoved);
	}
	mMainMenu->setTransitions(new engine::FadeTransition(MENU_FADE_SECONDS), new engine::FadeTransition(MENU_FADE_SECONDS));
	engine::Container* aLayer = dynamic_cast<engine::Container*>(mApp->mScene->getLayer("skymenu"));
	if (aLayer != nullptr)
		aLayer->addChild(mMainMenu.get());
	mMainMenu->show();
}
