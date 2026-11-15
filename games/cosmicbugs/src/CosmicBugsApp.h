#ifndef __COSMICBUGS_COSMICBUGSAPP_H__
#define __COSMICBUGS_COSMICBUGSAPP_H__

// The port's host for Cosmic Bugs (no original counterpart: the original was a Win32 program on Retro64's engine,
// with DirectDraw and BASS). The framework runs the window, the clock, the input and the sound; each of its 100 Hz
// updates runs one pass of the original's main loop (GameFrame: the game steps once for each millisecond passed),
// which draws into the engine's 800x600 16-bit screen, and the widget presents that screen.

#include "R64.h"
#include "R64ScreenDiff.h"
#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{

class MemoryImage;

class CosmicBugsWidget : public Widget
{
public:
	// What shows the engine's screen. The desktop builds' window display (SurfaceDisplay, made by CreateDDInterface below)
	// keeps the screen itself: the widget widens what changed into mPixels, the 32-bit copy the display shows, and hands it the
	// rectangles. The web build, and a desktop run with SURFACE_DISPLAY=off, keep the framework's display, and the widget draws
	// its picture of the screen (mScreenImage) instead.
	MemoryImage* mScreenImage;
	r64::ScreenDiff mScreenDiff;
	std::vector<uint32_t> mPixels;
	std::vector<Rect> mFrameRects;

	CosmicBugsWidget();
	virtual ~CosmicBugsWidget();

	void Update() override;
	void Draw(Graphics* g) override;
	void KeyDown(KeyCode theKey) override;
	void KeyUp(KeyCode theKey) override;
	void MouseMove(int x, int y) override;
	void MouseDrag(int x, int y) override;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
};

class CosmicBugsApp : public SexyAppBase
{
public:
	CosmicBugsWidget* mWidget;
	bool mGameStarted;
	bool mGameEnded;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (r64 SetScreenMode)

	// The engine's screen: 800x600 RGB565, and whether it was presented since the widget last copied it
	std::vector<uint16_t> mScreen;
	bool mPresented;
	int mMouseX, mMouseY;

	CosmicBugsApp();
	virtual ~CosmicBugsApp();

	void Init() override;
	void Shutdown() override;
	void CloseRequestAsync() override;
#ifndef __EMSCRIPTEN__
	// the window's own surface shows the screen (framework/SurfaceDisplay.h); SURFACE_DISPLAY=off in the environment keeps the
	// framework's display, for a test that compares the two
	DDInterface* CreateDDInterface() override;
#endif
	void SwitchScreenMode(bool theWantWindowed, bool theIs3d, bool theForce = false) override;
	using SexyAppBase::SwitchScreenMode;
	// -script's "state": logs the score, the lives, the board in its set and the board's filled cells, as the headless
	// runner (test/gamerun.cpp) prints them
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;

	// -livesplitlog=<file> (tests): one line for each write of the autosplit block that changes what a splitter reads
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;

	// One pass of the original's main loop
	void RunFrame();
	// The engine's requests (CosmicBugsApp.cpp's SysHost)
	void Present();
	void SetGameScreenMode(bool theFullscreen);
	void WarpMouse(int x, int y);
};

extern CosmicBugsApp* gApp;

} // namespace Sexy

#endif
