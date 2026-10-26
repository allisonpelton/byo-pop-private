#ifndef __WORDHARMONY_DICTIONARY_H__
#define __WORDHARMONY_DICTIONARY_H__

// The word list and the anagram finder (the controller's +0x10; 0x41e820-0x420b40). Not ported yet: only what the
// ball system calls is declared.

#include <string>
#include <vector>

class Dictionary
{
public:
	// 0x4205b0 (not read yet): the words theLetters and theChains can make, into theWords (the hints)
	bool findWords(const std::string& theLetters, const std::vector<std::string>& theChains,
	               std::vector<std::string>* theWords);
	// 0x420720 (not read yet): the same for solitaire and freematch, into a list the caller keeps only on success
	bool findAnyWords(const std::string& theLetters, const std::vector<std::string>& theChains,
	                  std::vector<std::string>& theWords);
};

#endif
