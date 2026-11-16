// port: the dictionary's flat arrays (WordSet.h)

#include "WordSet.h"
#include <algorithm>
#include <cstring>

using namespace Sexy;

void WordSet::Insert(const SexyString& theWord)
{
	// a word has no zero byte (LoadWordList reads text); one with it would end early in mLetters
	mPlaces.push_back((uint32_t) mLetters.size());
	mLetters.insert(mLetters.end(), theWord.c_str(), theWord.c_str() + theWord.size() + 1);
}

void WordSet::Insert(const std::vector<SexyString>& theWords)
{
	for (size_t i = 0; i < theWords.size(); i++)
		Insert(theWords[i]);
}

void WordSet::Settle()
{
	const char* aLetters = mLetters.data();
	// strcmp orders by unsigned byte, as the strings of a std::set are
	std::sort(mPlaces.begin(), mPlaces.end(), [aLetters](uint32_t theLeft, uint32_t theRight)
	{
		return strcmp(aLetters + theLeft, aLetters + theRight) < 0;
	});
	mPlaces.erase(std::unique(mPlaces.begin(), mPlaces.end(), [aLetters](uint32_t theLeft, uint32_t theRight)
	{
		return strcmp(aLetters + theLeft, aLetters + theRight) == 0;
	}), mPlaces.end());
	mPlaces.shrink_to_fit();

	// the letters of the dropped repeats go too (a mod's list repeats few words: the game's own are not added again),
	// so that the words follow each other in order and a lookup walks the memory forward
	std::vector<char> aSettled;
	aSettled.reserve(mLetters.size());
	for (size_t i = 0; i < mPlaces.size(); i++)
	{
		const char* aWord = aLetters + mPlaces[i];
		mPlaces[i] = (uint32_t) aSettled.size();
		aSettled.insert(aSettled.end(), aWord, aWord + strlen(aWord) + 1);
	}
	aSettled.shrink_to_fit();
	mLetters.swap(aSettled);
}

size_t WordSet::FirstNotBefore(const SexyString& theLetters) const
{
	const char* aLetters = theLetters.c_str();
	size_t aLow = 0, aHigh = mPlaces.size();
	while (aLow < aHigh)
	{
		size_t aMid = aLow + (aHigh - aLow) / 2;
		if (strcmp(WordAt(aMid), aLetters) < 0)
			aLow = aMid + 1;
		else
			aHigh = aMid;
	}
	return aLow;
}

bool WordSet::Contains(const SexyString& theWord) const
{
	return Lookup(theWord) == LOOKUP_WORD;
}

WordSet::LookupResult WordSet::Lookup(const SexyString& theLetters) const
{
	size_t aIdx = FirstNotBefore(theLetters);
	if (aIdx == mPlaces.size())
		return LOOKUP_NONE;
	const char* aWord = WordAt(aIdx);
	if (strncmp(aWord, theLetters.c_str(), theLetters.size()) != 0)
		return LOOKUP_NONE;
	return (aWord[theLetters.size()] == 0) ? LOOKUP_WORD : LOOKUP_PREFIX;
}
