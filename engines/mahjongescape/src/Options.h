#ifndef __MAHJONG_OPTIONS_H__
#define __MAHJONG_OPTIONS_H__

// The options (0x411b80..0x412620 (mahjongchina), 0x412110..0x412b80 (mahjongjapan): the volumes, the screen, the hint
// and the deal's fours) and the choice of the tiles (0x415480 (mahjongchina), 0x415a10 (mahjongjapan))

#include "Dialog.h"
#include "Object.h"
#include "Res.h"
#include "Widgets.h"

namespace mahjong
{

// 0x411b80 (mahjongchina), 0x412110 (mahjongjapan) (vtable 0x51c550 (mahjongchina), 0x51d660 (mahjongjapan), 0x54
// bytes): a note with an OK button, of the hardware acceleration
class OptionsNote : public Dialog
{
public:
	// 0x411b80 (mahjongchina), 0x412110 (mahjongjapan)
	OptionsNote() : Dialog(0xce, 0x96) {}
	// 0x411ba0 (mahjongchina), 0x412130 (mahjongjapan), 0x411bc0 (mahjongchina), 0x412150 (mahjongjapan)
	~OptionsNote() override {}

	void Create() override;               // 0x411bd0 (mahjongchina), 0x412160 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x411cb0 (mahjongchina), 0x412240 (mahjongjapan)
};

// 0x412590 (mahjongchina), 0x412af0 (mahjongjapan) (vtable 0x51c5a8 (mahjongchina), 0x51d6b8 (mahjongjapan), 0xac
// bytes): the music's (0) or the sounds' (1) volume
class VolumeSlider : public jb::SliderBar
{
public:
	int mKind;  // +0xa8

	// 0x412590 (mahjongchina), 0x412af0 (mahjongjapan)
	explicit VolumeSlider(int theKind) : SliderBar(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, -5, -6), mKind(theKind) {}
	// 0x4125c0 (mahjongchina), 0x412b20 (mahjongjapan), 0x4125e0 (mahjongchina), 0x412b40 (mahjongjapan)
	~VolumeSlider() override {}

	void OnValue(int theValue) override;  // 0x412b50 (mahjongjapan)
};

// 0x411cd0 (mahjongchina), 0x412260 (mahjongjapan) (vtable 0x51c57c (mahjongchina), 0x51d68c (mahjongjapan), 0x80
// bytes): from the main menu, with the share of fours in the list's deals
class OptionsBox : public Dialog
{
public:
	jb::CheckBox* mFullScreen = nullptr;  // +0x54
	jb::CheckBox* mWide = nullptr;        // +0x58 only when the desktop is wide
	jb::CheckBox* mAutoHint = nullptr;    // +0x5c
	jb::CheckBox* mAccel = nullptr;       // +0x60 command 2
	jb::RadioGroup mQuads;                // +0x64 0, 25 or 85 percent
	bool mFromMenu;                       // +0x7c

	// 0x411cd0 (mahjongchina), 0x412260 (mahjongjapan)
	explicit OptionsBox(bool theFromMenu) : Dialog(theFromMenu ? 0xc7 : 0xc2, 100), mFromMenu(theFromMenu) {}
	// 0x411d40 (mahjongchina), 0x4122d0 (mahjongjapan), 0x411d60 (mahjongchina), 0x4122f0 (mahjongjapan)
	~OptionsBox() override {}

	void Create() override;               // 0x411dc0 (mahjongchina), 0x412350 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x4122f0 (mahjongchina), 0x412880 (mahjongjapan)
};

// 0x415240 (mahjongchina), 0x4157c0 (mahjongjapan) (vtable 0x51c700 (mahjongchina), 0x51d818 (mahjongjapan), 0xf4
// bytes): a tile set, shown as a few tiles of image theImage on back theBack
class StyleToggle : public jb::Toggle
{
public:
	int mBack;  // +0xf0 the frame of IMAGE_SPECIAL_FACES

	// 0x415240 (mahjongchina), 0x4157c0 (mahjongjapan)
	StyleToggle(int theImage, int theBack, jb::RadioGroup* theGroup, int theId)
		: Toggle(theImage, theGroup, theId), mBack(theBack) {}
	// 0x415270 (mahjongchina), 0x4157f0 (mahjongjapan), 0x415290 (mahjongchina), 0x415810 (mahjongjapan)
	~StyleToggle() override {}

	// 0x415460 (mahjongchina), 0x4159f0 (mahjongjapan) the box's Apply
	bool OnLButtonDblClk(int theFlags, int theX, int theY) override;
	void DrawNormal() override;  // 0x4152b0 (mahjongchina), 0x415820 (mahjongjapan)
	void OnInit() override { SetUserFlag(true); }  // 0x401060 (mahjongjapan)

	// 0x4152f0 (mahjongchina), 0x415870 (mahjongjapan)
	void DrawTiles();
	// 0x415380 (mahjongchina), 0x415900 (mahjongjapan)
	void DrawBack(int theX, int theY);
	// 0x4153e0 (mahjongchina), 0x415960 (mahjongjapan)
	void DrawFace(int theX, int theY, int theKind);
};

// 0x415480 (mahjongchina), 0x415a10 (mahjongjapan) (vtable 0x51c770 (mahjongchina), 0x51d888 (mahjongjapan), 0x70
// bytes): the tile set of the list (0) or of the escape (1)
class StyleBox : public Dialog
{
public:
	bool mEscape;            // +0x54
	jb::RadioGroup mStyles;  // +0x58

	// 0x415480 (mahjongchina), 0x415a10 (mahjongjapan)
	explicit StyleBox(bool theEscape) : Dialog(0xa0, 0x50), mEscape(theEscape) {}
	// 0x4154e0 (mahjongchina), 0x415a70 (mahjongjapan), 0x415500 (mahjongchina), 0x415a90 (mahjongjapan)
	~StyleBox() override {}

	void Create() override;               // 0x415560 (mahjongchina), 0x415af0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x4158b0 (mahjongchina), 0x415e40 (mahjongjapan)

	// 0x4158e0 (mahjongchina), 0x415e70 (mahjongjapan): the play layer under it takes the tile set
	void Apply();
};

} // namespace mahjong

#endif
