#ifndef __BIGMONEY_GAMESELECTOR_H__
#define __BIGMONEY_GAMESELECTOR_H__

// GameSelector.cpp: the game type (Action / Strategy / Puzzle) and skill (1..3) chooser panel on the right
// of the screen, with the START bar and Mr. Moneybags' side-bar overlay. It rolls down over a finished
// board (SlideIn) and rolls up when a game or the help screen starts (SlideOut), animating a screen
// snapshot.
// Original: class Sexy::GameSelector : public Widget, public ButtonListener, sizeof 0xe0 (new in
// WinBMApp::ShowGameSelector 0x41b840), vftable 0x4f5c8c (57 slots: new virtuals 54-56), ButtonListener
// vftable 0x4f5c68. Offsets are absolute (Widget is 0x80 bytes in 1.2).
// Behaviour spec: re/bigmoney/analysis/screens.md.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinBMApp;
class Graphics;
class WidgetManager;
class ButtonWidget;
class DDImage;
class LockableButton;
class OverlayWidget;
class StartButton;

class GameSelector : public Widget, public ButtonListener
{
public:
	enum
	{
		START_BUTTON_ID = 0,        // mStartButton and mButton0
		BUTTON1_ID = 1,
		BUTTON2_ID = 2,
		SKILL_BUTTON_ID = 11,       // 11..13 = skill 1..3
		GAMETYPE_BUTTON_ID = 20,    // 20..22 = GameType 0..2
		PANEL_X = 173,              // selectback.jpg is drawn at (173, mPanelY)
		SLIDE_TOP_H = 58,           // snapshot strips while sliding: top 0..58,
		SLIDE_MID_H = 362,          // stretched middle 58..420,
		SLIDE_BOTTOM_SRC_Y = 420,   // bottom 420..477
		SLIDE_BOTTOM_H = 57
	};

	// +0x080 ButtonListener vfptr
	WinBMApp*		mApp;                   // +0x084
	int				mSkill;                 // +0x088 1..3, -1 = none (StartGame copies it to app->mSkillLevel)
	int				mGameType;              // +0x08c GameType, -1 = none (StartGame copies it to app->mGameType)
	int				mPanelY;                // +0x090 y of the panel and its buttons; always 0
	int				mUnused98[2];           // +0x098 zeroed, never read
	bool			mSlidingIn;             // +0x0a0
	bool			mSlidingOut;            // +0x0a1
	DDImage*		mSnapshot;              // +0x0a4 screen capture drawn while sliding
	int				mMidHeight;             // +0x0a8 stretched middle strip height 0..362 (+-8 per update)
	int				mSlideY;                // +0x0b0 top strip y (-100 -> 0 sliding in; 0, -2 per update sliding out)
	StartButton*	mStartButton;           // +0x0b4 id 0, Rect(180, 429, 455, 38)
	ButtonWidget*	mButton0;               // +0x0b8 id 0, never resized (inert leftover)
	ButtonWidget*	mButton1;               // +0x0bc id 1, never resized (inert); StartGame disables it
	ButtonWidget*	mButton2;               // +0x0c0 id 2, never resized (inert)
	LockableButton*	mTypeButtons[3];        // +0x0c4 ids 20..22, Rect(178 + 154i, 58, 154, 266)
	ButtonWidget*	mSkillButtons[3];       // +0x0d0 ids 11..13, Rect({29,186,339}[i] + 173, 359, 113, 52)
	OverlayWidget*	mOverlay;               // +0x0dc full-screen left bar + floating logo; GameSelector drives its mAlpha

public:
	GameSelector(WinBMApp* theApp);                                 // 0x4107c0
	virtual ~GameSelector();                                        // 0x40fb90 (scalar deleting dtor 0x410bd0) slot 0
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;  // 0x4100b0 slot 7
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x410140 slot 8
	virtual void	Update() override;                                       // 0x40fe90 slot 9
	virtual void	Draw(Graphics* g) override;                              // 0x410bf0 slot 13
	virtual void	OrderInManagerChanged() override;                        // 0x4101d0 slot 15
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x410d50 slot 25
	virtual void	StartGame();                                    // 0x410340 slot 54 (new)
	virtual void	SlideIn();                                      // 0x4103e0 slot 55 (new)
	virtual void	SlideOut();                                     // 0x410590 slot 56 (new)

	virtual void	ButtonPress(int theId, int theClickCount) override;      // 0x410330 ButtonListener slot 0 (click sound)
	virtual void	ButtonDepress(int theId) override;                       // 0x410730 ButtonListener slot 2

	void			SetGameType(int theType);                       // 0x40fcb0
	void			SetSkill(int theSkill);                         // 0x40fdc0
	void			LayoutButtons();                                // 0x410260 (called by Resize)

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
