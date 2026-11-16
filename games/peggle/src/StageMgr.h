// StageMgr.h: the ten stages of the adventure (five levels each, with the dialogs the master speaks before and during
// them) and the tips shown while a level loads, read from levels/stages.cfg. The original's Sexy::StageMgr,
// Sexy::StageInfo and Sexy::LevelInfo.

#ifndef __STAGEMGR_H__
#define __STAGEMGR_H__

#include "Common.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Sexy
{

class ConfigParser;

// The last use time a level has before it was picked (0x441fd0, 0x43d890)
static const int LEVEL_NEVER_USED = -1000;

class LevelInfo
{
public:
	int mLastUseTime = LEVEL_NEVER_USED; // +8: the number StageMgr::MarkLastUseTime gave it last
	int mLevelIndex = 0;				 // +0xc: the level's number in its stage
	int mStageIndex = 0;				 // +0x10: the stage's number
	std::string mFileName;				 // +0x14: the level's file in levels/ without ".dat"
	std::string mName;					 // +0x30: the name the player sees
	std::string mDialogText;			 // +0x4c: what the master says before the level
	std::string mDialogTitle;			 // +0x68
	bool mExcludedFromRandom = false;	 // +0x84: never chosen as a random level
};

typedef std::shared_ptr<LevelInfo> LevelInfoPtr;

// A line of a stage's dialog or of the credits (0x20 bytes in the original)
struct StageText
{
	std::string mText;
	int mValue = -1; // +0x1c: the number after the text in the file
};

class StageInfo
{
public:
	static const int NUM_TEXT_PAGES = 5;

	std::vector<StageText> mTexts[NUM_TEXT_PAGES]; // +8: the pages of the stage's dialog; the credits' lines follow the dialog's
	std::vector<LevelInfoPtr> mLevels;			   // +0x58

	LevelInfo* GetLevelInfo(int theLevel); // 0x43d7d0: none when there is no such level
};

typedef std::shared_ptr<StageInfo> StageInfoPtr;

class StageMgr
{
public:
	StageMgr() {}

	void Load(const char* theFileName);			// 0x468830: reads "levels/stages.cfg" (or the file given)
	StageInfo* GetStageInfo(int theStage);		// 0x43d820: none when there is no such stage
	int GetNumStages() const { return (int)mStages.size(); } // 0x43d870
	LevelInfo* FindLevel(const std::string& theFileName);	  // 0x442ff0: by file name, none when there is none
	void ResetLastUseTimes();					// 0x43d890
	void MarkLastUseTime(LevelInfo& theLevel);	// 0x437620

	int mUseCounter = 0;						// +8: counts the levels picked since the last ResetLastUseTimes
	std::vector<StageInfoPtr> mStages;			// +0xc
	std::map<std::string, LevelInfoPtr, StringLessNoCase> mLevelsByName; // +0x1c
	std::vector<std::string> mTips;				// +0x28

private:
	void Clear();								// 0x45f460
	void ReadConfig(const std::string& theFileName);	// 0x4685f0
	void ReadStageConfig(ConfigParser& theParser, StageInfo& theStage);	// 0x467d50
	void ReadKeyVal(ConfigParser& theParser);	// 0x468300
};

} // namespace Sexy

#endif
