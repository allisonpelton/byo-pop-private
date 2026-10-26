#ifndef __EGGGRID_H__
#define __EGGGRID_H__

// The board's grid of eggs at +0x8c (vtable 0x4fa408, 0x46e8 bytes; 0x415770-0x418460; docs/GAME_NOTES.md
// section 4): a hex grid of 33-pixel cells, 15 columns of 100 rows, the odd rows shifted right by 16, under a
// ceiling that comes down. Work in progress: the functions not translated yet are TODO.

#include "PtrList.h"

namespace Sexy
{

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

class EggGrid
{
public:
	float				mCeiling;			// +4 the ceiling's offset (grid y = screen y - this)
	float				mRows;				// +8 the rows in use
	int					mMaxColor;			// +0x10 the highest colour placed (4)
	GridCell			mCells[15][100];	// +0x14 column, row
	GridCell			mScratch[15][100];	// +0x1784 Match's copy of mCells
	GridDynamite*		mDynamite[15][100];	// +0x2ef4 the stick at a cell
	Dynomite*			mBoard;				// +0x4664
	FossilizedApp*		mApp;				// +0x4668
	PtrList<GridDynamite> mDynamites;		// +0x466c
	bool				mField4688;			// +0x4688
	bool				mField4690;			// +0x4690
	bool				mField4691;			// +0x4691
	PtrList<GridTarget>	mTargets;			// +0x4694
	bool				mField46ac;			// +0x46ac
	int					mField46b0;			// +0x46b0 (200)
	int					mField46b4;			// +0x46b4 (200)
	PtrList<void>		mList46b8;			// +0x46b8
	bool				mField46d0;			// +0x46d0
	bool				mField46d1;			// +0x46d1
	int					mField46e0;			// +0x46e0 (100)
	bool				mLoneEgg;			// +0x46e4 set before a landing egg is placed; cleared when it touches its colour
	bool				mField46e5;			// +0x46e5

	EggGrid(FossilizedApp* theApp, Dynomite* theBoard);		// 0x415770
	virtual ~EggGrid();										// 0x417420

	static void			PixelToCell(int x, int y, GridPoint* theCell);		// 0x415890
	void				ScreenToGrid(int x, int y, GridPoint* thePoint);	// 0x4158f0
	static void			CellToPixel(int theCol, int theRow, GridPoint* thePoint);	// 0x415920
	void				GridToScreen(int x, int y, GridPoint* thePoint);	// 0x415950
	void				CountRows();										// 0x415980
	void				ResetCeiling();										// 0x4159f0
	unsigned int		Probe(int x, int y, bool theGhost);					// 0x4169c0
	void				Break(int x, int y, GridCell* theCell);				// 0x415a20
	void				Place(int theCol, int theRow, int theColor, int theKind);	// 0x416590
	void				Remove(int theCol, int theRow, bool isMatch);		// 0x416800
	int					DropLoose();										// 0x416b80 (TODO)
	void				Match(int theCol, int theRow, int theColor);		// 0x417bd0
	void				Blast(int theCol, int theRow);						// 0x418100 (a bomb lands)
};

}

#endif
