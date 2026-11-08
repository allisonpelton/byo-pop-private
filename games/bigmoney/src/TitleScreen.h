#ifndef __BIGMONEY_TITLESCREEN_H__
#define __BIGMONEY_TITLESCREEN_H__

// TitleScreen.cpp: loading screen. Mr. Moneybags, the logo and the bottom bar slide in, the cigar tip and
// eyes glow, rows of dollar signs scroll, the "Deluxe" badge flashes in, the gold loading bar fills and
// the "CLICK HERE TO PLAY!" link appears.
// Original: class Sexy::TitleScreen : public Widget, public ButtonListener, sizeof 0x118 (new in
// WinBMApp::Init 0x41d9a0), vftable 0x4fbf0c (54 slots), ButtonListener vftable 0x4fbeec.
// Offsets are absolute (Widget is 0x80 bytes in 1.2). Behaviour spec: re/bigmoney/analysis/screens.md.
// The registered original's paths only: the registration code (the trial's play limit, the nag delay on the
// bar, the register link, the expiry and quit link, the unregistered status texts) is left out
// (GAME_NOTES.md, "Registration code").

#include "Widget.h"
#include "ButtonListener.h"
#include "Common.h"
#include "HDPixels.h"

#include <vector>

namespace Sexy
{

class WinBMApp;
class Graphics;
class WidgetManager;
class HyperlinkWidget;
class MemoryImage;

class TitleScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		PLAY_LINK_ID = 1
	};

	// +0x080 ButtonListener vfptr
	WinBMApp*		mApp;                   // +0x084
	double			mLoadProgress;          // +0x088 loading bar fraction 0..1 (exactly 1.0 when done)
	bool			mLoadDone;              // +0x098 bar complete and play link shown
	int				mLogoY;                 // +0x0b0 bigmoneylogo y offset, -239 -> 0
	double			mLogoAngle;             // +0x0b8 += 0.016 per update from update 25
	int				mMoneybagsDY;           // +0x0c0 mrmoneybags offset below its rest position, 403 -> 0
	double			mMoneybagsAngle;        // +0x0c8 += 0.02 per update
	int				mBarDY;                 // +0x0d0 barandlogo offset (moves the bar, loading bar and texts), 149 -> 0
	double			mBarAngle;              // +0x0d8 += 0.025 per update from update 50
	int				mStripScroll;           // +0x0e0 dollar rows scroll 64..0, -1 every 8 updates
	double			mCigarPhase;            // +0x0e8 += 0.05 every 8 updates, wraps above 1.0
	double			mEyePhase;              // +0x0f0 += 0.06 every 8 updates, wraps above 1.0
	HyperlinkWidget* mPlayLink;             // +0x0fc id 1, mFont1OutlineUnderlined, (32,255,32); shown when loaded
	MemoryImage*	mDeluxeImage;           // +0x100 IMAGE_DELUXOR with the luminosity boost applied
	ulong*			mDeluxeHSL;             // +0x104 IMAGE_DELUXOR as HSL (hue bits 0-7, saturation 8-15, luminosity 16-23)
	ulong*			mDeluxeWork;            // +0x108 scratch HSL buffer
	int				mDeluxeLum;             // +0x10c luminosity added to the badge, 256 -> 0 (-16 every 8 updates once shown)
	bool			mDeluxeDirty;           // +0x110 rebuild mDeluxeImage in Draw (port: initialised; the original leaves it unset but never reads it first)
	bool			mDeluxeShown;           // +0x111 set at update 180
	// port: the same for a mod's HD badge (HDPixels.h): its HSL copy and the edited HD pixels
	std::vector<ulong> mDeluxeHDHSL;
	HDPixels		mDeluxeHD;

public:
	TitleScreen(WinBMApp* theApp);                                  // 0x41b120
	virtual ~TitleScreen();                                         // 0x419630 (scalar deleting dtor 0x419760) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;  // 0x4196d0 slot 7
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x419700 slot 8
	virtual void	Update() override;                                       // 0x41a810 slot 9
	virtual void	Draw(Graphics* g) override;                              // 0x419950 slot 13
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x412c50 slot 25 (jmp Widget::Resize)
	virtual void	ButtonDepress(int theId) override;                       // 0x419730 ButtonListener slot 2

	using Widget::Resize;
};

}

#endif
