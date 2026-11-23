#ifndef __FEEDINGFRENZY_SCORELIST_H__
#define __FEEDINGFRENZY_SCORELIST_H__

// The best scores of one way of playing the game (ScoreList, constructor 0x4353b0; the controller keeps one for the
// normal game at +0x140 and one for the timed game at +0x174): the ten highest, each with the name it was entered
// under, and the furthest stage the player has reached. The registry keys scores\normal and scores\timed
// below the game's own keep them.

#include <string>
#include <vector>

namespace feedingfrenzy
{

// A score: the name (at most 16 letters) and the points
struct ScoreEntry
{
	std::string mName;     // +0x00
	unsigned int mScore;   // +0x10
};

class ScoreList
{
public:
	ScoreList() : mFurthestStage(0) {}

	// 0x4353b0: the key's scores and furthest stage are read, the scores that have points only
	void load(const std::string& theKeyName);
	// 0x434d40: the furthest stage and every score are written
	void save(const std::string& theKeyName) const;

	// 0x459160: how many scores the list has
	int getCount() const { return (int)mEntries.size(); }
	const ScoreEntry& getEntry(int theIndex) const { return mEntries[theIndex]; }
	// 0x434c70: the place a score would take: how many of the list's scores are higher than it
	int getRank(unsigned int theScore) const;
	// 0x435260: a score is put before the first score that is no higher than it, and the list keeps its ten highest
	void add(const std::string& theName, unsigned int theScore);

	// 0x434c10 and 0x434c20: the furthest stage reached (a number from 1 that the stage manifest gives), which only grows
	int getFurthestStage() const { return mFurthestStage; }
	void reachStage(int theStage)
	{
		if (mFurthestStage < theStage)
			mFurthestStage = theStage;
	}

private:
	std::vector<ScoreEntry> mEntries;  // +0x04
	int mFurthestStage;                // +0x10
};

} // namespace feedingfrenzy

#endif
