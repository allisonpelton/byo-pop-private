// The high scores (see HighScores.h).

#include "HighScores.h"
#include "Exe.h"
#include "File.h"
#include "Globals.h"
#include "Language.h"

namespace bonnies
{

HighScores gHighScores;
int gHighScoreRank;
int gHighScoreTable;

// 0x40ce80
HighScores::HighScores()
{
	mLoaded = 0;
}

// 0x40cd90
void HighScores::save()
{
	int aFile = fileOpen(lit(0x457d90), true, false);
	if (aFile != -1)
	{
		HighScore* anEntry = mScores;
		int aTables = 2;
		do
		{
			int aLeft = 5;
			do
			{
				fileWriteInt(aFile, &anEntry->mUsed);
				fileWriteString(aFile, &anEntry->mName);
				fileWriteString(aFile, &anEntry->mWord);
				fileWriteInt(aFile, &anEntry->mScore);
				anEntry++;
				aLeft--;
			} while (aLeft != 0);
			aTables--;
		} while (aTables != 0);
		fileClose(aFile);
	}
}

// 0x40ce10
void HighScores::reset()
{
	HighScore* anEntry = mScores;
	uint32_t aTable = 0x457d18;
	for (int t = 0; t < 2; t++)
	{
		uint32_t anAddress = aTable;
		int aLeft = 5;
		do
		{
			anEntry->mUsed = 1;
			anEntry->mName = getText(exeI32(anAddress));
			anEntry->mWord = getText(exeI32(anAddress + 4));
			anEntry->mScore = exeI32(anAddress + 8);
			anAddress += 0xc;
			anEntry++;
			aLeft--;
		} while (aLeft != 0);
		aTable += 0x3c;
	}
}

// 0x40ceb0
void HighScores::load()
{
	mLoaded = 1;
	int aFile = fileOpen(lit(0x457d90), false, false);
	if (aFile == -1)
	{
		reset();
		return;
	}
	HighScore* anEntry = mScores;
	int aTables = 2;
	do
	{
		int aLeft = 5;
		do
		{
			fileReadInt(aFile, &anEntry->mUsed);
			fileReadString(aFile, anEntry->mName);
			fileReadString(aFile, anEntry->mWord);
			fileReadInt(aFile, &anEntry->mScore);
			anEntry++;
			aLeft--;
		} while (aLeft != 0);
		aTables--;
	} while (aTables != 0);
	fileClose(aFile);
}

// 0x40cf50
int HighScores::add(int theTable, Str theName, Str theWord, int theScore)
{
	if (mLoaded == 0)
		load();
	if ((int)theWord.size() == 0)
		theWord = getText(0x532);
	int aRank = 0;
	HighScore* aTable = &mScores[theTable * 5];
	while (aTable[aRank].mUsed != 0 && theScore < aTable[aRank].mScore)
	{
		aRank++;
		if (4 < aRank)
			return -1;
	}
	if (aRank + 1 < 5)
	{
		int aLeft = 5 - (aRank + 1);
		int k = 4;
		do
		{
			aTable[k].mUsed = aTable[k - 1].mUsed;
			aTable[k].mName = aTable[k - 1].mName;
			aTable[k].mWord = aTable[k - 1].mWord;
			aLeft--;
			aTable[k].mScore = aTable[k - 1].mScore;
			k--;
		} while (aLeft != 0);
	}
	HighScore& anEntry = aTable[aRank];
	anEntry.mUsed = 1;
	anEntry.mWord = theWord;
	anEntry.mName = theName;
	anEntry.mScore = theScore;
	// The word capitalised
	Str aWord = "";
	if ((int)anEntry.mWord.size() == 0)
		anEntry.mWord = getText(0x41b);
	int i = 0;
	if (0 < (int)anEntry.mWord.size())
	{
		do
		{
			unsigned char aChar = at(anEntry.mWord, i);
			aWord += (char)(i == 0 ? toUpper(aChar) : toLower(aChar));
			i++;
		} while (i < (int)anEntry.mWord.size());
	}
	anEntry.mWord = aWord;
	save();
	return aRank;
}

} // namespace bonnies
