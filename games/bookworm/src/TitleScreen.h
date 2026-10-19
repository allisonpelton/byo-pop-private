#ifndef __BOOKWORM_TITLESCREEN_H__
#define __BOOKWORM_TITLESCREEN_H__

// TitleScreen.cpp: the title / loading screen. The logo slides down, the loading bar slides up and fills
// while the loading thread runs, and the play link (GT_TITLE_CLICK_TO_PLAY) appears when loading is done.
// Original: class Sexy::TitleScreen : public Widget, public ButtonListener, sizeof 0xc8 (new in
// BookWormApp::Init 0x414a10), vftable 0x5011fc (49 slots), ButtonListener vftable 0x5011dc (+0x58).
// Offsets are absolute (the 2004 Widget is 0x58 bytes). Behaviour spec: re/bookworm/analysis/screens.md.
// The port is the registered build (partner.xml NoReg: mIsRegistered and mBuildUnlocked are true), so the
// trial paths are not ported: the trial-expiry check and dialog (slot 47 ShowTrialExpiredDialog 0x4336f0),
// the register link's dialog and the unregistered status texts. The trial-only fields are kept for layout.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class HyperlinkWidget;
class ShadowFont;

class TitleScreen : public Widget, public ButtonListener
{
public:
	enum
	{
		REGISTER_LINK_ID = 0,
		PLAY_LINK_ID = 1
	};

	// +0x58 ButtonListener vfptr
	BookWormApp*	mApp;                   // +0x5c
	int				mLoadingBarY;           // +0x60 property LoadingBarY (default 389; partner.xml 330)
	// +0x64 pad
	double			mLoadProgress;          // +0x68 loading fraction 0..1, exactly 1.0 when done
	double			mNagProgress;           // +0x70 trial nag timer, += 0.01/mNagSeconds per update, max 1.0
	bool			mLoadDone;              // +0x78 bar complete, play link shown
	int				mShownTimesPlayed;      // +0x7c trial: play count shown by the nag text (only MarkDirty here)
	bool			mWantRegister;          // +0x80 trial: register link clicked
	bool			mTrialExpired;          // +0x81 trial: always false when registered; copied to app->mTimedOut
	int				mNagSeconds;            // +0x84 trial: 10, or mTimesPlayed + 5 after 10 plays; 1 when registered
	int				mMaxPlays;              // +0x88 property MaxPlays (default 10)
	int				mUnused8C;              // +0x8c never written or read
	int				mLogoY;                 // +0x90 logo y, -4 * logo height -> 0
	// +0x94 pad
	double			mLogoAngle;             // +0x98 += 0.016 per update from update 25
	int				mBarDY;                 // +0xa0 loading bar y offset, 2 * app height -> 0
	// +0xa4 pad
	double			mBarAngle;              // +0xa8 += 0.025 per update from update 50
	HyperlinkWidget* mRegisterLink;         // +0xb0 id 0 GT_TITLE_REGISTER_NOW, hidden when registered
	HyperlinkWidget* mPlayLink;             // +0xb4 id 1 GT_TITLE_CLICK_TO_PLAY, (255,255,0), shown when loaded
	ShadowFont*		mPlayFont;              // +0xb8 Lockergnome14Outline, red shadow at (1,1)
	ShadowFont*		mTextFont;              // +0xbc Verdana12Bold, black shadow at (1,1)
	ShadowFont*		mVersionFont;           // +0xc0 Verdana10Bold, black shadow at (1,1)
	// +0xc4 pad (sizeof 0xc8)

public:
	TitleScreen(BookWormApp* theApp);                               // 0x434380
	virtual ~TitleScreen();                                         // 0x433300 (scalar deleting dtor 0x433540) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x433470 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x4334b0 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x4334f0 slot 13 (shared jump to Widget::Resize)
	virtual void	Draw(Graphics* g);                              // 0x433900 slot 16
	virtual void	Update();                                       // 0x433e80 slot 17
	// slot 47 ShowTrialExpiredDialog 0x4336f0: trial only, not ported
	virtual void	DrawLoadingBar(Graphics* g);                    // 0x433570 slot 48

	virtual void	ButtonPress(int theId, int theClickCount);      // 0x401450 ButtonListener slot 2 (shared with BWGameSelector)
	virtual void	ButtonDepress(int theId);                       // 0x433500 ButtonListener slot 4

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
