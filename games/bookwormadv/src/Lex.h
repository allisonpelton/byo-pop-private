// The game's word list (no RTTI; 0x28 bytes, made by GameApp::LoadingThreadProc 0x4473e0 as the app's mLex, +0x940):
// the words of data/compressed.txt, upper-cased, as a set (constructor 0x4bc020, Load 0x4bb250). The board asks
// it whether a word is one (0x41a4d0).
//
// port: the original's std::set<std::string> held 167,624 words at 40 to 64 bytes each (7.3 MB of the port's heap,
// more in the original's); here the letters of every word lie one after another in one array, each with a zero byte
// after it, and a second array holds where each word starts, in the set's order (byte by byte, as std::string compares).
// About 2.3 MB. IsWord is a binary search over the places, as the set's find walked its tree, with the same answers.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy {

class Lex {
public:
	// 0x4bc020
	explicit Lex(bool theFlag) : mFlag(theFlag) {}

	// Reads the word list (theDefsFile, data/defs.bin, is not read: the game has none). Words of 3 to 16
	// letters are kept, and also added to theWords when given. False when the list cannot be opened.
	bool Load(const std::string& theDefsFile, const std::string& theWordFile, std::vector<std::string>* theWords = nullptr);

	// A word in capitals (0x4bc0b0 is the set's find)
	bool IsWord(const std::string& theWord) const;
	size_t GetNumWords() const { return mPlaces.size(); }
	// The words in the set's order (the test hooks walk them as the set's iterator did)
	const char* GetWord(size_t theIndex) const { return &mLetters[mPlaces[theIndex]]; }

	bool mFlag;                     // +0x00, the constructor's argument

private:
	std::vector<char> mLetters;     // every word and a zero byte after it
	std::vector<uint32_t> mPlaces;  // where each word starts in mLetters, sorted and without repeats once loaded
};

}  // namespace Sexy
