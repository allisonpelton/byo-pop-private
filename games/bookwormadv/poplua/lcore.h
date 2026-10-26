// PopLua internals: objects, the state, and the core modules' functions (Lua 5.0.2's lobject.h, lstate.h,
// lstring.h, ltable.h, lfunc.h, lgc.h, ldo.h, lvm.h, ltm.h, ldebug.h in one header). The layout follows the
// original where a script could see the difference (docs/POPLUA.md); elsewhere it is simply Lua 5.0's.
#ifndef POPLUA_LCORE_H
#define POPLUA_LCORE_H

#include <climits>
#include <cstring>

#include "lua.h"

typedef unsigned char lu_byte;
typedef uint32_t lu_hash;
typedef uint32_t Instruction;
typedef size_t lu_mem;

#define MAX_INT (INT_MAX - 2)
#define MAXSTACK 250   // registers per function; RK operands >= MAXSTACK are constants
#define LUA_MAXCCALLS 200
#define LUA_MAXCALLS 4096
#define BASIC_CI_SIZE 8
#define BASIC_STACK_SIZE (2 * LUA_MINSTACK)
#define EXTRA_STACK 5
#define MINSTRTABSIZE 32
#define LFIELDS_PER_FLUSH 32
#define MAXBITS 24     // the array part holds the int keys 0 .. 2^MAXBITS
#define MAXASIZE (1 << MAXBITS)

// ---- objects ----------------------------------------------------------------------------------------

union GCObject;
struct Table;
struct TString;
struct Proto;
struct UpVal;
union Closure;
struct global_State;

#define CommonHeader \
	GCObject* next;  \
	lu_byte tt;      \
	lu_byte marked

struct GCheader {
	CommonHeader;
};

union Value {
	GCObject* gc;
	void* p;
	lua_Number n;
	lua_Int i;
	int b;
};

struct TObject {
	int tt;
	Value value;
};

#define ttisnil(o) ((o)->tt == LUA_TNIL)
#define ttisnumber(o) ((o)->tt == LUA_TNUMBER)
#define ttisint(o) ((o)->tt == LUA_TINT)
#define ttisstring(o) ((o)->tt == LUA_TSTRING)
#define ttistable(o) ((o)->tt == LUA_TTABLE)
#define ttisfunction(o) ((o)->tt == LUA_TFUNCTION)
#define ttisboolean(o) ((o)->tt == LUA_TBOOLEAN)
#define ttisuserdata(o) ((o)->tt == LUA_TUSERDATA)
#define ttisthread(o) ((o)->tt == LUA_TTHREAD)
#define ttislightuserdata(o) ((o)->tt == LUA_TLIGHTUSERDATA)

#define ttype(o) ((o)->tt)
#define gcvalue(o) ((o)->value.gc)
#define pvalue(o) ((o)->value.p)
#define nvalue(o) ((o)->value.n)
#define ivalue(o) ((o)->value.i)
#define tsvalue(o) (&(o)->value.gc->ts)
#define uvalue(o) (&(o)->value.gc->u)
#define clvalue(o) (&(o)->value.gc->cl)
#define hvalue(o) (&(o)->value.gc->h)
#define bvalue(o) ((o)->value.b)
#define thvalue(o) (&(o)->value.gc->th)

#define l_isfalse(o) (ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

// collectable objects are those with a tag above LUA_TINT
#define iscollectable(o) (ttype(o) >= LUA_TSTRING)

#define setnvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TNUMBER; i_o->value.n = (x); }
#define chgnvalue(obj, x) ((obj)->value.n = (x))
#define setivalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TINT; i_o->value.i = (x); }
#define setpvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TLIGHTUSERDATA; i_o->value.p = (x); }
#define setbvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TBOOLEAN; i_o->value.b = (x); }
#define setsvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TSTRING; i_o->value.gc = (GCObject*)(x); }
#define setuvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TUSERDATA; i_o->value.gc = (GCObject*)(x); }
#define setthvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TTHREAD; i_o->value.gc = (GCObject*)(x); }
#define setclvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TFUNCTION; i_o->value.gc = (GCObject*)(x); }
#define sethvalue(obj, x) { TObject* i_o = (obj); i_o->tt = LUA_TTABLE; i_o->value.gc = (GCObject*)(x); }
#define setnilvalue(obj) ((obj)->tt = LUA_TNIL)
#define setobj(obj1, obj2) { const TObject* o2 = (obj2); TObject* o1 = (obj1); o1->tt = o2->tt; o1->value = o2->value; }
#define setobjs2s setobj
#define setobj2s setobj
#define setsvalue2s setsvalue
#define setobjt2t setobj
#define setobj2t setobj
#define setobj2n setobj
#define setsvalue2n setsvalue
#define setttype(obj, tt) (ttype(obj) = (tt))

typedef TObject* StkId;

struct TString {
	CommonHeader;
	lu_byte reserved;
	lu_hash hash;     // case-insensitive (the characters are folded to upper case)
	size_t len;
	TString* alias;   // the original's cache of a case-insensitively equal key (no effect on results)
	// the characters follow, with a terminating '\0'
};
#define getstr(ts) ((char*)((ts) + 1))
#define svalue(o) getstr(tsvalue(o))

struct Udata {
	CommonHeader;
	Table* metatable;
	size_t len;
	// the block follows (aligned for doubles)
	double align_;
};
#define udata_block(u) ((void*)(&(u)->align_))

struct LocVar {
	TString* varname;
	int startpc;
	int endpc;
};

struct Proto {
	CommonHeader;
	TObject* k;
	Instruction* code;
	Proto** p;
	int* lineinfo;
	LocVar* locvars;
	TString** upvalues;
	TString* source;
	int sizeupvalues;
	int sizek;
	int sizecode;
	int sizelineinfo;
	int sizep;
	int sizelocvars;
	int lineDefined;
	int selfflag;     // PopLua: a method (`def' in a class); SELFORGLOBAL marks its value as a method
	lu_byte nups;
	lu_byte numparams;
	lu_byte is_vararg;
	lu_byte maxstacksize;
};

struct UpVal {
	CommonHeader;
	TObject* v;      // points to stack or to its own value
	TObject value;   // the value (when closed)
};

#define ClosureHeader \
	CommonHeader;     \
	lu_byte isC;      \
	lu_byte nupvalues

struct CClosure {
	ClosureHeader;
	lua_CFunction f;
	TObject upvalue[1];
};

struct LClosure {
	ClosureHeader;
	Proto* p;
	TObject g;      // the environment table
	UpVal* upvals[1];
};

union Closure {
	CClosure c;
	LClosure l;
};

#define iscfunction(o) (ttype(o) == LUA_TFUNCTION && clvalue(o)->c.isC)
#define isLfunction(o) (ttype(o) == LUA_TFUNCTION && !clvalue(o)->c.isC)

// tables; node flag 1: the key was defined (DEFGLOBAL/DEFTABLE) and stays with a nil value; flag 2: constant
#define NODE_DEFINED 1
#define NODE_CONST 2

struct Node {
	TObject i_key;
	TObject i_val;
	int flags;
	Node* next;
};

// Table::prot bits (setprot; 0x20 makes string keys case-sensitive)
#define PROT_CASESENSITIVE 0x20

struct Table {
	CommonHeader;
	lu_byte flags;      // 1<<p means the tagmethod p is not present
	lu_byte prot;       // PopLua protection flags
	lu_byte lsizenode;  // log2 of the size of `node'
	Table* metatable;
	TObject* array;
	Node* node;
	Node* lastfree;     // any free position is before this position
	GCObject* gclist;
	int sizearray;
	Table* parent;      // PopLua: the class chain for string keys
};

#define lmod(s, size) ((int)((s) & ((size) - 1)))
#define twoto(x) (1 << (x))
#define sizenode(t) (twoto((t)->lsizenode))
#define gnode(t, i) (&(t)->node[i])
#define gkey(n) (&(n)->i_key)
#define gval(n) (&(n)->i_val)

#define sizeclosure_c(n) (sizeof(CClosure) + sizeof(TObject) * ((n) - 1))
#define sizeclosure_l(n) (sizeof(LClosure) + sizeof(TObject*) * ((n) - 1))

extern const TObject luaO_nilobject;

// ---- the state --------------------------------------------------------------------------------------

struct stringtable {
	GCObject** hash;
	lu_hash nuse;
	int size;
};

#define CI_C 1          // call is running a C function
#define CI_HASFRAME 2   // call is on a static frame of luaV_execute
#define CI_CALLING 4    // call is running a Lua function
#define CI_SAVEDPC 8    // `savedpc' is valid
#define CI_YIELD 16

struct CallInfo {
	StkId base;
	StkId top;
	int state;
	const Instruction* savedpc;
	const Instruction** pc;
	int tailcalls;
};

#define ci_func(ci) (clvalue((ci)->base - 1))

// tag methods (PopLua's list: MOD and SIZ inserted)
enum TMS {
	TM_INDEX, TM_NEWINDEX, TM_GC, TM_MODE, TM_EQ,
	TM_ADD, TM_SUB, TM_MUL, TM_DIV, TM_POW, TM_MOD, TM_UNM, TM_SIZ,
	TM_LT, TM_LE, TM_CONCAT, TM_CALL,
	TM_N
};

struct global_State {
	stringtable strt;
	GCObject* rootgc;     // every collectable object but the strings
	GCObject* rootudata;  // userdata with finalizers waiting to be collected
	GCObject* tmudata;    // userdata to be finalized
	char* buff;           // temporary buffer for string concatenation
	size_t buffsize;
	lu_mem GCthreshold;
	lu_mem nblocks;       // number of `bytes' currently allocated
	int gcstepmul;        // collectgarbage("setstepmul") (kept for the scripts; the collector is not incremental)
	lua_CFunction panic;
	TObject _registry;
	TObject _defaultmeta;
	lua_State* mainthread;
	Node dummynode[1];
	TString* tmname[TM_N];
	Table* mt[LUA_TTHREAD + 1];  // metatables of the basic types
};

struct lua_longjmp;

struct lua_State {
	CommonHeader;
	StkId top;
	StkId base;
	global_State* l_G;
	CallInfo* ci;
	StkId stack_last;
	StkId stack;
	int stacksize;
	CallInfo* end_ci;
	CallInfo* base_ci;
	unsigned short size_ci;
	unsigned short nCcalls;
	lu_byte allowhook;
	GCObject* openupval;
	lua_longjmp* errorJmp;
	ptrdiff_t errfunc;
	TObject _gt;
};

#define G(L) (L->l_G)
#define gt(L) (&L->_gt)
#define registry(L) (&G(L)->_registry)

union GCObject {
	GCheader gch;
	TString ts;
	Udata u;
	Closure cl;
	Table h;
	Proto p;
	UpVal uv;
	lua_State th;
};

#define gcotots(o) (&(o)->ts)
#define gcotou(o) (&(o)->u)
#define gcotocl(o) (&(o)->cl)
#define gcotoh(o) (&(o)->h)
#define gcotop(o) (&(o)->p)
#define gcotouv(o) (&(o)->uv)
#define ngcotouv(o) gcotouv(o)
#define gcototh(o) (&(o)->th)
#define valtogco(v) ((GCObject*)(v))

#define LUA_TPROTO (LUA_TMETHOD + 1)
#define LUA_TUPVAL (LUA_TMETHOD + 2)

// ---- memory (lmem) ----------------------------------------------------------------------------------

void* luaM_realloc(lua_State* L, void* oldblock, lu_mem oldsize, lu_mem size);
void* luaM_growaux(lua_State* L, void* block, int* size, int size_elem, int limit, const char* errormsg);
#define luaM_free(L, b, s) luaM_realloc(L, (b), (s), 0)
#define luaM_freelem(L, b) luaM_realloc(L, (b), sizeof(*(b)), 0)
#define luaM_freearray(L, b, n, t) luaM_realloc(L, (b), (lu_mem)(n) * (lu_mem)sizeof(t), 0)
#define luaM_malloc(L, t) luaM_realloc(L, NULL, 0, (t))
#define luaM_new(L, t) ((t*)luaM_malloc(L, sizeof(t)))
#define luaM_newvector(L, n, t) ((t*)luaM_malloc(L, (lu_mem)(n) * (lu_mem)sizeof(t)))
#define luaM_growvector(L, v, nelems, size, t, limit, e) \
	if (((nelems) + 1) > (size)) ((v) = (t*)luaM_growaux(L, v, &(size), sizeof(t), limit, e))
#define luaM_reallocvector(L, v, oldn, n, t) \
	((v) = (t*)luaM_realloc(L, v, (lu_mem)(oldn) * (lu_mem)sizeof(t), (lu_mem)(n) * (lu_mem)sizeof(t)))

// ---- objects (lobject) ------------------------------------------------------------------------------

int luaO_log2(unsigned int x);
int luaO_int2fb(unsigned int x);
#define fb2int(x) (((x) & 7) << ((x) >> 3))
int luaO_rawequalObj(const TObject* t1, const TObject* t2);
int luaO_str2d(const char* s, lua_Number* result);
int luaO_str2int(const char* s, lua_Int* result);
const char* luaO_pushvfstring(lua_State* L, const char* fmt, va_list argp);
const char* luaO_pushfstring(lua_State* L, const char* fmt, ...);
void luaO_chunkid(char* out, const char* source, int len);

// ---- strings (lstring) ------------------------------------------------------------------------------

#define sizestring(l) (sizeof(TString) + ((lu_mem)(l) + 1) * sizeof(char))
#define sizeudata(l) (sizeof(Udata) + (l))
#define luaS_new(L, s) (luaS_newlstr(L, s, strlen(s)))
#define luaS_newliteral(L, s) (luaS_newlstr(L, "" s, (sizeof(s) / sizeof(char)) - 1))
#define luaS_fix(s) ((s)->marked |= (1 << 4))
void luaS_resize(lua_State* L, int newsize);
Udata* luaS_newudata(lua_State* L, size_t s);
void luaS_freeall(lua_State* L);
TString* luaS_newlstr(lua_State* L, const char* str, size_t l);
lu_hash luaS_hash(const char* str, size_t l);

// ---- tables (ltable) --------------------------------------------------------------------------------

const TObject* luaH_getnum(Table* t, lua_Int key);
TObject* luaH_setnum(lua_State* L, Table* t, lua_Int key);
const TObject* luaH_getstr(Table* t, TString* key);
const TObject* luaH_getstr_f(Table* t, TString* key, int* flags);
const TObject* luaH_get(Table* t, const TObject* key);
const TObject* luaH_get_f(Table* t, const TObject* key, int* flags);
TObject* luaH_set(lua_State* L, Table* t, const TObject* key);
TObject* luaH_set_f(lua_State* L, Table* t, const TObject* key, int flags);
Node* luaH_getnode(Table* t, const TObject* key);  // the node holding key (NULL if absent or in the array)
Table* luaH_new(lua_State* L, int narray, int lnhash);
void luaH_free(lua_State* L, Table* t);
int luaH_next(lua_State* L, Table* t, StkId key);
int luaH_getn(Table* t);
Node* luaH_mainposition(const Table* t, const TObject* key);

// ---- functions (lfunc) ------------------------------------------------------------------------------

Proto* luaF_newproto(lua_State* L);
Closure* luaF_newCclosure(lua_State* L, int nelems);
Closure* luaF_newLclosure(lua_State* L, int nelems, TObject* e);
UpVal* luaF_findupval(lua_State* L, StkId level);
void luaF_close(lua_State* L, StkId level);
void luaF_freeproto(lua_State* L, Proto* f);
void luaF_freeclosure(lua_State* L, Closure* c);
const char* luaF_getlocalname(const Proto* func, int local_number, int pc);

// ---- garbage collector (lgc) ------------------------------------------------------------------------

#define luaC_checkGC(L) { if (G(L)->nblocks >= G(L)->GCthreshold) luaC_collectgarbage(L); }
void luaC_separateudata(lua_State* L);
void luaC_callGCTM(lua_State* L);
void luaC_sweep(lua_State* L, int all);
void luaC_collectgarbage(lua_State* L);
void luaC_link(lua_State* L, GCObject* o, lu_byte tt);

// ---- tag methods (ltm) ------------------------------------------------------------------------------

#define gfasttm(g, et, e) (((et) == NULL) ? NULL : ((et)->flags & (1u << (e))) ? NULL : luaT_gettm(et, e, (g)->tmname[e]))
#define fasttm(l, et, e) gfasttm(G(l), et, e)
const TObject* luaT_gettm(Table* events, TMS event, TString* ename);
const TObject* luaT_gettmbyobj(lua_State* L, const TObject* o, TMS event);
void luaT_init(lua_State* L);
extern const char* const luaT_typenames[];

// ---- calls and errors (ldo) -------------------------------------------------------------------------

#define luaD_checkstack(L, n) \
	if ((char*)L->stack_last - (char*)L->top <= (n) * (int)sizeof(TObject)) luaD_growstack(L, n);
#define incr_top(L) { luaD_checkstack(L, 1); L->top++; }
#define savestack(L, p) ((char*)(p) - (char*)L->stack)
#define restorestack(L, n) ((TObject*)((char*)L->stack + (n)))
#define saveci(L, p) ((char*)(p) - (char*)L->base_ci)
#define restoreci(L, n) ((CallInfo*)((char*)L->base_ci + (n)))

typedef void (*Pfunc)(lua_State* L, void* ud);
void luaD_resetprotection(lua_State* L);
int luaD_protectedparser(lua_State* L, void* z, int bin);
void luaD_callhook(lua_State* L, int event, int line);
StkId luaD_precall(lua_State* L, StkId func);
void luaD_call(lua_State* L, StkId func, int nResults);
int luaD_pcall(lua_State* L, Pfunc func, void* u, ptrdiff_t oldtop, ptrdiff_t ef);
void luaD_poscall(lua_State* L, int wanted, StkId firstResult);
void luaD_reallocCI(lua_State* L, int newsize);
void luaD_reallocstack(lua_State* L, int newsize);
void luaD_growstack(lua_State* L, int n);
[[noreturn]] void luaD_throw(lua_State* L, int errcode);
int luaD_rawrunprotected(lua_State* L, Pfunc f, void* ud);

// ---- the virtual machine (lvm) ----------------------------------------------------------------------

#define tostring(L, o) ((ttype(o) == LUA_TSTRING) || (luaV_tostring(L, o)))
#define tonumber(o, n) (ttype(o) == LUA_TNUMBER || (((o) = luaV_tonumber(o, n)) != NULL))
#define equalobj(L, o1, o2) ((ttype(o1) == ttype(o2) || (ttisnumber(o1) && ttisint(o2)) || (ttisint(o1) && ttisnumber(o2))) && luaV_equalval(L, o1, o2))
lua_Int luaV_ftol(lua_Number d);  // _ftol2_sse: truncation, INT_MIN when out of range
int luaV_lessthan(lua_State* L, const TObject* l, const TObject* r);
int luaV_equalval(lua_State* L, const TObject* t1, const TObject* t2);
const TObject* luaV_tonumber(const TObject* obj, TObject* n);
const TObject* luaV_toint(const TObject* obj, TObject* n);
int luaV_tostring(lua_State* L, StkId obj);
const TObject* luaV_gettable(lua_State* L, const TObject* t, TObject* key, int loop);
const TObject* luaV_gettable_f(lua_State* L, const TObject* t, TObject* key, int loop, int* flags);
void luaV_settable(lua_State* L, const TObject* t, TObject* key, StkId val);
StkId luaV_execute(lua_State* L);
void luaV_concat(lua_State* L, int total, int last);
void luaV_defset(lua_State* L, const TObject* t, TObject* key, const TObject* val, int flags);

// ---- debug and errors (ldebug) ----------------------------------------------------------------------

#define pcRel(pc, p) ((int)((pc) - (p)->code) - 1)
#define getfuncline(f, pc) (((f)->lineinfo) ? (f)->lineinfo[pc] : 0)
[[noreturn]] void luaG_typeerror(lua_State* L, const TObject* o, const char* opname);
[[noreturn]] void luaG_concaterror(lua_State* L, StkId p1, StkId p2);
[[noreturn]] void luaG_aritherror(lua_State* L, const TObject* p1, const TObject* p2);
[[noreturn]] void luaG_ordererror(lua_State* L, const TObject* p1, const TObject* p2);
[[noreturn]] void luaG_runerror(lua_State* L, const char* fmt, ...);
[[noreturn]] void luaG_errormsg(lua_State* L);
int luaG_checkcode(const Proto* pt);
int luaG_currentline(lua_State* L, CallInfo* ci);

// ---- loading (lundump) ------------------------------------------------------------------------------

struct ZIO {
	size_t n;           // bytes still unread
	const char* p;      // current position in buffer
	lua_Chunkreader reader;
	void* data;
	const char* name;
};
#define EOZ (-1)
#define zgetc(z) (((z)->n--) > 0 ? (int)((unsigned char)(*(z)->p++)) : luaZ_fill(z))
#define zname(z) ((z)->name)
void luaZ_init(ZIO* z, lua_Chunkreader reader, void* data, const char* name);
size_t luaZ_read(ZIO* z, void* b, size_t n);
int luaZ_lookahead(ZIO* z);
int luaZ_fill(ZIO* z);

Proto* luaU_undump(lua_State* L, ZIO* Z);
// 0x825e10: while luaL_loadfile reads a file, its full path; every function undumped then takes "@" and the
// path as its source in place of the one compiled into it
extern const char* luaU_loadname;
#define LUA_SIGNATURE "\033Lua"
#define VERSION 0x56

// ---- the compiler (lparser) -------------------------------------------------------------------------

Proto* luaY_parser(lua_State* L, ZIO* z);
// 0x71a8a0: enters the 32 reserved words in the string table (luaY_parser does it itself when it was not done)
void luaX_init(lua_State* L);
// the first line number of the next chunk compiled is luaX_linebase + 1 (the exe keeps it in lua_State+0x60: the
// in-game console sets it around a compile, 0x6d73b0)
extern int luaX_linebase;
// the debugger's expression compiler (0x6db610 sets DAT_00825e24/28 around a compile): while luaY_evalproto is set,
// the chunk compiled sees the locals of that function that are active at luaY_evalpc as its own first registers
// (and its upvalues by name); the caller supplies the values
extern const Proto* luaY_evalproto;
extern const Instruction* luaY_evalpc;
// ldump.cpp: saves a function in the chunk format lundump.cpp reads (the exe has no dumper)
int luaU_dump(lua_State* L, const Proto* Main, lua_Chunkwriter w, void* data);
int lua_dump(lua_State* L, lua_Chunkwriter writer, void* data);

// ---- the state (lstate) -----------------------------------------------------------------------------

void luaE_freethread(lua_State* L, lua_State* L1);

#endif
