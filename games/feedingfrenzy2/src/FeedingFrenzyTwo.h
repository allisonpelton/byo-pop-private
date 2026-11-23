#ifndef __FEEDINGFRENZY2_FEEDINGFRENZYTWO_H__
#define __FEEDINGFRENZY2_FEEDINGFRENZYTWO_H__

// The game's application (FeedingFrenzyTwo, 0x378 bytes; its factory 0x49e800 (feedingfrenzy2)), which the engine
// keeps (+0x288) and asks for its configuration (slot 4) and then to start (slot 8): the scene and its containers, the
// screens, the theme and the application's states. Translated so far: the archive of the configuration, and of the
// start the scene, the screen manager, the two named containers and the first state.

#include "Container.h"
#include "FunFacts.h"
#include "GameData.h"
#include "GameMapScreen.h"
#include "GenericScreen.h"
#include "GenericUserSelectScreen.h"
#include "MainMenu.h"
#include "PlayerRanks.h"
#include "Scene.h"
#include "Stage.h"
#include "StageCompleteScreen.h"
#include "StageIntroScreen.h"
#include "StageScreenData.h"
#include "State.h"
#include "StringTable.h"
#include "Theme.h"
#include "UserData.h"
#include "sigslot.h"

class FeedingFrenzyTwo;
class ScreenMgr;

// A state of the application (FeedingFrenzyTwo::AppState, vtable 0x55a770 (feedingfrenzy2), through
// AbstractState<FeedingFrenzyTwo>, its constructor 0x48ca70 (feedingfrenzy2), which keeps the application)
class AppState : public engine::State
{
public:
	explicit AppState(FeedingFrenzyTwo* theApp) : mApp(theApp) {}

	// 0x1c: a key pressed, passed on by the application's key slot; AppState's own does nothing
	virtual void keyDown(int theKey) {}

protected:
	FeedingFrenzyTwo* mApp;  // +0x0c
};

// The screen the menu shows on entering (the application's +0x330), set by what sends the player to it
enum MenuScreen
{
	MENU_SCREEN_MAIN = 0,
	MENU_SCREEN_HIGH_SCORES = 1,
	MENU_SCREEN_MAP = 2
};

class FeedingFrenzyTwo : public engine::Object, public sigslot::HasSlots
{
public:
	FeedingFrenzyTwo();
	~FeedingFrenzyTwo() override;

	// Slot 4 (0x48c080 (feedingfrenzy2)): the archives read, not loose files, then the string table and from it the
	// vendor and the name. The options and the window's title, size and rate are not yet translated (the port's host
	// sets the window)
	void configure();
	// Slot 8 (0x408050 (feedingfrenzy2)): the frame's update connected, the scene made and enabled, the screen
	// manager and the containers made, the key-down and activate slots connected, and the splash state begun. The
	// options read first, the key-up, minimize and restore slots, the music, the popups, the debug display and the
	// screensaver are not yet translated
	void start(bool theSaverMode);
	// 0x4a1a70 (feedingfrenzy2), from the start: the sound system made over the window and given to the engine (its
	// slot 0xfc), and a line logged on whether it started
	void startSound();

	// 0x45dbe0 (feedingfrenzy2): each frame, before the scene's update: the application's state updated (the
	// playing time and the sound's update are not yet translated)
	void updateSlot(engine::UpdateContext* theContext);
	// 0x48b740 (feedingfrenzy2), connected to the shell's screens' action signals: passed to the current state
	void actionSlot(const std::string& theAction) { mStateMgr.action(theAction); }
	// 0x4932a0 (feedingfrenzy2), connected to the engine's key-down signal: passed to the current state; F11 then
	// saves a screenshot (the engine's slot 0xe0, "ff2snap"), not yet translated
	void keyDownSlot(int theKey);
	// 0x48b520 (feedingfrenzy2), connected to the engine's activate signal: the focus lost pauses the play (the
	// application's sound at +0x38 paused, and resumed when it comes back, the options' +0xb8 told and the engine's
	// global 0x5ac648's slot 0x34 called: not yet translated), and whether the window has it is kept
	void activateSlot(bool theActive);
	// 0x40c0b0 (feedingfrenzy2): the stage's intro screen's data, the timed mode's own ("<id>_timed") first in that
	// mode; null for none
	ScreenDataEntry* getScreenData(const std::string& theStageId);

	engine::Ref<engine::Scene> mScene;                 // +0x20
	engine::Ref<ScreenMgr> mScreenMgr;                 // +0x24
	engine::Ref<GameData> mGameData;                   // +0x3c: made by the splash's loader
	engine::Ref<MainMenu> mMainMenu;                   // +0x28: made by the splash's loader
	// +0x2c: the debug level select (DebugSelectMenu, resource/shell/debugLevelSelect.xml, a list of the game's
	// stages), made by the loader: not yet translated (only the main menu's debugSelect button, hidden and
	// switched off, has been seen to lead to it)
	engine::Ref<Stage> mStage;                         // +0x40: made by the splash's loader
	engine::Ref<engine::Container> mContainerSlot;     // +0x44: "ContainerSlot", in the scene
	engine::Ref<engine::Container> mStageContainer;    // +0x48: "StageContainer"
	engine::Ref<StringTable> mStrings;                 // +0x4c: resource/baseStrings.xml, read by configure
	engine::Ref<engine::GenericScreen> mSplashScreen;  // +0x54
	// The shell's plain screens, made by the splash's loader
	engine::Ref<engine::GenericScreen> mPauseScreen;             // +0x58
	engine::Ref<engine::GenericScreen> mUserEntry;               // +0x5c: a new player's name
	engine::Ref<engine::GenericUserSelectScreen> mUserSelect;   // +0x60: the players' list
	engine::Ref<engine::GenericScreen> mUserDeleteConfirm;       // +0x64
	engine::Ref<engine::GenericScreen> mAppQuitConfirm;          // +0x68
	engine::Ref<engine::GenericScreen> mUserDuplicateConfirm;    // +0x6c
	engine::Ref<engine::GenericScreen> mUserListFullConfirm;     // +0x70
	engine::Ref<engine::GenericScreen> mResetHighScoresConfirm;  // +0x74
	engine::Ref<engine::GenericScreen> mGameExitConfirm;         // +0x78
	engine::Ref<engine::GenericScreen> mRestoreGameScreen;       // +0x7c
	engine::Ref<engine::GenericScreen> mGenericLoadScreen;       // +0x80
	engine::Ref<GameMapScreen> mGameMap;                         // +0x84
	engine::Ref<engine::GenericScreen> mCredits;                 // +0x88
	// +0x8c: the high scores (HighScoreScreen): not yet translated
	// +0x90 and +0x94: the continue offer and the game over, each made as the bonus stage's end
	// (StageCompleteScreenBonus, its constructor 0x4969a0 (feedingfrenzy2)) but read by the plain layout's load, so
	// without the splats and the effects' group; with no count running, its press of the mouse does nothing, and a
	// plain screen behaves the same
	engine::Ref<engine::GenericScreen> mGameOverContinue;        // +0x90: resource/shell/gameOverContinuesLeft.xml
	engine::Ref<engine::GenericScreen> mGameOver;                // +0x94: resource/shell/gameOverNoContinuesLeft.xml
	engine::Ref<StageIntroScreen> mStageIntro;                   // +0x98
	engine::Ref<StageCompleteScreenNormal> mStageCompleteNormal; // +0x9c
	engine::Ref<StageCompleteScreenBonus> mStageCompleteBonus;   // +0xa0
	// +0xa4: the options (FeedingFrenzyTwoOptionScreen): not yet translated
	engine::Ref<engine::Theme> mShellTheme;            // +0xac: resource/themes/shellTheme.xml, falling back on the
	                                                   // minimal theme
	engine::Ref<engine::Theme> mTheme;                 // +0xb0: the minimal theme, made by the splash
	engine::Ref<UserData> mUserData;                   // +0xb4: made by the splash's loader
	engine::Ref<StageScreenData> mStageScreenData;     // +0xbc
	engine::Ref<FunFacts> mFunFacts;                   // +0xc0
	engine::Ref<PlayerRanks> mPlayerRanks;             // +0xc4
	std::string mVendor;                               // +0xc8: the string table's vendor
	std::string mAppName;                              // +0xe4: the string table's appName
	bool mMenuRunning;                                 // +0x320: while the menu is the current state, so its stage
	                                                   // goes on under a popup
	bool mActive;                                      // +0x321: the window has the focus (true until told)
	bool mStageInPlay;                                 // +0x323: set as the play begins and comes back from a popup,
	                                                   // cleared as the stage ends: the quit popup's gameExitSave
	                                                   // reads it (not yet translated)
	bool mFreshGame;                                   // +0x324: a game started anew (not one restored), until its
	                                                   // first stage is complete: the stage intro shows the clock's
	                                                   // starting total
	int mMenuScreen;                                   // +0x330: MenuScreen
	engine::StateMgr<AppState> mStateMgr;              // +0x354
};

// The application (0x5ac624 (feedingfrenzy2)), set by its constructor
FeedingFrenzyTwo* getApp();

#endif
