// Sexy::LuaInterface (constructor 0x4c4fb0, 0x2d8 bytes, vtable 0x7b5584): Bookworm Adventures' Lua state, a
// PopLua with the game's own free functions in the sexy table (RegisterSexyFunctions 0x4c5a50), the
// conversation panel's (convpanel.*, 0x4c01b0) and the profile's (profile.*, 0x48cec0). One instance,
// gLuaInterface (0x825958), made by GameApp::Init; it is also the app's mLua.
#pragma once

#include <string>

#include "PopLua.h"

namespace Sexy {

class LuaInterface : public PopLua {
public:
	LuaInterface();

	// 0x4c5a50: sexy.PrintConsole and the other functions of the game (the constructor and GameApp::Init call it)
	void RegisterSexyFunctions();

	// vf02 (0x4c62f0): back to a window, then PopLua's report (the log), then the game says it has to end and
	// shuts down (0x44fe00)
	void ReportError(const std::string& theMessage) override;
};

extern LuaInterface* gLuaInterface;  // 0x825958

}  // namespace Sexy
