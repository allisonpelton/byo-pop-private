#ifndef __FEEDINGFRENZY2_SPLASHSTATE_H__
#define __FEEDINGFRENZY2_SPLASHSTATE_H__

// The first of the application's states (FeedingFrenzyTwo::SplashState, 0x4c bytes, vtable 0x55a818 (feedingfrenzy2);
// its constructor 0x490650 (feedingfrenzy2)): the splash screen shown with a load bar and a bubbling "LOADING" while a
// thread loads the game, then the menu, the game or the screensaver. Translated so far: the screen made and shown, the
// load bar found and emptied, the theme loaded. The loader, the bubble text and the phases after the wait are not yet
// translated.

#include "FeedingFrenzyTwo.h"
#include "Image.h"

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
	// 0x0c (0x426f20 (feedingfrenzy2)): the wait before the loading counted down
	void update(engine::UpdateContext* theContext) override;

private:
	float mTimer;                        // +0x18: the seconds to wait, counted down
	engine::Ref<engine::Image> mLoadBar; // +0x1c: the screen's "loadBar"
	int mLoadBarWidth;                   // +0x20: its surface's
	int mLoadBarHeight;                  // +0x24
	// +0x28 the loader's thread, +0x2c the bubble text, +0x30 the phase, +0x34 the loader's abort flag, +0x38 and
	// +0x3c the progress loaded and shown: not yet translated
	bool mSaverMode;                     // +0x40: run as the screensaver
};

#endif
