#ifndef __PIZZAFRENZY_PIZZAFRENZYAPP_H__
#define __PIZZAFRENZY_PIZZAFRENZYAPP_H__

// The port's host for Pizza Frenzy (no original counterpart: the original was the engine's Windows layer,
// 0x4bcc50 (pizzafrenzy) on). The engine runs one frame per call, so the framework drives it: the widget runs a frame
// once the frame's time has come (the original loop's test, 0x4bdf20), draws the 800x600 screen and turns the
// framework's input into the application's signals (as the window procedure did, 0x4bd3b0). The host interface
// (engines/sprout/Host.h) is implemented in PizzaFrenzyApp.cpp over the framework and the platform layer.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>
#include <string>
#include <vector>

namespace engine
{
class ScreenGraphics;
}

namespace Sexy
{

class MemoryImage;
class SurfaceDisplay;

class PizzaFrenzyWidget : public Widget
{
public:
	MemoryImage* mScreenImage;  // the screen the framework draws, null when the window's display keeps it (mSurfaceDisplay)
	// port (for speed): the window's display (framework/SurfaceDisplay.h), native builds: it shows mSurfaceBits itself,
	// the rectangles of it that changed handed over each frame; null in the web build and with SURFACE_DISPLAY=off
	SurfaceDisplay* mSurfaceDisplay;
	std::vector<uint32_t> mSurfaceBits;
	const uint32_t* mPixels;  // the 800x600 screen, 0xffRRGGBB: the picture's pixels, or mSurfaceBits
	engine::ScreenGraphics* mGraphics;
	uint64_t mNextFrame;     // the run loop's time for the next frame, in the host's ticks
	bool mKeyIsDown[256];    // the keys that went down and not up yet: a repeat of one is no WM_KEYDOWN's first

	PizzaFrenzyWidget();
	~PizzaFrenzyWidget() override;

	void Update() override;
	void Draw(Graphics* g) override;
	void KeyDown(KeyCode theKey) override;
	void KeyUp(KeyCode theKey) override;
	void KeyChar(SexyChar theChar) override;
	void MouseMove(int x, int y) override;
	void MouseDrag(int x, int y) override;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;

	// the focus is gone: a key let go elsewhere is up
	void ClearKeys();
};

class PizzaFrenzyApp : public SexyAppBase
{
public:
	PizzaFrenzyWidget* mWidget;
	bool mEngineRunning;
	bool mQuitRequested;
	int mFrames;             // port: the engine's frames
	uint64_t mEngineClock;   // port: -fast: the engine's ticks, one frame period a frame
	bool mSteadyClock = false;  // port(mod): the fixes mod's steadyClock: the engine's clock as with -fast, in real time
	bool mGameSeen = false;  // port (test commands): the state has shown a game's screen, so the menu is its end
	// port: -statelog=<file>, the values the identical-play runs compare with the original's (LogState)
	FILE* mStateLog = nullptr;
	std::vector<std::string> mLoggedState;  // port: the values the state log last wrote

	PizzaFrenzyApp();
	~PizzaFrenzyApp() override;

	void Init() override;
#ifndef __EMSCRIPTEN__
	// port (for speed): the window's display is the surface display, which keeps the screen and copies only what changed
	DDInterface* CreateDDInterface() override;
#endif
	void Shutdown() override;
	// port: -seed=N, for the game's srand of the time
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	void GotFocus() override;
	void LostFocus() override;

	// port: the standard test commands (docs/TEST_COMMANDS.md, src/TestCommands.cpp)
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// port: the memreport test command: what the engine holds beside the framework's own pictures and sounds
	void AddMemReportHolders(Platform::MemReport* theReport) override;
	// port: the state command's JSON, and the move command's line (the bot's next move, or "null")
	std::string TestState();
	std::string TestMove();
	// port: -statelog: "<frame> <name> <value>" for each value that changed since the last line (the names of
	// games/pizzafrenzy/tools/identical/case.sh), floats as their bits, "-" for a value that does not exist
	void LogState();

	// A frame of the engine (0x4bd210); an exception the original would not survive ends the program
	void RunFrame();
	void EngineFailed(const char* theWhat);

	// The run loop's frame period in the host's ticks (0x4bc870: the clock's rate divided by the frame rate)
	uint64_t FramePeriod() const;
};

extern PizzaFrenzyApp* gApp;

} // namespace Sexy

#endif
