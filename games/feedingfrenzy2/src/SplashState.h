#ifndef __FEEDINGFRENZY2_SPLASHSTATE_H__
#define __FEEDINGFRENZY2_SPLASHSTATE_H__

// The first of the application's states (FeedingFrenzyTwo::SplashState, 0x4c bytes, vtable 0x55a818 (feedingfrenzy2);
// its constructor 0x490650 (feedingfrenzy2)): the splash screen shown with a load bar and a bubbling "LOADING" while a
// thread loads the game, then the menu, the game or the screensaver. Translated so far: the screen made and shown, the
// load bar found and emptied, the theme loaded, the bubble text shown. The loader and the phases after the wait are
// not yet translated.

#include "BubbleText.h"
#include "FeedingFrenzyTwo.h"
#include "Image.h"

// Where the splash is (+0x30)
enum SplashPhase
{
	SPLASH_WAITING = 0,    // for its screen to settle and the timer to run out
	SPLASH_LOADING = 1,    // the loader running, the bar following its progress
	SPLASH_FAILED = 2,     // the loader's thread could not start: the program ends
	SPLASH_LOADED = 3,     // set by the loader at its end
	SPLASH_BURSTING = 4    // the bubbles bursting, then the next state
};

class SplashState : public AppState
{
public:
	// The splash screen made (once: it is kept in the application until this state leaves) from the disk's
	// splash/splashScreen.xml, with a fade of a quarter second in and out
	SplashState(FeedingFrenzyTwo* theApp, bool theSaverMode);

	// 0x04 (0x409540 (feedingfrenzy2))
	void enter() override;
	// 0x08 (0x4a0890 (feedingfrenzy2)): the application's splash screen let go
	void exit() override;
	// 0x0c (0x426f20 (feedingfrenzy2)): the wait, the load bar, the burst and the next state
	void update(engine::UpdateContext* theContext) override;

private:
	// 0x499300 (feedingfrenzy2): the bar emptied and the loader started
	void startLoading();
	// 0x47e450 (feedingfrenzy2), run by the loader's thread (0x49f2c0 (feedingfrenzy2)): the game's data, stages and
	// screens loaded, the progress set after each part; the phase set to loaded at the end
	void load();
	// A plain screen of the shell's, as the loader makes each (inlined): read with the theme (none for null), given
	// its transitions and its actions passed to the application
	engine::GenericScreen* loadShellScreen(const char* thePath, engine::Theme* theTheme, engine::Transition* theIn,
	                                       engine::Transition* theOut);

	float mTimer;                        // +0x18: the seconds to wait, counted down
	engine::Ref<engine::Image> mLoadBar; // +0x1c: the screen's "loadBar"
	int mLoadBarWidth;                   // +0x20: its surface's
	int mLoadBarHeight;                  // +0x24
	// +0x28: the loader's thread (the port loads at once, without one)
	engine::Ref<BubbleText> mBubbleText; // +0x2c
	int mPhase;                          // +0x30: SplashPhase
	bool mAbortLoading;                  // +0x34: set when the state leaves while loading (not yet translated)
	float mLoadProgress;                 // +0x38: 0 to 1, set by the loader
	float mShownProgress;                // +0x3c: the bar's, following the loader's by a hundredth a frame
	bool mSaverMode;                     // +0x40: run as the screensaver
	bool mNextStateLogged;               // port: the next state not yet translated, logged once
};

#endif
