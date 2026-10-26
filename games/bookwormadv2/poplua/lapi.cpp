// The C API (Lua 5.0.2's lapi.c). PopLua's additions: ints (lua_pushint, lua_toint, and numbers that accept
// them), metatables for the basic types (5.1's G->mt), and the class parent and protection flags of tables.
#include "lcore.h"

[[maybe_unused]] const char lua_ident[] = "$Lua: " LUA_VERSION " $";

#define LUA_MAXCSTACK 2048
#define api_incr_top(L) { L->top++; }
#define api_checknelems(L, n) ((void)0)

static TObject* negindex(lua_State* L, int idx)
{
	if (idx > LUA_REGISTRYINDEX)
		return L->top + idx;
	else
		switch (idx) {  // pseudo-indices
		case LUA_REGISTRYINDEX:
			return registry(L);
		case LUA_GLOBALSINDEX:
			return gt(L);
		default: {
			TObject* func = (L->base - 1);
			idx = LUA_GLOBALSINDEX - idx;
			return (idx <= clvalue(func)->c.nupvalues) ? &clvalue(func)->c.upvalue[idx - 1] : NULL;
		}
		}
}

static TObject* luaA_index(lua_State* L, int idx)
{
	if (idx > 0)
		return L->base + (idx - 1);
	else
		return negindex(L, idx);
}

static TObject* luaA_indexAcceptable(lua_State* L, int idx)
{
	if (idx > 0) {
		TObject* o = L->base + (idx - 1);
		if (o >= L->top)
			return NULL;
		else
			return o;
	}
	else
		return negindex(L, idx);
}

void luaA_pushobject(lua_State* L, const TObject* o)
{
	setobj2s(L->top, o);
	incr_top(L);
}

int lua_checkstack(lua_State* L, int size)
{
	int res;
	if ((L->top - L->base + size) > LUA_MAXCSTACK)
		res = 0;  // stack overflow
	else {
		luaD_checkstack(L, size);
		if (L->ci->top < L->top + size)
			L->ci->top = L->top + size;
		res = 1;
	}
	return res;
}

lua_CFunction lua_atpanic(lua_State* L, lua_CFunction panicf)
{
	lua_CFunction old = G(L)->panic;
	G(L)->panic = panicf;
	return old;
}

const char* lua_version(void)
{
	return LUA_VERSION;
}

lua_Int lua_ftol(lua_Number d)
{
	return luaV_ftol(d);
}

int lua_str2int(const char* s, lua_Int* result)
{
	return luaO_str2int(s, result);
}

int lua_str2number(const char* s, lua_Number* result)
{
	return luaO_str2d(s, result);
}

// ---- basic stack manipulation ------------------------------------------------------------------------

int lua_gettop(lua_State* L)
{
	return (int)(L->top - L->base);
}

void lua_settop(lua_State* L, int idx)
{
	if (idx >= 0) {
		while (L->top < L->base + idx)
			setnilvalue(L->top++);
		L->top = L->base + idx;
	}
	else
		L->top += idx + 1;  // `subtract' index (index is negative)
}

void lua_remove(lua_State* L, int idx)
{
	StkId p = luaA_index(L, idx);
	while (++p < L->top)
		setobjs2s(p - 1, p);
	L->top--;
}

void lua_insert(lua_State* L, int idx)
{
	StkId p;
	StkId q;
	p = luaA_index(L, idx);
	for (q = L->top; q > p; q--)
		setobjs2s(q, q - 1);
	setobjs2s(p, L->top);
}

void lua_replace(lua_State* L, int idx)
{
	setobj(luaA_index(L, idx), L->top - 1);  // write barrier
	L->top--;
}

void lua_xmove(lua_State* from, lua_State* to, int n)
{
	int i;
	from->top -= n;
	for (i = 0; i < n; i++) {
		setobj2s(to->top, from->top + i);
		api_incr_top(to);
	}
}

void lua_pushvalue(lua_State* L, int idx)
{
	setobj2s(L->top, luaA_index(L, idx));
	api_incr_top(L);
}

// ---- access functions (stack -> C) -------------------------------------------------------------------

int lua_type(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	if (o == NULL)
		return LUA_TNONE;
	return ttype(o) == LUA_TMETHOD ? LUA_TFUNCTION : ttype(o);
}

const char* lua_typename(lua_State* L, int t)
{
	(void)L;
	return (t == LUA_TNONE) ? "no value" : luaT_typenames[t];
}

int lua_iscfunction(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	return (o == NULL) ? 0 : (ttype(o) == LUA_TFUNCTION || ttype(o) == LUA_TMETHOD) && clvalue(o)->c.isC;
}

int lua_isnumber(lua_State* L, int idx)
{
	TObject n;
	const TObject* o = luaA_indexAcceptable(L, idx);
	return (o != NULL && luaV_tonumber(o, &n) != NULL);
}

int lua_isstring(lua_State* L, int idx)
{
	int t = lua_type(L, idx);
	return (t == LUA_TSTRING || t == LUA_TNUMBER || t == LUA_TINT);
}

int lua_isuserdata(lua_State* L, int idx)
{
	const TObject* o = luaA_indexAcceptable(L, idx);
	return (o != NULL && (ttisuserdata(o) || ttislightuserdata(o)));
}

int lua_rawequal(lua_State* L, int index1, int index2)
{
	StkId o1 = luaA_indexAcceptable(L, index1);
	StkId o2 = luaA_indexAcceptable(L, index2);
	return (o1 == NULL || o2 == NULL) ? 0 : luaO_rawequalObj(o1, o2);
}

int lua_equal(lua_State* L, int index1, int index2)
{
	StkId o1 = luaA_indexAcceptable(L, index1);
	StkId o2 = luaA_indexAcceptable(L, index2);
	return (o1 == NULL || o2 == NULL) ? 0 : equalobj(L, o1, o2);
}

int lua_lessthan(lua_State* L, int index1, int index2)
{
	StkId o1 = luaA_indexAcceptable(L, index1);
	StkId o2 = luaA_indexAcceptable(L, index2);
	return (o1 == NULL || o2 == NULL) ? 0 : luaV_lessthan(L, o1, o2);
}

lua_Number lua_tonumber(lua_State* L, int idx)
{
	TObject n;
	const TObject* o = luaA_indexAcceptable(L, idx);
	if (o != NULL && (o = luaV_tonumber(o, &n)) != NULL)
		return nvalue(o);
	else
		return 0;
}

lua_Int lua_toint(lua_State* L, int idx)
{
	TObject n;
	const TObject* o = luaA_indexAcceptable(L, idx);
	if (o != NULL && (o = luaV_toint(o, &n)) != NULL)
		return ivalue(o);
	return 0;
}

int lua_isint(lua_State* L, int idx)
{
	TObject n;
	const TObject* o = luaA_indexAcceptable(L, idx);
	return (o != NULL && luaV_toint(o, &n) != NULL);
}

int lua_toboolean(lua_State* L, int idx)
{
	const TObject* o = luaA_indexAcceptable(L, idx);
	return (o != NULL) && !l_isfalse(o);
}

const char* lua_tostring(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	if (o == NULL)
		return NULL;
	else if (ttisstring(o))
		return svalue(o);
	else
		return luaV_tostring(L, o) ? svalue(o) : NULL;
}

size_t lua_strlen(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	if (o == NULL)
		return 0;
	else if (ttisstring(o))
		return tsvalue(o)->len;
	else
		return luaV_tostring(L, o) ? tsvalue(o)->len : 0;
}

// 0x6eee00
size_t lua_objlen(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	if (o == NULL)
		return 0;
	switch (ttype(o)) {
	case LUA_TSTRING:
		return tsvalue(o)->len;
	case LUA_TUSERDATA:
		return uvalue(o)->len;
	case LUA_TTABLE:
		return (size_t)luaH_getn(hvalue(o));
	case LUA_TNUMBER:
	case LUA_TINT:
		return luaV_tostring(L, o) ? tsvalue(o)->len : 0;
	default:
		return 0;
	}
}

lua_CFunction lua_tocfunction(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	return (o == NULL || !lua_iscfunction(L, idx)) ? NULL : clvalue(o)->c.f;
}

void* lua_touserdata(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	if (o == NULL)
		return NULL;
	switch (ttype(o)) {
	case LUA_TUSERDATA:
		return udata_block(uvalue(o));
	case LUA_TLIGHTUSERDATA:
		return pvalue(o);
	default:
		return NULL;
	}
}

lua_State* lua_tothread(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	return (o == NULL || !ttisthread(o)) ? NULL : thvalue(o);
}

const void* lua_topointer(lua_State* L, int idx)
{
	StkId o = luaA_indexAcceptable(L, idx);
	if (o == NULL)
		return NULL;
	switch (ttype(o)) {
	case LUA_TTABLE:
		return hvalue(o);
	case LUA_TFUNCTION:
	case LUA_TMETHOD:
		return clvalue(o);
	case LUA_TTHREAD:
		return thvalue(o);
	case LUA_TUSERDATA:
	case LUA_TLIGHTUSERDATA:
		return lua_touserdata(L, idx);
	default:
		return NULL;
	}
}

// ---- push functions (C -> stack) ---------------------------------------------------------------------

void lua_pushnil(lua_State* L)
{
	setnilvalue(L->top);
	api_incr_top(L);
}

void lua_pushnumber(lua_State* L, lua_Number n)
{
	setnvalue(L->top, n);
	api_incr_top(L);
}

void lua_pushint(lua_State* L, lua_Int n)
{
	setivalue(L->top, n);
	api_incr_top(L);
}

void lua_pushlstring(lua_State* L, const char* s, size_t len)
{
	luaC_checkGC(L);
	setsvalue2s(L->top, luaS_newlstr(L, s, len));
	api_incr_top(L);
}

void lua_pushstring(lua_State* L, const char* s)
{
	if (s == NULL)
		lua_pushnil(L);
	else
		lua_pushlstring(L, s, strlen(s));
}

const char* lua_pushvfstring(lua_State* L, const char* fmt, va_list argp)
{
	luaC_checkGC(L);
	return luaO_pushvfstring(L, fmt, argp);
}

const char* lua_pushfstring(lua_State* L, const char* fmt, ...)
{
	const char* ret;
	va_list argp;
	luaC_checkGC(L);
	va_start(argp, fmt);
	ret = luaO_pushvfstring(L, fmt, argp);
	va_end(argp);
	return ret;
}

void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n)
{
	Closure* cl;
	luaC_checkGC(L);
	cl = luaF_newCclosure(L, n);
	cl->c.f = fn;
	L->top -= n;
	while (n--)
		setobj2n(&cl->c.upvalue[n], L->top + n);
	setclvalue(L->top, cl);
	api_incr_top(L);
}

void lua_pushboolean(lua_State* L, int b)
{
	setbvalue(L->top, (b != 0));  // ensure that true is 1
	api_incr_top(L);
}

void lua_pushlightuserdata(lua_State* L, void* p)
{
	setpvalue(L->top, p);
	api_incr_top(L);
}

// ---- get functions (Lua -> stack) --------------------------------------------------------------------

void lua_gettable(lua_State* L, int idx)
{
	StkId t = luaA_index(L, idx);
	setobj2s(L->top - 1, luaV_gettable(L, t, L->top - 1, 0));
}

void lua_rawget(lua_State* L, int idx)
{
	StkId t = luaA_index(L, idx);
	setobj2s(L->top - 1, luaH_get(hvalue(t), L->top - 1));
}

void lua_rawgeti(lua_State* L, int idx, int n)
{
	StkId o = luaA_index(L, idx);
	setobj2s(L->top, luaH_getnum(hvalue(o), n));
	api_incr_top(L);
}

void lua_newtable(lua_State* L)
{
	luaC_checkGC(L);
	sethvalue(L->top, luaH_new(L, 0, 0));
	api_incr_top(L);
}

static Table* getmt(lua_State* L, const TObject* obj)
{
	switch (ttype(obj)) {
	case LUA_TTABLE:
		return hvalue(obj)->metatable;
	case LUA_TUSERDATA:
		return uvalue(obj)->metatable;
	case LUA_TMETHOD:
		return G(L)->mt[LUA_TFUNCTION];
	default:
		return G(L)->mt[ttype(obj)];
	}
}

int lua_getmetatable(lua_State* L, int objindex)
{
	const TObject* obj = luaA_indexAcceptable(L, objindex);
	Table* mt = (obj == NULL) ? NULL : getmt(L, obj);
	if (mt == NULL)
		return 0;
	sethvalue(L->top, mt);
	api_incr_top(L);
	return 1;
}

void lua_getfenv(lua_State* L, int idx)
{
	StkId o = luaA_index(L, idx);
	if (isLfunction(o) || (ttype(o) == LUA_TMETHOD && !clvalue(o)->c.isC))
		setobj2s(L->top, &clvalue(o)->l.g)
	else
		setobj2s(L->top, gt(L));
	api_incr_top(L);
}

// ---- set functions (stack -> Lua) --------------------------------------------------------------------

void lua_settable(lua_State* L, int idx)
{
	StkId t = luaA_index(L, idx);
	luaV_settable(L, t, L->top - 2, L->top - 1);
	L->top -= 2;  // pop index and value
}

void lua_rawset(lua_State* L, int idx)
{
	StkId t = luaA_index(L, idx);
	setobj2t(luaH_set(L, hvalue(t), L->top - 2), L->top - 1);
	L->top -= 2;
}

void lua_rawseti(lua_State* L, int idx, int n)
{
	StkId o = luaA_index(L, idx);
	setobj2t(luaH_setnum(L, hvalue(o), n), L->top - 1);
	L->top--;
}

int lua_setmetatable(lua_State* L, int objindex)
{
	TObject* obj = luaA_index(L, objindex);
	Table* mt = ttisnil(L->top - 1) ? NULL : hvalue(L->top - 1);
	int res = 1;
	switch (ttype(obj)) {
	case LUA_TTABLE:
		hvalue(obj)->metatable = mt;
		break;
	case LUA_TUSERDATA:
		uvalue(obj)->metatable = mt;
		break;
	default:
		G(L)->mt[ttype(obj) == LUA_TMETHOD ? LUA_TFUNCTION : ttype(obj)] = mt;
		break;
	}
	L->top--;
	return res;
}

int lua_setfenv(lua_State* L, int idx)
{
	StkId o;
	int res = 0;
	L->top--;
	o = luaA_index(L, idx);
	if (isLfunction(o)) {
		res = 1;
		clvalue(o)->l.g = *(L->top);
	}
	return res;
}

void lua_setparent(lua_State* L, int idx)
{
	StkId t = luaA_index(L, idx);
	hvalue(t)->parent = ttistable(L->top - 1) ? hvalue(L->top - 1) : NULL;
	L->top--;
}

int lua_getparent(lua_State* L, int idx)
{
	StkId t = luaA_indexAcceptable(L, idx);
	if (t == NULL || !ttistable(t) || hvalue(t)->parent == NULL)
		return 0;
	sethvalue(L->top, hvalue(t)->parent);
	api_incr_top(L);
	return 1;
}

int lua_getprot(lua_State* L, int idx)
{
	StkId t = luaA_indexAcceptable(L, idx);
	return (t == NULL || !ttistable(t)) ? 0 : hvalue(t)->prot;
}

void lua_setprot(lua_State* L, int idx, int prot)
{
	StkId t = luaA_index(L, idx);
	if (ttistable(t))
		hvalue(t)->prot = (lu_byte)prot;
}

// ---- `load' and `call' functions (run Lua code) ------------------------------------------------------

void lua_call(lua_State* L, int nargs, int nresults)
{
	StkId func;
	func = L->top - (nargs + 1);
	luaD_call(L, func, nresults);
}

// Execute a protected call.
struct CallS {  // data to `f_call'
	StkId func;
	int nresults;
};

static void f_call(lua_State* L, void* ud)
{
	CallS* c = (CallS*)ud;
	luaD_call(L, c->func, c->nresults);
}

int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc)
{
	CallS c;
	int status;
	ptrdiff_t func;
	func = (errfunc == 0) ? 0 : savestack(L, luaA_index(L, errfunc));
	c.func = L->top - (nargs + 1);  // function to be called
	c.nresults = nresults;
	status = luaD_pcall(L, f_call, &c, savestack(L, c.func), func);
	return status;
}

// Execute a protected C call.
struct CCallS {  // data to `f_Ccall'
	lua_CFunction func;
	void* ud;
};

static void f_Ccall(lua_State* L, void* ud)
{
	CCallS* c = (CCallS*)ud;
	Closure* cl;
	cl = luaF_newCclosure(L, 0);
	cl->c.f = c->func;
	setclvalue(L->top, cl);  // push function
	incr_top(L);
	setpvalue(L->top, c->ud);  // push only argument
	incr_top(L);
	luaD_call(L, L->top - 2, 0);
}

int lua_cpcall(lua_State* L, lua_CFunction func, void* ud)
{
	CCallS c;
	int status;
	c.func = func;
	c.ud = ud;
	status = luaD_pcall(L, f_Ccall, &c, savestack(L, L->top), 0);
	return status;
}

int lua_load(lua_State* L, lua_Chunkreader reader, void* data, const char* chunkname)
{
	ZIO z;
	int status;
	int c;
	if (!chunkname)
		chunkname = "?";
	luaZ_init(&z, reader, data, chunkname);
	c = luaZ_lookahead(&z);
	status = luaD_protectedparser(L, &z, (c == LUA_SIGNATURE[0]));
	return status;
}

// ---- garbage-collection functions --------------------------------------------------------------------

// GC values are expressed in Kbytes: #bytes/2^10
#define GCscalel(x) ((x) >> 10)
#define GCscale(x) ((int)GCscalel(x))
#define GCunscale(x) ((lu_mem)(x) << 10)

int lua_getgcthreshold(lua_State* L)
{
	return GCscale(G(L)->GCthreshold);
}

int lua_getgccount(lua_State* L)
{
	return GCscale(G(L)->nblocks);
}

void lua_setgcthreshold(lua_State* L, int newthreshold)
{
	if (newthreshold < 0)
		newthreshold = 0;
	G(L)->GCthreshold = GCunscale(newthreshold);
	luaC_checkGC(L);
}

// ---- miscellaneous functions -------------------------------------------------------------------------

int lua_error(lua_State* L)
{
	luaG_errormsg(L);
}

int lua_next(lua_State* L, int idx)
{
	StkId t;
	int more;
	t = luaA_index(L, idx);
	more = luaH_next(L, hvalue(t), L->top - 1);
	if (more) {
		api_incr_top(L);
	}
	else  // no more elements
		L->top -= 1;  // remove key
	return more;
}

void lua_concat(lua_State* L, int n)
{
	if (n >= 2) {
		luaC_checkGC(L);
		luaV_concat(L, n, (int)(L->top - L->base) - 1);
		L->top -= (n - 1);
	}
	else if (n == 0) {  // push empty string
		setsvalue2s(L->top, luaS_newlstr(L, NULL, 0));
		api_incr_top(L);
	}
	// else n == 1; nothing to do
}

void* lua_newuserdata(lua_State* L, size_t size)
{
	Udata* u;
	luaC_checkGC(L);
	u = luaS_newudata(L, size);
	setuvalue(L->top, u);
	api_incr_top(L);
	return udata_block(u);
}

// PopLua (inline in LuaSupport's PushFullObject, 0x6d2830): push the full userdata that owns `block' (a block
// lua_newuserdata returned, still alive)
void lua_pushudata(lua_State* L, void* block)
{
	Udata* u = (Udata*)((char*)block - offsetof(Udata, align_));
	setuvalue(L->top, u);
	api_incr_top(L);
}

int lua_pushupvalues(lua_State* L)
{
	Closure* func;
	int n, i;
	func = clvalue(L->base - 1);
	n = func->c.nupvalues;
	luaD_checkstack(L, n + LUA_MINSTACK);
	for (i = 0; i < n; i++) {
		setobj2s(L->top, &func->c.upvalue[i]);
		L->top++;
	}
	return n;
}

static const char* aux_upvalue(lua_State* L, int funcindex, int n, TObject** val)
{
	Closure* f;
	StkId fi = luaA_index(L, funcindex);
	if (!ttisfunction(fi) && ttype(fi) != LUA_TMETHOD)
		return NULL;
	f = clvalue(fi);
	if (f->c.isC) {
		if (n > f->c.nupvalues)
			return NULL;
		*val = &f->c.upvalue[n - 1];
		return "";
	}
	else {
		Proto* p = f->l.p;
		if (n > p->sizeupvalues)
			return NULL;
		*val = f->l.upvals[n - 1]->v;
		return getstr(p->upvalues[n - 1]);
	}
}

const char* lua_getupvalue(lua_State* L, int funcindex, int n)
{
	const char* name;
	TObject* val;
	name = aux_upvalue(L, funcindex, n, &val);
	if (name) {
		setobj2s(L->top, val);
		api_incr_top(L);
	}
	return name;
}

const char* lua_setupvalue(lua_State* L, int funcindex, int n)
{
	const char* name;
	TObject* val;
	name = aux_upvalue(L, funcindex, n, &val);
	if (name) {
		L->top--;
		setobj(val, L->top);
	}
	return name;
}
