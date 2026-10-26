// PopLua: a reimplementation of the Lua 5.0.2 fork inside Bookworm Adventures (docs/POPLUA.md).
// The C API is Lua 5.0's, so the game's bindings translate call for call from the original's; PopLua's
// additions are marked. Everything is C++ (errors unwind with a C++ exception, see ldo.cpp).
#ifndef POPLUA_LUA_H
#define POPLUA_LUA_H

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#define LUA_VERSION "Lua 5.0.2"

// option for multiple returns in lua_pcall and lua_call
#define LUA_MULTRET (-1)

// pseudo-indices
#define LUA_REGISTRYINDEX (-10000)
#define LUA_GLOBALSINDEX (-10001)
#define lua_upvalueindex(i) (LUA_GLOBALSINDEX - (i))

// error codes for lua_load and lua_pcall
#define LUA_ERRRUN 1
#define LUA_ERRFILE 2
#define LUA_ERRSYNTAX 3
#define LUA_ERRMEM 4
#define LUA_ERRERR 5

struct lua_State;
typedef int (*lua_CFunction)(lua_State* L);

// functions that read/write blocks when loading/dumping Lua chunks
typedef const char* (*lua_Chunkreader)(lua_State* L, void* ud, size_t* sz);
typedef int (*lua_Chunkwriter)(lua_State* L, const void* p, size_t sz, void* ud);

// basic types; PopLua inserts LUA_TINT (an int32 value) after LUA_TNUMBER
#define LUA_TNONE (-1)
#define LUA_TNIL 0
#define LUA_TBOOLEAN 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER 3
#define LUA_TINT 4
#define LUA_TSTRING 5
#define LUA_TTABLE 6
#define LUA_TFUNCTION 7
#define LUA_TUSERDATA 8
#define LUA_TTHREAD 9
#define LUA_TMETHOD 10  // only in VM registers: a function fetched as a method

#define LUA_MINSTACK 20

typedef double lua_Number;
typedef int32_t lua_Int;

// state manipulation
lua_State* lua_open(void);
void lua_close(lua_State* L);
lua_CFunction lua_atpanic(lua_State* L, lua_CFunction panicf);
lua_State* lua_newthread(lua_State* L);

// basic stack manipulation
int lua_gettop(lua_State* L);
void lua_settop(lua_State* L, int idx);
void lua_pushvalue(lua_State* L, int idx);
void lua_remove(lua_State* L, int idx);
void lua_insert(lua_State* L, int idx);
void lua_replace(lua_State* L, int idx);
int lua_checkstack(lua_State* L, int sz);
void lua_xmove(lua_State* from, lua_State* to, int n);

// access functions (stack -> C)
int lua_isnumber(lua_State* L, int idx);
int lua_isint(lua_State* L, int idx);  // PopLua: an int, a number or a decimal int string
int lua_isstring(lua_State* L, int idx);
int lua_iscfunction(lua_State* L, int idx);
int lua_isuserdata(lua_State* L, int idx);
int lua_type(lua_State* L, int idx);
const char* lua_typename(lua_State* L, int tp);

int lua_equal(lua_State* L, int idx1, int idx2);
int lua_rawequal(lua_State* L, int idx1, int idx2);
int lua_lessthan(lua_State* L, int idx1, int idx2);

lua_Number lua_tonumber(lua_State* L, int idx);
lua_Int lua_toint(lua_State* L, int idx);  // PopLua: numbers are truncated, strings read with strtol
int lua_toboolean(lua_State* L, int idx);
const char* lua_tostring(lua_State* L, int idx);
size_t lua_strlen(lua_State* L, int idx);
size_t lua_objlen(lua_State* L, int idx);  // PopLua (5.1's): a table's length is its 0-based border
lua_CFunction lua_tocfunction(lua_State* L, int idx);
void* lua_touserdata(lua_State* L, int idx);
lua_State* lua_tothread(lua_State* L, int idx);
const void* lua_topointer(lua_State* L, int idx);

// push functions (C -> stack)
void lua_pushnil(lua_State* L);
void lua_pushnumber(lua_State* L, lua_Number n);
void lua_pushint(lua_State* L, lua_Int n);  // PopLua
void lua_pushlstring(lua_State* L, const char* s, size_t l);
void lua_pushstring(lua_State* L, const char* s);
const char* lua_pushvfstring(lua_State* L, const char* fmt, va_list argp);
const char* lua_pushfstring(lua_State* L, const char* fmt, ...);
void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n);
void lua_pushboolean(lua_State* L, int b);
void lua_pushlightuserdata(lua_State* L, void* p);

// get functions (Lua -> stack)
void lua_gettable(lua_State* L, int idx);
void lua_rawget(lua_State* L, int idx);
void lua_rawgeti(lua_State* L, int idx, int n);
void lua_newtable(lua_State* L);
void* lua_newuserdata(lua_State* L, size_t sz);
int lua_getmetatable(lua_State* L, int objindex);
void lua_getfenv(lua_State* L, int idx);

// set functions (stack -> Lua)
void lua_settable(lua_State* L, int idx);
void lua_rawset(lua_State* L, int idx);
void lua_rawseti(lua_State* L, int idx, int n);
int lua_setmetatable(lua_State* L, int objindex);
int lua_setfenv(lua_State* L, int idx);

// PopLua: the class parent link (searched for string keys before __index) and the protection flags
void lua_setparent(lua_State* L, int idx);  // pops a table (or nil) and makes it t's parent
int lua_getparent(lua_State* L, int idx);   // pushes t's parent and returns 1, or returns 0
int lua_getprot(lua_State* L, int idx);
void lua_setprot(lua_State* L, int idx, int prot);
void lua_pushudata(lua_State* L, void* block);  // pushes the live full userdata whose block this is

// `load' and `call' functions (load and run Lua code)
void lua_call(lua_State* L, int nargs, int nresults);
int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
int lua_cpcall(lua_State* L, lua_CFunction func, void* ud);
int lua_load(lua_State* L, lua_Chunkreader reader, void* dt, const char* chunkname);

// coroutine functions
int lua_yield(lua_State* L, int nresults);
int lua_resume(lua_State* L, int narg);

// garbage-collection functions
int lua_getgcthreshold(lua_State* L);
int lua_getgccount(lua_State* L);
void lua_setgcthreshold(lua_State* L, int newthreshold);

// PopLua: the conversions the VM uses (the original CRT's), for the libraries
lua_Int lua_ftol(lua_Number d);                            // truncation; INT_MIN when out of range
int lua_str2int(const char* s, lua_Int* result);           // strtol, base 10, trailing spaces allowed
int lua_str2number(const char* s, lua_Number* result);     // strtod, decimal only

// miscellaneous functions
const char* lua_version(void);
int lua_error(lua_State* L);
int lua_next(lua_State* L, int idx);
void lua_concat(lua_State* L, int n);

// some useful macros
#define lua_boxpointer(L, u) (*(void**)(lua_newuserdata(L, sizeof(void*))) = (u))
#define lua_unboxpointer(L, i) (*(void**)(lua_touserdata(L, i)))
#define lua_pop(L, n) lua_settop(L, -(n)-1)
#define lua_register(L, n, f) (lua_pushstring(L, n), lua_pushcfunction(L, f), lua_settable(L, LUA_GLOBALSINDEX))
#define lua_pushcfunction(L, f) lua_pushcclosure(L, f, 0)
#define lua_isfunction(L, n) (lua_type(L, n) == LUA_TFUNCTION)
#define lua_istable(L, n) (lua_type(L, n) == LUA_TTABLE)
#define lua_islightuserdata(L, n) (lua_type(L, n) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L, n) (lua_type(L, n) == LUA_TNIL)
#define lua_isboolean(L, n) (lua_type(L, n) == LUA_TBOOLEAN)
#define lua_isnone(L, n) (lua_type(L, n) == LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type(L, n) <= 0)
#define lua_pushliteral(L, s) lua_pushlstring(L, "" s, (sizeof(s) / sizeof(char)) - 1)

// compatibility with ref system
#define LUA_NOREF (-2)
#define LUA_REFNIL (-1)
#define lua_ref(L, lock) ((lock) ? luaL_ref(L, LUA_REGISTRYINDEX) : (lua_pushstring(L, "unlocked references are obsolete"), lua_error(L), 0))
#define lua_unref(L, ref) luaL_unref(L, LUA_REGISTRYINDEX, (ref))
#define lua_getref(L, ref) lua_rawgeti(L, LUA_REGISTRYINDEX, ref)

#define lua_getregistry(L) lua_pushvalue(L, LUA_REGISTRYINDEX)

// debug API
struct lua_Debug {
	int event;
	const char* name;      // (n)
	const char* namewhat;  // (n) `global', `local', `field', `method'
	const char* what;      // (S) `Lua', `C', `main', `tail'
	const char* source;    // (S)
	int currentline;       // (l)
	int nups;              // (u) number of upvalues
	int linedefined;       // (S)
	char short_src[60];    // (S)
	int i_ci;              // private: active function
};
int lua_getstack(lua_State* L, int level, lua_Debug* ar);
int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar);
const char* lua_getlocal(lua_State* L, const lua_Debug* ar, int n);
const char* lua_getupvalue(lua_State* L, int funcindex, int n);
const char* lua_setlocal(lua_State* L, const lua_Debug* ar, int n);
const char* lua_setupvalue(lua_State* L, int funcindex, int n);

#endif
