#ifndef __MAHJONGJAPAN_HELP_H__
#define __MAHJONGJAPAN_HELP_H__

// The help of the list (0x40e100) and of the escape (0x40e4d0), pages turned in a dialog

#include "Dialog.h"

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

} // namespace mahjongjapan

#endif
