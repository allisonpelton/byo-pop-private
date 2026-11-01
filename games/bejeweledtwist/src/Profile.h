// The players (0x21c bytes, no virtuals; 0x7333e5-0x736463, app +0x7ec): the list of names in users\users.dat and
// the current player, whose stats, settings and games in progress are in users\<name>\profile.dat. A name that is
// not a valid file name is stored under a folder name with '_' for its odd characters (0x733e9a).
//
// The fields keep the original's offsets in their comments; those whose meaning is not yet known are named by them.
#pragma once

#include "Buffer.h"
#include "Common.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class Profile {
public:
	// A game in progress (0x2c bytes)
	struct SavedGame {
		Buffer mData;      // +0x00
		int mUnknown20;    // +0x20: saved as a short
		int mUnknown24;    // +0x24: saved as a short
		int mUnknown28;    // +0x28

		void Read(const Buffer& theBuffer);   // 0x6a3156
		void Write(Buffer& theBuffer) const;  // 0x6a3192
	};

	std::vector<SexyString> mNames;   // +0x00 (first at +0x04): every player, in users.dat's order
	SexyString mName;                 // +0x10: the current player
	std::map<int, int> mMap2C;        // +0x2c: emptied when a player is loaded (its use is not yet known)
	std::string mString38;            // +0x38
	bool mIsNew = false;              // +0x54: made this session, not loaded
	int mUnknown58 = 0;               // +0x58
	int mUnknown5C[6] = {};           // +0x5c
	int mUnknown74 = 0;               // +0x74
	int mUnknown78 = 0;               // +0x78
	std::vector<bool> mBools80;       // +0x80
	int mUnknown8C = 0;               // +0x8c
	int mUnknown90 = 0;               // +0x90
	int mUnknown94 = 0;               // +0x94
	int mUnknown98 = 0;               // +0x98
	bool mCustomCursors = false;      // +0x9c: the app's custom cursors (SetCustomCursorsEnabled)
	SavedGame mSavedGames[3];         // +0xa0
	int mUnknown124 = 0;              // +0x124
	int mUnknown128[6] = {};          // +0x128
	int mUnknown140[6] = {};          // +0x140
	int mUnknown158 = 0;              // +0x158
	int mUnknown15C = 0;              // +0x15c
	int mUnknown160 = 0;              // +0x160
	int mUnknown164 = 0;              // +0x164
	int mUnknown168 = 0;              // +0x168
	int mUnknown16C = 0;              // +0x16c
	unsigned char mBits170[0x40] = {};  // +0x170: 512 flags, the first in each byte's top bit
	bool mUnknown1B0 = false;         // +0x1b0
	bool mUnknown1B1 = false;         // +0x1b1
	int mUnknown1B4[10] = {};         // +0x1b4
	int mUnknown1DC = 0;              // +0x1dc
	unsigned int mFlags1E0 = 0;       // +0x1e0
	unsigned int mFlags1E4 = 0;       // +0x1e4
	unsigned int mFlags1E8 = 0;       // +0x1e8
	int mUnknown1EC = 0;              // +0x1ec
	int mUnknown1F0 = 0;              // +0x1f0
	std::vector<int> mInts1F4;        // +0x1f4: saved as shorts
	std::vector<int> mInts204;        // +0x204: as many as mInts1F4
	int mUnknown214 = 0;              // +0x214
	int mUnknown218 = 0;              // +0x218

	Profile();   // 0x734ef5
	~Profile();  // 0x734da6

	// 0x735a09: reads users\users.dat into the list, checking each player's file; a player whose file does not load is
	// dropped (and its folder deleted). theLastName gets the name of the player who played last.
	void LoadUsers(SexyString* theLastName);
	void SaveUsers();                               // 0x733f07
	bool Load(const SexyString& theName);           // 0x735001: makes theName the current player
	void Save();                                    // 0x734087: the current player's profile.dat
	bool Select(const SexyString& theName);         // 0x7354f8: saves the current player, then loads theName
	bool SelectAny();                               // 0x7359c0: the first player that loads
	// 0x73561d: a new player of that name (or, if the name is taken, the game's "same name" dialog)
	void Create(const SexyString& theName);
	bool AddName(const SexyString& theName);        // 0x734f91: false when the name is already in the list
	bool RemoveName(const SexyString& theName);     // 0x73485a
	void Delete(const SexyString& theName);         // 0x7348ec: the player and their folder
	void ClearMap();                                // 0x734a73

	static std::string GetUserFolder(const SexyString& theName);  // 0x73400c: "<app data>users\<file name>"

	bool GetFlag1E0(int theBit) const { return (mFlags1E0 & (1u << (theBit & 31))) != 0; }
	void SetFlag1E0(int theBit, bool on);           // 0x7337f8
	bool GetFlag1E4(int theBit) const { return (mFlags1E4 & (1u << (theBit & 31))) != 0; }  // 0x733897
	void SetFlag1E4(int theBit, bool on);           // 0x7338b6
	bool GetBit170(int theBit) const;               // 0x733838
	void SetBit170(int theBit);                     // 0x73386e
};

}  // namespace Sexy
