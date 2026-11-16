// StatsMgr.cpp: see StatsMgr.h.

#include "StatsMgr.h"

#include "Buffer.h"
#include "DataSync.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>

using namespace Sexy;

// the version of the statistics file (0x461bd0): 2 added the trophy results, 3 the second set of levels and counts
static const int STATS_VERSION = 3;

// 0x437670
void LevelStats::SyncState(DataSync& theSync)
{
	theSync.SyncLong(mTimesCleared);
	theSync.SyncLong(mTimesFailed);
	theSync.SyncLong(mSecondsCleared);
	theSync.SyncLong(mSecondsFailed);
	theSync.SyncLong(mUnknown20);
	theSync.SyncLong(mUnknown24);
	theSync.SyncLong(mUnknown28);
	for (int i = 0; i < 4; i++)
		theSync.SyncLong(mUnknown10[i]);
}

// 0x4376f0
void LevelStats::Add(const LevelStats& theOther)
{
	mTimesCleared += theOther.mTimesCleared;
	mTimesFailed += theOther.mTimesFailed;
	mSecondsCleared += theOther.mSecondsCleared;
	mSecondsFailed += theOther.mSecondsFailed;
	mUnknown20 += theOther.mUnknown20;
	for (int i = 0; i < 4; i++)
		mUnknown10[i] += theOther.mUnknown10[i];
	if (mUnknown24 < theOther.mUnknown24)
	{
		mUnknown24 = theOther.mUnknown24;
		mUnknown28 = theOther.mUnknown28;
	}
}

// 0x437750
void TrophyStats::SyncState(DataSync& theSync)
{
	theSync.SyncLong(mCleared);
	theSync.SyncLong(mFailed);
}

// 0x461b00
StatsMgr::StatsMgr()
{
	Clear();
}

// 0x452850
void StatsMgr::Clear()
{
	mLevelStats.clear();
	mLevelStatsDuel.clear();
	mCreationTime = (int)time(nullptr);
	mName.clear();
	mGamesPlayed = 0;
	for (int i = 0; i < NUM_STAGE_COUNTS; i++)
	{
		mStageCountsA[i] = 0;
		mStageCountsB[i] = 0;
	}
	for (int i = 0; i < NUM_GAME_COUNTS; i++)
	{
		mClearedA[i] = 0;
		mClearedB[i] = 0;
		mFailedA[i] = 0;
		mFailedB[i] = 0;
	}
	mTrophyStats.clear();
}

// 0x45a260: the count of levels, then each level's name and numbers
void StatsMgr::SyncLevelStatsMap(DataSync& theSync, LevelStatsMap& theMap)
{
	if (theSync.IsReading())
	{
		theMap.clear();
		int aCount = 0;
		theSync.SyncLong(aCount);
		for (int i = 0; i < aCount; i++)
		{
			std::string aName;
			theSync.SyncString(aName);
			theMap[aName].SyncState(theSync);
		}
	}
	else
	{
		int aCount = (int)theMap.size();
		theSync.SyncLong(aCount);
		for (LevelStatsMap::iterator anItr = theMap.begin(); anItr != theMap.end(); ++anItr)
		{
			std::string aName = anItr->first;
			theSync.SyncString(aName);
			anItr->second.SyncState(theSync);
		}
	}
}

// 0x44d2f0: the count of trophies, then each trophy's number and results
void StatsMgr::SyncTrophyStatsMap(DataSync& theSync)
{
	if (theSync.IsReading())
	{
		mTrophyStats.clear();
		int aCount = 0;
		theSync.SyncLong(aCount);
		for (int i = 0; i < aCount; i++)
		{
			int aTrophyId = 0;
			theSync.SyncLong(aTrophyId);
			mTrophyStats[aTrophyId].SyncState(theSync);
		}
	}
	else
	{
		int aCount = (int)mTrophyStats.size();
		theSync.SyncLong(aCount);
		for (TrophyStatsMap::iterator anItr = mTrophyStats.begin(); anItr != mTrophyStats.end(); ++anItr)
		{
			int aTrophyId = anItr->first;
			theSync.SyncLong(aTrophyId);
			anItr->second.SyncState(theSync);
		}
	}
}

// 0x461bd0
void StatsMgr::SyncState(DataSync& theSync)
{
	int aVersion = STATS_VERSION;
	theSync.SyncLong(aVersion);
	theSync.mVersion = aVersion;

	theSync.SyncLong(mCreationTime);
	theSync.SyncString(mName);
	theSync.SyncLong(mGamesPlayed);
	for (int i = 0; i < NUM_STAGE_COUNTS; i++)
		theSync.SyncLong(mStageCountsA[i]);
	for (int i = 0; i < NUM_GAME_COUNTS; i++)
	{
		theSync.SyncLong(mClearedA[i]);
		theSync.SyncLong(mFailedA[i]);
	}
	SyncLevelStatsMap(theSync, mLevelStats);

	if (aVersion > 1)
		SyncTrophyStatsMap(theSync);

	if (aVersion > 2)
	{
		for (int i = 0; i < NUM_STAGE_COUNTS; i++)
			theSync.SyncLong(mStageCountsB[i]);
		for (int i = 0; i < NUM_GAME_COUNTS; i++)
		{
			theSync.SyncLong(mClearedB[i]);
			theSync.SyncLong(mFailedB[i]);
		}
		SyncLevelStatsMap(theSync, mLevelStatsDuel);
	}
}

// 0x461d20
void StatsMgr::Load(const char* theFileName)
{
	Clear();

	std::string aFileName;
	if (theFileName != nullptr)
		aFileName = theFileName;
	else
	{
		// the first of the statistics files in the user's folder
		std::vector<std::string> aNames = Platform::ListDirectory(Platform::VFS::Get().GetUserDir());
		std::sort(aNames.begin(), aNames.end());
		for (size_t i = 0; i < aNames.size(); i++)
		{
			const std::string& aName = aNames[i];
			if (aName.size() >= 9 && sexy_strnicmp(aName.c_str(), "stat_", 5) == 0 &&
				sexy_stricmp(aName.c_str() + aName.size() - 4, ".dat") == 0)
			{
				aFileName = "userdata/" + aName;
				break;
			}
		}
		if (aFileName.empty())
			return;
	}

	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(aFileName, &aBuffer))
		return;
	DataSync aSync;
	aSync.StartReadMemory(aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	try
	{
		SyncState(aSync);
	}
	catch (DataSyncException&)
	{
		// a file cut short keeps what was read up to there
	}
}

// 0x4620a0
void StatsMgr::Save(const char* theFileName)
{
	std::string aFileName;
	if (theFileName != nullptr)
		aFileName = theFileName;
	else
	{
		std::string aLetters;
		for (size_t i = 0; i < mName.size(); i++)
		{
			if (isalnum((unsigned char)mName[i]))
				aLetters += mName[i];
		}
		char aBuffer[512];
		snprintf(aBuffer, sizeof(aBuffer), "userdata/stat_%s_%x.dat", aLetters.c_str(), (unsigned int)mCreationTime);
		aFileName = aBuffer;
	}

	DataSync aSync;
	aSync.StartWriteMemory();
	SyncState(aSync);
	gSexyAppBase->WriteBytesToFile(aFileName, aSync.GetWrittenData().data(), (unsigned long)aSync.GetWrittenData().size());
}

// 0x459340: the trophy's results, started at nothing when it has none
TrophyStats& StatsMgr::GetStatsForTrophy(int theTrophyId)
{
	return mTrophyStats[theTrophyId];
}
