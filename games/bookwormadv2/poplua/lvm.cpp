// The virtual machine (luaV_execute 0x717540 and its helpers 0x715f10-0x7173b0): Lua 5.0.2's lvm.c with
// PopLua's int type, its 0-based constructors, the class parent chain, defined keys and the twelve new
// opcodes (docs/POPLUA.md). Each handler was read from the disassembly; the comments name what differs from
// 5.0.2. The original's per-instruction lookup cache changes no result and is left out.
#include <cmath>
#include <cstdio>

#include "lcore.h"
#include "lopcodes.h"

#define MAXTAGLOOP 100

// 0x765380 (_ftol2_sse): truncation; NaN and values out of the int range give INT_MIN
static lua_Int ftol(lua_Number d)
{
	if (!(d > -2147483649.0 && d < 2147483648.0))
		return INT32_MIN;
	return (lua_Int)d;
}

lua_Int luaV_ftol(lua_Number d)
{
	return ftol(d);
}

// "%.14g" as the original's CRT (Visual C++ 2005) prints it: three exponent digits and its names for the
// special values
static void fmtnumber(char* s, size_t size, lua_Number n)
{
	if (std::isnan(n))
		snprintf(s, size, "%s", std::signbit(n) ? "-1.#IND" : "1.#QNAN");
	else if (std::isinf(n))
		snprintf(s, size, "%s", n < 0 ? "-1.#INF" : "1.#INF");
	else {
		snprintf(s, size, "%.14g", n);
		char* e = strchr(s, 'e');
		if (e != NULL && (e[1] == '+' || e[1] == '-') && strlen(e + 2) == 2) {  // pad the exponent
			memmove(e + 3, e + 2, 3);
			e[2] = '0';
		}
	}
}

// 0x715f10: a number, an int (converted) or a numeric string
const TObject* luaV_tonumber(const TObject* obj, TObject* n)
{
	lua_Number num;
	if (ttisnumber(obj))
		return obj;
	if (ttisint(obj)) {
		setnvalue(n, (lua_Number)ivalue(obj));
		return n;
	}
	if (ttisstring(obj) && luaO_str2d(svalue(obj), &num)) {
		setnvalue(n, num);
		return n;
	}
	return NULL;
}

// 0x715f70: an int, a number (truncated) or a decimal int string
const TObject* luaV_toint(const TObject* obj, TObject* n)
{
	lua_Int i;
	if (ttisint(obj))
		return obj;
	if (ttisnumber(obj)) {
		setivalue(n, ftol(nvalue(obj)));
		return n;
	}
	if (ttisstring(obj) && luaO_str2int(svalue(obj), &i)) {
		setivalue(n, i);
		return n;
	}
	return NULL;
}

// 0x715fd0: an int prints with %d; a number with %.14g, and ".0" is added when that has no '.', so that a
// number never looks like an int
int luaV_tostring(lua_State* L, StkId obj)
{
	char s[64];
	if (ttisint(obj))
		snprintf(s, sizeof(s), "%d", (int)ivalue(obj));
	else if (ttisnumber(obj)) {
		fmtnumber(s, sizeof(s) - 2, nvalue(obj));
		if (strchr(s, '.') == NULL)
			strcat(s, ".0");
	}
	else
		return 0;
	setsvalue2s(obj, luaS_new(L, s));
	return 1;
}

// 0x7160d0
static void callTMres(lua_State* L, const TObject* f, const TObject* p1, const TObject* p2)
{
	setobj2s(L->top, f);  // push function
	setobj2s(L->top + 1, p1);  // 1st argument
	setobj2s(L->top + 2, p2);  // 2nd argument
	luaD_checkstack(L, 3);  // cannot check before (could invalidate p1, p2)
	L->top += 3;
	luaD_call(L, L->top - 3, 1);
	L->top--;  // result will be in L->top
}

// 0x716160
static void callTM(lua_State* L, const TObject* f, const TObject* p1, const TObject* p2, const TObject* p3)
{
	setobj2s(L->top, f);  // push function
	setobj2s(L->top + 1, p1);  // 1st argument
	setobj2s(L->top + 2, p2);  // 2nd argument
	setobj2s(L->top + 3, p3);  // 3th argument
	luaD_checkstack(L, 4);  // cannot check before (could invalidate p1...p3)
	L->top += 4;
	luaD_call(L, L->top - 4, 0);
}

// ---- gettable ----------------------------------------------------------------------------------------
// A lookup finds a key when its value is not nil or when it was defined (NODE_DEFINED): a defined key holds
// a nil that hides the parent chain and __index.

#define found(v, flags) (!ttisnil(v) || ((flags) & NODE_DEFINED))

static const TObject* gettable_f(lua_State* L, const TObject* t, TObject* key, int loop, int* flags);

// 0x716280 / 0x716200: a string key in the class chain, by raw lookups (no metatables)
static const TObject* getparent(lua_State* L, Table* p, TString* key, int loop, int* flags)
{
	for (; p != NULL; p = p->parent, loop++) {
		const TObject* v;
		if (loop > MAXTAGLOOP)
			luaG_runerror(L, "loop in gettable");
		v = luaH_getstr_f(p, key, flags);
		if (found(v, *flags))
			return v;
	}
	return &luaO_nilobject;
}

// 0x7162d0: the table t has no `key'. A string key goes to the parent chain when t has a parent (and then
// never to __index); anything else to __index.
static const TObject* indexmiss(lua_State* L, const TObject* t, TObject* key, int loop, int* flags)
{
	Table* h = hvalue(t);
	const TObject* tm;
	if (ttisstring(key) && h->parent != NULL)
		return getparent(L, h->parent, tsvalue(key), loop, flags);
	tm = fasttm(L, h->metatable, TM_INDEX);
	if (tm == NULL)
		return &luaO_nilobject;
	if (ttisfunction(tm)) {
		callTMres(L, tm, t, key);
		*flags = 0;
		return L->top;
	}
	return gettable_f(L, tm, key, loop, flags);
}

// 0x7163a0: t is not a table
static const TObject* indexother(lua_State* L, const TObject* t, TObject* key, int loop, int* flags)
{
	const TObject* tm = luaT_gettmbyobj(L, t, TM_INDEX);
	if (ttisnil(tm))
		luaG_typeerror(L, t, "index");
	if (ttisfunction(tm)) {
		callTMres(L, tm, t, key);
		*flags = 0;
		return L->top;
	}
	return gettable_f(L, tm, key, loop, flags);
}

// 0x716420
static const TObject* gettable_f(lua_State* L, const TObject* t, TObject* key, int loop, int* flags)
{
	if (loop > MAXTAGLOOP)
		luaG_runerror(L, "loop in gettable");
	if (ttistable(t)) {
		const TObject* v = luaH_get_f(hvalue(t), key, flags);
		if (found(v, *flags))
			return v;
		return indexmiss(L, t, key, loop + 1, flags);
	}
	return indexother(L, t, key, loop + 1, flags);
}

const TObject* luaV_gettable_f(lua_State* L, const TObject* t, TObject* key, int loop, int* flags)
{
	*flags = 0;
	return gettable_f(L, t, key, loop, flags);
}

const TObject* luaV_gettable(lua_State* L, const TObject* t, TObject* key, int loop)
{
	int flags = 0;
	return gettable_f(L, t, key, loop, &flags);
}

// ---- settable ----------------------------------------------------------------------------------------

// 0x7037e0: the slot of `key' in t; a missing key is created unless mode has bit 1 (it then returns
// luaO_nilobject). An existing string key reports its flags.
static TObject* setslot(lua_State* L, Table* t, const TObject* key, int* flags, int mode)
{
	const TObject* p = luaH_get_f(t, key, flags);
	t->flags = 0;
	if (p != &luaO_nilobject)
		return (TObject*)p;
	if (mode & 2)
		return (TObject*)&luaO_nilobject;
	return luaH_set_f(L, t, key, *flags);
}

// 0x7166e0 (SETTABLE, SETGLOBAL and the API)
void luaV_settable(lua_State* L, const TObject* t, TObject* key, StkId val)
{
	const TObject* tm;
	int loop = 0;
	for (;;) {
		if (ttistable(t)) {  // `t' is a table?
			Table* h = hvalue(t);
			int flags = 0;
			TObject* oldval = setslot(L, h, key, &flags, 0);  // do a primitive set
			if (!ttisnil(oldval) || (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) {
				setobj2t(oldval, val);
				return;
			}
			// else will try the tag method
		}
		else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_NEWINDEX)))
			luaG_typeerror(L, t, "index");
		if (ttisfunction(tm)) {
			callTM(L, tm, t, key, val);
			return;
		}
		t = tm;  // else repeat with `tm'
		if (++loop > MAXTAGLOOP)
			luaG_runerror(L, "loop in settable");
	}
}

// 0x716920 (SETSELFORGLOBAL): an existing key of self (in self itself, not its parents) is set there; any
// other key goes to the globals
static void setselforglobal(lua_State* L, const TObject* self, const TObject* gt, TObject* key, StkId val)
{
	const TObject* t = self;
	const TObject* tm;
	int mode = (ttistable(self) ? hvalue(self)->prot : 0) | 2;  // self never gets a new key
	int switched = 0;
	int loop = 0;
	for (;;) {
		if (ttistable(t)) {
			Table* h = hvalue(t);
			int flags = 0;
			TObject* slot = setslot(L, h, key, &flags, mode);
			if (slot == &luaO_nilobject && !switched) {  // not a field of self: a global
				t = gt;
				mode = ttistable(gt) ? hvalue(gt)->prot : 0;
				switched = 1;
				if (++loop > MAXTAGLOOP)
					luaG_runerror(L, "loop in settable");
				continue;
			}
			if (!ttisnil(slot) || (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) {
				if (slot != &luaO_nilobject)  // (the original would write into luaO_nilobject)
					setobj2t(slot, val);
				return;
			}
		}
		else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_NEWINDEX)))
			luaG_typeerror(L, t, "index");
		if (ttisfunction(tm)) {
			callTM(L, tm, t, key, val);
			return;
		}
		t = tm;
		if (++loop > MAXTAGLOOP)
			luaG_runerror(L, "loop in settable");
	}
}

// 0x716b90 (DEFGLOBAL, DEFTABLE, CONSTGLOBAL, CONSTTABLE): a new key gets `flags' (an existing one keeps
// its own)
void luaV_defset(lua_State* L, const TObject* t, TObject* key, const TObject* val, int flags)
{
	const TObject* tm;
	int loop = 0;
	for (;;) {
		if (ttistable(t)) {
			Table* h = hvalue(t);
			TObject* slot = luaH_set_f(L, h, key, flags);
			if (!ttisnil(slot) || (tm = fasttm(L, h->metatable, TM_NEWINDEX)) == NULL) {
				setobj2t(slot, val);
				return;
			}
		}
		else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_NEWINDEX)))
			luaG_typeerror(L, t, "index");
		if (ttisfunction(tm)) {
			callTM(L, tm, t, key, val);
			return;
		}
		t = tm;
		if (++loop > MAXTAGLOOP)
			luaG_runerror(L, "loop in settable");
	}
}

// ---- arithmetic and comparisons ----------------------------------------------------------------------

// 0x716d20
static int call_binTM(lua_State* L, const TObject* p1, const TObject* p2, StkId res, TMS event)
{
	ptrdiff_t result = savestack(L, res);
	const TObject* tm = luaT_gettmbyobj(L, p1, event);  // try first operand
	if (ttisnil(tm))
		tm = luaT_gettmbyobj(L, p2, event);  // try second operand
	if (!ttisfunction(tm))
		return 0;
	callTMres(L, tm, p1, p2);
	res = restorestack(L, result);  // previous call may change stack
	setobjs2s(res, L->top);
	return 1;
}

// 0x716da0
static const TObject* get_compTM(lua_State* L, Table* mt1, Table* mt2, TMS event)
{
	const TObject* tm1 = fasttm(L, mt1, event);
	const TObject* tm2;
	if (tm1 == NULL)
		return NULL;  // no metamethod
	if (mt1 == mt2)
		return tm1;  // same metatables => same metamethods
	tm2 = fasttm(L, mt2, event);
	if (tm2 == NULL)
		return NULL;  // no metamethod
	if (luaO_rawequalObj(tm1, tm2))  // same metamethods?
		return tm1;
	return NULL;
}

// 0x716e20
static int call_orderTM(lua_State* L, const TObject* p1, const TObject* p2, TMS event)
{
	const TObject* tm1 = luaT_gettmbyobj(L, p1, event);
	const TObject* tm2;
	if (ttisnil(tm1))
		return -1;  // no metamethod?
	tm2 = luaT_gettmbyobj(L, p2, event);
	if (!luaO_rawequalObj(tm1, tm2))  // different metamethods?
		return -1;
	callTMres(L, tm1, p1, p2);
	return !l_isfalse(L->top);
}

// 0x716e90
static int luaV_strcmp(const TString* ls, const TString* rs)
{
	const char* l = getstr(ls);
	size_t ll = ls->len;
	const char* r = getstr(rs);
	size_t lr = rs->len;
	for (;;) {
		int temp = strcoll(l, r);
		if (temp != 0)
			return temp;
		else {  // strings are equal up to a `\0'
			size_t len = strlen(l);  // index of first `\0' in both strings
			if (len == lr)  // r is finished?
				return (len == ll) ? 0 : 1;
			else if (len == ll)  // l is finished?
				return -1;  // l is smaller than r (because r is not finished)
			// both strings longer than `len'; go on comparing (after the `\0')
			len++;
			l += len;
			ll -= len;
			r += len;
			lr -= len;
		}
	}
}

// 0x716f00: ints and numbers compare with each other
int luaV_lessthan(lua_State* L, const TObject* l, const TObject* r)
{
	int res;
	if (ttisnumber(l)) {
		if (ttisnumber(r))
			return nvalue(l) < nvalue(r);
		if (ttisint(r))
			return nvalue(l) < (lua_Number)ivalue(r);
	}
	else if (ttisint(l)) {
		if (ttisint(r))
			return ivalue(l) < ivalue(r);
		if (ttisnumber(r))
			return (lua_Number)ivalue(l) < nvalue(r);
	}
	else if (ttype(l) == ttype(r)) {
		if (ttisstring(l))
			return luaV_strcmp(tsvalue(l), tsvalue(r)) < 0;
		else if ((res = call_orderTM(L, l, r, TM_LT)) != -1)
			return res;
	}
	luaG_ordererror(L, l, r);
}

// 0x716fb0
static int luaV_lessequal(lua_State* L, const TObject* l, const TObject* r)
{
	int res;
	if (ttisnumber(l)) {
		if (ttisnumber(r))
			return nvalue(l) <= nvalue(r);
		if (ttisint(r))
			return nvalue(l) <= (lua_Number)ivalue(r);
	}
	else if (ttisint(l)) {
		if (ttisint(r))
			return ivalue(l) <= ivalue(r);
		if (ttisnumber(r))
			return (lua_Number)ivalue(l) <= nvalue(r);
	}
	else if (ttype(l) == ttype(r)) {
		if (ttisstring(l))
			return luaV_strcmp(tsvalue(l), tsvalue(r)) <= 0;
		else if ((res = call_orderTM(L, l, r, TM_LE)) != -1)  // first try `le'
			return res;
		else if ((res = call_orderTM(L, r, l, TM_LT)) != -1)  // else try `lt'
			return !res;
	}
	luaG_ordererror(L, l, r);
}

// 0x717090: equal tags, or an int and a number
int luaV_equalval(lua_State* L, const TObject* t1, const TObject* t2)
{
	const TObject* tm;
	switch (ttype(t1)) {
	case LUA_TNIL:
		return 1;
	case LUA_TNUMBER:
		if (ttisnumber(t2))
			return nvalue(t1) == nvalue(t2);
		return nvalue(t1) == (lua_Number)ivalue(t2);
	case LUA_TINT:
		if (ttisint(t2))
			return ivalue(t1) == ivalue(t2);
		return (lua_Number)ivalue(t1) == nvalue(t2);
	case LUA_TBOOLEAN:
		return bvalue(t1) == bvalue(t2);
	case LUA_TLIGHTUSERDATA:
		return pvalue(t1) == pvalue(t2);
	case LUA_TUSERDATA:
		if (uvalue(t1) == uvalue(t2))
			return 1;
		tm = get_compTM(L, uvalue(t1)->metatable, uvalue(t2)->metatable, TM_EQ);
		break;  // will try TM
	case LUA_TTABLE:
		if (hvalue(t1) == hvalue(t2))
			return 1;
		tm = get_compTM(L, hvalue(t1)->metatable, hvalue(t2)->metatable, TM_EQ);
		break;  // will try TM
	default:
		return gcvalue(t1) == gcvalue(t2);
	}
	if (tm == NULL)
		return 0;  // no TM?
	callTMres(L, tm, t1, t2);  // call TM
	return !l_isfalse(L->top);
}

// 0x7171a0
void luaV_concat(lua_State* L, int total, int last)
{
	do {
		StkId top = L->base + last + 1;
		int n = 2;  // number of elements handled in this pass (at least 2)
		if (!tostring(L, top - 2) || !tostring(L, top - 1)) {
			if (!call_binTM(L, top - 2, top - 1, top - 2, TM_CONCAT))
				luaG_concaterror(L, top - 2, top - 1);
		}
		else if (tsvalue(top - 1)->len > 0) {  // if len=0, do nothing
			// at least two string values; get as many as possible
			lu_mem tl = (lu_mem)tsvalue(top - 1)->len + (lu_mem)tsvalue(top - 2)->len;
			char* buffer;
			int i;
			while (n < total && tostring(L, top - n - 1)) {  // collect total length
				size_t l = tsvalue(top - n - 1)->len;
				if (l >= MAX_INT - tl)
					luaG_runerror(L, "string length overflow");
				tl += l;
				n++;
			}
			if (tl > G(L)->buffsize) {  // luaZ_openspace
				size_t newsize = tl < 32 ? 32 : tl;
				G(L)->buff = (char*)luaM_realloc(L, G(L)->buff, G(L)->buffsize, newsize);
				G(L)->buffsize = newsize;
			}
			buffer = G(L)->buff;
			tl = 0;
			for (i = n; i > 0; i--) {  // concat all strings
				size_t l = tsvalue(top - i)->len;
				memcpy(buffer + tl, svalue(top - i), l);
				tl += l;
			}
			setsvalue2s(top - n, luaS_newlstr(L, buffer, tl));
		}
		total -= n - 1;  // got `n' strings to create 1 new
		last -= n - 1;
	} while (total > 1);  // repeat until only 1 result left
}

// 0x7173b0: the arithmetic of anything but number op number and int op int: both operands are converted
// to numbers (the result is a number) or the metamethod is called
static void Arith(lua_State* L, StkId ra, const TObject* rb, const TObject* rc, TMS op)
{
	TObject tempb, tempc;
	const TObject *b, *c;
	if ((b = luaV_tonumber(rb, &tempb)) != NULL && (c = luaV_tonumber(rc, &tempc)) != NULL) {
		switch (op) {
		case TM_ADD:
			setnvalue(ra, nvalue(b) + nvalue(c));
			break;
		case TM_SUB:
			setnvalue(ra, nvalue(b) - nvalue(c));
			break;
		case TM_MUL:
			setnvalue(ra, nvalue(b) * nvalue(c));
			break;
		case TM_DIV:
			setnvalue(ra, nvalue(b) / nvalue(c));
			break;
		case TM_POW: {
			const TObject* f = luaH_getstr(hvalue(gt(L)), G(L)->tmname[TM_POW]);
			ptrdiff_t res = savestack(L, ra);
			if (!ttisfunction(f))
				luaG_runerror(L, "`__pow' (`^' operator) is not a function");
			callTMres(L, f, b, c);
			ra = restorestack(L, res);  // previous call may change stack
			setobjs2s(ra, L->top);
			break;
		}
		case TM_MOD:
			setnvalue(ra, fmod(nvalue(b), nvalue(c)));
			break;
		default:
			break;
		}
	}
	else if (!call_binTM(L, rb, rc, ra, op))
		luaG_aritherror(L, rb, rc);
}

// ---- the interpreter ---------------------------------------------------------------------------------

#define RA(i) (base + GETARG_A(i))
#define RB(i) (base + GETARG_B(i))
#define RC(i) (base + GETARG_C(i))
#define RKB(i) ((GETARG_B(i) < MAXSTACK) ? RB(i) : k + GETARG_B(i) - MAXSTACK)
#define RKC(i) ((GETARG_C(i) < MAXSTACK) ? RC(i) : k + GETARG_C(i) - MAXSTACK)
#define KBx(i) (k + GETARG_Bx(i))

#define dojump(pc, i) ((pc) += (i))

// a number, an int or a numeric string, converted to a number in place (FORLOOP's limit and step)
static int tonumber_inplace(StkId o)
{
	lua_Number n;
	if (ttisnumber(o))
		return 1;
	if (ttisint(o)) {
		setnvalue(o, (lua_Number)ivalue(o));
		return 1;
	}
	if (ttisstring(o) && luaO_str2d(svalue(o), &n)) {
		setnvalue(o, n);
		return 1;
	}
	return 0;
}

// the handlers of SELF, ALTSELF and SELFORGLOBAL check that the key is a string (the original returns from
// luaV_execute without a word otherwise; no compiled code does that)
#define checkkey(L, k) { if (!ttisstring(k)) luaG_runerror(L, "method name is not a string"); }

StkId luaV_execute(lua_State* L)
{
	LClosure* cl;
	TObject* k;
	const Instruction* pc;
	StkId lastmove = NULL;  // the register the last MOVE wrote (ALTSELF reads it)
callentry:  // entry point when calling new functions
retentry:   // entry point when returning to old functions
	L->ci->pc = &pc;
	L->ci->state = CI_HASFRAME;
	pc = L->ci->savedpc;
	cl = &clvalue(L->base - 1)->l;
	k = cl->p->k;
	// main loop of interpreter
	for (;;) {
		const Instruction i = *pc++;
		StkId base = L->base;
		StkId ra = RA(i);
		switch (GET_OPCODE(i)) {
		case OP_MOVE:
			setobjs2s(ra, RB(i));
			lastmove = ra;
			break;
		case OP_LOADK:
			setobj2s(ra, KBx(i));
			break;
		case OP_LOADBOOL:
			setbvalue(ra, GETARG_B(i));
			if (GETARG_C(i))
				pc++;  // skip next instruction (if C)
			break;
		case OP_LOADNIL: {
			TObject* rb = RB(i);
			do {
				setnilvalue(rb--);
			} while (rb >= ra);
			break;
		}
		case OP_GETUPVAL: {
			int b = GETARG_B(i);
			setobj2s(ra, cl->upvals[b]->v);
			break;
		}
		case OP_GETGLOBAL: {
			TObject* rb = KBx(i);
			int flags = 0;
			const TObject* v = luaH_getstr_f(hvalue(&cl->g), tsvalue(rb), &flags);
			if (!found(v, flags))
				v = indexmiss(L, &cl->g, rb, 0, &flags);
			setobj2s(L->base + GETARG_A(i), v);
			break;
		}
		case OP_GETTABLE: {
			StkId rb = RB(i);
			TObject* rc = RKC(i);
			int flags = 0;
			const TObject* v;
			if (ttistable(rb)) {
				v = luaH_get_f(hvalue(rb), rc, &flags);
				if (!found(v, flags))
					v = indexmiss(L, rb, rc, 0, &flags);
			}
			else
				v = indexother(L, rb, rc, 0, &flags);
			setobj2s(L->base + GETARG_A(i), v);
			break;
		}
		case OP_GETSELFORGLOBAL: {  // R(A) := self[K] when self has it (or its class), else the global
			TObject* rb = KBx(i);
			int flags = 0;
			const TObject* v;
			if (ttistable(base)) {
				v = luaH_get_f(hvalue(base), rb, &flags);
				if (!found(v, flags))
					v = indexmiss(L, base, rb, 1, &flags);
			}
			else
				v = indexother(L, base, rb, 1, &flags);
			if (!found(v, flags)) {
				flags = 0;
				v = luaH_getstr_f(hvalue(&cl->g), tsvalue(rb), &flags);
				if (!found(v, flags))
					v = indexmiss(L, &cl->g, rb, 0, &flags);
			}
			setobj2s(L->base + GETARG_A(i), v);
			break;
		}
		case OP_SETGLOBAL:
			luaV_settable(L, &cl->g, KBx(i), ra);
			break;
		case OP_SETSELFORGLOBAL:
			setselforglobal(L, base, &cl->g, KBx(i), ra);
			break;
		case OP_SETUPVAL: {
			UpVal* uv = cl->upvals[GETARG_B(i)];
			setobj(uv->v, ra);
			break;
		}
		case OP_SETTABLE:
			luaV_settable(L, ra, RKB(i), RKC(i));
			break;
		case OP_NEWTABLE: {  // C is the number of hash nodes (luaH_new takes it as a size)
			int b = GETARG_B(i);
			b = fb2int(b);
			sethvalue(ra, luaH_new(L, b, GETARG_C(i)));
			luaC_checkGC(L);
			break;
		}
		case OP_SELF:      // R(A+1) := R(B); R(A) := R(B)[RK(C)], a function from a table tagged as a method
		case OP_ALTSELF: {  // the same with R(A+1) := the register the last MOVE wrote
			StkId rb = RB(i);
			TObject* rc = RKC(i);
			int flags = 0;
			const TObject* v;
			checkkey(L, rc);
			if (GET_OPCODE(i) == OP_SELF) {
				setobjs2s(ra + 1, rb);
			}
			else if (lastmove != NULL) {
				setobjs2s(ra + 1, lastmove);
			}
			else
				setnilvalue(ra + 1);
			if (ttistable(rb)) {
				v = luaH_getstr(hvalue(rb), tsvalue(rc));
				if (ttisnil(v))
					v = indexmiss(L, rb, rc, 0, &flags);
				ra = L->base + GETARG_A(i);
				setobj2s(ra, v);
				if (ttisfunction(ra))
					setttype(ra, LUA_TMETHOD);
			}
			else {
				v = indexother(L, rb, rc, 0, &flags);
				setobj2s(L->base + GETARG_A(i), v);
			}
			break;
		}
		case OP_SELFORGLOBAL: {  // R(A+1) := R(B); R(A) := self[RK(C)] (a method if it is one) or the global
			StkId rb = RB(i);
			TObject* rc = RKC(i);
			const TObject* v = &luaO_nilobject;
			checkkey(L, rc);
			setobjs2s(ra + 1, rb);
			if (ttistable(base)) {
				v = luaH_getstr(hvalue(base), tsvalue(rc));
				if (v == &luaO_nilobject) {
					int flags = 0;
					v = indexmiss(L, base, rc, 0, &flags);
				}
			}
			if (v == &luaO_nilobject) {  // not a field of self: the global, by a raw lookup
				v = luaH_getstr(hvalue(&cl->g), tsvalue(rc));
				setobj2s(L->base + GETARG_A(i), v);
			}
			else {
				ra = L->base + GETARG_A(i);
				setobj2s(ra, v);
				if (ttisfunction(ra) && !clvalue(ra)->c.isC && clvalue(ra)->l.p->selfflag)
					setttype(ra, LUA_TMETHOD);
			}
			break;
		}
		case OP_ADD: {
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (ttisnumber(rb) && ttisnumber(rc)) {
				setnvalue(ra, nvalue(rb) + nvalue(rc));
			}
			else if (ttisint(rb) && ttisint(rc)) {
				setivalue(ra, (lua_Int)((uint32_t)ivalue(rb) + (uint32_t)ivalue(rc)));
			}
			else
				Arith(L, ra, rb, rc, TM_ADD);
			break;
		}
		case OP_SUB: {
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (ttisnumber(rb) && ttisnumber(rc)) {
				setnvalue(ra, nvalue(rb) - nvalue(rc));
			}
			else if (ttisint(rb) && ttisint(rc)) {
				setivalue(ra, (lua_Int)((uint32_t)ivalue(rb) - (uint32_t)ivalue(rc)));
			}
			else
				Arith(L, ra, rb, rc, TM_SUB);
			break;
		}
		case OP_MUL: {
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (ttisnumber(rb) && ttisnumber(rc)) {
				setnvalue(ra, nvalue(rb) * nvalue(rc));
			}
			else if (ttisint(rb) && ttisint(rc)) {
				setivalue(ra, (lua_Int)((uint32_t)ivalue(rb) * (uint32_t)ivalue(rc)));
			}
			else
				Arith(L, ra, rb, rc, TM_MUL);
			break;
		}
		case OP_DIV: {  // int / int is C's (truncating) division
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (ttisnumber(rb) && ttisnumber(rc)) {
				setnvalue(ra, nvalue(rb) / nvalue(rc));
			}
			else if (ttisint(rb) && ttisint(rc)) {
				lua_Int d = ivalue(rc);
				lua_Int n = ivalue(rb);
				if (d == 0)
					luaG_runerror(L, "attempt to divide by zero");
				setivalue(ra, (d == -1) ? (lua_Int)(0u - (uint32_t)n) : n / d);
			}
			else
				Arith(L, ra, rb, rc, TM_DIV);
			break;
		}
		case OP_POW: {  // int ^ int is an int
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (ttisint(rb) && ttisint(rc)) {
				setivalue(ra, ftol(pow((lua_Number)ivalue(rb), (lua_Number)ivalue(rc))));
			}
			else
				Arith(L, ra, rb, rc, TM_POW);
			break;
		}
		case OP_MOD: {  // only int % int (C's remainder)
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (ttisint(rb) && ttisint(rc)) {
				lua_Int d = ivalue(rc);
				if (d == 0)
					luaG_runerror(L, "attempt to mod by zero");
				setivalue(ra, (d == -1) ? 0 : ivalue(rb) % d);
			}
			else
				luaG_runerror(L, "attempt to perform modulo on non-integers");
			break;
		}
		case OP_UNM: {
			const TObject* rb = RB(i);
			TObject temp;
			const TObject* n;
			if (ttisint(rb)) {
				setivalue(ra, (lua_Int)(0u - (uint32_t)ivalue(rb)));
			}
			else if ((n = luaV_tonumber(rb, &temp)) != NULL) {
				setnvalue(ra, -nvalue(n));
			}
			else {
				setnilvalue(&temp);
				if (!call_binTM(L, RB(i), &temp, ra, TM_UNM))
					luaG_aritherror(L, RB(i), &temp);
			}
			break;
		}
		case OP_INT: {  // R(A) := int(R(B)), truncating
			const TObject* rb = RB(i);
			TObject temp;
			const TObject* n;
			if (ttisint(rb)) {
				setivalue(ra, ivalue(rb));
			}
			else if ((n = luaV_tonumber(rb, &temp)) != NULL) {
				setivalue(ra, ftol(nvalue(n)));
			}
			else
				luaG_typeerror(L, rb, "perform arithmetic on");
			break;
		}
		case OP_NOT: {
			int res = l_isfalse(RB(i));  // next assignment may change this value
			setbvalue(ra, res);
			break;
		}
		case OP_SIZ: {  // R(A) := *R(B): a table's size (an int), else the __siz metamethod
			StkId rb = RB(i);
			if (ttistable(rb)) {
				setivalue(ra, luaH_getn(hvalue(rb)));
			}
			else if (!call_binTM(L, rb, &luaO_nilobject, ra, TM_SIZ))
				luaG_typeerror(L, rb, "get size of");
			break;
		}
		case OP_CONCAT: {
			int b = GETARG_B(i);
			int c = GETARG_C(i);
			luaV_concat(L, c - b + 1, c);  // may change `base' (and `ra')
			base = L->base;
			setobjs2s(RA(i), base + b);
			luaC_checkGC(L);
			break;
		}
		case OP_JMP:
			dojump(pc, GETARG_sBx(i));
			break;
		case OP_EQ: {  // an int equals a number of the same value
			TObject* rb = RKB(i);
			TObject* rc = RKC(i);
			if (equalobj(L, rb, rc) != GETARG_A(i))
				pc++;
			else
				dojump(pc, GETARG_sBx(*pc) + 1);
			break;
		}
		case OP_LT:
			if (luaV_lessthan(L, RKB(i), RKC(i)) != GETARG_A(i))
				pc++;
			else
				dojump(pc, GETARG_sBx(*pc) + 1);
			break;
		case OP_LE:
			if (luaV_lessequal(L, RKB(i), RKC(i)) != GETARG_A(i))
				pc++;
			else
				dojump(pc, GETARG_sBx(*pc) + 1);
			break;
		case OP_TEST: {
			TObject* rb = RB(i);
			if (l_isfalse(rb) == GETARG_C(i))
				pc++;
			else {
				setobjs2s(ra, rb);
				dojump(pc, GETARG_sBx(*pc) + 1);
			}
			break;
		}
		case OP_CALL:
		case OP_TAILCALL:
		case OP_CALLSELFORGLOBAL:
		case OP_TAILCALLSELFORGLOBAL: {
			// a callee tagged as a method gets the self argument R(A+1); a plain function called by
			// CALLSELFORGLOBAL/TAILCALLSELFORGLOBAL does not, so the arguments move down by one
			StkId firstResult;
			int b = GETARG_B(i);
			int nresults = GETARG_C(i) - 1;
			OpCode op = GET_OPCODE(i);
			if (b != 0)
				L->top = ra + b;  // else previous instruction set top
			if (ttype(ra) == LUA_TMETHOD)
				setttype(ra, LUA_TFUNCTION);
			else if (op == OP_CALLSELFORGLOBAL || op == OP_TAILCALLSELFORGLOBAL) {
				StkId p;
				L->top--;
				for (p = ra + 1; p < L->top; p++)
					setobjs2s(p, p + 1);
			}
			firstResult = luaD_precall(L, ra);
			if (firstResult) {
				if (firstResult > L->top) {  // yield?
					(L->ci - 1)->savedpc = pc;
					(L->ci - 1)->state = CI_SAVEDPC;
					return NULL;
				}
				// it was a C function (`precall' called it); adjust results
				luaD_poscall(L, nresults, firstResult);
				if (nresults >= 0)
					L->top = L->ci->top;
			}
			else {  // it is a Lua function
				if (op == OP_CALL || op == OP_CALLSELFORGLOBAL) {  // regular call?
					(L->ci - 1)->savedpc = pc;  // save `pc' to return later
					(L->ci - 1)->state = (CI_SAVEDPC | CI_CALLING);
					goto callentry;  // restart luaV_execute over new Lua function
				}
				else {  // tail call: put new frame in place of previous one
					int aux;
					base = (L->ci - 1)->base;  // `luaD_precall' may change the stack
					ra = RA(i);
					if (L->openupval)
						luaF_close(L, base);
					for (aux = 0; ra + aux < L->top; aux++)  // move frame down
						setobjs2s(base + aux - 1, ra + aux);
					(L->ci - 1)->top = L->top = base + aux;  // correct top
					(L->ci - 1)->savedpc = L->ci->savedpc;
					(L->ci - 1)->tailcalls++;  // one more call lost
					(L->ci - 1)->state = CI_SAVEDPC;
					L->ci--;  // remove new frame
					L->base = L->ci->base;
				}
				goto callentry;
			}
			break;
		}
		case OP_RETURN: {
			CallInfo* ci = L->ci - 1;  // previous function frame
			int b = GETARG_B(i);
			if (b != 0)
				L->top = ra + b - 1;
			if (L->openupval)
				luaF_close(L, base);
			L->ci->state = CI_SAVEDPC;  // deactivate current function
			L->ci->savedpc = pc;
			// previous function was running `here'?
			if (!(ci->state & CI_CALLING))
				return ra;  // no: return
			else {  // yes: continue its execution
				int nresults = GETARG_C(*(ci->savedpc - 1)) - 1;
				luaD_poscall(L, nresults, ra);
				if (nresults >= 0)
					L->top = L->ci->top;
				goto retentry;
			}
		}
		case OP_FORLOOP: {  // all ints: the loop stays in ints
			if (ttisint(ra) && ttisint(ra + 1) && ttisint(ra + 2)) {
				lua_Int step = ivalue(ra + 2);
				lua_Int idx = (lua_Int)((uint32_t)ivalue(ra) + (uint32_t)step);
				lua_Int limit = ivalue(ra + 1);
				if (step > 0 ? idx <= limit : idx >= limit) {
					dojump(pc, GETARG_sBx(i));  // jump back
					ivalue(ra) = idx;  // update index
				}
			}
			else {
				lua_Number step, idx, limit;
				if (ttisint(ra)) {
					setnvalue(ra, (lua_Number)ivalue(ra));
				}
				else if (!ttisnumber(ra))
					luaG_runerror(L, "`for' initial value must be a number");
				if (!tonumber_inplace(ra + 1))
					luaG_runerror(L, "`for' limit must be a number");
				if (!tonumber_inplace(ra + 2))
					luaG_runerror(L, "`for' step must be a number");
				step = nvalue(ra + 2);
				idx = nvalue(ra) + step;  // increment index
				limit = nvalue(ra + 1);
				if (step > 0 ? idx <= limit : idx >= limit) {
					dojump(pc, GETARG_sBx(i));  // jump back
					chgnvalue(ra, idx);  // update index
				}
			}
			break;
		}
		case OP_TFORLOOP: {
			int nvar = GETARG_C(i) + 1;
			StkId cb = ra + nvar + 2;  // call base
			setobjs2s(cb, ra);
			setobjs2s(cb + 1, ra + 1);
			setobjs2s(cb + 2, ra + 2);
			L->top = cb + 3;  // func. + 2 args (state and index)
			luaD_call(L, cb, nvar);
			L->top = L->ci->top;
			ra = L->base + GETARG_A(i) + 2;  // final position of first result
			cb = ra + nvar;
			do {  // move results to proper positions
				nvar--;
				setobjs2s(ra + nvar, cb + nvar);
			} while (nvar > 0);
			if (ttisnil(ra))  // break loop?
				pc++;  // skip jump (break loop)
			else
				dojump(pc, GETARG_sBx(*pc) + 1);  // jump back
			break;
		}
		case OP_TFORPREP: {  // for compatibility only
			if (ttistable(ra)) {
				setobjs2s(ra + 1, ra);
				setobj2s(ra, luaH_getstr(hvalue(gt(L)), luaS_newliteral(L, "next")));
			}
			dojump(pc, GETARG_sBx(i));
			break;
		}
		case OP_SETLIST:
		case OP_SETLISTO: {  // item n of a constructor goes to the int key n-1
			int bc;
			int n;
			Table* h;
			if (!ttistable(ra))
				luaG_runerror(L, "SETLIST on a non-table");
			h = hvalue(ra);
			bc = GETARG_Bx(i);
			if (GET_OPCODE(i) == OP_SETLIST)
				n = (bc & (LFIELDS_PER_FLUSH - 1)) + 1;
			else {
				n = (int)(L->top - ra) - 1;
				L->top = L->ci->top;
			}
			bc &= ~(LFIELDS_PER_FLUSH - 1);  // bc = bc - bc%FPF
			for (; n > 0; n--)
				setobj2t(luaH_setnum(L, h, bc + n - 1), ra + n);
			break;
		}
		case OP_CLOSE:
			luaF_close(L, ra);
			break;
		case OP_CONSTGLOBAL:
			luaV_defset(L, &cl->g, KBx(i), ra, NODE_DEFINED | NODE_CONST);
			break;
		case OP_CONSTTABLE:
			luaV_defset(L, ra, RKB(i), RKC(i), NODE_DEFINED | NODE_CONST);
			break;
		case OP_DEFGLOBAL:
			luaV_defset(L, &cl->g, KBx(i), ra, NODE_DEFINED);
			break;
		case OP_DEFTABLE:
			luaV_defset(L, ra, RKB(i), RKC(i), NODE_DEFINED);
			break;
		case OP_BREAK:  // a marker for the debugger
			break;
		case OP_CLOSURE: {
			Proto* p;
			Closure* ncl;
			int nup, j;
			p = cl->p->p[GETARG_Bx(i)];
			nup = p->nups;
			ncl = luaF_newLclosure(L, nup, &cl->g);
			ncl->l.p = p;
			for (j = 0; j < nup; j++, pc++) {
				if (GET_OPCODE(*pc) == OP_GETUPVAL)
					ncl->l.upvals[j] = cl->upvals[GETARG_B(*pc)];
				else
					ncl->l.upvals[j] = luaF_findupval(L, base + GETARG_B(*pc));
			}
			setclvalue(ra, ncl);
			luaC_checkGC(L);
			break;
		}
		default:
			break;
		}
	}
}
