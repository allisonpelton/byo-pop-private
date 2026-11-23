// Sexy::BadMoveLimitBoard (bejeweled3, vftable 0x86742c, constructor 0x6dd370, 0x3250 bytes) over Sexy::QuestBoard: the Quest boards of the Challenge "Bad Move Limit" (no
// quest of quest.cfg has it: the setting BadMovesAllowed is the number of swaps that find no match the game allows). The swap that is allowed back is a bad move; with
// all of them used and the board at rest the game ends. See docs/NOTES.md, "Quest".
#pragma once

#include "QuestBoard.h"

namespace Sexy {

class BadMoveLimitBoard : public QuestBoard {
public:
	int mBadMovesAllowed = 0;   // +0x3248: "BadMovesAllowed"
	int mBadMovesUsed = 0;      // +0x324c: the swaps that found no match

	void NewGame() override;                                 // slot 135 (0x6e4f60)
	void Update() override;                                  // slot 22 (0x6e4f30)
	void OnSwapFailed(const SwapRecord& theSwap) override;   // slot 192 (0x6c6050)
	bool HintAllowed() const override;                       // slot 98 (0x6c6c40)
	void DrawModeOverlay(Graphics* g) override;              // slots 124 (0x6ccb70, the text) and 240: the bad moves left
};

}  // namespace Sexy
