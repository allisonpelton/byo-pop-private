#ifndef __MAHJONG_HELP_H__
#define __MAHJONG_HELP_H__

// The help of the list (0x40e060 (mahjongchina), 0x40e100 (mahjongjapan)), of the escape (0x40e430 (mahjongchina),
// 0x40e4d0 (mahjongjapan)) and of the main menu (0x40ef80 (mahjongchina), 0x40f060 (mahjongjapan)): pages turned in a
// dialog, most with an example drawn beside them

#include "Dialog.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjong
{

// 0x40e8b0 (mahjongchina), 0x40e950 (mahjongjapan) (vtable 0x51bd40 (mahjongchina), 0x51cdb8 (mahjongjapan), 0xec
// bytes): the example beside a page of the help, in the tile set theTiles
class HelpExample : public jb::Object
{
public:
	jb::Font* mFont = nullptr;  // +0x90
	int mTiles;                 // +0x94
	int mExample = 0;           // +0x98 0 none; 1 and 4 the points (images 0x166, 0x167); 2 and 5 the kinds;
	                            // 3, 6 and 7 the tiles that match
	std::string mPointsText;    // +0x9c (STRING_PICTURE_FREE_TILE)
	std::string mTimeText;      // +0xa4 (STRING_PICTURE_NOT_FREE)
	std::string mNames1[4];     // +0xac (STRING_PICTURE_SEASON_NAME, by tabs)
	std::string mNames2[4];     // +0xcc (STRING_PICTURE_FLOWER_NAME, by tabs)

	// 0x40e8b0 (mahjongchina), 0x40e950 (mahjongjapan)
	explicit HelpExample(int theTiles) : mTiles(theTiles) {}
	// 0x40e960 (mahjongchina), 0x40ea00 (mahjongjapan), 0x40e980 (mahjongchina), 0x40ea20 (mahjongjapan)
	~HelpExample() override {}

	void Draw() override;  // 0x40eb50 (mahjongchina), 0x40ebf0 (mahjongjapan)
	void Init() override;  // 0x40ea20 (mahjongchina), 0x40eac0 (mahjongjapan)

	// 0x40ee50 (mahjongchina), 0x40ef30 (mahjongjapan): one of the magic tiles' faces (IMAGE_SPECIAL_FACES)
	void DrawSpecial(int theX, int theY, int theKind);
	// 0x40eec0 (mahjongchina), 0x40efa0 (mahjongjapan): a blank tile with face theKind (1-based) of the tile set
	void DrawTile(int theX, int theY, int theKind);
};

// 0x40e060 (mahjongchina), 0x40e100 (mahjongjapan) (vtable 0x51bce8 (mahjongchina), 0x51cd60 (mahjongjapan), 100
// bytes): the help of the list, five pages
class HelpFree : public Dialog
{
public:
	int mPage = 0;                    // +0x54
	jb::Button* mPrevious = nullptr;  // +0x58
	jb::Button* mNext = nullptr;      // +0x5c
	HelpExample* mExample = nullptr;  // +0x60

	// 0x40e060 (mahjongchina), 0x40e100 (mahjongjapan)
	HelpFree() : Dialog(0xa3, 0x50) {}
	// 0x40e080 (mahjongchina), 0x40e120 (mahjongjapan), 0x40e0a0 (mahjongchina), 0x40e140 (mahjongjapan)
	~HelpFree() override {}

	void Create() override;               // 0x40e0b0 (mahjongchina), 0x40e150 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x40e290 (mahjongchina), 0x40e330 (mahjongjapan)

	// 0x40e2f0 (mahjongchina), 0x40e390 (mahjongjapan)
	void ShowPage();
};

// 0x40e430 (mahjongchina), 0x40e4d0 (mahjongjapan) (vtable 0x51bd14 (mahjongchina), 0x51cd8c (mahjongjapan), 100
// bytes): the help of the escape, six pages
class HelpEscape : public Dialog
{
public:
	int mPage = 0;                    // +0x54
	jb::Button* mPrevious = nullptr;  // +0x58
	jb::Button* mNext = nullptr;      // +0x5c
	HelpExample* mExample = nullptr;  // +0x60

	// 0x40e430 (mahjongchina), 0x40e4d0 (mahjongjapan)
	HelpEscape() : Dialog(0xa3, 0x50) {}
	// 0x40e450 (mahjongchina), 0x40e4f0 (mahjongjapan), 0x40e470 (mahjongchina), 0x40e510 (mahjongjapan)
	~HelpEscape() override {}

	void Create() override;               // 0x40e480 (mahjongchina), 0x40e520 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x40e660 (mahjongchina), 0x40e700 (mahjongjapan)

	// 0x40e6c0 (mahjongchina), 0x40e760 (mahjongjapan)
	void ShowPage();
};

// 0x40f320 (mahjongchina), 0x40f400 (mahjongjapan) (vtable 0x51bdc4 (mahjongchina), 0x51ce3c (mahjongjapan), 0x94
// bytes): the picture beside the main menu's help
class HelpPicture : public jb::Object
{
public:
	int mPicture = 0;  // +0x90 0 none, 1 IMAGE_HELP_MAP_PICTURE, 2 IMAGE_HELP_LAYOUT_PICTURE

	// 0x40f320 (mahjongchina), 0x40f400 (mahjongjapan)
	HelpPicture() {}
	// 0x40f340 (mahjongchina), 0x40f420 (mahjongjapan), 0x40f360 (mahjongchina), 0x40f440 (mahjongjapan)
	~HelpPicture() override {}

	void Draw() override;  // 0x40f370 (mahjongchina), 0x40f450 (mahjongjapan)
	void Init() override {}  // 0x42cc80 (mahjongjapan)
};

// 0x40ef80 (mahjongchina), 0x40f060 (mahjongjapan) (vtable 0x51bd98 (mahjongchina), 0x51ce10 (mahjongjapan), 100
// bytes): the main menu's help, four pages
class HelpMenu : public Dialog
{
public:
	int mPage = 0;                       // +0x54
	jb::Button* mPrevious = nullptr;     // +0x58
	jb::Button* mNext = nullptr;         // +0x5c
	HelpPicture* mPicture = nullptr;     // +0x60

	// 0x40ef80 (mahjongchina), 0x40f060 (mahjongjapan)
	HelpMenu() : Dialog(0xa3, 0x78) {}
	// 0x40efa0 (mahjongchina), 0x40f080 (mahjongjapan), 0x40efc0 (mahjongchina), 0x40f0a0 (mahjongjapan)
	~HelpMenu() override {}

	void Create() override;               // 0x40efd0 (mahjongchina), 0x40f0b0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x40f1b0 (mahjongchina), 0x40f290 (mahjongjapan)

	// 0x40f210 (mahjongchina), 0x40f2f0 (mahjongjapan)
	void ShowPage();
};

} // namespace mahjong

#endif
