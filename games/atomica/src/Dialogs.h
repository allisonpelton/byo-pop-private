#ifndef __ATOMICA_DIALOGS_H__
#define __ATOMICA_DIALOGS_H__

// Dialogs.cpp: the boxes of Atomica Deluxe (Atomica.exe 0x4167a0-0x416c30 the base, 0x420040 level up, 0x421f90 confirm
// new game, 0x417b90 options, 0x422f10 the menu box). They are widgets of the 2003 framework, not its Dialog: each is
// the picture of a monitor (the app's mDialogMonitor) that slides down from above the screen, takes the mouse (a base
// modal widget), draws its text and its buttons itself, and slides up again when it is closed.

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Support.h"
#include <string>

namespace Sexy
{

class AtomicaApp;
class WidgetManager;

// 0x416ac0: the base of the boxes (vftable 0x4af8a8)
class GameDialog : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;				// +0x90
	PtrList<Widget>	mButtons;			// +0x60 the buttons (added to the manager with the box)
	PtrList<Widget>	mOthers;			// +0x78 other widgets of the box (edit lines)
	WidgetManager*	mManager;			// +0xa0
	int				mBoxX;				// +0xa4 where the monitor stands (x)
	int				mBoxY;				// +0x94 how far it has slid down (starts at minus its height)
	int				mBoxEndY;			// +0x98 where it ends going up (minus its height)
	bool			mClosing;			// +0x9c slides up and is deleted
	bool			mCentered;			// +0xa8

	GameDialog(AtomicaApp* theApp, bool theCentered);	// 0x416ac0
	virtual ~GameDialog();								// 0x4167a0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x416890
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x416940
	virtual void	Update() override;					// 0x4169d0
	virtual void	Draw(Graphics* g) override;			// 0x416c00
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x416a90: the arguments are not used
	using Widget::Resize;
	// 0x416c30: a flat button with its text
	void			DrawButton(Graphics* g, Widget* theButton, const std::string& theText);
	ButtonWidget*	AddButton(int theId);				// the invisible button of the box (the box draws it)
	void			Close();
};

class Board;
class Grid;

// 0x420040: the level is made: its statistics, the rank
class LevelUpDialog : public GameDialog
{
public:
	Board*			mBoard;				// +0xb0
	Grid*			mGrid;				// +0xb4
	int				mRank;				// +0xb8
	ButtonWidget*	mContinue;			// +0xac

	LevelUpDialog(AtomicaApp* theApp, Board* theBoard);	// 0x420040
	virtual ~LevelUpDialog();							// 0x420170
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4201f0
	virtual void	Draw(Graphics* g) override;			// 0x4202b0
	virtual void	ButtonDepress(int theId) override;	// 0x420240
};

// 0x421f90: "confirm new game": give up the game in progress?
class ConfirmDialog : public GameDialog
{
public:
	ButtonWidget*	mYes;				// +0xac
	ButtonWidget*	mNo;				// +0xb0

	ConfirmDialog(AtomicaApp* theApp);					// 0x421f90
	virtual ~ConfirmDialog();							// 0x4220b0
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x422100
	virtual void	Draw(Graphics* g) override;			// 0x4221d0
	virtual void	ButtonDepress(int theId) override;	// 0x422170
};

// the rank names' addresses in the exe and the scores that give each rank (0x4cb0a0, 0x4cb100)
extern const unsigned int kRankNames[22];
extern const int kRankScores[23];
int RankOfScore(int theScore);

}

#endif
