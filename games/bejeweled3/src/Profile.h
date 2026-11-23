// Sexy::Profile (vtable 0x8665bc (bejeweled3), 0x2f8 bytes, made by 0x71b530): the players of the game and the loaded
// player's saved progress. users\users.dat lists the players; users\<name>\profile.dat holds one player's progress.
// The fields' comments start with their offsets in the original's object. Most of a player's progress is read and
// written whole (every field of the file, in the file's order) and read by the game's modes, which are translated
// later; those fields are named by what the file and the code that reads them show of them. Steam's achievements
// and stats (0x71c000, 0x71a700) are left out.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Sexy {

class Buffer;

class Profile {
public:
	// What profile.dat holds, by how many (0x71cd20)
	enum { STAT_COUNT = 38, COUNTER_COUNT = 5, BADGE_COUNT = 20, FLAG_ROW_COUNT = 5, FLAG_ROW_SIZE = 8,
		TUTORIAL_FLAG_COUNT = 4, RANK_FLAG_COUNT = 5, HINT_FLAG_COUNT = 0x28, USERS_BLOCK_SIZE = 8 };
	// The file's marker and the newest version it is written in; older than 0x47 is not read
	enum { FILE_MAGIC = 0x391beef5, FILE_VERSION = 0x47, MIN_FILE_VERSION = 0x47, USERS_FILE_VERSION = 8 };

	// users.dat
	std::wstring mName;                     // +0x60: the player loaded (the registry's "LastUser")
	std::wstring mUsersFileSecondName;      // +0x44: the second name users.dat holds after the first (written, never read)
	uint8_t mUsersBlockFirst[USERS_BLOCK_SIZE] = {};   // +0x20: eight bytes users.dat keeps (nothing reads them)
	uint8_t mUsersBlockSecond[USERS_BLOCK_SIZE] = {};  // +0x28
	int mUsersFileCount = 0;                // +0x40: a number users.dat keeps (nothing reads it)
	uint8_t mUsersBlockThird[USERS_BLOCK_SIZE] = {};   // +0x30
	uint8_t mUsersBlockFourth[USERS_BLOCK_SIZE] = {};  // +0x38
	std::vector<std::wstring> mUserNames;   // +0x14: every player, in the file's order

	// profile.dat
	uint32_t mSeenBits = 0;                 // +0x7c: one bit a thing the player has seen
	bool mTemporary = false;                // +0x80: the player's folder is the temporary one (0x71b940)
	std::wstring mProfileTextFirst;         // +0x84: text profile.dat keeps after the counters' blocks
	std::wstring mProfileTextSecond;        // +0xa0
	bool mFlagAfterText = false;            // +0xbc: a flag after the two texts
	bool mIsNew = true;                     // +0xbd: no profile.dat has been read for this player
	int mStats[STAT_COUNT] = {};            // +0xc0: lifetime counters (the badges' statistics)
	// +0x158: the Quest map's sections (rows) of eight quests each: a flag is 1 once the quest is won (0x684380 sets it, QuestMenu::Update; read by 0x673560 and the menu's
	// button loops). The quest at index n of quest.cfg is row n / 8, flag n % 8
	uint8_t mQuestsDone[FLAG_ROW_COUNT][FLAG_ROW_SIZE] = {};
	uint8_t mTutorialFlags[TUTORIAL_FLAG_COUNT] = {};       // +0x180: the tutorials the player has seen
	int mQuestSection = 0;                  // +0x184: the section of the Quest map the player was on (QuestMenu +0x14c, 0 to 4; saved from version 0x43)
	bool mQuestMenuFlag = false;            // +0x188: QuestMenu::Update sets it while the section on show is state 1 or 3 (the first open section or a finished one) and the
	                                        // menu's own flag (+0x128) is on; a click on a section or a quest clears it (0x674c90, 0x697640). Saved with the section
	int mCounters[COUNTER_COUNT] = {};      // +0x18c
	int mBadgeLevels[BADGE_COUNT] = {};     // +0x1a0: the level reached in each badge
	int mRank = 0;                          // +0x1f0: from mLifetimePoints (0x71a330)
	uint64_t mLifetimePoints = 0;           // +0x1f8
	int mRankSecond = 0;                    // +0x200: from mSecondPoints
	uint64_t mSecondPoints = 0;             // +0x208
	int mNumberAfterPoints = 0;             // +0x210
	int mNumberAfterThat = 0;               // +0x214
	bool mFlagAfterTutorials = false;         // +0x218: starts as the app's flag at +0x6cc, which the port has not
	uint32_t mHintFlags = 0;          // +0x21c: one bit for each hint box (Board::HINT_*) and help dialog the player has seen; bit 19 (0x80000) turns every box off. Saved as the long it was
	int mShortAfterNumber = 0;              // +0x220
	uint32_t mRankFlags[RANK_FLAG_COUNT] = {};  // +0x224: five bytes, one a word
	bool mOptionFirst = false;              // +0x238
	bool mOptionSecond = true;              // +0x239
	float mFraction = 0.42f;                // +0x23c: four bytes read and written as they are
	bool mTextOptionFirstOn = false;        // +0x240
	std::string mTextOptionFirst;           // +0x244
	bool mTextOptionSecondOn = false;       // +0x260
	std::string mTextOptionSecond;          // +0x264
	bool mTextOptionThirdOn = false;        // +0x280
	std::string mTextOptionThird;           // +0x284
	bool mFlagBeforeFractions = false;      // +0x2a0
	float mFractionFirst = 0.5f;            // +0x2a4
	float mFractionSecond = 0.5f;           // +0x2a8
	// +0x2ac: version 0x45 and later: one flag for each quest of quest.cfg (index 0 to 0x27) that is 1 once its help dialog (QuestHelpDialog, 0x6cd9f0) has been shown
	uint8_t mQuestHelpSeen[HINT_FLAG_COUNT] = {};
	std::map<std::wstring, std::wstring> mSettings;  // +0x2d4: name and value pairs

	Profile();                              // 0x71b530
	~Profile();                             // 0x71c4b0 (bejeweled3): the loaded player is saved
	// 0x71b1e0: every field of a player back to the state of a new one
	void ResetPlayer();
	// 0x71d410: reads users\users.dat and the files of the players it lists (a player whose file cannot be read is
	// dropped); false when there is no users.dat
	void LoadUsers();
	// 0x71c270: writes the registry's "LastUser" and users\users.dat
	void SaveUsers();
	// 0x71cd20: reads users\<theName>\profile.dat into the player; false when there is none or it is too old
	bool LoadPlayer(const std::wstring& theName);
	// 0x71bb10: writes the loaded player's users\<name>\profile.dat
	void SavePlayer();
	// 0x71d7c0: loads theName and makes it the player (the previous one is saved first); a player whose file cannot be
	// read is dropped from the list
	bool SelectUser(const std::wstring& theName);
	// 0x71d800: the first player of the list who can be loaded
	bool SelectFirstLoadableUser();
	// 0x71c820: makes theName a new player and the one loaded (the one loaded is saved first, a player's file kept as it
	// is); false, with the "SAME NAME" message shown, when a player of that name is already there. Writes users.dat
	// (with "LastUser" in the registry) and makes the player's folder
	bool CreateProfile(const std::wstring& theName);
	// 0x71a460: takes theName out of the list; true when it was there
	bool RemoveUserName(const std::wstring& theName);
	// 0x71b880: puts theName at the end of the list unless a name equal to it, in any case, is there
	bool AddUserName(const std::wstring& theName);
	// 0x71b6b0: users\<theName> with every character of the name below 'A' other than space and the digits turned to
	// '_' (users\_temp for an empty name)
	static std::string GetUserFolder(const std::wstring& theName);
	// 0x71a330: how many steps of 150000, 250000, 350000 ... points fit in thePoints
	static int GetRankForPoints(uint64_t thePoints);

};

}  // namespace Sexy
