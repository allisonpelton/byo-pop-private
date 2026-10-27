#ifndef __ROCKETMANIA_DIALOGS_H__
#define __ROCKETMANIA_DIALOGS_H__

// The dialogs of Rocket Mania Deluxe. All of them are 2003 dialogs (id + 2000 / + 3000 for the dialog's own buttons,
// docs/FRAMEWORK_2003.md) drawn by the game itself: the dialog picture with a title in the Showcard font, the
// game's own buttons (DialogButton, the dark button picture) and an app-wide click sound.

#include "Compat2004.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "CheckboxListener.h"
#include "SliderListener.h"
#include "EditListener.h"
#include "EditWidget.h"
#include "HyperlinkWidget.h"

namespace Sexy
{

class RocketApp;
class MTRand;

// 0x4118d0: a dialog with a black field behind its picture and a phase that goes round once in 2000 updates (the base of
// the options dialog)
class MoneyDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	double				mPhase;                 // +0xf0

public:
	MoneyDialog(RocketApp* theApp, int theId, bool isModal, const SexyString& theHeader, const SexyString& theLines,
		const SexyString& theFooter, int theButtonMode);

	virtual void		Update() override;      // 0x411b20
	virtual void		Draw(Graphics* g) override;     // 0x411980
};

// 0x403990: id 7 "Credits", opened over the options dialog; the picture, the names of the makers and a DONE button
class CreditsDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	double				mPhase;                 // +0xf0 these three move on in every update; nothing reads them
	double				mScroll1;               // +0xf8
	double				mScroll2;               // +0x100

public:
	CreditsDialog(RocketApp* theApp);

	virtual int			GetPreferredHeight(int theWidth) override;     // 0x403c10
	virtual void		Draw(Graphics* g) override;                    // 0x403c20
	virtual void		Update() override;                             // 0x4042c0
};

// 0x411ba0: id 1 "Options": the volumes, the screen mode, the cursors, the hints; the DONE button applies them
// (RocketApp::ButtonDepress). The original also lists a register button (the game is registered), an update check
// (online) and the partner links (the user's decision for the ports, 2026-10-24): none of them is ported.
class OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		CHECK_FULLSCREEN = 0,
		CHECK_CURSORS = 1,
		CHECK_HINTS = 2,
		CHECK_HARDWARE = 3,
		BUTTON_CREDITS = 6,
		BUTTON_RESET_HINTS = 0xe
	};

	bool				mFromMenu;              // +0x100 opened from the menu, not from a game
	int					mLastSoundUpdate;       // +0x104 the update at which the sound slider last clicked
	Slider*				mMusicSlider;           // +0x11c
	Slider*				mSfxSlider;             // +0x120
	Checkbox*			mFullscreenCheckbox;    // +0x124
	Checkbox*			m3DCheckbox;            // +0x128 hardware acceleration
	Checkbox*			mCursorsCheckbox;       // +0x12c
	Checkbox*			mHintsCheckbox;         // +0x130 "disable hints"
	DialogButton*		mResetHintsButton;      // +0x138
	DialogButton*		mCreditsButton;         // +0x13c

public:
	OptionsDialog(RocketApp* theApp, bool theFromMenu);
	virtual ~OptionsDialog();

	virtual void		SetColor(int theIdx, const Color& theColor) override;               // 0x4127c0
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;          // 0x412860
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;      // 0x412940
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x412a20
	virtual int			GetPreferredHeight(int theWidth) override;                         // 0x412770
	virtual void		Draw(Graphics* g) override;                                        // 0x412c60
	virtual void		ButtonPress(int theId) override;                                   // 0x413470
	virtual void		ButtonDepress(int theId) override;                                 // 0x4134b0
	virtual void		SliderVal(int theId, double theVal) override;                      // 0x413040
	virtual void		CheckboxChecked(int theId, bool checked) override;                 // 0x4130b0
	using Widget::Resize;
};

// 0x404380: id 0xc "Difficulty": EASY, MEDIUM, HARD and CANCEL (the ids 0 to 3)
class DifficultyDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	DialogButton*		mButtons[4];            // +0xf0 easy, +0xf4 medium, +0xf8 hard, +0xfc cancel

public:
	DifficultyDialog(RocketApp* theApp);
	virtual ~DifficultyDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x402630: id 0xf "Continue": a saved game of the mode exists; NEW GAME or CONTINUE SAVED GAME
class ContinueDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0x100
	int					mMode;                  // +0xec the mode whose saved game it is
	DialogButton*		mNewButton;             // +0x104 id 0
	DialogButton*		mContinueButton;        // +0x108 id 1
	int					mSavedDifficulty;       // +0xf4 read from the save file (1 to 3)
	int					mSavedScore;            // +0xf8
	int					mSavedLevel;            // +0xfc the level index (shown plus one)

public:
	ContinueDialog(RocketApp* theApp, int theMode);
	virtual ~ContinueDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x4064e0: id 0xe "High Scores": the whole screen. Three lists of ten (classic on the left, arcade on the right, strategy
// below them in the middle) and under each the best score of the day. It keeps the lists in Players/hiscores.dat (saved
// as it goes away) and is also where a score is entered (the record dialog hands over its name, AddScore).
class HighScoreDisplayDialog : public Dialog2004
{
public:
	enum
	{
		NUM_MODES = 3,
		NUM_RANKS = 10
	};

	RocketApp*			mApp;                   // +0xec
	HyperlinkWidget*	mMenuButton;            // +0xf0 the link under the lists (id 0)
	int					mOpenedFrom;            // +0xf4 0 after a game, 1 from the menu, 2 in a game, 3 from the options
	int					mTodayMode;             // +0xf8 the mode whose best score of the day was last beaten (-1)
	int					mHighlightMode;         // +0xfc the entry that was just added is drawn in colour (-1)
	int					mHighlightRank;         // +0x100
	bool				mCheckPending;          // +0x104 (nothing sets it)
	int					mTick;                  // +0x108 goes up every 20 updates; the colours of the list move with it
	SexyString			mTodayName[NUM_MODES];  // +0x10c the best score of the day's name, per mode
	int					mTodayScore[NUM_MODES]; // +0x13c and the score (0 when there is none)
	SexyString			mNames[NUM_MODES * NUM_RANKS];  // +0x148 mode * 10 + rank
	int					mScores[NUM_MODES * NUM_RANKS]; // +0x328

public:
	HighScoreDisplayDialog(RocketApp* theApp, int theOpenedFrom);
	virtual ~HighScoreDisplayDialog();

	int					FindRank(int theMode, int theScore);    // 0x407a30: 1 to 10, 0 when the score is not on the list
	void				AddScore(const SexyString& theName);    // 0x4084b0
	void				CheckScore();                           // 0x4073b0
	void				LoadScores();                           // 0x406c80
	void				SaveScores();                           // 0x407080

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;      // 0x407a60
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x407a90
	virtual void		Draw(Graphics* g) override;                                    // 0x407ac0
	virtual void		Update() override;                                             // 0x408450
	virtual void		ButtonPress(int theId) override;                               // 0x408800
	virtual void		ButtonDepress(int theId) override;                             // 0x408820
};

// 0x414060: id 3 "HIGH SCORE": the line about the score that was made, and a field for the name; OK enters it
class RecordScoreDialog : public MoneyDialog, public EditListener
{
public:
	EditWidget*			mEdit;                  // +0x100

public:
	RecordScoreDialog(RocketApp* theApp, const SexyString& theMessage);
	virtual ~RecordScoreDialog();

	virtual int			GetPreferredHeight(int theWidth) override;                         // 0x414370
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;          // 0x414390
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;      // 0x4143d0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x414400
	virtual void		Draw(Graphics* g) override;                                        // 0x414450
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;   // 0x414720
	using Widget::Resize;

private:
	void				DrawEditFrame(Graphics* g, const Rect& theRect);                   // 0x4145c0
};

// 0x408f60: id 0xb "Hint". Hints 0 (arcade), 1 (strategy) and 2 (classic) explain a mode before its first game, as a strip of
// small animated pictures under the mode's title; "Click Here To Play" starts the game that waited for it. Hints 3 (gem),
// 4 (bomb), 5 (clock), 6 (coin) and 7 (matches) are a smaller box that the board opens the first time a bonus item turns up.
// The checkbox "Don't show this again" turns the hint off for good (RocketApp::mHintFlags).
class HintDialog : public Dialog2004, public CheckboxListener
{
public:
	enum
	{
		HINT_ARCADE,
		HINT_STRATEGY,
		HINT_CLASSIC,
		HINT_GEM,
		HINT_BOMB,
		HINT_CLOCK,
		HINT_COIN,
		HINT_MATCHES
	};

	RocketApp*			mApp;                   // +0xf0
	Checkbox*			mCheckbox;              // +0xf4
	DialogButton*		mPlayButton;            // +0xf8
	int					mHint;                  // +0xfc
	int					mStartTick;             // +0x100 mUpdateCnt when it opened: the pictures' clock
	bool				mContinuing;            // +0x104 the game that waits is a saved one
	int					mLabelX;                // +0x108 where "Don't show this again" is written
	int					mLabelY;                // +0x10c
	int					mHelpWidth;             // +0x110 the width of a picture in the strip

public:
	HintDialog(RocketApp* theApp, int theHint, bool theContinuing);
	virtual ~HintDialog();

	static bool			IsModeHint(int theHint);        // 0x40c1c0
	void				Layout(bool theFull);           // 0x4094f0: the whole dialog (modes) or the small one (bonus items)

	virtual void		CheckboxChecked(int theId, bool checked) override;          // 0x40c170
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;   // 0x409470
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x4094b0
	virtual void		Update() override;                                          // 0x40c270
	virtual void		Draw(Graphics* g) override;                                 // 0x40aef0
	virtual void		ButtonPress(int theId) override;                            // 0x40c190
	virtual void		ButtonDepress(int theId) override;                          // 0x40c1e0

private:
	// the pieces of Draw; theTime counts from the picture's own start (it is negative before it)
	void				DrawTextLine(Graphics* g, const SexyString& theText, double theLine);  // 0x4095c0 and 0x409720: centred on line theLine
	void				DrawFadingText(Graphics* g, const SexyString& theText, int theX, int theY, int theAlpha);  // 0x409880
	void				DrawScene(Graphics* g, int theX, int theIndex);                          // 0x409b30: one of the nine help pictures
	void				DrawRotateScene(Graphics* g, int theX, int theTime);                     // 0x409b60
	void				DrawLanternsScene(Graphics* g, int theX, int theTime);                   // 0x409ef0
	void				DrawRocketsScene(Graphics* g, int theX, int theTime);                    // 0x40a0e0
	void				DrawMatchesScene(Graphics* g, int theX, int theTime);                    // 0x40a210
	void				DrawScreenFillScene(Graphics* g, int theX, int theTime);                 // 0x40a3c0
	void				DrawNewItem(Graphics* g, int theX, int theTime, int theImage);          // 0x40a580
	void				DrawMatchStrips(Graphics* g, int theX, int theTime);                     // 0x40a7a0
	void				DrawLaunchScene(Graphics* g, int theX, int theTime, bool theRockets, bool theArcade);  // 0x40a890
	void				DrawDragScene(Graphics* g, int theX, int theTime);                       // 0x40acc0
};

// 0x40c2c0, 0x110 bytes: id 0xd, the screen that the board puts up when a level is finished (kind 4: "Well Done!") and when
// the game is over (kind 5: "Congratulations!"). A rank picture and name, and under them a page of figures that turns every
// 150 updates (the score, the rank to come, the longest fuse, the largest launch and the items collected: the pages that do
// not apply to the mode or the level are skipped, Step); fireworks burst behind it from time to time. For a level the figures
// are the level's, for the game they are the sums (the dialog adds the level's into the sums when it opens). Its one link
// goes on to the next question (kind 4, Board::F4286b0) or to the high scores (kind 5). It also draws the pause: when the
// window loses the focus it darkens and says so, and its link goes away until the focus comes back.
class LevelUpDialog : public Dialog2004
{
public:
	enum
	{
		KIND_LEVEL = 4,
		KIND_GAME = 5
	};

	int					mStartTick;             // +0xec mUpdateCnt when it opened
	bool				mGameOver;              // +0xf0 the kind is KIND_GAME
	RocketApp*			mApp;                   // +0xf4
	int					mAnim;                  // +0xf8 goes up every 10 updates: the colour of the rank name moves with it
	MTRand*				mRand;                  // +0xfc the board's random generator (the fireworks)
	HyperlinkWidget*	mButton;                // +0x100 the link under the rank (id 0)
	int					mKind;                  // +0x104
	int					mPhase;                 // +0x108 the page: 0 the score, 1 the next rank, 2 bombs, 3 fuse, 4 coins, 5 rockets,
	                                            //        6 gems, 7 clocks, 8 matches, 9 the level
	int					mPauseAlpha;            // +0x10c 0 to 200: how dark the pause is

public:
	LevelUpDialog(RocketApp* theApp, int theKind, MTRand* theRand);
	virtual ~LevelUpDialog();

	void				Step();                                             // 0x40f5a0: on to the next page that applies

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;      // 0x40c970
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;                                  // 0x410340
	virtual void		Draw(Graphics* g) override;                         // 0x40f660
	virtual void		ButtonDepress(int theId) override;                  // 0x4102c0

private:
	// the pages, in the lower left of the dialog (centred on x = 0x61)
	void				DrawLine(Graphics* g, const SexyString& theText, int theY);
	void				DrawScorePage(Graphics* g);                         // 0x40c9c0
	void				DrawLevelPage(Graphics* g);                         // 0x40ce00
	void				DrawRankPage(Graphics* g, int thePointsToNext, int theScore);  // 0x40d100
	void				DrawBombsPage(Graphics* g);                         // 0x40ea00
	void				DrawFusePage(Graphics* g);                          // 0x40da40
	void				DrawCoinsPage(Graphics* g);                         // 0x40f280
	void				DrawRocketsPage(Graphics* g);                       // 0x40de60
	void				DrawGemsPage(Graphics* g);                          // 0x40e2a0
	void				DrawClocksPage(Graphics* g);                        // 0x40e5c0
	void				DrawMatchesPage(Graphics* g);                       // 0x40ee40
	void				DrawCount(Graphics* g, int theCount, unsigned int theSingular, unsigned int thePlural, int theY);
};

}

#endif
