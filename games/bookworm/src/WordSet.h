#ifndef __BOOKWORM_WORDSET_H__
#define __BOOKWORM_WORDSET_H__

// port: the dictionary (wordlist.txt, 151,381 words, and the mods' words), held in two flat arrays where the original
// holds a std::set<SexyString>: the letters of every word one after another, each followed by a zero byte, and
// where each word starts, in the set's order (byte by byte, as the set orders its strings). A set node of a word
// costs 48 bytes and more beside the word (more in 64 bits, and with the heap's own); here a word costs its letters, one
// byte and four for its place: about 2 MB where the set took 7 to 12. Lookups are a binary search over the places,
// as the set's are a walk down its tree, and give the same answers.
//
// Add words with Insert, then Settle once before the first lookup (it sorts and drops repeated words; LoadWordList
// and LoadModWords do). Not thread safe: the loading thread fills it, the game thread asks.

#include "Common.h"
#include <cstdint>
#include <vector>

namespace Sexy
{

class WordSet
{
public:
	enum LookupResult
	{
		LOOKUP_NONE = 0,    // no word starts with the letters
		LOOKUP_PREFIX,      // some word starts with them, and they are not a word
		LOOKUP_WORD         // they are a word
	};

	void			Insert(const SexyString& theWord);
	void			Insert(const std::vector<SexyString>& theWords);
	void			Settle();                                       // sort, drop repeats, give back spare room

	bool			Contains(const SexyString& theWord) const;      // std::set::find(...) != end()
	LookupResult	Lookup(const SexyString& theLetters) const;     // the first word not before theLetters: does it start with them, is it them
	size_t			size() const { return mPlaces.size(); }
	bool			empty() const { return mPlaces.empty(); }

private:
	size_t			FirstNotBefore(const SexyString& theLetters) const;  // std::set::lower_bound: a place's index, or size()
	const char*		WordAt(size_t theIdx) const { return &mLetters[mPlaces[theIdx]]; }

	std::vector<char>		mLetters;   // each word and the zero byte after it
	std::vector<uint32_t>	mPlaces;    // where each word starts in mLetters, the words in order once settled
};

}

#endif
