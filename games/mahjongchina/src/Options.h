#ifndef __MAHJONGCHINA_OPTIONS_H__
#define __MAHJONGCHINA_OPTIONS_H__

// The options (0x412110..0x412b80: the volumes, the screen, the hint and the deal's fours) and the choice of the
// tiles (0x415a10)

#include "Dialog.h"
#include "Object.h"
#include "Widgets.h"

namespace mahjongchina
{

// 0x412110 (vtable 0x51d660, 0x54 bytes): a note with an OK button, of the hardware acceleration
class OptionsNote : public Dialog
{
public:
	// 0x412110
	OptionsNote() : Dialog(0xce, 0x96) {}
	// 0x412130, 0x412150
	~OptionsNote() override {}

	void Create() override;               // 0x412160
	bool OnCommand(int theCmd) override;  // 0x412240
};

// 0x412af0 (vtable 0x51d6b8, 0xac bytes): the music's (0) or the sounds' (1) volume
class VolumeSlider : public jb::SliderBar
{
public:
	int mKind;  // +0xa8

	// 0x412af0
	explicit VolumeSlider(int theKind) : SliderBar(299, 300, -5, -6), mKind(theKind) {}
	// 0x412b20, 0x412b40
	~VolumeSlider() override {}

	void OnValue(int theValue) override;  // 0x412b50
};

// 0x412260 (vtable 0x51d68c, 0x80 bytes): from the main menu, with the share of fours in the list's deals
class OptionsBox : public Dialog
{
public:
	jb::CheckBox* mFullScreen = nullptr;  // +0x54
	jb::CheckBox* mWide = nullptr;        // +0x58 only when the desktop is wide
	jb::CheckBox* mAutoHint = nullptr;    // +0x5c
	jb::CheckBox* mAccel = nullptr;       // +0x60 command 2
	jb::RadioGroup mQuads;                // +0x64 0, 25 or 85 percent
	bool mFromMenu;                       // +0x7c

	// 0x412260
	explicit OptionsBox(bool theFromMenu) : Dialog(theFromMenu ? 0xc7 : 0xc2, 100), mFromMenu(theFromMenu) {}
	// 0x4122d0, 0x4122f0
	~OptionsBox() override {}

	void Create() override;               // 0x412350
	bool OnCommand(int theCmd) override;  // 0x412880
};

// 0x4157c0 (vtable 0x51d818, 0xf4 bytes): a tile set, shown as a few tiles of image theImage on back theBack
class StyleToggle : public jb::Toggle
{
public:
	int mBack;  // +0xf0 the frame of image 0x66

	// 0x4157c0
	StyleToggle(int theImage, int theBack, jb::RadioGroup* theGroup, int theId)
		: Toggle(theImage, theGroup, theId), mBack(theBack) {}
	// 0x4157f0, 0x415810
	~StyleToggle() override {}

	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;  // 0x4159f0 the box's Apply
	void DrawNormal() override;  // 0x415820
	void OnInit() override { SetUserFlag(true); }  // 0x401060

	// 0x415870
	void DrawTiles();
	// 0x415900
	void DrawBack(int theX, int theY);
	// 0x415960
	void DrawFace(int theX, int theY, int theKind);
};

// 0x415a10 (vtable 0x51d888, 0x70 bytes): the tile set of the list (0) or of the escape (1)
class StyleBox : public Dialog
{
public:
	bool mEscape;            // +0x54
	jb::RadioGroup mStyles;  // +0x58

	// 0x415a10
	explicit StyleBox(bool theEscape) : Dialog(0xa0, 0x50), mEscape(theEscape) {}
	// 0x415a70, 0x415a90
	~StyleBox() override {}

	void Create() override;               // 0x415af0
	bool OnCommand(int theCmd) override;  // 0x415e40

	// 0x415e70: the play layer under it takes the tile set
	void Apply();
};

} // namespace mahjongchina

#endif
