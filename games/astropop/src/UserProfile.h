#pragma once

// UserProfile: one player of AstroPop Deluxe (class of 0x64 bytes, constructor 0x42dc60, destructor body 0x42cbc0).
// A profile is a set of numbered settings with three kinds (0 int, 1 float, 2 string; ids 0..0x73, the kind of each
// from the constructor's table), two lists of 6-int records, and a table of 20 x 10 (int, int) pairs. It lives in the
// registry (Users\<name>\<id>, written by WriteRegistry 0x425b30, read by ReadRegistry 0x427200) and in the player's
// file users\data<number>.usr (Save 0x4276b0, Load 0x42eca0).
//
// The three setting stores are std::map<int,int|float|string> (original +0x48, +0x30, +0x3c) and the kind table is
// +0x54; the original's vector of vectors is +0x04, the two record lists +0x14 and +0x24 (+0x10, +0x20 are the
// vectors' allocator slots).

#include <map>
#include <string>
#include <vector>

namespace Sexy
{

enum
{
	NUM_PROFILE_SETTINGS = 0x74,       // ids 0 .. 0x73
	PROFILE_NAME = 0xd,                // string: the player's name
	PROFILE_NUMBER = 0xe,              // int: the number in users\data<number>.usr
	PROFILE_VERSION = 0xf,             // string (kind 2): the profile's version (it is read as a float)
	PROFILE_UNLOCK_A = 0x30,           // int: unlock flags; always equals PROFILE_UNLOCK_B - 0x193 when untouched
	PROFILE_UNLOCK_B = 0x32            // int
};

struct ProfileScore
{
	int mValue;
	int mAux;
};

// A saved record of six ints (the original keeps them in two lists of at most five)
struct ProfileRecord
{
	int mField[6];
};

class UserProfile
{
public:
	std::vector<std::vector<ProfileScore> > mScores;   // +0x04 (20 x 10)
	std::vector<ProfileRecord> mRecordsA;               // +0x14
	std::vector<ProfileRecord> mRecordsB;               // +0x24
	std::map<int, float> mFloats;                       // +0x30
	std::map<int, std::string> mStrings;                // +0x3c
	std::map<int, int> mInts;                           // +0x48
	std::map<int, int> mKinds;                          // +0x54 setting id -> 0 int, 1 float, 2 string
	bool mLoaded;                                       // +0x60 the file or the registry gave the profile
	bool mValid;                                        // +0x61 the registry writes worked; ctor true
	bool mUnknown62;                                    // +0x62 ctor true (no reader found so far)

	UserProfile();                                      // 0x42dc60
	~UserProfile();                                     // 0x42cbc0

	bool GetInt(int theId, int* theValue) const;        // 0x40e830
	bool GetFloat(int theId, float* theValue) const;    // 0x40e870
	bool GetString(int theId, std::string* theValue) const; // 0x420f40
	void SetInt(int theId, int theValue);               // 0x4243c0
	void SetFloat(int theId, float theValue);           // 0x424400
	void SetString(int theId, const std::string& theValue); // 0x4271a0
	bool HasModeUnlocked(int theMode) const;            // 0x40e8b0 (modes 2 and 3 are bits 2 and 6 of PROFILE_UNLOCK_A)

	void ResetGame();                                   // 0x424540 (settings 0x4f .. 0x64: no saved game)
	void ClearScores();                              // 0x42dbd0 (20 x 10 zeroed pairs)
	bool Load(float* theVersion, std::string* theName, bool theSkipFile); // 0x42eca0
	bool Save();                                        // 0x4276b0 (users\data<number>.usr; false when the write fails)
	void WriteRegistry();                               // 0x425b30
	bool ReadRegistry();                                // 0x427200

	static int NextUserNumber();                        // 0x4258b0
};

}
