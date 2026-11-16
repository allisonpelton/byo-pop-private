#include "Lex.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "PakInterface.h"
#include "SexyAppBase.h"

using namespace Sexy;

// One word per line, prefix-compressed: a number is how many leading letters of the previous word the next
// words keep, until the next number ("aa", "2h", "3ed", "ing" -> AA, AAH, AAHED, AAHING). As in BookWorm Deluxe.
// 0x4bb250, with 0x4baf70 (the next character of the file) and 0x4bb070 (the file opened through the PakInterface):
// p_fgetc and p_fopen here
bool Lex::Load(const std::string& theDefsFile, const std::string& theWordFile, std::vector<std::string>* theWords)
{
	PFILE* aFile = p_fopen(theWordFile.c_str(), "r");
	if (aFile == nullptr)
		return false;

	enum { BUFFER_SIZE = 256 };  // the original's two buffers (it does not check their size)
	char aWord[BUFFER_SIZE];
	char aPrevWord[BUFFER_SIZE];
	memset(aWord, 0, sizeof(aWord));
	memset(aPrevWord, 0, sizeof(aPrevWord));
	int aLength = 0;
	int aPrefixLength = 0;
	bool inNumber = false;
	for (;;) {
		int aChar = p_fgetc(aFile);
		if (aChar == EOF)
			break;
		if (gSexyAppBase != nullptr && gSexyAppBase->mShutdown)
			break;
		char aLetter = (char)aChar;
		if (aLetter >= '0' && aLetter <= '9') {
			if (!inNumber) {
				aPrefixLength = aLetter - '0';
				inNumber = true;
			} else {
				aPrefixLength = aPrefixLength * 10 + (aLetter - '0');
			}
		} else if (aLetter == '\r' || aLetter == '\n') {
			if (aLength > 0) {
				memcpy(aPrevWord, aWord, aLength);
				if (aLength >= 3 && aLength <= 16) {
					// port: the word goes to the end of the letters (the set inserted it in its place; the places are
					// sorted once the file is read)
					mPlaces.push_back((uint32_t)mLetters.size());
					for (int i = 0; i < aLength; i++)
						mLetters.push_back((char)toupper((unsigned char)aWord[i]));
					mLetters.push_back('\0');
					if (theWords != nullptr)
						theWords->push_back(&mLetters[mPlaces.back()]);
				}
				aLength = 0;
			}
			inNumber = false;
		} else {
			if (aPrefixLength > 0 && aLength == 0) {
				aLength = aPrefixLength < BUFFER_SIZE - 1 ? aPrefixLength : BUFFER_SIZE - 1;
				memcpy(aWord, aPrevWord, aLength);
			}
			if (aLength < BUFFER_SIZE - 1)
				aWord[aLength++] = aLetter;
		}
	}
	p_fclose(aFile);

	// port: the places in the set's order, each word once (the set kept the first of equal words; they are the same
	// letters, so which place stays does not matter)
	const char* aLetters = mLetters.data();
	std::sort(mPlaces.begin(), mPlaces.end(),
	          [aLetters](uint32_t a, uint32_t b) { return strcmp(aLetters + a, aLetters + b) < 0; });
	mPlaces.erase(std::unique(mPlaces.begin(), mPlaces.end(),
	                          [aLetters](uint32_t a, uint32_t b) { return strcmp(aLetters + a, aLetters + b) == 0; }),
	              mPlaces.end());
	mLetters.shrink_to_fit();
	mPlaces.shrink_to_fit();
	return true;
}

// port: the set's find, as a binary search over the sorted places
bool Lex::IsWord(const std::string& theWord) const
{
	const char* aLetters = mLetters.data();
	const char* aWord = theWord.c_str();
	std::vector<uint32_t>::const_iterator anItr = std::lower_bound(mPlaces.begin(), mPlaces.end(), aWord,
		[aLetters](uint32_t thePlace, const char* theKey) { return strcmp(aLetters + thePlace, theKey) < 0; });
	return anItr != mPlaces.end() && strcmp(aLetters + *anItr, aWord) == 0 && theWord.size() == strlen(aLetters + *anItr);
}
