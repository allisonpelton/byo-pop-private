#pragma once

// Profile.h: the player profiles of Bejeweled 2 ("users/<name>/profile.dat") and the roster of their names.
// Original: class Sexy::Profile (sizeof 0x94), one object that WinDMApp owns (mProfile @0xe74): it holds the roster
// of every profile name (+0x0) and the data of the profile that is loaded (+0x10 on).
// Offsets in the comments are the original's.

#include <map>
#include <string>
#include <vector>

namespace Sexy
{

// A puzzle pack's progress in a profile (the value of the original's map entries, from +0x1c on).
struct PuzzleProgress
{
	std::vector<unsigned char> mSolved;  // +0x20 (vector<byte>) one byte per puzzle of the pack, as saved
	short mPuzzleNum;                    // +0x2c the puzzle the player is on
	PuzzleProgress() : mPuzzleNum(0) {}
};

class Profile
{
public:
	enum
	{
		PROFILE_MAGIC = 0x691beef4,   // first long of profile.dat
		PROFILE_VERSION = 3,          // second long; a file with a lower version is refused
		NUM_STATS = 8,
		NUM_MODE_LEVELS = 4
	};

	std::vector<std::string> mNames;                    // +0x00 every profile name (the folders of users/)
	std::string mName;                                  // +0x10 the loaded profile ("" when none)
	std::map<std::string, PuzzleProgress> mProgress;    // +0x2c puzzle pack name -> progress
	std::string mPuzzlePack;                            // +0x38 the pack the player is on ("Galaxy One")
	bool mIsNew;                                        // +0x54 just created by CreateProfile
	int mStat[NUM_STATS];                               // +0x58..+0x74 counters the Board's end of game updates
	bool mFlag78;                                       // +0x78 the Options dialog's checkbox 0x1ac sets it
	int mTotal7C;                                       // +0x7c counter that unlocks mode 6 at 80
	int mModeLevel[NUM_MODE_LEVELS];                    // +0x80..+0x8c counters that unlock modes 4, 5 and 7 (at 17, 8, 280)
	unsigned int mHints;                                // +0x90 bit n set: hint n is turned off (DisableHint)

public:
	Profile();

	void ReadProfileList();                             // 0x5d07cd users/ folders -> mNames
	bool LoadProfile(const std::string& theName);       // 0x5d0bd1 false (and the name leaves the roster) when the file fails
	bool LoadProfileHelper(const std::string& theName); // 0x5d08cb
	bool WriteProfile();                                // 0x5cfb45 false when no profile is loaded
	void WriteProfileList() {}                          // 0x5cf3aa (empty in the original)
	bool CreateProfile(const std::string& theName);     // 0x5d0d76
	bool RenameProfile(const std::string& theOldName, const std::string& theNewName); // 0x5d0bf8
	void DeleteProfile(const std::string& theName);     // 0x5d020e
	bool GetAnyProfile();                               // 0x5d0eae loads the first profile of the roster that loads
	bool ListAddName(const std::string& theName);       // 0x5d075d false when the roster has it (any case)
	bool ListRemoveName(const std::string& theName);    // 0x5d017c
	PuzzleProgress* GetPuzzleProgress();                // 0x5d0df3 the current pack's entry (made when missing)
	int GetPuzzleNum();                                 // 0x5d0ee6
	bool WantsHint(int theHint) const;                  // 0x5cf3ae
	void DisableHint(int theHint);                      // 0x5cf3cc
	bool IsModeUnlocked(int theMode) const;             // 0x5cf3e3
};

}  // namespace Sexy
