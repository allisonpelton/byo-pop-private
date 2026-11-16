// PlayerInfo.cpp: see PlayerInfo.h.

#include "PlayerInfo.h"

#include "DataSync.h"
#include "SexyAppBase.h"

#include <cstdio>

using namespace Sexy;

// the version of the details file the game writes, and the oldest it still reads (0x426980)
static const int PLAYER_DETAILS_VERSION = 0x17;
static const int OLDEST_PLAYER_DETAILS_VERSION = 0x13;

// the parameters of GetAdventureEfficiencyBonus (tweakable constants in the original's PlayerInfo.cpp)
static const int ADVENTURE_BONUS_MAXIMUM = 10000000;
static const int ADVENTURE_BONUS_LOST_BALL_COST = 200000;
static const int ADVENTURE_BONUS_MINIMUM = 100000;

// 0x429200
PlayerInfo::PlayerInfo()
{
	mUseSeq = 0;
	mUserId = 0;
	mUnknown5c = 0;
	Reset(false);
}

// 0x421850
void PlayerInfo::Reset(bool theKeepEarned)
{
	mDirty = true;
	mUnknownED = true;
	mUnknownEE = true;
	mComputerPlayer = true;
	mUnknown54 = 0;
	mColorblind = false;
	mAdventureIntroPending = true;
	mUnknownEF = false;
	mTipsSeen = 0;
	mTipCount = 0;
	mOtherTipCount = 0;
	mComputerSkill = -1;
	for (int i = 0; i < NUM_TIP_KINDS; i++)
		mTipCounts[i] = 0;
	mAdventureStage = -1;
	mMaxStage = 0;
	mMaxLevel = 0;
	mAdventureLevel = 0;
	mSelCharacter = 0;
	mSelCharacter2 = 1;
	mUnknownB4.clear();
	mUnknownD0.clear();
	mUnknown38 = 0;
	mUnknown124 = 0;
	mUnknown128 = 0;
	mAdventuresBeaten = 0;
	mUnknown58 = 0;
	mLevelSelectStage = 0;
	mAdventureLostBalls = 0;
	mAdventureStats.Reset();
	mUnknown204 = 0;
	mUnknown208 = 0;
	if (!theKeepEarned)
	{
		mUnknownFC.clear();
		mUnknown108.clear();
		mUnknown118.clear();
		mUnknownF0.clear();
		mTotalStats.Reset();
	}
	mUnknown114 = 0;
}

// 0x403670
void PlayerInfo::SyncSummary(DataSync& theSync)
{
	theSync.SyncString(mName);
	theSync.SyncLong(mUseSeq);
	theSync.SyncLong(mUserId);
}

// 0x426980
void PlayerInfo::SyncDetails(DataSync& theSync)
{
	if (theSync.IsReading())
		Reset(false);

	int aVersion = PLAYER_DETAILS_VERSION;
	theSync.SyncLong(aVersion);
	if (aVersion < OLDEST_PLAYER_DETAILS_VERSION || aVersion > PLAYER_DETAILS_VERSION)
		return;

	theSync.SyncLong(mMaxStage);
	theSync.SyncLong(mMaxLevel);
	theSync.SyncLong(mLevelSelectStage);
	theSync.SyncLong(mAdventureStage);
	theSync.SyncLong(mAdventureLevel);
	theSync.SyncLong(mSelCharacter);
	theSync.SyncLong(mSelCharacter2);
	theSync.SyncString(mUnknownB4);
	theSync.SyncLong(mUnknown38);
	theSync.SyncLongSet(mUnknownFC);
	theSync.SyncLongSet(mUnknownF0);
	theSync.SyncLongMap(mUnknown108);
	theSync.SyncString(mUnknownD0);
	theSync.SyncBoolBit(mUnknownED);
	theSync.SyncBoolBit(mComputerPlayer);
	theSync.SyncBoolBit(mUnknownEE);
	theSync.SyncBoolBit(mColorblind);
	theSync.SyncBoolBit(mAdventureIntroPending);
	theSync.SyncBoolBit(mUnknownEF);
	theSync.EndBit();
	theSync.SyncLong(mComputerSkill);
	theSync.SyncByte(mUnknown54);
	theSync.SyncByte(mUnknown124);
	theSync.SyncByte(mUnknown128);
	theSync.SyncStringSet(mUnknown118);
	theSync.SyncLong(mAdventuresBeaten);
	theSync.SyncByte(mUnknown58);
	theSync.SyncLong(mTipsSeen);
	theSync.SyncLong(mUnknown204);
	theSync.SyncLong(mUnknown208);
	theSync.SyncLong(mAdventureLostBalls);
	mTotalStats.SyncState(theSync);
	mAdventureStats.SyncState(theSync);
	if (aVersion > 0x14)
	{
		theSync.SyncShort(mTipCount);
		for (int i = 0; i < NUM_TIP_KINDS; i++)
			theSync.SyncByte(mTipCounts[i]);
	}
	if (aVersion > 0x15)
		theSync.SyncShort(mOtherTipCount);
	if (aVersion > 0x16)
		theSync.SyncShort(mUnknown114);
}

// 0x426bd0
void PlayerInfo::LoadDetails()
{
	char aName[64];
	snprintf(aName, sizeof(aName), "userdata/user%d.dat", mUserId);
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(aName, &aBuffer))
		return;
	DataSync aSync;
	aSync.StartReadMemory(aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	try
	{
		SyncDetails(aSync);
	}
	catch (DataSyncException&)
	{
		// a file cut short keeps what was read up to there
	}
}

// 0x426d60
void PlayerInfo::SaveDetails()
{
	mDirty = false;
	DataSync aSync;
	aSync.StartWriteMemory();
	SyncDetails(aSync);
	char aName[64];
	snprintf(aName, sizeof(aName), "userdata/user%d.dat", mUserId);
	gSexyAppBase->WriteBytesToFile(aName, aSync.GetWrittenData().data(), (unsigned long)aSync.GetWrittenData().size());
}

// 0x426f00
void PlayerInfo::SaveIfDirty()
{
	if (mDirty)
		SaveDetails();
}

// 0x41f860
void PlayerInfo::DeleteUserFiles()
{
	char aName[64];
	snprintf(aName, sizeof(aName), "userdata/user%d.dat", mUserId);
	gSexyAppBase->EraseFile(aName);
	DeleteSavedGames();
}

// 0x412680
std::string PlayerInfo::GetSavedGameName(int theKind) const
{
	const char* aFormat = nullptr;
	switch (theKind)
	{
	case SAVED_GAME_STAGE: aFormat = "userdata/stage%d.sav"; break;
	case SAVED_GAME_ARCADE: aFormat = "userdata/arcade%d.sav"; break;
	case SAVED_GAME_DUEL: aFormat = "userdata/duel%d.sav"; break;
	case SAVED_GAME_TROPHY: aFormat = "userdata/trophy%d.sav"; break;
	default: return std::string();
	}
	char aName[64];
	snprintf(aName, sizeof(aName), aFormat, mUserId);
	return aName;
}

// 0x412940
void PlayerInfo::DeleteSavedGame(int theKind)
{
	std::string aName = GetSavedGameName(theKind);
	if (!aName.empty())
		gSexyAppBase->EraseFile(aName);
}

// 0x4129c0
void PlayerInfo::DeleteSavedGames()
{
	DeleteSavedGame(SAVED_GAME_STAGE);
	DeleteSavedGame(SAVED_GAME_ARCADE);
	DeleteSavedGame(SAVED_GAME_TROPHY);
	DeleteSavedGame(SAVED_GAME_DUEL);
}

// 0x41f820
void PlayerInfo::RestartAdventure()
{
	mUnknown204 = 0;
	mUnknown208 = 0;
	mAdventureLostBalls = 0;
	mAdventureStage = 0;
	mAdventureLevel = 0;
	mAdventureIntroPending = true;
	DeleteSavedGame(SAVED_GAME_STAGE);
	mAdventureStats.Reset();
	mDirty = true;
}

// 0x4036b0
bool PlayerInfo::IsLevelUnlocked(int theStage, int theLevel) const
{
	if (mMaxStage < theStage)
		return false;
	if (theStage < mMaxStage)
		return true;
	return theLevel < mMaxLevel;
}

// 0x4037e0
int PlayerInfo::GetMaxLevel() const
{
	return mMaxStage * 5 + mMaxLevel;
}

// 0x4037f0
int PlayerInfo::GetAdventureEfficiencyBonus() const
{
	int aBonus = ADVENTURE_BONUS_MAXIMUM - ADVENTURE_BONUS_LOST_BALL_COST * mAdventureLostBalls;
	return aBonus >= ADVENTURE_BONUS_MINIMUM ? aBonus : ADVENTURE_BONUS_MINIMUM;
}

// 0x403840 (the original adds the stats to both the totals and the adventure's)
void PlayerInfo::AddTotalStats(const GameStats& theStats)
{
	mTotalStats.Add(theStats);
	mAdventureStats.Add(theStats);
	mDirty = true;
}

// 0x403870
bool PlayerInfo::JustBeatAdventure() const
{
	return mAdventuresBeaten > 0 && mAdventureStage == 11 && mAdventureLevel == 0 && mUnknown208 == 0;
}

// 0x4038a0
bool PlayerInfo::JustStartingAdventure() const
{
	return mAdventureStage < 1 && mAdventureLevel == 0 && mUnknown208 == 0;
}

// 0x4036e0
void PlayerInfo::SetColorblind(bool theColorblind)
{
	if (theColorblind != mColorblind)
	{
		mColorblind = theColorblind;
		mDirty = true;
	}
}

// 0x403700
void PlayerInfo::SetSelCharacter(int theCharacter)
{
	if (mSelCharacter != theCharacter)
	{
		mSelCharacter = theCharacter;
		mDirty = true;
	}
}

// 0x403720
void PlayerInfo::SetSelCharacters(int theFirst, int theSecond)
{
	if (mSelCharacter != theFirst)
	{
		mSelCharacter = theFirst;
		mDirty = true;
	}
	if (mSelCharacter2 != theSecond)
	{
		mSelCharacter2 = theSecond;
		mDirty = true;
	}
}

// 0x403750
void PlayerInfo::SetComputerPlayer(bool theComputerPlayer)
{
	if (mComputerPlayer != theComputerPlayer)
	{
		mComputerPlayer = theComputerPlayer;
		mDirty = true;
	}
}

// 0x403770
void PlayerInfo::SetComputerSkill(int theSkill)
{
	if (mComputerSkill != theSkill)
	{
		mComputerSkill = theSkill;
		mDirty = true;
	}
}

// 0x403790
void PlayerInfo::SetLevelSelectStage(int theStage)
{
	if (mLevelSelectStage != theStage)
	{
		mLevelSelectStage = theStage;
		mDirty = true;
	}
}

// 0x4037b0
void PlayerInfo::IncTip(int theTip)
{
	mTipCount++;
	if ((unsigned int)theTip < NUM_TIP_KINDS)
		mTipCounts[theTip]++;
	else
		mOtherTipCount++;
	mDirty = true;
}

// 0x4038c0
bool PlayerInfo::SetTip(int theTip)
{
	unsigned int aBit = 1u << (theTip & 31);
	if (mTipsSeen & aBit)
		return false;
	mTipsSeen |= aBit;
	mDirty = true;
	return true;
}

// 0x4038f0
bool PlayerInfo::IsTipSet(int theTip) const
{
	return (mTipsSeen & (1u << (theTip & 31))) != 0;
}
