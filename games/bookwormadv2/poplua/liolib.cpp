// The io and os libraries (0x70bad0-0x70e430), reduced to what a sandboxed port needs: files live in memory,
// read through the host's reader (poplua_setreader) and written through its writer when they are closed or
// flushed (poplua_setwriter; without one, opening for writing fails). Lua 5.0.2's read formats, lines and
// seek work on them. os keeps the clock and the calendar; it runs no commands and touches no files.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#include "lauxlib.h"

#define FILEHANDLE "FILE*"

static poplua_WriteFunc writefunc;

void poplua_setwriter(poplua_WriteFunc f)
{
	writefunc = f;
}

struct MemFile {
	std::string* path;
	std::string* data;
	size_t pos;
	int writable;
	int dirty;
	int closed;
};

static MemFile* tofile(lua_State* L, int findex)
{
	MemFile* f = (MemFile*)luaL_checkudata(L, findex, FILEHANDLE);
	if (f == NULL)
		luaL_argerror(L, findex, "bad file");
	if (f->closed)
		luaL_error(L, "attempt to use a closed file");
	return f;
}

static int pushresult(lua_State* L, int i, const char* filename)
{
	if (i) {
		lua_pushboolean(L, 1);
		return 1;
	}
	lua_pushnil(L);
	if (filename)
		lua_pushfstring(L, "%s: cannot open", filename);
	else
		lua_pushliteral(L, "cannot open");
	lua_pushint(L, 0);
	return 3;
}

static int flushfile(MemFile* f)
{
	if (f->writable && f->dirty) {
		f->dirty = 0;
		return writefunc != NULL && writefunc(f->path->c_str(), f->data->data(), f->data->size());
	}
	return 1;
}

static int aux_close(lua_State* L, MemFile* f)
{
	int ok = flushfile(f);
	f->closed = 1;
	delete f->path;
	delete f->data;
	f->path = f->data = NULL;
	(void)L;
	return ok;
}

static int io_gc(lua_State* L)
{
	MemFile* f = (MemFile*)luaL_checkudata(L, 1, FILEHANDLE);
	if (f != NULL && !f->closed)
		aux_close(L, f);
	return 0;
}

static int io_tostring(lua_State* L)
{
	char buff[32];
	MemFile* f = (MemFile*)luaL_checkudata(L, 1, FILEHANDLE);
	if (f->closed)
		strcpy(buff, "closed");
	else
		snprintf(buff, sizeof(buff), "%08llX", (unsigned long long)(uintptr_t)f);
	lua_pushfstring(L, "file (%s)", buff);
	return 1;
}

// io.open(name [, mode]): "r" reads through the host; "w", "a" and "+" modes need its writer
static int io_open(lua_State* L)
{
	const char* filename = luaL_checkstring(L, 1);
	const char* mode = luaL_optstring(L, 2, "r");
	int writable = strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+');
	std::string* data = new std::string();
	if (mode[0] != 'w') {  // "r", "r+", "a": the old contents
		char* buf;
		size_t size;
		if (poplua_readfile(filename, &buf, &size)) {
			data->assign(buf, size);
			free(buf);
		}
		else if (mode[0] == 'r') {
			delete data;
			return pushresult(L, 0, filename);
		}
	}
	if (writable && writefunc == NULL) {
		delete data;
		return pushresult(L, 0, filename);
	}
	MemFile* f = (MemFile*)lua_newuserdata(L, sizeof(MemFile));
	f->path = new std::string(filename);
	f->data = data;
	f->pos = mode[0] == 'a' ? data->size() : 0;
	f->writable = writable;
	f->dirty = mode[0] == 'w';
	f->closed = 0;
	luaL_getmetatable(L, FILEHANDLE);
	lua_setmetatable(L, -2);
	return 1;
}

static int io_close(lua_State* L)
{
	return pushresult(L, aux_close(L, tofile(L, 1)), NULL);
}

static int io_type(lua_State* L)
{
	MemFile* f = (MemFile*)luaL_checkudata(L, 1, FILEHANDLE);
	if (f == NULL)
		lua_pushnil(L);
	else if (f->closed)
		lua_pushliteral(L, "closed file");
	else
		lua_pushliteral(L, "file");
	return 1;
}

static int f_flush(lua_State* L)
{
	return pushresult(L, flushfile(tofile(L, 1)), NULL);
}

// ---- reading -----------------------------------------------------------------------------------------

static int read_number(lua_State* L, MemFile* f)
{
	const char* s = f->data->c_str() + f->pos;
	char* end;
	lua_Number d = strtod(s, &end);
	if (end == s)
		return 0;
	f->pos += (size_t)(end - s);
	lua_pushnumber(L, d);
	return 1;
}

static int test_eof(lua_State* L, MemFile* f)
{
	lua_pushlstring(L, NULL, 0);
	return f->pos < f->data->size();
}

static int read_line(lua_State* L, MemFile* f)
{
	const std::string& d = *f->data;
	if (f->pos >= d.size())
		return 0;
	size_t e = d.find('\n', f->pos);
	size_t end = (e == std::string::npos) ? d.size() : e;
	lua_pushlstring(L, d.data() + f->pos, end - f->pos);
	f->pos = (e == std::string::npos) ? d.size() : e + 1;
	return 1;
}

static int read_chars(lua_State* L, MemFile* f, size_t n)
{
	const std::string& d = *f->data;
	size_t avail = f->pos < d.size() ? d.size() - f->pos : 0;
	if (n > avail)
		n = avail;
	lua_pushlstring(L, d.data() + f->pos, n);
	f->pos += n;
	return n > 0;
}

static int g_read(lua_State* L, MemFile* f, int first)
{
	int nargs = lua_gettop(L) - 1;
	int success;
	int n;
	if (nargs == 0) {  // no arguments?
		success = read_line(L, f);
		n = first + 1;  // to return 1 result
	}
	else {  // ensure stack space for all results and for auxlib's buffer
		luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
		success = 1;
		for (n = first; nargs-- && success; n++) {
			if (lua_type(L, n) == LUA_TNUMBER || lua_type(L, n) == LUA_TINT) {
				size_t l = (size_t)lua_toint(L, n);
				success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
			}
			else {
				const char* p = lua_tostring(L, n);
				luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
				switch (p[1]) {
				case 'n':  // number
					success = read_number(L, f);
					break;
				case 'l':  // line
					success = read_line(L, f);
					break;
				case 'a':  // file
					read_chars(L, f, ~(size_t)0);  // read MAX_SIZE_T chars
					success = 1;  // always success
					break;
				case 'w':  // word
					return luaL_error(L, "obsolete option `*w' to `read'");
				default:
					return luaL_argerror(L, n, "invalid format");
				}
			}
		}
	}
	if (!success) {
		lua_pop(L, 1);  // remove last result
		lua_pushnil(L);  // push nil instead
	}
	return n - first;
}

static int f_read(lua_State* L)
{
	return g_read(L, tofile(L, 1), 2);
}

static int io_readline(lua_State* L)
{
	MemFile* f = (MemFile*)lua_touserdata(L, lua_upvalueindex(2));
	if (f == NULL || f->closed)
		luaL_error(L, "file is already closed");
	if (read_line(L, f))
		return 1;
	if (lua_toboolean(L, lua_upvalueindex(3))) {  // generator created file?
		lua_settop(L, 0);
		lua_pushvalue(L, lua_upvalueindex(2));
		aux_close(L, f);
	}
	return 0;
}

static void aux_lines(lua_State* L, int idx, int close)
{
	lua_pushliteral(L, FILEHANDLE);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, idx);
	lua_pushboolean(L, close);  // close/not close file when finished
	lua_pushcclosure(L, io_readline, 3);
}

static int f_lines(lua_State* L)
{
	tofile(L, 1);  // check that it's a valid file handle
	aux_lines(L, 1, 0);
	return 1;
}

static int io_lines(lua_State* L)
{
	luaL_checkstring(L, 1);
	lua_settop(L, 1);
	lua_pushliteral(L, "r");
	int n = io_open(L);
	if (lua_isnil(L, -n))
		luaL_error(L, "%s", lua_tostring(L, -n + 1));
	aux_lines(L, lua_gettop(L), 1);
	return 1;
}

// ---- writing -----------------------------------------------------------------------------------------

static int g_write(lua_State* L, MemFile* f, int arg)
{
	int nargs = lua_gettop(L) - 1;
	if (!f->writable)
		return pushresult(L, 0, NULL);
	for (; nargs--; arg++) {
		size_t l;
		const char* s = luaL_checklstring(L, arg, &l);
		if (f->pos > f->data->size())
			f->data->resize(f->pos);
		f->data->replace(f->pos, l, s, l);
		f->pos += l;
		f->dirty = 1;
	}
	return pushresult(L, 1, NULL);
}

static int f_write(lua_State* L)
{
	return g_write(L, tofile(L, 1), 2);
}

static int f_seek(lua_State* L)
{
	static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
	static const char* const modenames[] = {"set", "cur", "end", NULL};
	MemFile* f = tofile(L, 1);
	int op = luaL_findstring(luaL_optstring(L, 2, "cur"), modenames);
	long offset = (long)luaL_optint(L, 3, 0);
	luaL_argcheck(L, op != -1, 2, "invalid mode");
	long base = mode[op] == SEEK_SET ? 0 : mode[op] == SEEK_CUR ? (long)f->pos : (long)f->data->size();
	if (base + offset < 0) {
		lua_pushnil(L);
		lua_pushliteral(L, "invalid seek");
		return 2;
	}
	f->pos = (size_t)(base + offset);
	lua_pushint(L, (lua_Int)f->pos);
	return 1;
}

static const luaL_reg iolib[] = {
	{"close", io_close},
	{"lines", io_lines},
	{"open", io_open},
	{"type", io_type},
	{NULL, NULL}
};

static const luaL_reg flib[] = {
	{"flush", f_flush},
	{"read", f_read},
	{"lines", f_lines},
	{"seek", f_seek},
	{"write", f_write},
	{"close", io_close},
	{"__gc", io_gc},
	{"__tostring", io_tostring},
	{NULL, NULL}
};

// ---- os ----------------------------------------------------------------------------------------------

static int io_clock(lua_State* L)
{
	lua_pushnumber(L, ((lua_Number)clock()) / (lua_Number)CLOCKS_PER_SEC);
	return 1;
}

static void setfield(lua_State* L, const char* key, int value)
{
	lua_pushstring(L, key);
	lua_pushint(L, value);
	lua_rawset(L, -3);
}

static void setboolfield(lua_State* L, const char* key, int value)
{
	lua_pushstring(L, key);
	lua_pushboolean(L, value);
	lua_rawset(L, -3);
}

static int getboolfield(lua_State* L, const char* key)
{
	int res;
	lua_pushstring(L, key);
	lua_gettable(L, -2);
	res = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return res;
}

static int getfield(lua_State* L, const char* key, int d)
{
	int res;
	lua_pushstring(L, key);
	lua_gettable(L, -2);
	if (lua_isnumber(L, -1))
		res = lua_toint(L, -1);
	else {
		if (d == -2)
			return luaL_error(L, "field `%s' missing in date table", key);
		res = d;
	}
	lua_pop(L, 1);
	return res;
}

static int io_date(lua_State* L)
{
	const char* s = luaL_optstring(L, 1, "%c");
	time_t t = (time_t)(luaL_optnumber(L, 2, -1));
	struct tm* stm;
	if (t == (time_t)(-1))  // no time given?
		t = time(NULL);  // use current time
	if (*s == '!') {  // UTC?
		stm = gmtime(&t);
		s++;  // skip `!'
	}
	else
		stm = localtime(&t);
	if (stm == NULL)  // invalid date?
		lua_pushnil(L);
	else if (strcmp(s, "*t") == 0) {
		lua_newtable(L);
		setfield(L, "sec", stm->tm_sec);
		setfield(L, "min", stm->tm_min);
		setfield(L, "hour", stm->tm_hour);
		setfield(L, "day", stm->tm_mday);
		setfield(L, "month", stm->tm_mon + 1);
		setfield(L, "year", stm->tm_year + 1900);
		setfield(L, "wday", stm->tm_wday + 1);
		setfield(L, "yday", stm->tm_yday + 1);
		setboolfield(L, "isdst", stm->tm_isdst);
	}
	else {
		char b[256];
		if (strftime(b, sizeof(b), s, stm))
			lua_pushstring(L, b);
		else
			return luaL_error(L, "`date' format too long");
	}
	return 1;
}

static int io_time(lua_State* L)
{
	if (lua_isnoneornil(L, 1))  // called without args?
		lua_pushnumber(L, (lua_Number)time(NULL));  // return current time
	else {
		time_t t;
		struct tm ts;
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_settop(L, 1);  // make sure table is at the top
		ts.tm_sec = getfield(L, "sec", 0);
		ts.tm_min = getfield(L, "min", 0);
		ts.tm_hour = getfield(L, "hour", 12);
		ts.tm_mday = getfield(L, "day", -2);
		ts.tm_mon = getfield(L, "month", -2) - 1;
		ts.tm_year = getfield(L, "year", -2) - 1900;
		ts.tm_isdst = getboolfield(L, "isdst");
		t = mktime(&ts);
		if (t == (time_t)(-1))
			lua_pushnil(L);
		else
			lua_pushnumber(L, (lua_Number)t);
	}
	return 1;
}

static int io_difftime(lua_State* L)
{
	lua_pushnumber(L, difftime((time_t)(luaL_checknumber(L, 1)), (time_t)(luaL_optnumber(L, 2, 0))));
	return 1;
}

static int io_getenv(lua_State* L)
{
	luaL_checkstring(L, 1);
	lua_pushnil(L);  // the environment is not exposed
	return 1;
}

static int io_exit(lua_State* L)
{
	return luaL_error(L, "os.exit is not available");
}

static const luaL_reg syslib[] = {
	{"clock", io_clock},
	{"date", io_date},
	{"difftime", io_difftime},
	{"exit", io_exit},
	{"getenv", io_getenv},
	{"time", io_time},
	{NULL, NULL}
};

// 0x70e430
int luaopen_io(lua_State* L)
{
	luaL_openlib(L, LUA_OSLIBNAME, syslib, 0);
	luaL_newmetatable(L, FILEHANDLE);
	// file methods
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	luaL_openlib(L, NULL, flib, 0);
	lua_pop(L, 1);
	luaL_openlib(L, LUA_IOLIBNAME, iolib, 0);
	return 1;
}
