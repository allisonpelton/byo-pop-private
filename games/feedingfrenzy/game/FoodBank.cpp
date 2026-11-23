#include "FoodBank.h"
#include "PersistKey.h"

using namespace engine;
using namespace feedingfrenzy;

namespace
{
const char* const GAME_KEY = "GameHouse\\FeedingFrenzy\\";
const char* const COUNT_VALUE = "foodCount";
// The most the count holds, and the counts at which a rank begins
const unsigned int MAX_COUNT = 10000000;
const unsigned int RANK_COUNTS[5] = {500, 1500, 3000, 4000, 10000};
}

// 0x431650
void FoodBank::load(const std::string& theKeyName)
{
	PersistKey aKey(GAME_KEY + theKeyName, theKeyName + "\\");
	mCount = (unsigned int)aKey.readInt(COUNT_VALUE, 0);
}

// 0x431790
void FoodBank::save(const std::string& theKeyName) const
{
	PersistKey aKey(GAME_KEY + theKeyName, theKeyName + "\\");
	aKey.writeInt(COUNT_VALUE, (int)mCount);
}

// 0x4315f0
void FoodBank::add(int theCount)
{
	unsigned int aCount = mCount + theCount;
	if (aCount > MAX_COUNT - 1)
		aCount = MAX_COUNT;
	mCount = aCount;
}

// 0x431610
int FoodBank::getRank() const
{
	int aRank = 0;
	while (aRank < 5 && mCount >= RANK_COUNTS[aRank])
		aRank++;
	return aRank;
}
