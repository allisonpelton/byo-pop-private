// Sexy::PopLua (constructor 0x66f510, 0x2c8 bytes, derived from LuaLib::LuaSupport): the Lua state of a PopCap
// Lua game with the framework's own classes bound: sexy.Graphics, sexy.WidgetContainer, sexy.Widget and
// sexy.WidgetManager, and the free functions Rand, sexy.PlaySample, PushXMLFileData, StringToInt and
// StringWrapAnalyze. Most Graphics drawing functions read their own arguments, since scripts call them several
// ways (0x669100-0x66ce40); the widget ones are field getters and setters (0x66e490-0x66f4d0).
#pragma once

#include "LuaSupport.h"
#include "Rect.h"

namespace Sexy {

class PopLua : public LuaLib::LuaSupport {
public:
	// Opens the state and binds the classes and functions (the original did this in its constructor)
	void OpenPopLua();

	// LuaSupport's hooks for the app (vf07, vf21) and its development features, which no script of
	// Bookworm Adventures uses: reloading the scripts at the end of the frame (vf14, done in vf21) and save
	// points of the globals (vf22, vf23). The original's vf07 closed its log file; here they all do nothing.
	virtual void AppShutdown() {}
	virtual void UpdateState() {}
	virtual void DeferReload(const std::string& thePath) {}
	virtual void CreateSavePoint(const std::string& theName) {}
	virtual void PopSavePoint(const std::string& theName) {}
};

// A rect argument at idx: a table {mX, mY, mWidth, mHeight} or four ints (0x669340)
Rect LuaCheckRect(lua_State* L, int idx);

}  // namespace Sexy
