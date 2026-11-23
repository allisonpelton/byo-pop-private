// Sexy::MoveLimitBoard (bejeweled3, vftable 0x868a84) over Sexy::QuestBoard: the Quest boards with a number of moves (quest.cfg's Challenge "Move Limit", the setting
// Moves). Each swap that finds a match uses a move; with none left and the board at rest the game ends. See docs/NOTES.md, "Quest".
#pragma once

#include "QuestBoard.h"

namespace Sexy {

class MoveLimitBoard : public QuestBoard {
public:
	int mMoves = 0;         // +0x3248: "Moves", the moves the quest gives
	int mMovesLeft = 0;     // +0x324c: the moves left

	void NewGame() override;                                                    // slot 135 (0x6e0170)
	void Update() override;                                                     // slot 22 (0x6e00e0)
	void OnSwapMatched(const SwapRecord& theSwap) override;                     // slot 191 (0x6c5a60)
	void MakeHypercubeGroup(Piece* theCube, int theColor) override;             // slot 180 (0x6cfb50)
	bool HintAllowed() const override;                                          // slot 98 (0x6c6500)
	void DrawModeOverlay(Graphics* g) override;                                 // slots 240 (0x6cba10), 47 and 242: the moves left
};

}  // namespace Sexy
