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
class Swirl;

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
	void	Save(FILE* theFile);	// 0x405990
	void	Load(FILE* theFile);	// 0x405a00
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
	float			mStartCountdown;	// +0x8c counts down from 50 at a game's start; at 0 the field is set up
	int				mSwirlTimer;		// +0x90 updates to the next swirl of the background
	float			mAngle;				// +0x94 where the ring of the swirls starts (0.1 degree an update)
	bool			mSwirlsCatchUp;		// +0x98 the swirls are brought up to date at the start of a game: the update goes on until one has gone
	Color			mFlash;				// +0x9c fades to black by 5 an update
	float			mNeedleTarget;		// +0xac where the geiger needle goes (the combo times 12)
	float			mNeedle;			// +0xb0
	float			mNeedleSpeed;		// +0xb4
	int				mNeedleTick;		// +0xb8
	bool			mNeedleBlink;		// +0xbc
	int				mDetail;			// +0xc0 1: every row of a swirl is drawn (the original's detail setting)
	Meter			mMeter;				// +0xc4
	int				mFlicker;			// +0xf0 an object of the original's (0x41ffa0, constructed here and by Start) whose +0xc is 0xff or 0xdc;
										// nothing reads it, but its update (0x41ffd0) draws a random number every update
	Atomicus*		mAtomicus;			// +0xf4
	PtrList<Effect>	mEffects;			// +0xf8
	PtrList<Swirl>	mSwirls;			// +0x110 the rings of the background, the oldest first, and the copies the flown atoms circle in
	float			mBombPhase;			// +0x128 the bomb button's lamp
	float			mBombFrame;			// +0x12c
	float			mProgress;			// +0x130 the bomb's charge, 0 to 48
	float			mPulse;				// +0x134 128 to 255
	float			mPulseStep;			// +0x138
	int				mNumColorsStart;	// +0x13c the colours the next game starts with
	int				mMonitorSlide;		// +0x140
	bool			mBombOn;			// +0x144 the bomb goes off
	int				mBombTimer;			// +0x148 updates to the bomb's next step
	int				mBombAtomsPerStep;	// +0x14c atoms the bomb bursts a step
	float			mBombChargePerAtom;	// +0x150 what a burst atom takes from the bomb's charge
	Color			mBombTint;			// +0x154
	bool			mBombShake;			// +0x164 the bomb's shake
	BonusMolecules*	mBonus;				// +0x168
	int				mGetReadyAlpha;		// +0x16c the 'get ready' sign's alpha (0x163 at the start)
	int				mGameOverAlpha;		// +0x170 the 'game over' sign's alpha
	int				mLevelUpAlpha;		// +0x174 the 'level up' sign's alpha
	float			mGeigerGlow;		// +0x178
	float			mGeigerGlowStep;	// +0x17c
	int				mFreezeProgress;	// +0x180 the freeze (water element): how far it has come
	int				mFreezeMoves;		// +0x184 the moves the field stays frozen
	Swirl*			mGlowSwirl;			// +0x188 the swirl the freeze has come to
	int				mFreezeSwirlTimer;	// +0x18c updates to the next swirl the freeze reaches (10)
	int				mSavedZero;			// +0x190 0 throughout: saved and loaded, nothing reads it
	int				mGhostGlow;			// +0x194 0xff, and down to 0x7f while the atoms are no obstacle
	int				mGhostMoves;		// +0x198 the moves the atoms are no obstacle for the path (air element)
	Color			mTint;				// +0x19c the colour cast of the field
	bool			mTintOn;			// +0x1ac
	float			mTintF[3];			// +0x1b0
	int				mScore;				// +0x1bc
	int				mBombSignAlpha;		// +0x1c0 the 'atomic bomb' sign's alpha
	int				mStatUpdates;		// +0x1c4 the game's totals for its end page (seven ints, saved as one block): updates played
	int				mStatSavedZero;		// +0x1c8 0 throughout: saved and loaded, nothing reads or counts it
	int				mStatAtoms;			// +0x1cc atoms in molecules
	int				mStatMolecules;		// +0x1d0 molecules
	int				mStatBestCombo;		// +0x1d4 the biggest combo
	int				mStatBonusMolecules;	// +0x1d8 bonus molecules made
	int				mStatBonusUpgrades;	// +0x1dc bonus molecules of a higher level made
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

	Swirl*	LastTarget() { return mSwirls.Get(mSwirls.Size() - 1); }	// 0x4321f0: the newest swirl: the flown atoms go to a copy of it
	void	AddProgress(int theAmount);		// 0x432210
	void	CheckBomb();					// 0x4322c0: the bomb button was pressed
	void	StepBomb();						// 0x432360
	void	FloatScore(int theX, int theY, int thePoints, const Color& theColor);	// 0x4324f0
	void	StartFreeze(int theMoves);		// 0x4325c0
	void	StepFreeze();					// 0x432610
	void	CountDown();					// 0x432690
	void	StepAir();						// 0x4326e0: every update the air element is on
	void	StartGhost(int theMoves);		// 0x4326c0
	void	SetTint(int theR, int theG, int theB);	// 0x4328a0
	void	NextLevel();					// 0x432b80
	void	Save(FILE* theFile);			// 0x432930
	void	Load(FILE* theFile);			// 0x433ca0
};

}

#endif
