// Sexy::WallBlastGoal (bejeweled3, vftable 0x86b814, constructor 0x6ab720, 0x260 bytes; quest.cfg's Goal "Wall Blast"), Sexy::GoldLinesGoal (vftable 0x86affc, constructor
// 0x6afcd0, 0x940 bytes; "Gold Lines") and Sexy::GoldLinesFlipGoal (vftable 0x86b93c, constructor 0x6afe00, 0x944 bytes; "Gold Lines Flip"), goals of a Quest board that mark
// cells of the 8x8 grid. See docs/NOTES.md, "Quest, part 3".
#pragma once

#include "QuestGoal.h"

#include <string>
#include <vector>

namespace Sexy {

// "Wall Blast": a wall of cells (the setting Grids, one chosen by a draw) that a gem cleared on it, or a special's reach, breaks; the goal is every cell of the wall
class WallBlastGoal : public QuestGoal {
public:
	enum { COLS = 8, ROWS = 8, CELL_COUNT = 64 };

	// a wall cell (the exe's 4 bytes at +0x118 + 4 * (col + 8 * row))
	struct WallCell {
		bool mPresent = false;        // +0: the wall is still there
		bool mWasPresent = false;     // +1: the copy made by Load (only the exe's drawing reads it)
		bool mFlipA = false;          // +2: a random flag of the wall's look
		bool mFlipB = false;          // +3: another
	};

	int mWallCount = 0;               // +0x110: the cells the wall had (the goal's size)
	int mDropThreshold = 0;           // +0x114: "DropThreshold" (the special gems' drops, which the goal's slot 46 never lets happen: see NOTES.md)
	WallCell mCells[CELL_COUNT];      // +0x118
	int mDropCountdown = 0;           // +0x218: DropThreshold again after Load

	void Load() override;                                      // slot 53 (0xd4), 0x6b0fb0
	int GetLevelMax() override { return mWallCount; }          // slot 22 (0x58)
	void OnCellHit(int theCol, int theRow) override;           // slot 16 (0x40), 0x6afcb0
	void OnPieceClearing(Piece* thePiece, bool theFlag) override;   // slot 39 (0x9c), 0x6ad9d0
	void DrawBehindPieces(Graphics* g) override;               // slot 59 (0x6afe80) in the exe
	std::string DescribeState() const override;

	// 0x6ad740 (bejeweled3): the wall at the cell is broken: the score (+0xd24) and the progress (+0xe00) go up by one. port: the exe's sparks (random draws too) are not made
	void BreakCell(int theCol, int theRow);
	int CellsLeft() const;
};

// the 8x8 gold cells of "Gold Lines" and "Gold Lines Flip" (the exe's 0x20-byte record at +0x11c + 0x20 * col + 0x100 * row)
class GoldLinesGoal : public QuestGoal {
public:
	enum { COLS = 8, ROWS = 8 };
	// the ticks a cell waits for each step of distance from the middle of a run (0x6b1380)
	enum { TICKS_PER_STEP = 10 };
	// the pixel size of a cell in 1200ths (the exe divides the piece's position by it) and the grid's span
	enum { CELL_SIZE = 128, GRID_SPAN = 1024 };

	struct GoldCell {
		std::vector<int> mDelays;     // +0xc: the ticks left of each flip still to come
		int mLevel = 0;               // +0x18: -1 not touched yet (Gold Lines Flip), 0 lead, 1 gold
	};

	int mSquares = 0;                 // +0x110: "Squares", the cells that must be gold
	bool mOnlyCheckSwaps = false;     // +0x118: "OnlyCheckSwaps" is TRUE
	bool mIsFlip = false;             // port: Gold Lines Flip, whose lead cells are drawn
	GoldCell mCells[ROWS][COLS];

	void Load() override;                                      // slot 53 (0xd4), 0x6b04f0
	bool CanEndGame() override;                                // slot 10 (0x28), 0x6aaa00
	void UpdateTick() override;                                // slot 11 (0x2c), 0x6abeb0
	int GetLevelMax() override { return mSquares; }            // slot 22 (0x58), 0x6aa200
	bool IsLevelFull() const override;                         // slot 26 (0x68), 0x6aa1d0
	void OnRunFound(const std::vector<Piece*>& thePieces) override;   // slot 17 (0x44), 0x6b1380
	void OnPieceCleared(Piece* thePiece) override;             // slot 40 (0xa0), 0x6aa220
	void DrawBehindPieces(Graphics* g) override;
	std::string DescribeState() const override;

	// 0x6b78f0 (slot 72): the cell turns one step on (to lead, then to gold); a cell that is gold scores
	void FlipCell(int theRow, int theCol);
	int GoldCount() const;
};

// "Gold Lines Flip" (0x6b0f20): the same, with every cell starting at -1, so the first flip of a cell only turns it to lead and the second makes it gold
class GoldLinesFlipGoal : public GoldLinesGoal {
public:
	void Load() override;   // slot 53 (0xd4), 0x6b0f20
};

}  // namespace Sexy
