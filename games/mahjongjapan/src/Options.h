#ifndef __MAHJONGJAPAN_OPTIONS_H__
#define __MAHJONGJAPAN_OPTIONS_H__

// The options (0x412260, the volumes and the screen) and the choice of the tiles (0x415a10)

#include "Dialog.h"

namespace mahjongjapan
{

// 0x412260 (vtable 0x51d68c, 0x80 bytes)
class OptionsBox : public Dialog
{
public:
	bool mFromMenu;  // +0x54

	// 0x412260
	explicit OptionsBox(bool theFromMenu) : Dialog(theFromMenu ? 0xc7 : 0xc2, 100), mFromMenu(theFromMenu) {}
	~OptionsBox() override {}

	void Create() override;
	bool OnCommand(int theCmd) override;
};

// 0x415a10 (vtable 0x51d888, 0x70 bytes): the tile set of the list (0) or of the escape (1)
class StyleBox : public Dialog
{
public:
	bool mEscape;  // +0x54

	// 0x415a10
	explicit StyleBox(bool theEscape) : Dialog(0xa0, 0x50), mEscape(theEscape) {}
	~StyleBox() override {}

	void Create() override;
	bool OnCommand(int theCmd) override;
};

} // namespace mahjongjapan

#endif
