#ifndef __NINGPO_NINGPOAPP_H__
#define __NINGPO_NINGPOAPP_H__

// The port's host for Ning Po (no original counterpart: the original was a Win32 program on Pyrogon's framework).
// The game keeps its own loop, as the original did (PGApp::mainLoop, and one in each dialog): DoMainLoop runs it,
// and whenever it sleeps (pg::Host::Sleep) the framework runs, which handles the window, the input and the sound
// and calls the widget. The widget turns the input into the game's events (as the original's window procedure
// 0x433290 did), posts its 10 ms tick and presents the game's 640x480 screen.

#include "SexyAppBase.h"
#include "Widget.h"

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

	NingPoApp();
	virtual ~NingPoApp();

	void Init() override;
	void DoMainLoop() override;
	void CloseRequestAsync() override;
	void GotFocus() override;
	void LostFocus() override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	using SexyAppBase::SwitchScreenMode;

	// The event the window procedure would have posted, with the modifier keys held
	void PostEvent(int theType, int theCode, int theChar, int theX, int theY, int theValue);

	// pg::Host::Sleep (the host is NingPoHost in NingPoApp.cpp): the framework runs for theMs
	void RunFramework(int theMs);
	// pg::Host::SetFullscreen
	void SetGameFullscreen(bool theFullscreen);
};

extern NingPoApp* gApp;

} // namespace Sexy

#endif
