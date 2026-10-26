// The debug library (0x6f5e30-0x6f6f30): Lua 5.0.2's ldblib.c without hooks (sethook and gethook do nothing)
// and without the interactive debug prompt.
#include <cstring>

#include "lauxlib.h"

static void settabss(lua_State* L, const char* i, const char* v)
{
	lua_pushstring(L, i);
	lua_pushstring(L, v);
	lua_rawset(L, -3);
}

static void settabsi(lua_State* L, const char* i, int v)
{
	lua_pushstring(L, i);
	lua_pushint(L, v);
	lua_rawset(L, -3);
}

// 0x6f5e30
static int getinfo(lua_State* L)
{
	lua_Debug ar;
	const char* options = luaL_optstring(L, 2, "flnSu");
	if (lua_isnumber(L, 1)) {
		if (!lua_getstack(L, lua_toint(L, 1), &ar)) {
			lua_pushnil(L);  // level out of range
			return 1;
		}
	}
	else if (lua_isfunction(L, 1)) {
		lua_pushfstring(L, ">%s", options);
		options = lua_tostring(L, -1);
		lua_pushvalue(L, 1);
	}
	else
		return luaL_argerror(L, 1, "function or level expected");
	if (!lua_getinfo(L, options, &ar))
		return luaL_argerror(L, 2, "invalid option");
	lua_newtable(L);
	for (; *options; options++) {
		switch (*options) {
		case 'S':
			settabss(L, "source", ar.source);
			settabss(L, "short_src", ar.short_src);
			settabsi(L, "linedefined", ar.linedefined);
			settabss(L, "what", ar.what);
			break;
		case 'l':
			settabsi(L, "currentline", ar.currentline);
			break;
		case 'u':
			settabsi(L, "nups", ar.nups);
			break;
		case 'n':
			settabss(L, "name", ar.name);
			settabss(L, "namewhat", ar.namewhat);
			break;
		case 'f':
			lua_pushliteral(L, "func");
			lua_pushvalue(L, -3);
			lua_rawset(L, -3);
			break;
		}
	}
	return 1;  // return table
}

static int getlocal(lua_State* L)
{
	lua_Debug ar;
	const char* name;
	if (!lua_getstack(L, luaL_checkint(L, 1), &ar))  // level out of range?
		return luaL_argerror(L, 1, "level out of range");
	name = lua_getlocal(L, &ar, luaL_checkint(L, 2));
	if (name) {
		lua_pushstring(L, name);
		lua_pushvalue(L, -2);
		return 2;
	}
	else {
		lua_pushnil(L);
		return 1;
	}
}

static int setlocal(lua_State* L)
{
	lua_Debug ar;
	if (!lua_getstack(L, luaL_checkint(L, 1), &ar))  // level out of range?
		return luaL_argerror(L, 1, "level out of range");
	luaL_checkany(L, 3);
	lua_pushstring(L, lua_setlocal(L, &ar, luaL_checkint(L, 2)));
	return 1;
}

static int auxupvalue(lua_State* L, int get)
{
	const char* name;
	int n = luaL_checkint(L, 2);
	luaL_checktype(L, 1, LUA_TFUNCTION);
	if (lua_iscfunction(L, 1))
		return 0;  // cannot touch C upvalues from Lua
	name = get ? lua_getupvalue(L, 1, n) : lua_setupvalue(L, 1, n);
	if (name == NULL)
		return 0;
	lua_pushstring(L, name);
	lua_insert(L, -(get + 1));
	return get + 1;
}

static int getupvalue(lua_State* L)
{
	return auxupvalue(L, 1);
}

static int setupvalue(lua_State* L)
{
	luaL_checkany(L, 3);
	return auxupvalue(L, 0);
}

static int sethook(lua_State* L)
{
	(void)L;
	return 0;
}

static int gethook(lua_State* L)
{
	lua_pushnil(L);
	return 1;
}

#define LEVELS1 12  // size of the first part of the stack
#define LEVELS2 10  // size of the second part of the stack

// 0x6f6be0
static int errorfb(lua_State* L)
{
	int level = 1;  // skip level 0 (it's this function)
	int firstpart = 1;  // still before eventual `...'
	lua_Debug ar;
	if (lua_gettop(L) == 0)
		lua_pushliteral(L, "");
	else if (!lua_isstring(L, 1))
		return 1;  // no string message
	else
		lua_pushliteral(L, "\n");
	lua_pushliteral(L, "stack traceback:");
	while (lua_getstack(L, level++, &ar)) {
		if (level > LEVELS1 && firstpart) {
			// no more than `LEVELS2' more levels?
			if (!lua_getstack(L, level + LEVELS2, &ar))
				level--;  // keep going
			else {
				lua_pushliteral(L, "\n\t...");  // too many levels
				while (lua_getstack(L, level + LEVELS2, &ar))  // find last levels
					level++;
			}
			firstpart = 0;
			continue;
		}
		lua_pushliteral(L, "\n\t");
		lua_getinfo(L, "Snl", &ar);
		lua_pushfstring(L, "%s:", ar.short_src);
		if (ar.currentline > 0)
			lua_pushfstring(L, "%d:", ar.currentline);
		switch (*ar.namewhat) {
		case 'g':  // global
		case 'l':  // local
		case 'f':  // field
		case 'm':  // method
			lua_pushfstring(L, " in function `%s'", ar.name);
			break;
		default: {
			if (*ar.what == 'm')  // main?
				lua_pushfstring(L, " in main chunk");
			else if (*ar.what == 'C' || *ar.what == 't')
				lua_pushliteral(L, " ?");  // C function or tail call
			else
				lua_pushfstring(L, " in function <%s:%d>", ar.short_src, ar.linedefined);
		}
		}
		lua_concat(L, lua_gettop(L));
	}
	lua_concat(L, lua_gettop(L));
	return 1;
}

static const luaL_reg dblib[] = {
	{"getlocal", getlocal},
	{"getinfo", getinfo},
	{"gethook", gethook},
	{"getupvalue", getupvalue},
	{"sethook", sethook},
	{"setlocal", setlocal},
	{"setupvalue", setupvalue},
	{"traceback", errorfb},
	{NULL, NULL}
};

// 0x6f6f30
int luaopen_debug(lua_State* L)
{
	luaL_openlib(L, LUA_DBLIBNAME, dblib, 0);
	lua_pushliteral(L, "_TRACEBACK");
	lua_pushcfunction(L, errorfb);
	lua_settable(L, LUA_GLOBALSINDEX);
	return 1;
}
