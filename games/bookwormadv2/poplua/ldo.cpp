// Stack and call handling (Lua 5.0.2's ldo.c, as the original has it without the debug hooks). Errors unwind
// with a C++ exception instead of longjmp, so C++ objects on the way are destroyed properly.
#include <cstdlib>

#include "lcore.h"
#include "lopcodes.h"

#define MEMERRMSG "not enough memory"

struct lua_longjmp {
	lua_longjmp* previous;
	int status;  // error code
};

static void seterrorobj(lua_State* L, int errcode, StkId oldtop)
{
	switch (errcode) {
	case LUA_ERRMEM:
		setsvalue2s(oldtop, luaS_newliteral(L, MEMERRMSG));
		break;
	case LUA_ERRERR:
		setsvalue2s(oldtop, luaS_newliteral(L, "error in error handling"));
		break;
	case LUA_ERRSYNTAX:
	case LUA_ERRRUN:
		setobjs2s(oldtop, L->top - 1);  // error message on current top
		break;
	}
	L->top = oldtop + 1;
}

void luaD_throw(lua_State* L, int errcode)
{
	if (L->errorJmp) {
		L->errorJmp->status = errcode;
		throw L->errorJmp;
	}
	G(L)->panic(L);
	exit(EXIT_FAILURE);
}

int luaD_rawrunprotected(lua_State* L, Pfunc f, void* ud)
{
	lua_longjmp lj;
	lj.status = 0;
	lj.previous = L->errorJmp;  // chain new error handler
	L->errorJmp = &lj;
	try {
		(*f)(L, ud);
	}
	catch (lua_longjmp*) {
		if (lj.status == 0)
			lj.status = LUA_ERRRUN;
	}
	L->errorJmp = lj.previous;  // restore old error handler
	return lj.status;
}

static void restore_stack_limit(lua_State* L)
{
	L->stack_last = L->stack + L->stacksize - 1 - EXTRA_STACK;
	if (L->size_ci > LUA_MAXCALLS) {  // there was an overflow?
		int inuse = (int)(L->ci - L->base_ci);
		if (inuse + 1 < LUA_MAXCALLS)  // can `undo' overflow?
			luaD_reallocCI(L, LUA_MAXCALLS);
	}
}

// ---- stack reallocation ------------------------------------------------------------------------------

static void correctstack(lua_State* L, TObject* oldstack)
{
	CallInfo* ci;
	GCObject* up;
	L->top = (L->top - oldstack) + L->stack;
	for (up = L->openupval; up != NULL; up = up->gch.next)
		gcotouv(up)->v = (gcotouv(up)->v - oldstack) + L->stack;
	for (ci = L->base_ci; ci <= L->ci; ci++) {
		ci->top = (ci->top - oldstack) + L->stack;
		ci->base = (ci->base - oldstack) + L->stack;
	}
	L->base = L->ci->base;
}

void luaD_reallocstack(lua_State* L, int newsize)
{
	TObject* oldstack = L->stack;
	luaM_reallocvector(L, L->stack, L->stacksize, newsize, TObject);
	L->stacksize = newsize;
	L->stack_last = L->stack + newsize - 1 - EXTRA_STACK;
	correctstack(L, oldstack);
}

void luaD_reallocCI(lua_State* L, int newsize)
{
	CallInfo* oldci = L->base_ci;
	luaM_reallocvector(L, L->base_ci, L->size_ci, newsize, CallInfo);
	L->size_ci = (unsigned short)newsize;
	L->ci = (L->ci - oldci) + L->base_ci;
	L->end_ci = L->base_ci + L->size_ci;
}

void luaD_growstack(lua_State* L, int n)
{
	if (n <= L->stacksize)  // double size is enough?
		luaD_reallocstack(L, 2 * L->stacksize);
	else
		luaD_reallocstack(L, L->stacksize + n + EXTRA_STACK);
}

static void luaD_growCI(lua_State* L)
{
	if (L->size_ci > LUA_MAXCALLS)  // overflow while handling overflow?
		luaD_throw(L, LUA_ERRERR);
	else {
		luaD_reallocCI(L, 2 * L->size_ci);
		if (L->size_ci > LUA_MAXCALLS)
			luaG_runerror(L, "stack overflow");
	}
}

// ---- calls -------------------------------------------------------------------------------------------

// 0x708ed0: the extra arguments go to the `arg' table at the int keys 0 .. n-1, with arg.n = n (an int)
static void adjust_varargs(lua_State* L, int nfixargs, StkId base)
{
	int i;
	Table* htab;
	TObject nname;
	int actual = (int)(L->top - base);  // actual number of arguments
	if (actual < nfixargs) {
		luaD_checkstack(L, nfixargs - actual);
		for (; actual < nfixargs; ++actual)
			setnilvalue(L->top++);
	}
	actual -= nfixargs;  // number of extra arguments
	htab = luaH_new(L, actual, 1);  // create `arg' table
	for (i = 0; i < actual; i++)  // put extra arguments into `arg' table
		setobj2n(luaH_setnum(L, htab, i), L->top - actual + i);
	// store counter in field `n'
	setsvalue(&nname, luaS_newliteral(L, "n"));
	setivalue(luaH_set(L, htab, &nname), actual);
	L->top -= actual;  // remove extra elements from the stack
	sethvalue(L->top, htab);
	incr_top(L);
}

static StkId tryfuncTM(lua_State* L, StkId func)
{
	const TObject* tm = luaT_gettmbyobj(L, func, TM_CALL);
	StkId p;
	ptrdiff_t funcr = savestack(L, func);
	if (!ttisfunction(tm))
		luaG_typeerror(L, func, "call");
	// open a hole inside the stack at `func'
	for (p = L->top; p > func; p--)
		setobjs2s(p, p - 1);
	incr_top(L);
	func = restorestack(L, funcr);  // previous call may change stack
	setobj2s(func, tm);  // tag method is the new function to be called
	return func;
}

// 0x709190
StkId luaD_precall(lua_State* L, StkId func)
{
	LClosure* cl;
	ptrdiff_t funcr = savestack(L, func);
	if (ttype(func) == LUA_TMETHOD)
		setttype(func, LUA_TFUNCTION);
	if (!ttisfunction(func))  // `func' is not a function?
		func = tryfuncTM(L, func);  // check the `function' tag method
	if (L->ci + 1 == L->end_ci)
		luaD_growCI(L);
	cl = &clvalue(func)->l;
	if (!cl->isC) {  // Lua function? prepare its call
		CallInfo* ci;
		Proto* p = cl->p;
		if (p->is_vararg)  // varargs?
			adjust_varargs(L, p->numparams, func + 1);
		luaD_checkstack(L, p->maxstacksize);
		ci = ++L->ci;  // now `enter' new function
		L->base = L->ci->base = restorestack(L, funcr) + 1;
		ci->top = L->base + p->maxstacksize;
		ci->savedpc = p->code;  // starting point
		ci->tailcalls = 0;
		ci->state = CI_SAVEDPC;
		while (L->top < ci->top)
			setnilvalue(L->top++);
		L->top = ci->top;
		return NULL;
	}
	else {  // if is a C function, call it
		CallInfo* ci;
		int n;
		luaD_checkstack(L, LUA_MINSTACK);  // ensure minimum stack size
		ci = ++L->ci;  // now `enter' new function
		L->base = L->ci->base = restorestack(L, funcr) + 1;
		ci->top = L->top + LUA_MINSTACK;
		ci->state = CI_C;  // a C function
		n = (*clvalue(L->base - 1)->c.f)(L);  // do the actual call
		return L->top - n;
	}
}

void luaD_poscall(lua_State* L, int wanted, StkId firstResult)
{
	StkId res;
	res = L->base - 1;  // res == final position of 1st result
	L->ci--;
	L->base = L->ci->base;  // restore base
	// move results to correct place
	while (wanted != 0 && firstResult < L->top) {
		setobjs2s(res++, firstResult++);
		wanted--;
	}
	while (wanted-- > 0)
		setnilvalue(res++);
	L->top = res;
}

// Call a function (C or Lua). The function to be called is at *func. The arguments are on the stack, right
// after the function. When returns, all the results are on the stack, starting at the original function
// position.
void luaD_call(lua_State* L, StkId func, int nResults)
{
	StkId firstResult;
	if (++L->nCcalls >= LUA_MAXCCALLS) {
		if (L->nCcalls == LUA_MAXCCALLS)
			luaG_runerror(L, "C stack overflow");
		else if (L->nCcalls >= (LUA_MAXCCALLS + (LUA_MAXCCALLS >> 3)))
			luaD_throw(L, LUA_ERRERR);  // error while handing stack error
	}
	firstResult = luaD_precall(L, func);
	if (firstResult == NULL)  // is a Lua function?
		firstResult = luaV_execute(L);  // call it
	luaD_poscall(L, nResults, firstResult);
	L->nCcalls--;
	luaC_checkGC(L);
}

// ---- coroutines --------------------------------------------------------------------------------------

static void resume(lua_State* L, void* ud)
{
	StkId firstResult;
	int nargs = *(int*)ud;
	CallInfo* ci = L->ci;
	if (ci == L->base_ci)  // no activation record?
		luaD_precall(L, L->top - (nargs + 1));  // start coroutine
	else {  // inside a yield
		if (ci->state & CI_C) {  // `common' yield? finish interrupted execution of `OP_CALL'
			int nresults = GETARG_C(*((ci - 1)->savedpc - 1)) - 1;
			luaD_poscall(L, nresults, L->top - nargs);  // complete it
			if (nresults >= 0)
				L->top = L->ci->top;
		}
		else  // yielded inside a hook: just continue its execution
			ci->state &= ~CI_YIELD;
	}
	firstResult = luaV_execute(L);
	if (firstResult != NULL)  // return?
		luaD_poscall(L, LUA_MULTRET, firstResult);  // finalize this coroutine
}

static int resume_error(lua_State* L, const char* msg)
{
	L->top = L->ci->base;
	setsvalue2s(L->top, luaS_new(L, msg));
	incr_top(L);
	return LUA_ERRRUN;
}

int lua_resume(lua_State* L, int nargs)
{
	int status;
	lu_byte old_allowhooks;
	if (L->ci == L->base_ci) {
		if (nargs >= L->top - L->base)
			return resume_error(L, "cannot resume dead coroutine");
	}
	else if (!(L->ci->state & CI_YIELD))  // not inside a yield?
		return resume_error(L, "cannot resume non-suspended coroutine");
	old_allowhooks = L->allowhook;
	status = luaD_rawrunprotected(L, resume, &nargs);
	if (status != 0) {  // error?
		L->ci = L->base_ci;  // go back to initial level
		L->base = L->ci->base;
		L->nCcalls = 0;
		luaF_close(L, L->base);  // close eventual pending closures
		seterrorobj(L, status, L->base);
		L->allowhook = old_allowhooks;
		restore_stack_limit(L);
	}
	return status;
}

int lua_yield(lua_State* L, int nresults)
{
	CallInfo* ci = L->ci;
	if (L->nCcalls > 0)
		luaG_runerror(L, "attempt to yield across metamethod/C-call boundary");
	if (ci->state & CI_C) {  // usual yield
		if ((ci - 1)->state & CI_C)
			luaG_runerror(L, "cannot yield a C function");
		if (L->top - nresults > L->base) {  // is there garbage in the stack?
			int i;
			for (i = 0; i < nresults; i++)  // move down results
				setobjs2s(L->base + i, L->top - nresults + i);
			L->top = L->base + nresults;
		}
	}
	ci->state |= CI_YIELD;
	return -1;
}

// ---- protected calls and the parser ------------------------------------------------------------------

int luaD_pcall(lua_State* L, Pfunc func, void* u, ptrdiff_t old_top, ptrdiff_t ef)
{
	int status;
	unsigned short oldnCcalls = L->nCcalls;
	ptrdiff_t old_ci = saveci(L, L->ci);
	lu_byte old_allowhooks = L->allowhook;
	ptrdiff_t old_errfunc = L->errfunc;
	L->errfunc = ef;
	status = luaD_rawrunprotected(L, func, u);
	if (status != 0) {  // an error occurred?
		StkId oldtop = restorestack(L, old_top);
		luaF_close(L, oldtop);  // close eventual pending closures
		seterrorobj(L, status, oldtop);
		L->nCcalls = oldnCcalls;
		L->ci = restoreci(L, old_ci);
		L->base = L->ci->base;
		L->allowhook = old_allowhooks;
		restore_stack_limit(L);
	}
	L->errfunc = old_errfunc;
	return status;
}

struct SParser {  // data to `f_parser'
	ZIO* z;
	int bin;
};

static void f_parser(lua_State* L, void* ud)
{
	SParser* p;
	Proto* tf;
	Closure* cl;
	luaC_checkGC(L);
	p = (SParser*)ud;
	tf = p->bin ? luaU_undump(L, p->z) : luaY_parser(L, p->z);
	cl = luaF_newLclosure(L, 0, gt(L));
	cl->l.p = tf;
	setclvalue(L->top, cl);
	incr_top(L);
}

int luaD_protectedparser(lua_State* L, void* z, int bin)
{
	SParser p;
	int status;
	ptrdiff_t oldtopr = savestack(L, L->top);  // save current top
	p.z = (ZIO*)z;
	p.bin = bin;
	status = luaD_rawrunprotected(L, f_parser, &p);
	if (status != 0) {  // error?
		StkId oldtop = restorestack(L, oldtopr);
		seterrorobj(L, status, oldtop);
	}
	return status;
}
