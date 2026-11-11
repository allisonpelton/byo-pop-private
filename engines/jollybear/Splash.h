#ifndef __JB_SPLASH_H__
#define __JB_SPLASH_H__

// The engine's opening screens: a layer (0x42b6c0 (mahjongchina), 0x425320 (mahjongjapan), vtable
// 0x51d120 (mahjongchina), 0x51e03c (mahjongjapan)) holding the logo object (0x42f650 (mahjongjapan), vtable
// 0x51cef4 (mahjongchina), 0x51e448 (mahjongjapan)). The game's first layer is Splash(true): it shows the publisher's
// "SplashLogo" image from the exe's folder when there is one, then Splash(false) shows Jolly Bear's own logo; after two
// seconds of the last the app starts the game (App::StartGame, +0x1c).

#include "Layer.h"
#include "Object.h"

#include <string>

namespace jb
{

class Font;

// 0x42f650 (mahjongjapan)
class Logo : public Object
{
public:
	int mTime = 0xffff;       // +0x90 the milliseconds left (0xffff: not started)
	int mX = 0;               // +0x94 the SplashLogo image's place
	int mY = 0;               // +0x98
	bool mSplashLogo;         // +0x9c
	Font* mFont = nullptr;    // +0xa4 the version's

	// 0x42f650 (mahjongjapan)
	explicit Logo(bool theSplashLogo);
	// 0x42a340 (mahjongchina), 0x42f680 (mahjongjapan), 0x42a360 (mahjongchina), 0x42f6a0 (mahjongjapan)
	~Logo() override {}

	void Draw() override;     // 0x42f6b0 (mahjongjapan)
	void Init() override;     // 0x42f7f0 (mahjongjapan)
	bool Update() override;   // 0x42f8c0 (mahjongjapan)
};

// 0x42b6c0 (mahjongchina), 0x425320 (mahjongjapan)
class Splash : public Layer
{
public:
	bool mSplashLogo;         // +0x50 SplashLogo is wanted, then: it was loaded

	// 0x42b6c0 (mahjongchina), 0x425320 (mahjongjapan)
	explicit Splash(bool theSplashLogo);
	// 0x42b6e0 (mahjongchina), 0x425340 (mahjongjapan), 0x42b700 (mahjongchina), 0x425360 (mahjongjapan)
	~Splash() override {}

	void Load() override;     // 0x42b760 (mahjongchina), 0x425370 (mahjongjapan)
};

// 0x5d8148 (mahjongchina), 0x5d8e10 (mahjongjapan): "v" and the four parts of the exe's file version
// (0x42b600 (mahjongchina), 0x426850 (mahjongjapan)), drawn on Jolly Bear's logo; the host sets it
extern std::string gVersion;

} // namespace jb

#endif
