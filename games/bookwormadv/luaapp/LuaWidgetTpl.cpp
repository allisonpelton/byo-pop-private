#include "LuaWidgetTpl.h"

using namespace Sexy;
using namespace LuaLib;

void Sexy::PushGraphics(Graphics* g)
{
	gLuaSupport->PushObject(ClassOf<Graphics>::info, g);
}

// port: the Lua state (the original's methods read it inline)
static lua_State* LS()
{
	return gLuaSupport->L;
}

// 0x40cb60 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ButtonPress(int theId, int theClickCount)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theClickCount);
	CallLinkedMethod("ButtonPress", 2);
}

// 0x40cbe0 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ButtonDepress(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonDepress", 1);
}

// 0x40cdd0 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ButtonDownTick(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonDownTick", 1);
}

// 0x40ccc0 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ButtonMouseEnter(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonMouseEnter", 1);
}

// 0x40cd20 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ButtonMouseLeave(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonMouseLeave", 1);
}

// 0x40cd80 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ButtonMouseMove(int theId, int theX, int theY)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theX);
	lua_pushint(LS(), theY);
	CallLinkedMethod("ButtonMouseMove", 3);
}

// 0x40ce70 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::EditWidgetText(int theId, const SexyString& theString)
{
	lua_pushint(LS(), theId);
	lua_pushlstring(LS(), theString.data(), theString.size());
	CallLinkedMethod("EditWidgetText", 2);
}

// 0x40cee0 (LuaWidgetTpl<Widget>'s)
bool LuaWidgetTplBase::AllowKey(int theId, KeyCode theKey)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), (int)theKey);
	return CallLinkedMethod("AllowKey", 2).Bool();
}

// 0x40cf60 (LuaWidgetTpl<Widget>'s)
bool LuaWidgetTplBase::AllowChar(int theId, SexyChar theChar)
{
	char s[2] = {(char)theChar, 0};
	lua_pushint(LS(), theId);
	lua_pushstring(LS(), s);
	return CallLinkedMethod("AllowChar", 2).Bool();
}

// 0x40cfe0 (LuaWidgetTpl<Widget>'s)
bool LuaWidgetTplBase::AllowText(int theId, const SexyString& theText)
{
	lua_pushint(LS(), theId);
	lua_pushlstring(LS(), theText.data(), theText.size());
	return CallLinkedMethod("AllowText", 2).Bool();
}

// 0x40f6b0
void LuaWidgetTplBase::ScrollPosition(int theId, double thePosition)
{
	lua_pushint(LS(), theId);
	lua_pushnumber(LS(), thePosition);
	CallLinkedMethod("ScrollPosition", 2);
}

// 0x40cc40 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::CheckboxChecked(int theId, bool checked)
{
	lua_pushint(LS(), theId);
	lua_pushboolean(LS(), checked);
	CallLinkedMethod("CheckboxChecked", 2);
}

// 0x40d050 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ListClicked(int theId, int theIdx, int theClickCount)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theIdx);
	lua_pushint(LS(), theClickCount);
	CallLinkedMethod("ListClicked", 3);
}

// 0x40d0a0 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ListClosed(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ListClosed", 1);
}

// 0x40d100 (LuaWidgetTpl<Widget>'s)
void LuaWidgetTplBase::ListHiliteChanged(int theId, int theOldIdx, int theNewIdx)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theOldIdx);
	lua_pushint(LS(), theNewIdx);
	CallLinkedMethod("ListHiliteChanged", 3);
}
