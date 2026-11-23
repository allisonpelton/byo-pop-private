// Sexy::QuestGoal (bejeweled3, vftable 0x86a6bc, 72 slots, 0x10d bytes), the goal object of a Quest board (QuestBoard's +0x3180), and the goals made on it
// that the port has: ClearGoal (vftable 0x86ab5c, 0x11c bytes; quest.cfg's Goal "Clear Gems") and DigGoal (src/DiamondMineBoard.h). The exe's QuestBoard hands
// most of its slots to the goal when it has one (QuestBoard::NewGame, 0x6ddc70, makes it from the quest's setting Goal); the base's slots are no-ops or
// constants that a goal overrides. The port has the base's slots that the translated goals and boards reach. See docs/NOTES.md, "Quest".
#pragma once

#include <string>

namespace Sexy {

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

// the goal of the quest's setting Goal (0x6ddc70 compares the setting's upper-case text with the names in a fixed order); null for a goal that is not made
// yet, and for no goal
QuestGoal* MakeQuestGoal(const std::string& theName, QuestBoard* theBoard);

}  // namespace Sexy
