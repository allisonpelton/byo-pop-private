#ifndef __MUMMYMAZE_SCREENS_H__
#define __MUMMYMAZE_SCREENS_H__

// The full-screen widgets of Mummy Maze Deluxe: TitleScreen (WinMM.exe 0x42a5f0), ScrollScreen (0x42b390, the parchment
// that slides down over the menu and the panels), MainMenu (0x4165a0), GamePanel (0x412ea0), SidePanel (0x4291d0) and
// the screens that follow a level. Behaviour spec: docs/GAME_NOTES.md.
// Each is a top-level widget (the 2002 framework has no child widgets); the buttons they make are top-level too.

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "HyperlinkWidget.h"
#include "DialogButton.h"
#include <string>
#include <vector>

namespace Sexy
{

class MummyApp;
class MemoryImage;

// 0x42a5f0: the title picture, a bar that fills as the sounds load, and the link that starts the game
class TitleScreen : public Widget, public ButtonListener
{
public:
	MummyApp*			mApp;                   // +0x50
	double				mProgress;              // +0x58 the loading bar's fill, 0..1
	double				mDelayProgress;         // +0x60
	bool				mPlayShown;             // +0x68 the play link has appeared
	int					mPlaysShown;            // +0x6c
	bool				mClicked;               // +0x70
	bool				mDelayed;               // +0x71
	int					mDivisor;               // +0x74
	HyperlinkWidget*	mRegisterLink;          // +0x78 id 0
	HyperlinkWidget*	mPlayLink;              // +0x7c id 1

public:
	TitleScreen(MummyApp* theApp);
	virtual ~TitleScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		ButtonPress(int theId) override;
	using Widget::Resize;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;
};

// 0x42b390: the parchment that scrolls down from the top of the window, with the torch beside it. Its state (+0x58): 0 comes
// down, 1 rests, 2 bounces down to 200, 3 holds, 4 goes back up, 5 goes up, 6 is gone.
class ScrollScreen : public Widget, public ButtonListener
{
public:
	MummyApp*			mApp;                   // +0x4c
	int					mOffset;                // +0x50 how far down the parchment hangs (0x1dc: out of sight)
	int					mTorchFrame;            // +0x54 0..16
	int					mScrollState;           // +0x58
	double				mScrollTime;            // +0x60

public:
	ScrollScreen(MummyApp* theApp);              // 0x42b390
	virtual ~ScrollScreen();

	virtual void		Update() override;       // 0x42b580
	virtual void		Draw(Graphics* g) override;  // 0x42b400
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;
};

// 0x4165a0: the main menu: classic mode, adventure, tutorial and quit
class MainMenu : public ScrollScreen
{
public:
	HyperlinkWidget*	mLinks[4];              // +0x6c .. ids 0 classic, 1 adventure, 2 quit, 3 tutorial
	double				mFade;                  // +0x80

public:
	MainMenu(MummyApp* theApp, int theEntry);
	virtual ~MainMenu();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
};

// 0x412ea0: the panel that comes down when the explorer is lost, or from the side panel: undo, try again, save and quit,
// abandon hope (the world map in an adventure)
class GamePanel : public ScrollScreen
{
public:
	HyperlinkWidget*	mLinks[4];              // +0x6c .. ids 0 undo move, 1 try again, 2 save and quit, 3 abandon hope / world map
	bool				mLeaving;               // +0x7c
	double				mFade;                  // +0x80

public:
	GamePanel(MummyApp* theApp);
	virtual ~GamePanel();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
};

// 0x414290: the parchment with the game-over text that comes down over the high scores and goes up again
class GameOverScroll : public ScrollScreen
{
public:
	int					mHold;                  // +0x6c how long it has rested

public:
	GameOverScroll(MummyApp* theApp);
	virtual ~GameOverScroll();

	virtual void		Update() override;      // 0x414330
	virtual void		Draw(Graphics* g) override;  // 0x4142f0
};

// 0x4291d0: the panel at the left of the board (0x96 wide): the buttons, the score, the Ankh
class SidePanel : public Widget, public ButtonListener
{
public:
	MummyApp*			mApp;                   // +0x50
	int					mScore;                 // +0x54
	int					mMumY;                  // +0x58 where the mummy logo hangs
	int					mAlpha;                 // +0x5c
	int					mAnkhAlpha;             // +0x60
	ButtonWidget*		mButtons[5];            // +0x64 ..: ids 10 undo, 11 retry, 12 options, 13 world map, 14 quit

public:
	SidePanel(MummyApp* theApp);
	virtual ~SidePanel();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x42a340
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		ButtonPress(int theId) override;
	using Widget::Resize;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;
};

// 0x422480: the screen after a level: the time, the points, and the link to the next chamber
class LevelCompleteScreen : public Widget, public ButtonListener
{
public:
	MummyApp*			mApp;                   // +0x54
	HyperlinkWidget*	mNextLink;              // +0x58 id 0 "enter the next chamber" / "claim the hidden treasure"
	HyperlinkWidget*	mMapLink;               // +0x5c id 1 "return to world map" (adventure)
	int					mSeconds;               // +0x60 the time the level took
	int					mBase;                  // +0x64 base points
	int					mBonus;                 // +0x68 bonus points
	int					mVariant;               // +0x6c which picture (0 to 2)
	int					mFrame;                 // +0x70 animation frame

public:
	LevelCompleteScreen(MummyApp* theApp);
	virtual ~LevelCompleteScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		ButtonPress(int theId) override;
	using Widget::Resize;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;
};

// 0x4143a0 / 0x413c30: the high-score table (a classic game ends with it)
class HighScoreScreen : public Widget, public ButtonListener
{
public:
	MummyApp*			mApp;
	HyperlinkWidget*	mMenuLink;              // id 0 "click here to return to main menu"
	std::string			mNames[5];
	int					mScores[5];
	int					mRank;                  // 0..4: the new score's place, -1 none
	int					mCounter;               // +0xc0
	bool				mNameAsked;             // +0xc4 the dialog for the name has been shown
	double				mFade;                  // +0xc8 0 .. 255

public:
	HighScoreScreen(MummyApp* theApp);
	virtual ~HighScoreScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		ButtonPress(int theId) override;
	using Widget::Resize;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;

	void				Load();
	void				Save();
	int					RankOf(int theScore);
	void				AskName();                  // 0x414ac0
	void				SetName(const SexyString& theName);   // 0x415b40
};

// 0x401ca0: the victory screen (not ported yet)
class VictoryScreen : public Widget
{
public:
	VictoryScreen(MummyApp* theApp) : mApp(theApp) {}
	MummyApp*			mApp;
	void				SetName(const SexyString& theName) { (void) theName; }
};

// 0x417010: the adventure's world map (not ported yet)
class WorldMap : public Widget
{
public:
	WorldMap(MummyApp* theApp) : mApp(theApp) {}
	MummyApp*			mApp;
};

}

#endif
