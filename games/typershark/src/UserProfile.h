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
//   0x41f1d0   the test of app+0x470 (registered), taken as true
//   STL instances and string helpers of the original, replaced by the standard library: 0x4201a0 (copy of a
//   vector<StatRecord>), 0x4213f0 (its push_back), 0x413760 / 0x413790 / 0x41de70 / 0x420300 / 0x41e110 (its helpers),
//   0x414d20 (its clear), 0x421310 (vector<T*>::push_back), 0x421380 (vector<int>::push_back), 0x4172b0 and 0x4163e0
//   (SexyString from an int: "%d"), 0x413da0 (SexyString to an int: atol), 0x4165f0 (SexyString assignment),
//   0x416290 / 0x416310 (SexyString constructors), 0x417140 (the destructor of one stat group)
//
// The class has more members that the screens use; the ones whose role is not known keep the name mField<offset>.

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
	int						mField0;			// +0x00 the best of the value that 0x412720 sets
	int						mBestScore;			// +0x04 the best score of the mode (0x412750)
	int						mField8;			// +0x08 the last value that 0x412720 set
	int						mLastScore;			// +0x0c the last score of the mode (0x412750)
	std::vector<StatRecord>	mRecords;			// +0x10 (pad) +0x14 first, +0x18 last, +0x1c end: 35 records
};

class UserProfile
{
public:
	WinWSApp*				mApp;					// +0x00
	int						mLastScore;				// +0x04 score of the last game (0x412750); the score table takes it
	int						mLastMode;				// +0x08 mode of the last game; -1 for the abyss mode (group 5)
	int						mField0c;				// +0x0c third value of 0x412750; the score table keeps it
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
	int						mField64;				// +0x64 saved in the file, no other role known
	int						mField68;				// +0x68 saved in the file, no other role known
	int						mField6c[6];			// +0x6c counted by 0x412690 (with mField234)
	int						mField84[4];			// +0x84 counted by 0x4126b0 (with mField24c)
	int						mField94[19];			// +0x94 -1 at the start
	std::vector<StatRecord>	mRecordsE0;				// +0xe0 (pad) +0xe4 first, +0xe8 last, +0xec end: 19 records
	StatGroup				mGroups[6];				// +0xf0, 0x20 bytes each
	int						mField1b0;				// +0x1b0 saved game (0x412520 / 0x412580)
	int						mField1b4;				// +0x1b4 saved game
	double					mDouble1b8;				// +0x1b8 saved game; the file holds it as int(value * 100)
	int						mField1c0;				// +0x1c0 saved game
	int						mField1c4;				// +0x1c4 saved game
	int						mField1c8;				// +0x1c8 saved game
	int						mField1cc;				// +0x1cc saved game
	int						mField1d0;				// +0x1d0 saved game
	int						mField1d4;				// +0x1d4 saved game
	int						mField1d8;				// +0x1d8 saved game
	bool					mFlag1dc;				// +0x1dc registry SeenTutorial
	int						mField1e0;				// +0x1e0 saved in the file
	int						mField1e4;				// +0x1e4 registry AbyssBoss (-1 at the start)
	int						mField1e8;				// +0x1e8 registry NBTL
	int						mField1ec;				// +0x1ec written to the file in place of mField1b4
	bool					mSpacePauses;			// +0x1f0 registry SpacePauses
	int						mField1f4;				// +0x1f4 registry SKTR (counted with the sharks killed, 0x412610)
	int						mField1f8;				// +0x1f8 registry PKTR (counted with the piranhas killed, 0x412660)
	bool					mFlag1fc;				// +0x1fc true at the start
	bool					mShowHints;				// +0x1fd true at the start
	int						mWPM[10];				// +0x200 the last ten speeds (registry WPM<i>)
	int						mField228;				// +0x228 index of the next mWPM entry (registry LastWPMIndex)
	int						mField22c;				// +0x22c registry NGC
	bool					mFlag230;				// +0x230 registry RNGC, true at the start
	int						mField234[6];			// +0x234 registry GTT<i>
	int						mField24c[4];			// +0x24c registry SGTT<i>
	char					mField25c[0x24];		// +0x25c registry vl<i> (flags)
	int						mField280;				// +0x280 registry vd
	bool					mFlag284;				// +0x284 registry svm
	std::string				mGameVersion;			// +0x288 the game version of the data (the app's, or the file's after a load)
	std::vector<ProfilePair*>	mVector2a4;			// +0x2a4 (pad) +0x2a8 first, +0x2ac last, +0x2b0 end: the map screen's pairs
	std::vector<int>		mVector2b4;				// +0x2b4 (pad) +0x2b8 first, +0x2bc last, +0x2c0 end: the map screen's node ids

public:
	UserProfile(WinWSApp* theApp);														// 0x421f50
	UserProfile(WinWSApp* theApp, const std::string& theName, int theUserNumber, int theBestDepth, unsigned int theSeed,
		int theSharksKilled, int thePiranhasKilled, int theAbyssSharksKilled, int theAbyssPiranhasKilled);	// 0x4222d0
	~UserProfile();																		// body 0x417170

	std::string				GetName();													// 0x415060
	void					SetName(const std::string& theName);						// 0x416380
	void					SetUserNumber(int theNumber);								// 0x412380
	int						GetField9c();												// 0x412340
	int						GetField8c();												// 0x412350
	void					SetField1e8(int theValue);									// 0x412390
	void					CountKill(int theType);										// 0x4123a0
	void					AddWPM(int theSpeed);										// 0x4123f0
	int						GetKillCount(int theType);									// 0x4124d0
	void					RecordResultWithDouble(bool theFlag, int theField1b0, int theField1b4, double theDouble1b8,
								int theField1c0, int theField1c4, int theField1c8);		// 0x412520
	void					RecordResult(bool theFlag, int theField1b0, int theField1b4, int theField1c0, int theField1c4,
								int theField1c8);										// 0x412580
	int						GetBestScore();												// 0x4125d0
	void					CountSharkKilled(bool theAbyss);							// 0x412610
	void					SetField1f4(int theValue);									// 0x412640
	void					SetField1f8(int theValue);									// 0x412650
	void					CountPiranhaKilled(bool theAbyss);							// 0x412660
	void					CountField6c(int theIndex);									// 0x412690
	void					CountField84(int theIndex);									// 0x4126b0
	int						GetField1e4();												// 0x4126d0
	void					SetField1e4(int theValue);									// 0x4126e0
	void					ClearFields1cc();											// 0x4126f0
	void					SetGroupField8(int theGroup, int theValue);					// 0x412720
	void					SetLastGame(int theMode, int theScore, int theField0c);	// 0x412750
	void					SetAdValue(int theIndex, double theValue);					// 0x416970
	double					GetAdValue(int theIndex);									// 0x416c00
	void					SetSpacePauses(bool theValue);								// 0x416ed0
	void					EnsureUsersUserValue();										// port: the first lines of 0x416970, 0x416c00, 0x416ed0 and 0x41f1d0
	void					WriteDataToDisk(bool theCopyField1b4);						// 0x4172f0
	bool					LoadDataFromDisk();											// 0x422630

private:
	void					InitDefaults();
};

}

#endif
