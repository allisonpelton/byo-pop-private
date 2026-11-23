// Sexy::FillerBoard (bejeweled3, vftable 0x86817c, constructor 0x6dcc00, 0x3368 bytes) over Sexy::QuestBoard: the Quest boards of the Challenge "Filler" (the
// Avalanche quests). The board starts with a few gems (NumStartGems) and gems drop in after each move (DropPerMove) instead of the grid being filled to the top: the
// fill (slot 203, 0x6db550) makes as many new gems as the credit allows, spread over the columns by how much room each has. The game is lost when the top row
// stays full. See docs/NOTES.md, "Quest".
#pragma once

#include "CurvedVal.h"
#include "QuestBoard.h"

namespace Sexy {

class FillerBoard : public QuestBoard {
public:
	// 0x6db550: the drop is tried again this many times before one more gem is made for the empty cells; fewer gems than MIN_FILL_BONUS_GEMS may be asked to make a bonus
	// move; the weight of a column is its free cells to the power GRID_COLUMN_WEIGHT_POWER
	enum { TRIES_BEFORE_EXTRA_GEM = 500, MIN_FILL_BONUS_GEMS = 10, GRID_COLUMN_WEIGHT_POWER = 4 };
	// port: a fill that has found no arrangement in this many tries takes the last one (the exe has no limit; Board's own is 200000)
	enum { TRY_LIMIT = 20000 };

	int mDropPerMove = 0;          // +0x3248: "DropPerMove": the gems the credit grows by for each move that finds a match
	int mCredits = 0;              // +0x324c: "NumStartGems" at the start: the gems still to be dropped in
	bool mWasSettled = false;      // +0x3250: the board was at rest in the last update
	bool mDropAllowed = false;     // +0x3251: no hint comes while a drop is owed (0x6c6b10)
	int mGemsAtMoveStart = 0;      // +0x3254: the gems on the screen when the board last left its rest
	int mGemsDropped = 0;          // +0x3258: the gems the fills have dropped since then
	int mGemsRemoved = 0;          // +0x325c: the gems that have been taken off since then
	float mFullWarning = 0.0f;     // +0x3268: 0 to 1, how long the top row has stayed full (0x6dc770 adds 0.0075 in each update at rest)
	CurvedVal mAssist;             // +0x32f8: "AssistCurve": the number of tries the fill asks for a move that makes a bonus, by how many cells are empty

	void NewGame() override;                                        // slot 135 (0x6e3420)
	void Update() override;                                         // slot 22 (0x6e3130)
	int GetGridY() const override { return 0x7d; }                 // slot 172 (0x6c52d0)
	bool OnPieceRemoving(Piece* thePiece) override;                 // slot 176 (0x6d7c20)
	void MakeHypercubeGroup(Piece* theCube, int theColor) override; // slot 180 (0x6cff00)
	void OnSwapMatched(const SwapRecord& theSwap) override;         // slot 191 (0x6c52a0)
	void NoMoreMoves(bool theAnnounce) override;                    // slot 161 (0x6c50f0)
	bool HintAllowed() const override;                              // slot 98 (0x6c6b10)
	void UpdateFalling() override;                                  // slot 214 (0x6c6e90)
	void UpdateWarnings() override;                                 // slot 181 (0x6dc770)
	void FillInBlanks(bool theNoMatchCheck) override;               // slot 203 (0x6db550)
	void DrawModeOverlay(Graphics* g) override;                     // slots 218, 220, 115, 26 and 47: the credit and the warning

private:
	// slot 199 (0x6c5100): a run of 3 or more of one colour, along a row or a column, that holds a gem made in this update
	bool FillMadeRun() const;
};

}  // namespace Sexy
