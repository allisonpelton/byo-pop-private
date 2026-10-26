// LuaLib::LuaSupport: see LuaSupport.h.
#include "LuaSupport.h"

#include <cstdio>
#include <cstring>

namespace LuaLib {

LuaSupport* gLuaSupport = nullptr;

LuaSupport::LuaSupport()
{
	gLuaSupport = this;
}

LuaSupport::~LuaSupport()
{
	Close();
	for (auto& c : mClasses)
		delete c.second;
	if (gLuaSupport == this)
		gLuaSupport = nullptr;
}

void LuaSupport::Open()
{
	L = lua_open();
	lua_CFunction libs[] = {luaopen_base, luaopen_table, luaopen_io, luaopen_string, luaopen_math, luaopen_debug,
		luaopen_support};
	for (lua_CFunction f : libs) {
		f(L);
		lua_settop(L, 0);
	}
	lua_pushliteral(L, "CClasses");
	lua_newtable(L);
	lua_settable(L, LUA_GLOBALSINDEX);
	// sexy.__cobj_compare: the __eq of every class
	lua_pushcfunction(L, Compare);
	SetPath("sexy.__cobj_compare");
}

void LuaSupport::Close()
{
	if (L != nullptr) {
		lua_close(L);  // collects every userdata: the owned objects are deleted
		L = nullptr;
	}
	mObjects.clear();
}

// ---- classes ----

ClassInfo* LuaSupport::FindClass(const std::string& name)
{
	auto it = mClasses.find(name);
	return it == mClasses.end() ? nullptr : it->second;
}

ClassInfo* LuaSupport::CreateClass(const std::string& name, Destroy destroy)
{
	ClassInfo* info = FindClass(name);
	if (info == nullptr) {
		info = new ClassInfo;
		info->name = name;
		mClasses[name] = info;
	}
	info->destroy = destroy;
	luaL_newmetatable(L, name.c_str());
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, GC);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__eq");
	lua_pushcfunction(L, Compare);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__cobj_compare");
	lua_pushcfunction(L, Compare);
	lua_rawset(L, -3);
	SetPath("CClasses." + name);
	return info;
}

void LuaSupport::SetBaseClass(ClassInfo* derived, ClassInfo* base, Upcast up)
{
	derived->bases.emplace_back(base, up);
	PushPath("CClasses." + derived->name);
	lua_newtable(L);
	lua_pushliteral(L, "__index");
	PushPath("CClasses." + base->name);
	lua_rawset(L, -3);
	lua_setmetatable(L, -2);
	lua_pop(L, 1);
}

void LuaSupport::AddMethod(ClassInfo* cls, const std::string& name, Thunk thunk)
{
	PushPath("CClasses." + cls->name);
	lua_pushstring(L, name.c_str());
	lua_pushlightuserdata(L, cls);
	lua_pushlightuserdata(L, (void*)thunk);
	lua_pushcclosure(L, Trampoline, 2);
	lua_rawset(L, -3);
	lua_pop(L, 1);
}

// ---- objects ----

void LuaSupport::PushObject(ClassInfo* cls, void* obj, bool owned)
{
	if (obj == nullptr) {
		lua_pushnil(L);
		return;
	}
	auto range = mObjects.equal_range(obj);
	for (auto it = range.first; it != range.second; ++it)
		if (it->second->cls == cls) {
			lua_pushudata(L, it->second);
			return;
		}
	CObj* o = (CObj*)lua_newuserdata(L, sizeof(CObj));
	o->obj = obj;
	o->cls = cls;
	o->owned = owned;
	luaL_getmetatable(L, cls->name.c_str());
	lua_setmetatable(L, -2);
	mObjects.emplace(obj, o);
}

// The pointer of an object of class `from' as a pointer to `to', through the base classes, or NULL if `to'
// is none of them (0x6d2d30 walks the class infos the same way)
static void* Convert(void* p, ClassInfo* from, ClassInfo* to)
{
	if (from == to)
		return p;
	for (auto& b : from->bases) {
		void* q = Convert(b.second(p), b.first, to);
		if (q != nullptr)
			return q;
	}
	return nullptr;
}

void* LuaSupport::ToObject(int idx, ClassInfo* cls, bool* deleted)
{
	if (deleted != nullptr)
		*deleted = false;
	if (lua_type(L, idx) != LUA_TUSERDATA || !lua_getmetatable(L, idx))
		return nullptr;
	lua_pushliteral(L, "__cobj_compare");
	lua_rawget(L, -2);
	bool isobj = lua_tocfunction(L, -1) == Compare;
	lua_pop(L, 2);
	if (!isobj)
		return nullptr;
	CObj* o = (CObj*)lua_touserdata(L, idx);
	if (o->obj == nullptr) {
		if (deleted != nullptr)
			*deleted = true;
		return nullptr;
	}
	return Convert(o->obj, o->cls, cls);
}

void* LuaSupport::CheckObject(int idx, ClassInfo* cls)
{
	if (lua_isnoneornil(L, idx))
		return nullptr;
	bool deleted;
	void* p = ToObject(idx, cls, &deleted);
	if (deleted)
		luaL_error(L, "Deleted object of type '%s' can no longer be accessed",
			((CObj*)lua_touserdata(L, idx))->cls->name.c_str());
	if (p == nullptr)
		luaL_typerror(L, idx, cls->name.c_str());
	return p;
}

void LuaSupport::ObjectDeleted(void* obj)
{
	// the userdata leave the map: a new object at the same address gets new ones
	auto range = mObjects.equal_range(obj);
	for (auto it = range.first; it != range.second; ++it) {
		it->second->obj = nullptr;
		it->second->owned = false;
	}
	mObjects.erase(obj);
}

void LuaSupport::SetOwned(void* obj, bool owned)
{
	auto range = mObjects.equal_range(obj);
	for (auto it = range.first; it != range.second; ++it)
		it->second->owned = owned;
}

// 0x6cf460 (__gc): forget the userdata; an owned object is deleted with it
int LuaSupport::GC(lua_State* L)
{
	LuaSupport* self = gLuaSupport;
	CObj* o = (CObj*)lua_touserdata(L, 1);
	if (o == nullptr || self == nullptr || o->obj == nullptr)
		return 0;
	auto range = self->mObjects.equal_range(o->obj);
	for (auto it = range.first; it != range.second; ++it)
		if (it->second == o) {
			self->mObjects.erase(it);
			break;
		}
	if (o->obj != nullptr && o->owned && o->cls->destroy != nullptr) {
		void* obj = o->obj;
		o->obj = nullptr;
		o->cls->destroy(obj);
	}
	return 0;
}

// __eq and __cobj_compare: two userdata stand for the same object
int LuaSupport::Compare(lua_State* L)
{
	CObj* a = (CObj*)lua_touserdata(L, 1);
	CObj* b = (CObj*)lua_touserdata(L, 2);
	lua_pushboolean(L, a != nullptr && b != nullptr && a->obj == b->obj);
	return 1;
}

// 0x6cf370: every bound method; upvalues: the class, the thunk
int LuaSupport::Trampoline(lua_State* L)
{
	ClassInfo* cls = (ClassInfo*)lua_touserdata(L, lua_upvalueindex(1));
	Thunk thunk = (Thunk)lua_touserdata(L, lua_upvalueindex(2));
	bool deleted;
	void* self = gLuaSupport->ToObject(1, cls, &deleted);
	if (deleted)
		return luaL_error(L, "Deleted object of type '%s' can no longer be accessed",
			((CObj*)lua_touserdata(L, 1))->cls->name.c_str());
	if (self == nullptr)
		return luaL_typerror(L, 1, cls->name.c_str());
	lua_remove(L, 1);
	return thunk(L, self);
}

// ---- paths and calls ----

bool LuaSupport::PushPath(const std::string& path)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	size_t start = 0;
	for (;;) {
		size_t dot = path.find('.', start);
		std::string part = path.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			return false;
		}
		lua_pushstring(L, part.c_str());
		lua_gettable(L, -2);
		lua_remove(L, -2);
		if (dot == std::string::npos)
			return true;
		start = dot + 1;
	}
}

void LuaSupport::SetPath(const std::string& path)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	size_t start = 0;
	for (;;) {
		size_t dot = path.find('.', start);
		std::string part = path.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
		lua_pushstring(L, part.c_str());
		if (dot == std::string::npos) {
			lua_pushvalue(L, -3);
			lua_settable(L, -3);
			lua_pop(L, 2);
			return;
		}
		lua_pushvalue(L, -1);
		lua_gettable(L, -3);
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			lua_newtable(L);
			lua_pushvalue(L, -2);
			lua_pushvalue(L, -2);
			lua_settable(L, -5);
		}
		lua_remove(L, -2);
		lua_remove(L, -2);
		start = dot + 1;
	}
}

static int Traceback(lua_State* L)
{
	lua_pushliteral(L, "_TRACEBACK");
	lua_rawget(L, LUA_GLOBALSINDEX);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	return 1;
}

// The function and its nargs arguments are on top: calls it with the traceback handler; 0 or an error status
// (the message reported and popped)
int LuaSupport::PCall(int nargs, int nresults)
{
	int base = lua_gettop(L) - nargs;  // the function
	lua_pushcfunction(L, Traceback);
	lua_insert(L, base);
	int status = lua_pcall(L, nargs, nresults, base);
	lua_remove(L, base);
	if (status != 0) {
		const char* msg = lua_tostring(L, -1);
		std::string m = msg != nullptr ? msg : "(error object is not a string)";
		lua_pop(L, 1);
		ReportError(m);
	} else
		mResults = lua_gettop(L) - base + 1;
	return status;
}

bool LuaSupport::Call(const std::string& path, int nargs, int nresults)
{
	mResults = 0;
	size_t colon = path.find(':');
	int extra = 0;
	if (colon == std::string::npos) {
		bool found = PushPath(path);
		if (!found || !lua_isfunction(L, -1)) {
			lua_pop(L, nargs + (found ? 1 : 0));
			ReportError("LuaSupport::Call: no function " + path);
			return false;
		}
	} else {
		if (!PushPath(path.substr(0, colon))) {
			lua_pop(L, nargs);
			ReportError("LuaSupport::Call: no object " + path.substr(0, colon));
			return false;
		}
		lua_pushstring(L, path.substr(colon + 1).c_str());
		lua_gettable(L, -2);
		if (!lua_isfunction(L, -1)) {
			lua_pop(L, 2 + nargs);
			ReportError("LuaSupport::Call: no method " + path);
			return false;
		}
		lua_insert(L, -2);  // function, self
		extra = 1;
	}
	// the function (and self) go under the arguments
	for (int i = 0; i <= extra; i++)
		lua_insert(L, -(nargs + 1 + extra));
	return PCall(nargs + extra, nresults) == 0;
}

bool LuaSupport::DoFile(const std::string& path)
{
	int status = luaL_loadfile(L, path.c_str());
	if (status != 0) {
		const char* msg = lua_tostring(L, -1);
		std::string m = msg != nullptr ? msg : path;
		lua_pop(L, 1);
		ReportError(m);
		return false;
	}
	return PCall(0, 0) == 0;
}

void LuaSupport::ReportError(const std::string& message)
{
	fprintf(stderr, "Lua error: %s\n", message.c_str());
}

}  // namespace LuaLib
