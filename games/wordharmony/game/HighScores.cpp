#include "HighScores.h"
#include "Host.h"
#include <cstdio>
#include <cstring>

// 0x421100
HighScores::~HighScores()
{
	for (unsigned int i = 0; i < mEntries.size(); i++)
		delete mEntries[i];
}

// 0x421750: the scores of 0 are not added
void HighScores::load(const std::string& theName)
{
	char aValueName[32];
	mKey = theName;
	int i = 0;
	do
	{
		std::sprintf(aValueName, "scoreName%d", i);
		std::string aName;
		if (!engine::getHost()->registryReadString(mKey + "\\" + aValueName, aName))
			aName = "";
		std::sprintf(aValueName, "scoreValue%d", i);
		int aScore = 0;
		if (!engine::getHost()->registryReadInt(mKey + "\\" + aValueName, aScore))
			aScore = 0;
		if (aScore != 0)
			add(aName, (unsigned int)aScore);
		i++;
	} while (i < 10);
}

// 0x421200
void HighScores::save()
{
	char aValueName[32];
	int i = 0;
	std::vector<Entry*>::iterator anIt = mEntries.begin();
	if (anIt != mEntries.end())
	{
		do
		{
			Entry* anEntry = *anIt;
			std::sprintf(aValueName, "scoreName%d", i);
			engine::getHost()->registryWriteString(mKey + "\\" + aValueName,
			                                       std::string(anEntry->mName, strnlen(anEntry->mName, 16)));
			std::sprintf(aValueName, "scoreValue%d", i);
			engine::getHost()->registryWriteInt(mKey + "\\" + aValueName, (int)anEntry->mScore);
			i++;
			++anIt;
		} while (anIt != mEntries.end());
	}
}

// 0x421600: the entries cut off are not freed in the original; the port frees them
void HighScores::add(std::string theName, unsigned int theScore)
{
	Entry* anEntry = new Entry();
	std::strncpy(anEntry->mName, theName.c_str(), 16);
	anEntry->mScore = theScore;
	std::vector<Entry*>::iterator anIt = mEntries.begin();
	bool aPlaced = false;
	if (anIt != mEntries.end())
	{
		do
		{
			if ((*anIt)->mScore <= theScore)
			{
				mEntries.insert(anIt, anEntry);
				aPlaced = true;
				break;
			}
			++anIt;
		} while (anIt != mEntries.end());
	}
	if (!aPlaced)
		mEntries.push_back(anEntry);
	while (mEntries.size() >= 11)
	{
		delete mEntries.back();
		mEntries.pop_back();
	}
}

// 0x4210a0
unsigned int HighScores::getScore(unsigned int theIndex)
{
	if (theIndex < mEntries.size())
		return mEntries[theIndex]->mScore;
	return 0xffffffff;
}

// 0x4210d0
unsigned int HighScores::findRank(unsigned int theScore)
{
	unsigned int i = 0;
	while (i < mEntries.size() && theScore < mEntries[i]->mScore)
		i++;
	return i;
}

// 0x4213b0: the original reads on into the score after a name of 16 letters; the port stops there
std::string HighScores::getName(int theIndex)
{
	const char* aName = mEntries[theIndex]->mName;
	return std::string(aName, strnlen(aName, 16));
}
