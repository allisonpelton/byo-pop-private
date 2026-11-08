#ifndef __TYPERSHARK_TITLESCREEN_H__
#define __TYPERSHARK_TITLESCREEN_H__

// TitleScreen.cpp: the loading screen the app shows from Init until the data is loaded: the title picture, the bar and
// logo, and a gold bar that grows with SexyAppBase::GetLoadingThreadProgress (drawn at (0x95, 0x171) over the logo). When
// the loading thread has finished the app clicks the "CLICK HERE TO CONTINUE" button itself (WinWSApp::LoadingThreadProc
// calls ButtonDepress(0x191) for a registered game), which takes the screen away (WinWSApp::ShowMapScreen). The screen
// does not run any loading step: those are the app's (LoadingThreadProc 0x435560), which only tells the screen to redraw.
// The app makes it in Init: new TitleScreen(this), Resize to the window, AddControls(), AddWidget (WinWSApp.cpp), and keeps
// it at WinWSApp::mTitleScreen.
// Original: class Sexy::TitleScreen : public Widget, vftable 0x52f2cc, sizeof 0x7c, constructor 0x448e10, destructor body
// 0x4414c0 (deleting 0x441ff0, slot 0); the image and font loading that follows the constructor is the unnamed function 0x448f40.
//
// Not ported (registration; the retail partner.xml has NoReg true, so the original makes none of it either): the
// "REGISTER TYPERSHARK!" button (id 400, +0x6c, with the images title_register and title_register_over, placed at
// (0xcd, 0x15a, 0xe1, 0x1c)), the registered / trial stamps (+0x64 and +0x68, images stamp_registered and stamp_trial,
// drawn at (0x13, 0x7e)) and the "%d minute(s) remaining" text under the stamp (the MaxTime trial).

#include "Widget.h"

namespace Sexy
{

class ButtonWidget;
class Image;
class ImageFont;
class WinWSApp;

class TitleScreen : public Widget
{
public:
	enum
	{
		PLAY_BUTTON_ID = 0x191			// WinWSApp::ButtonDepress shows the map screen
	};

public:
	Image*					mTitleImage;	// +0x58 images\titlescreen (owned)
	Image*					mBarAndLogo;	// +0x5c images\barandlogo (owned)
	Image*					mGoldBar;		// +0x60 images\goldbar (owned): the loading bar, drawn as wide as the progress
	// +0x64 mStampRegistered, +0x68 mStampTrial, +0x6c mRegisterButton: not ported
	ImageFont*				mFont;			// +0x70 data\12ptfont.txt (owned), the play button's font
	WinWSApp*				mApp;			// +0x74 constructor argument
	ButtonWidget*			mPlayButton;	// +0x78 "CLICK HERE TO CONTINUE" (id 0x191), owned; hidden and disabled until the loading is over

public:
	TitleScreen(WinWSApp* theApp);									// 0x448e10
	virtual ~TitleScreen() override;								// 0x4414c0 (deleting 0x441ff0), slot 0

	void					AddControls();							// 0x448f40 loads the images and the font; Popup and mLoadingFailed when one is missing

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x449400 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4415d0 slot 2
	virtual void			Draw(Graphics* g) override;				// 0x4495c0 slot 16
};

}

#endif
