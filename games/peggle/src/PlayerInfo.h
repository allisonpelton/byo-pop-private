// PlayerInfo.h: one player's profile: the name, how far the player has got, the options chosen and the totals of what was
// played (the original's Sexy::PlayerInfo, 0x210 bytes). Profiles are kept by the ProfileMgr; a profile's details are in the
// file userdata/user<id>.dat and are read when the profile is first used.

#ifndef __PLAYERINFO_H__
#define __PLAYERINFO_H__

#include "Common.h"
#include "GameStats.h"

#include <map>
#include <set>
#include <string>

namespace Sexy
{

class DataSync;

static const int NUM_TIP_KINDS = 10;

// the saved game slots of GetSavedGameName
enum SavedGameKind
{
	SAVED_GAME_STAGE = 1,	// "userdata/stage<id>.sav"
	SAVED_GAME_ARCADE = 2,
	SAVED_GAME_DUEL = 3,
	SAVED_GAME_TROPHY = 4
};

class PlayerInfo
{
public:
	PlayerInfo();

	void Reset(bool theKeepEarned);		// 0x421850
	void SyncSummary(DataSync& theSync);		// 0x403670: what the profile list file has of each profile
	void SyncDetails(DataSync& theSync);		// 0x426980
	void LoadDetails();							// 0x426bd0
	void SaveDetails();							// 0x426d60
	void SaveIfDirty();							// 0x426f00
	void DeleteUserFiles();						// 0x41f860
	std::string GetSavedGameName(int theKind) const; // 0x412680
	void DeleteSavedGame(int theKind);			// 0x412940
	void DeleteSavedGames();					// 0x4129c0
	void RestartAdventure();					// 0x41f820

	bool IsLevelUnlocked(int theStage, int theLevel) const; // 0x4036b0
	int GetMaxLevel() const;					// 0x4037e0
	int GetAdventureEfficiencyBonus() const;	// 0x4037f0
	void AddTotalStats(const GameStats& theStats); // 0x403840
	bool JustBeatAdventure() const;				// 0x403870
	bool JustStartingAdventure() const;			// 0x4038a0
	void SetColorblind(bool theColorblind);		// 0x4036e0
	void SetSelCharacter(int theCharacter);		// 0x403700
	void SetSelCharacters(int theFirst, int theSecond); // 0x403720
	void SetComputerPlayer(bool theComputerPlayer); // 0x403750
	void SetComputerSkill(int theSkill);		// 0x403770
	void SetLevelSelectStage(int theStage);		// 0x403790
	void IncTip(int theTip);					// 0x4037b0
	bool SetTip(int theTip);					// 0x4038c0: true when the tip was not shown before
	bool IsTipSet(int theTip) const;			// 0x4038f0

	std::string mName;				   // 0x4
	int mUseSeq;					   // 0x20: grows with each use of the profile, to find the last one used
	int mUserId;					   // 0x24: names the profile's files
	int mMaxStage;					   // 0x28: the adventure's furthest stage reached
	int mMaxLevel;					   // 0x2c: and the furthest level of that stage
	int mAdventureStage;			   // 0x30: where the adventure is now (-1: not started)
	int mAdventureLevel;			   // 0x34
	int mUnknown38;					   // 0x38
	int mLevelSelectStage;			   // 0x3c: the stage shown by the level selection screen
	int mSelCharacter;				   // 0x40
	int mSelCharacter2;				   // 0x44: the second player's
	int mAdventuresBeaten;			   // 0x48
	bool mComputerPlayer;			   // 0x4c
	int mComputerSkill;				   // 0x50 (-1: none chosen)
	int mUnknown54;					   // 0x54: one byte in the file
	int mUnknown58;					   // 0x58: one byte in the file
	int mUnknown5c;					   // 0x5c
	bool mColorblind;				   // 0x60
	bool mAdventureIntroPending;	   // 0x61
	unsigned int mTipsSeen;			   // 0x64: a bit for each tip
	int mTipCount;					   // 0x68: tips shown in all
	int mOtherTipCount;				   // 0x6c: tips shown of kinds from NUM_TIP_KINDS up
	int mTipCounts[NUM_TIP_KINDS];	   // 0x70
	std::string mUnknown98;			   // 0x98 (not saved)
	std::string mUnknownB4;			   // 0xb4
	std::string mUnknownD0;			   // 0xd0
	bool mDirty;					   // 0xec: the details must be saved
	bool mUnknownED;				   // 0xed
	bool mUnknownEE;				   // 0xee
	bool mUnknownEF;				   // 0xef
	std::multiset<int> mUnknownF0;	   // 0xf0
	std::set<int> mTrophiesEarned;	   // 0xfc: the ids of the trophies earned
	std::map<int, int> mUnknown108;	   // 0x108
	int mTrophyRank;				   // 0x114: 0 none, 1 the adventure beaten, 2 all the trophies earned too, 3 all the levels cleared too (16 bits in the file)
	std::set<std::string, StringLessNoCase> mLevelsCleared; // 0x118: the levels cleared (by its use in MainMenu::SyncPlayerInfo)
	int mUnknown124;				   // 0x124: one byte in the file
	int mUnknown128;				   // 0x128: one byte in the file
	GameStats mTotalStats;			   // 0x12c
	GameStats mAdventureStats;		   // 0x198
	int mUnknown204;				   // 0x204
	int mUnknown208;				   // 0x208
	int mAdventureLostBalls;		   // 0x20c: counted against the adventure efficiency bonus
};

} // namespace Sexy

#endif
