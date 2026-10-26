#include "LuaWidgetTpl.h"

using namespace Sexy;
using namespace LuaLib;

void Sexy::PushGraphics(Graphics* g)
{
	gLuaSupport->PushObject(ClassOf<Graphics>::info, g);
}

static lua_State* LS()
{
	return gLuaSupport->L;
}

void LuaWidgetTplBase::ButtonPress(int theId, int theClickCount)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theClickCount);
	CallLinkedMethod("ButtonPress", 2);
}

void LuaWidgetTplBase::ButtonDepress(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonDepress", 1);
}

void LuaWidgetTplBase::ButtonDownTick(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonDownTick", 1);
}

void LuaWidgetTplBase::ButtonMouseEnter(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonMouseEnter", 1);
}

void LuaWidgetTplBase::ButtonMouseLeave(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ButtonMouseLeave", 1);
}

void LuaWidgetTplBase::ButtonMouseMove(int theId, int theX, int theY)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theX);
	lua_pushint(LS(), theY);
	CallLinkedMethod("ButtonMouseMove", 3);
}

void LuaWidgetTplBase::EditWidgetText(int theId, const SexyString& theString)
{
	lua_pushint(LS(), theId);
	lua_pushlstring(LS(), theString.data(), theString.size());
	CallLinkedMethod("EditWidgetText", 2);
}

bool LuaWidgetTplBase::AllowKey(int theId, KeyCode theKey)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), (int)theKey);
	return CallLinkedMethod("AllowKey", 2).Bool();
}

bool LuaWidgetTplBase::AllowChar(int theId, SexyChar theChar)
{
	char s[2] = {(char)theChar, 0};
	lua_pushint(LS(), theId);
	lua_pushstring(LS(), s);
	return CallLinkedMethod("AllowChar", 2).Bool();
}

bool LuaWidgetTplBase::AllowText(int theId, const SexyString& theText)
{
	lua_pushint(LS(), theId);
	lua_pushlstring(LS(), theText.data(), theText.size());
	return CallLinkedMethod("AllowText", 2).Bool();
}

void LuaWidgetTplBase::ScrollPosition(int theId, double thePosition)
{
	lua_pushint(LS(), theId);
	lua_pushnumber(LS(), thePosition);
	CallLinkedMethod("ScrollPosition", 2);
}

void LuaWidgetTplBase::CheckboxChecked(int theId, bool checked)
{
	lua_pushint(LS(), theId);
	lua_pushboolean(LS(), checked);
	CallLinkedMethod("CheckboxChecked", 2);
}

void LuaWidgetTplBase::ListClicked(int theId, int theIdx, int theClickCount)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theIdx);
	lua_pushint(LS(), theClickCount);
	CallLinkedMethod("ListClicked", 3);
}

void LuaWidgetTplBase::ListClosed(int theId)
{
	lua_pushint(LS(), theId);
	CallLinkedMethod("ListClosed", 1);
}

void LuaWidgetTplBase::ListHiliteChanged(int theId, int theOldIdx, int theNewIdx)
{
	lua_pushint(LS(), theId);
	lua_pushint(LS(), theOldIdx);
	lua_pushint(LS(), theNewIdx);
	CallLinkedMethod("ListHiliteChanged", 3);
}
