// Sexy::ArenaWinDialog (0x214 bytes, constructor 0x401180, vtables 0x7a976c and 0x7a98c0 (its ButtonListener part at
// +0x88); code 0x401180-0x401a12, destructor 0x401370; the app's makers are 0x4519e0 and 0x451a80): what the arena
// says when the player has beaten it. A GameDialog (dialog id 0x15, GameApp::DIALOG_ARENA_WIN) under the arena's
// banner with a "Continue" button of the footer: the first time (the scripts' mFirstTimeBeatingArena) it tells that
// the film reel, the media player of the cut scenes and songs, is unlocked; after that it lists how the run went
// (the time to clear, the potions left, the words spelled, the final level and the average word length), which the
// scripts give it with SetArenaWinStats once it is up.
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
	std::string mCallback;          // +0x1f8: the scripts' function the app was given (nothing reads it)

	// theCallback: the scripts' function that DoArenaWinDialog was given
	ArenaWinDialog(bool theFirstTime, const std::string& theCallback);  // 0x401180

	// 0x4431a0 (not virtual): the five lines of the list
	void SetStats(const std::string& theTime, const std::string& thePotions, const std::string& theWords,
	              const std::string& theLevel, const std::string& theAverageWordLength);

	// Widget
	void Draw(Graphics* g) override;  // 0x4014c0
};

}  // namespace Sexy
