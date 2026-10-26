#ifndef __PIXELUS_LOADINGSCREEN_H__
#define __PIXELUS_LOADINGSCREEN_H__

// LoadingScreen.cpp: the loading screen (Pixelus.exe 0x418960 and the Nc layer's 0x4e77e0-0x4e7700). Original:
// Nuclide's NcLoadingScreen : Widget (vftable 0x528630, ButtonListener part 0x528610, sizeof 0x110) built from a
// settings block, and the game's subclass (0x50e1e0) that only places the play link. The port keeps one class.
// The settings block of Pixelus (0x418960): the two link/status fonts data\Praitor24.txt (status text) and
// data\Praitor36.txt (links), the link colour (235,235,235) and its hover colour (white), the text's top 0x1ae
// and the loading bar's top 0x1bb. The trial and registration side of the Nc screen (trial counters, the register
// dialog, "UNPACKING SOUNDS") is for the shop version and not ported: this game is always fully registered.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class NcApp;
class Image;
class ImageFont;
class HyperlinkWidget;

class LoadingScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		REGISTER_LINK_ID = 0,
		PLAY_LINK_ID = 1
	};

	NcApp*			mApp;               // +0xd8
	int				mDelay;             // +0xd0 updates the finished bar stays before it fades (100)
	double			mFade;              // +0xc8 alpha of the finished bar, 255 -> 0
	double			mProgress;          // +0xe0
	double			mProgressCap;       // +0xe8
	bool			mLoadDone;          // +0xf0
	bool			mPlayRequested;     // +0xf1 (the register link; unused here)
	Image*			mImages[3];         // +0x108 NcFramework\images\ loadinggraphic, loadingbar, loadingbarempty
	ImageFont*		mStatusFont;        // +0x100 Praitor24
	ImageFont*		mLinkFont;          // +0x104 Praitor36
	HyperlinkWidget* mRegisterLink;     // +0xf8
	HyperlinkWidget* mPlayLink;         // +0xfc

public:
	LoadingScreen(NcApp* theApp);       // 0x4e77e0 with the settings of 0x418960
	virtual ~LoadingScreen();           // 0x4e6a50

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;       // 0x4e67b0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x4e67e0
	virtual void	Draw(Graphics* g) override;                                     // 0x4e6f30
	virtual void	Update() override;                                              // 0x418820 (0x4e6f70)
	virtual void	ButtonDepress(int theId) override;                              // 0x4e73c0

	void			DrawBar(Graphics* g);                                           // 0x4e6810
};

}

#endif
