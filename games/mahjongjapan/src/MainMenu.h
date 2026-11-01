#ifndef __MAHJONGJAPAN_MAINMENU_H__
#define __MAHJONGJAPAN_MAINMENU_H__

// The main menu (0x411980)

#include "Layer.h"

namespace mahjongjapan
{

// 0x411980 (vtable 0x51d638, 0x54 bytes)
class MainMenu : public jb::Layer
{
public:
	int m50 = -1;  // +0x50

	// 0x411980
	MainMenu() : Layer(0, 0) {}
	~MainMenu() override {}

	void Load() override;
	bool OnCommand(int theCmd) override;
};

} // namespace mahjongjapan

#endif
