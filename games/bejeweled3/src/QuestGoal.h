// Sexy::QuestGoal (bejeweled3, vftable 0x86a6bc, 72 slots, 0x10d bytes), the goal object of a Quest board (QuestBoard's +0x3180), and the goals made on it
// that the port has: ClearGoal (vftable 0x86ab5c, 0x11c bytes; quest.cfg's Goal "Clear Gems") and DigGoal (src/DiamondMineBoard.h). The exe's QuestBoard hands
// most of its slots to the goal when it has one (QuestBoard::NewGame, 0x6ddc70, makes it from the quest's setting Goal); the base's slots are no-ops or
// constants that a goal overrides. The port has the base's slots that the translated goals and boards reach. See docs/NOTES.md, "Quest".
#pragma once

#include <string>
#include <vector>

namespace Sexy {

class Graphics;
class Piece;
class QuestBoard;

class QuestGoal {
public:
	QuestBoard* mBoard = nullptr;   // +0x104: the board the goal belongs to (made by QuestBoard::CreateGoal)
	int mCounter = 0;               // +0x108: slot 54's counter of the updates (base 0x6a9aa0); no function of the export reads it
	bool mScores = false;           // +0x10c: the points of a match count on a board that does not score (QuestBoard's slot 152 reads it); 0 from the constructors

	QuestGoal() = default;
	virtual ~QuestGoal() = default;   // slot 0 (0x421860)

	// slot 53 (0xd4): the goal reads the quest's settings (the base does nothing)
	virtual void Load() {}
	// slot 54 (0xd8), run by QuestBoard::Update (0x6dd8a0). Base 0x6a9aa0: the counter goes up when the board's slot 103 (0x73c260) says it is not paused. port: the
	// counter is not kept (no function reads it)
	virtual void Update() {}
	// slot 10 (0x28), base: 1: the game may end now (QuestBoard's slot 161, 0x6d8e30, returns at once when it is false)
	virtual bool CanEndGame() { return true; }
	// slot 22 (0x58), base: 0: the size of the level in the bar's units (QuestBoard's slot 154, GetLevelMax)
	virtual int GetLevelMax() { return 0; }
	// slot 40 (0xa0), base a return: a piece is cleared (QuestBoard's slot 143 passes it on)
	virtual void OnPieceCleared(Piece* thePiece) { (void)thePiece; }
	// slots 18 and 39 (0x48 and 0x9c): a piece is taken out of the grid (QuestBoard's slots 176 and 142). The base's slot 18 returns 1, which lets the removal go on:
	// here true means the goal has kept the piece (the removal stops)
	virtual bool OnPieceRemoving(Piece* thePiece) { (void)thePiece; return false; }
	// slot 11 (0x2c, base 0x401df0 a return): the tick of the board's step (QuestBoard's slot 234, 0x6e5660, calls it before Board's step)
	virtual void UpdateTick() {}
	// slot 16 (0x40, base a return): a special's reach touches the cell (slot 197 of the board, see Board::OnCellHit)
	virtual void OnCellHit(int theCol, int theRow) { (void)theCol; (void)theRow; }
	// slot 17 (0x44, base 0x6a9b90 a return): a run of matched gems was found (QuestBoard's slot 211, 0x6c56a0). The exe gives the list of the clusters of one search; the port calls
	// it for each cluster in the search's order, with the cluster's pieces
	virtual void OnRunFound(const std::vector<Piece*>& thePieces) { (void)thePieces; }
	// slot 26 (0x68, base 0x6aa980, which is Board's 0x73b810): the level is full (QuestBoard's slot 217)
	virtual bool IsLevelFull() const;
	// slot 39 (0x9c, base 0x6a9b70 a return): a piece is being cleared (slot 142 of the board with the flag the second argument)
	virtual void OnPieceClearing(Piece* thePiece, bool theFlag) { (void)thePiece; (void)theFlag; }
	// port: what the goal draws behind the gems and over them (the exe draws by slots 56, 58 and 59); and the goal's keys for the test commands' state line (JSON members, each
	// starting with a comma)
	virtual void DrawBehindPieces(Graphics* g) { (void)g; }
	virtual void DrawOverPieces(Graphics* g) { (void)g; }
	virtual std::string DescribeState() const { return std::string(); }
};

// vftable 0x86ab5c (bejeweled3): the goal "Clear Gems": clear a number of gems (the setting Gems; GemsPerLevel more for each level past the first, and
// GemsMax at most when it is above 0). The goal overrides three of the base's slots
class ClearGoal : public QuestGoal {
public:
	int mGems = 0;            // +0x110: "Gems", the gems to clear in the first level
	int mGemsPerLevel = 0;    // +0x114: "GemsPerLevel", the gems more for each level after the first
	int mGemsMax = 0;         // +0x118: "GemsMax": the most the level asks for (0: no limit)

	void Load() override;                         // slot 53 (0xd4), 0x6ae510
	int GetLevelMax() override;                   // slot 22 (0x58), 0x6aa320
	void OnPieceCleared(Piece* thePiece) override;   // slot 40 (0xa0), 0x6aa360
};

// vftable 0x86aa34 (bejeweled3, constructor 0x6ae760, 0x110 bytes): the goal "Clear Bombs": clear a number of bombs (the setting Bombs: a gem that has a bomb's bits 0x60). The goal overrides
// three of the base's slots, and its constructor reads the setting, so the goal has its size before the board's first fill
class ClearBombsGoal : public QuestGoal {
public:
	int mBombs = 0;           // +0x110: "Bombs", the bombs to clear

	explicit ClearBombsGoal(QuestBoard* theBoard);   // 0x6ae760
	void Load() override;                         // slot 53 (0xd4), 0x6ae6b0: the setting again
	int GetLevelMax() override { return mBombs; }   // slot 22 (0x58), 0x6aa3b0
	void OnPieceCleared(Piece* thePiece) override;   // slot 40 (0xa0), 0x6ab280: a cleared piece with the bits 0x60 scores and counts
	std::string DescribeState() const override;
};

// the goal of the quest's setting Goal (0x6ddc70 compares the setting's upper-case text with the names in a fixed order); null for a goal that is not made
// yet, and for no goal
QuestGoal* MakeQuestGoal(const std::string& theName, QuestBoard* theBoard);

}  // namespace Sexy
