// Sexy::QuestBoard (bejeweled3, vftable 0x866fbc, 266 slots, constructor 0x6d9000 over Sexy::Board): what the Quest-derived boards share. The exe
// keeps a goal object at +0x3180 that most of its slots delegate to; the secret modes (Poker and Butterflies, started by 0x774060 with
// the secret flag at +0x3184 set) have none or only a counter, and with the flag set the board is an endless game: it has no level bar and
// no levels (slot 217, 0x6c5460, is false; slot 154, the level's size, is 0; slot 156, the level's end, does nothing). The port keeps what
// ButterflyBoard and PokerBoard read: the flag, the settings from secret.cfg, the background of the quest and the fill that stops once the
// game has ended. See docs/NOTES.md, "Butterflies and Poker".
#pragma once

#include "Board.h"

#include <vector>

namespace Sexy {

class QuestBoard : public Board {
public:
	bool mIsSecretMode = false;   // +0x3184: set by 0x774060's fourth argument, which the play menu gives for Poker and Butterflies
	// +0x3198 (0x6dd8a0): the updates this game has run: it goes up by 1 at the end of each update of QuestBoard::Update that is not paused (0x6cda70 and
	// 0x6ddc70 put it to 0). Read by Ice Storm's slot 22 (0x6e63c0) and 0x6e5850 (against the ice's own tick marks), 0x6e0450 (which keeps it as the
	// time the death began, +0x3cb8) and by 0x6d6d60 and 0x6db550 (the update a column or a chain was made in, +0x18, +0x218 and +0xdb0)
	int mUpdatesPlayed = 0;
	// +0x31a0 (0x6dd8a0): the updates this game has run, counted before the quest's clock; 0x6cda70 puts it to 0 and no function of the export reads it
	int mUpdatesRun = 0;

	bool HasLevels() const override { return !mIsSecretMode; }                       // slot 217 (0x6c5460, false in the secret modes)
	int GetLevelMax() const override { return mIsSecretMode ? 0 : Board::GetLevelMax(); }   // slot 154: 0 in the secret modes
	bool ShowsLevelNumber() const override { return !mIsSecretMode; }               // slot 249: a secret mode has no level to show
	bool IsFillStopped() const override { return mGameOverTicks != 0; }               // slot 203 (0x6dd4d0): no fill once the end has begun (+0xd1c)
	float GetLevelFactor() const override { return 1.0f; }                            // slot 122 (0x73b600): the quest boards score at 1 (Butterflies, 0x6c5340, gives 5 in a secret mode)
	void ChooseBackground() override;                                                  // slot 100: the setting BackgroundIdx (0x74e410)
	void ResetGame() override;                                                         // slot 85 (0x6cda70): Board's, then the two update counters to 0
	void Update() override;                                                            // slot 22 (0x6dd8a0): Board's update, then the counters (see QuestBoard.cpp)
	void UpdateTick() override;                                                        // slot 234 (0x6e5660): the goal's slot 0x2c, then Board's step
	void OnRunFound(const std::vector<Piece*>& thePieces) override;                    // slot 211 (0x6c56a0): the goal's slot 0x44

	// the settings of the quest (+0xc8, copied from secret.cfg by 0x774060 before the game is reset); a missing one reads as 0 (the exe's
	// atoi and wtof of the empty text)
	int SettingInt(const char* theName) const;
	double SettingDouble(const char* theName) const;
	// a setting that is a list of numbers ("2500, 5000, ..."): the exe parses it with commas (0x6dfa40 for Poker's HandValues, HandBuster and HandCountdown)
	std::vector<double> SettingList(const char* theName) const;
	bool HasSetting(const char* theName) const { return mSettings.find(theName) != mSettings.end(); }

};

}  // namespace Sexy
