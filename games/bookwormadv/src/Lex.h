// The game's word list (no RTTI; 0x28 bytes, made by GameApp::LoadingThreadProc 0x4473e0 as the app's mLex, +0x940):
// the words of data/compressed.txt, upper-cased, as a set (constructor 0x4bc020, Load 0x4bb250). The board asks
// it whether a word is one (0x41a4d0).
#pragma once

#include <set>
#include <string>
#include <vector>

namespace Sexy {

class Lex {
public:
	explicit Lex(bool theFlag) : mFlag(theFlag) {}

	// Reads the word list (theDefsFile, data/defs.bin, is not read: the game has none). Words of 3 to 16
	// letters are kept, and also added to theWords when given. False when the list cannot be opened.
	bool Load(const std::string& theDefsFile, const std::string& theWordFile, std::vector<std::string>* theWords = nullptr);

	// A word in capitals (0x4bc0b0 is the set's find)
	bool IsWord(const std::string& theWord) const { return mWords.count(theWord) != 0; }
	size_t GetNumWords() const { return mWords.size(); }

	bool mFlag;                     // +0x00, the constructor's argument
	std::set<std::string> mWords;   // +0x04
};

}  // namespace Sexy
