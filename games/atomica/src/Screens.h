#ifndef __ATOMICA_SCREENS_H__
#define __ATOMICA_SCREENS_H__

// The full-screen widgets of Atomica Deluxe: TitleScreen (Atomica.exe 0x42fb30-0x431270), the main menu and the
// game's other screens. Each is a top-level widget (the 2003 framework has no child widgets) that routes its
// buttons to itself.

#include "Widget.h"
#include "ButtonListener.h"
#include "HyperlinkWidget.h"
#include <string>

namespace Sexy
{

class AtomicaApp;
class Sprite;

// 0x42fe90: the title picture, a bar that fills as the game's files load, and the link that starts the game
class TitleScreen : public Widget, public ButtonListener
{
public:
	AtomicaApp*			mApp;                   // +0x5c
	Sprite*				mTitle;                 // +0x88 titlescreen
	Sprite*				mBar;                   // +0x8c progressbar
	Sprite*				mLogo;                  // +0x90 atomicatitle
	Sprite*				mDeluxe;                // +0x94 deluxe
	Sprite*				mPopCapLogo;            // +0x98 popcaplogo
	Sprite*				mPopCapUrl;             // +0x9c popcapurl
	Sprite*				mSponsor;               // +0xa0 sponsor
	HyperlinkWidget*	mPlayLink;              // +0x84 id 1 "click here to play"
	double				mProgress;              // +0x60 the loading bar's fill, 0..1
	double				mShown;                 // +0x68 how much of the bar the link has seen
	int					mHighest;               // +0x74
	bool				mLinkShown;             // +0x70
	bool				mClicked;               // +0x72 the link was clicked before loading had finished
	float				mSlide;                 // +0xac the Deluxe sign slides in from the left (starts at -800)
	int					mBlink;                 // +0x(DAT_004d40e4) a counter that runs 0..19
	std::string			mLoadingText;           // +0xc4

public:
	TitleScreen(AtomicaApp* theApp);
	virtual ~TitleScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

}

#endif
