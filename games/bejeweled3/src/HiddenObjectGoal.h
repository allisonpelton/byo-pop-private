// Sexy::HiddenObjectGoal (bejeweled3, vftable 0x86ba64, constructor 0x6afb60, 0x1f80 bytes; quest.cfg's Goal "Hidden Objects": Buried Treasure, Sandstorm and Sandstorm II): a few
// objects lie under the grid's 8x8 tiles; a gem cleared on a tile (or a special's reach) uncovers it, and an object is found when every tile of it is uncovered at once. The goal is a class
// of its own (it does not use HiddenGoal 0x86b24c or HiddenGridGoal 0x86b374). See docs/NOTES.md, "Quest, part 3".
#pragma once

#include "QuestGoal.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class HiddenObjectGoal : public QuestGoal {
public:
	enum { COLS = 8, ROWS = 8, CELL_COUNT = 64 };
	// 0x6afb60: the quest clock starts at 550 and the wait at 200 (QuestBoard's constructor puts 200 and 200)
	enum { CLOCK_START = 0x226, CLOCK_WAIT = 200 };
	// 0x6afb60: the refill time before RefillRateSec is read, ticks: a tile uncovered stays so (the countdown of a tile only runs below this)
	enum { REFILL_NEVER = 10000000 };
	// 0x6af970: RefillRateSec is in seconds and the countdown is in ticks of 1/100 s (the constant at 0x880d90)
	enum { TICKS_PER_SECOND = 100 };
	// 0x6aec80: the shapes (ItemSet 1, 2, 3 take the ids 0 to 2, 3 to 5 and 6 to 9), the layouts made, the tries a shape has for a place and the shape's size limits
	enum { SHAPE_COUNT = 10, LAYOUT_COUNT = 15, PLACE_TRIES = 0x1f3 };
	// the change of a tile's cover in one update (the exe's curves of 0x6c24c0 run 100 updates)
	static constexpr float REVEAL_STEP = 0.01f;

	// one of the exe's ten records of 0x6aec80 (0x38 bytes each): the shape's size and its pattern ("X" marks a cell of the shape; empty is a full rectangle)
	struct Shape {
		int mWidth;
		int mHeight;
		const char* mPattern;
	};
	static const Shape SHAPES[SHAPE_COUNT];

	// a hidden object (the exe's 0x48-byte record in the vector at +0x1f20)
	struct HiddenObject {
		int mCol = 0;               // +0
		int mRow = 0;               // +4
		int mWidth = 0;             // +8
		int mHeight = 0;            // +0xc
		int mShape = 0;             // +0x10 (the exe keeps the shape's picture id there)
		bool mFound = false;        // +0x18: every tile of it was uncovered
		const char* mPattern = "";  // +0x28: the shape's pattern
		int mSeed = 0;              // +0x44: a draw modulo 1000 (its looks)
	};

	// a tile (the exe's 0x78-byte record at +0x110 + 0x78 * col + 0x3c0 * row)
	struct Tile {
		int mRefill = 0;            // +0: the ticks the tile stays uncovered (0: covered; REFILL_NEVER and more never count down)
		int mObject = -1;           // +4: the index of the object under it (-1 none)
		float mReveal = 0.0f;       // the exe's curve of the tile (+0x50..): 0 covered, 1 uncovered
	};

	int mItemSet = 0;               // +0x1f10: "ItemSet"
	int mRefillTicks = REFILL_NEVER;   // +0x1f54: "RefillRateSec" times 100 (set when the setting is there)
	bool mNeverAllowCascades = false;   // board +0x3228: the setting NeverAllowCascades (the exe reads it in 0x6ddc70)
	Tile mTiles[ROWS][COLS];
	std::vector<HiddenObject> mObjects;   // +0x1f20: the layout's objects (+0x1f4c is their count)
	std::map<int, bool> mQueued;    // +0x1f2c: the ids of the pieces that have uncovered a tile since the board last stood settled (the value is true until the piece's tile is uncovered)

	explicit HiddenObjectGoal(QuestBoard* theBoard);   // 0x6afb60
	void Load() override;                                      // slot 53 (0xd4), 0x6af970
	int GetLevelMax() override { return (int)mObjects.size(); }   // slot 22 (0x58), 0x6a9950
	void Update() override;                                    // slot 54 (0xd8), 0x6c24c0
	void OnCellHit(int theCol, int theRow) override;           // slot 16 (0x40), 0x6b9a80
	void OnRunFound(const std::vector<Piece*>& thePieces) override;   // slot 17 (0x44), 0x6b7b80
	void DrawBehindPieces(Graphics* g) override;               // slot 59 (0x6b9ca0) in the exe
	std::string DescribeState() const override;

	// 0x6aec80 (bejeweled3): the layouts are made and the best one is kept
	void PlaceObjects();
	// 0x6b7a50: a piece that is not queued yet is queued and the queue is run; 0x6b75e0: each queued piece that is still on the board uncovers the tile it is on
	void QueueTile(int thePieceId);
	void RunQueue();
	// 0x6b7400: the tile is uncovered for the refill time
	void UncoverTile(int theCol, int theRow);
	// 0x6baff0: an object whose tiles are all uncovered is found: the score (+0xd24) and the progress (+0xe00) go up
	void CheckObjects();
	int FoundCount() const;
};

}  // namespace Sexy
