#ifndef __TALISMANIA_LEVELDATAREADER_H__
#define __TALISMANIA_LEVELDATAREADER_H__

// The reader of the chapter and level elements of a data file (the loaders 0x5a2350 and 0x5b2b00 and the readers of
// their children); the data handler hands every element to it first.

#include "AtlantisData.h"
#include "Engine.h"
#include "Params.h"

#include <set>

class LevelDataReader
{
public:
	explicit LevelDataReader(AtlantisData& theData) : mData(theData) {}

	// True when the element is one of the chapters' and levels' and has been read
	bool startElement(const std::string& theName, const engine::Params& p);
	// True when the element is one of them
	bool endElement(const std::string& theName);

private:
	void beginChapter(const engine::Params& p);
	void beginLevel(const engine::Params& p);

	AtlantisData& mData;
	ChapterClass mChapter;
	GameLevelClass mLevel;
	std::vector<BuildRankCost>* mCosts = nullptr;  // the build costs the rank elements go to
	std::vector<ZoneStartClass>* mZones = nullptr;  // the meter zones the zone starts go to
	std::set<std::string> mReported;
};

#endif
