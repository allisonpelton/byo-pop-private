#include "MainMenu.h"

#include "FeedingFrenzyTwo.h"
#include "Transition.h"

using namespace engine;

namespace
{

// The fades in and out (the float at 0x5668b0 (feedingfrenzy2))
const float MAIN_MENU_FADE_SECONDS = 0.25f;

} // namespace

void MainMenu::init()
{
	load("resource/shell/mainMenu.xml", getApp()->mShellTheme.get());
	FadeTransition* aFade = new FadeTransition(MAIN_MENU_FADE_SECONDS);
	setTransitions(aFade, aFade);
	Component* aDebugButton = findComponent("debugSelect");
	if (aDebugButton != nullptr)
	{
		aDebugButton->setTakesInput(false);
		aDebugButton->setVisible(false);
	}
	// The link button (gameLinkBtn) is shown in the original when the options (the application's +0xb8, its +0x58)
	// name a link of the distributor's, with the images "<the link's prefix>Normal.png" and "High.png"; else it is
	// hidden and switched off, as here. The options are not yet translated, and the button leads out of the game, so
	// is to be taken out
	Component* aLinkButton = findComponent("gameLinkBtn");
	aLinkButton->setVisible(false);
	aLinkButton->setTakesInput(false);
}
