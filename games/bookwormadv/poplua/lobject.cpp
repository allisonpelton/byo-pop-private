// Memory, generic object functions, the chunk reader and tag methods (Lua 5.0.2's lmem.c, lobject.c, lzio.c
// and ltm.c, with PopLua's type and event names).
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "lcore.h"

// ---- lmem -------------------------------------------------------------------------------------------

#define MINSIZEARRAY 4

void* luaM_growaux(lua_State* L, void* block, int* size, int size_elems, int limit, const char* errormsg)
{
	void* newblock;
	int newsize = (*size) * 2;
	if (newsize < MINSIZEARRAY)
		newsize = MINSIZEARRAY;  // minimum size
	else if (*size >= limit / 2) {  // cannot double it?
		if (*size < limit - MINSIZEARRAY)  // try something smaller...
			newsize = limit;     // still have at least MINSIZEARRAY free places
		else
			luaG_runerror(L, errormsg);
	}
	newblock = luaM_realloc(L, block, (lu_mem)(*size) * (lu_mem)size_elems, (lu_mem)newsize * (lu_mem)size_elems);
	*size = newsize;  // update only when everything else is OK
	return newblock;
}

void* luaM_realloc(lua_State* L, void* block, lu_mem oldsize, lu_mem size)
{
	if (size == 0) {
		if (block != NULL) {
			free(block);
			block = NULL;
		}
		else
			return NULL;  // avoid `nblocks' computations when oldsize==size==0
	}
	else {
		block = realloc(block, size);
		if (block == NULL) {
			if (L)
				luaD_throw(L, LUA_ERRMEM);
			else
				return NULL;  // error before creating state!
		}
	}
	if (L) {
		G(L)->nblocks -= oldsize;
		G(L)->nblocks += size;
	}
	return block;
}

// ---- lobject ----------------------------------------------------------------------------------------

const TObject luaO_nilobject = {LUA_TNIL, {NULL}};

// converts an integer to a "floating point byte", represented as (mmmmmxxx), where the real value is
// (xxx) * 2^(mmmmm)
int luaO_int2fb(unsigned int x)
{
	int m = 0;  // mantissa
	while (x >= (1 << 3)) {
		x = (x + 1) >> 1;
		m++;
	}
	return (m << 3) | (int)x;
}

int luaO_log2(unsigned int x)
{
	static const lu_byte log_8[255] = {
		0,
		1, 1,
		2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
	};
	if (x >= 0x00010000) {
		if (x >= 0x01000000)
			return log_8[((x >> 24) & 0xff) - 1] + 24;
		else
			return log_8[((x >> 16) & 0xff) - 1] + 16;
	}
	else {
		if (x >= 0x00000100)
			return log_8[((x >> 8) & 0xff) - 1] + 8;
		else if (x)
			return log_8[(x & 0xff) - 1];
		return -1;  // special `log' for 0
	}
}

// the original (0x6f7060): equal tags only (an int never raw-equals a number)
int luaO_rawequalObj(const TObject* t1, const TObject* t2)
{
	if (ttype(t1) != ttype(t2))
		return 0;
	switch (ttype(t1)) {
	case LUA_TNIL:
		return 1;
	case LUA_TNUMBER:
		return nvalue(t1) == nvalue(t2);
	case LUA_TBOOLEAN:
		return bvalue(t1) == bvalue(t2);
	case LUA_TINT:
		return ivalue(t1) == ivalue(t2);
	case LUA_TLIGHTUSERDATA:
		return pvalue(t1) == pvalue(t2);
	default:
		return gcvalue(t1) == gcvalue(t2);
	}
}

// 0x6f70b0: strtod, then only white space may follow
// strtod as the original's CRT reads numbers: decimal only (no hexadecimal, "inf" or "nan")
static lua_Number crt_strtod(const char* s, char** endptr)
{
	const char* p = s;
	while (isspace((unsigned char)*p))
		p++;
	if (*p == '+' || *p == '-')
		p++;
	if (!isdigit((unsigned char)*p) && !(*p == '.' && isdigit((unsigned char)p[1]))) {
		*endptr = (char*)s;
		return 0;
	}
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {  // reads the "0" only
		*endptr = (char*)p + 1;
		return (s[0] == '-' || (p > s && p[-1] == '-')) ? -0.0 : 0.0;
	}
	return strtod(s, endptr);
}

// 0x6f70b0
int luaO_str2d(const char* s, lua_Number* result)
{
	char* endptr;
	lua_Number res = crt_strtod(s, &endptr);
	if (endptr == s)
		return 0;  // no conversion
	while (isspace((unsigned char)(*endptr)))
		endptr++;
	if (*endptr != '\0')
		return 0;  // invalid trailing characters?
	*result = res;
	return 1;
}

// 0x6f7120: a decimal int (strtol, so out-of-range values are clamped)
int luaO_str2int(const char* s, lua_Int* result)
{
	char* endptr;
	long res = strtol(s, &endptr, 10);
	if (endptr == s)
		return 0;
	while (isspace((unsigned char)(*endptr)))
		endptr++;
	if (*endptr != '\0')
		return 0;
	if (res > INT32_MAX)
		res = INT32_MAX;  // (a 64-bit long)
	else if (res < INT32_MIN)
		res = INT32_MIN;
	*result = (lua_Int)res;
	return 1;
}

static void pushstr(lua_State* L, const char* str)
{
	setsvalue2s(L->top, luaS_new(L, str));
	incr_top(L);
}

// this function handles only `%d', `%c', %f, `%s', `%p' and `%%' formats
const char* luaO_pushvfstring(lua_State* L, const char* fmt, va_list argp)
{
	int n = 1;
	pushstr(L, "");
	for (;;) {
		const char* e = strchr(fmt, '%');
		if (e == NULL)
			break;
		setsvalue2s(L->top, luaS_newlstr(L, fmt, e - fmt));
		incr_top(L);
		switch (*(e + 1)) {
		case 's': {
			const char* s = va_arg(argp, char*);
			pushstr(L, s ? s : "(null)");
			break;
		}
		case 'c': {
			char buff[2];
			buff[0] = (char)va_arg(argp, int);
			buff[1] = '\0';
			pushstr(L, buff);
			break;
		}
		case 'd':
			setivalue(L->top, va_arg(argp, int));
			incr_top(L);
			luaV_tostring(L, L->top - 1);
			break;
		case 'f':
			setnvalue(L->top, va_arg(argp, lua_Number));
			incr_top(L);
			luaV_tostring(L, L->top - 1);
			break;
		case 'p': {
			char buff[4 * sizeof(void*) + 8];
			snprintf(buff, sizeof(buff), "%p", va_arg(argp, void*));
			pushstr(L, buff);
			break;
		}
		case '%':
			pushstr(L, "%");
			break;
		default: {
			char buff[3];
			buff[0] = '%';
			buff[1] = *(e + 1);
			buff[2] = '\0';
			pushstr(L, buff);
			break;
		}
		}
		n += 2;
		fmt = e + 2;
	}
	pushstr(L, fmt);
	luaV_concat(L, n + 1, (int)(L->top - L->base) - 1);
	L->top -= n;
	return svalue(L->top - 1);
}

const char* luaO_pushfstring(lua_State* L, const char* fmt, ...)
{
	const char* msg;
	va_list argp;
	va_start(argp, fmt);
	msg = luaO_pushvfstring(L, fmt, argp);
	va_end(argp);
	return msg;
}

void luaO_chunkid(char* out, const char* source, int bufflen)
{
	if (*source == '=') {
		strncpy(out, source + 1, bufflen);  // remove first char
		out[bufflen - 1] = '\0';            // ensures null termination
	}
	else {  // out = "source", or "...source"
		if (*source == '@') {
			int l;
			source++;  // skip the `@'
			bufflen -= (int)sizeof(" `...' ");
			l = (int)strlen(source);
			strcpy(out, "");
			if (l > bufflen) {
				source += (l - bufflen);  // get last part of file name
				strcat(out, "...");
			}
			strcat(out, source);
		}
		else {  // out = [string "string"]
			int len = (int)strcspn(source, "\n");  // stop at first newline
			bufflen -= (int)sizeof(" [string \"...\"] ");
			if (len > bufflen)
				len = bufflen;
			strcpy(out, "[string \"");
			if (source[len] != '\0') {  // must truncate?
				strncat(out, source, len);
				strcat(out, "...");
			}
			else
				strcat(out, source);
			strcat(out, "\"]");
		}
	}
}

// ---- lzio -------------------------------------------------------------------------------------------

int luaZ_fill(ZIO* z)
{
	size_t size;
	const char* buff = z->reader(NULL, z->data, &size);
	if (buff == NULL || size == 0)
		return EOZ;
	z->n = size - 1;
	z->p = buff;
	return (int)((unsigned char)(*(z->p++)));
}

int luaZ_lookahead(ZIO* z)
{
	if (z->n == 0) {
		int c = luaZ_fill(z);
		if (c == EOZ)
			return c;
		z->n++;
		z->p--;
	}
	return (int)((unsigned char)(*z->p));
}

void luaZ_init(ZIO* z, lua_Chunkreader reader, void* data, const char* name)
{
	z->reader = reader;
	z->data = data;
	z->name = name;
	z->n = 0;
	z->p = NULL;
}

size_t luaZ_read(ZIO* z, void* b, size_t n)
{
	while (n) {
		size_t m;
		if (z->n == 0) {
			if (luaZ_fill(z) == EOZ)
				return n;  // return number of missing bytes
			else {
				++z->n;  // filbuf removed first byte; put back it
				--z->p;
			}
		}
		m = (n <= z->n) ? n : z->n;  // min. between n and z->n
		memcpy(b, z->p, m);
		z->n -= m;
		z->p += m;
		b = (char*)b + m;
		n -= m;
	}
	return 0;
}

// ---- ltm --------------------------------------------------------------------------------------------

// the original's names (0x79b660): `type' of an int is "int"
const char* const luaT_typenames[] = {
	"nil", "boolean", "userdata", "number", "int", "string", "table", "function", "userdata", "thread",
	"method", "proto", "upval"
};

void luaT_init(lua_State* L)
{
	static const char* const luaT_eventname[] = {  // ORDER TM (0x79b688)
		"__index", "__newindex", "__gc", "__mode", "__eq",
		"__add", "__sub", "__mul", "__div", "__pow", "__mod", "__unm", "__siz",
		"__lt", "__le", "__concat", "__call"
	};
	int i;
	for (i = 0; i < TM_N; i++) {
		G(L)->tmname[i] = luaS_new(L, luaT_eventname[i]);
		luaS_fix(G(L)->tmname[i]);  // never collect these names
	}
}

// function to be used with macro "fasttm": optimized for absence of tag methods
const TObject* luaT_gettm(Table* events, TMS event, TString* ename)
{
	const TObject* tm = luaH_getstr(events, ename);
	if (ttisnil(tm)) {  // no tag method?
		events->flags |= (lu_byte)(1u << event);  // cache this fact
		return NULL;
	}
	else
		return tm;
}

// 0x719c60
const TObject* luaT_gettmbyobj(lua_State* L, const TObject* o, TMS event)
{
	Table* mt;
	switch (ttype(o)) {
	case LUA_TTABLE:
		mt = hvalue(o)->metatable;
		break;
	case LUA_TUSERDATA:
		mt = uvalue(o)->metatable;
		break;
	default:
		mt = (ttype(o) <= LUA_TTHREAD) ? G(L)->mt[ttype(o)] : NULL;
	}
	return (mt ? luaH_getstr(mt, G(L)->tmname[event]) : &luaO_nilobject);
}
