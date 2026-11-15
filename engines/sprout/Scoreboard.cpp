#include "Scoreboard.h"

using namespace engine;

namespace
{
const char* const RANK_PARAM = "rank";  // 0x521320 (pizzafrenzy)
}

// 0x479690 (pizzafrenzy)
void ScoreEntry::rankToParams(Params& theParams)
{
	theParams.setInt(RANK_PARAM, mRank);
}

// 0x4796b0 (pizzafrenzy)
void ScoreEntry::rankFromParams(const Params& theParams)
{
	mRank = (int)theParams.getInt(RANK_PARAM, mRank);
}

// 0x4796d0 (pizzafrenzy): the value is 0, which is what the entry has
void ScoreEntry::rankDefaultParams(Params& theParams)
{
	theParams.setInt(RANK_PARAM, 0);
}

// 0x4830c0 (pizzafrenzy)
ScoreEntry* Scoreboard::getEntry(int theIndex)
{
	if (theIndex < 0 || theIndex >= (int)mEntries.size())
		return nullptr;
	return mEntries[theIndex];
}

// 0x4830f0 (pizzafrenzy)
int Scoreboard::getRank(ScoreEntry* theEntry)
{
	int aPosition = findPosition(theEntry);
	if (aPosition != (int)mEntries.size())
		return mEntries[aPosition]->getRank();
	if ((int)mEntries.size() < mMaxCount)
		return (int)mEntries.size() + 1;
	return 0;
}

// 0x483840 (pizzafrenzy)
bool Scoreboard::add(ScoreEntry* theEntry)
{
	mLastAddedIndex = -1;
	int aRank = getRank(theEntry);
	if (aRank > 0)
	{
		theEntry->setRank(aRank);
		return insert(theEntry);
	}
	return false;
}

// 0x483720 (pizzafrenzy): the entry goes before the first one whose rank is not below its own, the ranks after it count
// up, and the entries past the most the board holds are let go
bool Scoreboard::insert(ScoreEntry* theEntry)
{
	size_t aPlace = 0;  // 0x4831c0 (pizzafrenzy)
	while (aPlace < mEntries.size() && mEntries[aPlace]->getRank() < theEntry->getRank())
		aPlace++;
	if ((int)aPlace >= mMaxCount)
		return false;
	mLastAddedIndex = (int)aPlace;
	mEntries.insert(mEntries.begin() + aPlace, Ref<ScoreEntry>(theEntry));
	int aRank = theEntry->getRank();
	for (size_t i = aPlace; i < mEntries.size(); i++)
		mEntries[i]->setRank(aRank++);
	while ((int)mEntries.size() > mMaxCount)
		mEntries.pop_back();
	return true;
}

// 0x483350 (pizzafrenzy)
void Scoreboard::clear()
{
	mEntries.clear();
}
