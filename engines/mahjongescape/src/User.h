#ifndef __MAHJONG_USER_H__
#define __MAHJONG_USER_H__

// The player (0x415920..0x415ee0 (mahjongchina), 0x415eb0..0x416490 (mahjongjapan), vtable 0x51c7d0 (mahjongchina),
// 0x51d8e8 (mahjongjapan), the global at 0x532298 (mahjongchina), 0x532e38 (mahjongjapan)): the settings and the
// records, saved as <user folder><id>.usr, and the names of the saved games, <id>_<level>.sav for the list and
// <id>_<level>_D.sav for the escape.

#include "Edition.h"
#include "Stream.h"

#include <string>

namespace mahjong
{

// 0x415920 (mahjongchina), 0x415eb0 (mahjongjapan)
class User : public jb::Streamable
{
public:
	// +0x04 (0x53229c (mahjongchina), 0x532e3c (mahjongjapan))
	int mId = -1;
	std::string mName;  // +0x08
	// +0x10 (0x5322a8 (mahjongchina), 0x532e48 (mahjongjapan)) a hint after 30 s without a click
	bool mAutoHint;
	// +0x11 (0x5322a9 (mahjongchina), 0x532e49 (mahjongjapan)) the tiles that cannot be taken are shaded
	bool mShadeCovered;
	// +0x14 (0x5322ac (mahjongchina), 0x532e4c (mahjongjapan)) the layout chosen in the list
	int mSelectedLayout;
	// +0x18 (0x5322b0 (mahjongchina), 0x532e50 (mahjongjapan)) the share of fours in a deal
	int mQuadPercent;
	// +0x1c (0x5322b4 (mahjongchina), 0x532e54 (mahjongjapan)) the list's tile set (0-5)
	int mStyleFree;
	// +0x20 (0x5322b8 (mahjongchina), 0x532e58 (mahjongjapan)) the list's background, or -1 for each in turn
	int mBackground;
	// +0x24 (0x5322bc (mahjongchina), 0x532e5c (mahjongjapan))
	bool mFreeTipShown;
	// +0x28 (0x5322c0 (mahjongchina), 0x532e60 (mahjongjapan)) seconds, 36000 for none
	int mBestTimes[edition::NUM_LAYOUT_RECORDS];
	// +0x2e4, China +0x280 (0x532518 (mahjongchina), 0x53311c (mahjongjapan))
	int mBestScores[edition::NUM_LAYOUT_RECORDS];
	// +0x5a0, China +0x4d8 (0x532770 (mahjongchina), 0x5333d8 (mahjongjapan)) the escape's levels won
	int mProgress;
	// +0x5a4, China +0x4dc (0x532774 (mahjongchina), 0x5333dc (mahjongjapan))
	int mStyleEscape;
	// +0x5a8, China +0x4e0 (0x532778 (mahjongchina), 0x5333e0 (mahjongjapan))
	int mEscBestTimes[edition::NUM_LEVELS];
	// +0x698, China +0x5b0 (0x532848 (mahjongchina), 0x5334d0 (mahjongjapan))
	int mEscBestScores[edition::NUM_LEVELS];
	// +0x788, China +0x680 (0x532918 (mahjongchina), 0x5335c0 (mahjongjapan)) the note on leaving the escape
	bool mShowQuitNote;
	int mMagic = 0;                // +0x78c (China +0x684) 'ME'
	int mVersion = 0;              // +0x790 (China +0x688)

	// 0x415920 (mahjongchina), 0x415eb0 (mahjongjapan)
	User() { Reset(-1); }
	// 0x415980 (mahjongchina), 0x415f10 (mahjongjapan), 0x4159a0 (mahjongchina), 0x415f30 (mahjongjapan)
	~User() override {}

	// 0x4159b0 (mahjongchina), 0x415f40 (mahjongjapan)
	void Reset(int theId);
	// 0x415a30 (mahjongchina), 0x415fc0 (mahjongjapan): the CRC takes in the exe's folder too
	void Serialize(jb::Stream& theStream) override;
	// 0x415b60 (mahjongchina), 0x416100 (mahjongjapan): a new player theName when the file is missing or bad
	void Load(int theId, const char* theName);
	// 0x415c50 (mahjongchina), 0x4161f0 (mahjongjapan)
	void Save();
	// 0x415cf0 (mahjongchina), 0x4162a0 (mahjongjapan)
	static void GetUserPath(int theId, std::string& thePath);
	// 0x415d60 (mahjongchina), 0x416310 (mahjongjapan): the pattern of the player's saved games
	static void GetSavePattern(int theId, std::string& thePattern, std::string& theFolder);
	// 0x415de0 (mahjongchina), 0x416390 (mahjongjapan), 0x415e60 (mahjongchina), 0x416410 (mahjongjapan)
	void GetFreeSave(int theLevel, std::string& thePath);
	void GetEscapeSave(int theLevel, std::string& thePath);
};

extern User gUser;  // 0x532298 (mahjongchina), 0x532e38 (mahjongjapan)

} // namespace mahjong

#endif
