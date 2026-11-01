#ifndef __MAHJONGJAPAN_HELP_H__
#define __MAHJONGJAPAN_HELP_H__

// The help of the list (0x40e100) and of the escape (0x40e4d0), pages turned in a dialog

#include "Dialog.h"
#include "Object.h"

namespace mahjongjapan
{

// 0x40e100 (vtable 0x51cd60, 100 bytes)
class HelpFree : public Dialog
{
public:
	int mPage = 0;  // +0x54

	// 0x40e100
	HelpFree() : Dialog(0xa3, 0x50) {}
	~HelpFree() override {}

	void Create() override;
	bool OnCommand(int theCmd) override;
};

// 0x40e4d0 (vtable 0x51cd8c, 100 bytes)
class HelpEscape : public Dialog
{
public:
	int mPage = 0;  // +0x54

	// 0x40e4d0
	HelpEscape() : Dialog(0xa3, 0x50) {}
	~HelpEscape() override {}

	void Create() override;
	bool OnCommand(int theCmd) override;
};

// 0x40f400 (vtable 0x51ce3c, 0x94 bytes): the picture beside the main menu's help
class HelpPicture : public jb::Object
{
public:
	int mPicture = 0;  // +0x90 0 none, 1 image 0x164, 2 image 0x165

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

} // namespace mahjongjapan

#endif
