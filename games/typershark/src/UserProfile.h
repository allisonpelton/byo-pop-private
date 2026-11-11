#ifndef __TYPERSHARK_USERPROFILE_H__
#define __TYPERSHARK_USERPROFILE_H__

// UserProfile.cpp: one player of Typer Shark Deluxe 1.02: the name, the counters, the stat groups, the saved game and the
// map lists. Saved in two places (UserProfileMgr.cpp): the registry (Users\<name>\..., written by
// UserProfileMgr::WriteUserProfileToRegistry 0x41f1d0, read by LoadUserProfileFromRegistry 0x425390) and users\user<N>.dat
// (WriteDataToDisk 0x4172f0, LoadDataFromDisk 0x422630). The file is a Buffer of strings: every number is the text of
// "%d" (Buffer::WriteString), read back with atol.
// Original: class Sexy::UserProfile (the export has no name; the traces say UserProfile::... and UserProfileMgr::...),
// no vftable, sizeof 0x2c8; constructors 0x421f50 (a new player) and 0x4222d0 (from the registry values), destructor body
// 0x417170. The field comments give the offsets in the original's object.
//
// Not ported (left out as the brief says):
//   0x4ad5f0   the demo recording trace called at the top of 0x416ed0 and 0x4172f0 (and of the manager's methods)
//   0x41f1d0   the test of app+0x470 (registered): removed (registration)
//   STL instances and string helpers of the original, replaced by the standard library: 0x4201a0 (copy of a
//   vector<StatRecord>), 0x4213f0 (its push_back), 0x413760 / 0x413790 / 0x41de70 / 0x420300 / 0x41e110 (its helpers),
//   0x414d20 (its clear), 0x421310 (vector<T*>::push_back), 0x421380 (vector<int>::push_back), 0x4172b0 and 0x4163e0
//   (SexyString from an int: "%d"), 0x413da0 (SexyString to an int: atol), 0x4165f0 (SexyString assignment),
//   0x416290 / 0x416310 (SexyString constructors), 0x417140 (the destructor of one stat group)
//
// The class has more members that the screens use.

#include <string>
#include <vector>

namespace Sexy
{

class WinWSApp;

// the 8-byte pairs of the map screen's list (malloc'd in the original); the profile owns them
struct ProfilePair
{
	int						mFirst;
	int						mSecond;
};

// the 0x30-byte record of the profile's vectors (the stat groups' 35 records and the 19 records at +0xe0): twelve ints
struct StatRecord
{
	int						mField[12];
};

// one of the six stat groups at +0xf0 (0x20 bytes: four ints and a vector of 35 records). The group number is the game mode
// (0 to 5; 5 is the abyss mode).
struct StatGroup
{
	int						mBestLevel;			// +0x00 the best of the values that 0x412720 (RecordGroupLevel) was given: the highest level number the diver reached in the mode (the deepest depth in the abyss group); only saved in the file
	int						mBestScore;			// +0x04 the best score of the mode (0x412750)
	int						mLastLevel;			// +0x08 the last value that 0x412720 (RecordGroupLevel) was given: the level number the diver reached in the mode's last game (the depth in the abyss group); only saved in the file
	int						mLastScore;			// +0x0c the last score of the mode (0x412750)
	std::vector<StatRecord>	mRecords;			// +0x10 (pad) +0x14 first, +0x18 last, +0x1c end: 35 records
};

class UserProfile
{
public:
	WinWSApp*				mApp;					// +0x00
	int						mLastScore;				// +0x04 score of the last game (0x412750); the score table takes it
	int						mLastMode;				// +0x08 mode of the last game; -1 for the abyss mode (group 5)
	int						mLastLevelReached;				// +0x0c third value of 0x412750 (SetLastGame): the level the diver was on in the last game, counted from 1; the score table keeps it with the score
	int						mLastDepth;				// +0x10 depth of the last abyss game
	std::string				mName;					// +0x14
	int						mUserNumber;			// +0x30 users\user<N>.dat
	int						mBestDepth;				// +0x34 deepest abyss depth (registry AnnoDomini)
	unsigned int			mSeed;					// +0x38 registry Seed; a new one is drawn at every load
	int						mSharksKilled;			// +0x3c NumSharksKilled
	int						mPiranhasKilled;		// +0x40 NumPiranhasKilled
	int						mKillCount[4];			// +0x44 kills by creature type 0xc, 0xe, 0xf and 0x10 (0x4123a0)
	int						mBossesKilled;			// +0x54 NumBossesKilled
	int						mAbyssSharksKilled;		// +0x58 NumAbyssSharksKilled
	int						mAbyssPiranhasKilled;	// +0x5c NumAbyssPiranhasKilled
	int						mLastLesson;			// +0x60 LastLesson
	int						mBestBonusWordPoints;				// +0x64 the most points of bonus words won in one air-tank round (the Board raises it when the air runs out outside a bonus level); saved in the file, never read
	int						mBestTreasureWords;		// +0x68 the most treasure words of any game (the board raises it); the stats screen's "Highest number of treasure words"
	int						mGemsFound[6];			// +0x6c how many of each of the six gems were found (CountGemFound 0x412690, with mGameGemsFound); the stats screen's "Gems found"
	int						mSecretsFound[4];		// +0x84 how many of each of the four secret items (crown, figurine, necklace, scepter) were found (CountSecretFound 0x4126b0, with mGameSecretsFound)
	int						mLessonOutcome[19];			// +0x94 per lesson (0 is the typing test): -1 until the lesson has been finished, 0 finished below its limits, 1 passed (the typing tutor sets it; the lesson list and the stats screen's tutorial page read it)
	std::vector<StatRecord>	mLessonRecords;				// +0xe0 (pad) +0xe4 first, +0xe8 last, +0xec end: 19 records, one per lesson (0 is the typing test): the typing tutor's best, last, total and run count for each measure (RecordTutorWPM and the like; the stats screen's tutorial page reads them)
	StatGroup				mGroups[6];				// +0xf0, 0x20 bytes each
	int						mSavedLives;				// +0x1b0 saved game: the diver's lives (0x412520 / 0x412580)
	int						mSavedScore;				// +0x1b4 saved game: the score (from the diver's score, or from mSavedScoreBackup when RecordResult is told the game ended)
	double					mSavedEmpCharge;				// +0x1b8 saved game: the diver's EMP charge (a double; the file holds it as int(value * 100))
	int						mSavedLevel;				// +0x1c0 saved game: the level in play (1 to 0x23 in an adventure game, 100 plus the endless levels played in the endless play, 0 in the abyss mode); -1 or 0 means there is nothing to continue; RecordResult stores it, ButtonDepress's CONTINUE? reads it
	int						mSavedDifficulty;				// +0x1c4 saved game: the difficulty (0 easy to 4 xtreme); RecordResult stores nothing when it is -1, a lost game stores -10; ButtonDepress puts it back in the app's mDifficulty
	int						mSavedLevelNumber;				// +0x1c8 saved game: the diver's level number (Player::mLevelNumber, counted from 0); a continued game's map shows it plus one
	int						mSavedGameTime;				// +0x1cc saved game: the play time of the finished levels (the app's mGameTimeSoFar)
	int						mSavedPausedTime;				// +0x1d0 saved game: the paused time of the finished levels (the app's mPausedTimeSoFar)
	int						mSavedLettersWrong;				// +0x1d4 saved game: the letters typed wrong in the finished levels (the app's mLettersWrongSoFar)
	int						mSavedLettersRight;				// +0x1d8 saved game: the letters typed right in the finished levels (the app's mLettersRightSoFar)
	bool					mSeenTutorial;				// +0x1dc registry SeenTutorial: the player has been through the first level's opening tutorial (the Board sets it when the first wave is cleared; the Board's hints and its title fade read it)
	int						mSavedTypingTime;				// +0x1e0 saved in the file: the typing time of the finished levels (the app's mTypingTimeSoFar)
	int						mAbyssBossIndex;				// +0x1e4 registry AbyssBoss (-1 at the start): which abyss boss is called for next; the Board takes it at its first update and saves it back as it counts up (GetAbyssBossIndex, SetAbyssBossIndex)
	int						mEndlessLevelsPlayed;				// +0x1e8 registry NBTL: the endless levels played before the one in play (the lesson manager's mEndlessCount minus one, stored by SaveGameProgress); ButtonDepress gives it back to mEndlessCount when the saved game continues
	int						mSavedScoreBackup;				// +0x1ec the copy of mSavedScore that WriteDataToDisk takes at each save that does not record a score; RecordResult puts it back in mSavedScore when it is told the game ended, and the file holds it in place of mSavedScore
	bool					mSpacePauses;			// +0x1f0 registry SpacePauses
	int						mGameSharksKilled;				// +0x1f4 registry SKTR: the sharks killed in the current game (CountSharkKilled adds one with mSharksKilled, 0x412610; a game's start sets it to 0; the win dialog shows it)
	int						mGamePiranhasKilled;				// +0x1f8 registry PKTR: the piranhas killed in the current game (CountPiranhaKilled adds one with mPiranhasKilled, 0x412660; a game's start sets it to 0; the win dialog shows it)
	bool					mHintBoxesWanted;				// +0x1fc true at the start: the player has not ticked "do not show hints" in the hint box; the app copies it to mHintsAllowed when the player is chosen, made or closes a hint box
	bool					mShowHints;				// +0x1fd true at the start
	int						mWPM[10];				// +0x200 the last ten speeds (registry WPM<i>)
	int						mNextWPMIndex;				// +0x228 index in mWPM of the entry AddWPM writes next, back to 0 after 9 (registry LastWPMIndex)
	int						mGamesCompleted;		// +0x22c registry NGC: finished adventure games ("expeditions completed"; 1-2 bronze, 3-4 silver, 5 or more gold medal)
	bool					mCompletionPending;				// +0x230 registry RNGC, true at the start of a game (ButtonDepress and RestartBoard set it): the game's finish has not been counted yet; the map's final scroll adds one to mGamesCompleted only while it is set and then clears it
	int						mGameGemsFound[6];			// +0x234 registry GTT<i>: how many of each of the six gems were found in the current game (CountGemFound adds one with mGemsFound; a game's start sets them to 0; the win dialog shows them)
	int						mGameSecretsFound[4];			// +0x24c registry SGTT<i>: how many of each of the four secret items were found in the current game (CountSecretFound adds one with mSecretsFound; a game's start sets them to 0; the win dialog shows them)
	char					mVisitedLevels[0x24];		// +0x25c registry vl<i>: one flag for each of the 0x24 levels, set when the level is started or its treasure found; the map draws that site's chest open and offers its hint; cleared when a game starts at another difficulty than mVisitedDifficulty
	int						mVisitedDifficulty;				// +0x280 registry vd: the difficulty that the mVisitedLevels flags belong to; a game that starts at another difficulty sets it and clears the flags
	bool					mOpenChestHintSeen;				// +0x284 registry svm: the map's hint about open treasure chests has been shown (set once by the map's Update, which then writes the registry; the map's SetVisible reads it)
	std::string				mGameVersion;			// +0x288 the game version of the data (the app's, or the file's after a load)
	std::vector<ProfilePair*>	mSavedRouteDots;			// +0x2a4 (pad) +0x2a8 first, +0x2ac last, +0x2b0 end: the map screen's route dots (mRouteDots) of the saved game; CopyMapListsToUser fills them, the data file writes and reads them, ButtonDepress copies them back to the map screen when the saved game continues
	std::vector<int>		mSavedRouteNodeIds;				// +0x2b4 (pad) +0x2b8 first, +0x2bc last, +0x2c0 end: the ids of the sites the map screen's ship has sailed through (mRouteNodes) in the saved game; CopyMapListsToUser fills them, the data file writes and reads them, ButtonDepress turns them back into sites when the saved game continues

public:
	UserProfile(WinWSApp* theApp);														// 0x421f50
	UserProfile(WinWSApp* theApp, const std::string& theName, int theUserNumber, int theBestDepth, unsigned int theSeed,
		int theSharksKilled, int thePiranhasKilled, int theAbyssSharksKilled, int theAbyssPiranhasKilled);	// 0x4222d0
	~UserProfile();																		// body 0x417170

	std::string				GetName();													// 0x415060
	void					SetName(const std::string& theName);						// 0x416380
	void					SetUserNumber(int theNumber);								// 0x412380
	int						GetLessonTwoOutcome();												// 0x412340 returns mLessonOutcome[2]; nothing calls it (the original's function at this address is LessonManager's GetCurrentWaveType)
	int						GetNecklacesFound();												// 0x412350 returns mSecretsFound[2], how many necklaces were found; nothing calls it (the original's function at this address is LessonManager's GetLevel)
	void					SetEndlessLevelsPlayed(int theValue);									// 0x412390 sets mEndlessLevelsPlayed (SaveGameProgress passes the endless levels played before the one in play; the registry load passes NBTL)
	void					CountKill(int theType);										// 0x4123a0
	void					AddWPM(int theSpeed);										// 0x4123f0
	int						GetKillCount(int theType);									// 0x4124d0
	void					RecordResultWithDouble(bool theStoreScore, int theSavedLives, int theSavedScore, double theSavedEmpCharge,
								int theSavedLevel, int theSavedDifficulty, int theSavedLevelNumber);		// 0x412520 the saved game's level, difficulty and level number
	void					RecordResult(bool theStoreScore, int theSavedLives, int theSavedScore, int theSavedLevel, int theSavedDifficulty,
								int theSavedLevelNumber);										// 0x412580
	int						GetBestScore();												// 0x4125d0
	// ---- added for W1 (the typing-test graphs of the tutorial page of the stats screen) ----
	int						GetAverageWPM(int theLesson);								// 0x413d10 the lesson's total words per minute over its runs (0 when it has none)
	int						GetAverageAdjustedWPM(int theLesson);						// 0x413d40 the same for the adjusted words per minute
	int						GetAverageAccuracy(int theLesson);							// 0x413d70 the same for the accuracy
	// ---- added for T1 (the typing tutor records a finished lesson; defined in TypingTutorLessons.cpp) ----
	void					RecordTutorWPM(int theValue, int theLesson);				// 0x413bf0 records[theLesson].mField[0..3]: best, last, total, count of the raw words per minute
	void					RecordTutorAdjustedWPM(int theValue, int theLesson);		// 0x413c50 mField[4..7]: the same for the adjusted words per minute
	void					RecordTutorAccuracy(int theValue, int theLesson);			// 0x413cb0 mField[8..11]: the same for the accuracy
	void					CountSharkKilled(bool theAbyss);							// 0x412610
	void					SetGameSharksKilled(int theValue);									// 0x412640 sets mGameSharksKilled (0 at a game's start; the registry load passes SKTR)
	void					SetGamePiranhasKilled(int theValue);									// 0x412650 sets mGamePiranhasKilled (0 at a game's start; the registry load passes PKTR)
	void					CountPiranhaKilled(bool theAbyss);							// 0x412660
	void					CountGemFound(int theIndex);									// 0x412690
	void					CountSecretFound(int theIndex);									// 0x4126b0
	int						GetAbyssBossIndex();												// 0x4126d0 which abyss boss is called for next
	void					SetAbyssBossIndex(int theValue);									// 0x4126e0 sets which abyss boss is called for next (the Board counts it up and saves it; an abyss game's start sets it from the chosen depth; the registry load passes AbyssBoss)
	void					ClearSavedGameTotals();											// 0x4126f0 zeroes the saved game's play time, paused time, letters wrong, letters right and typing time (a lost game clears them)
	void					RecordGroupLevel(int theGroup, int theValue);					// 0x412720 sets the group's last level to the level number the diver reached (the depth in the abyss group) and raises its best level when it is higher
	void					SetLastGame(int theMode, int theScore, int theLevelReached);	// 0x412750
	void					SetAdValue(int theIndex, double theValue);					// 0x416970
	double					GetAdValue(int theIndex);									// 0x416c00
	void					SetSpacePauses(bool theValue);								// 0x416ed0
	void					EnsureUsersUserValue();										// port: the first lines of 0x416970, 0x416c00, 0x416ed0 and 0x41f1d0
	void					WriteDataToDisk(bool theCopyScoreToBackup);						// 0x4172f0
	bool					LoadDataFromDisk();											// 0x422630

private:
	void					InitDefaults();
};

}

#endif
