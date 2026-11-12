#ifndef __MAHJONG_MAINMENU_H__
#define __MAHJONG_MAINMENU_H__

// The main menu (0x410e80..0x411b80 (mahjongchina), 0x411380..0x412110 (mahjongjapan)): the escape and the list as two
// large buttons whose captions fade in on hover (0x410e80 (mahjongchina), 0x4113b0 (mahjongjapan)), the options, help
// and quit buttons, the player's name (0x411060 (mahjongchina), 0x4115b0 (mahjongjapan), a click opens the players),
// and the backdrop with its two animations (0x411200 (mahjongchina), 0x411760 (mahjongjapan)). A partner's logo (<exe
// folder>MenuLogo) is drawn as a picture that goes nowhere (the original links it to the registry's PartnerURL, which
// this install lacks).

#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjong
{

// 0x410e80 (mahjongchina), 0x4113b0 (mahjongjapan) (vtable 0x51c3d8 (mahjongchina), 0x51d4e8 (mahjongjapan), 0xf0
// bytes)
class MenuButton : public jb::Button
{
public:
	jb::Font* mFont = nullptr;  // +0xe4
	// +0xe8 STRING_CLASSIC_HINT, else the escape's (STRING_ADVENTURE_BEGIN_HINT, or the next string once begun)
	int mTextId;
	int mAlpha = 0;             // +0xec

	// 0x410e80 (mahjongchina), 0x4113b0 (mahjongjapan)
	MenuButton(int theImage, int theTextId) : Button(theImage, 3, false), mTextId(theTextId) {}
	// 0x410ec0 (mahjongchina), 0x4113f0 (mahjongjapan), 0x410ee0 (mahjongchina), 0x411410 (mahjongjapan)
	~MenuButton() override {}

	void Draw() override;    // 0x410f70 (mahjongchina), 0x4114a0 (mahjongjapan)
	bool Update() override;  // 0x410f20 (mahjongchina), 0x411450 (mahjongjapan)
	void OnInit() override;  // 0x410ef0 (mahjongchina), 0x411420 (mahjongjapan)
};

// 0x411060 (mahjongchina), 0x4115b0 (mahjongjapan) (vtable 0x51c448 (mahjongchina), 0x51d558 (mahjongjapan), 0xf8
// bytes): command 1
class PlayerButton : public jb::Button
{
public:
	std::string mText;               // +0xe4 (STRING_CHANGE_PLAYER)
	jb::Font* mFont = nullptr;       // +0xec
	jb::Font* mHoverFont = nullptr;  // +0xf0

	// 0x411060 (mahjongchina), 0x4115b0 (mahjongjapan)
	PlayerButton() : Button(1, 2, true) {}
	// 0x4110c0 (mahjongchina), 0x411610 (mahjongjapan), 0x4110e0 (mahjongchina), 0x411630 (mahjongjapan)
	~PlayerButton() override {}

	void DrawNormal() override;  // 0x4111a0 (mahjongchina), 0x411700 (mahjongjapan)
	void DrawHover() override;   // 0x4111d0 (mahjongchina), 0x411730 (mahjongjapan)
	void OnInit() override;      // 0x411140 (mahjongchina), 0x411690 (mahjongjapan)
};

// 0x411200 (mahjongchina), 0x411760 (mahjongjapan) (vtable 0x51c4b8 (mahjongchina), 0x51d5c8 (mahjongjapan), 0x94
// bytes): the backdrop and the greeting
class MenuBackdrop : public jb::Object
{
public:
	jb::Font* mFont = nullptr;  // +0x90

	// 0x411200 (mahjongchina), 0x411760 (mahjongjapan)
	MenuBackdrop() {}
	// 0x411220 (mahjongchina), 0x411780 (mahjongjapan), 0x411240 (mahjongchina), 0x4117a0 (mahjongjapan)
	~MenuBackdrop() override {}

	void Draw() override;  // 0x411250 (mahjongchina), 0x4117b0 (mahjongjapan)
	void Init() override;  // 0x4112f0 (mahjongchina), 0x411860 (mahjongjapan)
};

// 0x411410 (mahjongchina), 0x411980 (mahjongjapan) (vtable 0x51c528 (mahjongchina), 0x51d638 (mahjongjapan), 0x54
// bytes)
class MainMenu : public jb::Layer
{
public:
	int mPending = -1;  // +0x50 the command that waited for a new player

	// 0x411410 (mahjongchina), 0x411980 (mahjongjapan)
	MainMenu() : Layer(0, 0) {}
	// 0x411430 (mahjongchina), 0x4119a0 (mahjongjapan), 0x411450 (mahjongchina), 0x4119c0 (mahjongjapan)
	~MainMenu() override {}

	void Load() override;                 // 0x411460 (mahjongchina), 0x4119d0 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x411810 (mahjongchina), 0x411db0 (mahjongjapan)
	void AfterActiveChange() override;    // 0x411aa0 (mahjongchina), 0x412020 (mahjongjapan)

	// 0x411af0 (mahjongchina), 0x412080 (mahjongjapan): a player exists, or the new player's box is shown and theCmd
	// waits for it
	bool HasPlayer(int theCmd);
};

} // namespace mahjong

#endif
