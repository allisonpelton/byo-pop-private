// The game's word list (no RTTI; 0x28 bytes, made by GameApp::LoadingThreadProc as the app's mLex, +0x97c): the words of
// data/compressed.txt, upper-cased, in a hash map of the word and an int (constructor 0x49e2c0, Load 0x49d650, the
// lookup 0x49dad0 over the map's find 0x49e340; the port keeps a std::map). The int comes from
// data/compressed_attribs.dat and says what the word can be in a sentence: bit 1 an adjective, 2 a verb, 4 a noun.
// The board asks it whether a word is one (the lookup's callers) and tells the tile engine's script what the word is
// (0x486fa0); the scripts ask the board (IsNoun, IsVerb and IsAdjective, 0x41a1a0 and its neighbours).
#pragma once

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class Lex {
public:
	enum {
		FLAG_ADJECTIVE = 1,
		FLAG_VERB = 2,
		FLAG_NOUN = 4,
	};

	explicit Lex(bool theFlag) : mFlag(theFlag) {}

	// Reads the word list (theWordFile, data/compressed.txt) and the flags of its words (theAttribsFile,
	// data/compressed_attribs.dat: four bytes for each word of 3 to 16 letters, in the order of the list). Those words
	// are kept, and also added to theWords when given. False when a file cannot be opened or the flags run out (the
	// original gives up the same way and returns whatever its register held).
	bool Load(const std::string& theAttribsFile, const std::string& theWordFile, std::vector<std::string>* theWords = nullptr);

	// A word in capitals (0x49dad0): its flags, or 0 when it is not in the list (theFound says which)
	int GetFlags(const std::string& theWord, bool* theFound = nullptr) const
	{
		std::map<std::string, int>::const_iterator anItr = mWords.find(theWord);
		if (theFound != nullptr)
			*theFound = anItr != mWords.end();
		return anItr != mWords.end() ? anItr->second : 0;
	}
	bool IsWord(const std::string& theWord) const { return mWords.count(theWord) != 0; }
	size_t GetNumWords() const { return mWords.size(); }

	bool mFlag;                          // +0x00, the constructor's argument
	std::map<std::string, int> mWords;   // +0x04: the word and its flags
};

}  // namespace Sexy
