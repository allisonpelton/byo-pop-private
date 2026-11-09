#ifndef __MAHJONGCHINA_GAMEDIALOGS_H__
#define __MAHJONGCHINA_GAMEDIALOGS_H__

// The dialogs of the play layer (0x406940..0x409270, 0x409d90..0x40a0f0, 0x412110..0x412260): the menus of the list
// (0x406940) and of the escape (0x4077a0), the note on leaving the escape (0x407540), the list's result (0x406f90),
// the escape's level won (0x407cd0) with its scroll (0x408020), the tips (0x408670) with their pictures (0x408ba0),
// the question when no pair is left (0x409d90) and the box for a layout that would not load (0x412110).

#include "Dialog.h"
#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class CheckBox;
class Font;
}

namespace mahjongchina
{

class Board;
class PlayLayer;

// 0x406940 (vtable 0x51ca00, 0x58 bytes): the list's menu
class FreeMenu : public Dialog
{
public:
	PlayLayer* mOwner;  // +0x54

	// 0x406940
	explicit FreeMenu(PlayLayer* theOwner) : Dialog(0x1fe, 0x50), mOwner(theOwner) {}
	// 0x406970, 0x406990
	~FreeMenu() override {}

	void Create() override;               // 0x4069a0
	bool OnCommand(int theCmd) override;  // 0x406c70
};

// 0x406f90 (vtable 0x51ca2c, 0x7c bytes): the list's game won, with the time, the score and their records
class ResultBox : public Dialog
{
public:
	std::string mTime;       // +0x54
	std::string mScore;      // +0x5c
	std::string mBestTime;   // +0x64
	std::string mBestScore;  // +0x6c
	std::string mMessage;    // +0x74 the new records, if any

	// 0x406f90
	ResultBox(int theTime, int theBestTime, bool theNewTime, int theScore, int theBestScore, bool theNewScore);
	// 0x4070d0, 0x4070f0
	~ResultBox() override {}

	void Create() override;               // 0x407180
	bool OnCommand(int theCmd) override;  // 0x407490
};

// 0x407540 (vtable 0x51ca58, 0x58 bytes): the note that the escape is saved, shown on leaving it until turned off
class QuitNote : public Dialog
{
public:
	jb::CheckBox* mDontShow = nullptr;  // +0x54

	// 0x407540
	QuitNote() : Dialog(0xb8, 0x96) {}
	// 0x407560, 0x407580
	~QuitNote() override {}

	void Create() override;               // 0x407590
	bool OnCommand(int theCmd) override;  // 0x4076f0
};

// 0x4077a0 (vtable 0x51ca84, 0x58 bytes): the escape's menu
class EscapeMenu : public Dialog
{
public:
	PlayLayer* mOwner;  // +0x54

	// 0x4077a0
	explicit EscapeMenu(PlayLayer* theOwner) : Dialog(0x235, 0xa5), mOwner(theOwner) {}
	// 0x4077d0, 0x4077f0
	~EscapeMenu() override {}

	void Create() override;               // 0x407800
	bool OnCommand(int theCmd) override;  // 0x407a20
};

// 0x408020 (vtable 0x51ba5c, 0xac bytes): the level won's scroll of the next level's text, which fades in after
// 0.6 s. China's: Japan's scroll also held the time, the score, the bests and the records
class LevelDoneScroll : public jb::Object
{
public:
	jb::Font* mTextFont = nullptr;     // +0x8c (0x12e)
	std::string mNextLabel;            // +0x90 (0x22)
	std::string mNextTitle;            // +0x98 the next level's text
	int mDelay = 600;                  // +0xa0
	int mAlpha = 0;                    // +0xa4
	int mLines = 0;                    // +0xa8 of the next level's text

	// 0x408020
	explicit LevelDoneScroll(const char* theNextTitle) : mNextTitle(theNextTitle) {}
	// 0x408150, 0x408170
	~LevelDoneScroll() override {}

	void Draw() override;    // 0x408270
	void Init() override;    // 0x4084e0
	bool Update() override;  // 0x4085f0
};

// 0x407cd0 (vtable 0x51ba30, 0x70 bytes): the escape's level won, a dialog with the time and the score. China's:
// Japan's was a layer of its own with the bests and the records on the scroll
class LevelDone : public Dialog
{
public:
	std::string mTime;       // +0x54
	std::string mScore;      // +0x5c
	std::string mNextTitle;  // +0x64

	// 0x407cd0
	LevelDone(int theTime, int theScore, const char* theNextTitle);
	// 0x407e40
	~LevelDone() override {}

	void Create() override;               // 0x407ee0
	bool OnCommand(int theCmd) override;  // 0x407ff0
};

// 0x408ba0 (vtable 0x51cba4, 0xec bytes): a tip's picture: the image 0x3f the special tiles with their names, 0x172 a
// bobbing arrow, any other one an image with its labels
class TipPicture : public jb::Object
{
public:
	jb::Font* mFont = nullptr;  // +0x90
	int mImage;                 // +0x94
	std::string mLabel1;        // +0x98 (0x5e)
	std::string mLabel2;        // +0xa0 (0x67)
	std::string mNames1[4];     // +0xa8 (0x69, tab-separated)
	std::string mNames2[4];     // +0xc8 (0x5d)
	int mBob = 0;               // +0xe8

	// 0x408ba0
	explicit TipPicture(int theImage) : mImage(theImage) {}
	// 0x408c60, 0x408c80
	~TipPicture() override {}

	void Draw() override;    // 0x408ee0
	void Init() override;    // 0x408d20
	bool Update() override;  // 0x408ea0

	// 0x4091b0: a tile of a kind on the blank tile
	void DrawTile(int theX, int theY, int theKind);
};

// 0x408670 (vtable 0x51cb78, 0x5c bytes): a tip, at its place of the table 0x51cb30
class TipBox : public Dialog
{
public:
	int mId;        // +0x54
	Board* mBoard;  // +0x58

	// 0x408670
	TipBox(int theId, Board* theBoard);
	// 0x4086b0, 0x4086d0
	~TipBox() override {}

	void Create() override;               // 0x4086e0
	bool OnCommand(int theCmd) override;  // 0x408b70
};

// 0x409d90 (vtable 0x51cc54, 0x6c bytes): no pair is left: undo, shuffle or start again
class NoMovesBox : public Dialog
{
public:
	jb::RadioGroup mGroup;  // +0x54

	// 0x409d90
	NoMovesBox() : Dialog(0xda, 0xa5) {}
	// 0x409df0, 0x409e10
	~NoMovesBox() override {}

	void Create() override;               // 0x409e70
	bool OnCommand(int theCmd) override;  // 0x40a090
};

// 0x412110 (vtable 0x51d660, 0x54 bytes): a box with an OK button; its caller gives the title and the text
class ErrorBox : public Dialog
{
public:
	// 0x412110
	ErrorBox() : Dialog(0xce, 0x96) {}
	// 0x412130, 0x412150
	~ErrorBox() override {}

	void Create() override;               // 0x412160
	bool OnCommand(int theCmd) override;  // 0x412240
};

} // namespace mahjongchina

#endif
