// Sexy::QuestBoard (bejeweled3, vftable 0x866fbc, 266 slots, constructor 0x6d9000 over Sexy::Board): what the Quest-derived boards share. The exe
// keeps a goal object at +0x3180 that most of its slots delegate to; the secret modes (Poker and Butterflies, started by 0x774060 with
// the secret flag at +0x3184 set) have none or only a counter, and with the flag set the board is an endless game: it has no level bar and
// no levels (slot 217, 0x6c5460, is false; slot 154, the level's size, is 0; slot 156, the level's end, does nothing). The port keeps what
// ButterflyBoard and PokerBoard read: the flag, the settings from secret.cfg, the background of the quest and the fill that stops once the
// game has ended. See docs/NOTES.md, "Butterflies and Poker".
#pragma once

#include "Board.h"
#include "QuestGoal.h"

#include <vector>

namespace Sexy {

class QuestBoard : public Board {
public:
	// 0x6d9000 (the constructor) puts 200 in +0x318c and +0x3190: the clock (+0xd20) that 0x6ca350 starts counts down from the first, and QuestBoard::Update compares it
	// with the second
	enum { QUEST_CLOCK_START = 200 };
	// 0x6dd8a0: the clock at which the win is told (the start less 2), and the ones the Update looks at: 100 (the fanfare of the finished level, slot 162), 50 (below it the clock
	// waits for the effects list, +0x26b8, to empty), and 0 (the end of the level, slot 233)
	enum { QUEST_CLOCK_WIN_TICK = QUEST_CLOCK_START - 2, QUEST_CLOCK_FANFARE = 100, QUEST_CLOCK_EFFECTS_WAIT = 50 };
	// 0x6dd8a0: the game-over counter (+0xd1c) at which a lost quest goes back to the quest map
	enum { QUEST_LOST_LEAVE_TICKS = 300 };

	bool mIsSecretMode = false;   // +0x3184: set by 0x774060's fourth argument, which the play menu gives for Poker and Butterflies
	// +0x3198 (0x6dd8a0): the updates this game has run: it goes up by 1 at the end of each update of QuestBoard::Update that is not paused (0x6cda70 and
	// 0x6ddc70 put it to 0). Read by Ice Storm's slot 22 (0x6e63c0) and 0x6e5850 (against the ice's own tick marks), 0x6e0450 (which keeps it as the
	// time the death began, +0x3cb8) and by 0x6d6d60 and 0x6db550 (the update a column or a chain was made in, +0x18, +0x218 and +0xdb0)
	int mUpdatesPlayed = 0;
	QuestGoal* mGoal = nullptr;   // +0x3180: the goal object (made by CreateGoal from the setting Goal; the board owns it)
	int mQuestIndex = -1;         // +0x3234: the quest's index in quest.cfg (the exe adds 1000 for the secret quests); -1 for the secret modes
	int mQuestClock = 0;          // +0xd20: 0 while the quest is played; 0x6ca350 starts it at QUEST_CLOCK_START when the level is full, and it counts down to the quest's end
	bool mScoresPoints = false;   // +0x3187: the board scores the points of its matches (QuestBoard's slot 152): the secret modes, and the quests with RecordHighScores TRUE
	bool mRecordsHighScores = false;   // +0x31ac: the game records a high score (0x6ca240 and 0x6c7c60 hand it in): the secret modes, and the quests with RecordHighScores TRUE
	bool mQuestWon = false;       // port: the quest was won (the profile has it marked as done)
	// +0x3198 (0x6dd8a0): the updates this game has run, counted before the quest's clock; 0x6cda70 puts it to 0 and no function of the export reads it
	int mUpdatesRun = 0;

	bool HasLevels() const override { return !mIsSecretMode; }                       // slot 217 (0x6c5460, false in the secret modes)
	int GetLevelMax() const override;                                                  // slot 154: 0 in the secret modes, else the goal's slot 22 (0x58)
	bool ShowsLevelNumber() const override { return !mIsSecretMode; }               // slot 249: a secret mode has no level to show
	bool IsFillStopped() const override { return mGameOverTicks != 0 || mQuestClock != 0; }   // slot 203 (0x6dd4d0): no fill once the end has begun (+0xd1c or +0xd20)
	float GetLevelFactor() const override { return 1.0f; }                            // slot 122 (0x73b600): the quest boards score at 1 (Butterflies, 0x6c5340, gives 5 in a secret mode)
	const char* GetModeName() const override { return GetTimeLimit() == 0 ? "QuestTurnBased" : "QuestTimeBased"; }   // slot 89 (0x6c7460): "QuestTurnBased" with no time limit (slot 104 is 0), else "QuestTimeBased"
	void ChooseBackground() override;                                                  // slot 100: the setting BackgroundIdx (0x74e410)
	void ResetGame() override;                                                         // slot 85 (0x6cda70): Board's, then the two update counters to 0
	void Update() override;                                                            // slot 22 (0x6dd8a0): Board's update, then the counters (see QuestBoard.cpp)
	void UpdateTick() override;                                                        // slot 234 (0x6e5660): the goal's slot 0x2c, then Board's step
	void OnRunFound(const std::vector<Piece*>& thePieces) override;                    // slot 211 (0x6c56a0): the goal's slot 0x44
	void NewGame() override;                                                           // slot 135 (0x6ddc70): the quest's settings, the goal
	void OnPieceCleared(Piece* thePiece) override;                                     // slot 143: the goal's slot 40 (0xa0)
	bool OnPieceRemoving(Piece* thePiece) override;                                    // slots 176 and 142: the goal's slots 18 and 39
	void OnLevelFull() override;                                                       // slot 156 (0x6ca7e0, which runs 0x6ca350 when the secret flag is clear)
	void NoMoreMoves(bool theAnnounce) override;                                       // slot 161 (0x6d8e30)
	void AddPoints(int thePoints, int theMoveId = -1, bool theCountsForProgress = true, bool theUsesMultiplier = true) override;   // slot 152 (0x6cf290)
	void LeaveToMenu() override;                                                       // slot 232 (0x6d8e20, a jump to Board's 0x756730)
	~QuestBoard() override;

	// 0x6ddc70, the goal's part: the setting Goal (upper case) is looked up (MakeQuestGoal) and the goal that is there replaces the one before
	void CreateGoal();
	// the text of a setting with its quotes taken off and in upper case (the exe's compares, 0x403e30, ignore the case); empty when it is not there
	std::string SettingText(const char* theName) const;
	// 0x6dd8a0: the clock of a finished level counts down (see Update)
	void StepQuestClock();
	// the quest's end: the quest is marked as done in the profile, which is saved (the exe's QuestMenu does it, 0x684380, when the countdown that 0x67ed00 starts has run out)
	void MarkQuestDone();

	// the settings of the quest (+0xc8, copied from secret.cfg by 0x774060 before the game is reset); a missing one reads as 0 (the exe's
	// atoi and wtof of the empty text)
	int SettingInt(const char* theName) const;
	double SettingDouble(const char* theName) const;
	// a setting that is a list of numbers ("2500, 5000, ..."): the exe parses it with commas (0x6dfa40 for Poker's HandValues, HandBuster and HandCountdown)
	std::vector<double> SettingList(const char* theName) const;
	bool HasSetting(const char* theName) const { return mSettings.find(theName) != mSettings.end(); }

};

}  // namespace Sexy
