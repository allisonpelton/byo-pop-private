#ifndef __WORDHARMONY_WORDHARMONYAPP_H__
#define __WORDHARMONY_WORDHARMONYAPP_H__

// The port's host for Word Harmony (no original counterpart: the original was the engine's Windows layer, 0x454a40-
// 0x45ee00). The engine runs one frame per call, so the framework drives it: the widget runs a frame whenever the
// game's frame period has passed (the original loop's test, 0x4552b0), draws the 800x600 screen and turns the
// framework's input into the root window's (as the window procedure did). The host interface (engine/Host.h) is
// implemented in WordHarmonyApp.cpp over the framework and the platform layer.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdio>
#include <string>
#include <vector>

namespace wordharmony
{
class HostSoundSystem;
class ScreenGraphics;
}

namespace Sexy
{

class MemoryImage;

class WordHarmonyWidget : public Widget
{
public:
	MemoryImage* mScreenImage;
	wordharmony::ScreenGraphics* mGraphics;
	DWORD mLastLoop;       // the run loop's last tick count
	int mAccumulated;      // the milliseconds not yet run as frames (0x4552b0's carry)
	DWORD mLastSoundTick;

	WordHarmonyWidget();
	~WordHarmonyWidget() override;

	void Update() override;
	void Draw(Graphics* g) override;
	void KeyDown(KeyCode theKey) override;
	void KeyUp(KeyCode theKey) override;
	void MouseMove(int x, int y) override;
	void MouseDrag(int x, int y) override;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
};

class WordHarmonyApp : public SexyAppBase
{
public:
	WordHarmonyWidget* mWidget;
	bool mEngineRunning;
	bool mQuitRequested;
	std::string mCommandLine;  // the screen saver's arguments only (/s, /c, /p)
	int mFrames;               // port: the engine's frames, the identical-play runs' update counter
	uint32_t mEngineClock;     // port: -fast: the engine's milliseconds, one frame period a frame
	FILE* mStateLog;           // port: -statelog=<file>, the values that decide play (LogState)
	std::vector<std::pair<std::string, std::string>> mLoggedState;

	WordHarmonyApp();
	~WordHarmonyApp() override;

	void Init() override;
	void Shutdown() override;
	void GotFocus() override;
	void LostFocus() override;

	// The command line as GetCommandLineA() gave it to the game, with only the screen saver's arguments kept: the
	// framework's own (-seed, -script...) would read as them
	void SetGameCommandLine(int argc, char** argv);
	// A frame of the engine (0x4542e0); an exception the original would not survive ends the program
	void RunFrame();
	void EngineFailed(const char* theWhat);
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	// port: -statelog: "<frame> <name> <value>" for each value that changed in the frame
	void LogState();
};

extern WordHarmonyApp* gApp;

} // namespace Sexy

#endif
