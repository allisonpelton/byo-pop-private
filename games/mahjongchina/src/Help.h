#ifndef __MAHJONGCHINA_HELP_H__
#define __MAHJONGCHINA_HELP_H__

// The help of the list (0x40e100), of the escape (0x40e4d0) and of the main menu (0x40f060): pages turned in a
// dialog, most with an example drawn beside them

#include "Dialog.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjongchina
{

// 0x40e950 (vtable 0x51cdb8, 0xec bytes): the example beside a page of the help, in the tile set theTiles
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

	// 0x40e950
	explicit HelpExample(int theTiles) : mTiles(theTiles) {}
	// 0x40ea00, 0x40ea20
	~HelpExample() override {}

	void Draw() override;  // 0x40ebf0
	void Init() override;  // 0x40eac0

	// 0x40ef30: one of the magic tiles' faces (IMAGE_SPECIAL_FACES)
	void DrawSpecial(int theX, int theY, int theKind);
	// 0x40efa0: a blank tile with face theKind (1-based) of the tile set
	void DrawTile(int theX, int theY, int theKind);
};

// 0x40e100 (vtable 0x51cd60, 100 bytes): the help of the list, five pages
class HelpFree : public Dialog
{
public:
	int mPage = 0;                    // +0x54
	jb::Button* mPrevious = nullptr;  // +0x58
	jb::Button* mNext = nullptr;      // +0x5c
	HelpExample* mExample = nullptr;  // +0x60

	// 0x40e100
	HelpFree() : Dialog(0xa3, 0x50) {}
	// 0x40e120, 0x40e140
	~HelpFree() override {}

	void Create() override;               // 0x40e150
	bool OnCommand(int theCmd) override;  // 0x40e330

	// 0x40e390
	void ShowPage();
};

// 0x40e4d0 (vtable 0x51cd8c, 100 bytes): the help of the escape, six pages
class HelpEscape : public Dialog
{
public:
	int mPage = 0;                    // +0x54
	jb::Button* mPrevious = nullptr;  // +0x58
	jb::Button* mNext = nullptr;      // +0x5c
	HelpExample* mExample = nullptr;  // +0x60

	// 0x40e4d0
	HelpEscape() : Dialog(0xa3, 0x50) {}
	// 0x40e4f0, 0x40e510
	~HelpEscape() override {}

	void Create() override;               // 0x40e520
	bool OnCommand(int theCmd) override;  // 0x40e700

	// 0x40e760
	void ShowPage();
};

// 0x40f400 (vtable 0x51ce3c, 0x94 bytes): the picture beside the main menu's help
class HelpPicture : public jb::Object
{
public:
	int mPicture = 0;  // +0x90 0 none, 1 IMAGE_HELP_MAP_PICTURE, 2 IMAGE_HELP_LAYOUT_PICTURE

	// 0x40f400
	HelpPicture() {}
	// 0x40f420, 0x40f440
	~HelpPicture() override {}

	void Draw() override;  // 0x40f450
	void Init() override {}  // 0x42cc80
};

// 0x40f060 (vtable 0x51ce10, 100 bytes): the main menu's help, four pages
class HelpMenu : public Dialog
{
public:
	int mPage = 0;                       // +0x54
	jb::Button* mPrevious = nullptr;     // +0x58
	jb::Button* mNext = nullptr;         // +0x5c
	HelpPicture* mPicture = nullptr;     // +0x60

	// 0x40f060
	HelpMenu() : Dialog(0xa3, 0x78) {}
	// 0x40f080, 0x40f0a0
	~HelpMenu() override {}

	void Create() override;               // 0x40f0b0
	bool OnCommand(int theCmd) override;  // 0x40f290

	// 0x40f2f0
	void ShowPage();
};

} // namespace mahjongchina

#endif
