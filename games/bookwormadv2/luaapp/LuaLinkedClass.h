// LuaLib::LuaLinkedClass (vtable 0x7c16ec, 0x6c99f0-0x6caa60): a C++ object with a Lua object of its own. The
// scripts' classlink.lua keeps the pairs: CreateLinkedObject(className, cobj) makes the Lua object (unless the
// script already linked one with SetLinkedObject), CallLinkedMethod(cobj, name, ...) calls one of its methods
// and DeleteLinkedObject(cobj) drops it. C++ widgets, the app and the game's engine classes derive from it so
// that their virtual functions can be written in Lua.
#pragma once

#include <map>
#include <string>

#include "LuaSupport.h"

namespace LuaLib {

// The results of a call into Lua (the original's LuaClassRet, 0x6c9270): up to 8 numbers, strings or booleans.
struct LuaClassRet {
	enum Type { TNone, TNumber, TInt, TString, TBool };
	struct Value {
		Type type = TNone;
		double num = 0;
		std::string str;
		bool b = false;
	};
	Value v[8];
	int count = 0;

	// The first result as a boolean (0x6c94d0); anything else is an error and false.
	bool Bool() const;
	int Int() const;
	double Number() const;
	const std::string& String() const;
};

// 0x6cb190: calls the function at `path' (as LuaSupport::Call) with the `nargs' values on top of the stack and
// returns its results
LuaClassRet CallLua(const std::string& path, int nargs);

class LuaLinkedClass {
public:
	LuaLinkedClass();
	virtual ~LuaLinkedClass();

	// vf01: the class the object is pushed as (CClasses.<name>; also the Lua class classlink.lua makes)
	virtual const char* LuaGetRegisteredName() = 0;

	// vf03 (0x6c9b30): links `obj' (the object of the registered class this is part of) to a Lua object.
	void CreateLinkedObject(void* obj);
	// vf04 (0x6c9ce0): drops the Lua object and marks the userdata of the object deleted.
	void DeleteLinkedObject();
	// vf08 (0x6caa60): calls the Lua object's method with the `nargs' values on top of the stack.
	LuaClassRet CallLinkedMethod(const char* method, int nargs);

	bool IsLinked() const { return mLinkedObj != nullptr; }
	void* GetLinkedObject() const { return mLinkedObj; }
	// Pushes the object's userdata (nil while it is not linked)
	void PushLinkedObject();

	// The class's constants (ButtonId_Back = 0, ...), set on the Lua object and on the Lua class (the global of
	// the class's name) when the object is linked
	std::map<std::string, int> mEnums;   // +0c

protected:
	void* mLinkedObj = nullptr;          // +04
	const char* mLinkedName = nullptr;   // +08

private:
	// vf07 (0x6ca560)
	void RegisterEnums();
};

}  // namespace LuaLib
