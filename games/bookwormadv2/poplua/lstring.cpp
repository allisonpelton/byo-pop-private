// The string table (Lua 5.0.2's lstring.c). Strings are interned by their exact bytes, but PopLua's hash
// (0x719f60) folds the characters to upper case, so strings that differ only in case share a hash: table
// lookups (ltable.cpp) compare string keys case-insensitively.
#include "lcore.h"

void luaS_freeall(lua_State* L)
{
	luaM_freearray(L, G(L)->strt.hash, G(L)->strt.size, TString*);
}

void luaS_resize(lua_State* L, int newsize)
{
	GCObject** newhash = luaM_newvector(L, newsize, GCObject*);
	stringtable* tb = &G(L)->strt;
	int i;
	for (i = 0; i < newsize; i++)
		newhash[i] = NULL;
	// rehash
	for (i = 0; i < tb->size; i++) {
		GCObject* p = tb->hash[i];
		while (p) {  // for each node in the list
			GCObject* next = p->gch.next;  // save next
			lu_hash h = gcotots(p)->hash;
			int h1 = lmod(h, newsize);  // new position
			p->gch.next = newhash[h1];  // chain it
			newhash[h1] = p;
			p = next;
		}
	}
	luaM_freearray(L, tb->hash, tb->size, TString*);
	tb->size = newsize;
	tb->hash = newhash;
}

static TString* newlstr(lua_State* L, const char* str, size_t l, lu_hash h)
{
	TString* ts = (TString*)luaM_malloc(L, sizestring(l));
	stringtable* tb;
	ts->len = l;
	ts->hash = h;
	ts->marked = 0;
	ts->tt = LUA_TSTRING;
	ts->reserved = 0;
	ts->alias = NULL;
	memcpy(ts + 1, str, l * sizeof(char));
	((char*)(ts + 1))[l] = '\0';  // ending 0
	tb = &G(L)->strt;
	h = lmod(h, tb->size);
	ts->next = tb->hash[h];  // chain new entry
	tb->hash[h] = valtogco(ts);
	tb->nuse++;
	if (tb->nuse > (lu_hash)tb->size && tb->size <= MAX_INT / 2)
		luaS_resize(L, tb->size * 2);  // too crowded
	return ts;
}

lu_hash luaS_hash(const char* str, size_t l)
{
	lu_hash h = (lu_hash)l;  // seed
	size_t step = (l >> 5) + 1;  // if string is too long, don't hash all its chars
	size_t l1;
	for (l1 = l; l1 >= step; l1 -= step) {  // compute hash
		unsigned char c = (unsigned char)str[l1 - 1];
		if (c >= 'a' && c <= 'z')
			c = (unsigned char)(c - 'a' + 'A');  // the table at 0x79b560
		h = h ^ ((h << 5) + (h >> 2) + c);
	}
	return h;
}

TString* luaS_newlstr(lua_State* L, const char* str, size_t l)
{
	GCObject* o;
	lu_hash h = luaS_hash(str, l);
	for (o = G(L)->strt.hash[lmod(h, G(L)->strt.size)]; o != NULL; o = o->gch.next) {
		TString* ts = gcotots(o);
		if (ts->len == l && (memcmp(str, getstr(ts), l) == 0))
			return ts;
	}
	return newlstr(L, str, l, h);  // not found
}

Udata* luaS_newudata(lua_State* L, size_t s)
{
	Udata* u;
	u = (Udata*)luaM_malloc(L, sizeudata(s));
	u->marked = (1 << 1);  // is not finalized
	u->tt = LUA_TUSERDATA;
	u->len = s;
	u->metatable = NULL;
	// chain it on udata list
	u->next = G(L)->rootudata;
	G(L)->rootudata = valtogco(u);
	return u;
}
