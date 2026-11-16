#ifndef __FEEDINGFRENZY_FISHFOOD_H__
#define __FEEDINGFRENZY_FISHFOOD_H__

// The game's controller, FishFood (0x260 bytes, constructor 0x430ee0, vtable 0x4a6640). It is the application's
// controller (engine::App's +0x188): the run loop asks it for the window's configuration (configure, 0x42b8e0), then
// has it set the game up (init, 0x430b80). The controller keeps a state (+0x54) the application's update signal drives
// each frame (0x42fa80), and receives the screens' actions (0x42f190). The original's other two ways in, the screen
// saver's run and its settings dialog (the controller's slots 0x0c and 0x10, and the saver's stage list), are left
// out of the port, and so is the partner's message slot (0x20: 0x42eef0).

#include "App.h"
#include "FadeContainer.h"
#include "GameSession.h"
#include "GameStrings.h"
#include "ScreenFader.h"
#include "ScreenLayout.h"
#include "Scene.h"
#include "Settings.h"
#include "Signal.h"
#include "StageInfo.h"

#include <string>

namespace feedingfrenzy
{

// The window and the screen it shows (the controller's configure, 0x42b8e0, says them)
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int FRAME_RATE = 30;
const int WINDOW_ICON = 5;

class StageMgr;
class MainMenuScreen;

class FishFood : public engine::Controller, public engine::ScreenListener, public engine::HasSlots
{
public:
	FishFood();            // 0x430ee0
	~FishFood() override;  // 0x431310

	static FishFood* instance() { return sInstance; }  // the global 0x4c0880

	void configure(engine::AppConfig& theConfig) override;  // 0x04: 0x42b8e0
	void init() override;                                   // 0x08: 0x430b80
	void shutdown() override;                               // 0x14: 0x42c9d0

	// 0x42f190: a screen's action
	void screenAction(std::string theAction) override;

	// 0x42c0c0: the stage the game is in: the menu's in the loading and menu states, else the game's
	StageMgr* getStageMgr();
	// 0x42d230: a sound of the sound bank plays
	void playSound(const std::string& theName, float theVolume, float thePitch);

	GameSession* getSession() { return &mSession; }  // 0x42beb0 (+0x58)
	GameStrings& getStrings() { return mStrings; }  // 0x42c730's table (+0x30)
	Settings& getSettings() { return mSettings; }   // 0x42bb40 (+0xd4)

private:
	// The controller's state (+0x54), the part of the game it is in
	enum State
	{
		STATE_LOADING = 0  // the splash screen up to the main menu
	};

	void startup();                              // 0x430180
	void showSplash();                           // 0x42cc70
	void onAppUpdate(engine::UpdateInfo& theInfo);  // 0x42fa80
	void onAppActivate(bool theActive);          // 0x42c230
	void updateLoading(engine::UpdateInfo& theInfo);  // 0x42ec30
	void setState(int theState);                 // 0x42c3a0

	// The popup screens: one waiting to show (+0x7c, with the fade-in time at +0x84) and one to hide (+0x80)
	void showScreen(engine::ScreenLayout* theScreen, float theFadeSeconds);  // 0x42ba60
	void hideScreen(engine::ScreenLayout* theScreen, float theFadeSeconds);  // 0x42baf0
	void updateScreens();                                                    // 0x42ba80

	// The logo's slide between its hidden and shown positions (+0xb4 with its state at +0xb8)
	void slideLogoIn();                                  // 0x42bc40
	void slideLogoOut();                                 // 0x42bc90
	void updateLogo(engine::UpdateInfo& theInfo);        // 0x42bcd0

	void applySettings();  // 0x42bec0: the sound and music volumes, the full screen

	static FishFood* sInstance;

	engine::Ref<StageMgr> mGameStage;        // +0x20: the stage being played
	engine::Ref<StageMgr> mMenuStage;        // +0x24: the stage behind the menu
	int mState;                              // +0x54
	float mStateSeconds;                     // +0x5c: the seconds left of the state's wait
	int mStep;                               // +0x60: the step inside the state
	engine::Ref<engine::FadeContainer> mSplash;   // +0x64
	engine::Ref<engine::Scene> mScene;       // +0x8c
	engine::Ref<engine::Container> mRoot;    // +0x90: "RootGroup", the screens and the stages
	engine::Ref<engine::Container> mPopups;  // +0x94: "PopupGroup", the screens over them
	engine::Ref<ScreenFader> mFader;         // +0x88
	engine::Ref<engine::Container> mLogoGroup;  // +0xb4: the logo and its "TM"
	engine::Ref<engine::ScreenLayout> mShowingScreen;  // +0x7c
	engine::Ref<engine::ScreenLayout> mHidingScreen;   // +0x80
	float mShowFadeSeconds;                  // +0x84
	int mLogoState;                          // +0xb8: 0 hidden, 1 sliding in, 2 shown, 3 sliding out
	float mLogoSecondsLeft;                  // +0xbc
	float mLogoSlideSeconds;                 // +0xc0
	engine::Point mLogoHiddenPosition;       // +0xc4
	engine::Point mLogoShownPosition;        // +0xcc
	GameSession mSession;                    // +0x58: the game being played
	Settings mSettings;                      // +0xd4
	GameStrings mStrings;                    // +0x30
	StageManifest mManifest;                 // +0x1ec, +0x1fc, +0x214
};

} // namespace feedingfrenzy

#endif
