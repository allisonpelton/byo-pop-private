// Saving a compiled function: Lua 5.0.2's ldump.c in PopLua's chunk format, the inverse of lundump.cpp (the exe has
// no dumper; the port writes one so that the compiler's output can be compared with PopCap's compiled scripts).
#include "lcore.h"
#include "lopcodes.h"

struct DumpState {
	lua_State* L;
	lua_Chunkwriter write;
	void* data;
	int status;
};

static void DumpBlock(const void* b, size_t size, DumpState* D)
{
	if (D->status == 0)
		D->status = D->write(D->L, b, size, D->data);
}

static void DumpByte(int y, DumpState* D)
{
	char x = (char)y;
	DumpBlock(&x, 1, D);
}

static void DumpInt(int x, DumpState* D)
{
	int32_t v = (int32_t)x;
	DumpBlock(&v, sizeof v, D);
}

static void DumpSize(size_t x, DumpState* D)
{
	uint32_t v = (uint32_t)x;
	DumpBlock(&v, sizeof v, D);
}

static void DumpNumber(lua_Number x, DumpState* D)
{
	DumpBlock(&x, sizeof x, D);
}

static void DumpString(const TString* s, DumpState* D)
{
	if (s == NULL)
		DumpSize(0, D);
	else {
		size_t size = s->len + 1;  // include trailing '\0'
		DumpSize(size, D);
		DumpBlock(getstr(s), size, D);
	}
}

static void DumpFunction(const Proto* f, const TString* p, DumpState* D);

static void DumpConstants(const Proto* f, DumpState* D)
{
	int i, n = f->sizek;
	DumpInt(n, D);
	for (i = 0; i < n; i++) {
		const TObject* o = &f->k[i];
		DumpByte(ttype(o), D);
		switch (ttype(o)) {
		case LUA_TNUMBER:
			DumpNumber(nvalue(o), D);
			break;
		case LUA_TINT:
			DumpInt(ivalue(o), D);
			break;
		case LUA_TSTRING:
			DumpString(tsvalue(o), D);
			break;
		case LUA_TBOOLEAN:
			DumpByte(bvalue(o), D);
			break;
		case LUA_TNIL:
			break;
		}
	}
	n = f->sizep;
	DumpInt(n, D);
	for (i = 0; i < n; i++)
		DumpFunction(f->p[i], f->source, D);
}

static void DumpFunction(const Proto* f, const TString* p, DumpState* D)
{
	int i, n;
	DumpString(f->source == p ? NULL : f->source, D);
	DumpInt(f->lineDefined, D);
	DumpInt(f->selfflag, D);
	DumpByte(f->nups, D);
	DumpByte(f->numparams, D);
	DumpByte(f->is_vararg, D);
	DumpByte(f->maxstacksize, D);
	n = f->sizelineinfo;
	DumpInt(n, D);
	DumpBlock(f->lineinfo, n * sizeof(int), D);
	n = f->sizelocvars;
	DumpInt(n, D);
	for (i = 0; i < n; i++) {
		DumpString(f->locvars[i].varname, D);
		DumpInt(f->locvars[i].startpc, D);
		DumpInt(f->locvars[i].endpc, D);
	}
	n = f->sizeupvalues;
	DumpInt(n, D);
	for (i = 0; i < n; i++)
		DumpString(f->upvalues[i], D);
	DumpConstants(f, D);
	n = f->sizecode;
	DumpInt(n, D);
	DumpBlock(f->code, n * sizeof(Instruction), D);
}

static void DumpHeader(DumpState* D)
{
	DumpBlock(LUA_SIGNATURE, sizeof(LUA_SIGNATURE) - 1, D);
	DumpByte(VERSION, D);
	DumpByte(1, D);  // little-endian
	DumpByte(1, D);  // chunk format
	DumpByte(4, D);  // sizeof(int)
	DumpByte(4, D);  // sizeof(size_t)
	DumpByte(4, D);  // sizeof(Instruction)
	DumpByte(SIZE_OP, D);
	DumpByte(SIZE_A, D);
	DumpByte(SIZE_B, D);
	DumpByte(SIZE_C, D);
	DumpByte(8, D);  // sizeof(lua_Number)
	DumpNumber(31415926.0, D);  // to check the number format
}

// dump a function as a precompiled chunk; returns the writer's last status (0 = ok)
int luaU_dump(lua_State* L, const Proto* Main, lua_Chunkwriter w, void* data)
{
	DumpState D;
	D.L = L;
	D.write = w;
	D.data = data;
	D.status = 0;
	DumpHeader(&D);
	DumpFunction(Main, NULL, &D);
	return D.status;
}

// the function on the top of the stack, as a precompiled chunk (Lua 5.0.2's lua_dump); 1 if it is not a Lua function
int lua_dump(lua_State* L, lua_Chunkwriter writer, void* data)
{
	const TObject* o = L->top - 1;
	if (!isLfunction(o))
		return 1;
	return luaU_dump(L, clvalue(o)->l.p, writer, data);
}
