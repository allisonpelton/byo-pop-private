#ifndef __ATOMICA_MENU_H__
#define __ATOMICA_MENU_H__

// Menu.cpp: the main menu of Atomica Deluxe (Atomica.exe 0x4198e0, 0x4191d0, 0x4195a0, 0x419760 and 0x41a010): two sliding
// doors, and behind them a panel with the two kinds of game (each with an expert button beside it), the saved game and
// the exit; after a game the panel shows the final score, the rank and the high scores, with "main menu" and "play again".
// port: the doors and the panel move in a simpler way than the original's.

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
	Sprite*			mDoorLeft;			// +0x3020 (of the app in the original)
	Sprite*			mDoorRight;			// +0x3024
	Sprite*			mPanel;				// +0x3028
	int				mDoor;				// +0x78 how far the doors are open (0 to 0x14a)
	int				mPanelY;			// +0x7c how far down the panel stands
	int				mAlpha;				// +0x80 the panel's contents fade in
	int				mDoorTick;			// +0x84
	bool			mHasSave;			// +0xb0 a saved game is there
	bool			mGameOver;			// +0xb8 the panel shows the end of a game
	bool			mOpening;
	bool			mPanelShown;
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

	MainMenu(AtomicaApp* theApp, bool theGameOver);	// 0x4198e0
	virtual ~MainMenu();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x431d20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x4194e0
	virtual void	Update() override;					// 0x4191d0
	virtual void	Draw(Graphics* g) override;			// 0x41a010
	virtual void	ButtonDepress(int theId) override;	// 0x4195a0
	void			ShowPage(bool theGameOver);

private:
	bool			Over(int theId) const;
	void			FillButton(Graphics* g, int theId);
};

// 0x41ea40: the high score tables of both kinds of game, shown from the options box (the doors are open at once)
class HighScoresBox : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;				// +0x5c
	Sprite*			mDoorLeft;
	Sprite*			mDoorRight;
	Sprite*			mPanel;
	ButtonWidget*	mReturn;			// +0x60 id 0
	int				mAlpha;				// +0x6c

	HighScoresBox(AtomicaApp* theApp);					// 0x41ea40
	virtual ~HighScoresBox();
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;			// 0x41ec90
	virtual void	ButtonDepress(int theId) override;
};

}

#endif
