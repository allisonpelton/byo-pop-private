// The high score tables (see HighScores.h).

#include "HighScores.h"
#include "Buffer.h"

#include <cstring>
#include <ctime>

using namespace Sexy;

// 0x41e320: the start values are the original's scores; the names are the port's own (the original's are texts of its code)
void ScoreTable::Reset()
{
	static const int kScores[NUM_SCORES] = {40000, 25000, 15000, 10000, 5000};
	static const char* const kNames[NUM_SCORES] = {"Quark Quinn", "Proton Pat", "Neutron Nick", "Electron Eve", "Ion Ivy"};
	for (int i = 0; i < NUM_SCORES; i++)
	{
		mScores[i] = kScores[i];
		memset(mNames[i], 0, NAME_SIZE);
		strncpy(mNames[i], kNames[i], NAME_SIZE - 1);
	}
	mBestToday = -1;
	memset(mBestName, 0, NAME_SIZE);
	mDate = -1;
}

// 0x41e260
void ScoreTable::Read(Buffer* theBuffer)
{
	theBuffer->ReadBytes((unsigned char*) mScores, sizeof(mScores));
	theBuffer->ReadBytes((unsigned char*) mNames, sizeof(mNames));
	theBuffer->ReadBytes((unsigned char*) &mBestToday, 4);
	theBuffer->ReadBytes((unsigned char*) mBestName, NAME_SIZE);
	theBuffer->ReadBytes((unsigned char*) &mDate, 4);
	for (int i = 0; i < NUM_SCORES; i++)
		mNames[i][NAME_SIZE - 1] = 0;
	mBestName[NAME_SIZE - 1] = 0;
}

// 0x41e2c0
void ScoreTable::Write(Buffer* theBuffer) const
{
	theBuffer->WriteBytes((const unsigned char*) mScores, sizeof(mScores));
	theBuffer->WriteBytes((const unsigned char*) mNames, sizeof(mNames));
	theBuffer->WriteBytes((const unsigned char*) &mBestToday, 4);
	theBuffer->WriteBytes((const unsigned char*) mBestName, NAME_SIZE);
	theBuffer->WriteBytes((const unsigned char*) &mDate, 4);
}

int ScoreTable::DateCode()
{
	time_t aNow = time(NULL);
	struct tm* aTime = gmtime(&aNow);	// the original's GetSystemTime: UTC
	if (aTime == NULL)
		return 0;
	return ((aTime->tm_mon + 1) + (aTime->tm_year + 1900) * 12) * 31 + aTime->tm_mday;
}

int ScoreTable::Place(int theScore) const
{
	int aPlace = 0;
	while ((aPlace < NUM_SCORES) && (mScores[aPlace] >= theScore))
		aPlace++;
	return aPlace;
}

// 0x41e440: the score is worth a name if it is in the table (a place before the end), or the best of the day
bool ScoreTable::Qualifies(int theScore) const
{
	if (theScore == 0)
		return false;
	// the loop of the original stops at the first score below the new one
	int aPlace = 0;
	while ((aPlace < NUM_SCORES) && !(mScores[aPlace] < theScore))
		aPlace++;
	return (mDate != DateCode()) || (mBestToday == -1) || (mBestToday < theScore) || (aPlace < NUM_SCORES);
}

// 0x41e530: a name for the score: the best of the day, and a place in the table
void ScoreTable::Add(int theScore, const char* theName)
{
	if ((theName == NULL) || (theName[0] == 0))
		return;
	int aToday = DateCode();
	if ((mDate != aToday) || (mBestToday == -1) || (mBestToday < theScore))
	{
		memset(mBestName, 0, NAME_SIZE);
		strncpy(mBestName, theName, NAME_SIZE - 1);
		mBestToday = theScore;
		mDate = aToday;
	}
	int aPlace = 0;
	while (theScore <= mScores[aPlace])
	{
		aPlace++;
		if (aPlace > 4)
			return;
	}
	for (int i = NUM_SCORES - 1; i > aPlace; i--)
	{
		mScores[i] = mScores[i - 1];
		memcpy(mNames[i], mNames[i - 1], NAME_SIZE);
	}
	mScores[aPlace] = theScore;
	memset(mNames[aPlace], 0, NAME_SIZE);
	strncpy(mNames[aPlace], theName, NAME_SIZE - 1);
}
