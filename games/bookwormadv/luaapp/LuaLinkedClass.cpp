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

int LuaClassRet::Int() const
{
	return count > 0 && (v[0].type == TNumber || v[0].type == TInt) ? (int)v[0].num : 0;
}

double LuaClassRet::Number() const
{
	return count > 0 && (v[0].type == TNumber || v[0].type == TInt) ? v[0].num : 0;
}

const std::string& LuaClassRet::String() const
{
	return count > 0 && v[0].type == TString ? v[0].str : kNone.str;
}

LuaLinkedClass::LuaLinkedClass() {}

LuaLinkedClass::~LuaLinkedClass()
{
	DeleteLinkedObject();
}

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
	// vf07 (0x6ca560) then registers the class's enums in gLinkedObjectTable; no class here has any
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
	if (!s->Call("CallLinkedMethod", nargs + 2, LUA_MULTRET))
		return r;
	int n = s->Results();
	for (int i = 0; i < n && i < 8; i++) {
		int idx = -n + i;
		LuaClassRet::Value& v = r.v[i];
		switch (lua_type(L, idx)) {
		case LUA_TNUMBER: v.type = LuaClassRet::TNumber; v.num = lua_tonumber(L, idx); break;
		case LUA_TINT: v.type = LuaClassRet::TInt; v.num = lua_toint(L, idx); break;
		case LUA_TSTRING: v.type = LuaClassRet::TString; v.str = lua_tostring(L, idx); break;
		case LUA_TBOOLEAN: v.type = LuaClassRet::TBool; v.b = lua_toboolean(L, idx) != 0; break;
		default: s->ReportError("Invalid Lua Result: Invalid return type in LuaClassRet"); break;
		}
	}
	r.count = n < 8 ? n : 8;
	lua_pop(L, n);
	return r;
}
