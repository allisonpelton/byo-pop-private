#ifndef __MAHJONGCHINA_MAINMENU_H__
#define __MAHJONGCHINA_MAINMENU_H__

// The main menu (0x411380..0x412110): the escape and the list as two large buttons whose captions fade in on hover
// (0x4113b0), the options, help and quit buttons, the player's name (0x4115b0, a click opens the players), and the
// backdrop with its two animations (0x411760). A partner's logo (<exe folder>MenuLogo) is drawn as a picture that goes
// nowhere (the original links it to the registry's PartnerURL, which this install lacks).

#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjongchina
{

// 0x4113b0 (vtable 0x51d4e8, 0xf0 bytes)
class MenuButton : public jb::Button
{
public:
	jb::Font* mFont = nullptr;  // +0xe4
	int mTextId;                // +0xe8 STRING_CLASSIC_HINT, else the escape's (STRING_ADVENTURE_BEGIN_HINT, or the next string once begun)
	int mAlpha = 0;             // +0xec

	// 0x4113b0
	MenuButton(int theImage, int theTextId) : Button(theImage, 3, false), mTextId(theTextId) {}
	// 0x4113f0, 0x411410
	~MenuButton() override {}

	void Draw() override;    // 0x4114a0
	bool Update() override;  // 0x411450
	void OnInit() override;  // 0x411420
};

// 0x4115b0 (vtable 0x51d558, 0xf8 bytes): command 1
class PlayerButton : public jb::Button
{
public:
	std::string mText;               // +0xe4 (STRING_CHANGE_PLAYER)
	jb::Font* mFont = nullptr;       // +0xec
	jb::Font* mHoverFont = nullptr;  // +0xf0

	// 0x4115b0
	PlayerButton() : Button(1, 2, true) {}
	// 0x411610, 0x411630
	~PlayerButton() override {}

	void DrawNormal() override;  // 0x411700
	void DrawHover() override;   // 0x411730
	void OnInit() override;      // 0x411690
};

// 0x411760 (vtable 0x51d5c8, 0x94 bytes): the backdrop and the greeting
class MenuBackdrop : public jb::Object
{
public:
	jb::Font* mFont = nullptr;  // +0x90

	// 0x411760
	MenuBackdrop() {}
	// 0x411780, 0x4117a0
	~MenuBackdrop() override {}

	void Draw() override;  // 0x4117b0
	void Init() override;  // 0x411860
};

// 0x411980 (vtable 0x51d638, 0x54 bytes)
class MainMenu : public jb::Layer
{
public:
	int mPending = -1;  // +0x50 the command that waited for a new player

	// 0x411980
	MainMenu() : Layer(0, 0) {}
	// 0x4119a0, 0x4119c0
	~MainMenu() override {}

	void Load() override;                 // 0x4119d0
	bool OnCommand(int theCmd) override;  // 0x411db0
	void AfterActiveChange() override;    // 0x412020

	// 0x412080: a player exists, or the new player's box is shown and theCmd waits for it
	bool HasPlayer(int theCmd);
};

} // namespace mahjongchina

#endif
