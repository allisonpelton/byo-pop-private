#ifndef __BONNIES_DICTIONARY_H__
#define __BONNIES_DICTIONARY_H__

// The word list (0x404450-0x405d60, 0x84 bytes, the object 0x475e78), read from Data/Dictionary.txt, and the odds of
// the letters that fall onto the board (the table 0x466470, which the dictionary's file can change).

#include "Str.h"

namespace bonnies
{

// A letter's odds (12 bytes): the table 0x466470 holds 37, A to Z and then the special tiles
struct LetterOdds
{
	int mCount;               // +0x00: its weight
	int mCumulative;          // +0x04: the weights up to and including it
	char mChar;               // +0x08
};

extern LetterOdds gLetterOdds[37];  // 0x466470
extern float gVowelShare;           // 0x46662c: the vowels' share of A to Z's weights
extern int gLetterWeights;          // 0x475e7c: the weights of A to Z
extern int gAllWeights;             // 0x475e80: the weights of all 37
extern int gLetterTotal;            // 0x475e84: where A to Z's cumulative weights end
extern int gAllTotal;               // 0x475e88: and all of them

// Reads the table's first contents from the exe
void loadLetterOdds();

class Dictionary
{
public:
	// loadResources (0x402c20): new, the Str's ctor
	Dictionary();
	// 0x404510
	~Dictionary();

	// 0x4054f0: reads Data/Dictionary.txt
	void init();
	// 0x404880: whether theWord is a word (its tiles expanded first when theExpand), its spelling and rarity in
	// mMatch and mMatchRarity; '?' and '!' stand for any letter
	int isWord(Str theWord, int theExpand);
	// 0x404c50: the same for a word of letters and at most the wild tiles' first
	int lookup(Str theWord);
	// port (the memory report): the bytes of the three tables, the strings' own storage included
	size_t memoryBytes() const;

	int mCount;               // +0x00
	Str* mWords;              // +0x04: upper case, accents removed, in order
	Str* mSpellings;          // +0x08: as they are shown
	uint8_t* mRarities;       // +0x0c (1 unless the file says)
	int mStarts[26];          // +0x10: the first word of each letter
	Str mMatch;               // +0x78
	int mMatchRarity;         // +0x80
};

// 0x404570: theWord with its two-letter tiles spelled out (and its wild tiles as '*' when theWild)
Str expandWord(Str theWord, int theWild);
// 0x405050: upper case without accents, in place
char* normalizeWord(char* theWord);
// 0x404450: the odds from the letters' weights
void resetLetterOdds();
// 0x4050e0: the odds changed by the letters on the board (fewer of those it has, vowels by how many it lacks)
void updateLetterOdds();
// 0x405390: a letter for column theColumn, from A to Z or (theAll) all the tiles
char randomLetter(int theColumn, int theAll);

} // namespace bonnies

#endif
