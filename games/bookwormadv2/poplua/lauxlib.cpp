// The auxiliary library (Lua 5.0.2's lauxlib.c, 0x6f1a50-0x6f2e00). PopLua's differences: argument checks
// name the expected type directly, luaL_checkint reads ints, and the size of a table (luaL_getn) is the
// one luaL_setn stored, else a numeric field `n', else the 0-based border of the table.
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include "lauxlib.h"
#include "lcore.h"

#define abs_index(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

static poplua_PrintFunc printfunc;
static poplua_ReadFunc readfunc;
static poplua_ExistsFunc existsfunc;

void poplua_setprint(poplua_PrintFunc f)
{
	printfunc = f;
}

void poplua_setreader(poplua_ReadFunc f)
{
	readfunc = f;
}

void poplua_setexists(poplua_ExistsFunc f)
{
	existsfunc = f;
}

void poplua_print(const char* s, size_t len)
{
	if (printfunc)
		printfunc(s, len);
	else
		fwrite(s, 1, len, stdout);
}

int poplua_readfile(const char* path, char** data, size_t* size)
{
	if (readfunc)
		return readfunc(path, data, size);
	FILE* f = fopen(path, "rb");
	if (f == NULL)
		return 0;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	*data = (char*)malloc(n > 0 ? (size_t)n : 1);
	*size = n > 0 ? fread(*data, 1, (size_t)n, f) : 0;
	fclose(f);
	return 1;
}

int poplua_fileexists(const char* path)
{
	if (existsfunc)
		return existsfunc(path);
	char* data;
	size_t size;
	if (!poplua_readfile(path, &data, &size))
		return 0;
	free(data);
	return 1;
}

// ---- error-report functions --------------------------------------------------------------------------

int luaL_argerror(lua_State* L, int narg, const char* extramsg)
{
	lua_Debug ar;
	lua_getstack(L, 0, &ar);
	lua_getinfo(L, "n", &ar);
	if (strcmp(ar.namewhat, "method") == 0) {
		narg--;  // do not count `self'
		if (narg == 0)  // error is in the self argument itself?
			return luaL_error(L, "calling `%s' on bad self (%s)", ar.name, extramsg);
	}
	if (ar.name == NULL)
		ar.name = "?";
	return luaL_error(L, "bad argument #%d to `%s' (%s)", narg, ar.name, extramsg);
}

// 0x6f1b40: PopLua names the class of a C++ object (a full userdata; its metatable's name in the registry,
// as luaL_newmetatable records it) when it is the wrong one
int luaL_typerror(lua_State* L, int narg, const char* tname)
{
	const char* msg;
	if (lua_type(L, narg) == LUA_TUSERDATA) {
		const char* got = NULL;
		if (lua_getmetatable(L, narg)) {
			lua_rawget(L, LUA_REGISTRYINDEX);
			got = lua_tostring(L, -1);
		}
		msg = lua_pushfstring(L, "cobj:%s expected, got cobj:%s", tname, got != NULL ? got : "?");
	} else
		msg = lua_pushfstring(L, "%s expected, got %s", tname, lua_typename(L, lua_type(L, narg)));
	return luaL_argerror(L, narg, msg);
}

void luaL_where(lua_State* L, int level)
{
	lua_Debug ar;
	if (lua_getstack(L, level, &ar)) {  // check function at level
		lua_getinfo(L, "Snl", &ar);  // get info about it
		if (ar.currentline > 0) {  // is there info?
			lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
			return;
		}
	}
	lua_pushliteral(L, "");  // else, no information available...
}

int luaL_error(lua_State* L, const char* fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	luaL_where(L, 1);
	lua_pushvfstring(L, fmt, argp);
	va_end(argp);
	lua_concat(L, 2);
	return lua_error(L);
}

int luaL_findstring(const char* name, const char* const list[])
{
	int i;
	for (i = 0; list[i]; i++)
		if (strcmp(list[i], name) == 0)
			return i;
	return -1;  // name not found
}

int luaL_newmetatable(lua_State* L, const char* tname)
{
	lua_pushstring(L, tname);
	lua_rawget(L, LUA_REGISTRYINDEX);  // get registry.name
	if (!lua_isnil(L, -1))  // name already in use?
		return 0;  // leave previous value on top, but return 0
	lua_pop(L, 1);
	lua_newtable(L);  // create metatable
	lua_pushstring(L, tname);
	lua_pushvalue(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);  // registry.name = metatable
	lua_pushvalue(L, -1);
	lua_pushstring(L, tname);
	lua_rawset(L, LUA_REGISTRYINDEX);  // registry[metatable] = name
	return 1;
}

void luaL_getmetatable(lua_State* L, const char* tname)
{
	lua_pushstring(L, tname);
	lua_rawget(L, LUA_REGISTRYINDEX);
}

void* luaL_checkudata(lua_State* L, int ud, const char* tname)
{
	const char* tn;
	if (!lua_getmetatable(L, ud))
		return NULL;  // no metatable?
	lua_rawget(L, LUA_REGISTRYINDEX);  // get registry[metatable]
	tn = lua_tostring(L, -1);
	if (tn && (strcmp(tn, tname) == 0)) {
		lua_pop(L, 1);
		return lua_touserdata(L, ud);
	}
	else {
		lua_pop(L, 1);
		return NULL;
	}
}

void luaL_checkstack(lua_State* L, int space, const char* mes)
{
	if (!lua_checkstack(L, space))
		luaL_error(L, "stack overflow (%s)", mes);
}

void luaL_checktype(lua_State* L, int narg, int t)
{
	if (lua_type(L, narg) != t)
		luaL_typerror(L, narg, lua_typename(L, t));
}

void luaL_checkany(lua_State* L, int narg)
{
	if (lua_type(L, narg) == LUA_TNONE)
		luaL_argerror(L, narg, "value expected");
}

const char* luaL_checklstring(lua_State* L, int narg, size_t* len)
{
	const char* s = lua_tostring(L, narg);
	if (!s)
		luaL_typerror(L, narg, "string");
	if (len)
		*len = lua_strlen(L, narg);
	return s;
}

const char* luaL_optlstring(lua_State* L, int narg, const char* def, size_t* len)
{
	if (lua_isnoneornil(L, narg)) {
		if (len)
			*len = (def ? strlen(def) : 0);
		return def;
	}
	else
		return luaL_checklstring(L, narg, len);
}

// 0x6f22f0
lua_Number luaL_checknumber(lua_State* L, int narg)
{
	lua_Number d = lua_tonumber(L, narg);
	if (d == 0 && !lua_isnumber(L, narg))  // avoid extra test when d is not 0
		luaL_typerror(L, narg, "number");
	return d;
}

lua_Number luaL_optnumber(lua_State* L, int narg, lua_Number def)
{
	if (lua_isnoneornil(L, narg))
		return def;
	else
		return luaL_checknumber(L, narg);
}

// 0x6f2340
lua_Int luaL_checkint(lua_State* L, int narg)
{
	lua_Int d = lua_toint(L, narg);
	if (d == 0 && !lua_isint(L, narg))
		luaL_typerror(L, narg, "int");
	return d;
}

lua_Int luaL_optint(lua_State* L, int narg, lua_Int def)
{
	if (lua_isnoneornil(L, narg))
		return def;
	else
		return luaL_checkint(L, narg);
}

int luaL_getmetafield(lua_State* L, int obj, const char* event)
{
	if (!lua_getmetatable(L, obj))  // no metatable?
		return 0;
	lua_pushstring(L, event);
	lua_rawget(L, -2);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2);  // remove metatable and metafield
		return 0;
	}
	else {
		lua_remove(L, -2);  // remove only metatable
		return 1;
	}
}

int luaL_callmeta(lua_State* L, int obj, const char* event)
{
	obj = abs_index(L, obj);
	if (!luaL_getmetafield(L, obj, event))  // no metafield?
		return 0;
	lua_pushvalue(L, obj);
	lua_call(L, 1, 1);
	return 1;
}

void luaL_openlib(lua_State* L, const char* libname, const luaL_reg* l, int nup)
{
	if (libname) {
		lua_pushstring(L, libname);
		lua_gettable(L, LUA_GLOBALSINDEX);  // check whether lib already exists
		if (lua_isnil(L, -1)) {  // no?
			lua_pop(L, 1);
			lua_newtable(L);  // create it
			lua_pushstring(L, libname);
			lua_pushvalue(L, -2);
			lua_settable(L, LUA_GLOBALSINDEX);  // register it with given name
		}
		lua_insert(L, -(nup + 1));  // move library table to below upvalues
	}
	for (; l->name; l++) {
		int i;
		lua_pushstring(L, l->name);
		for (i = 0; i < nup; i++)  // copy upvalues to the top
			lua_pushvalue(L, -(nup + 1));
		lua_pushcclosure(L, l->func, nup);
		lua_settable(L, -(nup + 3));
	}
	lua_pop(L, nup);  // remove upvalues
}

// 0x6ef1c0 (lua_getfield in the original)
void luaL_getfield(lua_State* L, int idx, const char* k)
{
	idx = abs_index(L, idx);
	lua_pushstring(L, k);
	lua_gettable(L, idx);
}

void luaL_setfield(lua_State* L, int idx, const char* k)
{
	idx = abs_index(L, idx);
	lua_pushstring(L, k);
	lua_insert(L, -2);
	lua_settable(L, idx);
}

// ---- getn and setn -----------------------------------------------------------------------------------

// 0x6f2790: the int on top (an int, a number or an int string), or -1; pops `topop' values
static int checkint(lua_State* L, int topop)
{
	int n = lua_isint(L, -1) ? lua_toint(L, -1) : -1;
	lua_pop(L, topop);
	return n;
}

// 0x6f27f0
static void getsizes(lua_State* L)
{
	luaL_getfield(L, LUA_REGISTRYINDEX, "LUA_SIZES");
	if (lua_isnil(L, -1)) {  // no `size' table?
		lua_pop(L, 1);  // remove nil
		lua_newtable(L);  // create it
		lua_pushvalue(L, -1);  // `size' will be its own metatable
		lua_setmetatable(L, -2);
		lua_pushliteral(L, "k");
		luaL_setfield(L, -2, "__mode");  // metatable(N).__mode = "k"
		lua_pushvalue(L, -1);
		luaL_setfield(L, LUA_REGISTRYINDEX, "LUA_SIZES");  // store in register
	}
}

// 0x6f28f0: sizes[t] = n
void luaL_setn(lua_State* L, int t, int n)
{
	t = abs_index(L, t);
	getsizes(L);
	lua_pushvalue(L, t);
	lua_pushint(L, n);
	lua_rawset(L, -3);  // sizes[t] = n
	lua_pop(L, 1);  // remove `sizes'
}

// 0x6f29c0: sizes[t], else t.n, else the border of the 0-based sequence
int luaL_getn(lua_State* L, int t)
{
	int n;
	t = abs_index(L, t);
	getsizes(L);  // try sizes[t]
	lua_pushvalue(L, t);
	lua_rawget(L, -2);
	if ((n = checkint(L, 2)) >= 0)
		return n;
	luaL_getfield(L, t, "n");  // else try t.n
	if ((n = checkint(L, 1)) >= 0)
		return n;
	return (int)lua_objlen(L, t);
}

// ---- generic buffer manipulation ---------------------------------------------------------------------

#define bufflen(B) ((B)->p - (B)->buffer)
#define bufffree(B) ((size_t)(LUAL_BUFFERSIZE - bufflen(B)))
#define LIMIT (LUA_MINSTACK / 2)

static int emptybuffer(luaL_Buffer* B)
{
	size_t l = bufflen(B);
	if (l == 0)
		return 0;  // put nothing on stack
	else {
		lua_pushlstring(B->L, B->buffer, l);
		B->p = B->buffer;
		B->lvl++;
		return 1;
	}
}

static void adjuststack(luaL_Buffer* B)
{
	if (B->lvl > 1) {
		lua_State* L = B->L;
		int toget = 1;  // number of levels to concat
		size_t toplen = lua_strlen(L, -1);
		do {
			size_t l = lua_strlen(L, -(toget + 1));
			if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
				toplen += l;
				toget++;
			}
			else
				break;
		} while (toget < B->lvl);
		lua_concat(L, toget);
		B->lvl = B->lvl - toget + 1;
	}
}

char* luaL_prepbuffer(luaL_Buffer* B)
{
	if (emptybuffer(B))
		adjuststack(B);
	return B->buffer;
}

void luaL_addlstring(luaL_Buffer* B, const char* s, size_t l)
{
	while (l--)
		luaL_putchar(B, *s++);
}

void luaL_addstring(luaL_Buffer* B, const char* s)
{
	luaL_addlstring(B, s, strlen(s));
}

void luaL_pushresult(luaL_Buffer* B)
{
	emptybuffer(B);
	lua_concat(B->L, B->lvl);
	B->lvl = 1;
}

void luaL_addvalue(luaL_Buffer* B)
{
	lua_State* L = B->L;
	size_t vl = lua_strlen(L, -1);
	if (vl <= bufffree(B)) {  // fit into buffer?
		memcpy(B->p, lua_tostring(L, -1), vl);  // put it there
		B->p += vl;
		lua_pop(L, 1);  // remove from stack
	}
	else {
		if (emptybuffer(B))
			lua_insert(L, -2);  // put buffer before new value
		B->lvl++;  // add new value into B stack
		adjuststack(B);
	}
}

void luaL_buffinit(lua_State* L, luaL_Buffer* B)
{
	B->L = L;
	B->p = B->buffer;
	B->lvl = 0;
}

// ---- references --------------------------------------------------------------------------------------

#define FREELIST_REF 0  // free list of references
#define RESERVED_REFS 1

int luaL_ref(lua_State* L, int t)
{
	int ref;
	t = abs_index(L, t);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);  // remove from stack
		return LUA_REFNIL;  // `nil' has a unique fixed reference
	}
	lua_rawgeti(L, t, FREELIST_REF);  // get first free element
	ref = lua_toint(L, -1);  // ref = t[FREELIST_REF]
	lua_pop(L, 1);  // remove it from stack
	if (ref != 0) {  // any free element?
		lua_rawgeti(L, t, ref);  // remove it from list
		lua_rawseti(L, t, FREELIST_REF);  // (t[FREELIST_REF] = t[ref])
	}
	else {  // no free elements
		ref = luaL_getn(L, t);
		if (ref < RESERVED_REFS)
			ref = RESERVED_REFS;  // skip reserved references
		ref++;  // create new reference
		luaL_setn(L, t, ref);
	}
	lua_rawseti(L, t, ref);
	return ref;
}

void luaL_unref(lua_State* L, int t, int ref)
{
	if (ref >= 0) {
		t = abs_index(L, t);
		lua_rawgeti(L, t, FREELIST_REF);
		lua_rawseti(L, t, ref);  // t[ref] = t[FREELIST_REF]
		lua_pushint(L, ref);
		lua_rawseti(L, t, FREELIST_REF);  // t[FREELIST_REF] = ref
	}
}

// ---- load functions ----------------------------------------------------------------------------------

struct LoadS {
	const char* s;
	size_t size;
};

static const char* getS(lua_State* L, void* ud, size_t* size)
{
	LoadS* ls = (LoadS*)ud;
	(void)L;
	if (ls->size == 0)
		return NULL;
	*size = ls->size;
	ls->size = 0;
	return ls->s;
}

int luaL_loadbuffer(lua_State* L, const char* buff, size_t size, const char* name)
{
	LoadS ls;
	ls.s = buff;
	ls.size = size;
	return lua_load(L, getS, &ls, name);
}

// The current directory as the scripts see it: "" (the default) is the game's data folder, which keeps the
// paths of the port's scripts relative (the original's are absolute, under the install folder)
static char cwd[260];

void poplua_setcwd(const char* dir)
{
	snprintf(cwd, sizeof cwd, "%s", dir);
}

const char* poplua_getcwd(void)
{
	return cwd;
}

// 0x6cbb60: name made absolute against the current directory (leading ..\ go up from it), with backslashes
void poplua_fullpath(const char* name, char* out, size_t size)
{
	char buf[264];
	if (name[0] == '\\' || name[0] == '/' || (name[0] != '\0' && name[1] == ':'))
		snprintf(buf, sizeof buf, "%s", name);
	else {
		snprintf(buf, sizeof buf - 1, "%s", cwd);
		size_t n = strlen(buf);
		if (n > 0 && buf[n - 1] != '\\')
			strcat(buf, "\\");
		while (name[0] == '.' && name[1] == '.') {
			if (n > 0)
				n--;
			while (n > 1 && buf[n - 1] != '\\')
				n--;
			name += 3;
			buf[n] = '\0';
		}
		size_t end = strlen(buf);
		snprintf(buf + end, sizeof buf - end, "%s", name);
	}
	for (char* p = buf; *p; p++)
		if (*p == '/')
			*p = '\\';
	snprintf(out, size, "%s", buf);
}

// 0x6f3600: a file is read under its full path, and its functions get "@" and that path as their source. A
// name that is not found is read as its compiled file: a .lua as the .luc of the same name, any other name
// with .luc added. (The original also reads the .luc in place of a .lua that is there when the .luc is newer:
// a feature for its developers.)
int luaL_loadfile(lua_State* L, const char* filename)
{
	char* data;
	size_t size;
	int status;
	int fnameindex = lua_gettop(L) + 1;  // index of filename on the stack
	char full[264];
	if (filename[0] != '\0' && filename[1] != ':')
		poplua_fullpath(filename, full, sizeof full);
	else
		snprintf(full, sizeof full, "%s", filename);
	lua_pushfstring(L, "@%s", full);
	if (!poplua_readfile(full, &data, &size)) {
		char luc[268];
		snprintf(luc, sizeof luc - 4, "%s", full);
		char* ext = strrchr(luc, '.');
		if (ext && tolower((unsigned char)ext[1]) == 'l' && tolower((unsigned char)ext[2]) == 'u' &&
			tolower((unsigned char)ext[3]) == 'a' && ext[4] == '\0')
			ext[3] = 'c';
		else
			strcat(luc, ".luc");
		if (!poplua_readfile(luc, &data, &size)) {
			lua_pushfstring(L, "cannot read %s: %s", filename, strerror(ENOENT));
			lua_remove(L, fnameindex);
			return LUA_ERRFILE;
		}
	}
	luaU_loadname = full;  // (lua_load catches the errors)
	status = luaL_loadbuffer(L, data, size, lua_tostring(L, -1));
	luaU_loadname = NULL;
	free(data);
	lua_remove(L, fnameindex);
	return status;
}

// ---- compatibility code ------------------------------------------------------------------------------

static void callalert(lua_State* L, int status)
{
	if (status != 0) {
		lua_pushliteral(L, "_ALERT");
		lua_gettable(L, LUA_GLOBALSINDEX);
		if (lua_isfunction(L, -1)) {
			lua_insert(L, -2);
			lua_call(L, 1, 0);
		}
		else {  // no _ALERT function; print it on stderr
			fprintf(stderr, "%s\n", lua_tostring(L, -2));
			lua_pop(L, 2);  // remove error message and _ALERT
		}
	}
}

static int aux_do(lua_State* L, int status)
{
	if (status == 0)  // parse OK?
		status = lua_pcall(L, 0, LUA_MULTRET, 0);  // call main
	callalert(L, status);
	return status;
}

int lua_dofile(lua_State* L, const char* filename)
{
	return aux_do(L, luaL_loadfile(L, filename));
}

int lua_dobuffer(lua_State* L, const char* buff, size_t size, const char* name)
{
	return aux_do(L, luaL_loadbuffer(L, buff, size, name));
}

int lua_dostring(lua_State* L, const char* str)
{
	return lua_dobuffer(L, str, strlen(str), str);
}
