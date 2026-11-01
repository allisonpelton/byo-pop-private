#ifndef __BONNIES_BONNIESHOST_H__
#define __BONNIES_BONNIESHOST_H__

// The port's host for Bonnie's Bookstore (no original counterpart: the original was an SDL 1.2 program with FMOD 3).
// The game keeps its own loop, as the original did (bonniesMain, and one in each modal dialog): DoMainLoop runs it,
// and whenever it sleeps (bonnies::Host::sleep) the framework runs, which handles the window, the input and the sound
// and calls the widget. The widget turns the input into SDL 1.2's events, which the engine polls, and presents the
// game's 800x600 screen.

#include "SexyAppBase.h"
#include "Widget.h"

namespace Sexy
{

class MemoryImage;

class BonniesWidget : public Widget
{
public:
	MemoryImage* mScreenImage;
	bool mScreenChanged;

	BonniesWidget();
	virtual ~BonniesWidget();

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

	BonniesHostApp();
	virtual ~BonniesHostApp();

	void Init() override;
	void DoMainLoop() override;
	void CloseRequestAsync() override;
	void GotFocus() override;
	void LostFocus() override;
	void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	using SexyAppBase::SwitchScreenMode;

	// bonnies::Host::sleep: the framework runs for theMs
	void RunFramework(int theMs);
	// bonnies::Host::setFullScreen
	void SetGameFullscreen(bool theFullscreen);
};

extern BonniesHostApp* gHostApp;

} // namespace Sexy

#endif
