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
#include "EditWidget.h"
#include "EditListener.h"
#include "Slider.h"
#include "SliderListener.h"
#include "Color.h"
#include <string>

// Menu.cpp 0x41a010: the 256 colours the score rows and the entry box cycle through
void BuildScoreColors(Sexy::Color* theColors, int theAlpha);

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

// 0x41cc00: a score in the high score table: the player types a name
class HighScoreDialog : public GameDialog, public EditListener
{
public:
	float			mSpin;				// +0xb0 the colour the box is at (up 0.05 an update)
	int				mScore;				// +0xb4
	int				mTick;				// +0xb8 0 to 300 in fours: the arrows beside the score come and go
	const char*		mPhrase;			// +0xc0 what the monitor says
	EditWidget*		mEdit;				// +0xc4
	ButtonWidget*	mOk;				// +0xc8 id 2
	ButtonWidget*	mSkip;				// +0xcc id 3

	HighScoreDialog(AtomicaApp* theApp, int theScore);	// 0x41cc00
	virtual ~HighScoreDialog();							// 0x41ca20
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x41caf0
	virtual void	Update() override;
	virtual void	Draw(Graphics* g) override;			// 0x41cf90
	virtual void	ButtonDepress(int theId) override;	// 0x41cf00
	virtual void	EditWidgetText(int theId, const SexyString& theString) override;
};

// 0x417b90: "confirm quit": quit the game in progress, saving it or not
class QuitDialog : public GameDialog
{
public:
	ButtonWidget*	mSaveQuit;			// +0xac id 0
	ButtonWidget*	mQuit;				// +0xb0 id 1
	ButtonWidget*	mCancel;			// +0xb4 id 2
	bool			mQuitAfter;			// +0xb8 the game ends when the box goes

	QuitDialog(AtomicaApp* theApp);						// 0x417b90
	virtual ~QuitDialog();								// 0x417d30
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x417db0
	virtual void	Draw(Graphics* g) override;			// 0x417ef0
	virtual void	ButtonDepress(int theId) override;	// 0x417e40
	virtual void	ButtonPress(int theId) override;	// 0x417e90
};

// 0x422f10: the game options (volumes, full screen, the colour-blind symbols, hints) and the credits
class OptionsDialog : public GameDialog, public SliderListener
{
public:
	ButtonWidget*	mDone;				// +0xb0 id 0 (BACK on the credits page)
	ButtonWidget*	mHighScores;		// +0xb8 id 3
	ButtonWidget*	mCredits;			// +0xc0 id 5
	ButtonWidget*	mResetHints;		// +0xc4 id 9
	ButtonWidget*	mBoxFullscreen;		// +0xc8 id 6
	ButtonWidget*	mBoxCursors;		// +0xcc id 7
	ButtonWidget*	mBoxSymbols;		// +0xd0 id 8
	ButtonWidget*	mBoxHints;			// +0xd4 id 10
	Slider*			mMusic;				// +0xd8 id 1
	Slider*			mSfx;				// +0xdc id 0
	bool			mFullscreen;		// +0xe0
	bool			mCursors;			// +0xe1
	bool			mSymbols;			// +0xe2 the colour-blind fix
	bool			mNoHints;			// +0xe3
	bool			mCreditsPage;		// +0xe8
	bool			mFirstUpdate;		// +0xe9

	OptionsDialog(AtomicaApp* theApp);					// 0x422f10
	virtual ~OptionsDialog();							// 0x4229d0
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x422ac0
	virtual void	Update() override;					// 0x4234a0
	virtual void	Draw(Graphics* g) override;			// 0x4234f0
	virtual void	ButtonDepress(int theId) override;	// 0x424430 and 0x422c70
	virtual void	SliderVal(int theId, double theVal) override;	// 0x422d00
};

// the rank names' addresses in the exe and the scores that give each rank (0x4cb0a0, 0x4cb100)
extern const unsigned int kRankNames[22];
extern const int kRankScores[23];
int RankOfScore(int theScore);

}

#endif
