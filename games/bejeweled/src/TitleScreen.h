#ifndef __TITLESCREEN_H__
#define __TITLESCREEN_H__

// TitleScreen.cpp: loading screen with the green progress bar and the "CLICK HERE TO PLAY!" link.
// Original: class Sexy::TitleScreen : public Widget, public ButtonListener, sizeof 0xb8.
// Vtables: Widget 0x507674, ButtonListener 0x507650 (only ButtonDepress overridden: no click sound).
// Offsets are absolute; Ghidra's TitleScreen_data.offset_N is absolute N+0x8c.
// Behaviour spec: games/bejeweled/docs/GAME_NOTES.md (TitleScreen section).
// The port is always registered (SexyApp::mIsRegistered and mBuildUnlocked are true), so only the
// registered code paths are ported: no trial nag, trial circle or status line.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinDMApp;
class Graphics;
class WidgetManager;
class HyperlinkWidget;

class TitleScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		REGISTER_LINK_ID = 0,
		PLAY_LINK_ID = 1,
		LOADING_BAR_X = 143,
		LOADING_BAR_WIDTH = 340
	};

	// +0x088 ButtonListener vptr
	WinDMApp*		mApp;                   // +0x08c
	double			mLoadingProgress;       // +0x090 fraction of the green bar drawn (0..1)
	double			mDelayProgress;         // +0x098 nag delay timer 0..1 (+0.01/mTotalTimerSec per update); caps the bar only when unregistered
	bool			mLoadingComplete;       // +0x0a0 bar reached exactly 1.0 with app->mLoaded; play link shown
	int				mDisplayedPlayCount;    // +0x0a4 vestigial counter; only causes MarkDirty
	bool			mRegisterClicked;       // +0x0a8 register link clicked; Update opens the dialog once loaded
	// +0x0a9 mMustRegister (trial expired: nag mode) is not ported; it is always false when registered.
	int				mTotalTimerSec;         // +0x0ac nag delay length in seconds (property TotalTimerSec)
	HyperlinkWidget* mRegisterLink;         // +0x0b0 link id 0 (mainfont1, green); visible while unregistered (never in the port)
	HyperlinkWidget* mPlayLink;             // +0x0b4 link id 1 (afont1, green); hidden until loading completes

public:
	virtual ~TitleScreen();                                         // 0x4d360b (scalar deleting dtor 0x4d36ec)
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x4d3661 slot 20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x4d3691 slot 21
	virtual void	ButtonDepress(int theId);                       // 0x4d36c1 ButtonListener slot 2
	virtual void	Draw(Graphics* g);                              // 0x4d370b slot 26
	virtual void	Update();                                       // 0x4d3c6a slot 22
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x4d4118 slot 40
	TitleScreen(WinDMApp* theApp);                                  // 0x4d41c8

	using Widget::Resize;
};

}

#endif
