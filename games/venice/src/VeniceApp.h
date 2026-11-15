// The game's app and its widget: the app reads the partner's settings and the saved volumes, shows the logos and the loading
// screen, and the widget that fills the window runs the scenes 250 times a second and draws them.

#ifndef VENICE_APP_H
#define VENICE_APP_H

#include "SceneObject.h"
#include "SexyApp.h"
#include "Widget.h"

class VeniceWidget;

// 0x738 bytes in the original (its own fields after the framework's)
class VeniceApp : public Sexy::SexyApp
{
public:
	VeniceWidget* mWidget = nullptr; // 0x730
	int mDesktopWidth = 0; // 0xa2a18c: the size of the desktop (the game does not fit a window on a desktop this size)
	int mDesktopHeight = 0; // 0xa2a190

	VeniceApp(); // 0x43fa60
	~VeniceApp() override; // 0x440040
	void Init() override; // 0x440190
	void InitHook() override; // 0x4402d0
	void ShutdownHook() override; // 0x43ffa0
	void LostFocus() override; // 0x440410
	void GotFocus() override; // 0x440450
	void WriteToRegistry() override;
	bool HandleScriptCommand(const Sexy::ScriptCommand& theCommand) override; // TestCommands.cpp: the standard test commands
	void HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) override; // port: -livesplitlog

	// port (TestCommands.cpp): what a test can ask of the frames since the loading was done (frame 0 is the first update after it):
	// commands that wait for a frame (after), a trace of each frame's end, and the ticks each frame runs
	void RunFrameCommands(int theFramesDone);
	void GiveBotInput(int theFramesDone, const char* theName);
	void TraceFrame(int theFrame, int theTicksRun);
	int ScheduledTicks(int theFrame);
};

// The one app (0xa2a178)
extern VeniceApp* gApp;

// 0x4407c0: the clock the game's updates count from starts again (after a pause or a screen that took long to make)
void ResetTickBase();

// The widget that fills the window and runs and draws the scenes (0xa0 bytes in the original)
class VeniceWidget : public Sexy::Widget
{
public:
	VeniceWidget() {} // 0x4404f0
	~VeniceWidget() override {} // 0x4405a0
	SceneObject mLoadingScreen; // 0xa2a1b0: the picture behind the loading bar
	SceneObject mLoadingBar; // 0xa2a308: the bar, drawn clipped to the part that is loaded
	SceneObject mPausedOverlay; // 0xa2a5c0: the "game paused" sign shown while the window has no focus
	bool mPausedOverlayMade = false; // 0xa2a474

	void Update() override; // 0x440650
	void Draw(Sexy::Graphics* g) override; // 0x440e60
	void MouseEnter() override; // 0x4411f0
	void MouseLeave() override; // 0x441210
	void MouseMove(int theX, int theY) override; // 0x441190
	void MouseDrag(int theX, int theY) override; // 0x4411c0
	void MouseDown(int theX, int theY, int theClickCount) override; // 0x441310
	void MouseUp(int theX, int theY, int theClickCount) override; // 0x4413e0
	void MouseWheel(int theDelta) override; // 0x441230
	void KeyDown(Sexy::KeyCode theKey) override; // 0x441430
	void KeyUp(Sexy::KeyCode theKey) override; // 0x4414a0

	using Sexy::Widget::MouseDown;
	using Sexy::Widget::MouseUp;
};

#endif
