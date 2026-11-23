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

class PokerBoard;
class IceStormBoard;
class DigBoard;

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

protected:
	virtual void DrawContent(Graphics* g);  // 0x7839f0 (bejeweled3), slot 94 (Lightning: 0x780130)
	void DrawStatisticsPanel(Graphics* g);  // 0x6a6470 (bejeweled3), slot 93
	void DrawStatisticsTable(Graphics* g);  // 0x6a60d0 (bejeweled3), slot 92
	virtual void DrawStatisticsLabels(Graphics* g); // 0x783a80 (bejeweled3), slot 89 (Lightning: 0x780160)
	virtual void DrawStatisticsValues(Graphics* g); // 0x783f00 (bejeweled3), slot 90 (Lightning: 0x7805e0)
	void DrawTopScoresPanel(Graphics* g);   // 0x6a7840 (bejeweled3), slot 91
	void DrawTopScores(Graphics* g);        // 0x6a68b0 (bejeweled3), slot 88
	void DrawSpecialGems(Graphics* g);      // 0x6a70f0 (bejeweled3), slot 87
};

// Sexy::SpeedEndLevelDialog (bejeweled3, vtable 0x86d034, 0x390 bytes, made by 0x716ab0 from the board's slot 233; the port's class is LightningEndLevelDialog): the screen a Lightning
// game ends on, the same dialog (id 0x26, the same three buttons, 0x6a7c80) with its own statistics and a results chart under them. Update (0x782550) is
// Classic's. See EndLevelDialog.cpp.
class LightningEndLevelDialog : public ClassicEndLevelDialog {
public:
	int mBonusSecondsTotal = 0;   // +0x38c: the board's +0x30b0, the seconds of all the time gems matched (0x716ab0)

	LightningEndLevelDialog(Board* theBoard, int theBonusSecondsTotal);   // 0x716ab0 (bejeweled3)

	void Draw(Graphics* g) override;   // 0x780aa0 (bejeweled3), slot 26

protected:
	void DrawContent(Graphics* g) override;            // 0x780130 (bejeweled3), slot 94: the table and the top scores, with no panel labels
	void DrawStatisticsLabels(Graphics* g) override;   // 0x780160 (bejeweled3), slot 89
	void DrawStatisticsValues(Graphics* g) override;   // 0x7805e0 (bejeweled3), slot 90

private:
	void DrawResults(Graphics* g);   // the second half of 0x780aa0: the graph, the icons with their counts, the chart's side labels
};

// Sexy::ButterflyEndLevelDialog (bejeweled3, vtable 0x86be7c, 0x390 bytes, made by 0x784c30 from the board's slot 257, 0x6d0590): the screen a Butterflies
// game ends on, Classic's dialog (id 0x26, the same three buttons, 0x6a7c80) with its own statistics. Its update (0x784cc0) and content (0x784370)
// do what Classic's do (0x784340, 0x7839f0), its Draw (0x784cb0) is a jump to Classic's 0x6a87a0. See EndLevelDialog.cpp.
class ButterflyEndLevelDialog : public ClassicEndLevelDialog {
public:
	explicit ButterflyEndLevelDialog(Board* theBoard);   // 0x784c30 (bejeweled3)

protected:
	void DrawStatisticsLabels(Graphics* g) override;   // 0x784400 (bejeweled3), slot 89
	void DrawStatisticsValues(Graphics* g) override;   // 0x784880 (bejeweled3), slot 90
};

// Sexy::InfernoEndLevelDialog (bejeweled3, vtable 0x8662c4, 0x390 bytes, made by 0x6d3180 from the board's slot 257): the screen an Ice Storm game
// ends on, Classic's dialog (id 0x26, 0x6a7c80) with its own statistics: the highest multiplier, the columns crushed, the best column combo
// and the time (strings 0x10f to 0x112). Its Draw (0x790500) adds the boxes and icons of the ICESTORM_GAMEOVER_DLG_ constants: not translated.
class IceStormEndLevelDialog : public ClassicEndLevelDialog {
public:
	int mBestChain = 0;   // +0x38c: the board's +0x3670, the most columns crushed in a chain

	explicit IceStormEndLevelDialog(IceStormBoard* theBoard);   // made inline by 0x6d3180 (bejeweled3): 0x6a7c80, the vftable, the board and its best chain

protected:
	void DrawContent(Graphics* g) override;             // 0x6a5f50 (bejeweled3), slot 94: the table and the top scores, as Lightning's
	void DrawStatisticsLabels(Graphics* g) override;    // 0x78fbb0 (bejeweled3), slot 89
	void DrawStatisticsValues(Graphics* g) override;    // 0x790030 (bejeweled3), slot 90
};

// Sexy::DigGoalEndLevelDialog (bejeweled3, vtable 0x865c54, made by the board's slot 257): the screen a Diamond Mine game ends on, Classic's dialog (id 0x26, 0x6a7c80) with
// its own statistics: Max Depth, Total Time, Best Move and Best Treasure (strings 0xcc to 0xcf; the values 0xd0 to 0xd5). Its drawing of the frames, the treasure's
// tallies and badges (0x733af0, 0x7331f0) is not translated.
class DigEndLevelDialog : public ClassicEndLevelDialog {
public:
	int mDepth = 0;                    // the rows of depth the game reached
	int mBestTreasureValue = 0;        // the most one treasure was worth
	std::string mBestTreasureText;     // the name of the artifact that was worth most (empty: none was found)

	explicit DigEndLevelDialog(DigBoard* theBoard);

protected:
	void DrawContent(Graphics* g) override;             // slot 94: the table and the top scores, as Lightning's
	void DrawStatisticsLabels(Graphics* g) override;    // 0x732c90 (bejeweled3), slot 89
	void DrawStatisticsValues(Graphics* g) override;    // 0x7353d0 (bejeweled3), slot 90
};

// Sexy::PokerEndLevelDialog (bejeweled3, vtable 0x86c4c4, 0x3b8 bytes, made by 0x782cf0 from the board's slot 257, 0x6d1540): the screen a Poker game
// ends on: Classic's dialog (id 0x26, the same three buttons, 0x6a7c80) with its own statistics (Best Hand, Number of Hands, Skulls Busted, Skull
// Coin Flips: strings 0x191 to 0x194) and, in place of Classic's drawing, a chart of how many of each hand were made (0x782e10). See EndLevelDialog.cpp.
class PokerEndLevelDialog : public ClassicEndLevelDialog {
public:
	int mHandCounts[8] = {};      // +0x39c: the board's +0x39e4 (the hands a skull took, then the seven kinds)
	int mHands = 0;               // the hands the game scored
	int mBestHand = 0;            // the highest kind it made (0: none)
	int mSkullsBusted = 0;        // the board's +0x3974
	int mCoinFlips = 0;           // the board's counter 3

	explicit PokerEndLevelDialog(PokerBoard* theBoard);   // 0x782cf0 (bejeweled3)

	void Draw(Graphics* g) override;   // 0x782e10 (bejeweled3), slot 26

protected:
	void DrawContent(Graphics* g) override;            // 0x782580 (bejeweled3), slot 94: the table and the top scores, as Lightning's
	void DrawStatisticsLabels(Graphics* g) override;   // 0x782870 (bejeweled3), slot 89
	void DrawStatisticsValues(Graphics* g) override;   // 0x7825b0 (bejeweled3), slot 90
};

}  // namespace Sexy
