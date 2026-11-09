// WinWSAppLoad.cpp: the data-file loaders of WinWSApp that LoadingThreadProc (0x435560) runs after the level config: the word
// lists of data\waves.txt (0x434740), the long words of data\twords.txt (0x430d20), the gem names (0x42b0c0) and ship names
// (0x42b400) and the themes of data\themes.txt (0x434d60). Each returns false when its file does not open (LoadThemes also
// when the file is malformed). The level config is read by LessonManager::LoadConfig.
//
// The original reads the five files with the old iostream: an ifstream in text mode and getline(buffer, 20, '\n') (40 in the
// themes), which takes at most 19 (39) characters of a line, so a longer line comes back in pieces. Its loops end on the
// stream's end-of-file flag alone, and the last read of a file that ends with a newline gives an empty line. GetLine below
// does the same on the framework's text-mode PFILE (CR LF pairs become LF there as in the CRT).
//
// Not translated: the string every loader appends each line to and never reads again; the standard-library instances (the
// vector of vectors's resize 0x431560, 0x42f820, the map's operator[] 0x42f930, the strings' and vectors' copies
// 0x4150e0, 0x429600, 0x406cb0, 0x414630); the iostream and its locking (0x508306, 0x5083bb, 0x5084a1 ...); and the exception
// funclets of these functions.
// Deviations (port:): none that show. Each "#DIFF-", "#LENGTH-" and "#THEME" block adds one to the loading task count and one
// to the finished count and redraws the loading screen, as the original does (the bar fills more as the file goes on).

#include "WinWSApp.h"
#include "GameText.h"
#include "TitleScreen.h"
#include "PakInterface.h"
#include "Common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

using namespace Sexy;

// The stream state the loops look at (ios::eofbit and ios::failbit) of the original's ifstream, and the file it reads
struct LoadStream
{
	PFILE*	mFile;
	bool	mEOF;
	bool	mFail;

	LoadStream(PFILE* theFile) : mFile(theFile), mEOF(false), mFail(false) {}
};

// 0x5084a1: istream::get(buffer, size, '\n') with the getline flag. Reads up to size - 1 characters of the line, takes the
// newline out of the file without storing it, and ends the buffer with a 0. At the end of the file it sets eofbit (and failbit
// when it read nothing).
static void GetLine(LoadStream& theStream, char* theBuffer, int theSize)
{
	unsigned int aCount = 0;
	if (theStream.mEOF || theStream.mFail)
	{
		theStream.mFail = true;     // ipfx: the stream is not good
	}
	else if (theSize != 0)
	{
		if (theSize != 1)
		{
			do
			{
				int aChar = p_fgetc(theStream.mFile);
				if (aChar == EOF)
				{
					theStream.mEOF = true;
					if (aCount == 0)
						theStream.mFail = true;
					break;
				}
				if (aChar == '\n')
					break;
				theBuffer[aCount] = (char) aChar;
				aCount++;
			} while (aCount < (unsigned int) theSize - 1U);
		}
	}
	theBuffer[aCount] = 0;
}

// 0x416620 (the part that matters): the letters a to z become capitals
static void MakeUpperCase(std::string& theString)
{
	for (size_t i = 0; i < theString.length(); i++)
	{
		if ((theString[i] > '`') && (theString[i] < '{'))
			theString[i] = (char) (theString[i] - 0x20);
	}
}

// 0x416840: takes the spaces, line feeds, carriage returns and tabs off both ends
static void TrimWhiteSpace(std::string& theString)
{
	unsigned int aSize = (unsigned int) theString.length();
	unsigned int aFirst = 0;
	if (aSize != 0)
	{
		do
		{
			char aChar = theString[aFirst];
			if ((aChar != ' ') && (aChar != '\n') && (aChar != '\r') && (aChar != '\t'))
				break;
			aFirst++;
		} while (aFirst < aSize);
	}

	int aLast = (int) aSize;
	do
	{
		aLast--;
		if (aLast < 0)
			break;
	} while ((theString[aLast] == ' ') || (theString[aLast] == '\n') || (theString[aLast] == '\r') || (theString[aLast] == '\t'));

	theString = theString.substr(aFirst, (unsigned int) ((aLast - (int) aFirst) + 1));
}

// 0x42b0c0: the gem names of data\gemnames.txt, every line of more than 3 characters
bool WinWSApp::LoadGemNames()
{
	PFILE* aFile = p_fopen(GTC(0x0052c9f8), "r");       // data\gemnames.txt
	if (aFile == NULL)
		return false;

	LoadStream aStream(aFile);
	char aBuffer[20];
	while (!aStream.mEOF)
	{
		GetLine(aStream, aBuffer, 20);
		std::string aLine = aBuffer;
		if (aLine.length() > 3)
			mGemNames.push_back(aLine);
	}

	p_fclose(aFile);
	return true;
}

// 0x42b400: the ship names of data\shipnames.txt, the same way
bool WinWSApp::LoadShipNames()
{
	PFILE* aFile = p_fopen(GTC(0x0052ca0c), "r");       // data\shipnames.txt
	if (aFile == NULL)
		return false;

	LoadStream aStream(aFile);
	char aBuffer[20];
	while (!aStream.mEOF)
	{
		GetLine(aStream, aBuffer, 20);
		std::string aLine = aBuffer;
		if (aLine.length() > 3)
			mShipNames.push_back(aLine);
	}

	p_fclose(aFile);
	return true;
}

// 0x430d20: the long words of data\twords.txt. A line with "#DIFF-" starts a list: the number after it (up to five
// characters) is the key (the word length that GetLongWord asks for); the lines up to the one with "#END" are its words, in
// capitals ("#END" itself is not kept).
bool WinWSApp::LoadWordLists()
{
	PFILE* aFile = p_fopen(GTC(0x0052cd84), "r");       // data\twords.txt
	if (aFile == NULL)
		return false;

	LoadStream aStream(aFile);
	char aBuffer[20];
	std::string aLine;
	while (!aStream.mEOF)
	{
		GetLine(aStream, aBuffer, 20);
		aLine = aBuffer;
		if (aLine.find(GT(0x0052cd7c)) != std::string::npos)        // "#DIFF-"
		{
			mCompletedLoadingThreadTasks++;
			mNumLoadingThreadTasks++;
			mTitleScreen->MarkDirty();

			int aLength = (int) atol(aLine.substr(6, 5).c_str());
			while (!aStream.mEOF)
			{
				if (aLine.find(GT(0x0052cd74)) != std::string::npos)    // "#END"
					break;

				GetLine(aStream, aBuffer, 20);
				aLine = aBuffer;
				MakeUpperCase(aLine);
				if (aLine.compare(GT(0x0052cd74)) != 0)
					mLongWords[aLength].push_back(aLine);
			}
		}
	}

	p_fclose(aFile);
	return true;
}

// 0x434740: the word lists of data\waves.txt. A line with "#LENGTH-" starts a list: the number after it (up to five characters)
// is the word length, 3 to 7; the lines up to the one with "#END" are the words (as they are in the file) and go into
// mWordLists[length - 3]. A length outside 3 to 7 is reported with a message that asks the player to post it, and the line is
// dropped.
bool WinWSApp::LoadWaves()
{
	PFILE* aFile = p_fopen(GTC(0x0052d3b8), "r");       // data\waves.txt
	if (aFile == NULL)
		return false;

	LoadStream aStream(aFile);
	mWordLists.resize(5);       // 0x431560

	char aBuffer[20];
	std::string aLine;
	while (!aStream.mEOF)
	{
		GetLine(aStream, aBuffer, 20);
		aLine = aBuffer;
		if (aLine.find(GT(0x0052d3ac)) != std::string::npos)        // "#LENGTH-"
		{
			mCompletedLoadingThreadTasks++;
			mNumLoadingThreadTasks++;
			mTitleScreen->MarkDirty();

			std::string aDigits = aLine.substr(8, 5);
			int aWordLength = (int) atol(aDigits.c_str());
			while (!aStream.mEOF)
			{
				if (aLine.find(GT(0x0052cd74)) != std::string::npos)    // "#END"
					break;

				GetLine(aStream, aBuffer, 20);
				aLine = aBuffer;
				if (aLine.compare(GT(0x0052cd74)) != 0)
				{
					int aListIndex = aWordLength - 3;
					if ((aListIndex < 0) || (mWordLists.size() == 0) || ((unsigned int) mWordLists.size() <= (unsigned int) aListIndex))
						Popup(StrFormat(GTC(0x0052d368), aWordLength, aLine.c_str(), aDigits.c_str()));
					else
						mWordLists[aListIndex].push_back(aLine);
				}
			}
		}
	}

	p_fclose(aFile);
	return true;
}

// 0x434d60: the themes of data\themes.txt. A line that starts with "#THEME" begins a theme (a new list in mThemes); the lines up
// to the one that starts with "#END" are its words, in capitals, trimmed, and kept when under 8 characters. Any other line is
// skipped, unless it starts with "#": that is an error. A theme without its "#END" and an unknown "#" line are reported with
// a message and the load fails.
bool WinWSApp::LoadThemes()
{
	PFILE* aFile = p_fopen(GTC(0x0052d430), "r");       // data\themes.txt
	if (aFile == NULL)
		return false;

	LoadStream aStream(aFile);
	char aBuffer[40];
	std::string aLine;
	while (!aStream.mEOF)
	{
		GetLine(aStream, aBuffer, 40);
		aLine = aBuffer;
		if (aLine.find(GT(0x0052d428)) == 0)        // "#THEME"
		{
			mNumLoadingThreadTasks++;
			mCompletedLoadingThreadTasks++;
			mTitleScreen->MarkDirty();

			mThemes.resize(mThemes.size() + 1);     // 0x431560
			bool aFoundEnd = false;
			if (!aStream.mEOF)
			{
				do
				{
					if (aLine.find(GT(0x0052cd74)) == 0)    // "#END"
						break;

					GetLine(aStream, aBuffer, 40);
					aLine = aBuffer;
					MakeUpperCase(aLine);       // 0x416620
					TrimWhiteSpace(aLine);      // 0x416840

					if (aLine.find(GT(0x0052cd74)) == 0)
						aFoundEnd = true;
					if (aLine.find(GT(0x0052cd74)) != 0)
					{
						if (strlen(aLine.c_str()) < 8)
							mThemes[mThemes.size() - 1].push_back(aLine);
					}
				} while (!aStream.mEOF);

				if (aFoundEnd)
					continue;
			}

			Popup(GT(0x0052d3f4));      // "#THEME found without matching #END in themes.txt"
			p_fclose(aFile);
			return false;
		}

		if (aLine.c_str()[0] == '#')
		{
			Popup(StrFormat(GTC(0x0052d3c8), aLine.c_str()));   // "Illegal statement found in themes.txt: %s"
			p_fclose(aFile);
			return false;
		}
	}

	p_fclose(aFile);
	return true;
}
