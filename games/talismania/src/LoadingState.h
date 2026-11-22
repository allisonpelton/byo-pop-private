#ifndef __TALISMANIA_LOADINGSTATE_H__
#define __TALISMANIA_LOADINGSTATE_H__

// The application's states (AtlantisApp::AppState over engine::State) and the first of them, the loading state
// (LoadingState): it shows the loading screen (splash/loadingscreen.xml, a GenericScreen) in the scene's "skymenu" layer
// and starts loading the application options' initial data set. Translated so far: the entering (0x4060a0), without the
// data set's loading, the screen's later updates and the exit.

#include "GenericScreen.h"
#include "State.h"

class AtlantisApp;

// 0x4: the application is the first field of every state
class AppState : public engine::State
{
public:
	explicit AppState(AtlantisApp* theApp) : mApp(theApp) {}

protected:
	AtlantisApp* mApp;  // +0x04
};

class LoadingState : public AppState
{
public:
	explicit LoadingState(AtlantisApp* theApp) : AppState(theApp) {}

	// 0x4060a0 (slot 0)
	void enter() override;
	// 0x4064e0 (slot 0x10): the bar follows the data set's progress; a file is read each frame (the original reads on a
	// thread). When the set is done the screen's text says "Initializing Game" for a frame, then the screen is hidden and
	// the title state would follow (not yet translated)
	void update(engine::UpdateContext* theContext) override;
	// 0x4067d0 (slot 0x04): the screen let go
	void exit() override;

private:
	bool mInitializingShown = false;             // +0x08
	bool mLoaderStarted = false;
	engine::Ref<engine::GenericScreen> mScreen;  // +0x0c
};

#endif
