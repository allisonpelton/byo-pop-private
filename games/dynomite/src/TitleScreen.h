#ifndef __TITLESCREEN_H__
#define __TITLESCREEN_H__

// The title screen (vtables 0x4fcd98 and 0x4fcd88 for its ButtonListener; 0xa8 bytes): the title picture, the
// loading bar, the version, and the play link, which appears when the loading is done. The port plays as the registered
// copy: the original's parts for an unregistered one (a register link with changing texts, the free plays left, a quit
// link once they ran out, and a bar slowed by the number of plays) are taken out with their fields.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class FossilizedApp;
class HyperlinkWidget;
class Pic;

class TitleScreen : public Widget, public ButtonListener
{
public:
	FossilizedApp*		mApp;					// +0x5c
	double				mProgress;				// +0x60 the bar
	bool				mLoadingDone;			// +0x70
	HyperlinkWidget*	mPlayLink;				// +0x84 id 1
	int					mUnused90;				// +0x90
	Pic*				mCobrand;				// +0x9c
	bool				mCobranded;				// +0xa0

	TitleScreen(FossilizedApp* theApp);			// 0x438b20
	virtual ~TitleScreen();						// 0x4388d0

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x438970
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4389b0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4389f0
	virtual void		Draw(Graphics* g) override;										// 0x4391a0
	virtual void		Update() override;												// 0x439b20
	virtual void		ButtonDepress(int theId) override;								// 0x438ab0
};

}

#endif
