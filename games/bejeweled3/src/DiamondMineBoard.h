// Sexy::DigBoard, the port's DigBoard (bejeweled3, vftable 0x8657d4, 266 slots, 0x3280 bytes, made by 0x737420 over Sexy::TimeLimitBoard (vftable 0x86a234,
// 0x6dd3b0) over Sexy::QuestBoard; the mode's start 0x774060 picks it when the quest's setting Challenge reads DIG, the secret quest 3 of secret.cfg,
// "Diamond Mine"), and Sexy::DigGoal (vftable 0x86bbac, 72 slots, made by 0x6c4230 from QuestBoard's new game 0x6ddc70, the board's +0x3180) that holds
// the mode's logic: the secret mode where the bottom of the grid is dirt. A gem that is cleared hits the four cells round it; dirt takes its hits
// and goes, and gold, diamonds and artifacts in the dirt are collected by a hit and are the score. When the top six rows hold no dirt the grid
// scrolls (the dirt and the gems rise by a row, a new row of dirt comes in at the bottom), and a scroll adds seconds to the clock. The game ends when the
// time is out. See docs/NOTES.md, "Diamond Mine", for the slots it changes (each function here says its slot and address).
#pragma once

#include "QuestGoal.h"
#include "TimeLimitBoard.h"
#include "CurvedVal.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class DigBoard;
class ClassicEndLevelDialog;

// 0x784f80, 0x784fe0 and 0x784e90 (bejeweled3): a table of 500 floats that turns an even random number into one with a chosen spread. It is made from a curve
// (the settings BrickStrSpread, MineStrSpread and ArtifactSpread): the curve's value at i/500 is how likely the numbers near i are
class SpreadTable {
public:
	enum { TABLE_SIZE = 500 };
	float mTable[TABLE_SIZE];
	SpreadTable();                          // 0x784f80: the table is i / 500
	void Build(CurvedVal& theCurve);        // 0x784fe0
	double Lookup(double theValue) const;   // 0x784e90
};

class DigGoal : public QuestGoal {
public:
	enum { COLS = 8, ROWS = 8 };
	// the kinds of what is in a block of dirt (a brick, the exe's 0x20-byte record in the map at +0x228): 0x6b3700 sets the kind at +8
	enum { KIND_ARTIFACT = 0, KIND_DIRT = 1, KIND_MONEY = 2 };
	// the piece flag of a brick (0x10000, set by 0x6bbb10 and 0x6b3a50)
	enum { FLAG_BRICK = 0x10000 };
	// the dirt's strengths: 4 is hit down only by a hit of a special gem, 5 is the dark rock (0x6b59f0)
	enum { STRENGTH_NEEDS_POWER = 4, STRENGTH_DARK_ROCK = 5, STRENGTH_DIRT_MAX = 5, STRENGTH_MONEY_MAX = 7 };
	// 0x6a9780: the updates the tally of a kind is shown for after a treasure (+0x3c0)
	enum { TALLY_SHOWN_TICKS = 0x78 };
	// 0x6bbb10: how a piece's cell hides: the kinds of the money (the strengths 1 to 3 are gold, 4 to 7 diamonds)
	enum { GOLD_STRENGTH_MAX = 3, TREASURE_RANGE_KINDS = 7 };
	// 0x6be3e0: a new row's gems are given colours again up to this many times while they make a match
	enum { NEW_ROW_COLOR_TRIES = 25 };
	// 0x6be3e0: after a scroll with no move left, this many tries to put a hypercube in the top row
	enum { NO_MOVE_HYPERCUBE_TRIES = 49 };
	// 0x6c0430: the scroll's two curves
	static const char* const SCROLL_DEPTH_CURVE;    // +0x518: 0 to 1
	static const char* const SCROLL_STEPS_CURVE;    // +0x588: 0 to 10

	// the exe's record of one block of dirt (0x20 bytes in the map at +0x228, by the piece's id)
	struct Brick {
		int mHitsLeft = 0;          // +0: the hits left (an artifact's block holds the artifact's index here, a block of money its strength)
		int mHitsInitial = 0;       // +4
		int mKind = KIND_DIRT;      // +8
		int mSeed = 0;              // +0xc: a number from the app's Rand() (its looks)
		bool mFirstInColumn = false;   // +0x18: the first block made in its column by the call of 0x6bbb10 (its looks)
	};

	// one artifact of the settings Artifact1 to Artifact46: "id, minDepth, value, text" (the exe's 100-byte record in the vector at +0x11c)
	struct Artifact {
		std::string mId;            // +0
		int mMinDepth = 0;          // +0x38: the depth from which it can be found
		int mValue = 0;             // +0x3c: times ArtifactBaseValue
		std::string mText;          // +0x1c: the name the player reads
	};

	// the grid of the settings (Grids): a cell is a strength and a letter
	struct GridCell {
		int mStrength = 0;          // +0: the digit
		char mLetter = 0;           // +4: H hypercube, S shuffle, R random, M money, A artifact (upper case)
	};

	// a gem that was cleared: the four cells round it are hit (the exe's 16-byte entry in the list at +0x42c)
	struct HitEntry {
		int mCol = 0;
		int mRow = 0;
		bool mPowered = false;      // the gem had no colour (a hypercube) or was cleared by a hypercube
		int mMoveId = -1;           // +0x188 of the piece
	};

	// the board (+0x104, QuestGoal::mBoard) is a DigBoard
	DigBoard* Dig() const;

	// the settings (0x6c0430 reads them)
	int mTargetCount = 0;                       // +0x3bc: "TargetCount"
	int mDigCountPerScroll = 1;                 // +0x4fc: "DigCountPerScroll"
	int mGoldValue = 0;                         // +0x3ac: "GoldValue"
	int mDiamondValue = 0;                      // +0x3b0: "DiamondValue"
	int mArtifactBaseValue = 0;                 // +0x3b4: "ArtifactBaseValue"
	int mArtifactSkipTileCount = 0;             // +0xa64: "ArtifactSkipTileCount"
	int mPowerGemThresholdDepth0 = 0;           // +0xa68: "PowerGemThresholdDepth0"
	int mPowerGemThresholdDepth20 = 0;          // +0xa6c
	int mPowerGemThresholdDepth40 = 0;          // +0xa70
	int mArtifactMinTiles = 0;                  // +0x3fc: "ArtifactMinTiles"
	int mArtifactMaxTiles = 0;                  // +0x400: "ArtifactMaxTiles"
	std::vector<int> mTreasureRange;            // "TreasureRange": a minimum and a maximum for each kind of money
	std::vector<Artifact> mArtifacts;           // +0x11c
	std::vector<GridCell> mGridCells;           // +0x50c: the cells of "Grids", row after row
	bool mRandomAfterGrid = false;              // +0xb10: set when the setting MaxBrickStrPerLevel is there: the rows past the grid are random
	CurvedVal mMinBrickStr;                     // +0x7d8: "MinBrickStrPerLevel"
	CurvedVal mMaxBrickStr;                     // +0x848: "MaxBrickStrPerLevel"
	CurvedVal mEdgeBrickStr;                    // +0x8b8: "EdgeBrickStrPerLevel"
	CurvedVal mMinMineStr;                      // +0x928: "MinMineStrPerLevel"
	CurvedVal mMaxMineStr;                      // +0x998: "MaxMineStrPerLevel"
	CurvedVal mMineProb;                        // +0xa78: "MineProbPerLevel"
	CurvedVal mDarkRockFrequency;               // +0x768: "DarkRockFrequency"
	CurvedVal mArtifactPossRange;               // "ArtifactPossRange"
	SpreadTable mBrickStrSpread;                // +0xa08
	SpreadTable mArtifactSpread;                // +0xa24
	SpreadTable mMineStrSpread;                 // +0xa40

	// the game's state
	std::map<int, Brick> mBricks;               // +0x228: by the piece's id
	std::vector<int> mHypercubeBrickIds;        // +0x45c: the ids of the dirt that hides a hypercube
	std::vector<int> mPlacedArtifacts;          // +0x1d4: the artifacts that were put in the dirt in this game
	std::vector<int> mCollectedArtifacts;       // +0x1bc: the artifacts the player has collected
	std::vector<HitEntry> mHits;                // +0x42c
	bool mHitMask[ROWS][COLS];                  // +0x21c: the cells a hit has reached since the update began (0x6b59f0 does not hit one twice)
	double mDepth = 0.0;                        // +0x4f0: the rows that have scrolled by
	double mRowsGenerated = 0.0;                // +0x4f8: the rows of dirt made so far
	bool mWholeBoardClear = false;              // +0x647: the last scroll came with the grid empty of dirt (the time bonus is the mega one and the scroll is twice as long)
	int mTicksToArtifact = 0;                   // +0x404: the bricks left to make before the next artifact is due
	CurvedVal mScrollDepthCurve;                // +0x518
	CurvedVal mScrollStepsCurve;                // +0x588
	bool mScrolling = false;                    // a scroll is running
	int mScrollRowsDone = 0;
	int mScrollRows = 0;                        // (clear + 1) * DigCountPerScroll: the rows this scroll moves
	int mTallyTicks = 0;                        // +0x3c0
	int mTallies[3] = {};                       // +0x3c4: the value collected, by kind (0 artifacts, 1 dirt (none), 2 money)
	int mGoldCollected = 0;                     // the pieces of gold collected (the end screen's Treasure Found counts)
	int mDiamondsCollected = 0;
	int mArtifactsCollected = 0;
	int mBestTreasureValue = 0;                 // the most one treasure was worth
	int mScrollCount = 0;                       // the scrolls so far
	int mBricksDestroyed = 0;                   // the blocks of dirt, money and artifacts that are gone

	DigGoal() { Clear(); }
	void Clear();
	// 0x6c0430 (bejeweled3) and 0x6c4230 (the constructor): the settings of the quest are read
	void Load() override;                   // slot 53 (0xd4)
	int GetLevelMax() override { return mTargetCount; }   // slot 22 (0x58), 0x6a9810: the treasures a quest asks for (a secret mode's board gives 0 before it asks)
	// 0x744e00 (bejeweled3): the text of "Grids" is read into cells
	void ParseGrid(const std::string& theText);
	// 0x6bbb10 (bejeweled3): the bricks of the rows from theStartRow to the last
	void MakeRows(int theStartRow);
	// 0x6b3a50 (bejeweled3): the piece becomes a random block: dirt or, by chance, money
	void MakeRandomBrick(Piece* thePiece, int theColumn, double theDepth, bool& theMade);
	// 0x6b3700 (bejeweled3): a brick's record is set
	void SetBrick(Piece* thePiece, int theKind, int theStrength);
	// 0x6aae50 (bejeweled3): no row of the first six (or all eight) holds a brick
	bool RowsAreFree(bool theAllRows) const;
	// 0x6c2ad0 (bejeweled3): the goal's update, which the board's slot 22 (0x7374d0) runs
	void Update() override;                 // slot 54 (0xd8)
	// 0x6b59f0 (bejeweled3): a brick is hit: true when it is destroyed
	bool DamageBrick(Piece* thePiece, bool thePowered, bool theUseMask);
	// 0x6b59f0 (bejeweled3): the treasure of a brick is collected: what it is worth
	int CollectTreasure(Piece* thePiece, const Brick& theBrick);
	// slot 143 (0x6ad0d0): a gem was cleared: the four cells round it are to be hit
	void OnPieceCleared(Piece* thePiece) override;
	// slot 176 (0x6bb6a0) with slot 142 (0x6b6a80): a piece is taken out; true keeps it
	bool OnPieceRemoving(Piece* thePiece) override;
	// 0x6c2ad0 (bejeweled3), its middle: the scroll starts: the time bonus is given and its curves are set
	void StartScroll();
	// 0x6be730 (bejeweled3): the scroll's curve goes on; a row scrolls at each step of it
	void UpdateScroll();
	// 0x6be3e0 (bejeweled3): the grid scrolls by one row
	void ScrollOneRow();
	// 0x6be3e0 (bejeweled3), its end: a hypercube is made when no move is left
	void EnsureMove();
	// 0x6b9690 (bejeweled3): the bricks are put at their cells
	void PlaceBricks();
	// 0x6bbb10: the depth of the first row now (the rows that have scrolled and the scroll's own, rounded)
	int CurrentDepth() const;
	// 0x6b59f0 and others: a piece with the brick flag (0x10000)
	bool IsBrick(const Piece* thePiece) const { return thePiece != nullptr && (thePiece->mFlags & FLAG_BRICK) != 0; }
	Brick* FindBrick(const Piece* thePiece);
	int CountBricks() const { return (int)mBricks.size(); }
	// the artifacts that can still be put in the dirt at this depth, in the order 0x6bbb10 takes them
	std::vector<int> ArtifactPool(int theDepth);
	// the game's whole depth in rows, as the end screen shows it
	int DepthShown() const;
};

class DigBoard : public TimeLimitBoard {
public:
	// 0x736f20: after a bonus, with less than this many hundredths left the clock's limit is put to 90 seconds
	// port: the updates a bonus' seconds are shown for beside the score
	enum { BONUS_SHOWN_TICKS = 300 };
	enum { SHORT_CLOCK_HUNDREDTHS = 9001, BONUS_LIMIT_SECONDS = 90, BONUS_BASE_TICKS = 0x23f0, BONUS_TICKS_LONG = 200 };

	// DigBoard's own
	int mMegaTimeBonus = 0;                 // +0x3278: "MegaTimeBonus"
	int mTimeLeftCap = 0;                   // +0x3274: the time left that a bonus gave, in hundredths (0: none yet); the clock shows no more than it
	int mLastBonusSeconds = 0;              // the seconds the last bonus gave (shown for a while)
	int mBonusShownTicks = 0;               // the updates it is still shown for
	bool mBonusWasMega = false;
	bool mDigDestroying = false;            // the goal's own hit is taking a broken brick out (the removal hook does not hit it again)
	bool mDigScrolling = false;             // the grid is scrolling: the pieces that go are not clears

	DigBoard();     // 0x737420 (bejeweled3)
	// the goal (QuestBoard's +0x3180) is a DigGoal, made by QuestBoard::CreateGoal from the setting Goal "Dig"
	DigGoal* Goal() const { return static_cast<DigGoal*>(mGoal); }

	const char* GetModeName() const override { return "BuriedTreasure"; }   // slot 89 (0x736e20): the string at +0x3258
	void ResetGame() override;                                                // slot 85 (0x737290)
	void NewGame() override;                                                  // slot 135 (0x7375e0, TimeLimitBoard's 0x6e5030 first)
	void Update() override;                                                   // slot 22 (0x7374d0)
	void UpdateTick() override;                                               // slot 234 (0x6e5660) and the time's end (slot 216, 0x73b890)
	void UpdateFalling() override;                                            // slot 214 (0x736d60)
	void OnPieceCleared(Piece* thePiece) override;                            // slot 143 (0x6ad0d0 through the goal)
	bool OnPieceRemoving(Piece* thePiece) override;                           // slots 176 and 142 (0x6bb6a0, 0x6b6a80 through the goal)
	void NoMoreMoves(bool theAnnounce) override;                              // slot 161 (0x737500)
	bool BotMayMove() const override;                                         // port: test only
	// the goal's own hit on a brick: true when it broke and was taken out (0x6c2ad0's calls of 0x6b59f0, which removes it through slot 176)
	bool DigHit(Piece* thePiece, bool thePowered, bool theUseMask);
	void AddPoints(int thePoints, int theMoveId = -1, bool theCountsForProgress = true, bool theUsesMultiplier = true) override;
	ClassicEndLevelDialog* MakeEndLevelDialog() override;                     // slot 257
	void DrawModeOverlay(Graphics* g) override;                               // slot 43 (0x737370)

	// slot 187 (0x2ec), 0x736b50: the hundredths of a second left
	int GetTimeLeft() const override;
	// 0x736f20 (bejeweled3): seconds are added to the clock (the mega bonus for a grid with no dirt left)
	void GiveTimeBonus(bool theMega);
};

}  // namespace Sexy
