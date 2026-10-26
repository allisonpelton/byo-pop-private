#ifndef __TIPTOP_TIPTOPAPP_H__
#define __TIPTOP_TIPTOPAPP_H__

// The port's host for TipTop (no original counterpart: the original was a Win32 program on its LOWOS engine).
// The framework runs the window, the clock and the input; each of its 100 Hz updates runs one pass of the
// original's main loop (Game.h), which draws into LOWOS's 640x480 screen, and the widget presents that screen.

#include "SexyAppBase.h"
#include "Widget.h"

namespace Sexy
{

class MemoryImage;

class TipTopWidget : public Widget
{
public:
	MemoryImage* mScreenImage;

	TipTopWidget();
	virtual ~TipTopWidget();

	virtual void Update() override;
	virtual void Draw(Graphics* g) override;
	virtual void KeyChar(SexyChar theChar) override;
	virtual void KeyDown(KeyCode theKey) override;
	virtual void KeyUp(KeyCode theKey) override;
	virtual void MouseMove(int x, int y) override;
	virtual void MouseDrag(int x, int y) override;
	virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
};

class TipTopApp : public SexyAppBase
{
public:
	TipTopWidget* mWidget;
	bool mGameStarted;
	bool mGameEnded;
	bool mApplyingGameMode;  // SwitchScreenMode is the game's own request (LOWOS_ScreenScope)

	TipTopApp();
	virtual ~TipTopApp();

	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void CloseRequestAsync() override;
	virtual void GotFocus() override;
	virtual void LostFocus() override;
	virtual void SwitchScreenMode(bool wantWindowed, bool is3d, bool force = false) override;
	using SexyAppBase::SwitchScreenMode;

	// One pass of the original's main loop, then the screen mode it asked for
	void RunFrame();
};

extern TipTopApp* gApp;

} // namespace Sexy

#endif
