#ifndef __MAHJONGCHINA_USER_H__
#define __MAHJONGCHINA_USER_H__

// The player (0x415eb0..0x416490, vtable 0x51d8e8, the global at 0x532e38): the settings and the records, saved as
// <user folder><id>.usr, and the names of the saved games, <id>_<level>.sav for the list and <id>_<level>_D.sav for the
// escape.

#include "Stream.h"

#include <string>

namespace mahjongchina
{

// 0x415eb0
class User : public jb::Streamable
{
public:
	int mId = -1;                  // +0x04 (0x53229c)
	std::string mName;             // +0x08
	bool mAutoHint;                // +0x10 (0x5322a8) a hint after 30 s without a click
	bool mShadeCovered;            // +0x11 (0x5322a9) the tiles that cannot be taken are shaded
	int mSelectedLayout;           // +0x14 (0x5322ac) the layout chosen in the list
	int mQuadPercent;              // +0x18 (0x5322b0) the share of fours in a deal
	int mStyleFree;                // +0x1c (0x5322b4) the list's tile set (0-5)
	int mBackground;               // +0x20 (0x5322b8) the list's background, or -1 for each in turn
	bool mFreeTipShown;            // +0x24 (0x5322bc)
	int mBestTimes[150];           // +0x28 (0x5322c0) seconds, 36000 for none
	int mBestScores[150];          // +0x280
	int mProgress;                 // +0x4d8 (0x532770) the escape's levels won
	int mStyleEscape;              // +0x4dc (0x532774)
	int mEscBestTimes[52];         // +0x4e0
	int mEscBestScores[52];        // +0x5b0
	bool mShowQuitNote;            // +0x680 (0x532918) the note on leaving the escape
	int mMagic = 0;                // +0x684 'ME'
	int mVersion = 0;              // +0x688

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

} // namespace mahjongchina

#endif
