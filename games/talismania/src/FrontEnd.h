#ifndef __TALISMANIA_FRONTEND_H__
#define __TALISMANIA_FRONTEND_H__

// The front-end screens 0x4039b0 makes and the action handler 0x407510 that answers them: the main menu, the options,
// the high scores, the credits and the quit confirmation, each a GenericScreen pushed on the sky menu stack and popped
// by its own actions. Translated so far: those screens and their buttons' navigation. Not yet: the starting of a game
// (startstorymode, startheromode), the user management and entry screens, the new game question, the in-game menus,
// the options' controls and the high score tables' contents.

#include "GenericScreen.h"
#include "MenuStack.h"
#include "sigslot.h"

class AtlantisApp;

class FrontEnd : public sigslot::HasSlots
{
public:
	explicit FrontEnd(AtlantisApp* theApp);

	// The main menu pushed on the sky stack (the title state's entering)
	void showMainMenu();
	// 0x443690: the main menu taken off the stack when a game begins
	void hideMainMenu() { mSkyStack.pop(mMainMenu.get()); }
	// Called each frame with the seconds elapsed
	void update(double theElapsed) { mSkyStack.update(theElapsed); }

private:
	engine::GenericScreen* makeScreen(const std::string& theLayout);
	void onMainMenuAction(const std::string& theAction);
	void onOptionsAction(const std::string& theAction);
	void onHighScoresAction(const std::string& theAction);
	void onCreditsClicked();
	void startGame(const std::string& theModeName);
	void onQuitAction(const std::string& theAction);

	AtlantisApp* mApp;
	MenuStack mSkyStack;
	engine::Ref<engine::GenericScreen> mMainMenu;
	engine::Ref<engine::GenericScreen> mOptionsMenu;
	engine::Ref<engine::GenericScreen> mHighScores;
	engine::Ref<engine::GenericScreen> mCredits;
	engine::Ref<engine::GenericScreen> mQuitConfirm;
};

#endif
