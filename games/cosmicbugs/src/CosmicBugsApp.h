#ifndef __COSMICBUGS_COSMICBUGSAPP_H__
#define __COSMICBUGS_COSMICBUGSAPP_H__

// The port's host for Cosmic Bugs (no original counterpart: the original was a Win32 program on Retro64's engine,
// with DirectDraw and BASS). The framework runs the window, the clock, the input and the sound; each of its 100 Hz
// updates runs one pass of the original's main loop (GameFrame: the game steps once for each millisecond passed),
// which draws into the engine's 800x600 16-bit screen, and the widget presents that screen.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>
#include <vector>

namespace Sexy
{

class MemoryImage;

class CosmicBugsWidget : public Widget
{
public:
	MemoryImage* mScreenImage;

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

	// The engine's screen: 800x600 RGB565, and the part of it presented since the widget last copied it
	std::vector<uint16_t> mScreen;
	bool mPresented;
	bool mWholeScreen;
	int mDirtyLeft, mDirtyTop, mDirtyRight, mDirtyBottom;
	int mMouseX, mMouseY;

	CosmicBugsApp();
	virtual ~CosmicBugsApp();

	void Init() override;
	void Shutdown() override;
	void CloseRequestAsync() override;
	void SwitchScreenMode(bool theWantWindowed, bool theIs3d, bool theForce = false) override;
	using SexyAppBase::SwitchScreenMode;
	// -script's "state": logs the score, the lives, the board in its set and the board's filled cells, as the headless
	// runner (test/gamerun.cpp) prints them
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;

	// One pass of the original's main loop
	void RunFrame();
	// The engine's requests (CosmicBugsApp.cpp's SysHost)
	void AddUpdateRect(int x, int y, int w, int h);
	void Present();
	void SetGameScreenMode(bool theFullscreen);
	void WarpMouse(int x, int y);
};

extern CosmicBugsApp* gApp;

} // namespace Sexy

#endif
