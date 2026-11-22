// HighScoreMgr.cpp: see HighScoreMgr.h.

#include "HighScoreMgr.h"

#include "Buffer.h"
#include "DataSync.h"
#include "SexyAppBase.h"
#include "compat2004/Compat2004.h"

using namespace Sexy;

// the version of highscores.dat; a file with a lower version is not read (0x471eb0)
static const int HIGH_SCORES_VERSION = 2;
static const char* const HIGH_SCORES_FILE = "userdata/highscores.dat";

// the first table of the default scores in the adventure's tables starts at this score and the others at the
// next one (0x45c300)
static const int DEFAULT_SCORE_COUNT = 10;
static const int DEFAULT_SCORE_FIRST = 300000;
static const int DEFAULT_SCORE_STEP = 10000;
static const int DEFAULT_ADVENTURE_SCORE_FIRST = 20000000;
static const int DEFAULT_ADVENTURE_SCORE_FIRST_STEP = 3000000;
static const int DEFAULT_ADVENTURE_SCORE_STEP = 1500000;
static const char* const ADVENTURE_TABLE = "_adventure";

// the names the default scores are given (a random one for each, 0x45c300)
static const char* const DEFAULT_NAMES[] = {
	"Andy",	   "Anthony", "Ben",	 "Bill",	"Brian",  "Ace",   "Chad",	"Dave",	"Del",	"Ed",	 "Jimmy Goggle",
	"Eric",	   "Hans",	  "Isaac",	 "Jason",	"James",  "Jeff",  "Joe",	"John",	"Josh", "Juho",	 "Kathy",
	"Katrina", "Mark",	  "Matt",	 "Max",		"Shawn",  "Stephen", "Sukhbir", "Tysen", "Walter", "Wes"};
static const int DEFAULT_NAME_COUNT = (int)(sizeof(DEFAULT_NAMES) / sizeof(DEFAULT_NAMES[0]));

// 0x45a130 with 0x436670 for each entry: the count, then each name and score
void HighScoreMgr::SyncEntries(DataSync& theSync, HighScoreList& theList)
{
	if (theSync.IsReading())
	{
		theList.clear();
		int aCount = 0;
		theSync.SyncLong(aCount);
		for (int i = 0; i < aCount; i++)
		{
			HighScoreEntry anEntry;
			theSync.SyncString(anEntry.mName);
			theSync.SyncLong(anEntry.mScore);
			theList.push_back(anEntry);
		}
	}
	else
	{
		int aCount = (int)theList.size();
		theSync.SyncLong(aCount);
		for (HighScoreList::iterator anItr = theList.begin(); anItr != theList.end(); ++anItr)
		{
			theSync.SyncString(anItr->mName);
			theSync.SyncLong(anItr->mScore);
		}
	}
}

// 0x46e480: the count of tables, then each table's name and entries
void HighScoreMgr::SyncTables(DataSync& theSync)
{
	if (theSync.IsReading())
	{
		mTables.clear();
		int aCount = 0;
		theSync.SyncLong(aCount);
		for (int i = 0; i < aCount; i++)
		{
			std::string aName;
			theSync.SyncString(aName);
			SyncEntries(theSync, mTables[aName]);
		}
	}
	else
	{
		int aCount = (int)mTables.size();
		theSync.SyncLong(aCount);
		for (TableMap::iterator anItr = mTables.begin(); anItr != mTables.end(); ++anItr)
		{
			std::string aName = anItr->first;
			theSync.SyncString(aName);
			SyncEntries(theSync, anItr->second);
		}
	}
}

// 0x471eb0
void HighScoreMgr::SyncState(DataSync& theSync)
{
	int aVersion = HIGH_SCORES_VERSION;
	theSync.SyncLong(aVersion);
	theSync.mVersion = aVersion;
	if (aVersion >= HIGH_SCORES_VERSION)
		SyncTables(theSync);
}

// 0x471f00
bool HighScoreMgr::Load()
{
	mTables.clear();
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(HIGH_SCORES_FILE, &aBuffer))
		return false;
	DataSync aSync;
	aSync.StartReadMemory(aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	try
	{
		SyncState(aSync);
	}
	catch (DataSyncException&)
	{
		// a file cut short keeps the tables read up to there
	}
	return true;
}

// 0x472060
bool HighScoreMgr::Save()
{
	DataSync aSync;
	aSync.StartWriteMemory();
	SyncState(aSync);
	return gSexyAppBase->WriteBytesToFile(HIGH_SCORES_FILE, aSync.GetWrittenData().data(),
										  (unsigned long)aSync.GetWrittenData().size());
}

// 0x45c300: ten scores with random names, falling from the first score
void HighScoreMgr::FillDefaultScores(HighScoreList& theList, const std::string& theTable)
{
	bool isAdventure = sexy_stricmp(theTable.c_str(), ADVENTURE_TABLE) == 0;
	int aScore = isAdventure ? DEFAULT_ADVENTURE_SCORE_FIRST : DEFAULT_SCORE_FIRST;
	for (int i = 0; i < DEFAULT_SCORE_COUNT; i++)
	{
		HighScoreEntry anEntry;
		anEntry.mName = DEFAULT_NAMES[CrtRand() % DEFAULT_NAME_COUNT];
		anEntry.mScore = aScore;
		theList.push_back(anEntry);
		if (!isAdventure)
			aScore -= DEFAULT_SCORE_STEP;
		else
			aScore -= (i == 0) ? DEFAULT_ADVENTURE_SCORE_FIRST_STEP : DEFAULT_ADVENTURE_SCORE_STEP;
	}
}

// 0x4719d0: the table of that name (ignoring case), made empty when it is new
HighScoreList& HighScoreMgr::GetTable(const std::string& theTable)
{
	return mTables[theTable];
}

// 0x472210
void HighScoreMgr::Submit(const std::string& theTable, const std::string& theName, int theScore, bool theSubmit,
						  bool theReload)
{
	if (theScore <= 0)
		return;

	HighScoreList& aList = GetTable(theTable);
	if (aList.empty())
		FillDefaultScores(aList, theTable);

	for (HighScoreList::iterator anItr = aList.begin(); anItr != aList.end(); ++anItr)
		anItr->mIsNew = false;

	// the new score goes before the first one it beats
	HighScoreList::iterator aPos = aList.begin();
	while (aPos != aList.end() && aPos->mScore >= theScore)
		++aPos;

	if ((aPos != aList.end() || (int)aList.size() < MAX_ENTRIES) && theSubmit)
	{
		if (!theReload)
		{
			HighScoreEntry anEntry;
			anEntry.mName = theName;
			anEntry.mScore = theScore;
			anEntry.mIsNew = true;
			aList.insert(aPos, anEntry);
			while ((int)aList.size() > MAX_ENTRIES)
				aList.pop_back();
			Save();
		}
		else
		{
			// another copy of the game may have saved scores since: read them and put this one in
			Load();
			Submit(theTable, theName, theScore, true, false);
		}
	}
}

// 0x472430
HighScoreList* HighScoreMgr::GetScores(const std::string& theTable, bool theOnlyNew)
{
	if (theTable.empty())
		return nullptr;

	HighScoreList& aList = GetTable(theTable);
	if (aList.empty())
		FillDefaultScores(aList, theTable);

	if (theOnlyNew)
	{
		HighScoreList::iterator anItr = aList.begin();
		while (anItr != aList.end() && !anItr->mIsNew)
			++anItr;
		if (anItr == aList.end())
			return nullptr;
	}
	return &aList;
}
