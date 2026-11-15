#ifndef __MAHJONG_MAHJONGAPP_H__
#define __MAHJONG_MAHJONGAPP_H__

// The port's host for the Mahjong Escape games. The game (MahjongGame, the original's app at 0x40f4a0 (mahjongchina),
// 0x424a30 (mahjongjapan), vtable 0x51be1c (mahjongchina), 0x51df94 (mahjongjapan)) runs on JollyBear's engine
// (engine/); the framework gives it the window, the clock, the input and the sound device. The widget calls the
// manager's frame (the original's WM_TIMER) whenever the frame's pacing allows, sends it the mouse and the keys as the
// window procedure did, and shows the 800x600 screen the engine presents.

#include "App.h"
#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>
#include <vector>

namespace mahjong
{

// 0x40f4a0 (mahjongchina), 0x424a30 (mahjongjapan) (vtable 0x51be1c (mahjongchina), 0x51df94 (mahjongjapan)): the
// game's app, 25 ms a frame
class MahjongGame : public jb::App
{
public:
	// 0x40f4a0 (mahjongchina), 0x424a30 (mahjongjapan)
	MahjongGame() : App(0x19) {}
	// 0x40f4d0 (mahjongchina), 0x424a60 (mahjongjapan), 0x40f4f0 (mahjongchina), 0x424a80 (mahjongjapan)
	~MahjongGame() override {}

	// 0x40f500 (mahjongchina), 0x40f540 (mahjongjapan) the splash, with SplashLogo
	jb::Layer* CreateFirstLayer() override;
	void StartGame() override;               // 0x40f560 (mahjongchina), 0x40f5a0 (mahjongjapan) the main menu
	// 0x40f5c0 (mahjongchina), 0x40f620 (mahjongjapan) the players and the current one's settings
	void OnInit() override;
	// 0x40f5f0 (mahjongchina), 0x40f650 (mahjongjapan) the current player's settings saved
	void OnExit() override;
};

} // namespace mahjong

namespace Sexy
{

class MemoryImage;

class MahjongWidget : public Widget
{
public:
	MemoryImage* mScreenImage;
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
	mahjong::MahjongGame* mGame;
	bool mGameStarted;
	bool mGameEnded;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (jb::Host::SetFullScreen)
	uint32_t mNextFrame;     // the frame's pacing (jb::Host::Sleep)

	MahjongApp();
	virtual ~MahjongApp();

	void Init() override;
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	void Shutdown() override;
	void CloseRequestAsync() override;
	void GotFocus() override;
	void LostFocus() override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	void ReadFromRegistry() override;
	void WriteToRegistry() override;
	using SexyAppBase::SwitchScreenMode;

	// The standard test commands (TestCommands.cpp, docs/TEST_COMMANDS.md); only a test's -script, -commands or
	// -realinput reaches them
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// Every update: a board in play is noted, and a command waiting for its screen keeps -hold's updates running
	void TestUpdate();
	// The frame after a test command that waits for a screen to settle
	void TestFrame();

	// The manager's frame, when it is due; the game's end once the manager stopped
	void RunFrame();
	// The MK_ flags of the buttons held and Shift and Ctrl
	int InputFlags();
};

extern MahjongApp* gApp;

} // namespace Sexy

#endif
