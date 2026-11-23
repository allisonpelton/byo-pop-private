#ifndef __FEEDINGFRENZY2_FEEDINGFRENZY2APP_H__
#define __FEEDINGFRENZY2_FEEDINGFRENZY2APP_H__

// The port's host for Feeding Frenzy 2 (no original counterpart: the original was the engine's Windows layer, its run
// 0x4383b0 (feedingfrenzy2)). The engine runs one frame per call, so the framework drives it: the widget runs a frame
// once the frame's time has come (the run loop's test of the clock), shows the 800x600 screen and turns the
// framework's input into the engine's signals (as the window procedure did). The engine's platform services are
// engines/sprout2/host's EngineImpl.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>

class FeedingFrenzyTwo;

namespace engine
{
class EngineImpl;
class Surface;
}

namespace Sexy
{

class MemoryImage;

class FeedingFrenzy2Widget : public Widget
{
public:
	uint64_t mNextFrame;  // the run loop's time for the next frame, in the engine's ticks
	// The screen as the framework draws it: the display's pixels copied after each frame
	MemoryImage* mScreenImage;

	bool mKeyIsDown[256];  // the keys that went down and not up yet: a repeat of one is no WM_KEYDOWN's first
	int mMouseX;           // the last position the mouse was seen at (the engine's +0x2b0 and +0x2b4)
	int mMouseY;

	FeedingFrenzy2Widget();
	~FeedingFrenzy2Widget() override;

	void Update() override;
	void Draw(Graphics* g) override;

	// The window procedure's input (0x424b30 (feedingfrenzy2)), as the engine's signals
	void KeyDown(KeyCode theKey) override;
	void KeyUp(KeyCode theKey) override;
	void KeyChar(SexyChar theChar) override;
	void MouseMove(int x, int y) override;
	void MouseDrag(int x, int y) override;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseWheel(int theDelta) override;
	void ClearKeys();
};

class FeedingFrenzy2App : public SexyAppBase
{
public:
	FeedingFrenzy2Widget* mWidget;
	engine::EngineImpl* mEngine;
	// The display's back buffer, which each frame is drawn into (the engine's +0x2a4, a DirectDraw surface)
	engine::Surface* mDisplay;
	// The game's application (the engine's +0x288)
	FeedingFrenzyTwo* mGame;
	bool mEngineRunning;
	int mFrames;  // port: the engine's frames
	uint64_t mLastFrameTicks;  // the clock when the last frame began (the engine's +0x2c8)

	FeedingFrenzy2App();
	~FeedingFrenzy2App() override;

	void Init() override;
	void Shutdown() override;
	// port: the original keeps no settings of the framework's (its options are its own, and its players are in the
	// registry key the engine opens); the registry is marked read, so that the web build backs it up
	void ReadFromRegistry() override;
	void WriteToRegistry() override;
	// WM_ACTIVATE: the engine's activate signal
	void GotFocus() override;
	void LostFocus() override;
	// port: the test commands (docs/TEST_COMMANDS.md): unlock all
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	// The test command state: one line, STATE {...}, with the mode, the stage, the score, the lives, the food eaten,
	// the player's phase, its frenzy's multiplier and where it is (docs/TEST_COMMANDS.md)
	void logState();
	// The test command set <name> <value>: the score, the lives or the food eaten in the stage
	void setState(const std::string& theArgs);
	// The test commands win and lose: the stage's goal reached, or the player's fish ended as an eater ends it
	void endStage(bool isWin);

	// A frame of the engine (0x438700 (feedingfrenzy2))
	void RunFrame();
	// The run loop's frame period in the engine's ticks (the engine's +0x160: the clock's rate divided by the frame
	// rate)
	uint64_t FramePeriod() const;
};

extern FeedingFrenzy2App* gApp;

} // namespace Sexy

#endif
