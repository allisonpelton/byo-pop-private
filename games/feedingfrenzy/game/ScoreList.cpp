#include "ScoreList.h"
#include "PersistKey.h"

#include <cstdio>

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The registry: the game's key, the values of a list and the formats of a score's two
const char* const GAME_KEY = "GameHouse\\FeedingFrenzy\\";
const char* const FURTHEST_STAGE_VALUE = "maxStageReached";
const char* const NAME_VALUE_FORMAT = "scoreName%d";
const char* const SCORE_VALUE_FORMAT = "scoreValue%d";
// How many scores the list keeps, and how many letters of a name
const int MAX_SCORES = 10;
const int MAX_NAME_LENGTH = 16;
}

// 0x4353b0
void ScoreList::load(const std::string& theKeyName)
{
	PersistKey aKey(GAME_KEY + theKeyName, theKeyName + "\\");
	mFurthestStage = aKey.readInt(FURTHEST_STAGE_VALUE, 0);
	for (int i = 0; i < MAX_SCORES; i++)
	{
		char aValueName[32];
		sprintf(aValueName, NAME_VALUE_FORMAT, i);
		std::string aName = aKey.readString(aValueName, "");
		sprintf(aValueName, SCORE_VALUE_FORMAT, i);
		int aScore = aKey.readInt(aValueName, 0);
		if (aScore != 0)
			add(aName, (unsigned int)aScore);
	}
}

// 0x434d40
void ScoreList::save(const std::string& theKeyName) const
{
	PersistKey aKey(GAME_KEY + theKeyName, theKeyName + "\\");
	aKey.writeInt(FURTHEST_STAGE_VALUE, mFurthestStage);
	for (size_t i = 0; i < mEntries.size(); i++)
	{
		char aValueName[32];
		sprintf(aValueName, NAME_VALUE_FORMAT, (int)i);
		aKey.writeString(aValueName, mEntries[i].mName);
		sprintf(aValueName, SCORE_VALUE_FORMAT, (int)i);
		aKey.writeInt(aValueName, (int)mEntries[i].mScore);
	}
}

// 0x434c70
int ScoreList::getRank(unsigned int theScore) const
{
	int aRank = 0;
	while (aRank < (int)mEntries.size() && mEntries[aRank].mScore > theScore)
		aRank++;
	return aRank;
}

// 0x435260
void ScoreList::add(const std::string& theName, unsigned int theScore)
{
	ScoreEntry anEntry;
	anEntry.mName = theName.substr(0, MAX_NAME_LENGTH);
	anEntry.mScore = theScore;
	std::vector<ScoreEntry>::iterator anIt = mEntries.begin();
	while (anIt != mEntries.end() && anIt->mScore > theScore)
		++anIt;
	mEntries.insert(anIt, anEntry);
	while (mEntries.size() > (size_t)MAX_SCORES)
		mEntries.pop_back();
}
