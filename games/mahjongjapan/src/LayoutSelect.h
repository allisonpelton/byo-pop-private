#ifndef __MAHJONGJAPAN_LAYOUTSELECT_H__
#define __MAHJONGJAPAN_LAYOUTSELECT_H__

// The list of layouts (0x4026c0) and the choice of the list's background (0x401270)

#include "Dialog.h"
#include "Layer.h"

namespace mahjongjapan
{

// 0x402f80: the id of the layout at a place of the list (table 0x51c4c4), 0 past its end
int GetLayoutId(int theIndex);
// 0x4019b0: the backgrounds open: the escape's stages reached, plus 9
int CountBackgrounds();

// 0x401270 (vtable 0x51c390, 0x94 bytes)
class BackgroundBox : public Dialog
{
public:
	// 0x401270
	BackgroundBox() : Dialog(0x28, 0x50) {}
	~BackgroundBox() override {}

	void Create() override;
	bool OnCommand(int theCmd) override;
};

// 0x4026c0 (vtable 0x51c754, 0x8c bytes)
class LayoutSelect : public jb::Layer, public DialogListener
{
public:
	// 0x4026c0
	LayoutSelect();
	~LayoutSelect() override;

	void Load() override;
	bool OnCommand(int theCmd) override;
	bool OnYes(int theId) override;
	bool OnNo(int theId) override;
};

} // namespace mahjongjapan

#endif
