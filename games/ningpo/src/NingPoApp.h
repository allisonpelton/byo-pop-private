#ifndef __NINGPO_NINGPOAPP_H__
#define __NINGPO_NINGPOAPP_H__

// The port's host for Ning Po (no original counterpart: the original was a Win32 program on Pyrogon's framework).
// The game keeps its own loop, as the original did (PGApp::mainLoop, and one in each dialog): DoMainLoop runs it,
// and whenever it sleeps (pg::Host::Sleep) the framework runs, which handles the window, the input and the sound
// and calls the widget. The widget turns the input into the game's events (as the original's window procedure
// 0x433290 did), posts its 10 ms tick and presents the game's 640x480 screen.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdio>
#include <functional>
#include <string>

namespace ningpo
{
class NingPo;
}

namespace Sexy
{

class MemoryImage;

class NingPoWidget : public Widget
{
public:
	MemoryImage* mScreenImage;
	bool mScreenChanged;

	NingPoWidget();
	virtual ~NingPoWidget();

	void Update() override;
	void Draw(Graphics* g) override;
	void KeyChar(SexyChar theChar) override;
	void KeyDown(KeyCode theKey) override;
	void KeyUp(KeyCode theKey) override;
	void MouseMove(int x, int y) override;
	void MouseDrag(int x, int y) override;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
	void MouseWheel(int theDelta) override;
};

class NingPoApp : public SexyAppBase
{
public:
	NingPoWidget* mWidget;
	ningpo::NingPo* mGame;
	bool mGameRunning;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (pg::Host::SetFullscreen)
	// test hook: -statelog=<file> writes a line "<time> <name> <value>" each time one of the game's values that decide
	// play changes, <time> being the time of the event handled last (PGApp::mEventTime, the original's 0x47be70), so
	// that a run can be compared with the original's watched values (games/ningpo/tools/identical)
	std::string mStateLogFileName;
	FILE* mStateLog;
	int mStateLogValues[32];
	// a test command's action, run at the game's next read of its events (src/TestCommands.cpp)
	std::function<void()> mTestAction;

	NingPoApp();
	virtual ~NingPoApp();

	void Init() override;
	void DoMainLoop() override;
	void CloseRequestAsync() override;
	void GotFocus() override;
	void LostFocus() override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override;
	// the standard test commands (docs/TEST_COMMANDS.md; src/TestCommands.cpp)
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	void AddMemReportHolders(Platform::MemReport* theReport) override;
	using SexyAppBase::SwitchScreenMode;

	// The event the window procedure would have posted, with the modifier keys held
	void PostEvent(int theType, int theCode, int theChar, int theX, int theY, int theValue);

	// pg::Host::Sleep (the host is NingPoHost in NingPoApp.cpp): the framework runs for theMs
	void RunFramework(int theMs);
	// pg::Host::SetFullscreen
	void SetGameFullscreen(bool theFullscreen);
	// -statelog: the values that changed since the last call, each time the game reads its event queue (origrun reads
	// the original's watches at its waits and clock reads, which come at the same points of its loop)
	void UpdateStateLog();
	// pg::Host::EventRead: the state log, then a test command's action (src/TestCommands.cpp)
	void EventRead();
};

extern NingPoApp* gApp;

} // namespace Sexy

#endif
