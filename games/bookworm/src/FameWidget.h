#ifndef __BOOKWORM_FAMEWIDGET_H__
#define __BOOKWORM_FAMEWIDGET_H__

// FameWidget.cpp (0x421860-0x428f50): the Hall of Fame. A full-screen page with the app's four high-score
// tables (Classic and Action top 10, the 3 best and the 3 longest words), the entries of the game just
// played pulsing with a shadow, and one continue button. BookWormApp::ShowHallOfFame creates it (from
// the game selector, or after game over from the LevelUpWidget); it pauses the board and plays the
// hall-of-fame music while it is up.
// The original is also the internet high-score page (mode 1: download/submit over HTTP, tab buttons and
// period checkboxes, wait/error dialogs, a hidden resubmit button). Internet scores are off in the
// registered build (partner.xml NoInternetScores): none of it is ported. The tab buttons and checkboxes
// are created hidden by the local page too and stay hidden; they are kept as in the original.
// Original: class Sexy::FameWidget : public Widget, public ButtonListener (+0x58), public CheckboxListener
// (+0x5c), sizeof 0x2a0, vftable 0x4fc8dc (53 slots: 47 DrawLocalScores, 48 DrawInternetScores, 49
// ShowInternetControls, 50 SetTab, 51 SetPeriod, 52 SetTabAndPeriod), ButtonListener 0x4fc8c0,
// CheckboxListener 0x4fc8b8. Offsets are absolute (the 2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/fame.md section 1.

#include "Widget.h"
#include "ButtonListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class BookWormApp;
class Graphics;
class Image;
class ButtonWidget;
class Checkbox;
class WidgetManager;

class FameWidget : public Widget, public ButtonListener, public CheckboxListener
{
public:
	enum
	{
		NUM_FAME_TABS = 3,                  // internet tables: classic, action, best words
		NUM_FAME_PERIODS = 3,               // internet periods: today, this month, all-time

		FAME_ID_CONTINUE = 7,               // mContinueButton
		FAME_ID_INTERNET = 8,               // the internet button (not ported)
		FAME_ID_TAB_CLASSIC = 0x13,         // mTabButtons[0..2]
		FAME_ID_TAB_ACTION = 0x14,
		FAME_ID_TAB_WORDS = 0x15,
		FAME_ID_PERIOD_TODAY = 0x17,        // mPeriodBoxes[0..2]
		FAME_ID_PERIOD_MONTH = 0x18,
		FAME_ID_PERIOD_ALLTIME = 0x19,

		FAME_MODE_LOCAL = 0,                // mMode: the Hall of Fame
		FAME_MODE_INTERNET = 1              // the internet page (not ported)
	};

	// +0x58 ButtonListener vptr, +0x5c CheckboxListener vptr
	BookWormApp*	mApp;                               // +0x060
	int				mMode;                              // +0x064 FAME_MODE_LOCAL; only the internet code sets 1
	int				mTab;                               // +0x068 internet table 0..2 (SetTab)
	int				mPeriod;                            // +0x06c internet period 0..2 (SetPeriod)
	// +0x070 int mMonth, +0x074 int mNetState, +0x078 bool (written by the ctor only), +0x07c/+0x098/+0x0b4
	// std::string error/warning/warning-header texts of a server reply, +0x0d0 int HTTP error code: internet
	// high scores, not ported
	// +0x0d4 bool "has a game to submit" (board game log non-empty and score > 0), +0x0d5 bool "game log
	// signed": internet score submission, not ported
	// +0x0d8 HTTPTransfer, +0x1d4/+0x1f8/+0x21c std::multiset<HighScoreEntry>[3] downloaded tables: internet,
	// not ported
	Image*			mTabImages[NUM_FAME_TABS];          // +0x240 ihs_tab_names rows 0..2 (CopyImage slices, owned)
	Image*			mTabOverImages[NUM_FAME_TABS];      // +0x24c ihs_tab_names_hilite rows 0..2 (owned)
	Image*			mTabSelectedImages[NUM_FAME_TABS];  // +0x258 ihs_tab_names rows 3..5 (owned)
	Image*			mIndicatorOffImage;                 // +0x264 ihs_tab_indicator top half (owned)
	Image*			mIndicatorOnImage;                  // +0x268 ihs_tab_indicator bottom half (owned)
	// +0x26c ImagelessButton* resubmit (id 0, never visible; its ButtonDepress re-posts the first line of
	// thing.txt to the score server): not ported
	ButtonWidget*	mContinueButton;                    // +0x270 id 7, button_fame_continue
	// +0x274 ButtonWidget* the internet button (id 8), created only when IsInternetScoresEnabled(), which is
	// false in the registered build: not ported
	ButtonWidget*	mTabButtons[NUM_FAME_TABS];         // +0x278 ids 0x13..0x15, hidden on the local page
	Checkbox*		mPeriodBoxes[NUM_FAME_PERIODS];     // +0x284 ids 0x17..0x19, hidden on the local page
	// +0x290 WaitDialog* (dialog 25 while a transfer runs), +0x294 std::list of new internet places:
	// internet, not ported

public:
	FameWidget(BookWormApp* theApp);                                        // 0x428520
	virtual ~FameWidget();                                                  // 0x4270d0 (scalar deleting 0x428f20) slot 0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;        // 0x421cb0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;    // 0x421dc0 slot 2
	virtual void	SetVisible(bool isVisible) override;                    // 0x421990 slot 4
	virtual void	SetDisabled(bool isDisabled) override;                  // 0x421950 slot 10
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;// 0x421a50 slot 13
	virtual void	Draw(Graphics* g) override;                             // 0x421f00 slot 16
	virtual void	Update() override;                                      // 0x427f30 slot 17

	virtual void	DrawLocalScores(Graphics* g);                           // 0x424c30 slot 47
	// slot 48 DrawInternetScores (0x425880): internet, not ported
	virtual void	ShowInternetControls(bool show);                        // 0x4219e0 slot 49
	virtual void	SetTab(int theTab);                                     // 0x421860 slot 50
	virtual void	SetPeriod(int thePeriod);                               // 0x421910 slot 51
	virtual void	SetTabAndPeriod(int theTab, int thePeriod);             // 0x421920 slot 52

	// ButtonListener (+0x58)
	virtual void	ButtonPress(int theId) override;                        // 0x4220c0 (+0x58) slot 3
	virtual void	ButtonDepress(int theId) override;                      // 0x426c10 (+0x58) slot 4
	// CheckboxListener (+0x5c)
	virtual void	CheckboxChecked(int theId, bool checked) override;      // 0x421f50 (+0x5c) slot 0

	// Internet only, not ported: ShowWaitDialog 0x423fd0, StartNetTransfer 0x4241d0, MarkNewPlaces
	// 0x4249d0, ShowErrorDialog 0x4262b0, ShowWarningDialog 0x4264f0, AddNewPlace 0x426ab0, AddNetScore
	// 0x427550, ParseServerReply 0x427650, KillErrorDialog 0x421f30, KillWarningDialog 0x421f40, the free
	// StringToLowerInPlace 0x422f20 and MapTableIndex 0x4216e0.

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
