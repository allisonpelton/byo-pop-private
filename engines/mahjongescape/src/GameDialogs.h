#ifndef __MAHJONG_GAMEDIALOGS_H__
#define __MAHJONG_GAMEDIALOGS_H__

// The dialogs of the play layer (0x406b90..0x4090b0 (mahjongchina), 0x406940..0x409270 (mahjongjapan),
// 0x409c50..0x409fa0 (mahjongchina), 0x409d90..0x40a0f0 (mahjongjapan), 0x411b80..0x411cd0 (mahjongchina),
// 0x412110..0x412260 (mahjongjapan)): the menus of the list (0x406b90 (mahjongchina), 0x406940 (mahjongjapan)) and of
// the escape (0x4079d0 (mahjongchina), 0x4077a0 (mahjongjapan)), the note on leaving the escape
// (0x407770 (mahjongchina), 0x407540 (mahjongjapan)), the list's result (0x4071f0 (mahjongchina),
// 0x406f90 (mahjongjapan)), the escape's level won (0x407f10 (mahjongchina), 0x407cd0 (mahjongjapan)) with its scroll
// (0x408270 (mahjongchina), 0x408020 (mahjongjapan)), the tips (0x4084f0 (mahjongchina), 0x408670 (mahjongjapan)) with
// their pictures (0x408a20 (mahjongchina), 0x408ba0 (mahjongjapan)), the question when no pair is left
// (0x409c50 (mahjongchina), 0x409d90 (mahjongjapan)) and the box for a layout that would not load
// (0x411b80 (mahjongchina), 0x412110 (mahjongjapan)).

#include "Dialog.h"
#include "Layer.h"
#include "LevelDone.h"
#include "Object.h"

#include <string>

namespace jb
{
class CheckBox;
class Font;
}

namespace mahjong
{

class Board;
class PlayLayer;

// The message of a result: the records the player has just set, if any (the list's result and Japan's level won)
void FormatRecords(std::string& theMessage, bool theNewTime, bool theNewScore);

// 0x406b90 (mahjongchina), 0x406940 (mahjongjapan) (vtable 0x51b980 (mahjongchina), 0x51ca00 (mahjongjapan), 0x58
// bytes): the list's menu
class FreeMenu : public Dialog
{
public:
	PlayLayer* mOwner;  // +0x54

	// 0x406b90 (mahjongchina), 0x406940 (mahjongjapan)
	explicit FreeMenu(PlayLayer* theOwner) : Dialog(0x1fe, 0x50), mOwner(theOwner) {}
	// 0x406bc0 (mahjongchina), 0x406970 (mahjongjapan), 0x406be0 (mahjongchina), 0x406990 (mahjongjapan)
	~FreeMenu() override {}

	void Create() override;               // 0x406bf0 (mahjongchina), 0x4069a0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x406ec0 (mahjongchina), 0x406c70 (mahjongjapan)
};

// 0x4071f0 (mahjongchina), 0x406f90 (mahjongjapan) (vtable 0x51b9ac (mahjongchina), 0x51ca2c (mahjongjapan), 0x7c
// bytes): the list's game won, with the time, the score and their records
class ResultBox : public Dialog
{
public:
	std::string mTime;       // +0x54
	std::string mScore;      // +0x5c
	std::string mBestTime;   // +0x64
	std::string mBestScore;  // +0x6c
	std::string mMessage;    // +0x74 the new records, if any

	// 0x4071f0 (mahjongchina), 0x406f90 (mahjongjapan)
	ResultBox(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore, bool theNewScore);
	// 0x407350 (mahjongchina), 0x4070d0 (mahjongjapan), 0x407370 (mahjongchina), 0x4070f0 (mahjongjapan)
	~ResultBox() override {}

	void Create() override;               // 0x407400 (mahjongchina), 0x407180 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x4076c0 (mahjongchina), 0x407490 (mahjongjapan)
};

// 0x407770 (mahjongchina), 0x407540 (mahjongjapan) (vtable 0x51b9d8 (mahjongchina), 0x51ca58 (mahjongjapan), 0x58
// bytes): the note that the escape is saved, shown on leaving it until turned off
class QuitNote : public Dialog
{
public:
	jb::CheckBox* mDontShow = nullptr;  // +0x54

	// 0x407770 (mahjongchina), 0x407540 (mahjongjapan)
	QuitNote() : Dialog(0xb8, 0x96) {}
	// 0x407790 (mahjongchina), 0x407560 (mahjongjapan), 0x4077b0 (mahjongchina), 0x407580 (mahjongjapan)
	~QuitNote() override {}

	void Create() override;               // 0x4077c0 (mahjongchina), 0x407590 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x407920 (mahjongchina), 0x4076f0 (mahjongjapan)
};

// 0x4079d0 (mahjongchina), 0x4077a0 (mahjongjapan) (vtable 0x51ba04 (mahjongchina), 0x51ca84 (mahjongjapan), 0x58
// bytes): the escape's menu
class EscapeMenu : public Dialog
{
public:
	PlayLayer* mOwner;  // +0x54

	// 0x4079d0 (mahjongchina), 0x4077a0 (mahjongjapan)
	explicit EscapeMenu(PlayLayer* theOwner) : Dialog(0x235, 0xa5), mOwner(theOwner) {}
	// 0x407a00 (mahjongchina), 0x4077d0 (mahjongjapan), 0x407a20 (mahjongchina), 0x4077f0 (mahjongjapan)
	~EscapeMenu() override {}

	void Create() override;               // 0x407a30 (mahjongchina), 0x407800 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x407c50 (mahjongchina), 0x407a20 (mahjongjapan)
};

// 0x408a20 (mahjongchina), 0x408ba0 (mahjongjapan) (vtable 0x51cba4 (mahjongjapan), 0xec bytes): a tip's picture: the
// image 0x3f the special tiles with their names, 0x172 a bobbing arrow, any other one an image with its labels
class TipPicture : public jb::Object
{
public:
	jb::Font* mFont = nullptr;  // +0x90
	int mImage;                 // +0x94
	std::string mLabel1;        // +0x98 (STRING_PICTURE_FREE_TILE)
	std::string mLabel2;        // +0xa0 (STRING_PICTURE_NOT_FREE)
	std::string mNames1[4];     // +0xa8 (STRING_PICTURE_SEASON_NAME, tab-separated)
	std::string mNames2[4];     // +0xc8 (STRING_PICTURE_FLOWER_NAME)
	int mBob = 0;               // +0xe8

	// 0x408a20 (mahjongchina), 0x408ba0 (mahjongjapan)
	explicit TipPicture(int theImage) : mImage(theImage) {}
	// 0x408ae0 (mahjongchina), 0x408c60 (mahjongjapan), 0x408b00 (mahjongchina), 0x408c80 (mahjongjapan)
	~TipPicture() override {}

	void Draw() override;    // 0x408d60 (mahjongchina), 0x408ee0 (mahjongjapan)
	void Init() override;    // 0x408ba0 (mahjongchina), 0x408d20 (mahjongjapan)
	bool Update() override;  // 0x408d20 (mahjongchina), 0x408ea0 (mahjongjapan)

	// 0x408ff0 (mahjongchina), 0x4091b0 (mahjongjapan): a tile of a kind on the blank tile
	void DrawTile(int theX, int theY, int theKind);
};

// 0x4084f0 (mahjongchina), 0x408670 (mahjongjapan) (vtable 0x51bb00 (mahjongchina), 0x51cb78 (mahjongjapan), 0x5c
// bytes): a tip, at its place of the table 0x51bab8 (mahjongchina), 0x51cb30 (mahjongjapan)
class TipBox : public Dialog
{
public:
	int mId;        // +0x54
	Board* mBoard;  // +0x58

	// 0x4084f0 (mahjongchina), 0x408670 (mahjongjapan)
	TipBox(int theId, Board* theBoard);
	// 0x408530 (mahjongchina), 0x4086b0 (mahjongjapan), 0x408550 (mahjongchina), 0x4086d0 (mahjongjapan)
	~TipBox() override {}

	void Create() override;               // 0x408560 (mahjongchina), 0x4086e0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x4089f0 (mahjongchina), 0x408b70 (mahjongjapan)
};

// 0x409c50 (mahjongchina), 0x409d90 (mahjongjapan) (vtable 0x51bbdc (mahjongchina), 0x51cc54 (mahjongjapan), 0x6c
// bytes): no pair is left: undo, shuffle or start again
class NoMovesBox : public Dialog
{
public:
	jb::RadioGroup mGroup;  // +0x54

	// 0x409c50 (mahjongchina), 0x409d90 (mahjongjapan)
	NoMovesBox() : Dialog(0xda, 0xa5) {}
	// 0x409cb0 (mahjongchina), 0x409df0 (mahjongjapan), 0x409cd0 (mahjongchina), 0x409e10 (mahjongjapan)
	~NoMovesBox() override {}

	void Create() override;               // 0x409d30 (mahjongchina), 0x409e70 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x409f50 (mahjongchina), 0x40a090 (mahjongjapan)
};

// 0x411b80 (mahjongchina), 0x412110 (mahjongjapan) (vtable 0x51c550 (mahjongchina), 0x51d660 (mahjongjapan), 0x54
// bytes): a box with an OK button; its caller gives the title and the text
class ErrorBox : public Dialog
{
public:
	// 0x411b80 (mahjongchina), 0x412110 (mahjongjapan)
	ErrorBox() : Dialog(0xce, 0x96) {}
	// 0x411ba0 (mahjongchina), 0x412130 (mahjongjapan), 0x411bc0 (mahjongchina), 0x412150 (mahjongjapan)
	~ErrorBox() override {}

	void Create() override;               // 0x411bd0 (mahjongchina), 0x412160 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x411cb0 (mahjongchina), 0x412240 (mahjongjapan)
};

} // namespace mahjong

#endif
