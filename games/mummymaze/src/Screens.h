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
	bool				mPlayShown;             // +0x68 the play link has appeared
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
	virtual void		ButtonPress(int theId) override;      // 0x403430: the empty listener function
	using ScrollScreen::ButtonPress;
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
	virtual void		MouseDown(int x, int y, int theClickCount) override;                // 0x42a570: the Ankh asks what it is
	virtual void		ButtonDepress(int theId) override;
	virtual void		ButtonPress(int theId) override;
	using Widget::MouseDown;
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

// 0x401ca0: the victory screen: the same list, name box and links as the high scores (the original's twin classes share
// the functions that load and keep the list), but another picture, and the link comes after the name
class VictoryScreen : public HighScoreScreen
{
public:
	VictoryScreen(MummyApp* theApp);
	virtual ~VictoryScreen();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;   // 0x402a00
	virtual void		Update() override;                                          // 0x402c60
	virtual void		Draw(Graphics* g) override;                                 // 0x402a10
};

// 0x427050: a pyramid on the world map: a button that only works while the pyramid is open (mEnabled), and says why not
// when it is clicked while it is shut
class PyramidButton : public ButtonWidget
{
public:
	MummyApp*			mApp;                   // +0x7c
	bool				mHover;                 // +0xaa the mouse is over it (kept for the map's picture)
	bool				mEnabled;               // +0xab it can be entered

public:
	PyramidButton(int theId, ButtonListener* theListener, MummyApp* theApp);

	virtual void		Draw(Graphics* g) override;                         // 0x4271c0
	virtual void		MouseEnter() override;                              // 0x427d50
	virtual void		MouseLeave() override;                              // 0x427d70
	virtual void		MouseDown(int x, int y, int theBtnNum, int theClickCount) override;    // 0x4276d0
	virtual void		MouseUp(int x, int y, int theBtnNum, int theClickCount) override;      // 0x427d00
	using ButtonWidget::MouseDown;
	using ButtonWidget::MouseUp;
};

// a part of the map that is only there to be clicked
class MapHotspot : public ButtonWidget
{
public:
	MapHotspot(int theId, ButtonListener* theListener);
	virtual void		Draw(Graphics* g) override;
};

// 0x417010: the adventure's world map: fifteen pyramids, the explorer's marker that walks from one to another along the
// paths, and three corners to click (the statistics, the player's name and the treasure chest). It reads the saved
// adventure (adv<slot>.sav in the user folder; data/saves/adv<slot>.sav in the original) when it is made and writes it when the
// game takes it away (MummyApp::RemoveWorldMap, 0x41c300).
class WorldMap : public Widget, public ButtonListener
{
public:
	MummyApp*			mApp;                   // +0x50
	int					mCurrent;               // +0xb4 the pyramid the marker stands on
	int					mTarget;                // +0xb8 the pyramid it is walking to
	int					mNext;                  // +0xbc the pyramid it is walking to next
	int					mProgress[15];          // +0xc0 chambers done in each pyramid (15: the pyramid is solved), -1: shut
	ButtonWidget*		mEnterButton;           // +0x64 id 99
	ButtonWidget*		mBackButton;            // +0x68 id 98
	ButtonWidget*		mStatsButton;           // +0x6c id 50
	ButtonWidget*		mPlayerButton;          // +0x70 id 51
	ButtonWidget*		mChestButton;           // +0x74 id 52
	PyramidButton*		mPyramids[15];          // +0x78 ids 1 .. 15
	int					mStep;                  // +0xfc how far along the path the marker is (-1: it is standing)
	int					mMarkerX, mMarkerY;     // +0x100, +0x104
	int					mDistance[15];          // +0x108 (the walk's search)
	int					mHoverShown;            // +0x144 the pyramid whose name is shown
	int					mHoverAlpha;            // +0x148 how plainly
	bool				mHoverFading;           // +0x14c
	int					mSparkFrame[18];        // +0x150
	int					mSparkX[18];            // +0x198
	int					mSparkY[18];            // +0x1e0
	bool				mOpened;                // +0x14d the open pyramids have been made clickable
	bool				mChamberDone;           // +0x228 a chamber has just been won
	bool				mCheckEntry;            // +0x229 go straight on into the pyramid's next chamber
	bool				mInvalid;               // the saved game was bad: the main menu is back

public:
	WorldMap(MummyApp* theApp, int theSlot, bool theCheckEntry);
	virtual ~WorldMap();
	void				CropEnterButton();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x4187d0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x418840
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x4188b0
	virtual void		Draw(Graphics* g) override;                                         // 0x4183c0
	virtual void		Update() override;                                                  // 0x417c10
	virtual void		ButtonPress(int theId) override;                                    // 0x418a90
	virtual void		ButtonDepress(int theId) override;                                  // 0x418aa0
	using Widget::Resize;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;

	void				Save();                         // 0x4195d0
	void				Unlock();                       // 0x419940
	void				Open(int thePyramid);           // 0x418970
	void				Measure(int thePyramid, int theDistance);   // 0x418a30
	int					NextHop(int theFrom, int theTo);    // 0x4189b0
	void				EnableOpenPyramids();
};

}

#endif
