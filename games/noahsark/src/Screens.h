#ifndef __NOAH_SCREENS_H__
#define __NOAH_SCREENS_H__

// The full-screen widgets of Noah's Ark Deluxe: TitleScreen (WinNoah.exe 0x41f830-0x4205b0), IntroWidget (the
// main menu, 0x413ea0-0x414f10), NextLevelWidget (the level-up panel, 0x414f10-0x416650) and HighScoreWidget
// (the game-over screen, 0x411a70-0x413550). Behaviour spec: docs/GAME_NOTES.md section 5.
// Each is a top-level widget (the 2002 framework has no child widgets) that ButtonListener-routes its buttons
// to itself (TitleScreen, NextLevelWidget, HighScoreWidget) or to the app (IntroWidget).

#include "Widget.h"
#include "ButtonListener.h"
#include "HyperlinkWidget.h"
#include "DialogButton.h"
#include <string>
#include <vector>

namespace Sexy
{

class NoahApp;
class MemoryImage;

// 0x41f830: the title picture, a bar that fills as the sounds load, and the link that starts the game
class TitleScreen : public Widget, public ButtonListener
{
public:
	NoahApp*			mApp;                   // +0x50
	double				mProgress;              // +0x58 the loading bar's fill, 0..1
	double				mFade;                  // +0x60
	bool				mLinkShown;             // +0x68 the play link has appeared
	HyperlinkWidget*	mPlayLink;              // +0x84 id 1, "click here to play"
	bool				mClicked;               // +0x70-ish: the link was clicked before loading had finished

public:
	TitleScreen(NoahApp* theApp);
	virtual ~TitleScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x413ea0: the main menu, five buttons over the game selector picture and a line about the one pointed at
class IntroWidget : public Widget
{
public:
	NoahApp*			mApp;                   // +0x4c
	DialogButton*		mActionButton;          // +0x50 id 5
	DialogButton*		mStrategyButton;        // +0x54 id 6
	DialogButton*		mPuzzleButton;          // +0x58 id 7
	DialogButton*		mContinueButton;        // +0x5c id 8 (only with a saved game)
	DialogButton*		mQuitButton;            // +0x60 id 9
	int					mHover;                 // +0x64 which button the mouse is on: 0 none, 1..5
	int					mAlpha;                 // +0x68 the line's fade-in

public:
	IntroWidget(NoahApp* theApp);
	virtual ~IntroWidget();

	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
};

// 0x414f10: after a level, or after the bonus round
class NextLevelWidget : public Widget, public ButtonListener
{
public:
	NoahApp*			mApp;                   // +0x50
	HyperlinkWidget*	mNextLink;              // +0x54 id 0 "start the next level"
	MemoryImage*		mBackground;            // +0x58 the panel, drawn once
	int					mSeconds;               // +0x5c time the level took
	int					mTip;                   // +0x60 which tip (0..3)
	bool				mBonus;                 // +0x64 after the bonus round
	int					mSaved;                 // +0x6c animals saved on the level

public:
	NextLevelWidget(NoahApp* theApp);
	virtual ~NextLevelWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x411a70: the game-over screen with the five best scores
class HighScoreWidget : public Widget, public ButtonListener
{
public:
	NoahApp*			mApp;                   // +0x50
	HyperlinkWidget*	mMenuLink;              // +0x54 id 1 "return to main menu"
	std::string			mNames[5];              // +0x58 (0x10 each)
	int					mScores[5];             // +0xa8
	bool				mChecked;               // +0xbc the rank has been looked at
	double				mFade;                  // +0xc0 0..255

public:
	HighScoreWidget(NoahApp* theApp);
	virtual ~HighScoreWidget();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;

	void				Load();                         // 0x411ea0
	void				Save();                         // 0x412060
	int					RankOf(int theScore);           // 0x412680: 1..5 for a place, 0 for none
	void				AddScore(const std::string& theName); // 0x413550: put the game's score in
	void				CheckRank();                    // 0x412240
};

// 1st, 2nd, 3rd, 4th (0x4124d0)
std::string Ordinal(int theNumber);

}

#endif
