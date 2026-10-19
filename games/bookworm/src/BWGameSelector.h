#ifndef __BOOKWORM_BWGAMESELECTOR_H__
#define __BOOKWORM_BWGAMESELECTOR_H__

// BWGameSelector.cpp: the main menu. The backdrop, the big Lex, four image buttons (Classic, Action, Hall
// of Fame, Quit), a speech bubble greeting the current player with a change-player link, and a scroll-like
// transition: the screen rolls up when a game starts and unrolls when the player comes back from one.
// Original: class Sexy::BWGameSelector : public Widget, public ButtonListener, sizeof 0xb0 (new in
// BookWormApp::ShowGameSelector 0x410f00), vftable 0x4f93e4 (50 slots: Widget's 47 + 47..49),
// ButtonListener vftable 0x4f93c8 (+0x58). Offsets are absolute (the 2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/screens.md.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class BookWormApp;
class BigLex;
class ButtonWidget;
class HyperlinkWidget;
class DDImage;
class Graphics;
class WidgetManager;

class BWGameSelector : public Widget, public ButtonListener
{
public:
	enum
	{
		NOT_YOU_LINK_ID = 0,
		CLASSIC_BUTTON_ID = 1,
		ACTION_BUTTON_ID = 2,
		HALL_OF_FAME_BUTTON_ID = 3,
		QUIT_BUTTON_ID = 4
	};

	enum
	{
		TRANSITION_TOP_H = 58,              // snapshot strips during the transitions: top 0..58,
		TRANSITION_BAND_H = 362,            // middle band 58..420 (squashed to mBandHeight),
		TRANSITION_BOTTOM_SRC_Y = 420,      // bottom 420..477
		TRANSITION_BOTTOM_H = 57
	};

	// +0x58 ButtonListener vfptr
	BookWormApp*	mApp;                   // +0x5c
	BigLex*			mBigLex;                // +0x60 the big Lex at (70, 231, 133, 123); a top-level widget of its own
	ButtonWidget*	mClassicButton;         // +0x64 id 1 at (360, 50)
	ButtonWidget*	mActionButton;          // +0x68 id 2 at (340, 145)
	ButtonWidget*	mHallButton;            // +0x6c id 3 at (376, 240)
	ButtonWidget*	mQuitButton;            // +0x70 id 4 at (352, 350)
	HyperlinkWidget* mNotYouLink;           // +0x74 id 0 change-player link (GT_SELECTOR_NOT_YOU) in the bubble
	int				mBubbleX;               // +0x78 14
	int				mBubbleY;               // +0x7c 114
	int				mGameOption;            // +0x80 copied from/to app->mGameOption (meaning unknown)
	int				mGameMode;              // +0x84 GameMode, copied from/to app->mGameMode
	int				mBackdropY;             // +0x88 backdrop y, 0
	int				mUnused8C;              // +0x8c never used
	double			mUnused90;              // +0x90 zeroed by the ctor, never read
	bool			mOpening;               // +0x98 open transition running (coming back from a game)
	bool			mClosing;               // +0x99 close transition running (a game starts); the app tests it
	DDImage*		mSnapshot;              // +0x9c screen snapshot drawn during the transitions
	int				mBandHeight;            // +0xa0 height the middle band is drawn at, 0..362
	// +0xa4 pad
	int				mStripY;                // +0xa8 y offset of the whole snapshot
	// +0xac pad (sizeof 0xb0)

public:
	BWGameSelector(BookWormApp* theApp);                            // 0x401600
	virtual ~BWGameSelector();                                      // 0x401000 (scalar deleting dtor 0x401720) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x402430 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x401370 slot 2
	virtual void	SetDisabled(bool isDisabled);                   // 0x4013f0 slot 10
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x4334f0 slot 13 (shared jump to Widget::Resize)
	virtual void	Draw(Graphics* g);                              // 0x402cb0 slot 16
	virtual void	Update();                                       // 0x402190 slot 17
	virtual void	StartGame();                                    // 0x401460 slot 47
	virtual void	StartOpenTransition();                          // 0x4010c0 slot 48
	virtual void	StartCloseTransition();                         // 0x4011f0 slot 49

	void			SetGameMode(int theMode) { mGameMode = theMode; }           // 0x401340
	void			SetGameOption(int theOption) { mGameOption = theOption; }   // 0x401350
	void			DrawBubble(Graphics* g);                        // 0x402a50

	virtual void	ButtonPress(int theId, int theClickCount);      // 0x401450 ButtonListener slot 2 (shared with TitleScreen)
	virtual void	ButtonDepress(int theId);                       // 0x4014c0 ButtonListener slot 4

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
