// LuaSupport (0x6cd380-0x6cec80, registered from the table at 0x79be80): the globals the game adds to PopLua.
// print writes like the base library's (the original also echoes to the game's debug console), require
// completes the name with findfullpath first, and setprot, getprot and getclass reach PopLua's table
// extensions. The script debugger's commands (wait, display, go, step, ub) and tostr, getobj, gettableat and
// checktype are not ported: no script calls them.
#include <cctype>

#include "lauxlib.h"

// 0x6cd380
static int ls_print(lua_State* L)
{
	int n = lua_gettop(L);
	lua_pushliteral(L, "tostring");
	lua_gettable(L, LUA_GLOBALSINDEX);
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for (int i = 1; i <= n; i++) {
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		const char* s = lua_tostring(L, -1);
		if (s == NULL)
			return luaL_error(L, "`tostring' must return a string to `print'");
		lua_pop(L, 1);
		if (i > 1)
			luaL_addlstring(&b, "\t", 1);
		luaL_addstring(&b, s);
	}
	luaL_addlstring(&b, "\n", 1);
	luaL_pushresult(&b);
	size_t len;
	const char* out = luaL_checklstring(L, -1, &len);
	poplua_print(out, len);
	return 0;
}

// 0x6cd6b0: require(name) of the path findfullpath finds for it (the name itself when it finds none)
static int ls_require(lua_State* L)
{
	luaL_checkstring(L, 1);
	lua_settop(L, 1);
	lua_pushliteral(L, "findfullpath");
	lua_gettable(L, LUA_GLOBALSINDEX);
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	if (lua_isstring(L, -1))
		lua_replace(L, 1);
	lua_settop(L, 1);
	return poplua_requirecore(L, 1);
}

// 0x6ce2f0: setprot(t) protects t fully (7); setprot(t, "ldirwc") sets the bits the letters name (l lock 1,
// i 2, d 4, r 8, w 0x10, c case-sensitive keys 0x20); setprot(t, n) sets n
static int ls_setprot(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	int prot = 7;
	if (lua_gettop(L) >= 2) {
		if (lua_type(L, 2) == LUA_TSTRING) {
			prot = 0;
			for (const char* s = lua_tostring(L, 2); *s; s++) {
				switch (tolower((unsigned char)*s)) {
				case 'l': prot |= 1; break;
				case 'i': prot |= 2; break;
				case 'd': prot |= 4; break;
				case 'r': prot |= 8; break;
				case 'w': prot |= 0x10; break;
				case 'c': prot |= 0x20; break;
				}
			}
		}
		else
			prot = (int)lua_ftol(lua_tonumber(L, 2));
	}
	lua_setprot(L, 1, prot);
	return 0;
}

// 0x6ce500
static int ls_getprot(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushnumber(L, (lua_Number)lua_getprot(L, 1));
	return 1;
}

// 0x6ce560: getclass("Name") is the class table of that name; getclass(obj) is the class an object was made
// from (its metatable's __index), or nil
static int ls_getclass(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TSTRING) {
		const char* name = lua_tostring(L, 1);
		lua_pushvalue(L, 1);
		lua_gettable(L, LUA_GLOBALSINDEX);
		if (lua_isnil(L, -1))
			return luaL_error(L, "Unable to locate class '%s'", name);
		return 1;
	}
	luaL_checktype(L, 1, LUA_TTABLE);
	if (!lua_getmetatable(L, 1)) {
		lua_pushnil(L);
		return 1;
	}
	lua_pushliteral(L, "__index");
	lua_rawget(L, -2);
	if (!lua_istable(L, -1))
		lua_pushnil(L);
	return 1;
}

static const luaL_reg support_funcs[] = {
	{"print", ls_print},
	{"require", ls_require},
	{"setprot", ls_setprot},
	{"getprot", ls_getprot},
	{"getclass", ls_getclass},
	{NULL, NULL}
};

int luaopen_support(lua_State* L)
{
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_openlib(L, NULL, support_funcs, 0);
	lua_pop(L, 1);
	return 0;
}
