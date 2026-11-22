// StatsMgr.h: the statistics the game keeps about every level played (cleared and failed counts, time spent), and the
// results of the trophy pages, in a file userdata/stat_<name>_<time>.dat. The original's Sexy::StatsMgr. The game only
// records them; a screen never shows them.

#ifndef __STATSMGR_H__
#define __STATSMGR_H__

#include "Common.h"

#include <map>
#include <string>

namespace Sexy
{

class DataSync;

// What is known about one level (a record of 11 numbers that LogicMgr::RecordStats adds a game's numbers to, 0x437640,
// 0x4376f0).
struct LevelStats
{
	int mTimesCleared = 0;		 // +0
	int mTimesFailed = 0;		 // +4
	int mSecondsCleared = 0;	 // +8
	int mSecondsFailed = 0;		 // +0xc
	int mUnknown10[4] = {};		 // +0x10 (a game's numbers, zero after a failed game)
	int mUnknown20 = 0;			 // +0x20 (added up)
	unsigned int mUnknown24 = 0; // +0x24 (the larger of the two, with mUnknown28 of that record)
	int mUnknown28 = 0;			 // +0x28

	void SyncState(DataSync& theSync);	  // 0x437670
	void Add(const LevelStats& theOther); // 0x4376f0
};

struct TrophyStats
{
	int mCleared = 0;				   // times the trophy page was cleared
	int mFailed = 0;				   // times it was left or failed
	void SyncState(DataSync& theSync); // 0x437750
};

class StatsMgr
{
public:
	typedef std::map<std::string, LevelStats, StringLessNoCase> LevelStatsMap;
	typedef std::map<int, TrophyStats> TrophyStatsMap;

	static const int NUM_STAGE_COUNTS = 12;
	static const int NUM_GAME_COUNTS = 16;

	StatsMgr();							   // 0x461b00
	void Clear();						   // 0x452850
	void SyncState(DataSync& theSync);	   // 0x461bd0
	// 0x461d20: reads the file of that name; without one, the first "userdata/stat_*.dat" there is
	void Load(const char* theFileName);
	// 0x4620a0: writes the file of that name; without one, "userdata/stat_<the name's letters and digits>_<time>.dat"
	void Save(const char* theFileName);
	TrophyStats& GetStatsForTrophy(int theTrophyId); // 0x459340

	LevelStatsMap mLevelStats;			 // +0
	LevelStatsMap mLevelStatsDuel;		 // +0xc (the version 3 additions)
	std::string mName;					 // +0x18
	int mCreationTime;					 // +0x34 (seconds since 1970 when the statistics were started)
	int mStageCountsA[NUM_STAGE_COUNTS]; // +0x38, +0x68: games counted by the stage's number (0x437780, 0x4377a0)
	int mStageCountsB[NUM_STAGE_COUNTS];
	int mClearedA[NUM_GAME_COUNTS]; // +0x98: cleared games by a number of the game (0x4377c0, 0x437800)
	int mClearedB[NUM_GAME_COUNTS]; // +0xd8
	int mFailedA[NUM_GAME_COUNTS];	// +0x118: failed games by the same number (0x4377e0, 0x437820)
	int mFailedB[NUM_GAME_COUNTS];	// +0x158
	int mGamesPlayed;				// +0x198
	TrophyStatsMap mTrophyStats;	// +0x19c

private:
	void SyncLevelStatsMap(DataSync& theSync, LevelStatsMap& theMap); // 0x45a260
	void SyncTrophyStatsMap(DataSync& theSync);						  // 0x44d2f0
};

} // namespace Sexy

#endif
