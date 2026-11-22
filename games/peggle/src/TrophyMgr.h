// TrophyMgr.h: the trophy pages of the game's challenge mode, read from levels/trophy.cfg. A page holds the trophies
// that share a description; a trophy is one challenge (levels to play with a number of balls, lives, orange pegs, a
// score to reach, opponents). The original's Sexy::TrophyMgr and Sexy::TrophyInfo.

#ifndef __TROPHYMGR_H__
#define __TROPHYMGR_H__

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Sexy
{

class ConfigParser;

// Opponent values the file can give in place of a character (0x468900)
static const int TROPHY_OPPONENT_NONE = -2;
static const int TROPHY_OPPONENT_RANDOM = 10;

class TrophyInfo
{
public:
	// the original's constructor is 0x45f500; the defaults are in the member initialisers
	int mId = 0;			  // +8 (0 until the file gives one)
	std::string mName;		  // +0xc (the group's name)
	std::string mDesc;		  // +0x28
	std::string mSmallDesc;	  // +0x44 (the page's when the trophy has none)
	bool mUnknown60 = false;  // +0x60 (trophy.cfg never sets it)
	int mBalls = 0;			  // +0x64
	int mNumOrange = 0;		  // +0x68
	int mLives = 0;			  // +0x6c
	int mScoreReq = 0;		  // +0x70
	bool mScoreReset = true;  // +0x74
	bool mClearLevel = false; // +0x75
	std::vector<std::string> mLevels;			 // +0x78 (level names, "rand..." or "pick")
	std::vector<std::pair<int, int>> mOpponents; // +0x88 (character, difficulty)
};

typedef std::shared_ptr<TrophyInfo> TrophyInfoPtr;

struct TrophyPage
{
	std::string mName;		// +0
	std::string mDesc;		// +0x1c
	std::string mSmallDesc; // +0x38
	std::vector<TrophyInfoPtr> mTrophies; // +0x54
};

class TrophyMgr
{
public:
	TrophyMgr() {}

	// 0x474550: reads "levels/trophy.cfg" (or the file given)
	void Load(const char* theFileName);
	TrophyInfo* GetTrophyInfoById(int theId); // 0x4430b0: none when there is no such trophy

	std::vector<TrophyPage> mPages;				  // +8
	std::map<int, TrophyInfoPtr> mTrophyById;	  // +0x18

private:
	void Clear();															  // 0x471960
	void ReadConfig(const std::string& theFileName);						  // 0x474390
	void ReadTrophyPage(ConfigParser& theParser);							  // 0x473e70
	void ReadTrophyConfig(ConfigParser& theParser, TrophyInfo& theTrophy);	  // 0x468900
};

} // namespace Sexy

#endif
