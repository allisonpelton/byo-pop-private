// The debug interface and error messages (Lua 5.0.2's ldebug.c without hooks). Variable names in error
// messages come from a backward search for the instruction that set the register (as in Lua 5.1), which
// knows PopLua's opcodes; 5.0.2's symbolic execution and code checker are not kept.
#include "lcore.h"
#include "lopcodes.h"

#define LUA_IDSIZE 60
#define isLua(ci) (!((ci)->state & CI_C))

static int currentpc(CallInfo* ci)
{
	if (!isLua(ci))
		return -1;  // function is not a Lua function?
	if (ci->state & CI_HASFRAME)  // function has a frame?
		ci->savedpc = *ci->pc;  // use `pc' from there
	// function's pc is saved
	return pcRel(ci->savedpc, ci_func(ci)->l.p);
}

static int currentline(CallInfo* ci)
{
	int pc = currentpc(ci);
	if (pc < 0)
		return -1;  // only active lua functions have current-line information
	else
		return getfuncline(ci_func(ci)->l.p, pc);
}

int luaG_currentline(lua_State* L, CallInfo* ci)
{
	(void)L;
	return currentline(ci);
}

int lua_getstack(lua_State* L, int level, lua_Debug* ar)
{
	int status;
	CallInfo* ci;
	for (ci = L->ci; level > 0 && ci > L->base_ci; ci--) {
		level--;
		if (!(ci->state & CI_C))  // Lua function?
			level -= ci->tailcalls;  // skip lost tail calls
	}
	if (level > 0 || ci == L->base_ci)
		status = 0;  // there is no such level
	else if (level < 0) {  // level is of a lost tail call
		status = 1;
		ar->i_ci = 0;
	}
	else {
		status = 1;
		ar->i_ci = (int)(ci - L->base_ci);
	}
	return status;
}

static Proto* getluaproto(CallInfo* ci)
{
	return (isLua(ci) ? ci_func(ci)->l.p : NULL);
}

const char* lua_getlocal(lua_State* L, const lua_Debug* ar, int n)
{
	const char* name = NULL;
	CallInfo* ci = L->base_ci + ar->i_ci;
	Proto* fp = getluaproto(ci);
	if (fp) {  // is a Lua function?
		name = luaF_getlocalname(fp, n, currentpc(ci));
		if (name) {
			setobj2s(L->top, ci->base + (n - 1));  // push value
			incr_top(L);
		}
	}
	return name;
}

const char* lua_setlocal(lua_State* L, const lua_Debug* ar, int n)
{
	const char* name = NULL;
	CallInfo* ci = L->base_ci + ar->i_ci;
	Proto* fp = getluaproto(ci);
	L->top--;  // pop new value
	if (fp) {  // is a Lua function?
		name = luaF_getlocalname(fp, n, currentpc(ci));
		if (!name || name[0] == '(')  // `(' starts private locals
			name = NULL;
		else
			setobjs2s(ci->base + (n - 1), L->top);
	}
	return name;
}

static void funcinfo(lua_Debug* ar, StkId func)
{
	Closure* cl = clvalue(func);
	if (cl->c.isC) {
		ar->source = "=[C]";
		ar->linedefined = -1;
		ar->what = "C";
	}
	else {
		ar->source = getstr(cl->l.p->source);
		ar->linedefined = cl->l.p->lineDefined;
		ar->what = (ar->linedefined == 0) ? "main" : "Lua";
	}
	luaO_chunkid(ar->short_src, ar->source, LUA_IDSIZE);
}

static const char* travglobals(lua_State* L, const TObject* o)
{
	Table* g = hvalue(gt(L));
	int i = sizenode(g);
	while (i--) {
		Node* n = gnode(g, i);
		if (luaO_rawequalObj(o, gval(n)) && ttisstring(gkey(n)))
			return getstr(tsvalue(gkey(n)));
	}
	return NULL;
}

static void info_tailcall(lua_State* L, lua_Debug* ar)
{
	ar->name = ar->namewhat = "";
	ar->what = "tail";
	ar->linedefined = ar->currentline = -1;
	ar->source = "=(tail call)";
	luaO_chunkid(ar->short_src, ar->source, LUA_IDSIZE);
	ar->nups = 0;
	setnilvalue(L->top);
}

// ---- symbolic names ----------------------------------------------------------------------------------

static const char* kname(Proto* p, int c)
{
	if (ISK(c) && ttisstring(&p->k[c - MAXSTACK]))
		return svalue(&p->k[c - MAXSTACK]);
	else
		return "?";
}

// the last instruction before lastpc that set register reg, or -1
static int findsetreg(const Proto* p, int lastpc, int reg)
{
	int pc;
	int setreg = -1;  // keep last instruction that changed `reg'
	for (pc = 0; pc < lastpc; pc++) {
		Instruction i = p->code[pc];
		OpCode op = GET_OPCODE(i);
		int a = GETARG_A(i);
		switch (op) {
		case OP_LOADNIL:
			if (a <= reg && reg <= GETARG_B(i))
				setreg = pc;
			break;
		case OP_TFORLOOP:
			if (reg >= a + 2)
				setreg = pc;
			break;
		case OP_CALL:
		case OP_TAILCALL:
		case OP_CALLSELFORGLOBAL:
		case OP_TAILCALLSELFORGLOBAL:
			if (reg >= a)
				setreg = pc;
			break;
		case OP_SELF:
		case OP_ALTSELF:
		case OP_SELFORGLOBAL:
			if (reg == a || reg == a + 1)
				setreg = pc;
			break;
		case OP_JMP:
		case OP_EQ:
		case OP_LT:
		case OP_LE:
		case OP_SETGLOBAL:
		case OP_SETUPVAL:
		case OP_SETTABLE:
		case OP_SETSELFORGLOBAL:
		case OP_SETLIST:
		case OP_SETLISTO:
		case OP_CLOSE:
		case OP_RETURN:
		case OP_TFORPREP:
		case OP_CONSTGLOBAL:
		case OP_CONSTTABLE:
		case OP_DEFGLOBAL:
		case OP_DEFTABLE:
		case OP_BREAK:
			break;
		case OP_CLOSURE:
			if (reg == a)
				setreg = pc;
			pc += p->p[GETARG_Bx(i)]->nups;  // skip the pseudo-instructions
			break;
		default:
			if (reg == a)
				setreg = pc;
			break;
		}
	}
	return setreg;
}

static const char* getobjname(CallInfo* ci, int stackpos, const char** name)
{
	if (isLua(ci)) {  // a Lua function?
		Proto* p = ci_func(ci)->l.p;
		int pc = currentpc(ci);
		Instruction i;
		*name = luaF_getlocalname(p, stackpos + 1, pc);
		if (*name)  // is a local?
			return "local";
		pc = findsetreg(p, pc, stackpos);
		if (pc < 0)
			return NULL;
		i = p->code[pc];
		switch (GET_OPCODE(i)) {
		case OP_GETGLOBAL: {
			int g = GETARG_Bx(i);  // global index
			*name = svalue(&p->k[g]);
			return "global";
		}
		case OP_GETSELFORGLOBAL:
			*name = svalue(&p->k[GETARG_Bx(i)]);
			return "field or global";
		case OP_MOVE: {
			int a = GETARG_A(i);
			int b = GETARG_B(i);  // move from `b' to `a'
			if (b < a)
				return getobjname(ci, b, name);  // get name for `b'
			break;
		}
		case OP_GETTABLE:
			*name = kname(p, GETARG_C(i));
			return "field";
		case OP_GETUPVAL:
			*name = (p->upvalues && GETARG_B(i) < p->sizeupvalues) ? getstr(p->upvalues[GETARG_B(i)]) : "?";
			return "upvalue";
		case OP_SELF:
		case OP_ALTSELF:
			*name = kname(p, GETARG_C(i));
			return "method";
		case OP_SELFORGLOBAL:
			*name = kname(p, GETARG_C(i));
			return "method or global";
		default:
			break;
		}
	}
	return NULL;  // no useful name found
}

static const char* getfuncname(CallInfo* ci, const char** name)
{
	Instruction i;
	if ((isLua(ci) && ci->tailcalls > 0) || !isLua(ci - 1))
		return NULL;  // calling function is not Lua (or is unknown)
	ci--;  // calling function
	i = ci_func(ci)->l.p->code[currentpc(ci)];
	switch (GET_OPCODE(i)) {
	case OP_CALL:
	case OP_TAILCALL:
	case OP_CALLSELFORGLOBAL:
	case OP_TAILCALLSELFORGLOBAL:
		return getobjname(ci, GETARG_A(i), name);
	default:
		return NULL;  // no useful name can be found
	}
}

static int getinfo(lua_State* L, const char* what, lua_Debug* ar, StkId f, CallInfo* ci)
{
	int status = 1;
	for (; *what; what++) {
		switch (*what) {
		case 'S':
			funcinfo(ar, f);
			break;
		case 'l':
			ar->currentline = (ci) ? currentline(ci) : -1;
			break;
		case 'u':
			ar->nups = clvalue(f)->c.nupvalues;
			break;
		case 'n':
			ar->namewhat = (ci) ? getfuncname(ci, &ar->name) : NULL;
			if (ar->namewhat == NULL) {
				// try to find a global name
				if ((ar->name = travglobals(L, f)) != NULL)
					ar->namewhat = "global";
				else
					ar->namewhat = "";  // not found
			}
			break;
		case 'f':
			setobj2s(L->top, f);
			break;
		default:
			status = 0;  // invalid option
		}
	}
	return status;
}

int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar)
{
	int status = 1;
	if (*what == '>') {
		StkId f = L->top - 1;
		if (!ttisfunction(f))
			luaG_runerror(L, "value for `lua_getinfo' is not a function");
		status = getinfo(L, what + 1, ar, f, NULL);
		L->top--;  // pop function
	}
	else if (ar->i_ci != 0) {  // no tail call?
		CallInfo* ci = L->base_ci + ar->i_ci;
		status = getinfo(L, what, ar, ci->base - 1, ci);
	}
	else
		info_tailcall(L, ar);
	if (strchr(what, 'f'))
		incr_top(L);
	return status;
}

int luaG_checkcode(const Proto* pt)
{
	int pc;
	for (pc = 0; pc < pt->sizecode; pc++) {
		Instruction i = pt->code[pc];
		OpCode op = GET_OPCODE(i);
		if (op >= NUM_OPCODES || GETARG_A(i) >= pt->maxstacksize + 3)
			return 0;
		if (op == OP_JMP || op == OP_FORLOOP || op == OP_TFORPREP) {
			int dest = pc + 1 + GETARG_sBx(i);
			if (dest < 0 || dest >= pt->sizecode)
				return 0;
		}
		if ((op == OP_LOADK || op == OP_GETGLOBAL || op == OP_SETGLOBAL || op == OP_GETSELFORGLOBAL ||
		     op == OP_SETSELFORGLOBAL || op == OP_DEFGLOBAL || op == OP_CONSTGLOBAL) && GETARG_Bx(i) >= pt->sizek)
			return 0;
		if (op == OP_CLOSURE && GETARG_Bx(i) >= pt->sizep)
			return 0;
	}
	return pt->sizecode > 0 && GET_OPCODE(pt->code[pt->sizecode - 1]) == OP_RETURN;
}

// ---- errors ------------------------------------------------------------------------------------------

static int isinstack(CallInfo* ci, const TObject* o)
{
	StkId p;
	for (p = ci->base; p < ci->top; p++)
		if (o == p)
			return 1;
	return 0;
}

void luaG_typeerror(lua_State* L, const TObject* o, const char* op)
{
	const char* name = NULL;
	const char* t = (ttype(o) >= 0 && ttype(o) <= LUA_TMETHOD) ? luaT_typenames[ttype(o)] : "no value";
	const char* kind = (isinstack(L->ci, o)) ? getobjname(L->ci, (int)(o - L->base), &name) : NULL;
	if (kind)
		luaG_runerror(L, "attempt to %s %s `%s' (a %s value)", op, kind, name, t);
	else
		luaG_runerror(L, "attempt to %s a %s value", op, t);
}

void luaG_concaterror(lua_State* L, StkId p1, StkId p2)
{
	if (ttisstring(p1))
		p1 = p2;
	luaG_typeerror(L, p1, "concatenate");
}

void luaG_aritherror(lua_State* L, const TObject* p1, const TObject* p2)
{
	TObject temp;
	if (luaV_tonumber(p1, &temp) == NULL)
		p2 = p1;  // first operand is wrong
	luaG_typeerror(L, p2, "perform arithmetic on");
}

void luaG_ordererror(lua_State* L, const TObject* p1, const TObject* p2)
{
	const char* t1 = luaT_typenames[ttype(p1)];
	const char* t2 = luaT_typenames[ttype(p2)];
	if (t1[2] == t2[2])
		luaG_runerror(L, "attempt to compare two %s values", t1);
	else
		luaG_runerror(L, "attempt to compare %s with %s", t1, t2);
}

static void addinfo(lua_State* L, const char* msg)
{
	CallInfo* ci = L->ci;
	if (isLua(ci)) {  // is Lua code?
		char buff[LUA_IDSIZE];  // add file:line information
		int line = currentline(ci);
		luaO_chunkid(buff, getstr(getluaproto(ci)->source), LUA_IDSIZE);
		luaO_pushfstring(L, "%s:%d: %s", buff, line, msg);
	}
}

void luaG_errormsg(lua_State* L)
{
	if (L->errfunc != 0) {  // is there an error handling function?
		StkId errfunc = restorestack(L, L->errfunc);
		if (!ttisfunction(errfunc))
			luaD_throw(L, LUA_ERRERR);
		setobjs2s(L->top, L->top - 1);  // move argument
		setobjs2s(L->top - 1, errfunc);  // push function
		incr_top(L);
		luaD_call(L, L->top - 2, 1);  // call it
	}
	luaD_throw(L, LUA_ERRRUN);
}

void luaG_runerror(lua_State* L, const char* fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	addinfo(L, luaO_pushvfstring(L, fmt, argp));
	va_end(argp);
	luaG_errormsg(L);
}
