// The auxiliary library (Lua 5.0.2's lauxlib.h). PopLua's argument checks take ints (luaL_checkint reads an
// int, a number or a decimal int string), and luaL_getn/luaL_setn keep the sizes of 0-based tables.
#ifndef POPLUA_LAUXLIB_H
#define POPLUA_LAUXLIB_H

#include <cstdio>

#include "lua.h"

struct luaL_reg {
	const char* name;
	lua_CFunction func;
};

void luaL_openlib(lua_State* L, const char* libname, const luaL_reg* l, int nup);
int luaL_getmetafield(lua_State* L, int obj, const char* e);
int luaL_callmeta(lua_State* L, int obj, const char* e);
int luaL_typerror(lua_State* L, int narg, const char* tname);
int luaL_argerror(lua_State* L, int numarg, const char* extramsg);
const char* luaL_checklstring(lua_State* L, int numArg, size_t* l);
const char* luaL_optlstring(lua_State* L, int numArg, const char* def, size_t* l);
lua_Number luaL_checknumber(lua_State* L, int numArg);
lua_Number luaL_optnumber(lua_State* L, int nArg, lua_Number def);
lua_Int luaL_checkint(lua_State* L, int numArg);
lua_Int luaL_optint(lua_State* L, int nArg, lua_Int def);

void luaL_checkstack(lua_State* L, int sz, const char* msg);
void luaL_checktype(lua_State* L, int narg, int t);
void luaL_checkany(lua_State* L, int narg);

int luaL_newmetatable(lua_State* L, const char* tname);
void luaL_getmetatable(lua_State* L, const char* tname);
void* luaL_checkudata(lua_State* L, int ud, const char* tname);

void luaL_where(lua_State* L, int lvl);
int luaL_error(lua_State* L, const char* fmt, ...);

int luaL_findstring(const char* st, const char* const lst[]);

int luaL_ref(lua_State* L, int t);
void luaL_unref(lua_State* L, int t, int ref);

int luaL_getn(lua_State* L, int t);
void luaL_setn(lua_State* L, int t, int n);

int luaL_loadfile(lua_State* L, const char* filename);
int luaL_loadbuffer(lua_State* L, const char* buff, size_t sz, const char* name);

// PopLua: t[k] as the VM reads it (the class parents and __index)
void luaL_getfield(lua_State* L, int idx, const char* k);
void luaL_setfield(lua_State* L, int idx, const char* k);

#define luaL_argcheck(L, cond, numarg, extramsg) if (!(cond)) luaL_argerror(L, numarg, extramsg)
#define luaL_checkstring(L, n) (luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L, n, d) (luaL_optlstring(L, (n), (d), NULL))
#define luaL_checklong(L, n) ((long)luaL_checkint(L, n))
#define luaL_optlong(L, n, d) ((long)luaL_optint(L, n, d))

// generic buffer manipulation
#define LUAL_BUFFERSIZE BUFSIZ

struct luaL_Buffer {
	char* p;  // current position in buffer
	int lvl;  // number of strings in the stack (level)
	lua_State* L;
	char buffer[LUAL_BUFFERSIZE];
};

#define luaL_putchar(B, c) ((void)((B)->p < ((B)->buffer + LUAL_BUFFERSIZE) || luaL_prepbuffer(B)), (*(B)->p++ = (char)(c)))
#define luaL_addsize(B, n) ((B)->p += (n))

void luaL_buffinit(lua_State* L, luaL_Buffer* B);
char* luaL_prepbuffer(luaL_Buffer* B);
void luaL_addlstring(luaL_Buffer* B, const char* s, size_t l);
void luaL_addstring(luaL_Buffer* B, const char* s);
void luaL_addvalue(luaL_Buffer* B);
void luaL_pushresult(luaL_Buffer* B);

// compatibility
int lua_dofile(lua_State* L, const char* filename);
int lua_dostring(lua_State* L, const char* str);
int lua_dobuffer(lua_State* L, const char* buff, size_t sz, const char* n);

// the standard libraries (lualib.h)
#define LUA_COLIBNAME "coroutine"
int luaopen_base(lua_State* L);
#define LUA_TABLIBNAME "table"
int luaopen_table(lua_State* L);
#define LUA_IOLIBNAME "io"
#define LUA_OSLIBNAME "os"
int luaopen_io(lua_State* L);
#define LUA_STRLIBNAME "string"
int luaopen_string(lua_State* L);
#define LUA_MATHLIBNAME "math"
int luaopen_math(lua_State* L);
#define LUA_BITLIBNAME "bit"
int luaopen_bit(lua_State* L);
#define LUA_DBLIBNAME "debug"
int luaopen_debug(lua_State* L);

// PopLua's hooks for the host: where print and dprint write, and how scripts are found (dofile, loadfile,
// require and io.open read files through it)
typedef void (*poplua_PrintFunc)(const char* s, size_t len);
typedef int (*poplua_ReadFunc)(const char* path, char** data, size_t* size);  // malloc'd; 1 if found
typedef int (*poplua_WriteFunc)(const char* path, const char* data, size_t size);  // io's files; 1 if saved
typedef int (*poplua_ExistsFunc)(const char* path);  // findfullpath's test; 1 if the file can be read
void poplua_setprint(poplua_PrintFunc f);
void poplua_setreader(poplua_ReadFunc f);
void poplua_setwriter(poplua_WriteFunc f);
void poplua_setexists(poplua_ExistsFunc f);
void poplua_print(const char* s, size_t len);
int poplua_readfile(const char* path, char** data, size_t* size);
int poplua_fileexists(const char* path);
// The current directory the scripts' paths are made full against ("" by default: the paths stay relative)
void poplua_setcwd(const char* dir);
const char* poplua_getcwd(void);
void poplua_fullpath(const char* name, char* out, size_t size);

// the base library's require (0x700c70; mustexist 0 is require_if) and findfullpath (0x701230), which
// LuaSupport's require calls
int poplua_requirecore(lua_State* L, int mustexist);
int poplua_findfullpath(lua_State* L);

// LuaSupport (0x6cd380-0x6cec80): setprot, getprot, getclass, and require and print as the game has them
int luaopen_support(lua_State* L);

// Visual C++'s rand() and srand(): math.random's source, shared with the game code that called rand()
int poplua_rand(void);
void poplua_srand(unsigned int seed);

#endif
