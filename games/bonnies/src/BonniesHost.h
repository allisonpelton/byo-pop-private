#ifndef __BONNIES_BONNIESHOST_H__
#define __BONNIES_BONNIESHOST_H__

// The port's host for Bonnie's Bookstore (no original counterpart: the original was an SDL 1.2 program with FMOD 3).
// The game keeps its own loop, as the original did (bonniesMain, and one in each modal dialog): DoMainLoop runs it,
// and whenever it sleeps (bonnies::Host::sleep) the framework runs, which handles the window, the input and the sound
// and calls the widget. The widget turns the input into SDL 1.2's events, which the engine polls, and presents the
// game's 800x600 screen.

#include "SexyAppBase.h"
#include "Widget.h"

#include <cstdint>
#include <vector>

struct SDL_Cursor;

namespace Sexy
{

class MemoryImage;
class SurfaceDisplay;

class BonniesWidget : public Widget
{
public:
	MemoryImage* mScreenImage;  // the picture of the game's screen the framework draws (none when the window's display keeps the screen)
	bool mScreenChanged;
	std::vector<uint32_t> mShown;  // the display's copy of the game's screen: what it was when the display last had it
	bool mShownWhole;              // the display has been given the whole screen once

	BonniesWidget();
	virtual ~BonniesWidget();

	// copies what differs between the game's screen and the picture of it into the picture; whether anything did
	bool CopyScreenChanges();
	// hands the window's display the rows of the game's screen that differ from what it last had; whether any did
	bool SendScreenChanges();

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
	void MouseEnter() override;
	void MouseLeave() override;
};

class BonniesHostApp : public SexyAppBase
{
public:
	BonniesWidget* mWidget;
	bool mGameRunning;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (bonnies::Host::setFullScreen)
	bool mMouseInside;
	uint32_t mLastFrameworkRun;
	bool mScriptNoFocus;     // a script's "focus 0": the test run has lost the focus
#ifndef __EMSCRIPTEN__
	SDL_Cursor* mWindowCursor;  // the game's own cursor as the window's, nullptr until the game sets one
#endif

	BonniesHostApp();
	virtual ~BonniesHostApp();

	void Init() override;
#ifndef __EMSCRIPTEN__
	// the window's own display (framework/SurfaceDisplay.h): the game's screen and the rects of it that changed go to it
	DDInterface* CreateDDInterface() override;
	void EnforceCursor() override;
	// the game's own cursor (bonnies::Host::setCursor) as the window's cursor; nullptr: none. The cursor is freed here.
	void SetWindowCursor(SDL_Cursor* theCursor);
#endif
	void DoMainLoop() override;
	void CloseRequestAsync() override;
	void GotFocus() override;
	void LostFocus() override;
	bool HandleScriptCommand(const ScriptCommand& theCommand) override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	void AddMemReportHolders(Platform::MemReport* theReport) override;
	using SexyAppBase::SwitchScreenMode;

	// bonnies::Host::sleep: the framework runs for theMs
	void RunFramework(int theMs);
	// bonnies::Host::setFullScreen
	void SetGameFullscreen(bool theFullscreen);
};

extern BonniesHostApp* gHostApp;

} // namespace Sexy

#endif
