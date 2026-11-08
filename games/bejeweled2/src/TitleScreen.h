#ifndef __TITLESCREEN_H__
#define __TITLESCREEN_H__

// TitleScreen.h: the screen that shows while the game loads (Sexy::TitleScreen, 0x5d98fa-0x5daef5; the object is 0x31c
// bytes): the logos fly in, the loading bar fills and turns purple, and the "Play" link appears. It leaves when the
// player clicks the link. The original's trial and registration parts (play counts, the "register now" dialogs) are not
// ported.

#include "Widget.h"
#include "ButtonListener.h"
#include "CurvedVal.h"

namespace Sexy
{

class WinDMApp;
class HyperlinkWidget;
class Image;

class TitleScreen : public Widget, public ButtonListener
{
public:
	WinDMApp*		mApp;               // +0x8c (the ButtonListener subobject is at +0x88)
	double			mLoadProgress;      // +0x90 how far the loading has come, 0 to 1
	bool			mLoadedFlag;        // +0xa0 the loading is done and the play link shows
	double			mFade;              // +0xb0 (counts up to 1, never read)
	double			mLinkGlow;          // +0xb8 the glow of the play link: 0 to 1 as the mouse is over it

	// the counters hold an animation back for that many updates; the curves then run on
	int				mLogoDelay;         // +0xcc
	CurvedVal		mLogoY;             // +0xd0 the logo flies in: 800 to 0
	CurvedVal		mLogoAlpha;         // +0x100
	int				mLogo2Delay;        // +0x130
	CurvedVal		mLogo2Alpha;        // +0x138 "2"
	int				mLogo2GlowDelay;    // +0x168
	CurvedVal		mLogo2Glow;         // +0x170 the "2" lights up
	int				mBarDelay;          // +0x1a0
	CurvedVal		mBarAlpha;          // +0x1a8 the loading bar and the PopCap logo fade in
	int				mUnusedDelay;       // +0x1d8
	CurvedVal		mUnused;            // +0x1e0 (counts up, never read)
	int				mDeluxeDelay;       // +0x210
	CurvedVal		mDeluxeAlpha;       // +0x218 "Deluxe"
	int				mDeluxeGlowDelay;   // +0x248
	CurvedVal		mDeluxeGlow;        // +0x250 "Deluxe" lights up
	int				mBarFlashDelay;     // +0x280 (the loading bar's text waits for it)
	int				mBarFlashStart;     // +0x284 updates before the loading is shown as done
	CurvedVal		mBarFlash;          // +0x288 the flash at the bar's end
	float			mBarPurple;         // +0x2b8 the bar turns from blue to purple, 0 to 1
	bool			mLoadingCompleted;  // +0x2bc the player clicked the play link: the screen slides away
	CurvedVal		mSlide;             // +0x2c0 the slide away: 0 to 400
	HyperlinkWidget* mPlayLink;         // +0x2f0
	Image*			mLogo2GlowImage;    // +0x2f4 the "2" as a grey image, drawn additively
	Image*			mDeluxeGlowImage;   // +0x2f8
	bool			mSettled;           // +0x318 the animation is over and the window had the focus: nothing animates
	                                    // (the backdrop and this screen) while the focus is away

public:
	TitleScreen(WinDMApp* theApp);                                  // 0x5daef5
	virtual ~TitleScreen();

	void			SetupVals();                                    // 0x5d98fa
	Image*			MakeGlowImage(Image* theImage);                 // 0x5d98a5
	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5d9ac8
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5d9aeb
	virtual void	ButtonDepress(int theId) override;              // 0x5d9b2b
	virtual void	Draw(Graphics* g) override;                     // 0x5d9b7b
	virtual void	Update() override;                              // 0x5da77e
};

}

#endif
