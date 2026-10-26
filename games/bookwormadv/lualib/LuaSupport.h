// LuaLib::LuaSupport (0x6c9a00-0x6ee000, vtable 0x7c758c): the PopLua state of a PopCap Lua game, the registry
// of the C++ classes scripts can use, the userdata ("cobj") that stand for C++ objects in Lua, and calls from
// C++ into Lua by dotted path. LuaBind.h binds C++ methods to Lua with templates, as the original's
// MethodWrapper<> did. Nothing here is specific to one game.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "lauxlib.h"
#include "lua.h"

namespace LuaLib {

struct ClassInfo;
using Upcast = void* (*)(void*);  // a pointer to an object of a class to its base class's part
using Destroy = void (*)(void*);  // deletes an object (of the class itself)
using Thunk = int (*)(lua_State* L, void* self);

struct ClassInfo {
	std::string name;
	std::vector<std::pair<ClassInfo*, Upcast>> bases;  // in the order SetBaseClass added them
	Destroy destroy = nullptr;                          // for objects Lua owns
};

// The block of a C++ object's full userdata. The original's (0x14 bytes) holds the object, the userdata itself,
// the owned flag, the class name and the class.
struct CObj {
	void* obj;       // NULL once the object has been deleted from C++ (ObjectDeleted)
	ClassInfo* cls;  // the class it was pushed as
	bool owned;      // collecting the userdata deletes the object
};

class LuaSupport {
public:
	lua_State* L = nullptr;

	LuaSupport();
	virtual ~LuaSupport();

	// The state with PopLua's libraries, CClasses and the sexy table (0x6d02a0 pushes the registry into it).
	void Open();
	void Close();

	// ---- classes (0x6d1720, 0x6d1cd0, 0x6d2400) ----
	// CClasses.<name>: the class table, also the metatable of its objects (registry[name], as
	// luaL_newmetatable makes it), with __index (itself), __gc, __eq and __cobj_compare.
	ClassInfo* CreateClass(const std::string& name, Destroy destroy);
	// The derived class table gets the metatable {__index = base class table}; `up' converts pointers.
	void SetBaseClass(ClassInfo* derived, ClassInfo* base, Upcast up);
	// CClasses.<class>.<name> = a closure that checks self, removes it from the stack and calls `thunk'.
	void AddMethod(ClassInfo* cls, const std::string& name, Thunk thunk);
	ClassInfo* FindClass(const std::string& name);

	// ---- objects (0x6d2830, 0x6d2d30, 0x6cf460) ----
	// Pushes the userdata of `obj' as an object of `cls' (nil for NULL); an object pushed again as the same class
	// gets the same userdata while Lua keeps it alive.
	void PushObject(ClassInfo* cls, void* obj, bool owned = false);
	// The object at idx as a pointer to `cls' (through its base classes), or NULL if it is no object of that
	// class. Deleted objects give NULL with *deleted set.
	void* ToObject(int idx, ClassInfo* cls, bool* deleted = nullptr);
	// For arguments: nil (or none) gives NULL, an object of another class raises the argument error.
	void* CheckObject(int idx, ClassInfo* cls);
	// A C++ object is gone: its userdata stay, but using them raises an error.
	void ObjectDeleted(void* obj);
	// Lua no longer owns the object (C++ takes it over), or now does.
	void SetOwned(void* obj, bool owned);

	// ---- paths and calls (0x6d0dd0, 0x6d0e40, 0x6d0a30, vf08 0x6d77e0) ----
	// Pushes the value at a dotted path from the globals ("main.LoadResources"); false (and nothing pushed)
	// when a part of the way is no table.
	bool PushPath(const std::string& path);
	// Pops a value into a dotted path, making the missing tables on the way.
	void SetPath(const std::string& path);
	// A plain C function at a dotted path ("Rand", "sexy.PlaySample"; 0x6d1630)
	void RegisterFunction(const std::string& path, lua_CFunction f)
	{
		lua_pushcfunction(L, f);
		SetPath(path);
	}
	// Calls the function at `path' with the `nargs' values on top of the stack. "a.b:Method" calls a method
	// of the table a.b. With nresults >= 0 that many results stay on the stack; LUA_MULTRET keeps them all
	// and Results() says how many. Errors go to ReportError; false then, with nothing left on the stack.
	bool Call(const std::string& path, int nargs = 0, int nresults = 0);
	int Results() const { return mResults; }
	// Runs a script file (LuaInterface's DoFile, 0x4c5050).
	bool DoFile(const std::string& path);
	// A Lua error with its traceback (the original shows a dialog and ends the game; vf02).
	virtual void ReportError(const std::string& message);

private:
	static int GC(lua_State* L);
	static int Compare(lua_State* L);
	static int Trampoline(lua_State* L);
	int PCall(int nargs, int nresults);

	std::unordered_map<std::string, ClassInfo*> mClasses;
	std::unordered_multimap<void*, CObj*> mObjects;  // the live userdata of each object
	int mResults = 0;
};

extern LuaSupport* gLuaSupport;

}  // namespace LuaLib
