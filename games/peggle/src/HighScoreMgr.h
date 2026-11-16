// HighScoreMgr.h: the high score tables (the original's Sexy::HighScoreMgr), kept in userdata/highscores.dat. A table
// belongs to a level, a mode or "_adventure"; it has the best ten scores.

#ifndef __HIGHSCOREMGR_H__
#define __HIGHSCOREMGR_H__

#include "Common.h"

#include <list>
#include <map>
#include <string>

namespace Sexy
{

class DataSync;

struct HighScoreEntry
{
	std::string mName;
	int mScore = 0;
	bool mIsNew = false; // the score just submitted; not saved
};

typedef std::list<HighScoreEntry> HighScoreList;

class HighScoreMgr
{
public:
	typedef std::map<std::string, HighScoreList, StringLessNoCase> TableMap;

	static const int MAX_ENTRIES = 10;

	HighScoreMgr() {}

	void SyncState(DataSync& theSync);		   // 0x471eb0
	bool Load();							   // 0x471f00
	bool Save();							   // 0x472060
	// 0x472210: puts a score into a table when it makes the best ten; theSubmit false only asks (and does nothing),
	// theReload reads the file again first and then submits.
	void Submit(const std::string& theTable, const std::string& theName, int theScore, bool theSubmit, bool theReload);
	// 0x472430: a table (made with its default scores when it is new); none for an empty name, or, with theOnlyNew, when it
	// has no entry flagged new
	HighScoreList* GetScores(const std::string& theTable, bool theOnlyNew);

private:
	HighScoreList& GetTable(const std::string& theTable); // 0x4719d0
	void FillDefaultScores(HighScoreList& theList, const std::string& theTable); // 0x45c300
	void SyncTables(DataSync& theSync);							 // 0x46e480
	static void SyncEntries(DataSync& theSync, HighScoreList& theList); // 0x45a130

	TableMap mTables;
};

} // namespace Sexy

#endif
