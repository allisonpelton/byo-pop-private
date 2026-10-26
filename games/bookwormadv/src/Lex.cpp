#include "Lex.h"

#include <cctype>
#include <cstring>

#include "PakInterface.h"
#include "SexyAppBase.h"

using namespace Sexy;

// One word per line, prefix-compressed: a number is how many leading letters of the previous word the next
// words keep, until the next number ("aa", "2h", "3ed", "ing" -> AA, AAH, AAHED, AAHING). As in BookWorm Deluxe.
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
		char c = (char)aChar;
		if (c >= '0' && c <= '9') {
			if (!inNumber) {
				aPrefixLength = c - '0';
				inNumber = true;
			} else {
				aPrefixLength = aPrefixLength * 10 + (c - '0');
			}
		} else if (c == '\r' || c == '\n') {
			if (aLength > 0) {
				memcpy(aPrevWord, aWord, aLength);
				if (aLength >= 3 && aLength <= 16) {
					std::string aString(aWord, aLength);
					for (char& ch : aString)
						ch = (char)toupper((unsigned char)ch);
					mWords.insert(aString);
					if (theWords != nullptr)
						theWords->push_back(aString);
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
				aWord[aLength++] = c;
		}
	}
	p_fclose(aFile);
	return true;
}
