#ifndef __ATOMICA_GRID_H__
#define __ATOMICA_GRID_H__

// Grid.cpp, GridLevel.cpp, GridRules.cpp: the field of Atomica Deluxe, a widget of 9 columns and 8 rows of cells
// of 50 pixels (Atomica.exe 0x40c340-0x414830). It owns the atoms, finds the molecules (blocks of atoms of one
// type), and runs the levels' set-ups. Behaviour spec: docs/GAME_NOTES.md section 5.
//
// Original: vftable 0x4af748 (a Widget of the 2003 framework, 0x58 bytes, 43 slots; this class is 0x528 bytes).
// Update is slot 17 (0x412cd0), MouseDown slot 21 and MouseMove/MouseUp the ones at 0x40e7f0 / 0x40e710. The
// fields keep the original's offsets in the comments; unclear ones are named after theirs.

#include "Widget.h"
#include "Support.h"
#include "Atom.h"
#include "Color.h"

namespace Sexy
{

class AtomicaApp;
class PathFinder;
class Sparkle;
class Graphics;

class Grid : public Widget
{
public:
	enum
	{
		COLS = 9,
		ROWS = 8,
		CELL = 50
	};

	AtomicaApp*		mApp;				// +0x58
	float			mPulse;				// +0x5c a value that runs between 50 and 100 (the cursor's pulse)
	float			mPulseStep;			// +0x60 +-0.15
	int				mSelectGlow;		// +0x64 how strongly the held atom pulls: 0x4b when nothing is held, up to 0xff
	PtrList<Atom>	mAtoms;				// +0x68 every atom, placed or on its way
	PtrList<Sparkle> mSparkles;			// +0x80 the sparks of the field's reveal wave
	PtrList<PtrList<Atom> > mGroups;	// +0x98 the molecules just made (lists of their atoms)
	Atom*			mCells[COLS][ROWS];	// +0xb0
	Atom*			mSelected;			// +0x1d0 the atom the player holds
	int				mCellState[COLS][ROWS]; // +0x1d4 -1000000 .. 255: a cell's reveal; below 0x96 the atom is hidden
	int				mCursorX;			// +0x2f4
	int				mCursorY;			// +0x2f8
	Color			mCursorColor;		// +0x2fc the colour of the held atom
	int				mM30c;				// +0x30c
	int				mM310;				// +0x310
	int				mHoverCol;			// +0x314
	int				mHoverRow;			// +0x318
	int				mM31c;				// +0x31c
	int				mM320;				// +0x320
	PathFinder*		mPathFinder;		// +0x324
	int				mNumColors;			// +0x328
	bool			mB32c;				// +0x32c
	int				mCombo;				// +0x330 the molecules of the move, doubled by size
	bool			mB334;				// +0x334
	bool			mB335;				// +0x335 a whoosh is due
	bool			mNoMoveLeft;		// +0x336
	bool			mLevelOver;			// +0x337
	int				mOverTick;			// +0x338
	bool			mB33c;				// +0x33c
	bool			mB33d;				// +0x33d
	int				mGroupX;			// +0x340 where the molecule made last was (average of its atoms)
	int				mGroupY;			// +0x344
	Color			mGroupColor;		// +0x348 the sum, then the average of the colours of its atoms
	bool			mB358;				// +0x358
	float			mGeiger1;			// +0x35c the volume of the geiger counters
	float			mGeiger2;			// +0x360
	int				mI364;				// +0x364 how many bonus atoms are on the field
	int				mI368;				// +0x368
	Atom*			mTooltipAtom;		// +0x36c
	int				mTooltipAlpha;		// +0x370
	int				mStat374;			// +0x374 totals the board takes (FlushStats)
	int				mStat378;			// +0x378
	int				mStat37c;			// +0x37c
	int				mStat380;			// +0x380
	int				mStat384;			// +0x384
	int				mStat388;			// +0x388
	int				mStat38c;			// +0x38c
	float			mF390;				// +0x390
	float			mF394;				// +0x394
	int				mI398;				// +0x398
	float			mChromiumChance;	// +0x39c in percent: an atom made while none is chosen is chromium by this chance
	float			mBiohazardChance;	// +0x3a0
	int				mAtomsPerRound;		// +0x3a4
	bool			mB3a8;				// +0x3a8
	float			mF3ac;				// +0x3ac
	int				mRounds;			// +0x3b0
	bool			mHintShown[0xff];	// +0x3b4 a hint box has been shown (indexed by hint)
	bool			mB4b3;				// +0x4b3
	unsigned char	mB4b4;				// +0x4b4 the element (plus one) of an element atom that wants to act
	int				mI4b8;				// +0x4b8
	int				mI4bc;				// +0x4bc
	bool			mB4c0;				// +0x4c0
	bool			mUnsolvable;	// +0x4c1 the radiation round can no longer be solved
	int				mBonusTick;			// +0x4c4
	int				mArrowX;			// +0x4c8 a field's edge arrows: -1, 0 or 1
	int				mArrowY;			// +0x4cc
	int				mI4d0;				// +0x4d0
	bool			mRadiation[COLS][ROWS];	// +0x4d4 the radiation round's marked cells
	bool			mRadiationRound;	// +0x51c
	int				mI520;				// +0x520
	bool			mB524;				// +0x524
	bool			mB525;				// +0x525
	bool			mPausedByKey;		// +0x526

public:
	Grid(AtomicaApp* theApp);						// 0x40e3e0
	virtual ~Grid();								// 0x40c340

	// the helpers
	static void		CellToPixel(int theCol, int theRow, int* thePixel);	// 0x40c4c0: the middle of a cell
	static void		PixelToCell(int theX, int theY, int* theCell);		// 0x40c4f0
	void			FlyAwayAll();									// 0x40c540
	int				CountAtoms(bool theOnlyFlying);					// 0x40c6a0
	int				TypesInRect(int theCol1, int theRow1, int theCol2, int theRow2);	// 0x40c700
	bool			HasType(int theType);							// 0x40c830
	void			FlushStats();									// 0x40c8c0
	void			Start();										// 0x40e670
	void			Reset(int theNumColors);						// 0x411de0
	void			StartLevel(int theLevel);						// 0x40f860
	void			SetupBonusLevel(int theLevel);					// 0x40dd10
	void			SetupTimedLevel();								// 0x40f730
	void			EndRound();										// 0x40d140
	void			CheckUnsolvable();							// 0x40e250
	void			CheckHints();									// 0x40d450
	void			Obliterate(Atom* theAtom);						// 0x40ea60
	void			SpawnBonds(int theCol, int theRow);				// 0x40eba0
	int				Collect(int theCol1, int theRow1, int theCol2, int theRow2); // 0x40ef10
	void			DrawBond(Graphics* g, Atom* theA, Atom* theB, bool theFlag); // 0x40f2e0
	void			Burst(Atom* theAtom);							// 0x40f5d0
	bool			FindMolecules();								// 0x412280
	void			AfterMove(bool theTimerTick);					// 0x4127c0
	void			Magnetise();									// 0x412a80
	void			Save(FILE* f);									// 0x40c9a0
	void			Load(FILE* f);									// 0x40cd10

	// the widget
	virtual void	Update() override;								// 0x412cd0
	virtual void	Draw(Graphics* g) override;						// 0x413880 (the board draws it: DrawField)
	void			DrawField(Graphics* g);
	virtual void	MouseDown(int x, int y, int theBtnNum, int theClickCount) override;	// 0x40e7f0
	virtual void	MouseMove(int x, int y) override;				// 0x40e710
	virtual void	MouseDrag(int x, int y) override;
	virtual void	MouseUp(int x, int y, int theBtnNum) override;	// 0x40e7c0
	virtual void	KeyDown(KeyCode theKey) override;				// 0x40c880
	using Widget::MouseDown;
	using Widget::MouseUp;
};

}

#endif
