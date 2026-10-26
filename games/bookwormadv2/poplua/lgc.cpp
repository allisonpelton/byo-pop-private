// The garbage collector: Lua 5.0.2's stop-the-world mark and sweep (lgc.c), with PopLua's additions marked.
// The original collects incrementally (5.1 style, collectgarbage("setstepmul")); when objects die makes no
// difference to the scripts, which never depend on it, so the port keeps 5.0.2's simpler collector.
#include <vector>

#include "lcore.h"

#define FIXEDBIT 4
#define KEYWEAKBIT 1
#define VALUEWEAKBIT 2
#define KEYWEAK (1 << KEYWEAKBIT)
#define VALUEWEAK (1 << VALUEWEAKBIT)

#define unmark(x) ((x)->gch.marked &= (lu_byte)~1)
#define ismarked(x) ((x)->gch.marked & ((1 << FIXEDBIT) | 1))
#define stringmark(s) ((s)->marked |= 1)
#define isfinalized(u) (!((u)->marked & (1 << 1)))
#define markfinalized(u) ((u)->marked &= (lu_byte)~(1 << 1))

struct GCState {
	std::vector<GCObject*> gray;  // marked objects still to be traversed
	std::vector<Table*> wk;       // traversed key-weak tables (to be cleared)
	std::vector<Table*> wv;       // value-weak tables
	std::vector<Table*> wkv;      // key-value weak tables
	global_State* g;
};

static void reallymarkobject(GCState* st, GCObject* o);

#define markobject(st, o) { if (iscollectable(o) && !ismarked(gcvalue(o))) reallymarkobject(st, gcvalue(o)); }
#define condmarkobject(st, o, c) { if (iscollectable(o) && !ismarked(gcvalue(o)) && (c)) reallymarkobject(st, gcvalue(o)); }
#define markvalue(st, t) { if ((t) != NULL && !ismarked(valtogco(t))) reallymarkobject(st, valtogco(t)); }

static void reallymarkobject(GCState* st, GCObject* o)
{
	o->gch.marked |= 1;
	switch (o->gch.tt) {
	case LUA_TSTRING:
		break;
	case LUA_TUSERDATA:
		markvalue(st, gcotou(o)->metatable);
		break;
	default:  // functions, tables, threads and prototypes are traversed later
		st->gray.push_back(o);
	}
}

static void traverseproto(GCState* st, Proto* f)
{
	int i;
	if (f->source)
		stringmark(f->source);
	for (i = 0; i < f->sizek; i++) {  // mark literal strings
		if (ttisstring(f->k + i))
			stringmark(tsvalue(f->k + i));
	}
	for (i = 0; i < f->sizeupvalues; i++) {  // mark upvalue names
		if (f->upvalues[i])
			stringmark(f->upvalues[i]);
	}
	for (i = 0; i < f->sizep; i++)  // mark nested protos
		markvalue(st, f->p[i]);
	for (i = 0; i < f->sizelocvars; i++) {  // mark local-variable names
		if (f->locvars[i].varname)
			stringmark(f->locvars[i].varname);
	}
}

static void traverseclosure(GCState* st, Closure* cl)
{
	int i;
	if (cl->c.isC) {
		for (i = 0; i < cl->c.nupvalues; i++)  // mark its upvalues
			markobject(st, &cl->c.upvalue[i]);
	}
	else {
		markobject(st, &cl->l.g);
		markvalue(st, cl->l.p);
		for (i = 0; i < cl->l.nupvalues; i++) {  // mark its upvalues
			UpVal* u = cl->l.upvals[i];
			if (!u->marked) {  // open upvalues stay marked; their values are on a stack
				markobject(st, &u->value);
				u->marked = 1;
			}
		}
	}
}

static void traversetable(GCState* st, Table* h)
{
	int i;
	int weakkey = 0;
	int weakvalue = 0;
	const TObject* mode;
	markvalue(st, h->metatable);
	markvalue(st, h->parent);  // PopLua: the class chain
	mode = gfasttm(st->g, h->metatable, TM_MODE);
	h->marked &= (lu_byte)~(KEYWEAK | VALUEWEAK);
	if (mode && ttisstring(mode)) {  // is there a weak mode?
		weakkey = (strchr(svalue(mode), 'k') != NULL);
		weakvalue = (strchr(svalue(mode), 'v') != NULL);
		if (weakkey || weakvalue) {  // is really weak?
			h->marked |= (lu_byte)((weakkey << KEYWEAKBIT) | (weakvalue << VALUEWEAKBIT));
			if (weakkey && weakvalue)
				st->wkv.push_back(h);
			else if (weakkey)
				st->wk.push_back(h);
			else
				st->wv.push_back(h);
		}
	}
	if (!weakvalue) {
		i = h->sizearray;
		while (i--)
			markobject(st, &h->array[i]);
	}
	i = sizenode(h);
	while (i--) {
		Node* n = gnode(h, i);
		// every key is kept, also those of nodes whose value is nil: lookups compare string keys by content
		// (case-insensitively) and so read them
		condmarkobject(st, gkey(n), !weakkey);
		if (!ttisnil(gval(n)))
			condmarkobject(st, gval(n), !weakvalue);
	}
}

static void traversestack(GCState* st, lua_State* L1)
{
	StkId o, lim;
	CallInfo* ci;
	markobject(st, gt(L1));
	lim = L1->top;
	for (ci = L1->base_ci; ci <= L1->ci; ci++) {
		if (lim < ci->top && !(ci->state & CI_C))
			lim = ci->top;
	}
	for (o = L1->stack; o < L1->top; o++)
		markobject(st, o);
	for (; o <= lim; o++)
		setnilvalue(o);
	// shrink an oversized stack (5.0.2's checkstacksizes)
	if (L1->size_ci > LUA_MAXCALLS) {  // handling overflow?
		// cannot shrink while handling an overflow
	}
	else {
		int ci_used = (int)(L1->ci - L1->base_ci);
		int s_used = (int)(lim - L1->stack);
		if (4 * ci_used < L1->size_ci && 2 * BASIC_CI_SIZE < L1->size_ci)
			luaD_reallocCI(L1, L1->size_ci / 2);
		if (4 * s_used < L1->stacksize && 2 * (BASIC_STACK_SIZE + EXTRA_STACK) < L1->stacksize)
			luaD_reallocstack(L1, L1->stacksize / 2);
	}
}

static void propagatemarks(GCState* st)
{
	while (!st->gray.empty()) {
		GCObject* o = st->gray.back();
		st->gray.pop_back();
		switch (o->gch.tt) {
		case LUA_TTABLE:
			traversetable(st, gcotoh(o));
			break;
		case LUA_TFUNCTION:
			traverseclosure(st, gcotocl(o));
			break;
		case LUA_TTHREAD:
			traversestack(st, gcototh(o));
			break;
		case LUA_TPROTO:
			traverseproto(st, gcotop(o));
			break;
		}
	}
}

static int valismarked(const TObject* o)
{
	if (ttisstring(o))
		stringmark(tsvalue(o));  // strings are `values', so are never weak
	return !iscollectable(o) || (o->value.gc->gch.marked & 1);
}

// an entry of a weak table whose key or value was collected: the node stays in its chain with a dead key
static void removekey(Node* n)
{
	setnilvalue(gval(n));
	n->flags = 0;
	if (iscollectable(gkey(n)))
		setttype(gkey(n), LUA_TNONE);
}

static void cleartablekeys(std::vector<Table*>& l)
{
	for (Table* h : l) {
		int i = sizenode(h);
		while (i--) {
			Node* n = gnode(h, i);
			if (!valismarked(gkey(n)))
				removekey(n);
		}
	}
}

static void cleartablevalues(std::vector<Table*>& l)
{
	for (Table* h : l) {
		int i = h->sizearray;
		while (i--) {
			TObject* o = &h->array[i];
			if (!valismarked(o))
				setnilvalue(o);
		}
		i = sizenode(h);
		while (i--) {
			Node* n = gnode(h, i);
			if (!ttisnil(gval(n)) && !valismarked(gval(n)))
				removekey(n);
		}
	}
}

static void freeobj(lua_State* L, GCObject* o)
{
	switch (o->gch.tt) {
	case LUA_TPROTO:
		luaF_freeproto(L, gcotop(o));
		break;
	case LUA_TFUNCTION:
		luaF_freeclosure(L, gcotocl(o));
		break;
	case LUA_TUPVAL:
		luaM_freelem(L, gcotouv(o));
		break;
	case LUA_TTABLE:
		luaH_free(L, gcotoh(o));
		break;
	case LUA_TTHREAD:
		luaE_freethread(L, gcototh(o));
		break;
	case LUA_TSTRING: {
		TString* ts = gcotots(o);
		if (ts->alias != NULL && ts->alias->alias == ts)  // PopLua's lookup cache
			ts->alias->alias = NULL;
		luaM_free(L, o, sizestring(ts->len));
		break;
	}
	case LUA_TUSERDATA:
		luaM_free(L, o, sizeudata(gcotou(o)->len));
		break;
	}
}

static int sweeplist(lua_State* L, GCObject** p, int all)
{
	GCObject* curr;
	int freed = 0;
	while ((curr = *p) != NULL) {
		if (!all && ismarked(curr)) {
			unmark(curr);
			p = &curr->gch.next;
		}
		else {
			*p = curr->gch.next;
			freeobj(L, curr);
			freed++;
		}
	}
	return freed;
}

static void sweepstrings(lua_State* L, int all)
{
	int i;
	for (i = 0; i < G(L)->strt.size; i++)  // for each list
		G(L)->strt.nuse -= sweeplist(L, &G(L)->strt.hash[i], all);
}

static void checkSizes(lua_State* L, size_t deadmem)
{
	// check size of string hash
	if (G(L)->strt.nuse < (lu_hash)(G(L)->strt.size / 4) && G(L)->strt.size > MINSTRTABSIZE * 2)
		luaS_resize(L, G(L)->strt.size / 2);  // table is too big
	// check size of buffer
	if (G(L)->buffsize > 32 * 2) {  // buffer too big?
		size_t newsize = G(L)->buffsize / 2;
		G(L)->buff = (char*)luaM_realloc(L, G(L)->buff, G(L)->buffsize, newsize);
		G(L)->buffsize = newsize;
	}
	G(L)->GCthreshold = 2 * G(L)->nblocks - deadmem;  // new threshold
}

static void do1gcTM(lua_State* L, Udata* udata)
{
	const TObject* tm = fasttm(L, udata->metatable, TM_GC);
	if (tm != NULL) {
		setobj2s(L->top, tm);
		setuvalue(L->top + 1, udata);
		L->top += 2;
		luaD_call(L, L->top - 2, 0);
	}
}

void luaC_callGCTM(lua_State* L)
{
	lu_byte oldah = L->allowhook;
	L->allowhook = 0;
	L->top++;  // reserve space to keep udata while runs its gc method
	while (G(L)->tmudata != NULL) {
		GCObject* o = G(L)->tmudata;
		Udata* udata = gcotou(o);
		G(L)->tmudata = udata->next;  // remove udata from `tmudata'
		udata->next = G(L)->rootudata;  // return it to `root' list
		G(L)->rootudata = o;
		setuvalue(L->top - 1, udata);  // keep a reference to it
		unmark(o);
		markfinalized(udata);
		do1gcTM(L, udata);
	}
	L->top--;
	L->allowhook = oldah;
}

void luaC_sweep(lua_State* L, int all)
{
	sweeplist(L, &G(L)->rootudata, all);
	sweepstrings(L, all);
	sweeplist(L, &G(L)->rootgc, all);
}

// separate the userdata to be finalized; returns their memory
static size_t separateudata(lua_State* L)
{
	size_t deadmem = 0;
	GCObject** p = &G(L)->rootudata;
	GCObject* curr;
	GCObject* collected = NULL;  // to collect udata with gc event
	GCObject** lastcollected = &collected;
	while ((curr = *p) != NULL) {
		if (ismarked(curr) || isfinalized(gcotou(curr)))
			p = &curr->gch.next;  // don't bother with them
		else if (fasttm(L, gcotou(curr)->metatable, TM_GC) == NULL) {
			markfinalized(gcotou(curr));  // don't need finalization
			p = &curr->gch.next;
		}
		else {  // must call its gc method
			deadmem += sizeudata(gcotou(curr)->len);
			*p = curr->gch.next;
			curr->gch.next = NULL;  // link `curr' at the end of `collected' list
			*lastcollected = curr;
			lastcollected = &curr->gch.next;
		}
	}
	// insert collected udata with gc event into `tmudata' list
	*lastcollected = G(L)->tmudata;
	G(L)->tmudata = collected;
	return deadmem;
}

void luaC_separateudata(lua_State* L)
{
	separateudata(L);
}

static size_t mark(lua_State* L)
{
	size_t deadmem;
	GCState st;
	std::vector<Table*> wkv;
	int i;
	st.g = G(L);
	markobject(&st, registry(L));
	for (i = 0; i <= LUA_TTHREAD; i++)
		markvalue(&st, G(L)->mt[i]);
	traversestack(&st, G(L)->mainthread);
	if (L != G(L)->mainthread)  // another thread is running?
		markvalue(&st, L);  // cannot collect it
	propagatemarks(&st);  // mark all reachable objects
	cleartablevalues(st.wkv);
	cleartablevalues(st.wv);
	wkv.swap(st.wkv);  // keys must be cleared after preserving udata
	st.wv.clear();
	deadmem = separateudata(L);  // separate userdata to be preserved
	for (GCObject* u = G(L)->tmudata; u; u = u->gch.next) {  // mark `preserved' userdata
		u->gch.marked |= 1;
		markvalue(&st, gcotou(u)->metatable);
	}
	propagatemarks(&st);  // remark, to propagate `preserveness'
	cleartablekeys(wkv);
	// `propagatemarks' may resuscitate some weak tables; clear them too
	cleartablekeys(st.wk);
	cleartablevalues(st.wv);
	cleartablekeys(st.wkv);
	cleartablevalues(st.wkv);
	return deadmem;
}

void luaC_collectgarbage(lua_State* L)
{
	size_t deadmem = mark(L);
	luaC_sweep(L, 0);
	checkSizes(L, deadmem);
	luaC_callGCTM(L);
}

void luaC_link(lua_State* L, GCObject* o, lu_byte tt)
{
	o->gch.next = G(L)->rootgc;
	G(L)->rootgc = o;
	o->gch.marked = 0;
	o->gch.tt = tt;
}
