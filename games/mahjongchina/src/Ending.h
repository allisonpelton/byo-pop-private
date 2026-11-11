#ifndef __MAHJONGCHINA_ENDING_H__
#define __MAHJONGCHINA_ENDING_H__

// The escape's ending (0x412b80..0x413260): a picture, the player's title, and twelve lines that fade in one by one
// around it, shown when the sixtieth level is won and again from the map's button.

#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{
class Font;
}

namespace mahjongchina
{

// 0x412b80 (vtable 0x51d778, 0x110 bytes)
class EndPicture : public jb::Object
{
public:
	std::string mTitle;              // +0x90 (0x96 with the player's name)
	std::string mLines[11];          // +0x98 (STRING_SHOWCASE_NAMES, by tabs)
	jb::Font* mTitleFont = nullptr;  // +0xf8
	jb::Font* mFont = nullptr;       // +0xfc
	int mLine = 0;                   // +0x100 the line fading in, 1-based; 0 the picture's frame
	int mAlpha = 0;                  // +0x104
	int mDelay = 1000;               // +0x108 before the first fade
	bool mFirst = true;              // +0x10c

	// 0x412b80
	EndPicture() {}
	// 0x412c20, 0x412c40
	~EndPicture() override {}

	void Draw() override;    // 0x412cb0
	void Init() override;    // 0x412df0
	bool Update() override;  // 0x412ef0
};

// 0x412fe0 (vtable 0x51d7d0, 0x54 bytes)
class EndLayer : public jb::Layer
{
public:
	EndPicture* mPicture = nullptr;  // +0x50

	// 0x412fe0
	EndLayer() : Layer(0, 0) {}
	// 0x413000, 0x413020
	~EndLayer() override {}

	void Load() override;                 // 0x413030
	bool OnCommand(int theCmd) override;  // 0x4131a0
};

} // namespace mahjongchina

#endif
