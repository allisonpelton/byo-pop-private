#ifndef __TYPERSHARK_TITLESCREEN_H__
#define __TYPERSHARK_TITLESCREEN_H__

// TitleScreen.cpp: the loading screen the app shows from Init until the data is loaded: the title picture, the bar and
// logo, and a gold bar that grows with SexyAppBase::GetLoadingThreadProgress (drawn at (0x95, 0x171) over the logo). When
// the loading thread has finished the app presses the continue button's id itself (WinWSApp::LoadingThreadProc calls
// ButtonDepress(0x191)), which takes the screen away (WinWSApp::ShowMapScreen). The screen does not run any loading step:
// those are the app's (LoadingThreadProc 0x435560), which only tells the screen to redraw.
// The app makes it in Init: new TitleScreen(this), Resize to the window, AddControls(), AddWidget (WinWSApp.cpp), and keeps
// it at WinWSApp::mTitleScreen.
// Original: class Sexy::TitleScreen : public Widget, vftable 0x52f2cc, sizeof 0x7c, constructor 0x448e10, destructor body
// 0x4414c0 (deleting 0x441ff0, slot 0); the image loading that follows the constructor is the function 0x448f40, which the export lists by address only.
// The original's screen also had a register button, the registered / trial stamps and a "continue" button that a registered
// copy never shows: removed (registration).

#include "Widget.h"

namespace Sexy
{

class Image;
class WinWSApp;

class TitleScreen : public Widget
{
public:
	enum
	{
		CONTINUE_ID = 0x191				// what WinWSApp::LoadingThreadProc passes to ButtonDepress: it shows the map screen
	};

public:
	Image*					mTitleImage;	// +0x58 images\titlescreen (owned)
	Image*					mBarAndLogo;	// +0x5c images\barandlogo (owned)
	Image*					mGoldBar;		// +0x60 images\goldbar (owned): the loading bar, drawn as wide as the progress
	// +0x64 to +0x70 the stamps, the register button and the font of the continue button, +0x78 the continue button: removed (registration)
	WinWSApp*				mApp;			// +0x74 constructor argument

public:
	TitleScreen(WinWSApp* theApp);									// 0x448e10
	virtual ~TitleScreen() override;								// 0x4414c0 (deleting 0x441ff0), slot 0

	void					AddControls();							// 0x448f40 loads the images; Popup and mLoadingFailed when one is missing

	virtual void			Draw(Graphics* g) override;				// 0x4495c0 slot 16
};

}

#endif
