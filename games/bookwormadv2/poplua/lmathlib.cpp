// The math library (0x703aa0-0x705120): Lua 5.0.2's lmathlib.c with PopLua's ints. abs and mod keep ints,
// ceil, floor, round and random(m [, n]) return ints, min and max return the chosen argument itself, and
// random() draws from the C library's rand(), here Visual C++'s generator (the original's CRT), so that a
// seed gives the original's sequence. math.dist is not ported.
#include <cmath>

#include "lauxlib.h"

#undef PI
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI / 180.0)

// Visual C++'s rand() and srand() (per thread in the original; the game draws from it on its main thread)
static unsigned int crt_seed = 1;

void poplua_srand(unsigned int seed)
{
	crt_seed = seed;
}

int poplua_rand(void)
{
	crt_seed = crt_seed * 214013u + 2531011u;
	return (int)((crt_seed >> 16) & 0x7fff);
}

// 0x703aa0
static int math_abs(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TINT) {
		lua_Int i = luaL_checkint(L, 1);
		lua_pushint(L, (lua_Int)((uint32_t)(i ^ (i >> 31)) - (uint32_t)(i >> 31)));
	}
	else
		lua_pushnumber(L, fabs(luaL_checknumber(L, 1)));
	return 1;
}

static int math_sin(lua_State* L)
{
	lua_pushnumber(L, sin(luaL_checknumber(L, 1)));
	return 1;
}

static int math_cos(lua_State* L)
{
	lua_pushnumber(L, cos(luaL_checknumber(L, 1)));
	return 1;
}

static int math_tan(lua_State* L)
{
	lua_pushnumber(L, tan(luaL_checknumber(L, 1)));
	return 1;
}

static int math_asin(lua_State* L)
{
	lua_pushnumber(L, asin(luaL_checknumber(L, 1)));
	return 1;
}

static int math_acos(lua_State* L)
{
	lua_pushnumber(L, acos(luaL_checknumber(L, 1)));
	return 1;
}

static int math_atan(lua_State* L)
{
	lua_pushnumber(L, atan(luaL_checknumber(L, 1)));
	return 1;
}

static int math_atan2(lua_State* L)
{
	lua_pushnumber(L, atan2(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
	return 1;
}

// 0x704030
static int math_ceil(lua_State* L)
{
	lua_pushint(L, lua_ftol(ceil(luaL_checknumber(L, 1))));
	return 1;
}

// 0x7041c0
static int math_floor(lua_State* L)
{
	lua_pushint(L, lua_ftol(floor(luaL_checknumber(L, 1))));
	return 1;
}

// 0x7040e0: halves round up
static int math_round(lua_State* L)
{
	lua_Number x = luaL_checknumber(L, 1);
	lua_Number f = floor(x);
	lua_pushint(L, lua_ftol(f + 0.5 <= x ? ceil(x) : f));
	return 1;
}

// 0x704270: two ints give the C remainder; anything else fmod
static int math_mod(lua_State* L)
{
	if (lua_type(L, 1) == LUA_TINT && lua_type(L, 2) == LUA_TINT) {
		lua_Int b = luaL_checkint(L, 2);
		lua_Int a = luaL_checkint(L, 1);
		if (b == 0)
			luaL_error(L, "integer modulo by zero");  // (the original crashes)
		lua_pushint(L, (b == -1) ? 0 : a % b);
		return 1;
	}
	lua_pushnumber(L, fmod(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
	return 1;
}

static int math_sqrt(lua_State* L)
{
	lua_pushnumber(L, sqrt(luaL_checknumber(L, 1)));
	return 1;
}

static int math_pow(lua_State* L)
{
	lua_pushnumber(L, pow(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
	return 1;
}

static int math_log(lua_State* L)
{
	lua_pushnumber(L, log(luaL_checknumber(L, 1)));
	return 1;
}

static int math_log10(lua_State* L)
{
	lua_pushnumber(L, log10(luaL_checknumber(L, 1)));
	return 1;
}

static int math_exp(lua_State* L)
{
	lua_pushnumber(L, exp(luaL_checknumber(L, 1)));
	return 1;
}

static int math_deg(lua_State* L)
{
	lua_pushnumber(L, luaL_checknumber(L, 1) / RADIANS_PER_DEGREE);
	return 1;
}

static int math_rad(lua_State* L)
{
	lua_pushnumber(L, luaL_checknumber(L, 1) * RADIANS_PER_DEGREE);
	return 1;
}

static int math_frexp(lua_State* L)
{
	int e;
	lua_pushnumber(L, frexp(luaL_checknumber(L, 1), &e));
	lua_pushnumber(L, e);
	return 2;
}

static int math_ldexp(lua_State* L)
{
	lua_pushnumber(L, ldexp(luaL_checknumber(L, 1), luaL_checkint(L, 2)));
	return 1;
}

// 0x704ba0: the first smallest argument, as it was given
static int math_min(lua_State* L)
{
	int n = lua_gettop(L);  // number of arguments
	lua_Number dmin = luaL_checknumber(L, 1);
	int imin = 1;
	int i;
	for (i = 2; i <= n; i++) {
		lua_Number d = luaL_checknumber(L, i);
		if (d < dmin) {
			dmin = d;
			imin = i;
		}
	}
	lua_pushvalue(L, imin);
	return 1;
}

// 0x704d90
static int math_max(lua_State* L)
{
	int n = lua_gettop(L);  // number of arguments
	lua_Number dmax = luaL_checknumber(L, 1);
	int imax = 1;
	int i;
	for (i = 2; i <= n; i++) {
		lua_Number d = luaL_checknumber(L, i);
		if (d > dmax) {
			dmax = d;
			imax = i;
		}
	}
	lua_pushvalue(L, imax);
	return 1;
}

// 0x704f80: random() is a number in [0, 1); random(m) an int in [0, m-1]; random(m, n) an int in [m, n]
static int math_random(lua_State* L)
{
	lua_Number r = (lua_Number)(poplua_rand() % 0x7fff) / (lua_Number)0x7fff;
	switch (lua_gettop(L)) {  // check number of arguments
	case 0: {  // no arguments
		lua_pushnumber(L, r);  // Number between 0 and 1
		break;
	}
	case 1: {  // only upper limit
		lua_Int u = luaL_checkint(L, 1);
		lua_pushint(L, lua_ftol(floor((lua_Number)u * r)));
		break;
	}
	case 2: {  // lower and upper limits
		lua_Int l = luaL_checkint(L, 1);
		lua_Int u = luaL_checkint(L, 2);
		lua_pushint(L, lua_ftol(floor((lua_Number)(u + (1 - l)) * r)) + l);
		break;
	}
	default:
		return luaL_error(L, "wrong number of arguments");
	}
	return 1;
}

// 0x7050a0
static int math_randomseed(lua_State* L)
{
	poplua_srand((unsigned int)luaL_checkint(L, 1));
	return 0;
}

static const luaL_reg mathlib[] = {
	{"abs", math_abs},
	{"sin", math_sin},
	{"cos", math_cos},
	{"tan", math_tan},
	{"asin", math_asin},
	{"acos", math_acos},
	{"atan", math_atan},
	{"atan2", math_atan2},
	{"ceil", math_ceil},
	{"floor", math_floor},
	{"mod", math_mod},
	{"frexp", math_frexp},
	{"ldexp", math_ldexp},
	{"sqrt", math_sqrt},
	{"min", math_min},
	{"max", math_max},
	{"log", math_log},
	{"log10", math_log10},
	{"exp", math_exp},
	{"deg", math_deg},
	{"pow", math_pow},
	{"rad", math_rad},
	{"random", math_random},
	{"randomseed", math_randomseed},
	{"round", math_round},
	{NULL, NULL}
};

// 0x705120
int luaopen_math(lua_State* L)
{
	luaL_openlib(L, LUA_MATHLIBNAME, mathlib, 0);
	lua_pushliteral(L, "pi");
	lua_pushnumber(L, PI);
	lua_settable(L, -3);
	lua_pushliteral(L, "__pow");
	lua_pushcfunction(L, math_pow);
	lua_settable(L, LUA_GLOBALSINDEX);
	return 1;
}
