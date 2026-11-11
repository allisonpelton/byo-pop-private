#ifndef __ALCHEMY_TITLESCREEN_H__
#define __ALCHEMY_TITLESCREEN_H__

// The title screen (class 0x4e4c50 widget + 0x4e4c30 listener, ctor 0x4103e2, size 0x88): the picture, the loading bar
// and, when the loading is done, the "CLICK HERE TO PLAY!" link. The original also has a registration link and a
// play-count nag for the trial; the port is a registered copy, so it has neither.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinAlchApp;
class HyperlinkWidget;

class TitleScreen : public Widget, public ButtonListener
{
public:
	WinAlchApp*			mApp;               // +0x5c
	double				mProgress;          // +0x60 the bar (0..1)
	bool				mDone;              // +0x70 the loading ended and the link is up
	HyperlinkWidget*	mPlayLink;          // +0x84 id 1

	TitleScreen(WinAlchApp* theApp);
	virtual ~TitleScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;     // 0x410286
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x4102b3
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x4102e0
	virtual void		Draw(Graphics* g) override;     // 0x410622
	virtual void		Update() override;              // 0x4109d8
	virtual void		ButtonDepress(int theId) override; // 0x41035e
};

}

#endif
