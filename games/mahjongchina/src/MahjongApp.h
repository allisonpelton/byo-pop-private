#ifndef __MAHJONGCHINA_MAHJONGAPP_H__
#define __MAHJONGCHINA_MAHJONGAPP_H__

// The port's host for Mahjong Escape: Ancient China. The game (MahjongGame, the original's app at 0x424a30, vtable
// 0x51df94) runs on JollyBear's engine (engine/); the framework gives it the window, the clock, the input and the
// sound device. The widget calls the manager's frame (the original's WM_TIMER) whenever the frame's pacing allows,
// sends it the mouse and the keys as the window procedure did, and shows the 800x600 screen the engine presents.

#include "App.h"
#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>
#include <vector>

namespace mahjongchina
{

// 0x424a30 (vtable 0x51df94): the game's app, 25 ms a frame
class MahjongGame : public jb::App
{
public:
	MahjongGame() : App(0x19) {}
	// 0x424a60, 0x424a80
	~MahjongGame() override {}

	jb::Layer* CreateFirstLayer() override;  // 0x40f540 the splash, with SplashLogo
	void StartGame() override;               // 0x40f5a0 the main menu
	void OnInit() override;                  // 0x40f620 the players and the current one's settings
	void OnExit() override;                  // 0x40f650 the current player's settings saved
};

} // namespace mahjongchina

namespace Sexy
{

class MemoryImage;

class MahjongWidget : public Widget
{
public:
	MemoryImage* mScreenImage;
	std::vector<uint32_t> mScreen;  // the engine's last present
	bool mScreenChanged;
	int mButtons;                   // MK_LBUTTON 1, MK_RBUTTON 2, MK_MBUTTON 0x10

	MahjongWidget();
	virtual ~MahjongWidget();

	void Update() override;
	void Draw(Graphics* g) override;
	void KeyChar(SexyChar theChar) override;
	void KeyDown(KeyCode theKey) override;
	void MouseMove(int x, int y) override;
	void MouseDrag(int x, int y) override;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
};

class MahjongApp : public SexyAppBase
{
public:
	MahjongWidget* mWidget;
	mahjongchina::MahjongGame* mGame;
	bool mGameStarted;
	bool mGameEnded;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (jb::Host::SetFullScreen)
	uint32_t mNextFrame;     // the frame's pacing (jb::Host::Sleep)

	MahjongApp();
	virtual ~MahjongApp();

	void Init() override;
	void Shutdown() override;
	void CloseRequestAsync() override;
	void GotFocus() override;
	void LostFocus() override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	void ReadFromRegistry() override;
	void WriteToRegistry() override;
	using SexyAppBase::SwitchScreenMode;

	// The manager's frame, when it is due; the game's end once the manager stopped
	void RunFrame();
	// The MK_ flags of the buttons held and Shift and Ctrl
	int InputFlags();
};

extern MahjongApp* gApp;

} // namespace Sexy

#endif
