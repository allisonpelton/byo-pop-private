#ifndef __ATOMICA_BOARD_H__
#define __ATOMICA_BOARD_H__

// Board.cpp: the screen of a game (Atomica.exe 0x431300-0x434200 and the Meter 0x4056a0-0x405b00): the machine's
// frame, the score, the level, the level meter, Atomicus' monitor and the atomic bomb. The field itself is the
// Grid, another widget on top. Behaviour spec: docs/GAME_NOTES.md section 6.
//
// Original: Board is a Widget with a ButtonListener at +0x58 (vftable 0x4b2ef8 and 0x4b2ee4), 0x2e8 bytes. The
// offsets in the comments are the original's.

#include "Widget.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Support.h"
#include "Effects.h"
#include "Color.h"
#include <string>
#include <cstdio>

namespace Sexy
{

class AtomicaApp;
class Grid;
class Atomicus;
class BonusMolecules;
class Graphics;

// 0x4056c0: the level, the molecules made, and the meter the molecules fill (the board keeps it at +0xc4, so its
// fields are what the rest of the code reads as the board's +0xc8 to +0xec)
class Meter
{
public:
	int		mLevel;				// +4 the level, 1 on
	int		mMoleculesMade;		// +8
	float	mValue;				// +0xc what the meter holds
	float	mShown;				// +0x10 what it shows (it follows the value at 0.05 an update)
	float	mMax;				// +0x14 what a level needs
	int		mTick;				// +0x18 counts up every 20 updates
	int		mCountdown;			// +0x1c
	bool	mFull;				// +0x20 the level is made (or, in the action mode, lost)
	int		mOverTick;			// +0x24 updates since
	bool	mFrozen;			// +0x28 the action mode: the molecules take the meter down
	Meter();					// 0x405a80
	void	Reset();			// 0x4056b0
	void	Init();				// 0x4056c0
	void	Draw(Graphics* g);	// 0x405700
	void	Update();			// 0x405820
	void	Hit(int theAmount);	// 0x4058a0
	void	Add(int theAmount, int theCombo);	// 0x4058f0
	void	Save(FILE* f);		// 0x405990
	void	Load(FILE* f);		// 0x405a00
};

class Board : public Widget, public ButtonListener
{
public:
	AtomicaApp*		mApp;				// +0x74
	Grid*			mGrid;				// +0x78
	ButtonWidget*	mButtonOptions;		// +0x7c id 0
	ButtonWidget*	mButtonMenu;		// +0x80 id 2
	ButtonWidget*	mButtonBomb;		// +0x88 id 1
	PtrList<Widget>	mButtons;			// +0x5c
	int				mI90;				// +0x90 updates to the next swirl in the background (not ported)
	float			mF8c;				// +0x8c counts down from 50 at a game's start; at 0 the field is set up
	Color			mFlash;				// +0x9c fades to black by 5 an update
	float			mF0ac;				// +0xac where the geiger needle goes (the combo times 12)
	float			mNeedle;			// +0xb0
	float			mNeedleSpeed;		// +0xb4
	int				mNeedleTick;		// +0xb8
	bool			mNeedleBlink;		// +0xbc
	Meter			mMeter;				// +0xc4
	Atomicus*		mAtomicus;			// +0xf4
	PtrList<Effect>	mEffects;			// +0xf8
	float			mBombPhase;			// +0x128 the bomb button's lamp
	float			mBombFrame;			// +0x12c
	float			mProgress;			// +0x130 the bomb's charge, 0 to 48
	float			mPulse;				// +0x134 128 to 255
	float			mPulseStep;			// +0x138
	int				mNumColorsStart;	// +0x13c the colours the next game starts with
	int				mMonitorSlide;		// +0x140
	bool			mBombOn;			// +0x144 the bomb goes off
	int				mI148;				// +0x148 updates to the next step of it
	int				mI14c;				// +0x14c atoms it bursts a step
	float			mF150;				// +0x150
	Color			mBombTint;			// +0x154
	bool			mB164;				// +0x164 the bomb's shake
	BonusMolecules*	mBonus;				// +0x168
	int				mI16c;				// +0x16c the level sign's alpha (0x163 at the start)
	int				mI170;				// +0x170 the 'level complete' sign's alpha
	int				mI174;				// +0x174
	float			mGeigerGlow;		// +0x178
	float			mGeigerGlowStep;	// +0x17c
	int				mI180;				// +0x180 the freeze (water element): how far it has come
	int				mI184;				// +0x184 the moves the field stays frozen
	int				mI188;				// +0x188
	int				mI18c;				// +0x18c
	int				mI190;				// +0x190
	int				mI194;				// +0x194
	int				mI198;				// +0x198 the moves the atoms are no obstacle for the path (air element)
	Color			mTint;				// +0x19c the colour cast of the field
	bool			mTintOn;			// +0x1ac
	float			mTintF[3];			// +0x1b0
	int				mScore;				// +0x1bc
	int				mI1c0;				// +0x1c0 the 'atomic bomb' sign's alpha
	int				mStat1c4;			// +0x1c4 updates
	int				mStat1c8;			// +0x1c8
	int				mStat1cc;			// +0x1cc atoms in molecules
	int				mStat1d0;			// +0x1d0 molecules
	int				mStat1d4;			// +0x1d4 the biggest combo
	int				mStat1d8;			// +0x1d8
	int				mStat1dc;			// +0x1dc
	char			mTooltip[0x100];	// +0x1e0
	int				mMouseX;			// +0x2e0
	int				mMouseY;			// +0x2e4

public:
	Board(AtomicaApp* theApp);			// 0x432c20
	virtual ~Board();					// 0x4319c0

	void	Start();					// 0x431b50
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x431d20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x431d60
	virtual void	Update() override;					// 0x433080
	virtual void	Draw(Graphics* g) override;			// 0x433f10
	virtual void	MouseMove(int x, int y) override;	// 0x431db0
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override;	// 0x432000
	virtual void	ButtonDepress(int theId) override;	// 0x433c40
	using Widget::MouseDown;

	void*	LastTarget() { return NULL; }	// 0x4321f0: the swirl the flown atoms go to (port: not ported)
	void	AddProgress(int theAmount);		// 0x432210
	void	CheckBomb();					// 0x4322c0: the bomb button was pressed
	void	StepBomb();						// 0x432360
	void	FloatScore(int theX, int theY, int thePoints, const Color& theColor);	// 0x4324f0
	void	StartFreeze(int theMoves);		// 0x4325c0
	void	StepFreeze();					// 0x432610
	void	CountDown();					// 0x432690
	void	StartGhost(int theMoves);		// 0x4326c0
	void	SetTint(int theR, int theG, int theB);	// 0x4328a0
	void	NextLevel();					// 0x432b80
	void	Save(FILE* f);					// 0x432930
	void	Load(FILE* f);					// 0x433ca0
};

}

#endif
