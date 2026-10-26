// Tables (0x702830-0x703b00). PopLua's tables are Lua 5.1's ltable.c inside a 5.0 VM, changed so that
//  - arrays are 0-based: the array part holds the int keys 0 .. sizearray-1, and only LUA_TINT keys go there
//    (a number key, even an integral one, is hashed as a number and is a different key from the int);
//  - each node has flags: NODE_DEFINED keeps a key whose value is nil (DEFGLOBAL/DEFTABLE), and such a node is
//    "used" (next() returns it, a rehash keeps it, a lookup finds it);
//  - string keys compare case-insensitively unless the table has PROT_CASESENSITIVE (the string hash is
//    case-insensitive, lstring.cpp); the original caches a matched pair in TString::alias, kept here so that
//    a lookup finds the very node the original found;
//  - the class parent link (Table::parent) is searched by the VM (lvm.cpp), not here.
// Iteration order depends only on the sizes, the main positions and the free-position scan, which follow
// the original exactly, so `pairs' visits keys in the original's order (except keys hashed by address).
#include <cctype>

#include "lcore.h"

#define hashpow2(t, n) (gnode(t, lmod((n), sizenode(t))))
#define hashstr(t, str) hashpow2(t, (str)->hash)
#define hashboolean(t, p) hashpow2(t, p)
// for some types, it is better to avoid modulus by power of 2, as they tend to have many 2 factors
#define hashmod(t, n) (gnode(t, ((n) % ((sizenode(t) - 1) | 1))))
#define hashpointer(t, p) hashmod(t, (lu_hash)(size_t)(p))

#define dummynode (&dummynode_)
static Node dummynode_ = {{LUA_TNIL, {NULL}}, {LUA_TNIL, {NULL}}, 0, NULL};

// 0x7027c0: n + 1 normalizes -0; the two halves of the double are added
static Node* hashnum(const Table* t, lua_Number n)
{
	uint32_t a[2];
	n += 1;
	memcpy(a, &n, sizeof(a));
	a[0] += a[1];
	return hashmod(t, (lu_hash)a[0]);
}

// returns the `main' position of an element in a table (that is, the index of its hash value)
Node* luaH_mainposition(const Table* t, const TObject* key)
{
	switch (ttype(key)) {
	case LUA_TBOOLEAN:
		return hashboolean(t, (lu_hash)bvalue(key));
	case LUA_TLIGHTUSERDATA:
		return hashpointer(t, pvalue(key));
	case LUA_TNUMBER:
		return hashnum(t, nvalue(key));
	case LUA_TINT:
		return hashmod(t, (lu_hash)(ivalue(key) - 1));
	case LUA_TSTRING:
		return hashstr(t, tsvalue(key));
	default:
		return hashpointer(t, gcvalue(key));
	}
}

// returns the index for `key' if `key' is an appropriate key to live in the array part of the table, -1
// otherwise
static int arrayindex(const TObject* key)
{
	if (ttisint(key)) {
		lua_Int k = ivalue(key);
		if (k >= 0 && (k & 0xff000000) == 0)
			return k;
	}
	return -1;
}

static const TObject* getgeneric(Table* t, const TObject* key);

// returns the index of a `key' for table traversals: first the array part (index = key), then the hash part
// (index = sizearray + node number)
static int luaH_index(lua_State* L, Table* t, StkId key)
{
	int i;
	if (ttisnil(key))
		return -1;  // first iteration
	i = arrayindex(key);
	if (0 <= i && i < t->sizearray)
		return i;
	else {
		const TObject* v = luaH_get(t, key);
		if (v == &luaO_nilobject)
			luaG_runerror(L, "invalid key for `next'");
		i = (int)(((const lu_byte*)v - (const lu_byte*)gval(gnode(t, 0))) / sizeof(Node));
		return i + t->sizearray;  // hash elements are numbered after array ones
	}
}

#define nodeused(n) (!ttisnil(gval(n)) || ((n)->flags & NODE_DEFINED))

int luaH_next(lua_State* L, Table* t, StkId key)
{
	int i = luaH_index(L, t, key);  // find original element
	for (i++; i < t->sizearray; i++) {  // try first array part
		if (!ttisnil(&t->array[i])) {  // a non-nil value?
			setivalue(key, i);
			setobj2s(key + 1, &t->array[i]);
			return 1;
		}
	}
	for (i -= t->sizearray; i < sizenode(t); i++) {  // then hash part
		if (nodeused(gnode(t, i))) {
			setobj2s(key, gkey(gnode(t, i)));
			setobj2s(key + 1, gval(gnode(t, i)));
			return 1;
		}
	}
	return 0;  // no more elements
}

// ---- rehash ------------------------------------------------------------------------------------------

static int computesizes(int nums[], int* narray)
{
	int i;
	int twotoi;  // 2^i
	int a = 0;   // number of elements smaller than 2^i
	int na = 0;  // number of elements to go to array part
	int n = 0;   // optimal size for array part
	for (i = 0, twotoi = 1; twotoi / 2 < *narray; i++, twotoi *= 2) {
		if (nums[i] > 0) {
			a += nums[i];
			if (a > twotoi / 2) {  // more than half elements present?
				n = twotoi;  // optimal size (till now)
				na = a;      // all elements smaller than n will go to array part
			}
		}
		if (a == *narray)
			break;  // all elements already counted
	}
	*narray = n;
	return na;
}

// key k (0-based) counts in slice luaO_log2(k)+1, the 1-based index k+1's slice in 5.1
static int countint(const TObject* key, int* nums)
{
	int k = arrayindex(key);
	if (0 <= k && k < MAXASIZE) {  // is `key' an appropriate array index?
		nums[luaO_log2((unsigned int)k) + 1]++;  // count as such
		return 1;
	}
	else
		return 0;
}

static int numusearray(const Table* t, int* nums)
{
	int lg;
	int ttlg;      // 2^lg
	int ause = 0;  // summation of `nums'
	int i = 0;     // count to traverse all array keys
	for (lg = 0, ttlg = 1; lg <= MAXBITS; lg++, ttlg *= 2) {  // for each slice
		int lc = 0;  // counter
		int lim = ttlg;
		if (lim > t->sizearray) {
			lim = t->sizearray;  // adjust upper limit
			if (i > lim)
				break;  // no more elements to count
		}
		// count elements in range [2^(lg-1), 2^lg)
		for (; i < lim; i++) {
			if (!ttisnil(&t->array[i]))
				lc++;
		}
		nums[lg] += lc;
		ause += lc;
	}
	return ause;
}

static int numusehash(const Table* t, int* nums, int* pnasize)
{
	int totaluse = 0;  // total number of elements
	int ause = 0;      // summation of `nums'
	int i = sizenode(t);
	while (i--) {
		Node* n = &t->node[i];
		if (nodeused(n)) {
			ause += countint(gkey(n), nums);
			totaluse++;
		}
	}
	*pnasize += ause;
	return totaluse;
}

static void setarrayvector(lua_State* L, Table* t, int size)
{
	int i;
	luaM_reallocvector(L, t->array, t->sizearray, size, TObject);
	for (i = t->sizearray; i < size; i++)
		setnilvalue(&t->array[i]);
	t->sizearray = size;
}

static void setnodevector(lua_State* L, Table* t, int size)
{
	int lsize;
	if (size == 0) {  // no elements to hash part?
		t->node = dummynode;  // use common `dummynode'
		lsize = 0;
	}
	else {
		int i;
		lsize = luaO_log2((unsigned int)(size - 1)) + 1;  // ceillog2
		if (lsize > MAXBITS)
			luaG_runerror(L, "table overflow");
		size = twoto(lsize);
		t->node = luaM_newvector(L, size, Node);
		for (i = 0; i < size; i++) {
			Node* n = gnode(t, i);
			n->next = NULL;
			n->flags = 0;
			setnilvalue(gkey(n));
			setnilvalue(gval(n));
		}
	}
	t->lsizenode = (lu_byte)lsize;
	t->lastfree = gnode(t, size);  // all positions are free
}

static void resize(lua_State* L, Table* t, int nasize, int nhsize)
{
	int i;
	int oldasize = t->sizearray;
	int oldhsize = t->lsizenode;
	Node* nold = t->node;  // save old hash ...
	if (nasize > oldasize)  // array part must grow?
		setarrayvector(L, t, nasize);
	// create new hash part with appropriate size
	setnodevector(L, t, nhsize);
	if (nasize < oldasize) {  // array part must shrink?
		t->sizearray = nasize;
		// re-insert elements from vanishing slice
		for (i = nasize; i < oldasize; i++) {
			if (!ttisnil(&t->array[i]))
				setobjt2t(luaH_setnum(L, t, i), &t->array[i]);
		}
		// shrink array
		luaM_reallocvector(L, t->array, oldasize, nasize, TObject);
	}
	// re-insert elements from hash part, with their flags
	for (i = twoto(oldhsize) - 1; i >= 0; i--) {
		Node* old = nold + i;
		if (nodeused(old))
			setobjt2t(luaH_set_f(L, t, gkey(old), old->flags), gval(old));
	}
	if (nold != dummynode)
		luaM_freearray(L, nold, twoto(oldhsize), Node);  // free old array
}

static void rehash(lua_State* L, Table* t, const TObject* ek)
{
	int nasize, na;
	int nums[MAXBITS + 2];  // nums[i] = number of keys in slice i
	int i;
	int totaluse;
	for (i = 0; i <= MAXBITS + 1; i++)
		nums[i] = 0;  // reset counts
	nasize = numusearray(t, nums);  // count keys in array part
	totaluse = nasize;  // all those keys are integer keys
	totaluse += numusehash(t, nums, &nasize);  // count keys in hash part
	// count extra key
	nasize += countint(ek, nums);
	totaluse++;
	// compute new size for array part
	na = computesizes(nums, &nasize);
	// resize the table to new computed sizes
	resize(L, t, nasize, totaluse - na);
}

// ---- creation and freeing ------------------------------------------------------------------------------

Table* luaH_new(lua_State* L, int narray, int nhash)
{
	Table* t = luaM_new(L, Table);
	luaC_link(L, valtogco(t), LUA_TTABLE);
	t->metatable = NULL;
	t->flags = (lu_byte)(~0);
	t->prot = 0;
	t->parent = NULL;
	t->gclist = NULL;
	// temporary values (kept only if some malloc fails)
	t->array = NULL;
	t->sizearray = 0;
	t->lsizenode = 0;
	t->node = dummynode;
	setarrayvector(L, t, narray);
	setnodevector(L, t, nhash);
	return t;
}

void luaH_free(lua_State* L, Table* t)
{
	if (t->node != dummynode)
		luaM_freearray(L, t->node, sizenode(t), Node);
	luaM_freearray(L, t->array, t->sizearray, TObject);
	luaM_freelem(L, t);
}

static Node* getfreepos(Table* t)
{
	while (t->lastfree-- > t->node) {
		if (ttisnil(gkey(t->lastfree)))
			return t->lastfree;
	}
	return NULL;  // could not find a free place
}

// inserts a new key into a hash table; first, check whether key's main position is free. If not, check
// whether colliding node is in its main position or not: if it is not, move colliding node to an empty place
// and put new key in its main position; otherwise (colliding node is in its main position), new key goes to
// an empty position.
static TObject* newkey(lua_State* L, Table* t, const TObject* key, int flags)
{
	Node* mp = luaH_mainposition(t, key);
	if (nodeused(mp) || mp == dummynode) {
		Node* othern;
		Node* n = getfreepos(t);  // get a free place
		if (n == NULL) {  // cannot find a free place?
			rehash(L, t, key);  // grow table
			return luaH_set_f(L, t, key, flags);  // re-insert key into grown table
		}
		othern = luaH_mainposition(t, gkey(mp));
		if (othern != mp) {  // is colliding node out of its main position?
			// yes; move colliding node into free position
			while (othern->next != mp)
				othern = othern->next;  // find previous
			othern->next = n;  // redo the chain with `n' in place of `mp'
			*n = *mp;  // copy colliding node into free pos. (mp->next also goes)
			mp->next = NULL;  // now `mp' is free
			setnilvalue(gval(mp));
		}
		else {  // colliding node is in its own main position
			// new node will go into free position
			n->next = mp->next;  // chain new position
			mp->next = n;
			mp = n;
		}
	}
	setobj2t(gkey(mp), key);
	mp->flags = flags;
	return gval(mp);
}

// ---- lookups -----------------------------------------------------------------------------------------

// MSVC's _stricmp on C strings (ASCII)
static int stricmp_c(const char* a, const char* b)
{
	for (;;) {
		int ca = tolower((unsigned char)*a++);
		int cb = tolower((unsigned char)*b++);
		if (ca != cb || ca == 0)
			return ca - cb;
	}
}

static void setalias(TString* k, TString* key)
{
	if (k->alias != NULL)
		k->alias->alias = NULL;
	k->alias = key;
	if (key->alias != NULL)
		key->alias->alias = NULL;
	key->alias = k;
}

// 0x7035d0 (with flags) / 0x7034f0
const TObject* luaH_getstr_f(Table* t, TString* key, int* flags)
{
	Node* n = hashstr(t, key);
	do {  // check whether `key' is somewhere in the chain
		if (ttisstring(gkey(n))) {
			TString* k = tsvalue(gkey(n));
			if (k == key) {
				if (flags)
					*flags = n->flags;
				return gval(n);
			}
			if (!(t->prot & PROT_CASESENSITIVE)) {
				if (k->alias == key) {
					if (flags)
						*flags = n->flags;
					return gval(n);
				}
				if (nodeused(n) && k->hash == key->hash && stricmp_c(getstr(k), getstr(key)) == 0) {
					if (flags)
						*flags = n->flags;
					setalias(k, key);
					return gval(n);
				}
			}
		}
		n = n->next;
	} while (n);
	return &luaO_nilobject;
}

const TObject* luaH_getstr(Table* t, TString* key)
{
	return luaH_getstr_f(t, key, NULL);
}

// 0x7034a0
const TObject* luaH_getnum(Table* t, lua_Int key)
{
	if (key >= 0 && key < t->sizearray)
		return &t->array[key];
	else {
		Node* n = hashmod(t, (lu_hash)(key - 1));
		do {  // check whether `key' is somewhere in the chain
			if (ttisint(gkey(n)) && ivalue(gkey(n)) == key)
				return gval(n);  // that's it
			else
				n = n->next;
		} while (n);
		return &luaO_nilobject;
	}
}

// 0x703430: any other key type (nil never matches)
static const TObject* getgeneric(Table* t, const TObject* key)
{
	if (ttisnil(key))
		return &luaO_nilobject;
	Node* n = luaH_mainposition(t, key);
	do {  // check whether `key' is somewhere in the chain
		if (ttype(gkey(n)) == ttype(key)) {
			if (ttisnumber(key) ? nvalue(gkey(n)) == nvalue(key)
			                    : (ttisboolean(key) ? bvalue(gkey(n)) == bvalue(key)
			                                        : (ttislightuserdata(key) ? pvalue(gkey(n)) == pvalue(key)
			                                                                  : gcvalue(gkey(n)) == gcvalue(key))))
				return gval(n);  // that's it
		}
		n = n->next;
	} while (n);
	return &luaO_nilobject;
}

// 0x703590
const TObject* luaH_get(Table* t, const TObject* key)
{
	switch (ttype(key)) {
	case LUA_TINT:
		return luaH_getnum(t, ivalue(key));
	case LUA_TSTRING:
		return luaH_getstr(t, tsvalue(key));
	default:
		return getgeneric(t, key);
	}
}

// 0x7036d0: flags are reported for string keys only
const TObject* luaH_get_f(Table* t, const TObject* key, int* flags)
{
	if (ttisstring(key))
		return luaH_getstr_f(t, tsvalue(key), flags);
	if (flags)
		*flags = 0;
	return luaH_get(t, key);
}

Node* luaH_getnode(Table* t, const TObject* key)
{
	const TObject* v = luaH_get(t, key);
	if (v == &luaO_nilobject || (v >= t->array && v < t->array + t->sizearray))
		return NULL;
	return (Node*)((const char*)v - offsetof(Node, i_val));
}

// 0x7038c0: an existing key keeps its flags
TObject* luaH_set_f(lua_State* L, Table* t, const TObject* key, int flags)
{
	const TObject* p = luaH_get(t, key);
	t->flags = 0;
	if (p != &luaO_nilobject)
		return (TObject*)p;
	else {
		if (ttisnil(key))
			luaG_runerror(L, "table index is nil");
		else if (ttisnumber(key) && nvalue(key) != nvalue(key))
			luaG_runerror(L, "table index is NaN");
		return newkey(L, t, key, flags);
	}
}

TObject* luaH_set(lua_State* L, Table* t, const TObject* key)
{
	return luaH_set_f(L, t, key, 0);
}

// 0x703960 (does not reset the metamethod cache)
TObject* luaH_setnum(lua_State* L, Table* t, lua_Int key)
{
	const TObject* p = luaH_getnum(t, key);
	if (p != &luaO_nilobject)
		return (TObject*)p;
	else {
		TObject k;
		setivalue(&k, key);
		return newkey(L, t, &k, 0);
	}
}

// 0x7039b0
static int unbound_search(Table* t, unsigned int j)
{
	unsigned int i = j - 1;  // i is a present key (the last of the array part)
	// find `i' and `j' such that i is present and j is not
	if (!ttisnil(luaH_getnum(t, (lua_Int)j))) {
		do {
			i = j;
			j *= 2;
			if (j > 0x7ffffffdu) {  // overflow? table was built with bad purposes: resort to linear search
				unsigned int k = 1;
				while (!ttisnil(luaH_getnum(t, (lua_Int)k)))
					k++;
				return (int)k;
			}
		} while (!ttisnil(luaH_getnum(t, (lua_Int)j)));
	}
	// now do a binary search between them
	while (i < j - 1) {
		unsigned int m = (i + j) / 2;
		if (ttisnil(luaH_getnum(t, (lua_Int)m)))
			j = m;
		else
			i = m;
	}
	return (int)(i + 1);
}

// 0x703a40: the `*t' operator (and table.getn): the number of elements of a 0-based sequence (5.1's border)
int luaH_getn(Table* t)
{
	unsigned int j = (unsigned int)t->sizearray;
	if (j > 0 && ttisnil(&t->array[j - 1])) {
		// there is a boundary in the array part: (binary) search for it
		unsigned int i = 0;
		while (j - i > 1) {
			unsigned int m = (i + j) / 2;
			if (ttisnil(&t->array[m - 1]))
				j = m;
			else
				i = m;
		}
		return (int)i;
	}
	// else must find a boundary in hash part
	else if (t->node == dummynode)  // hash part is empty?
		return (int)j;  // that is easy...
	else
		return unbound_search(t, j);
}
