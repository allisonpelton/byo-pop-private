#include "LuaLinkedClass.h"

using namespace LuaLib;

static const LuaClassRet::Value kNone;

bool LuaClassRet::Bool() const
{
	if (count < 1 || v[0].type != TBool) {
		gLuaSupport->ReportError("Invalid Lua Result: Expected boolean in LuaClassRet");
		return false;
	}
	return v[0].b;
}

// 0x6c96c0
int LuaClassRet::Int() const
{
	return count > 0 && (v[0].type == TNumber || v[0].type == TInt) ? (int)v[0].num : 0;
}

// 0x6c97a0
const std::string& LuaClassRet::String() const
{
	return count > 0 && v[0].type == TString ? v[0].str : kNone.str;
}

// 0x6c99f0
LuaLinkedClass::LuaLinkedClass() {}

// 0x6c9a90
LuaLinkedClass::~LuaLinkedClass()
{
	DeleteLinkedObject();
}

// port: the push of the linked object that the original's CreateLinkedObject (0x6c9b30), RegisterEnums (0x6c9e10) and DeleteLinkedObject (0x6c9ce0) each make inline
void LuaLinkedClass::PushLinkedObject()
{
	LuaSupport* s = gLuaSupport;
	if (mLinkedObj == nullptr || mLinkedName == nullptr) {
		lua_pushnil(s->L);
		return;
	}
	s->PushObject(s->FindClass(mLinkedName), mLinkedObj);
}

void LuaLinkedClass::CreateLinkedObject(void* obj)
{
	LuaSupport* s = gLuaSupport;
	mLinkedName = LuaGetRegisteredName();
	mLinkedObj = obj;
	lua_pushstring(s->L, mLinkedName);
	PushLinkedObject();
	s->Call("CreateLinkedObject", 2, 0);
	RegisterEnums();
}

void LuaLinkedClass::RegisterEnums()
{
	if (mEnums.empty())
		return;
	LuaSupport* aSupport = gLuaSupport;
	lua_State* L = aSupport->L;
	auto aSet = [&]() {
		if (!lua_istable(L, -1))
			return;
		for (const auto& anEnum : mEnums) {
			lua_pushstring(L, anEnum.first.c_str());
			lua_pushint(L, anEnum.second);
			lua_settable(L, -3);
		}
	};
	lua_getglobal(L, "gLinkedObjectTable");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		aSupport->ReportError("Unable to locate gLinkedObjectTable in LuaLinkedClass::RegisterEnums");
		return;
	}
	PushLinkedObject();
	lua_gettable(L, -2);  // the Lua object
	aSet();
	lua_pop(L, 2);
	lua_getglobal(L, mLinkedName);  // the Lua class
	aSet();
	lua_pop(L, 1);
	// (the original then did the same for a second list of the class's, which no class here has)
}

void LuaLinkedClass::DeleteLinkedObject()
{
	if (mLinkedObj == nullptr)
		return;
	LuaSupport* s = gLuaSupport;
	if (s != nullptr && s->L != nullptr) {
		PushLinkedObject();
		s->Call("DeleteLinkedObject", 1, 0);
		s->ObjectDeleted(mLinkedObj);
	}
	mLinkedObj = nullptr;
}

LuaClassRet LuaLinkedClass::CallLinkedMethod(const char* method, int nargs)
{
	LuaClassRet r;
	LuaSupport* s = gLuaSupport;
	if (s == nullptr || s->L == nullptr)  // the app's state is gone (widgets deleted after it)
		return r;
	lua_State* L = s->L;
	if (mLinkedName == nullptr || mLinkedObj == nullptr) {
		lua_pop(L, nargs);  // the original leaves them on the stack
		s->ReportError("Linked call made on unregistered object.  Call RegisterObject first.");
		return r;
	}
	// CallLinkedMethod(cobj, method, args...)
	PushLinkedObject();
	lua_insert(L, -(nargs + 1));
	lua_pushstring(L, method);
	lua_insert(L, -(nargs + 1));
	return CallLua("CallLinkedMethod", nargs + 2);
}

// 0x6c9270 (LuaClassRet's constructor, which reads the results off the stack) is inline here
LuaClassRet LuaLib::CallLua(const std::string& path, int nargs)
{
	LuaClassRet aResult;
	LuaSupport* aSupport = gLuaSupport;
	lua_State* L = aSupport->L;
	if (!aSupport->Call(path, nargs, LUA_MULTRET))
		return aResult;
	int aCount = aSupport->Results();
	for (int i = 0; i < aCount && i < 8; i++) {
		int idx = -aCount + i;
		LuaClassRet::Value& v = aResult.v[i];
		switch (lua_type(L, idx)) {
		case LUA_TNUMBER: v.type = LuaClassRet::TNumber; v.num = lua_tonumber(L, idx); break;
		case LUA_TINT: v.type = LuaClassRet::TInt; v.num = lua_toint(L, idx); break;
		case LUA_TSTRING: v.type = LuaClassRet::TString; v.str = lua_tostring(L, idx); break;
		case LUA_TBOOLEAN: v.type = LuaClassRet::TBool; v.b = lua_toboolean(L, idx) != 0; break;
		default: aSupport->ReportError("Invalid Lua Result: Invalid return type in LuaClassRet"); break;
		}
	}
	aResult.count = aCount < 8 ? aCount : 8;
	lua_pop(L, aCount);
	return aResult;
}
