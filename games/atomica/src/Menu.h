#ifndef __ATOMICA_MENU_H__
#define __ATOMICA_MENU_H__

// Menu.cpp: the main menu of Atomica Deluxe (Atomica.exe 0x4198e0, 0x4191d0, 0x4195a0, 0x419760 and 0x41a010): two sliding
// doors, and behind them a panel with the two kinds of game (each with an expert button beside it), the saved game and
// the exit; after a game the panel shows the final score, the rank and the high scores, with "main menu" and "play again".
// The menu comes over the title screen (or the game that ended): the doors close over it with a slam (0x405e20 then deletes
// what is behind them), the panel comes down and its contents fade in; a game that starts makes the panel go and the doors
// open on the new board. The button handlers are the listener's slots 1 (press, 0x4195a0) and 2 (release, 0x419520).

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Support.h"

namespace Sexy
{

class AtomicaApp;
class ScoreTable;

class MainMenu : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;				// +0x74
	Sprite*			mDoorSheet;			// +0x301c (of the app in the original)
	Sprite*			mDoorLeft;			// +0x3020 (of the app in the original)
	Sprite*			mDoorRight;			// +0x3024
	Sprite*			mPanel;				// +0x3028
	int				mDoor;				// +0x78 how far the doors stand apart: 0x12f at first, 0 shut, 0x14a wide open
	int				mPanelY;			// +0x7c how far above (or, below 0, under) its place the panel stands
	int				mAlpha;				// +0x80 the panel's contents fade in
	int				mTimer;				// +0x84 updates the panel waits after the slam (0x4b)
	bool			mSlammed;			// +0x88 the doors have shut
	bool			mDoorOpenPlayed;	// +0x89
	bool			mSwoopPlayed;		// +0x8a
	bool			mClosing;			// +0x8b a game was started: the panel goes, then the doors open
	bool			mLeaving;			// +0xb9 "main menu" after a game: the panel goes down and comes back with the main page
	bool			mHasSave;			// +0xb0 a saved game is there
	bool			mGameOver;			// +0xb8 the panel shows the end of a game
	float			mSpin;				// +0xe0
	int				mInsertTimer;		// +0xe4 updates to the high score entry (100)

	// the end of the game (0x419760)
	int				mScore;				// +0xbc
	int				mLevelReached;		// +0xc0
	int				mTime;				// +0xc4 in hundredths of a second
	int				mAtoms;				// +0xc8
	int				mMolecules;			// +0xcc
	int				mCombo;				// +0xd0
	int				mBonusMolecules;	// +0xd4
	int				mBonusUpgrades;		// +0xd8
	int				mRank;				// +0xdc
	bool			mEntryShown;

	ButtonWidget*	mBtn[8];			// the buttons by id 0 to 7 (4 only with a saved game)
	PtrList<Widget>	mButtons;			// +0x5c

	MainMenu(AtomicaApp* theApp, bool theGameOver);	// 0x4198e0 / 0x419760
	virtual ~MainMenu();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x431d20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4194e0
	virtual void	Update() override;					// 0x4191d0
	virtual void	Draw(Graphics* g) override;			// 0x41a010
	virtual void	ButtonPress(int theId) override;	// 0x4195a0
	virtual void	ButtonDepress(int theId) override;	// 0x419520
	void			ShowPage(bool theGameOver);
	bool			PanelShown() const;					// port: the panel is up and complete (the bot waits for it)

private:
	bool			Over(int theId) const;
	void			FillButton(Graphics* g, int theId);
	void			BeginClosing();						// the head the game's buttons share (0x4195a0)
};

// 0x41ea40: the high score tables of both kinds of game, shown from the options box (the doors are open at once)
class HighScoresBox : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;				// +0x5c
	Sprite*			mDoorSheet;
	Sprite*			mDoorLeft;
	Sprite*			mDoorRight;
	Sprite*			mPanel;
	ButtonWidget*	mReturn;			// +0x60 id 0
	int				mDoor;				// +0x64 as in the menu
	int				mPanelY;			// +0x68
	int				mAlpha;				// +0x6c
	int				mTimer;				// +0x70 updates the panel waits after the slam (0xf)
	bool			mSlammed;			// +0x74
	bool			mDoorOpenPlayed;	// +0x75
	bool			mSwoopPlayed;		// +0x76
	bool			mClosing;			// +0x77 "return" was released: the panel goes, the doors open, the box goes
	float			mSpin;				// +0x90

	HighScoresBox(AtomicaApp* theApp);					// 0x41ea40
	virtual ~HighScoresBox();
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Update() override;					// 0x41e770
	virtual void	Draw(Graphics* g) override;			// 0x41ec90
	virtual void	ButtonPress(int theId) override;	// 0x41e9f0
	virtual void	ButtonDepress(int theId) override;	// 0x41e9e0
};

}

#endif
