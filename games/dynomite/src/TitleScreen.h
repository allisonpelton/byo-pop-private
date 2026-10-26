#ifndef __TITLESCREEN_H__
#define __TITLESCREEN_H__

// The title screen (vtables 0x4fcd98 and 0x4fcd88 for its ButtonListener; 0xa8 bytes): the title picture, the
// loading bar, the version, and three links (register, play, quit). The play link appears when the loading is done.
// The port's framework is always registered (SexyApp::mIsRegistered), so the parts for an unregistered copy (the
// changing register texts, the free plays left, the slowed bar) are left out; the fields they use are kept.

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
	double				mFakeProgress;			// +0x68 the unregistered copy's slowed bar
	bool				mLoadingDone;			// +0x70
	int					mFreePlaysShown;		// +0x74
	bool				mRegisterClicked;		// +0x78
	bool				mPlaysExpired;			// +0x79
	int					mSlowness;				// +0x7c
	HyperlinkWidget*	mRegisterLink;			// +0x80 id 0
	HyperlinkWidget*	mPlayLink;				// +0x84 id 1
	HyperlinkWidget*	mQuitLink;				// +0x88 id 2
	int					mUnused90;				// +0x90
	int					mRegisterTextIdx;		// +0x94
	int					mRegisterTextCountdown;	// +0x98
	Pic*				mCobrand;				// +0x9c
	bool				mCobranded;				// +0xa0
	bool				mShowRegistration;		// +0xa1

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
