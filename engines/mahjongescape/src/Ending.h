#ifndef __MAHJONG_ENDING_H__
#define __MAHJONG_ENDING_H__

// The escape's ending (0x412620..0x412ce0 (mahjongchina), 0x412b80..0x413260 (mahjongjapan)): a picture, the player's
// title, and the showcase's names (twelve, China's eleven) that fade in one by one around it, shown when the last level
// is won and again from the map's button.

#include "Edition.h"
#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjong
{

// 0x412620 (mahjongchina), 0x412b80 (mahjongjapan) (vtable 0x51c660 (mahjongchina), 0x51d778 (mahjongjapan), 0x110
// bytes)
class EndPicture : public jb::Object
{
public:
	std::string mTitle;              // +0x90 (0x96 with the player's name)
	std::string mLines[edition::NUM_SHOWCASE_LINES];  // +0x98 (STRING_SHOWCASE_NAMES, by tabs)
	jb::Font* mTitleFont = nullptr;  // +0xf8
	jb::Font* mFont = nullptr;       // +0xfc
	int mLine = 0;                   // +0x100 the line fading in, 1-based; 0 the picture's frame
	int mAlpha = 0;                  // +0x104
	int mDelay = 1000;               // +0x108 before the first fade
	bool mFirst = true;              // +0x10c

	// 0x412620 (mahjongchina), 0x412b80 (mahjongjapan)
	EndPicture() {}
	// 0x4126c0 (mahjongchina), 0x412c20 (mahjongjapan), 0x4126e0 (mahjongchina), 0x412c40 (mahjongjapan)
	~EndPicture() override {}

	void Draw() override;    // 0x412750 (mahjongchina), 0x412cb0 (mahjongjapan)
	void Init() override;    // 0x412880 (mahjongchina), 0x412df0 (mahjongjapan)
	bool Update() override;  // 0x412980 (mahjongchina), 0x412ef0 (mahjongjapan)
};

// 0x412a60 (mahjongchina), 0x412fe0 (mahjongjapan) (vtable 0x51c6b8 (mahjongchina), 0x51d7d0 (mahjongjapan), 0x54
// bytes)
class EndLayer : public jb::Layer
{
public:
	EndPicture* mPicture = nullptr;  // +0x50

	// 0x412a60 (mahjongchina), 0x412fe0 (mahjongjapan)
	EndLayer() : Layer(0, 0) {}
	// 0x412a80 (mahjongchina), 0x413000 (mahjongjapan), 0x412aa0 (mahjongchina), 0x413020 (mahjongjapan)
	~EndLayer() override {}

	void Load() override;                 // 0x412ab0 (mahjongchina), 0x413030 (mahjongjapan)
	bool OnCommand(int theCmd) override;  // 0x412c20 (mahjongchina), 0x4131a0 (mahjongjapan)
};

} // namespace mahjong

#endif
