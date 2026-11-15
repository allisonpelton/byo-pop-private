#ifndef __HEAVYWEAPON_TITLESCREEN_H__
#define __HEAVYWEAPON_TITLESCREEN_H__

// TitleScreen: the first screen. It plays intro.swf (a click with the focus skips it), then flashes white into the title
// picture, starts the loading and shows its bar; the logo fades in meanwhile. When the loading is done the bar slides
// away and "click here to play" appears, which opens the main menu.
// Original: a Widget of 0x88 bytes with a ButtonListener at +0x58 and a FlashListener at +0x5c, constructor 0x42ee70,
// vtable 0x52fc30 (listeners 0x52fc10 and 0x52fc08), destructor 0x42e6b0 (deleting 0x42eb30). +0x60, a copy of the
// app's completed loading tasks that nothing reads, and +0x70 and +0x74, which nothing uses, are left out.

#include "ButtonListener.h"
#include "FlashWidget.h"
#include "Widget.h"

namespace Sexy
{

class HeavyWeaponApp;
class HyperlinkWidget;

class TitleScreen : public Widget, public ButtonListener, public FlashListener
{
public:
	enum
	{
		BUTTON_PLAY = 0
	};

	int				mFade;				// +0x64 the white flash after the intro: 256 while it plays, then 2 less an update
	double			mLogoAngle;			// +0x68 the logo's fade-in, once the flash is over, up to pi
	int				mBarSlide;			// +0x78 the loading bar's slide off, once the loading is done, up to 304
	HeavyWeaponApp*	mApp;				// +0x7c
	HyperlinkWidget* mPlayButton;		// +0x80 "click here to play", shown when the bar is gone
	FlashWidget*	mFlashWidget;		// +0x84 the intro (gone once it has ended)

public:
	TitleScreen(HeavyWeaponApp* theApp);										// 0x42ee70
	virtual ~TitleScreen();														// 0x42e6b0 (deleting 0x42eb30)

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x42f050 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x42e760 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x42e860 slot 13
	virtual void	Draw(Graphics* g) override;									// 0x42eb60 slot 16
	virtual void	Update() override;											// 0x42e7a0 slot 17
	virtual void	GotFocus() override;										// 0x42ea90 slot 19
	virtual void	MouseDown(int x, int y, int theClickCount) override;		// 0x42ea60 slot 28
	virtual void	ButtonPress(int theId) override;							// 0x42e8e0 (listener slot 4)
	virtual void	ButtonDepress(int theId) override;							// 0x42e900 (listener slot 5)
	virtual void	FlashAnimEnded(int theId) override;							// 0x42eab0 (Flash listener slot 0)

	void			LoadingComplete();											// 0x42e840

	using Widget::Resize;
	using Widget::MouseDown;
	using ButtonListener::ButtonPress;
};

}

#endif
