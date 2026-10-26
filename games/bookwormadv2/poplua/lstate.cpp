// The global state and threads (Lua 5.0.2's lstate.c). Metatables are PopLua's (5.1's): a table or userdata
// without one has NULL, and the basic types have theirs in global_State::mt.
#include "lcore.h"

#define MEMERRMSG "not enough memory"

static void stack_init(lua_State* L1, lua_State* L)
{
	L1->stack = luaM_newvector(L, BASIC_STACK_SIZE + EXTRA_STACK, TObject);
	L1->stacksize = BASIC_STACK_SIZE + EXTRA_STACK;
	L1->top = L1->stack;
	L1->stack_last = L1->stack + (L1->stacksize - EXTRA_STACK) - 1;
	L1->base_ci = luaM_newvector(L, BASIC_CI_SIZE, CallInfo);
	L1->ci = L1->base_ci;
	L1->ci->state = CI_C;  // not a Lua function
	setnilvalue(L1->top++);  // `function' entry for this `ci'
	L1->base = L1->ci->base = L1->top;
	L1->ci->top = L1->top + LUA_MINSTACK;
	L1->size_ci = BASIC_CI_SIZE;
	L1->end_ci = L1->base_ci + L1->size_ci;
}

static void freestack(lua_State* L, lua_State* L1)
{
	luaM_freearray(L, L1->base_ci, L1->size_ci, CallInfo);
	luaM_freearray(L, L1->stack, L1->stacksize, TObject);
}

static void f_luaopen(lua_State* L, void* ud)
{
	int i;
	(void)ud;
	global_State* g = luaM_new(NULL, global_State);
	if (g == NULL)
		luaD_throw(L, LUA_ERRMEM);
	L->l_G = g;
	g->mainthread = L;
	g->GCthreshold = 0;  // mark it as unfinished state
	g->strt.size = 0;
	g->strt.nuse = 0;
	g->strt.hash = NULL;
	setnilvalue(&g->_defaultmeta);
	setnilvalue(registry(L));
	g->buff = NULL;
	g->buffsize = 0;
	g->gcstepmul = 200;
	g->panic = NULL;
	g->rootgc = NULL;
	g->rootudata = NULL;
	g->tmudata = NULL;
	setnilvalue(gkey(g->dummynode));
	setnilvalue(gval(g->dummynode));
	g->dummynode->next = NULL;
	g->dummynode->flags = 0;
	for (i = 0; i < TM_N; i++)
		g->tmname[i] = NULL;
	for (i = 0; i <= LUA_TTHREAD; i++)
		g->mt[i] = NULL;
	g->nblocks = sizeof(lua_State) + sizeof(global_State);
	stack_init(L, L);  // init stack
	luaS_resize(L, MINSTRTABSIZE);  // initial size of string table
	sethvalue(gt(L), luaH_new(L, 0, 4));  // table of globals
	sethvalue(registry(L), luaH_new(L, 4, 4));  // registry
	luaT_init(L);
	luaS_fix(luaS_newliteral(L, MEMERRMSG));
	g->GCthreshold = 4 * G(L)->nblocks;
}

static void preinit_state(lua_State* L)
{
	L->stack = NULL;
	L->stacksize = 0;
	L->errorJmp = NULL;
	L->allowhook = 1;
	L->openupval = NULL;
	L->size_ci = 0;
	L->nCcalls = 0;
	L->base_ci = L->ci = NULL;
	L->errfunc = 0;
	setnilvalue(gt(L));
}

static void close_state(lua_State* L)
{
	luaF_close(L, L->stack);  // close all upvalues for this thread
	if (G(L)) {  // close global state
		luaC_sweep(L, 1);  // collect all elements
		luaS_freeall(L);
		luaM_freearray(L, G(L)->buff, G(L)->buffsize, char);
	}
	freestack(L, L);
	if (G(L))
		luaM_freelem(NULL, G(L));
	luaM_freelem(NULL, L);
}

lua_State* lua_newthread(lua_State* L)
{
	lua_State* L1 = luaM_new(L, lua_State);
	luaC_link(L, valtogco(L1), LUA_TTHREAD);
	preinit_state(L1);
	L1->l_G = L->l_G;
	stack_init(L1, L);  // init stack
	setobj2n(gt(L1), gt(L));  // share table of globals
	setthvalue(L->top, L1);
	incr_top(L);
	return L1;
}

void luaE_freethread(lua_State* L, lua_State* L1)
{
	luaF_close(L1, L1->stack);  // close all upvalues for this thread
	freestack(L, L1);
	luaM_freelem(L, L1);
}

lua_State* lua_open(void)
{
	lua_State* L = luaM_new(NULL, lua_State);
	if (L) {  // allocation OK?
		L->tt = LUA_TTHREAD;
		L->marked = 0;
		L->next = NULL;
		preinit_state(L);
		L->l_G = NULL;
		if (luaD_rawrunprotected(L, f_luaopen, NULL) != 0) {
			// memory allocation error: free partial state
			close_state(L);
			L = NULL;
		}
	}
	return L;
}

static void callallgcTM(lua_State* L, void* ud)
{
	(void)ud;
	luaC_callGCTM(L);  // call GC metamethods for all udata
}

void lua_close(lua_State* L)
{
	L = G(L)->mainthread;  // only the main thread can be closed
	luaF_close(L, L->stack);  // close all upvalues for this thread
	luaC_separateudata(L);  // separate udata that have GC metamethods
	L->errfunc = 0;  // no error function during GC metamethods
	do {  // repeat until no more errors
		L->ci = L->base_ci;
		L->base = L->top = L->ci->base;
		L->nCcalls = 0;
	} while (luaD_rawrunprotected(L, callallgcTM, NULL) != 0);
	close_state(L);
}
