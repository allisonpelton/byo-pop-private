#ifndef __FEEDINGFRENZY2_FEEDINGFRENZYTWO_H__
#define __FEEDINGFRENZY2_FEEDINGFRENZYTWO_H__

// The game's application (FeedingFrenzyTwo, 0x378 bytes; its factory 0x49e800 (feedingfrenzy2)), which the engine
// keeps (+0x288) and asks for its configuration (slot 4) and then to start (slot 8): the scene and its containers, the
// screens, the theme and the application's states. Translated so far: the archive of the configuration, and of the
// start the scene, the screen manager, the two named containers and the first state.

#include "Container.h"
#include "FunFacts.h"
#include "GameData.h"
#include "GenericScreen.h"
#include "MainMenu.h"
#include "PlayerRanks.h"
#include "Scene.h"
#include "StageScreenData.h"
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
	// 0x48b740 (feedingfrenzy2), connected to the shell's screens' action signals: passed to the current state
	void actionSlot(const std::string& theAction) { mStateMgr.action(theAction); }

	engine::Ref<engine::Scene> mScene;                 // +0x20
	engine::Ref<ScreenMgr> mScreenMgr;                 // +0x24
	engine::Ref<GameData> mGameData;                   // +0x3c: made by the splash's loader
	engine::Ref<MainMenu> mMainMenu;                   // +0x28: made by the splash's loader
	// +0x2c: the debug level select (DebugSelectMenu, resource/shell/debugLevelSelect.xml, a list of the game's
	// stages), made by the loader: not yet translated (only the main menu's debugSelect button, hidden and
	// switched off, has been seen to lead to it)
	engine::Ref<engine::Container> mContainerSlot;     // +0x44: "ContainerSlot", in the scene
	engine::Ref<engine::Container> mStageContainer;    // +0x48: "StageContainer"
	engine::Ref<engine::GenericScreen> mSplashScreen;  // +0x54
	// The shell's plain screens, made by the splash's loader
	engine::Ref<engine::GenericScreen> mPauseScreen;             // +0x58
	engine::Ref<engine::GenericScreen> mUserEntry;               // +0x5c: a new player's name
	// +0x60: the players' list (GenericUserSelectScreen, 0x1b4 bytes, its constructor 0x49fc30 (feedingfrenzy2),
	// resource/shell/userSelect.xml): not yet translated
	engine::Ref<engine::GenericScreen> mUserDeleteConfirm;       // +0x64
	engine::Ref<engine::GenericScreen> mAppQuitConfirm;          // +0x68
	engine::Ref<engine::GenericScreen> mUserDuplicateConfirm;    // +0x6c
	engine::Ref<engine::GenericScreen> mUserListFullConfirm;     // +0x70
	engine::Ref<engine::GenericScreen> mResetHighScoresConfirm;  // +0x74
	engine::Ref<engine::GenericScreen> mGameExitConfirm;         // +0x78
	engine::Ref<engine::GenericScreen> mRestoreGameScreen;       // +0x7c
	engine::Ref<engine::GenericScreen> mGenericLoadScreen;       // +0x80
	// +0x84: the map (GameMapScreen): not yet translated
	engine::Ref<engine::GenericScreen> mCredits;                 // +0x88
	// +0x8c to +0xa4: the high scores (HighScoreScreen), the two game overs (StageCompleteScreenBonus), the stage's
	// start (StageIntroScreen), its two ends (StageCompleteScreenNormal and StageCompleteScreenBonus) and the options
	// (FeedingFrenzyTwoOptionScreen): not yet translated
	engine::Ref<engine::Theme> mShellTheme;            // +0xac: resource/themes/shellTheme.xml, falling back on the
	                                                   // minimal theme
	engine::Ref<engine::Theme> mTheme;                 // +0xb0: the minimal theme, made by the splash
	engine::Ref<StageScreenData> mStageScreenData;     // +0xbc
	engine::Ref<FunFacts> mFunFacts;                   // +0xc0
	engine::Ref<PlayerRanks> mPlayerRanks;             // +0xc4
	engine::StateMgr<AppState> mStateMgr;              // +0x354
};

// The application (0x5ac624 (feedingfrenzy2)), set by its constructor
FeedingFrenzyTwo* getApp();

#endif
