// Loading precompiled chunks (0x71c4c0-0x71d1f0): Lua 5.0.2's lundump.c with PopLua's format, which adds a
// byte to the header (it must be 1), an int after each function's line (the `self' flag of methods), the int
// constant type (tag 4) and the 49 opcodes. Only little-endian chunks are read (all the game's are).
#include <cstdio>

#include "lcore.h"
#include "lopcodes.h"

const char* luaU_loadname = NULL;

struct LoadState {
	lua_State* L;
	ZIO* Z;
	const char* name;
};

[[noreturn]] static void unexpectedEOZ(LoadState* S)
{
	luaG_runerror(S->L, "unexpected end of file in %s", S->name);
}

static int ezgetc(LoadState* S)
{
	int c = zgetc(S->Z);
	if (c == EOZ)
		unexpectedEOZ(S);
	return c;
}

static void ezread(LoadState* S, void* b, size_t n)
{
	size_t r = luaZ_read(S->Z, b, n);
	if (r != 0)
		unexpectedEOZ(S);
}

static int LoadInt(LoadState* S)
{
	int32_t x;
	ezread(S, &x, sizeof(x));
	return (int)x;
}

static size_t LoadSize(LoadState* S)
{
	uint32_t x;
	ezread(S, &x, sizeof(x));
	return x;
}

static lua_Number LoadNumber(LoadState* S)
{
	lua_Number x;
	ezread(S, &x, sizeof(x));
	return x;
}

static TString* LoadString(LoadState* S)
{
	size_t size = LoadSize(S);
	if (size == 0)
		return NULL;
	else {
		if (size > G(S->L)->buffsize) {
			G(S->L)->buff = (char*)luaM_realloc(S->L, G(S->L)->buff, G(S->L)->buffsize, size);
			G(S->L)->buffsize = size;
		}
		char* s = G(S->L)->buff;
		ezread(S, s, size);
		return luaS_newlstr(S->L, s, size - 1);  // remove trailing '\0'
	}
}

static void LoadCode(LoadState* S, Proto* f)
{
	int size = LoadInt(S);
	f->code = luaM_newvector(S->L, size, Instruction);
	f->sizecode = size;
	ezread(S, f->code, size * sizeof(Instruction));
}

static void LoadLocals(LoadState* S, Proto* f)
{
	int i, n;
	n = LoadInt(S);
	f->locvars = luaM_newvector(S->L, n, LocVar);
	f->sizelocvars = n;
	for (i = 0; i < n; i++)
		f->locvars[i].varname = NULL;
	for (i = 0; i < n; i++) {
		f->locvars[i].varname = LoadString(S);
		f->locvars[i].startpc = LoadInt(S);
		f->locvars[i].endpc = LoadInt(S);
	}
}

static void LoadLines(LoadState* S, Proto* f)
{
	int size = LoadInt(S);
	f->lineinfo = luaM_newvector(S->L, size, int);
	f->sizelineinfo = size;
	ezread(S, f->lineinfo, size * sizeof(int));
}

static void LoadUpvalues(LoadState* S, Proto* f)
{
	int i, n;
	n = LoadInt(S);
	if (n != 0 && n != f->nups)
		luaG_runerror(S->L, "bad nupvalues in %s: read %d; expected %d", S->name, n, f->nups);
	f->upvalues = luaM_newvector(S->L, n, TString*);
	f->sizeupvalues = n;
	for (i = 0; i < n; i++)
		f->upvalues[i] = NULL;
	for (i = 0; i < n; i++)
		f->upvalues[i] = LoadString(S);
}

static Proto* LoadFunction(LoadState* S, TString* p);

static void LoadConstants(LoadState* S, Proto* f)
{
	int i, n;
	n = LoadInt(S);
	f->k = luaM_newvector(S->L, n, TObject);
	f->sizek = n;
	for (i = 0; i < n; i++)
		setnilvalue(&f->k[i]);
	for (i = 0; i < n; i++) {
		TObject* o = &f->k[i];
		int t = ezgetc(S);
		switch (t) {
		case LUA_TNUMBER:
			setnvalue(o, LoadNumber(S));
			break;
		case LUA_TINT:
			setivalue(o, LoadInt(S));
			break;
		case LUA_TSTRING:
			setsvalue2n(o, LoadString(S));
			break;
		case LUA_TBOOLEAN:
			setbvalue(o, ezgetc(S));
			break;
		case LUA_TNIL:
			setnilvalue(o);
			break;
		default:
			luaG_runerror(S->L, "bad constant type (%d) in %s", t, S->name);
		}
	}
	n = LoadInt(S);
	f->p = luaM_newvector(S->L, n, Proto*);
	f->sizep = n;
	for (i = 0; i < n; i++)
		f->p[i] = NULL;
	for (i = 0; i < n; i++)
		f->p[i] = LoadFunction(S, f->source);
}

static Proto* LoadFunction(LoadState* S, TString* p)
{
	Proto* f = luaF_newproto(S->L);
	// anchor the prototype on the stack while it is loaded (a collection may run meanwhile)
	setttype(S->L->top, LUA_TPROTO);
	S->L->top->value.gc = valtogco(f);
	incr_top(S->L);
	f->source = LoadString(S);
	if (f->source == NULL)
		f->source = p;
	if (luaU_loadname != NULL) {
		char source[264];
		snprintf(source, sizeof source, "@%s", luaU_loadname);
		f->source = luaS_new(S->L, source);
	}
	luaS_fix(f->source);
	f->lineDefined = LoadInt(S);
	f->selfflag = LoadInt(S);
	f->nups = (lu_byte)ezgetc(S);
	f->numparams = (lu_byte)ezgetc(S);
	f->is_vararg = (lu_byte)ezgetc(S);
	f->maxstacksize = (lu_byte)ezgetc(S);
	LoadLines(S, f);
	LoadLocals(S, f);
	LoadUpvalues(S, f);
	LoadConstants(S, f);
	LoadCode(S, f);
	if (!luaG_checkcode(f))
		luaG_runerror(S->L, "bad code in %s", S->name);
	S->L->top--;
	return f;
}

static void TestSize(LoadState* S, int s, const char* what)
{
	int r = ezgetc(S);
	if (r != s)
		luaG_runerror(S->L, "virtual machine mismatch in %s: size of %s is %d but read %d", S->name, what, s, r);
}

static void LoadHeader(LoadState* S)
{
	char s[sizeof(LUA_SIGNATURE)];
	lua_Number x;
	ezread(S, s, sizeof(LUA_SIGNATURE) - 1);
	if (memcmp(s, LUA_SIGNATURE, sizeof(LUA_SIGNATURE) - 1) != 0)
		luaG_runerror(S->L, "bad signature in %s", S->name);
	if (ezgetc(S) != VERSION)
		luaG_runerror(S->L, "%s not a PopLua chunk", S->name);
	if (ezgetc(S) != 1)
		luaG_runerror(S->L, "%s is not little-endian", S->name);
	if (ezgetc(S) != 1)
		luaG_runerror(S->L, "unknown chunk format in %s", S->name);
	TestSize(S, 4, "int");
	TestSize(S, 4, "size_t");
	TestSize(S, 4, "Instruction");
	TestSize(S, SIZE_OP, "OP");
	TestSize(S, SIZE_A, "A");
	TestSize(S, SIZE_B, "B");
	TestSize(S, SIZE_C, "C");
	TestSize(S, 8, "number");
	x = LoadNumber(S);
	if ((long)x != (long)(lua_Number)31415926)
		luaG_runerror(S->L, "unknown number format in %s", S->name);
}

// load precompiled chunk
Proto* luaU_undump(lua_State* L, ZIO* Z)
{
	LoadState S;
	const char* s = zname(Z);
	if (*s == '@' || *s == '=')
		S.name = s + 1;
	else if (*s == LUA_SIGNATURE[0])
		S.name = "binary string";
	else
		S.name = s;
	S.L = L;
	S.Z = Z;
	LoadHeader(&S);
	return LoadFunction(&S, NULL);
}
