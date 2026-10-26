// The base library (0x6fe8b0-0x702490): Lua 5.0.2's lbaselib.c with PopLua's changes: tonumber always gives
// a number, type names ints "int", collectgarbage takes 5.1's options, unpack is 0-based, loadfile prefers
// the compiled .luc of a .lua, and the bit library is opened with it.
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lauxlib.h"

// 0x6feaa0: through tostring, written by the host's print hook
static int luaB_print(lua_State* L)
{
	int n = lua_gettop(L);  // number of arguments
	int i;
	luaL_Buffer b;
	lua_pushliteral(L, "tostring");
	lua_gettable(L, LUA_GLOBALSINDEX);
	luaL_buffinit(L, &b);
	for (i = 1; i <= n; i++) {
		const char* s;
		lua_pushvalue(L, -1);  // function to be called
		lua_pushvalue(L, i);  // value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  // get result
		if (s == NULL)
			return luaL_error(L, "`tostring' must return a string to `print'");
		lua_pop(L, 1);
		if (i > 1)
			luaL_addlstring(&b, "\t", 1);
		luaL_addstring(&b, s);
	}
	luaL_addlstring(&b, "\n", 1);
	luaL_pushresult(&b);
	poplua_print(lua_tostring(L, -1), lua_strlen(L, -1));
	return 0;
}

// 0x6fec60
static int luaB_tonumber(lua_State* L)
{
	int base = luaL_optint(L, 2, 10);
	if (base == 10) {  // standard conversion
		luaL_checkany(L, 1);
		if (lua_isnumber(L, 1)) {
			lua_pushnumber(L, lua_tonumber(L, 1));
			return 1;
		}
	}
	else {
		const char* s1 = luaL_checkstring(L, 1);
		char* s2;
		unsigned long n;
		luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
		n = strtoul(s1, &s2, base);
		if (s1 != s2) {  // at least one valid digit?
			while (isspace((unsigned char)(*s2)))
				s2++;  // skip trailing spaces
			if (*s2 == '\0') {  // no invalid trailing characters?
				lua_pushnumber(L, (lua_Number)(uint32_t)n);
				return 1;
			}
		}
	}
	lua_pushnil(L);  // else not a number
	return 1;
}

// 0x6fee90
static int luaB_error(lua_State* L)
{
	int level = luaL_optint(L, 2, 1);
	luaL_checkany(L, 1);
	if (!lua_isstring(L, 1) || level == 0)
		lua_pushvalue(L, 1);  // propagate error message without changes
	else {  // add extra information
		luaL_where(L, level);
		lua_pushvalue(L, 1);
		lua_concat(L, 2);
	}
	return lua_error(L);
}

static int luaB_getmetatable(lua_State* L)
{
	luaL_checkany(L, 1);
	if (!lua_getmetatable(L, 1)) {
		lua_pushnil(L);
		return 1;  // no metatable
	}
	luaL_getmetafield(L, 1, "__metatable");
	return 1;  // returns either __metatable field (if present) or metatable
}

// 0x6ff080
static int luaB_setmetatable(lua_State* L)
{
	int t = lua_type(L, 2);
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2, "nil or table expected");
	if (luaL_getmetafield(L, 1, "__metatable"))
		luaL_error(L, "cannot change a protected metatable");
	lua_settop(L, 2);
	lua_setmetatable(L, 1);
	return 1;
}

static void getfunc(lua_State* L)
{
	if (lua_isfunction(L, 1))
		lua_pushvalue(L, 1);
	else {
		lua_Debug ar;
		int level = luaL_optint(L, 1, 1);
		luaL_argcheck(L, level >= 0, 1, "level must be non-negative");
		if (lua_getstack(L, level, &ar) == 0)
			luaL_argerror(L, 1, "invalid level");
		lua_getinfo(L, "f", &ar);
		if (lua_isnil(L, -1))
			luaL_error(L, "no function environment for tail call at level %d", level);
	}
}

static int aux_getfenv(lua_State* L)
{
	lua_getfenv(L, -1);
	lua_pushliteral(L, "__fenv");
	lua_rawget(L, -2);
	return !lua_isnil(L, -1);
}

static int luaB_getfenv(lua_State* L)
{
	getfunc(L);
	if (!aux_getfenv(L))  // __fenv not defined?
		lua_pop(L, 1);  // remove it, to return real environment
	return 1;
}

static int luaB_setfenv(lua_State* L)
{
	luaL_checktype(L, 2, LUA_TTABLE);
	getfunc(L);
	if (aux_getfenv(L))  // __fenv defined?
		luaL_error(L, "`setfenv' cannot change a protected environment");
	else
		lua_pop(L, 2);  // remove __fenv and real environment table
	lua_pushvalue(L, 2);
	if (lua_isnumber(L, 1) && lua_tonumber(L, 1) == 0)
		lua_replace(L, LUA_GLOBALSINDEX);
	else if (lua_setfenv(L, -2) == 0)
		luaL_error(L, "`setfenv' cannot change environment of given function");
	return 0;
}

static int luaB_rawequal(lua_State* L)
{
	luaL_checkany(L, 1);
	luaL_checkany(L, 2);
	lua_pushboolean(L, lua_rawequal(L, 1, 2));
	return 1;
}

static int luaB_rawget(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checkany(L, 2);
	lua_rawget(L, 1);
	return 1;
}

static int luaB_rawset(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checkany(L, 2);
	luaL_checkany(L, 3);
	lua_rawset(L, 1);
	return 1;
}

static int luaB_gcinfo(lua_State* L)
{
	lua_pushnumber(L, (lua_Number)lua_getgccount(L));
	lua_pushnumber(L, (lua_Number)lua_getgcthreshold(L));
	return 2;
}

// 0x6ff700: 5.1's options, answered by this collector (which is not incremental): "step" collects
static int luaB_collectgarbage(lua_State* L)
{
	static const char* const opts[] = {"stop", "restart", "collect", "count", "step", "setpause", "setstepmul", NULL};
	static int pause = 200, stepmul = 200, stopped = 0;
	const char* o = luaL_optstring(L, 1, "collect");
	int o_i = luaL_findstring(o, opts);
	int ex = luaL_optint(L, 2, 0);
	int res = 0;
	if (o_i < 0)
		luaL_argerror(L, 1, "invalid option");
	switch (o_i) {
	case 0:  // stop
		stopped = 1;
		lua_setgcthreshold(L, 0x7fffffff >> 10);
		break;
	case 1:  // restart
		stopped = 0;
		lua_setgcthreshold(L, lua_getgccount(L));
		break;
	case 2:  // collect
	case 4:  // step
		lua_setgcthreshold(L, 0);
		res = (o_i == 4);
		if (stopped)
			lua_setgcthreshold(L, 0x7fffffff >> 10);
		break;
	case 3:  // count
		res = lua_getgccount(L);
		break;
	case 5:  // setpause
		res = pause;
		pause = ex;
		break;
	case 6:  // setstepmul
		res = stepmul;
		stepmul = ex;
		break;
	}
	lua_pushint(L, res);
	return 1;
}

// 0x6ff850
static int luaB_type(lua_State* L)
{
	luaL_checkany(L, 1);
	lua_pushstring(L, lua_typename(L, lua_type(L, 1)));
	return 1;
}

static int luaB_next(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 2);  // create a 2nd argument if there isn't one
	if (lua_next(L, 1))
		return 2;
	else {
		lua_pushnil(L);
		return 1;
	}
}

static int luaB_pairs(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushliteral(L, "next");
	lua_rawget(L, LUA_GLOBALSINDEX);  // return generator,
	lua_pushvalue(L, 1);  // state,
	lua_pushnil(L);  // and initial value
	return 3;
}

// 0-based: the first step gives index 0
static int luaB_ipairs(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	if (lua_isnone(L, 2)) {  // `for' start?
		lua_pushliteral(L, "ipairs");
		lua_rawget(L, LUA_GLOBALSINDEX);  // return generator,
		lua_pushvalue(L, 1);  // state,
		lua_pushint(L, -1);  // and initial value
		return 3;
	}
	else {  // `for' step
		int i = lua_toint(L, 2) + 1;  // next value
		lua_pushint(L, i);
		lua_rawgeti(L, 1, i);
		return (lua_isnil(L, -1)) ? 0 : 2;
	}
}

static int load_aux(lua_State* L, int status)
{
	if (status == 0)  // OK?
		return 1;
	else {
		lua_pushnil(L);
		lua_insert(L, -2);  // put before error message
		return 2;  // return nil plus error message
	}
}

// 0x6ffc20
static int luaB_loadstring(lua_State* L)
{
	size_t l;
	const char* s = luaL_checklstring(L, 1, &l);
	const char* chunkname = luaL_optstring(L, 2, s);
	return load_aux(L, luaL_loadbuffer(L, s, l, chunkname));
}

static int luaB_loadfile(lua_State* L)
{
	const char* fname = luaL_checkstring(L, 1);
	return load_aux(L, luaL_loadfile(L, fname));
}

// 0x6ffe10
static int luaB_dofile(lua_State* L)
{
	const char* fname = luaL_checkstring(L, 1);
	int n = lua_gettop(L);
	int status = luaL_loadfile(L, fname);
	if (status != 0)
		lua_error(L);
	lua_call(L, 0, LUA_MULTRET);
	return lua_gettop(L) - n;
}

// 0x6fffa0
static int luaB_assert(lua_State* L)
{
	luaL_checkany(L, 1);
	if (!lua_toboolean(L, 1))
		return luaL_error(L, "%s", luaL_optstring(L, 2, "assertion failed!"));
	lua_settop(L, 1);
	return 1;
}

// 0x700090
static int luaB_unpack(lua_State* L)
{
	int n, i;
	luaL_checktype(L, 1, LUA_TTABLE);
	n = luaL_getn(L, 1);
	luaL_checkstack(L, n, "table too big to unpack");
	for (i = 0; i < n; i++)  // push arg[0...n-1]
		lua_rawgeti(L, 1, i);
	return n;
}

static int luaB_pcall(lua_State* L)
{
	int status;
	luaL_checkany(L, 1);
	status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
	lua_pushboolean(L, (status == 0));
	lua_insert(L, 1);
	return lua_gettop(L);  // return status + all results
}

static int luaB_xpcall(lua_State* L)
{
	int status;
	luaL_checkany(L, 2);
	lua_settop(L, 2);
	lua_insert(L, 1);  // put error function under function to be called
	status = lua_pcall(L, 0, LUA_MULTRET, 1);
	lua_pushboolean(L, (status == 0));
	lua_replace(L, 1);
	return lua_gettop(L);  // return status + all results
}

// the original's "%p" (Visual C++ prints eight upper-case hex digits)
static void fmtpointer(char* buff, size_t size, const char* what, const void* p)
{
	snprintf(buff, size, "%s: %08llX", what, (unsigned long long)(uintptr_t)p);
}

// 0x7002f0
static int luaB_tostring(lua_State* L)
{
	char buff[64];
	luaL_checkany(L, 1);
	if (luaL_callmeta(L, 1, "__tostring"))  // is there a metafield?
		return 1;  // use its value
	switch (lua_type(L, 1)) {
	case LUA_TNUMBER:
	case LUA_TINT:
		lua_pushstring(L, lua_tostring(L, 1));
		return 1;
	case LUA_TSTRING:
		lua_pushvalue(L, 1);
		return 1;
	case LUA_TBOOLEAN:
		lua_pushstring(L, (lua_toboolean(L, 1) ? "true" : "false"));
		return 1;
	case LUA_TTABLE:
		fmtpointer(buff, sizeof(buff), "table", lua_topointer(L, 1));
		break;
	case LUA_TFUNCTION:
		fmtpointer(buff, sizeof(buff), "function", lua_topointer(L, 1));
		break;
	case LUA_TUSERDATA:
	case LUA_TLIGHTUSERDATA:
		fmtpointer(buff, sizeof(buff), "userdata", lua_touserdata(L, 1));
		break;
	case LUA_TTHREAD:
		fmtpointer(buff, sizeof(buff), "thread", (void*)lua_tothread(L, 1));
		break;
	case LUA_TNIL:
		lua_pushliteral(L, "nil");
		return 1;
	default:
		buff[0] = '\0';
		break;
	}
	lua_pushstring(L, buff);
	return 1;
}

static int luaB_newproxy(lua_State* L)
{
	lua_settop(L, 1);
	lua_newuserdata(L, 0);  // create proxy
	if (lua_toboolean(L, 1) == 0)
		return 1;  // no metatable
	else if (lua_isboolean(L, 1)) {
		lua_newtable(L);  // create a new metatable `m' ...
		lua_pushvalue(L, -1);  // ... and mark `m' as a valid metatable
		lua_pushboolean(L, 1);
		lua_rawset(L, lua_upvalueindex(1));  // weaktable[m] = true
	}
	else {
		int validproxy = 0;  // to check if weaktable[metatable(u)] == true
		if (lua_getmetatable(L, 1)) {
			lua_rawget(L, lua_upvalueindex(1));
			validproxy = lua_toboolean(L, -1);
			lua_pop(L, 1);  // remove value
		}
		luaL_argcheck(L, validproxy, 1, "boolean or proxy expected");
		lua_getmetatable(L, 1);  // metatable is valid; get it
	}
	lua_setmetatable(L, 2);
	return 1;
}

// ---- require (5.0.2's, through findfullpath) --------------------------------------------------------

#define LUA_PATH "LUA_PATH"
#define LUA_PATH_SEP ';'
#define LUA_PATH_MARK '?'
#define LUA_PATH_DEFAULT "?;?.lua"

static const char* getpath(lua_State* L)
{
	const char* path;
	lua_pushliteral(L, LUA_PATH);
	lua_gettable(L, LUA_GLOBALSINDEX);  // try global variable
	path = lua_tostring(L, -1);
	lua_pop(L, 1);
	if (path)
		return path;
	return LUA_PATH_DEFAULT;  // else use default (the environment is not read)
}

static const char* pushnextpath(lua_State* L, const char* path)
{
	const char* l;
	if (*path == '\0')
		return NULL;  // no more paths
	if (*path == LUA_PATH_SEP)
		path++;  // skip separator
	l = strchr(path, LUA_PATH_SEP);  // find next separator
	if (l == NULL)
		l = path + strlen(path);
	lua_pushlstring(L, path, (size_t)(l - path));  // directory name
	return l;
}

static void pushcomposename(lua_State* L)
{
	const char* path = lua_tostring(L, -1);
	const char* wild;
	int n = 1;
	while ((wild = strchr(path, LUA_PATH_MARK)) != NULL) {
		// is there stack space for prefix, name, and eventual last sufix?
		luaL_checkstack(L, 3, "too many marks in a path component");
		lua_pushlstring(L, path, (size_t)(wild - path));  // push prefix
		lua_pushvalue(L, 1);  // push package name (in place of MARK)
		path = wild + 1;  // continue after MARK
		n += 2;
	}
	lua_pushstring(L, path);  // push last sufix (`n' already includes this)
	lua_concat(L, n);
}

// The name a package is kept under in _LOADED: PopLua upper-cases it, so that names that differ only in case
// (and the paths findfullpath makes of them) load once.
static void pushloadedkey(lua_State* L, int name)
{
	char key[256];
	const char* s = lua_tostring(L, name);
	size_t n = 0;
	for (; s[n] != '\0' && n < sizeof key - 1; n++)
		key[n] = (char)toupper((unsigned char)s[n]);
	key[n] = '\0';
	lua_pushstring(L, key);
}

// 0x700c70: 5.0.2's require, keyed on the upper-cased name; without mustexist (require_if) a package that
// is missing or fails to load is no error
int poplua_requirecore(lua_State* L, int mustexist)
{
	const char* path;
	int status = LUA_ERRFILE;  // not found (no path yet)
	luaL_checkstring(L, 1);
	lua_settop(L, 1);
	lua_pushliteral(L, "_LOADED");
	lua_gettable(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, 2))
		return luaL_error(L, "`_LOADED' is not a table");
	path = getpath(L);
	pushloadedkey(L, 1);  // check package's name in book-keeping table
	lua_rawget(L, 2);
	if (lua_toboolean(L, -1))  // is it there?
		return 1;  // package is already loaded; return its result
	while (status == LUA_ERRFILE) {
		lua_settop(L, 3);  // reset stack position
		if ((path = pushnextpath(L, path)) == NULL)
			break;
		pushcomposename(L);
		status = luaL_loadfile(L, lua_tostring(L, -1));  // try to load it
	}
	switch (status) {
	case 0: {
		lua_pushliteral(L, "_REQUIREDNAME");
		lua_gettable(L, LUA_GLOBALSINDEX);  // save previous name
		lua_insert(L, -2);  // put it below function
		lua_pushliteral(L, "_REQUIREDNAME");
		lua_pushvalue(L, 1);
		lua_settable(L, LUA_GLOBALSINDEX);  // set new name
		lua_call(L, 0, 1);  // run loaded module
		lua_insert(L, -2);  // put result below previous name
		lua_pushliteral(L, "_REQUIREDNAME");
		lua_insert(L, -2);
		lua_settable(L, LUA_GLOBALSINDEX);  // reset to previous name
		if (lua_isnil(L, -1)) {  // no/nil return?
			lua_pushboolean(L, 1);
			lua_replace(L, -2);  // replace to true
		}
		pushloadedkey(L, 1);
		lua_pushvalue(L, -2);
		lua_rawset(L, 2);  // mark it as loaded
		return 1;  // return value
	}
	case LUA_ERRFILE:  // file not found
		if (mustexist)
			return luaL_error(L, "could not load package `%s' from path `%s'", lua_tostring(L, 1), getpath(L));
		return 1;
	default:
		if (mustexist)
			return luaL_error(L, "error loading package `%s' (%s)", lua_tostring(L, 1), lua_tostring(L, -1));
		return 1;
	}
}

// 0x701210
static int luaB_require(lua_State* L)
{
	return poplua_requirecore(L, 1);
}

// 0x701220
static int luaB_require_if(lua_State* L)
{
	return poplua_requirecore(L, 0);
}

// Where findfullpath starts: pass 0 the directory of the running script, pass 1 the current directory
// (the data folder, which the port's paths are relative to: the original has getcwd here). Pass 0 fails
// when no Lua function is running.
static int searchbase(lua_State* L, int pass, char* dir, size_t size)
{
	if (pass == 1) {
		snprintf(dir, size, "%s", poplua_getcwd());
		return 1;
	}
	lua_Debug ar;
	for (int level = 0; lua_getstack(L, level, &ar); level++) {
		lua_getinfo(L, "S", &ar);
		if (*ar.what == 'C')
			continue;
		poplua_fullpath(ar.source[0] != '\0' ? ar.source + 1 : ar.source, dir, size);  // past the '@'
		char* cut = strrchr(dir, '\\');
		if (cut)
			*cut = '\0';
		return 1;
	}
	return 0;
}

// 0x701230: the file that require(name) would find, searched for along LUA_PATH from the running script's
// folder and then from the current one, as a path with backslashes; nothing when there is none. A name of a
// .lua script is also found by its .luc (and any other name by name.luc), but returned as asked for.
int poplua_findfullpath(lua_State* L)
{
	luaL_checkstring(L, 1);
	for (int pass = 0; pass < 2; pass++) {
		char dir[260];
		if (!searchbase(L, pass, dir, sizeof dir))
			return 0;
		const char* path = getpath(L);
		for (;;) {
			lua_settop(L, 3);
			if ((path = pushnextpath(L, path)) == NULL)
				break;
			pushcomposename(L);
			const char* s = lua_tostring(L, -1);
			char full[260];
			if (strncmp(s, "\\\\", 2) == 0 || (s[0] != '\0' && s[1] == ':'))
				snprintf(full, sizeof full, "%s", s);
			else {
				size_t n = strlen(dir);
				memcpy(full, dir, n + 1);
				while (strncmp(s, "../", 3) == 0 || strncmp(s, "..\\", 3) == 0) {
					s += 3;
					while (n > 0 && full[n - 1] != '\\' && full[n - 1] != '/')
						n--;
					if (n == 0)
						break;
					n--;
				}
				if (n > 0 && full[n - 1] != '\\')
					full[n++] = '\\';
				snprintf(full + n, sizeof full - n, "%s", s);
			}
			for (char* p = full; *p; p++)
				if (*p == '/')
					*p = '\\';
			if (poplua_fileexists(full)) {
				lua_pushstring(L, full);
				return 1;
			}
			char luc[264];
			memcpy(luc, full, strlen(full) + 1);
			char* ext = strrchr(luc, '.');
			if (ext && tolower((unsigned char)ext[1]) == 'l' && tolower((unsigned char)ext[2]) == 'u' &&
				tolower((unsigned char)ext[3]) == 'a' && ext[4] == '\0')
				memcpy(ext, ".luc", 5);
			else
				memcpy(luc + strlen(luc), ".luc", 5);
			if (poplua_fileexists(luc)) {
				lua_pushstring(L, full);
				return 1;
			}
		}
	}
	return 0;
}

// 0x7009b0: what require(name) returned, if it ran (the name as findfullpath completes it)
static int luaB_scriptloaded(lua_State* L)
{
	luaL_checkstring(L, 1);
	lua_settop(L, 1);
	lua_pushliteral(L, "findfullpath");
	lua_gettable(L, LUA_GLOBALSINDEX);
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pushvalue(L, 1);
	}
	lua_replace(L, 1);
	lua_pushliteral(L, "_LOADED");
	lua_gettable(L, LUA_GLOBALSINDEX);
	if (!lua_istable(L, 2))
		return luaL_error(L, "`_LOADED' is not a table");
	pushloadedkey(L, 1);
	lua_rawget(L, 2);
	return 1;
}

// ---- coroutines --------------------------------------------------------------------------------------

static int auxresume(lua_State* L, lua_State* co, int narg)
{
	int status;
	if (!lua_checkstack(co, narg))
		luaL_error(L, "too many arguments to resume");
	lua_xmove(L, co, narg);
	status = lua_resume(co, narg);
	if (status == 0) {
		int nres = lua_gettop(co);
		if (!lua_checkstack(L, nres))
			luaL_error(L, "too many results to resume");
		lua_xmove(co, L, nres);  // move yielded values
		return nres;
	}
	else {
		lua_xmove(co, L, 1);  // move error message
		return -1;  // error flag
	}
}

static int luaB_coresume(lua_State* L)
{
	lua_State* co = lua_tothread(L, 1);
	int r;
	luaL_argcheck(L, co, 1, "coroutine expected");
	r = auxresume(L, co, lua_gettop(L) - 1);
	if (r < 0) {
		lua_pushboolean(L, 0);
		lua_insert(L, -2);
		return 2;  // return false + error message
	}
	else {
		lua_pushboolean(L, 1);
		lua_insert(L, -(r + 1));
		return r + 1;  // return true + `resume' returns
	}
}

static int luaB_auxwrap(lua_State* L)
{
	lua_State* co = lua_tothread(L, lua_upvalueindex(1));
	int r = auxresume(L, co, lua_gettop(L));
	if (r < 0) {
		if (lua_isstring(L, -1)) {  // error object is a string?
			luaL_where(L, 1);  // would be at level 1?
			lua_insert(L, -2);
			lua_concat(L, 2);
		}
		lua_error(L);  // propagate error
	}
	return r;
}

static int luaB_cocreate(lua_State* L)
{
	lua_State* NL = lua_newthread(L);
	luaL_argcheck(L, lua_isfunction(L, 1) && !lua_iscfunction(L, 1), 1, "Lua function expected");
	lua_pushvalue(L, 1);  // move function to top
	lua_xmove(L, NL, 1);  // move function from L to NL
	return 1;
}

static int luaB_cowrap(lua_State* L)
{
	luaB_cocreate(L);
	lua_pushcclosure(L, luaB_auxwrap, 1);
	return 1;
}

static int luaB_yield(lua_State* L)
{
	return lua_yield(L, lua_gettop(L));
}

static int luaB_costatus(lua_State* L)
{
	lua_State* co = lua_tothread(L, 1);
	luaL_argcheck(L, co, 1, "coroutine expected");
	if (L == co)
		lua_pushliteral(L, "running");
	else {
		lua_Debug ar;
		if (lua_getstack(co, 0, &ar) == 0 && lua_gettop(co) == 0)
			lua_pushliteral(L, "dead");
		else
			lua_pushliteral(L, "suspended");  // initial state
	}
	return 1;
}

static const luaL_reg co_funcs[] = {
	{"create", luaB_cocreate},
	{"wrap", luaB_cowrap},
	{"resume", luaB_coresume},
	{"yield", luaB_yield},
	{"status", luaB_costatus},
	{NULL, NULL}
};

// ---- bit (0x701e30-0x7023a0): ints ------------------------------------------------------------------

static int bit_bnot(lua_State* L)
{
	lua_pushint(L, ~luaL_checkint(L, 1));
	return 1;
}

static int bit_band(lua_State* L)
{
	int n = lua_gettop(L);
	uint32_t r = (uint32_t)luaL_checkint(L, 1);
	for (int i = 2; i <= n; i++)
		r &= (uint32_t)luaL_checkint(L, i);
	lua_pushint(L, (lua_Int)r);
	return 1;
}

static int bit_bor(lua_State* L)
{
	int n = lua_gettop(L);
	uint32_t r = (uint32_t)luaL_checkint(L, 1);
	for (int i = 2; i <= n; i++)
		r |= (uint32_t)luaL_checkint(L, i);
	lua_pushint(L, (lua_Int)r);
	return 1;
}

static int bit_bxor(lua_State* L)
{
	int n = lua_gettop(L);
	uint32_t r = (uint32_t)luaL_checkint(L, 1);
	for (int i = 2; i <= n; i++)
		r ^= (uint32_t)luaL_checkint(L, i);
	lua_pushint(L, (lua_Int)r);
	return 1;
}

static int bit_lshift(lua_State* L)
{
	uint32_t a = (uint32_t)luaL_checkint(L, 1);
	lua_pushint(L, (lua_Int)(a << (luaL_checkint(L, 2) & 31)));
	return 1;
}

static int bit_rshift(lua_State* L)
{
	uint32_t a = (uint32_t)luaL_checkint(L, 1);
	lua_pushint(L, (lua_Int)(a >> (luaL_checkint(L, 2) & 31)));
	return 1;
}

static int bit_arshift(lua_State* L)
{
	lua_Int a = luaL_checkint(L, 1);
	lua_pushint(L, a >> (luaL_checkint(L, 2) & 31));
	return 1;
}

static int bit_mod(lua_State* L)
{
	lua_Int a = luaL_checkint(L, 1);
	lua_Int b = luaL_checkint(L, 2);
	if (b == 0)
		luaL_error(L, "integer modulo by zero");
	lua_pushint(L, b == -1 ? 0 : a % b);
	return 1;
}

static const luaL_reg bit_funcs[] = {
	{"bnot", bit_bnot},
	{"band", bit_band},
	{"bor", bit_bor},
	{"bxor", bit_bxor},
	{"lshift", bit_lshift},
	{"rshift", bit_rshift},
	{"arshift", bit_arshift},
	{"mod", bit_mod},
	{NULL, NULL}
};

static const luaL_reg base_funcs[] = {
	{"error", luaB_error},
	{"getmetatable", luaB_getmetatable},
	{"setmetatable", luaB_setmetatable},
	{"getfenv", luaB_getfenv},
	{"setfenv", luaB_setfenv},
	{"next", luaB_next},
	{"ipairs", luaB_ipairs},
	{"pairs", luaB_pairs},
	{"print", luaB_print},
	{"tonumber", luaB_tonumber},
	{"tostring", luaB_tostring},
	{"type", luaB_type},
	{"assert", luaB_assert},
	{"unpack", luaB_unpack},
	{"rawequal", luaB_rawequal},
	{"rawget", luaB_rawget},
	{"rawset", luaB_rawset},
	{"pcall", luaB_pcall},
	{"xpcall", luaB_xpcall},
	{"collectgarbage", luaB_collectgarbage},
	{"gcinfo", luaB_gcinfo},
	{"loadfile", luaB_loadfile},
	{"dofile", luaB_dofile},
	{"loadstring", luaB_loadstring},
	{"require", luaB_require},
	{"scriptloaded", luaB_scriptloaded},
	{"findfullpath", poplua_findfullpath},
	{"require_if", luaB_require_if},
	{NULL, NULL}
};

static void base_open(lua_State* L)
{
	lua_pushliteral(L, "_G");
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_openlib(L, NULL, base_funcs, 0);  // open lib into global table
	lua_pushliteral(L, "_VERSION");
	lua_pushliteral(L, LUA_VERSION);
	lua_rawset(L, -3);  // set global _VERSION
	// `newproxy' needs a weaktable as upvalue
	lua_pushliteral(L, "newproxy");
	lua_newtable(L);  // new table `w'
	lua_pushvalue(L, -1);  // `w' will be its own metatable
	lua_setmetatable(L, -2);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "k");
	lua_rawset(L, -3);  // metatable(w).__mode = "k"
	lua_pushcclosure(L, luaB_newproxy, 1);
	lua_rawset(L, -3);  // set global `newproxy'
	lua_rawset(L, -1);  // set global _G
}

// 0x702490
int luaopen_base(lua_State* L)
{
	base_open(L);
	luaL_openlib(L, LUA_BITLIBNAME, bit_funcs, 0);
	luaL_openlib(L, LUA_COLIBNAME, co_funcs, 0);
	lua_newtable(L);
	lua_pushliteral(L, "_LOADED");
	lua_pushvalue(L, -2);
	lua_settable(L, LUA_GLOBALSINDEX);
	return 0;
}
