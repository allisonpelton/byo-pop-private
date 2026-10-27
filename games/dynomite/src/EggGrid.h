#ifndef __EGGGRID_H__
#define __EGGGRID_H__

// The board's grid of eggs at +0x8c (vtable 0x4fa408, 0x46e8 bytes; 0x415770-0x418460; docs/GAME_NOTES.md
// section 4): a hex grid of 33-pixel cells, 15 columns of 100 rows, the odd rows shifted right by 16, under a
// ceiling that comes down.

#include "PtrList.h"

namespace Sexy
{

class BinFile;
class Dynomite;
class FossilizedApp;

// a cell: the egg's colour in the low byte, flags above
struct GridCell
{
	unsigned int		mBits;
};

// a point of the grid or of the screen (the original passes an int[2])
struct GridPoint
{
	int					mX;
	int					mY;
};

// a thing on the board an egg can hit (the list at +0x4694)
struct GridTarget
{
	int					mHitBits;		// +0 what Probe answers for it
	int					mX1;			// +4
	int					mY1;			// +8
	int					mX2;			// +0xc
	int					mY2;			// +0x10
	bool				mHit;			// +0x14
};

class EggGrid;

// vtable 0x4fcd74 (only its destructor), 0x28 bytes: a stick of dynamite between two eggs of a colour, sliding 5
// pixels along one of three angles with its fuse sparkling (the grid's list at +0x466c)
class GridDynamite
{
public:
	int					mAngle;			// +4 315, 335 or 5
	int					mWhich;			// +8 the picture, 0-2
	float				mX;				// +0xc in the grid
	float				mY;				// +0x10
	float				mDX;			// +0x14
	float				mDY;			// +0x18
	EggGrid*			mGrid;			// +0x1c
	float				mTime;			// +0x20 to 5 by quarters
	float				mSpark;			// +0x24 the spark's frame (0 to 3)

	GridDynamite(EggGrid* theGrid, int x, int y);	// 0x438270
	virtual ~GridDynamite();						// 0x438200

	void				Update();					// 0x438210
	void				Draw();						// 0x438350
};

// vtable 0x4fc744, 0x307c bytes: a depth-first search for a path of eggs from a cell to the top row (400 steps at
// most, after which it gives up and answers yes)
class CeilingFinder
{
public:
	struct Visit
	{
		int				mCol;
		int				mRow;
		int				mUnused[2];
	};

	Visit				mVisited[400];		// +4
	short				mVisitedCount;		// +0x1904
	int					mSteps;				// +0x1908
	GridCell			mCells[15][100];	// +0x190c

	CeilingFinder();						// 0x430c90
	virtual ~CeilingFinder();				// 0x430ca0

	int					Search(int theCol, int theRow);			// 0x430cb0
	int					Find(int theCol, int theRow, GridCell* theCells);	// 0x430e60
};

// vtable 0x4fcb68 (only its destructor), 0x14 bytes: a baby in an egg for a while (the cell's bit 0x100), which
// hatches if the egg breaks in time (the grid's list at +0x46b8)
class BabyEgg
{
public:
	int					mCol;			// +4
	int					mRow;			// +8
	int					mTime;			// +0xc
	EggGrid*			mGrid;			// +0x10

	BabyEgg(EggGrid* theGrid);											// 0x4349d0: a random egg
	BabyEgg(EggGrid* theGrid, int theCol, int theRow, int theTime);		// 0x434ac0
	virtual ~BabyEgg();													// 0x434b10

	bool				Update();										// 0x434b40: false when it is over
};

class EggGrid
{
public:
	float				mCeiling;			// +4 the ceiling's offset (grid y = screen y - this)
	float				mRows;				// +8 the rows in use
	int					mBottom;			// +0xc the screen y of the lowest row's top (Update; a baby egg's pick)
	int					mMaxColor;			// +0x10 the highest colour placed (4)
	GridCell			mCells[15][100];	// +0x14 column, row
	GridCell			mScratch[15][100];	// +0x1784 Match's copy of mCells
	GridDynamite*		mDynamite[15][100];	// +0x2ef4 the stick at a cell
	Dynomite*			mBoard;				// +0x4664
	FossilizedApp*		mApp;				// +0x4668
	PtrList<GridDynamite> mDynamites;		// +0x466c
	int					mPastLineFrames;	// +0x4684 frames the eggs have been past the board's line
	bool				mWarning;			// +0x4688 the warning's blink, on or off
	int					mWarningBlink;		// +0x468c
	bool				mField4690;			// +0x4690 (Draw hides nothing past the line while it is set)
	bool				mField4691;			// +0x4691 (set when the eggs first pass the line)
	PtrList<GridTarget>	mTargets;			// +0x4694
	bool				mBabyEggsOn;		// +0x46ac
	int					mBabyEggChance;		// +0x46b0 one in this many frames (200)
	int					mField46b4;			// +0x46b4 (200)
	PtrList<BabyEgg>	mBabyEggs;			// +0x46b8
	bool				mField46d0;			// +0x46d0 the ceiling is drawn (20 pixels higher foot)
	bool				mBreakPastLine;		// +0x46d1 Update breaks the eggs past the line
	int					mBlastX;			// +0x46d4 Blast's sums, then the middle
	int					mBlastY;			// +0x46d8
	int					mBlastCount;		// +0x46dc
	int					mEggCount;			// +0x46e0 matchable eggs in the lowest ten rows (100)
	bool				mLoneEgg;			// +0x46e4 set before a landing egg is placed; cleared when it touches its colour
	bool				mField46e5;			// +0x46e5 (eggs past the right edge are kept)

	EggGrid(FossilizedApp* theApp, Dynomite* theBoard);		// 0x415770
	virtual ~EggGrid();										// 0x417420

	static void			PixelToCell(int x, int y, GridPoint* theCell);		// 0x415890
	void				ScreenToGrid(int x, int y, GridPoint* thePoint);	// 0x4158f0
	static void			CellToPixel(int theCol, int theRow, GridPoint* thePoint);	// 0x415920
	void				GridToScreen(int x, int y, GridPoint* thePoint);	// 0x415950
	void				CountRows();										// 0x415980
	void				ResetCeiling();										// 0x4159f0
	bool				HasColor(int theColor);								// 0x415f00
	// 0x415f70: a saved game's grid: the cells, the sticks of dynamite (to -1, -1) and the babies in eggs
	void				SaveState(BinFile* theFile);
	void				LoadState(BinFile* theFile);						// 0x416150
	int					PickColor();										// 0x4163e0
	void				ClearDynamite();									// 0x416500
	void				AddRow(int theRow);									// 0x416b00: random eggs across a row
	void				Clear();											// 0x417190
	// 0x4171c0: a Stomped puzzle's grid (or a saved game's): the cells, then the sticks of dynamite to (-1, -1);
	// cells of colour 20 get a random colour
	void				Load(BinFile* theFile);
	void				PushDown();											// 0x418020: two new rows on top
	void				BreakPastLine();									// 0x416e30
	void				Update();											// 0x4174c0
	void				Draw();												// 0x417760
	unsigned int		Probe(int x, int y, bool theGhost);					// 0x4169c0
	void				Break(int x, int y, GridCell* theCell);				// 0x415a20
	void				Place(int theCol, int theRow, int theColor, int theKind);	// 0x416590
	void				Remove(int theCol, int theRow, bool isMatch);		// 0x416800
	int					DropLoose();										// 0x416b80
	void				Match(int theCol, int theRow, int theColor);		// 0x417bd0
	void				Blast(int theCol, int theRow);						// 0x418100 (a bomb lands)
	void				BlastCell(int theCol, int theRow);					// 0x417020
};

}

#endif
