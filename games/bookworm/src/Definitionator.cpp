#include "Definitionator.h"
#include "PakInterface.h"

using namespace Sexy;

// 0x41dd30
Definitionator::Definitionator(BookWormApp* theApp)
{
	mApp = theApp;
}

// 0x41d8d0
Definitionator::~Definitionator()
{
}

// 0x41d920
// One definition per line: WORD, then the first space or tab, then the definition, trimmed of spaces, tabs,
// CR and LF on both sides. Lines without a separator, with an empty word or definition, or with the
// definition "*" (= none) are skipped; a later duplicate replaces an earlier one. No case change.
// The original reads it with an ifstream in text mode (CR LF -> LF, Ctrl-Z ends the file) and
// getline(buffer, 4096, '\n'): a final line without a newline counts. Returns true if any definition was
// read (the app ignores the result).
bool Definitionator::Load(const SexyString& theFileName)
{
	// port: through the framework's file layer (game folder, case-insensitive), "r" = the same text mode
	PFILE* aFile = p_fopen(theFileName.c_str(), "r");
	if (aFile == NULL)
		return false;

	const char* aTrimChars = " \t\r\n";
	bool atEnd = false;
	while (!atEnd)
	{
		// one getline: up to the '\n' (consumed), or to the end of the file (failing when nothing was read)
		std::string aBuffer;
		bool gotNewline = false;
		for (;;)
		{
			int aChar = p_fgetc(aFile);
			if (aChar == EOF)
			{
				atEnd = true;
				break;
			}
			if (aChar == '\n')
			{
				gotNewline = true;
				break;
			}
			aBuffer += (char)aChar;
		}
		if (atEnd && !gotNewline && aBuffer.empty())
			break;
		// port: the original's line buffer holds 4095 characters (a longer line makes its loop never end);
		// popdefs.txt's longest line is 326, so lines are read whole here.

		std::string aLine(aBuffer.c_str()); // the original builds the string with strlen
		if (aLine.length() != 0)
		{
			std::string::size_type aSepPos = aLine.find_first_of(" \t");
			if (aSepPos != std::string::npos)
			{
				std::string aWord = aLine.substr(0, aSepPos);
				std::string aDefinition = aLine.substr(aSepPos, aLine.length());
				aDefinition.erase(aDefinition.find_last_not_of(aTrimChars) + 1); // npos + 1 = 0: all trimmed
				aDefinition.erase(0, aDefinition.find_first_not_of(aTrimChars));
				if ((aWord.length() != 0) && (aDefinition.length() != 0) && (aDefinition.compare(0, aDefinition.length(), "*") != 0))
					mDefinitions[aWord] = aDefinition;
			}
		}
	}
	p_fclose(aFile);

	// port: the original's OutputDebugStringA("Time to load %d definitions (%s): %d\n") is dropped
	return mDefinitions.size() != 0;
}

// 0x41d2b0
// A copy of the definition, or "" when the word has none (exact, case-sensitive lookup).
SexyString Definitionator::GetDefinition(const SexyString& theWord)
{
	SexyString aDefinition;
	std::map<std::string, std::string>::iterator anItr = mDefinitions.find(theWord);
	if (anItr != mDefinitions.end())
		aDefinition = anItr->second;
	return aDefinition;
}
