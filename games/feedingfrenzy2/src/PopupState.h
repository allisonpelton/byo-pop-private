#ifndef __FEEDINGFRENZY2_POPUPSTATE_H__
#define __FEEDINGFRENZY2_POPUPSTATE_H__

// A state put over another for a popup screen (FeedingFrenzyTwo::PopupState, 0x9c bytes, vtable 0x55a900
// (feedingfrenzy2); its constructor 0x48a9c0 (feedingfrenzy2)). Its screens go into the popups' own screen manager,
// put in the scene over the game's; the screens under it stop taking the mouse and fade to the shell theme's
// popupDarkenColor over the first screen's transition in. Closing hides the screen, fades the colour back and, once
// the popups' manager has settled, puts back the state it was pushed over (or changes to the next one, when one is
// set). Its actions are the popups' buttons: the player's name entry, the players' list, the confirmations, pause,
// the options and the game's exit.

#include "FeedingFrenzyTwo.h"
#include "KeyInterpolator.h"
#include "TimeDriver.h"

class PopupState : public AppState
{
public:
	// 0x48a9c0 (feedingfrenzy2): the popups' manager made by the first popup; the screen to show and the state to put
	// back, no next state
	PopupState(FeedingFrenzyTwo* theApp, engine::GenericScreen* theScreen, AppState* thePreviousState);

	// 0x04 (0x4173c0 (feedingfrenzy2)): the game's screens darkened and stopped, the popups' manager put in the scene
	// and the screen shown
	void enter() override;
	// 0x08 (0x488330 (feedingfrenzy2)): the game's screens given their colour back
	void exit() override;
	// 0x0c (0x475190 (feedingfrenzy2)): the fade, and once closed and settled the state put back
	void update(engine::UpdateContext* theContext) override;
	// 0x10 (0x414e50 (feedingfrenzy2)): the popups' buttons
	void action(const std::string& theAction) override;
	// (0x14, 0x18 and 0x20 do nothing. 0x1c (0x49b710 (feedingfrenzy2)), the F key switching between the window and
	// the full screen unless the name entry is up, is called by the application's key slot, not yet translated; so is
	// 0x28 (0x50a0ab (feedingfrenzy2)), the popup taken away at once)

	// 0x24 (0x490ea0 (feedingfrenzy2)): the screen hidden, and the state closing
	virtual void close();
	// 0x2c (0x48d830 (feedingfrenzy2)): the screen shown in the popups' manager, in place of the one there
	virtual void show(engine::GenericScreen* theScreen);
	// 0x494340 (feedingfrenzy2): the name entry shown empty, its Cancel button there only while there are players
	void showUserEntry();

	// The shutdown's (0x495650 (feedingfrenzy2)) release of the popups' manager
	static void releaseScreenMgr();

private:
	// 0x473e90 (feedingfrenzy2): the clock moved on (back, while closing) and the game's screens given the colour
	void fade(engine::UpdateContext* theContext);

	engine::Ref<engine::GenericScreen> mScreen;     // +0x10
	engine::Ref<AppState> mPreviousState;           // +0x14
	engine::Ref<AppState> mNextState;               // +0x18
	bool mClosing;                                  // +0x1c
	bool mOptionsFromPause;                         // +0x1d: the options shown from the pause (pauseOptions, not yet
	                                                // translated), so pauseResume goes back to the pause; cleared by
	                                                // enter
	engine::ColorKeyInterpolator mDarken;           // +0x20: from white at 0 to the darkening colour at 1
	engine::TimeDriver mDarkenTime;                 // +0x6c
};

#endif
