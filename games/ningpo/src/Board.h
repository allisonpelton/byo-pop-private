#ifndef __NINGPO_BOARD_H__
#define __NINGPO_BOARD_H__

// The board (0x1698 bytes at NingPo+0x6b28, 0x405e50..0x406cd0) and its tiles (0x20 bytes, 0x40c740..0x40c840).
// A layout (a .nml file of the archive) gives the 144 cells: x and y on a grid of half tiles, z the level. A new game
// puts the 144 tiles of the set into random cells, and the tiles are drawn in an order sorted once (0x4067f0) so that
// the higher levels and the tiles to the left and below come last.

#include "Pg.h"

namespace ningpo
{

using pg::PGPoint;
using pg::PGRect;
using pg::PyTime;

struct Tile
{
	bool mActive;            // +0x00 on the board
	bool mSelected;          // +0x01
	int mType;               // +0x04 what matches: the 34 suits and honours, the flowers 34, the seasons 35
	int mX;                  // +0x08 in half tiles
	int mY;                  // +0x0c
	int mZ;                  // +0x10
	int mFace;               // +0x14 the cell of the tile set's sheet: 13 columns of 39 x 55
	PyTime mRemovedTime;     // +0x18 the removed tile fades out for 500 ms

	Tile();                                 // 0x40c740
	void getFaceRect(PGRect* theRect) const;   // 0x40c760: the face, without the tile's edges
	void getRect(PGRect* theRect) const;       // 0x40c7d0: the whole tile
	void getDirtyRect(PGRect* theRect) const;  // 0x40c840: the tile with its shadows
};

class Board
{
public:
	enum
	{
		NUM_TILES = 0x90,
		NUM_UNDOS = 0x48,
	};

	Tile* mUndo[NUM_UNDOS][2];      // +0x0000 the pairs removed
	int mUndoIndex;                 // +0x0240 the last pair's, -1 when there is none
	Tile* mDrawOrder[NUM_TILES];    // +0x0244
	Tile mTiles[NUM_TILES];         // +0x0488
	Tile* mHover;                   // +0x1688 the tile under the mouse
	Tile* mSelected;                // +0x168c
	int mTilesLeft;                 // +0x1690
	int mMoves;                     // +0x1694 the moves the board has (0x405ef0)

	Board();                        // 0x405e50

	// 0x405eb0: the free tiles
	int countFree();
	// 0x405ef0: for each type with a free tile, half of its free tiles (rounded up)
	int countMoves();
	// 0x405fc0: the tiles inside theClip; the hint's tiles pulse with thePulse, and theFade (the "show moves"
	// button) lights the free tiles
	void draw(const PGRect* theClip, Tile* theHint0, Tile* theHint1, Tile* theHint2, Tile* theHint3, float thePulse,
		float theFade);
	// 0x4061f0: the topmost tile at a point
	Tile* tileAt(const PGPoint* thePoint, bool theActiveOnly);
	// 0x4062f0: the tile of a cell
	Tile* tileAtCell(int theX, int theY, int theZ, bool theActiveOnly);
	// 0x406350: no tile above covers the whole tile (the hint shows only matches that can be seen)
	bool isVisible(Tile* theTile);
	// 0x4064a0: nothing on top, and the left or the right side open
	bool isFree(Tile* theTile);
	// 0x406600: theName's cells into theCells (theSize bytes, three ints a cell); false when the file is missing or
	// too large
	bool loadLayout(const char* theName, int* theCells, unsigned int theSize);
	// 0x4066d0
	void newGame(const char* theLayout);
	// 0x4067f0
	void sortDrawOrder();
	// 0x4068c0: the other tiles on the board of theTile's type, at most 3; their number
	int getMatches(Tile* theTile, Tile** theMatches);
	// 0x406910: the next free tile from theStart (cyclic) that matches theTile, its index in *theIndex (-1 for none)
	Tile* nextFreeMatch(Tile* theTile, int* theIndex, int theStart);
	// 0x4069a0: a move (two free tiles that match), the theNth of those the board has, counted cyclically; false when
	// there is none. Either output may be null
	bool findMove(Tile** theTile, Tile** theOther, int theNth);
	// 0x406aa0
	void deselect();
	// 0x406ac0: the pair taken off and pushed on the undo list
	void removePair(Tile* theTile, Tile* theOther);
	// 0x406b50: the last pair back on the board, or nulls
	void undo(Tile** theTile, Tile** theOther);
	// 0x406be0
	void clearUndo();
	// 0x406c00: the tiles' faces swapped at random until there is a move
	void shuffle();
};

// 0x4065a0: one line of a layout, "x,y,z", into theCell; the next line
const char* ParseLayoutLine(const char* theLine, int* theCell);
// 0x406850: the order the tiles are drawn in
int CompareDrawOrder(Tile** a, Tile** b);

} // namespace ningpo

#endif
