// Sexy::BalanceBoard (bejeweled3, vftable 0x86789c, constructor 0x6dd090, 0x3390 bytes) over Sexy::QuestBoard: the Quest boards of the Challenge "Balance". Clearing red
// gems pushes a scale one way and clearing blue gems the other; the game is lost when the scale tips over. The level is full when Gems red and Gems blue have been cleared
// (the quests have no Goal). See docs/NOTES.md, "Quest".
#pragma once

#include "QuestBoard.h"

namespace Sexy {

class BalanceBoard : public QuestBoard {
public:
	enum { COLOR_RED = 0, COLOR_BLUE = 6 };
	bool CanPause() const override { return true; }   // slot 107 (0x6c53b0)

	int mGemsPerColor = 0;          // +0x3248: "Gems": the red gems and the blue gems that count
	int mBalanceScale = 0;          // +0x324c: "Balance": the difference of the two counts that tips the scale in the roll's time of one update
	int mRollSpeedDivisor = 0;      // +0x3254: "RollSpeedDivisor"
	int mCascadeThreshold = 0;      // +0x3258: "CascadeThreshold": a longer cascade than this has the fill check for matches (Board's fill is asked with 0)
	float mBalance = 0.0f;          // +0x3250: the scale, from -1 to 1 (above 1 either way the game is lost)
	int mBlueCount = 0;             // +0x325c: the blue gems cleared, at most Gems
	int mRedCount = 0;              // +0x3260: the red gems cleared, at most Gems

	void NewGame() override;                                    // slot 135 (0x6e4c80)
	void Update() override;                                     // slot 22 (0x6e3c90)
	void OnPieceCleared(Piece* thePiece) override;              // slot 143 (0x6d06a0)
	int GetLevelMax() const override { return mGemsPerColor * 2; }   // slot 154 (0x6c53a0)
	int GetGridY() const override { return 0x73; }             // slot 172 (0x6c5370): +0x2c50 (0, as Butterflies' 0x6c52f0 reads it) and 0x73
	int GetGridX() const override { return 0x1c2; }            // slot 171 (0x6c5380): +0x2c4c (0) and 0x1c2
	void DrawModeOverlay(Graphics* g) override;                 // slots 220, 238, 239, 244 and 248: the scale and the counts
};

}  // namespace Sexy
