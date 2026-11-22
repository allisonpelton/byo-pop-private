// Sexy::Board (vtable 0x864cb4 (bejeweled3), made by 0x74ae30) and Sexy::ClassicBoard (RTTI vftable 0x86511c, 0x3088
// bytes, made by 0x73b390): the game screen of Classic mode, and Sexy::Piece (0x630 bytes, made by 0x7415c0): one gem.
// The board is an 8 by 8 grid of pieces; the exe measures it in 1200ths of the art height (a cell is 128), draws it at
// (0x20b, 0x4b) of that and scales every measure by the app's art size over 1200.
// port: only what Classic's start and first fill read is translated (docs/NOTES.md, "The board"): the swap, the matches,
// the update (0x759170) and the other modes' boards come with their steps. The exe's board has about 200 virtual
// functions that the modes override; Classic's overrides are constants (0x73b5a0 and 0x73bc00 to 0x73bc30), which this
// port reads in place (each use says which slot it stands for).
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "MTRand.h"
#include "Widget.h"

#include <vector>

namespace Sexy {

class Board;

// One gem. Fields are named by what the code that reads them does with them; the ones that nothing translated reads
// are left out (the fill also sets +0x182 to 1 and +0x244 to 0 on each piece it makes: nothing translated reads them).
class Piece {
public:
	enum { NO_COLOR = -1, COLOR_COUNT = 7 };
	// +0x228: what the gem is (the bits the fill reads; the specials come with their steps)
	enum { FLAG_RANDOM_PICK = 0x60 };

	Board* mBoard;          // +0
	int mId;                // +4: the board's piece counter (+0xf4) when it was made
	int mCol;               // +8
	int mRow;               // +0xc
	float mX;               // +0x10: where it is drawn, in 1200ths of the art height from the board's corner (a cell is 128)
	float mY;               // +0x14: new pieces start above the board and fall to row * 128
	int mCreatedUpdate;     // +0x18: the board's update count when it was made
	float mFallDelay;       // +0x1c: 0 until it first falls, then when it starts to fall (the column's, plus a step)
	int mColumnTag;         // +0x188: what the column's tag (board +0x218) held when it was made
	bool mCanFall;          // +0x17e: set by every pass of the fill over it
	bool mCanSwap;          // +0x17f: 1 at the start
	int mSpecialState;      // +0x20c: -1 keeps it from falling
	int mFallStartUpdate;   // +0x218: the board's update count when it first fell
	int mColor;             // +0x220: -1 for none, else 0 to 6 (red, white, green, yellow, purple, orange, blue)
	int mFlags;             // +0x228: 0 for a plain gem
	int mBusyCount;         // +0x230: above 0 while something is done to it: the fill waits for it

	explicit Piece(Board* theBoard);   // 0x7203b0 (bejeweled3), with 0x720120 (bejeweled3)
};

class Board : public Widget, public ButtonListener {
public:
	enum { COLS = 8, ROWS = 8, CELL = 128 };
	// 0x73bd80 and 0x73bd70 (bejeweled3): where the grid is, in 1200ths of the art height
	enum { GRID_X = 0x20b, GRID_Y = 0x4b };

	Piece* mGrid[ROWS][COLS];        // +0xf8: row after row; null where there is no gem
	int mUpdateCount = 0;            // +0x38: the board's updates so far
	int mPieceCounter = 0;           // +0xf4: the number of pieces made, each one's id
	float mColumnDelay[COLS];        // +0x1f8: where a column's last piece started to fall (the grid's ninth row)
	int mColumnTag[COLS];            // +0x218: -1 after a fill (the grid's tenth row)
	MTRand mRand;                    // +0x258: the board's own random numbers, seeded from the app's Rand() when it is made
	int mLastRow = ROWS - 1;         // +0x22e4: the last row
	int mFillHeld = 0;               // +0x2ef8: while it is not 0 the fill does nothing
	int mBackgroundIndex = -1;       // +0x2864: which of the game's backgrounds, in the order of 0x8d2170
	int mHintCount = 0;              // +0xe04: 0 after a reset (0x73c410 reads it)
	std::vector<int> mColorList;     // +0x105c: the colours a new piece may get: each of the seven twice
	bool mNoSavedGame = true;        // port: the saved game of this mode (0x74e1f0) is not read: a new game starts
	int mScore = 0;                  // +0xd64
	CurvedVal mCoverCurve;           // +0x2898: 1 from the constructor (0x74b76c): while it is 1 the main menu behind is hidden (0x6a3530)

	Board();                                   // 0x74ae30 (bejeweled3)
	~Board() override;
	void ButtonDepress(int theId) override {}  // 0x741130 and the others' buttons come with their steps

	// slot 85, 0x74a880 (bejeweled3): the board is emptied for a new game
	void ResetGame();
	// slot 135, 0x74e410 (bejeweled3): a new game: its background is chosen and the first fill is made
	void NewGame();
	// slot 203, 0x763e90 (bejeweled3): makes the pieces that the empty cells need above the grid, lets them fall into
	// place one row at a time and gives them colours that make no match and leave a move. theNoMatchCheck is the
	// exe's second argument (set when it gave up). Classic's first fill is this with 0 for it.
	void FillInBlanks(bool theNoMatchCheck);

	// 0x73ba40 (bejeweled3): the grid has three or more of a colour in a row or column
	bool HasMatches() const;
	// 0x75a8e0 (bejeweled3) with no piece to include: some swap of two neighbours makes a run of three. theNeedBonus is
	// its last argument: only a run of four or more, or a crossing of two, counts
	bool HasMove(bool theNeedBonus);

	// 0x7415c0 (bejeweled3): a new piece at a cell
	Piece* MakePiece(int theCol, int theRow);
	// 0x73bd80 and 0x73bd70 (bejeweled3)
	int GetGridX() const { return GRID_X; }
	int GetGridY() const { return GRID_Y; }

	// slot 89, 0x7413e0 (bejeweled3)
	virtual const char* GetModeName() const { return "Classic"; }
	// 0x751d50 and 0x750320 (bejeweled3): the board's panel and gems
	void Draw(Graphics* g) override;
	void DrawPiece(Graphics* g, const Piece* thePiece);   // slot 237, 0x757df0 (bejeweled3)
};

class ClassicBoard : public Board {
public:
	ClassicBoard();   // 0x73b390 (bejeweled3)
};

}  // namespace Sexy
