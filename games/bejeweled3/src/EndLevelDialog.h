// Sexy::ClassicEndLevelDialog (bejeweled3, vtable 0x86c01c, 0x390 bytes, made by 0x7842b0 over Sexy::EndLevelDialog's 0x6a7c80; the exe
// makes it from the board's slot 233, 0x73b100): the screen a Classic game ends on, dialog id 0x26. See EndLevelDialog.cpp for what is
// and is not made.
#pragma once

#include "Bej3Dialog.h"
#include "CurvedVal.h"

#include <string>
#include <vector>

namespace Sexy {

class Board;
class DialogButton;

class ClassicEndLevelDialog : public Bej3Dialog {
public:
	enum { DIALOG_ID = 0x26 };
	// the ids 0x6a7c80 gives its buttons (the Quest game's NEW QUEST, id 1, is not made)
	enum { BUTTON_MAIN_MENU = 0, BUTTON_PLAY_AGAIN = 2, BUTTON_BADGES = 3 };

	int mCounters[38] = {};            // +0x2c8: the board's counters (+0xc84) as the game ended
	int mScore = 0;                    // +0x2c4: the board's score (+0xd24)
	int mLevelIndex = 0;               // +0x378: the board's +0xe04
	int mLevelNumber = 0;              // +0x37c: the board's +0xe0c (nothing reads it)
	int mScoreRank = -1;               // +0x380: where the leaderboard put the score (its index), -1 for no place
	CurvedVal mScoreCurve;             // +0x250: the shown score counts up to the score by it
	std::vector<int> mScores;          // +0x24c: the leaderboard's list, the best first
	std::vector<std::string> mNames;   // its names (UTF-8)
	DialogButton* mMainMenuButton = nullptr;    // id 0, +0xc0 of its Bej3Button
	DialogButton* mBadgesButton = nullptr;      // id 3
	DialogButton* mPlayAgainButton = nullptr;   // id 2

	// 0x7842b0 with 0x6a7c80 (bejeweled3)
	explicit ClassicEndLevelDialog(Board* theBoard);
	~ClassicEndLevelDialog() override;

	void Update() override;                 // 0x784340 (bejeweled3), slot 22
	void Draw(Graphics* g) override;        // 0x6a87a0 (bejeweled3): Classic's slot 26 (0x784330) jumps to it
	void ButtonDepress(int theId) override; // 0x6a91e0 (bejeweled3)

private:
	void DrawContent(Graphics* g);          // 0x7839f0 (bejeweled3), slot 94
	void DrawStatisticsPanel(Graphics* g);  // 0x6a6470 (bejeweled3), slot 93
	void DrawStatisticsTable(Graphics* g);  // 0x6a60d0 (bejeweled3), slot 92
	void DrawStatisticsLabels(Graphics* g); // 0x783a80 (bejeweled3), slot 89
	void DrawStatisticsValues(Graphics* g); // 0x783f00 (bejeweled3), slot 90
	void DrawTopScoresPanel(Graphics* g);   // 0x6a7840 (bejeweled3), slot 91
	void DrawTopScores(Graphics* g);        // 0x6a68b0 (bejeweled3), slot 88
	void DrawSpecialGems(Graphics* g);      // 0x6a70f0 (bejeweled3), slot 87
};

}  // namespace Sexy
