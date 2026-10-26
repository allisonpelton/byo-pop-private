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

class EggGrid
{
public:
	float				mCeiling;			// +4 the ceiling's offset (grid y = screen y - this)
	float				mRows;				// +8 the rows in use
	int					mField10;			// +0x10 (4)
	GridCell			mCells[15][100];	// +0x14 column, row
	GridCell			mCells2[15][100];	// +0x2ef4
	Dynomite*			mBoard;				// +0x4664
	FossilizedApp*		mApp;				// +0x4668
	PtrList<void>		mList466c;			// +0x466c
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
	bool				mField46e4;			// +0x46e4
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
};

}

#endif
