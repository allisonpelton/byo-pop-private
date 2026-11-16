#ifndef __FEEDINGFRENZY2_FEEDINGFRENZYTWO_H__
#define __FEEDINGFRENZY2_FEEDINGFRENZYTWO_H__

// The game's application (FeedingFrenzyTwo, 0x378 bytes; its factory 0x49e800 (feedingfrenzy2)), which the engine
// keeps (+0x288) and asks for its configuration (slot 4) and then to start (slot 8): the scene and its containers, the
// screens, the theme and the application's states. Translated so far: the archive of the configuration, and of the
// start the scene, the screen manager, the two named containers and the first state.

#include "Container.h"
#include "GenericScreen.h"
#include "Scene.h"
#include "State.h"
#include "Theme.h"
#include "sigslot.h"

class FeedingFrenzyTwo;
class ScreenMgr;

// A state of the application (FeedingFrenzyTwo::AppState, vtable 0x55a770 (feedingfrenzy2), through
// AbstractState<FeedingFrenzyTwo>, its constructor 0x48ca70 (feedingfrenzy2), which keeps the application)
class AppState : public engine::State
{
public:
	explicit AppState(FeedingFrenzyTwo* theApp) : mApp(theApp) {}

protected:
	FeedingFrenzyTwo* mApp;  // +0x0c
};

class FeedingFrenzyTwo : public engine::Object, public sigslot::HasSlots
{
public:
	FeedingFrenzyTwo();
	~FeedingFrenzyTwo() override;

	// Slot 4 (0x48c080 (feedingfrenzy2)): the archives read, not loose files. The string table
	// (resource/baseStrings.xml: the vendor and name), the options and the window's title, size and rate are not yet
	// translated (the port's host sets the window)
	void configure();
	// Slot 8 (0x408050 (feedingfrenzy2)): the frame's update connected, the scene made and enabled, the screen
	// manager and the containers made, and the splash state begun. The options read first, the key, minimize, restore
	// and activate slots, the music, the popups, the debug display and the screensaver are not yet translated
	void start(bool theSaverMode);

	// 0x45dbe0 (feedingfrenzy2): each frame, before the scene's update: the application's state updated (the
	// playing time and the sound's update are not yet translated)
	void updateSlot(engine::UpdateContext* theContext);

	engine::Ref<engine::Scene> mScene;                 // +0x20
	engine::Ref<ScreenMgr> mScreenMgr;                 // +0x24
	engine::Ref<engine::Container> mContainerSlot;     // +0x44: "ContainerSlot", in the scene
	engine::Ref<engine::Container> mStageContainer;    // +0x48: "StageContainer"
	engine::Ref<engine::GenericScreen> mSplashScreen;  // +0x54
	engine::Ref<engine::Theme> mTheme;                 // +0xb0
	engine::StateMgr<AppState> mStateMgr;              // +0x354
};

#endif
