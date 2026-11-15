#ifndef __PIZZAFRENZY_PIZZAFRENZY_H__
#define __PIZZAFRENZY_PIZZAFRENZY_H__

// The game's application (0x194 bytes, constructor 0x40c170, vtable 0x4f9a50; made by 0x40d0d0): the controller the
// engine's run loop starts. It keeps the settings, the scene, the sounds, the screens and the game's managers, and
// a state machine (+0x24) whose current state gets each frame's update and each screen's action.

#include "GenericApplication.h"
#include "GameMusicPlayer.h"
#include "GenericScreen.h"
#include "ImageNode.h"
#include "ScreenMgr.h"
#include "Settings.h"
#include "SoundMgr.h"
#include "State.h"
#include "TextItem.h"
#include "TileManifest.h"
#include "UserGameData.h"

#include <string>

namespace engine
{
class Scene;
}

namespace pizzafrenzy
{

class PizzaFrenzy : public engine::GenericApplication
{
public:
	PizzaFrenzy();             // 0x40c170
	~PizzaFrenzy() override;   // 0x40c6e0, 0x408b10

	static PizzaFrenzy* instance() { return sInstance; }  // 0x532900

	void configure(engine::AppConfig& theConfig) override;  // 0x04: 0x4050d0
	void init() override;                                   // 0x08: 0x409220
	void shutdown() override;                               // 0x14: 0x401df0
	void setMusicEnabled(bool theEnabled) override;         // 0x1c: 0x402340

	// The game's own slots
	virtual void update(engine::UpdateInfo& theInfo) override;  // 0x20: 0x401d90, from the application's update signal
	virtual void screenAction(std::string theAction);           // 0x24: 0x401030, from a screen's action signal
	virtual void windowCommand();                               // 0x28: 0x4059d0, SC_MINIMIZE and SC_MAXIMIZE
	virtual void windowActivate(bool theActive);                // 0x2c: 0x405a90, from the activate signal
	virtual void setFullScreen(bool theFullScreen);             // 0x30: 0x401090
	virtual void timerTick(int theMs);                          // 0x34: 0x4010b0, from the timer signal

	// 0x401570, from a screen's action signal (the loading state connects the screens it makes)
	void onScreenAction(std::string theAction) { screenAction(theAction); }

	engine::Ref<engine::Scene> mScene;                 // +0x20
	engine::StateMgr mStates;                          // +0x24
	engine::Ref<engine::SoundMgr> mSounds;             // +0x3c
	engine::Ref<engine::ScreenMgr> mScreens;           // +0x40
	engine::Ref<Settings> mSettings;                   // +0x44
	engine::Ref<engine::GenericScreen> mSplashScreen;  // +0x5c
	engine::Ref<engine::ImageNode> mLoadBar;           // +0x60: the splash screen's load bar, shown as far as loaded
	int mLoadBarWidth;                                 // +0x64
	int mLoadBarHeight;                                // +0x68
	engine::Ref<engine::GenericScreen> mDeleteUserConfirm;   // +0x74
	engine::Ref<engine::TextItem> mDeleteUserText;           // +0x78: its question
	std::string mDeleteUserTemplate;                         // +0x7c: the question as the layout has it
	engine::Ref<engine::GenericScreen> mRestoreGameConfirm;  // +0xa8
	engine::Ref<engine::GenericScreen> mAppQuitConfirm;      // +0xac
	engine::Ref<engine::GenericScreen> mGameExitConfirm;     // +0xb0
	engine::Ref<engine::GenericScreen> mPauseMenu;           // +0xd4
	engine::Ref<TileManifest> mTileManifest;                 // +0x124: the game's data
	engine::Ref<UserGameData> mUserGameData;                 // +0x128: the current player's
	engine::Ref<GameMusicPlayer> mMusicPlayer;               // +0x130

private:
	void onAppUpdate(engine::UpdateInfo& theInfo) { update(theInfo); }  // 0x401560
	void onWindowCommand() { windowCommand(); }                          // 0x401580
	void onActivate(bool theActive) { windowActivate(theActive); }       // 0x401590
	void onTimer(int theMs) { timerTick(theMs); }                        // 0x4015a0

	static PizzaFrenzy* sInstance;

	std::string mTitle;  // the configuration's title (the string table's 0xcb)
	bool mActive;        // +0x188: the window has the focus
};

} // namespace pizzafrenzy

#endif
