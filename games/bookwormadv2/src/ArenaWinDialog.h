// Sexy::ArenaWinDialog (0x214 bytes, constructor 0x401060, vtables 0x6f6b8c and 0x6f6ce0 (its ButtonListener part at
// +0x88)): what the arena says when the player has beaten it. A GameDialog (dialog id 0x15, GameApp::DIALOG_ARENA_WIN)
// under the arena's banner with a "Continue" button of the footer. It lists how the run went (the time to clear, the
// potions left, the words spelled, the final level and the average word length), which the scripts give it with
// SetArenaWinStats once it is up. Continue calls the scripts' function the app was given and ends the dialog.
#pragma once

#include <string>

#include "GameDialog.h"

namespace Sexy {

class ArenaWinDialog : public GameDialog {
public:
	bool mFirstTime;                // +0x168: the first win (the dialog is 20 pixels higher, for the unlock's text)
	std::string mTime;              // +0x16c: the time to clear
	std::string mPotions;           // +0x188: the potions left
	std::string mWords;             // +0x1a4: the words spelled
	std::string mLevel;             // +0x1c0: the final level
	std::string mAverageWordLength; // +0x1dc
	std::string mCallback;          // +0x1f8: the scripts' function the app was given (Continue calls it)

	// theCallback: the scripts' function that DoArenaWinDialog was given
	ArenaWinDialog(bool theFirstTime, const std::string& theCallback);  // 0x401060

	// 0x438020 (not virtual): the five lines of the list
	void SetStats(const std::string& theTime, const std::string& thePotions, const std::string& theWords,
	              const std::string& theLevel, const std::string& theAverageWordLength);

	// ButtonListener (the part at +0x88; 0x401870)
	void ButtonDepress(int theId) override;

	// Widget
	void Draw(Graphics* g) override;  // 0x4013a0
};

}  // namespace Sexy
