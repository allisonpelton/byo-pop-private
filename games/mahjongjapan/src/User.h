#ifndef __MAHJONGJAPAN_USER_H__
#define __MAHJONGJAPAN_USER_H__

// The player (0x415eb0..0x416490, vtable 0x51d8e8, the global at 0x532e38): the settings and the records, saved as
// <user folder><id>.usr, and the names of the saved games, <id>_<level>.sav for the list and <id>_<level>_D.sav for the
// escape.

#include "Stream.h"

#include <string>

namespace mahjongjapan
{

// 0x415eb0
class User : public jb::Streamable
{
public:
	int mId = -1;                  // +0x04 (0x532e3c)
	std::string mName;             // +0x08
	bool mAutoHint;                // +0x10 (0x532e48) a hint after 30 s without a click
	bool mShadeCovered;            // +0x11 (0x532e49) the tiles that cannot be taken are shaded
	int mSelectedLayout;           // +0x14 (0x532e4c) the layout chosen in the list
	int mQuadPercent;              // +0x18 (0x532e50) the share of fours in a deal
	int mStyleFree;                // +0x1c (0x532e54) the list's tile set (0-5)
	int mBackground;               // +0x20 (0x532e58) the list's background, or -1 for each in turn
	bool mFreeTipShown;            // +0x24 (0x532e5c)
	int mBestTimes[175];           // +0x28 (0x532e60) seconds, 36000 for none
	int mBestScores[175];          // +0x2e4 (0x53311c)
	int mProgress;                 // +0x5a0 (0x5333d8) the escape's levels won
	int mStyleEscape;              // +0x5a4 (0x5333dc)
	int mEscBestTimes[60];         // +0x5a8 (0x5333e0)
	int mEscBestScores[60];        // +0x698 (0x5334d0)
	bool mShowQuitNote;            // +0x788 (0x5335c0) the note on leaving the escape
	int mMagic = 0;                // +0x78c 'ME'
	int mVersion = 0;              // +0x790

	// 0x415eb0
	User() { Reset(-1); }
	// 0x415f10, 0x415f30
	~User() override {}

	// 0x415f40
	void Reset(int theId);
	// 0x415fc0: the CRC takes in the exe's folder too
	void Serialize(jb::Stream& theStream) override;
	// 0x416100: a new player theName when the file is missing or bad
	void Load(int theId, const char* theName);
	// 0x4161f0
	void Save();
	// 0x4162a0
	static void GetUserPath(int theId, std::string& thePath);
	// 0x416310: the pattern of the player's saved games
	static void GetSavePattern(int theId, std::string& thePattern, std::string& theFolder);
	// 0x416390, 0x416410
	void GetFreeSave(int theLevel, std::string& thePath);
	void GetEscapeSave(int theLevel, std::string& thePath);
};

extern User gUser;  // 0x532e38

} // namespace mahjongjapan

#endif
