// The compiler (0x70e430-0x714600): Lua 5.0.2's lparser.c with PopLua's dialect: C-style blocks (`{ }' or a single
// statement after if/while/for conditions, `then'/`do' ... `end' as before), `else if', `continue', `class',
// `def', `const', `super', `cobj', `Table', `__POS', compound assignments (`+=' ... and `++'/`--'), `for i < n',
// methods with an implicit `self' (GETSELFORGLOBAL / SELFORGLOBAL for the names in them) and the `FuncTable@SelfTable:Function'
// call. Every function says where it is in the exe; what the exe does differently from 5.0.2 is noted.
#include <cctype>
#include <cstdio>
#include <cstring>

#include "lparser.h"

// the debugger's expression compiler (0x6db610; DAT_00825e24 and DAT_00825e28 in the exe)
const Proto* luaY_evalproto = NULL;
const Instruction* luaY_evalpc = NULL;

#define getlocvar(fs, i) ((fs)->f->locvars[(fs)->actvar[i]])
#define UNARY_PRIORITY 8

// the structure of an assignment's left hand sides (restassign / the `def' and `const' lists)
struct LHS_assign {
	LHS_assign* prev;
	expdesc v;  // variable (global, local, upvalue, or indexed)
};

// the state of a table constructor
struct ConsControl {
	expdesc v;     // last list item read
	expdesc* t;    // table descriptor
	int nh;        // total number of `record' elements
	int na;        // total number of array elements
	int tostore;   // number of array elements pending to be stored
};

static void chunk(LexState* ls, int multi);
static int subexpr(LexState* ls, expdesc* v, int limit, int skipfirst);
static void expr(LexState* ls, expdesc* v);
static void constructor(LexState* ls, expdesc* t);
static void body(LexState* ls, expdesc* e, int needself, int line, int hasparams);
static int explist1(LexState* ls, expdesc* v);
static void primaryexp(LexState* ls, expdesc* v);

// ---- tokens ------------------------------------------------------------------------------------------

// 0x70e540: luaX_next that reports it did (the exe's testnext/check call it after comparing the token)
static int testnext(LexState* ls, int c)
{
	if (ls->t.token == c) {
		luaX_next(ls);
		return 1;
	}
	return 0;
}

// 0x70e500
static void error_expected(LexState* ls, int token)
{
	luaX_syntaxerror(ls, luaO_pushfstring(ls->L, "`%s' expected", luaX_token2str(ls, token)));
}

// 0x70e590
static void check(LexState* ls, int c)
{
	if (!testnext(ls, c))
		error_expected(ls, c);
}

// 0x70e5b0
static void check_match(LexState* ls, int what, int who, int where)
{
	if (!testnext(ls, what)) {
		if (where == ls->linenumber)
			error_expected(ls, what);
		else {
			const char* swhat = luaX_token2str(ls, what);
			const char* swho = luaX_token2str(ls, who);
			luaX_syntaxerror(ls, luaO_pushfstring(ls->L, "`%s' expected (to close `%s' at line %d)", swhat, swho, where));
		}
	}
}

// 0x70e760 (str_checkname + next)
static TString* str_checkname(LexState* ls)
{
	TString* ts;
	if (ls->t.token != TK_NAME)
		luaX_syntaxerror(ls, "<name> expected");
	ts = ls->t.seminfo.ts;
	luaX_next(ls);
	return ts;
}

static void init_exp(expdesc* e, expkind k, int i)
{
	e->f = e->t = NO_JUMP;
	e->k = k;
	e->info = i;
	e->aux = 0;
}

// 0x70e720
static void codestring(LexState* ls, expdesc* e, TString* s)
{
	init_exp(e, VK, luaK_stringK(ls->fs, s));
}

static void checkname(LexState* ls, expdesc* e)
{
	codestring(ls, e, str_checkname(ls));
}

// 0x710240: the constant for a name the compiler itself uses
static int literalK(LexState* ls, const char* s)
{
	return luaK_stringK(ls->fs, luaS_new(ls->L, s));
}

// ---- local variables and upvalues --------------------------------------------------------------------

// 0x70e7c0
static int registerlocalvar(LexState* ls, TString* varname)
{
	FuncState* fs = ls->fs;
	Proto* f = fs->f;
	luaM_growvector(ls->L, f->locvars, fs->nlocvars, f->sizelocvars, LocVar, MAX_INT, "");
	f->locvars[fs->nlocvars].varname = varname;
	f->locvars[fs->nlocvars].startpc = 0;
	f->locvars[fs->nlocvars].endpc = 0;
	return fs->nlocvars++;
}

// 0x70e860
static void new_localvar(LexState* ls, TString* name, int n)
{
	FuncState* fs = ls->fs;
	if (fs->nactvar + n + 1 > MAXVARS)
		luaX_errorlimit(ls, "local variables", MAXVARS);
	fs->actvar[fs->nactvar + n] = registerlocalvar(ls, name);
}

// 0x70e8c0
static void adjustlocalvars(LexState* ls, int nvars)
{
	FuncState* fs = ls->fs;
	fs->nactvar += nvars;
	for (; nvars; nvars--)
		getlocvar(fs, fs->nactvar - nvars).startpc = fs->pc;
}

// 0x70e900
static void removevars(LexState* ls, int tolevel)
{
	FuncState* fs = ls->fs;
	while (fs->nactvar > tolevel)
		getlocvar(fs, --fs->nactvar).endpc = fs->pc;
}

// 0x70e970: a new local named by a literal, active at once
static void new_localvarstr(LexState* ls, const char* name)
{
	FuncState* fs;
	new_localvar(ls, luaS_new(ls->L, name), 0);
	fs = ls->fs;
	fs->nactvar++;
	getlocvar(fs, fs->nactvar - 1).startpc = fs->pc;
}

static int eqchars_nocase(const char* a, const char* b)
{
	for (; *a != 0 && *b != 0; a++, b++) {
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return 0;
	}
	return *a == *b;
}

// 0x70e640: the names equal ignoring case (the exe also links the two strings as each other's alias)
static int eqnocase(TString* a, TString* b)
{
	if (a == b || a->alias == b)
		return 1;
	if (a->hash == b->hash && a->len == b->len && eqchars_nocase(getstr(a), getstr(b))) {
		if (a->alias != NULL)
			a->alias->alias = NULL;
		a->alias = b;
		if (b->alias != NULL)
			b->alias->alias = NULL;
		b->alias = a;
		return 1;
	}
	return 0;
}

// 0x70e9c0
static int indexupvalue(FuncState* fs, TString* name, expdesc* v)
{
	int i;
	Proto* f = fs->f;
	for (i = 0; i < f->nups; i++) {
		if (fs->upvalues[i].k == v->k && fs->upvalues[i].info == v->info)
			return i;
	}
	// new one
	if (f->nups + 1 > MAXUPVALUES)
		luaX_errorlimit(fs->ls, "upvalues", MAXUPVALUES);
	luaM_growvector(fs->L, f->upvalues, f->nups, f->sizeupvalues, TString*, MAX_INT, "");
	f->upvalues[f->nups] = name;
	fs->upvalues[f->nups] = *v;
	return f->nups++;
}

// 0x70eb10
static int searchvar(FuncState* fs, TString* n)
{
	int i;
	for (i = fs->nactvar - 1; i >= 0; i--) {
		if (n == getlocvar(fs, i).varname)
			return i;
	}
	return -1;  // not found
}

// 0x70eb40
static void markupval(FuncState* fs, int level)
{
	BlockCnt* bl = fs->bl;
	while (bl && bl->nactvar > level)
		bl = bl->previous;
	if (bl)
		bl->upval = 1;
}

// 0x70eb60. PopLua: a global read inside a method (a function with `self') is a VSELFGLOBAL, and when a debugger
// function is being evaluated its upvalues are found by name (aux = 1)
static void singlevaraux(FuncState* fs, TString* n, expdesc* var, int base)
{
	if (fs == NULL) {  // no more levels?
		if (luaY_evalproto != NULL && luaY_evalproto->nups > 0) {
			int i;
			for (i = 0; i < luaY_evalproto->nups; i++) {
				if (i < luaY_evalproto->sizeupvalues && luaY_evalproto->upvalues[i] != NULL &&
					eqnocase(luaY_evalproto->upvalues[i], n)) {
					var->info = i;
					var->k = VUPVAL;
					var->aux = 1;
					return;
				}
			}
		}
		init_exp(var, VGLOBAL, NO_REG);  // default is global variable
		return;
	}
	else {
		int v = searchvar(fs, n);  // look up at current level
		if (v >= 0) {
			init_exp(var, VLOCAL, v);
			if (!base)
				markupval(fs, v);  // local will be used as an upval
		}
		else {  // not found at current level; try upper one
			singlevaraux(fs->prev, n, var, 0);
			if (var->k == VGLOBAL) {
				if (base) {
					if (fs->f->selfflag)
						var->k = VSELFGLOBAL;
					var->info = luaK_stringK(fs, n);  // info points to global name
				}
			}
			else if (var->k == VLOCAL || var->aux == 0) {
				var->info = indexupvalue(fs, n, var);
				var->k = VUPVAL;  // upvalue in this level
				var->aux = 0;
			}
		}
	}
}

// 0x70ec80
static void singlevar(LexState* ls, expdesc* var)
{
	TString* varname = str_checkname(ls);
	singlevaraux(ls->fs, varname, var, 1);
}

// 0x70ecc0
static void adjust_assign(LexState* ls, int nvars, int nexps, expdesc* e)
{
	FuncState* fs = ls->fs;
	int extra = nvars - nexps;
	if (e->k == VCALL) {
		extra++;  // includes call itself
		if (extra <= 0)
			extra = 0;
		else
			luaK_reserveregs(fs, extra - 1);
		luaK_setcallreturns(fs, e, extra);  // call provides the difference
	}
	else {
		if (e->k != VVOID)  // last expression is not empty?
			luaK_exp2nextreg(fs, e);  // close last expression
		if (extra > 0) {
			int reg = fs->freereg;
			luaK_reserveregs(fs, extra);
			luaK_nil(fs, reg, extra);
		}
	}
}

// 0x70ed60: the end of a parameter list: the parameters become active
static void parlist_end(LexState* ls, int nparams, int dots)
{
	FuncState* fs = ls->fs;
	adjustlocalvars(ls, nparams);
	if (fs->nactvar > MAXPARAMS)
		luaX_errorlimit(ls, "parameters", MAXPARAMS);
	fs->f->numparams = (lu_byte)fs->nactvar;
	fs->f->is_vararg = (lu_byte)dots;
	if (dots)
		new_localvarstr(ls, "arg");
	luaK_reserveregs(fs, fs->nactvar);  // reserve register for parameters
}

// 0x70fc00
static void parlist(LexState* ls)
{
	// parlist -> [ param { `,' param } ]
	int nparams = 0;
	int dots = 0;
	if (ls->t.token != ')') {  // is `parlist' not empty?
		for (;;) {
			if (ls->t.token != TK_NAME) {
				if (ls->t.token != TK_DOTS)
					luaX_syntaxerror(ls, "<name> or `...' expected");
				luaX_next(ls);
				dots = 1;
				break;
			}
			new_localvar(ls, str_checkname(ls), nparams++);
			if (ls->t.token != ',')
				break;
			luaX_next(ls);
		}
	}
	parlist_end(ls, nparams, dots);
}

// ---- functions and blocks ------------------------------------------------------------------------------

// 0x70ee10
static void enterblock(FuncState* fs, BlockCnt* bl, int isbreakable)
{
	bl->breaklist = NO_JUMP;
	bl->continuelist = NO_JUMP;
	bl->isbreakable = isbreakable;
	bl->nactvar = fs->nactvar;
	bl->upval = 0;
	bl->previous = fs->bl;
	fs->bl = bl;
}

// 0x70ee10
static void leaveblock(FuncState* fs)
{
	BlockCnt* bl = fs->bl;
	fs->bl = bl->previous;
	removevars(fs->ls, bl->nactvar);
	if (bl->upval)
		luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0);
	fs->freereg = fs->nactvar;  // free registers
	luaK_patchtohere(fs, bl->breaklist);
}

// 0x70ee60
static void pushclosure(LexState* ls, FuncState* func, expdesc* v)
{
	FuncState* fs = ls->fs;
	Proto* f = fs->f;
	int i;
	luaM_growvector(ls->L, f->p, fs->np, f->sizep, Proto*, MAXARG_Bx, "constant table overflow");
	f->p[fs->np++] = func->f;
	init_exp(v, VRELOCABLE, luaK_codeABx(fs, OP_CLOSURE, 0, fs->np - 1));
	for (i = 0; i < func->f->nups; i++) {
		OpCode o = (func->upvalues[i].k == VLOCAL) ? OP_MOVE : OP_GETUPVAL;
		luaK_codeABC(fs, o, 0, func->upvalues[i].info, 0);
	}
}

// 0x70ef60. PopLua: the table of constants compares strings case-sensitively (it is an ordinary table whose
// strings are not folded); while a debugger expression is compiled the function being debugged supplies the
// first locals (their registers) and the upvalues
static void open_func(LexState* ls, FuncState* fs)
{
	lua_State* L = ls->L;
	Proto* f = luaF_newproto(L);
	fs->f = f;
	fs->prev = ls->fs;  // linked list of funcstates
	fs->ls = ls;
	fs->L = L;
	ls->fs = fs;
	fs->pc = 0;
	fs->lasttarget = 0;
	fs->jpc = NO_JUMP;
	fs->freereg = 0;
	fs->nk = 0;
	fs->h = luaH_new(L, 0, 0);
	fs->h->prot = PROT_CASESENSITIVE;
	fs->np = 0;
	fs->nlocvars = 0;
	fs->nactvar = 0;
	fs->bl = NULL;
	fs->curclass = NULL;
	f->source = ls->source;
	luaS_fix(f->source);
	f->maxstacksize = 2;  // registers 0/1 are always valid
	if (luaY_evalproto != NULL) {
		const Proto* ep = luaY_evalproto;
		int pc = (int)(luaY_evalpc - ep->code);
		int n = 0;
		int i;
		for (i = 0; i < ep->sizelocvars; i++) {
			const LocVar* lv = &ep->locvars[i];
			if (lv->startpc < pc && pc <= lv->endpc) {
				new_localvar(ls, lv->varname, n);
				f->locvars[n].startpc = 0;
				f->locvars[n].endpc = -1;
				n++;
			}
		}
		f->selfflag = (ep->selfflag != 0 && n >= 1) ? 1 : 0;
		fs->nactvar = n;
		fs->freereg = n;
		f->nups = ep->nups;
		f->sizeupvalues = ep->nups;
		f->upvalues = ep->nups ? luaM_newvector(L, ep->nups, TString*) : NULL;
		for (i = 0; i < ep->nups; i++) {
			f->upvalues[i] = i < ep->sizeupvalues ? ep->upvalues[i] : NULL;
			fs->upvalues[i].k = VVOID;
			fs->upvalues[i].info = 0;
			fs->upvalues[i].aux = 0;
			fs->upvalues[i].t = NO_JUMP;
			fs->upvalues[i].f = NO_JUMP;
		}
	}
}

// 0x70f140. PopLua also allocates (and clears) the 8 bytes per instruction of the inline caches here; the port
// has none
static void close_func(LexState* ls)
{
	lua_State* L = ls->L;
	FuncState* fs = ls->fs;
	Proto* f = fs->f;
	int i;
	for (i = 0; i < fs->nlocvars; i++) {  // the locals of a debugger function end with it
		if (f->locvars[i].endpc != -1)
			break;
		f->locvars[i].endpc = fs->pc;
	}
	removevars(ls, 0);
	luaK_codeABC(fs, OP_RETURN, 0, 1, 0);  // final return
	luaM_reallocvector(L, f->code, f->sizecode, fs->pc, Instruction);
	f->sizecode = fs->pc;
	luaM_reallocvector(L, f->lineinfo, f->sizelineinfo, fs->pc, int);
	f->sizelineinfo = fs->pc;
	luaM_reallocvector(L, f->k, f->sizek, fs->nk, TObject);
	f->sizek = fs->nk;
	luaM_reallocvector(L, f->p, f->sizep, fs->np, Proto*);
	f->sizep = fs->np;
	luaM_reallocvector(L, f->locvars, f->sizelocvars, fs->nlocvars, LocVar);
	f->sizelocvars = fs->nlocvars;
	luaM_reallocvector(L, f->upvalues, f->sizeupvalues, f->nups, TString*);
	f->sizeupvalues = f->nups;
	ls->fs = fs->prev;
}

// ---- the grammar --------------------------------------------------------------------------------------

// 0x7113f0
static int block_follow(int token)
{
	switch (token) {
	case '}':
	case TK_ELSE:
	case TK_ELSEIF:
	case TK_END:
	case TK_UNTIL:
	case TK_EOS:
		return 1;
	default:
		return 0;
	}
}

// 0x70f3f0
static void field(LexState* ls, expdesc* v)
{
	// field -> ['.' | ':'] NAME
	FuncState* fs = ls->fs;
	expdesc key;
	luaK_exp2anyreg(fs, v);
	luaX_next(ls);  // skip the dot or colon
	checkname(ls, &key);
	luaK_indexed(fs, v, &key);
}

// 0x70f4a0
static void luaY_index(LexState* ls, expdesc* v)
{
	// index -> '[' expr ']'
	luaX_next(ls);  // skip the '['
	expr(ls, v);
	luaK_exp2val(ls->fs, v);
	check(ls, ']');
}

// ---- table constructors --------------------------------------------------------------------------------

// 0x70f540
static void recfield(LexState* ls, ConsControl* cc)
{
	// recfield -> (NAME | `['exp1`]') = exp1
	FuncState* fs = ls->fs;
	int reg = ls->fs->freereg;
	expdesc key, val;
	int rkv, rkk;
	if (ls->t.token == TK_NAME) {
		if (cc->nh > MAX_INT)
			luaX_errorlimit(ls, "items in a constructor", MAX_INT);
		cc->nh++;
		checkname(ls, &key);
	}
	else  // ls->t.token == '['
		luaY_index(ls, &key);
	check(ls, '=');
	luaK_exp2RK(fs, &key);
	expr(ls, &val);
	rkv = luaK_exp2RK(fs, &val);  // (the exe evaluates the arguments of the call from the right)
	rkk = luaK_exp2RK(fs, &key);
	luaK_codeABC(fs, OP_SETTABLE, cc->t->info, rkk, rkv);
	fs->freereg = reg;  // free registers
}

// 0x70f690
static void lastlistfield(FuncState* fs, ConsControl* cc)
{
	if (cc->tostore == 0)
		return;
	if (cc->v.k == VCALL) {
		SETARG_C(getcode(fs, &cc->v), 0);  // luaK_setcallreturns(MULTRET)
		luaK_codeABx(fs, OP_SETLISTO, cc->t->info, cc->na - 1);
	}
	else {
		if (cc->v.k != VVOID)
			luaK_exp2nextreg(fs, &cc->v);
		luaK_codeABx(fs, OP_SETLIST, cc->t->info, cc->na - 1);
	}
	fs->freereg = cc->t->info + 1;  // free registers
}

// 0x70f720
static void listfield(LexState* ls, ConsControl* cc)
{
	expr(ls, &cc->v);
	if (cc->na > MAXARG_Bx)
		luaX_errorlimit(ls, "items in a constructor", MAXARG_Bx);
	cc->na++;
	cc->tostore++;
}

static void closelistfield(FuncState* fs, ConsControl* cc)
{
	if (cc->v.k == VVOID)
		return;  // there is no list item
	luaK_exp2nextreg(fs, &cc->v);
	cc->v.k = VVOID;
	if (cc->tostore == LFIELDS_PER_FLUSH) {
		luaK_codeABx(fs, OP_SETLIST, cc->t->info, cc->na - 1);  // flush
		cc->tostore = 0;  // no more items pending
		fs->freereg = cc->t->info + 1;  // free registers
	}
}

// 0x70f780
static void constructor(LexState* ls, expdesc* t)
{
	// constructor -> '{' [ field { sep field } [sep] ] '}'  (sep -> ',' | ';')
	FuncState* fs = ls->fs;
	int line = ls->linenumber;
	int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
	ConsControl cc;
	cc.na = cc.nh = cc.tostore = 0;
	cc.t = t;
	init_exp(t, VRELOCABLE, pc);
	init_exp(&cc.v, VVOID, 0);  // no value (yet)
	luaK_exp2nextreg(ls->fs, t);  // fix it at stack top (for gc)
	check(ls, '{');
	do {
		testnext(ls, ';');  // compatibility only
		if (ls->t.token == '}')
			break;
		closelistfield(fs, &cc);
		switch (ls->t.token) {
		case TK_NAME:  // may be listfields or recfields
			luaX_lookahead(ls);
			if (ls->lookahead.token != '=')  // expression?
				listfield(ls, &cc);
			else
				recfield(ls, &cc);
			break;
		case '[':  // constructor_item -> recfield
			recfield(ls, &cc);
			break;
		default:  // constructor_part -> listfield
			listfield(ls, &cc);
			break;
		}
	} while (testnext(ls, ',') || testnext(ls, ';'));
	check_match(ls, '}', '{', line);
	lastlistfield(fs, &cc);
	SETARG_B(fs->f->code[pc], luaO_int2fb(cc.na));  // set initial array size
	SETARG_C(fs->f->code[pc], luaO_log2(cc.nh) + 1);  // set initial table size
}

// ---- function bodies -----------------------------------------------------------------------------------

// 0x70fd90. PopLua: the parameter list may be missing (class constructors take their `(' from the caller); the
// body is `{ ... }' or `... end'; the function gets its `self' flag (and the local `self') for a method
static void body(LexState* ls, expdesc* e, int needself, int line, int hasparams)
{
	// body ->  [`(' parlist `)'] chunk END
	FuncState new_fs;
	int bline;
	int brace;
	open_func(ls, &new_fs);
	new_fs.f->lineDefined = line;
	if (hasparams)
		check(ls, '(');
	new_fs.f->selfflag = needself;
	if (needself)
		new_localvarstr(ls, "self");
	if (!hasparams)
		parlist_end(ls, 0, 0);
	else {
		parlist(ls);
		check(ls, ')');
	}
	bline = ls->linenumber;
	brace = ls->t.token == '{';
	if (brace)
		luaX_next(ls);
	chunk(ls, 1);
	if (!brace)
		check_match(ls, TK_END, TK_FUNCTION, line);
	else
		check_match(ls, '}', '{', bline);
	close_func(ls);
	pushclosure(ls, &new_fs, e);
}

// 0x70ff30
static int explist1(LexState* ls, expdesc* v)
{
	// explist1 -> expr { `,' expr }
	int n = 1;  // at least one expression
	expr(ls, v);
	while (testnext(ls, ',')) {
		luaK_exp2nextreg(ls->fs, v);
		expr(ls, v);
		n++;
	}
	return n;
}

// 0x70ffc0
static void funcargs(LexState* ls, expdesc* f)
{
	FuncState* fs = ls->fs;
	expdesc args;
	int base, nparams;
	int line = ls->linenumber;
	switch (ls->t.token) {
	case '(': {  // funcargs -> `(' [ explist1 ] `)'
		if (line != ls->lastline)
			luaX_syntaxerror(ls, "ambiguous syntax (function call x new statement)");
		luaX_next(ls);
		if (ls->t.token == ')')  // arg list is empty?
			args.k = VVOID;
		else {
			explist1(ls, &args);
			luaK_setcallreturns(fs, &args, LUA_MULTRET);
		}
		check_match(ls, ')', '(', line);
		break;
	}
	case '{':  // funcargs -> constructor
		constructor(ls, &args);
		break;
	case TK_STRING:  // funcargs -> STRING
		codestring(ls, &args, ls->t.seminfo.ts);
		luaX_next(ls);  // must use `seminfo' before `next'
		break;
	default:
		luaX_syntaxerror(ls, "function arguments expected");
		return;
	}
	base = f->info;  // base register for call
	if (args.k == VCALL)
		nparams = LUA_MULTRET;  // open call
	else {
		if (args.k != VVOID)
			luaK_exp2nextreg(fs, &args);  // close last argument
		nparams = fs->freereg - (base + 1);
	}
	init_exp(f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams + 1, 2));
	luaK_fixline(fs, line);
	fs->freereg = base + 1;  // call remove function and arguments and leaves (unless changed) one result
}

// ---- expressions ---------------------------------------------------------------------------------------

// 0x710100: prefixexp -> NAME | `(' expr `)' | `:' NAME (a global, explicitly)
static void prefixexp(LexState* ls, expdesc* v)
{
	switch (ls->t.token) {
	case '(': {
		int line = ls->linenumber;
		luaX_next(ls);
		expr(ls, v);
		check_match(ls, ')', '(', line);
		luaK_dischargevars(ls->fs, v);
		return;
	}
	case ':': {  // a global even where a name would be searched in `self' first
		TString* name;
		luaX_next(ls);
		if (ls->t.token != TK_NAME)
			luaX_syntaxerror(ls, "<name> expected");
		name = ls->t.seminfo.ts;
		luaX_next(ls);
		init_exp(v, VGLOBAL, NO_REG);
		v->info = luaK_stringK(ls->fs, name);
		return;
	}
	case TK_NAME:
		singlevar(ls, v);
		return;
	default:
		luaX_syntaxerror(ls, "unexpected symbol");
		return;
	}
}

// 0x7108d0: primaryexp -> prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | `@' prefixexp `:' NAME funcargs |
// funcargs }. PopLua: `{' does not start function arguments here; a call of a name that may be a method of
// `self' is SELFORGLOBAL + CALLSELFORGLOBAL
static void primaryexp(LexState* ls, expdesc* v)
{
	FuncState* fs = ls->fs;
	prefixexp(ls, v);
	for (;;) {
		switch (ls->t.token) {
		case '.':  // field
			field(ls, v);
			break;
		case '[': {  // `[' exp1 `]'
			expdesc key;
			luaK_exp2anyreg(fs, v);
			luaY_index(ls, &key);
			luaK_indexed(fs, v, &key);
			break;
		}
		case ':': {  // `:' NAME funcargs
			expdesc key;
			luaX_next(ls);
			checkname(ls, &key);
			luaK_self(fs, v, &key);
			funcargs(ls, v);
			break;
		}
		case '@': {  // `@' prefixexp `:' NAME funcargs: FuncTable@SelfTable:Function(args)
			expdesc e2, key;
			luaX_next(ls);
			prefixexp(ls, &e2);
			if (ls->t.token != ':')
				luaX_syntaxerror(ls, "Invalid syntax: ':' expected next for statement of form FuncTable@SelfTable:Function");
			luaX_next(ls);
			checkname(ls, &key);
			luaK_altself(fs, v, &e2, &key);
			funcargs(ls, v);
			break;
		}
		case '(':
		case TK_STRING:  // funcargs
			if (v->k == VSELFGLOBAL) {
				int func = ls->fs->freereg;
				int rk;
				v->k = VK;
				luaK_reserveregs(fs, 2);
				rk = luaK_exp2RK(ls->fs, v);
				luaK_codeABC(fs, OP_SELFORGLOBAL, func, 0, rk);
				if (v->k == VNONRELOC && v->info >= fs->nactvar && v->info < MAXSTACK)  // freeexp
					fs->freereg--;
				v->info = func;
				v->k = VNONRELOC;
				funcargs(ls, v);
				{
					Instruction* i = &fs->f->code[fs->pc - 1];
					*i = (*i & ~0x13u) | OP_CALLSELFORGLOBAL;  // OP_CALL -> OP_CALLSELFORGLOBAL
				}
			}
			else {
				luaK_exp2nextreg(fs, v);
				funcargs(ls, v);
			}
			break;
		default:
			return;
		}
	}
}

// 0x710290: `super.field' and `super:method(args)' inside a method of a class: the class's `base' table
static void superexpr(LexState* ls, expdesc* e)
{
	FuncState* fs = ls->fs;
	if (!(fs->nactvar >= 1 && strcmp(getstr(getlocvar(fs, 0).varname), "self") == 0 && fs->prev != NULL &&
			fs->prev->curclass != NULL))
		luaX_syntaxerror(ls, "'super' only allowed in method calls");
	luaX_next(ls);  // skip SUPER
	if (ls->t.token == '.') {
		int kbase = literalK(ls, "base");
		int kclass = luaK_stringK(fs, fs->prev->curclass);
		init_exp(e, VINDEXED, fs->freereg);
		e->aux = fs->freereg + 1;
		luaK_reserveregs(fs, 2);
		luaK_codeABx(fs, OP_GETGLOBAL, e->info, kclass);
		luaK_codeABx(fs, OP_LOADK, e->aux, kbase);
		field(ls, e);
	}
	else {
		expdesc key;
		int r, kclass, kbase;
		if (ls->t.token != ':')
			luaX_syntaxerror(ls, "Invalid syntax: ':' expected in statement of form super:method");
		luaX_next(ls);
		checkname(ls, &key);
		init_exp(e, VVOID, 0);
		r = luaK_exp2anyreg(fs, e);
		luaK_reserveregs(fs, 1);
		kclass = luaK_stringK(fs, fs->prev->curclass);
		kbase = literalK(ls, "base");
		luaK_codeABx(fs, OP_GETGLOBAL, r, kclass);
		luaK_codeABx(fs, OP_LOADK, r + 1, kbase);
		luaK_codeABC(fs, OP_GETTABLE, r, r, r + 1);
		luaK_codeABC(fs, OP_MOVE, r + 1, 0, 0);  // self, which ALTSELF passes on
		luaK_codeABC(fs, OP_ALTSELF, r, r, luaK_exp2RK(fs, &key));
		funcargs(ls, e);
	}
}

// 0x710470: `cobj:Name.Name@hex' is getobj("Name.Name", 0xhex)
static void cobjexp(LexState* ls, expdesc* e)
{
	FuncState* fs;
	TString* name;
	int kgetobj, kname, kid, base;
	luaX_next(ls);  // skip COBJ
	if (ls->t.token != ':')
		error_expected(ls, ':');
	luaX_next(ls);
	if (ls->t.token != TK_NAME)
		luaX_syntaxerror(ls, "<name> expected");
	name = ls->t.seminfo.ts;
	luaX_next(ls);
	while (ls->t.token == '.') {
		TString* part;
		char buff[260];
		size_t len;
		luaX_next(ls);
		if (ls->t.token != TK_NAME)
			luaX_syntaxerror(ls, "<name> expected");
		part = ls->t.seminfo.ts;
		luaX_next(ls);
		len = name->len + part->len + 1;
		if (len > 255)
			break;
		snprintf(buff, sizeof buff, "%s.%s", getstr(name), getstr(part));
		name = luaS_newlstr(ls->L, buff, len);
	}
	if (ls->t.token != '@')
		error_expected(ls, '@');
	ls->t.token = TK_INT;
	luaX_readhex(ls, ls->current, &ls->t.seminfo);
	fs = ls->fs;
	kgetobj = literalK(ls, "getobj");
	kname = luaK_stringK(fs, name);
	kid = luaK_intK(fs, ls->t.seminfo.i);
	init_exp(e, VVOID, 0);
	base = luaK_exp2anyreg(fs, e);
	luaK_reserveregs(fs, 3);
	luaK_codeABx(fs, OP_GETGLOBAL, base, kgetobj);
	luaK_codeABx(fs, OP_LOADK, base + 1, kname);
	luaK_codeABx(fs, OP_LOADK, base + 2, kid);
	init_exp(e, VCALL, luaK_codeABC(fs, OP_CALL, base, 3, 2));
	luaX_next(ls);
}

// 0x710760: `Table@hex' is gettableat(0xhex)
static void tableexp(LexState* ls, expdesc* e)
{
	FuncState* fs;
	int kfunc, kid, base;
	luaX_next(ls);  // skip TABLE
	if (ls->t.token != '@')
		error_expected(ls, '@');
	ls->t.token = TK_INT;
	luaX_readhex(ls, ls->current, &ls->t.seminfo);
	fs = ls->fs;
	kfunc = literalK(ls, "gettableat");
	kid = luaK_intK(fs, ls->t.seminfo.i);
	init_exp(e, VVOID, 0);
	base = luaK_exp2anyreg(fs, e);
	luaK_reserveregs(fs, 2);
	luaK_codeABx(fs, OP_GETGLOBAL, base, kfunc);
	luaK_codeABx(fs, OP_LOADK, base + 1, kid);
	init_exp(e, VCALL, luaK_codeABC(fs, OP_CALL, base, 2, 2));
	luaX_next(ls);
}

// 0x70fab0: `__POS' is the table {source, line}
static void posexpr(LexState* ls, expdesc* e)
{
	FuncState* fs = ls->fs;
	int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
	int ksource, kline;
	init_exp(e, VRELOCABLE, pc);
	luaK_exp2nextreg(fs, e);
	ksource = luaK_stringK(fs, ls->source);
	kline = luaK_intK(fs, ls->linenumber);
	luaK_reserveregs(ls->fs, 2);
	luaK_codeABx(fs, OP_LOADK, e->info + 1, ksource);
	luaK_codeABx(fs, OP_LOADK, e->info + 2, kline);
	luaK_codeABx(fs, OP_SETLIST, e->info, 1);
	fs->freereg = e->info + 1;
	SETARG_B(fs->f->code[pc], 2);
}

// 0x710b70
static void simpleexp(LexState* ls, expdesc* v)
{
	// simpleexp -> NUMBER | INT | STRING | NIL | TRUE | FALSE | constructor | FUNCTION body | super | cobj | Table |
	// __POS | primaryexp
	switch (ls->t.token) {
	case TK_NUMBER:
		init_exp(v, VK, luaK_numberK(ls->fs, ls->t.seminfo.r));
		luaX_next(ls);  // must use `seminfo' before `next'
		break;
	case TK_INT:
		init_exp(v, VK, luaK_intK(ls->fs, ls->t.seminfo.i));
		luaX_next(ls);
		break;
	case TK_STRING:
		codestring(ls, v, ls->t.seminfo.ts);
		luaX_next(ls);  // must use `seminfo' before `next'
		break;
	case TK_NIL:
		init_exp(v, VNIL, 0);
		luaX_next(ls);
		break;
	case TK_TRUE:
		init_exp(v, VTRUE, 0);
		luaX_next(ls);
		break;
	case TK_FALSE:
		init_exp(v, VFALSE, 0);
		luaX_next(ls);
		break;
	case '{':  // constructor
		constructor(ls, v);
		break;
	case TK_FUNCTION:
		luaX_next(ls);
		body(ls, v, 0, ls->linenumber, 1);
		break;
	case TK_SUPER:
		superexpr(ls, v);
		break;
	case TK_COBJ:
		cobjexp(ls, v);
		break;
	case TK_TABLE:
		tableexp(ls, v);
		break;
	case TK_POS:
		posexpr(ls, v);
		luaX_next(ls);
		break;
	default:
		primaryexp(ls, v);
		break;
	}
}

// 0x710f90
static UnOpr getunopr(int op)
{
	switch (op) {
	case '!':
	case TK_NOT:
		return OPR_NOT;
	case '-':
		return OPR_MINUS;
	case '*':
		return OPR_SIZ;
	case TK_INTCAST:
		return OPR_INTCAST;
	default:
		return OPR_NOUNOPR;
	}
}

// 0x7110e0
static BinOpr getbinopr(int op)
{
	switch (op) {
	case '+': return OPR_ADD;
	case '-': return OPR_SUB;
	case '*': return OPR_MULT;
	case '/': return OPR_DIV;
	case '%': return OPR_MOD;
	case '^': return OPR_POW;
	case TK_CONCAT: return OPR_CONCAT;
	case TK_NE:
	case TK_NE2: return OPR_NE;
	case TK_EQ: return OPR_EQ;
	case '<': return OPR_LT;
	case TK_LE: return OPR_LE;
	case '>': return OPR_GT;
	case TK_GE: return OPR_GE;
	case TK_AND: return OPR_AND;
	case TK_OR: return OPR_OR;
	default: return OPR_NOBINOPR;
	}
}

// the priorities of the binary operators (0x79b748)
static const struct {
	lu_byte left;   // left priority for each binary operator
	lu_byte right;  // right priority
} priority[] = {
	{6, 6}, {6, 6}, {7, 7}, {7, 7}, {10, 9}, {7, 7}, {5, 4},  // + - * / ^ % ..
	{3, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},           // != == < <= > >=
	{2, 2}, {1, 1}                                            // and or
};

// 0x7112b0: subexpr -> (simpleexp | unop subexpr) { binop subexpr }, where `binop' is any binary operator with a
// priority higher than `limit'; `skipfirst' says the first operand is already in `v'. It returns the first
// operator not read
static int subexpr(LexState* ls, expdesc* v, int limit, int skipfirst)
{
	BinOpr op;
	if (++ls->nestlevel > MAXSYNTAXLEVELS)
		luaX_syntaxerror(ls, "too many syntax levels");
	if (!skipfirst) {
		UnOpr uop = getunopr(ls->t.token);
		if (uop != OPR_NOUNOPR) {
			luaX_next(ls);
			subexpr(ls, v, UNARY_PRIORITY, 0);
			luaK_prefix(ls->fs, uop, v);
		}
		else
			simpleexp(ls, v);
	}
	// expand while operators have priorities higher than `limit'
	op = getbinopr(ls->t.token);
	while (op != OPR_NOBINOPR && priority[op].left > limit) {
		expdesc v2;
		int nextop;
		luaX_next(ls);
		luaK_infix(ls->fs, op, v);
		// read sub-expression with higher priority
		nextop = subexpr(ls, &v2, priority[op].right, 0);
		luaK_posfix(ls->fs, op, v, &v2);
		op = (BinOpr)nextop;
	}
	ls->nestlevel--;
	return op;
}

// 0x7113d0
static void expr(LexState* ls, expdesc* v)
{
	subexpr(ls, v, -1, 0);
}

// ---- statements ----------------------------------------------------------------------------------------

// 0x7114d0
static void block(LexState* ls, int multi)
{
	// block -> chunk
	FuncState* fs = ls->fs;
	BlockCnt bl;
	enterblock(fs, &bl, 0);
	chunk(ls, multi);
	leaveblock(fs);
}

// 0x711520
static void check_conflict(LexState* ls, LHS_assign* lh, expdesc* v)
{
	FuncState* fs = ls->fs;
	int extra = fs->freereg;  // eventual position to save local variable
	int conflict = 0;
	for (; lh; lh = lh->prev) {
		if (lh->v.k == VINDEXED) {
			if (lh->v.info == v->info) {  // conflict?
				conflict = 1;
				lh->v.info = extra;  // previous assignment will use safe copy
			}
			if (lh->v.aux == v->info) {  // conflict?
				conflict = 1;
				lh->v.aux = extra;  // previous assignment will use safe copy
			}
		}
	}
	if (conflict) {
		luaK_codeABC(fs, OP_MOVE, fs->freereg, v->info, 0);  // make copy
		luaK_reserveregs(fs, 1);
	}
}

// the assignment of the value in the last register to a variable
static void default_assign(LexState* ls, LHS_assign* lh)
{
	expdesc e;
	init_exp(&e, VNONRELOC, ls->fs->freereg - 1);
	luaK_storevar(ls->fs, &lh->v, &e);
}

// 0x7115b0. PopLua: `a += e' ... `a %= e' and `a++', `a--' (the compound operators are two tokens): the variable
// is read into a register, the operation done in place, and the result stored back
static void restassign(LexState* ls, LHS_assign* lh, int nvars)
{
	expdesc e;
	int op = -1;
	int incdec = 0;
	int nexps;
	FuncState* fs;
	if (!(VLOCAL <= lh->v.k && lh->v.k <= VINDEXED))
		luaX_syntaxerror(ls, "syntax error");
	if (ls->t.token == ',' && testnext(ls, ',')) {  // restassign -> ',' primaryexp restassign
		LHS_assign nv;
		nv.prev = lh;
		primaryexp(ls, &nv.v);
		if (nv.v.k == VLOCAL)
			check_conflict(ls, lh, &nv.v);
		restassign(ls, &nv, nvars + 1);
		default_assign(ls, lh);
		return;
	}
	switch (ls->t.token) {
	case '+':
		luaX_next(ls);
		if (ls->t.token == '+') {
			incdec = 1;
			luaX_next(ls);
		}
		else
			check(ls, '=');
		op = OP_ADD;
		break;
	case '-':
		luaX_next(ls);
		if (ls->t.token == '-') {
			incdec = 1;
			luaX_next(ls);
		}
		else
			check(ls, '=');
		op = OP_SUB;
		break;
	case '*':
		luaX_next(ls);
		check(ls, '=');
		op = OP_MUL;
		break;
	case '/':
		luaX_next(ls);
		check(ls, '=');
		op = OP_DIV;
		break;
	case '%':
		luaX_next(ls);
		check(ls, '=');
		op = OP_MOD;
		break;
	default:  // restassign -> `=' explist1
		check(ls, '=');
		break;
	}
	if (incdec) {
		init_exp(&e, VK, luaK_intK(ls->fs, 1));
		nexps = 1;
	}
	else
		nexps = explist1(ls, &e);
	fs = ls->fs;
	if (nexps != nvars) {
		adjust_assign(ls, nvars, nexps, &e);
		if (nexps > nvars)
			ls->fs->freereg -= nexps - nvars;  // remove extra values
		default_assign(ls, lh);
		return;
	}
	luaK_setcallreturns(fs, &e, 1);  // close last expression
	if (op == -1) {
		luaK_storevar(fs, &lh->v, &e);
		return;
	}
	{
		expdesc sv = lh->v;
		int savedfree = fs->freereg;
		int r1, r2;
		luaK_dischargevars(fs, &lh->v);
		luaK_dischargevars(fs, &e);
		fs->freereg = savedfree;
		r1 = luaK_exp2anyreg(fs, &lh->v);
		r2 = luaK_exp2anyreg(fs, &e);
		luaK_codeABC(fs, (OpCode)op, r1, r1, r2);
		switch (sv.k) {
		case VLOCAL:
			luaK_codeABC(fs, OP_MOVE, sv.info, r1, 0);
			break;
		case VUPVAL:
			luaK_codeABC(fs, OP_SETUPVAL, r1, sv.info, 0);
			break;
		case VGLOBAL:
			luaK_codeABx(fs, OP_SETGLOBAL, r1, sv.info);
			break;
		case VSELFGLOBAL:
			luaK_codeABx(fs, OP_SETSELFORGLOBAL, r1, sv.info);
			break;
		case VINDEXED:
			luaK_codeABC(fs, OP_SETTABLE, sv.info, sv.aux, r1);
			break;
		default:
			break;
		}
	}
}

// 0x713330 / 0x7134d0: `const a, b = ...' and `def a, b = ...': the targets are globals, fields of self or table
// fields; `store' is luaK_constvar or luaK_defvar
static void defassign(LexState* ls, LHS_assign* lh, int nvars, void (*store)(FuncState*, const expdesc*, expdesc*))
{
	expdesc e;
	int nexps = 0;
	FuncState* fs;
	if (lh->v.k != VGLOBAL && lh->v.k != VSELFGLOBAL && lh->v.k != VINDEXED)
		luaX_syntaxerror(ls, "syntax error");
	if (ls->t.token == ',' && testnext(ls, ',')) {
		LHS_assign nv;
		nv.prev = lh;
		primaryexp(ls, &nv.v);
		if (nv.v.k == VLOCAL)
			check_conflict(ls, lh, &nv.v);
		defassign(ls, &nv, nvars + 1, store);
		init_exp(&e, VNONRELOC, ls->fs->freereg - 1);
		store(ls->fs, &lh->v, &e);
		return;
	}
	if (ls->t.token == '=') {
		luaX_next(ls);
		nexps = explist1(ls, &e);
	}
	else {
		if (ls->t.token != ';')
			luaX_syntaxerror(ls, "syntax error");
		init_exp(&e, VVOID, 0);
		luaX_next(ls);
	}
	fs = ls->fs;
	if (nexps == nvars) {
		luaK_setcallreturns(fs, &e, 1);
		store(fs, &lh->v, &e);
		return;
	}
	adjust_assign(ls, nvars, nexps, &e);
	if (nexps > nvars)
		ls->fs->freereg -= nexps - nvars;  // remove extra values
	init_exp(&e, VNONRELOC, ls->fs->freereg - 1);
	store(ls->fs, &lh->v, &e);
}

// 0x7134b0 / 0x713650
static void conststat(LexState* ls)
{
	LHS_assign lh;
	lh.prev = NULL;
	primaryexp(ls, &lh.v);
	defassign(ls, &lh, 1, luaK_constvar);
}

static void defstat(LexState* ls)
{
	LHS_assign lh;
	lh.prev = NULL;
	primaryexp(ls, &lh.v);
	defassign(ls, &lh, 1, luaK_defvar);
}

// 0x7140c0
static void exprstat(LexState* ls)
{
	// stat -> func | assignment
	FuncState* fs = ls->fs;
	LHS_assign v;
	primaryexp(ls, &v.v);
	if (v.v.k == VCALL)  // stat -> func
		luaK_setcallreturns(fs, &v.v, 0);
	else {  // stat -> assignment
		v.prev = NULL;
		restassign(ls, &v, 1);
	}
}

// 0x713d20: a statement that starts with `super'
static void superstat(LexState* ls)
{
	FuncState* fs = ls->fs;
	LHS_assign v;
	superexpr(ls, &v.v);
	if (v.v.k == VCALL)
		luaK_setcallreturns(fs, &v.v, 0);
	else {
		v.prev = NULL;
		restassign(ls, &v, 1);
	}
}

// 0x711940: cond -> expr, or `(' expr `)' followed by more of an expression; the exits when false are in v->f
static void cond(LexState* ls, expdesc* v)
{
	FuncState* fs = ls->fs;
	if (ls->t.token == '(') {
		int line = ls->linenumber;
		check(ls, '(');
		expr(ls, v);
		check_match(ls, ')', '(', line);
		luaK_dischargevars(fs, v);
		if (getbinopr(ls->t.token) != OPR_NOBINOPR)
			subexpr(ls, v, -1, 1);
	}
	else
		subexpr(ls, v, -1, 0);
	if (v->k == VNIL)
		v->k = VFALSE;  // `falses' are all equal here
	luaK_goiftrue(fs, v);
	luaK_patchtohere(fs, v->t);
}

// 0x712c80
static void test_then_block(LexState* ls, expdesc* v, int* pbrace)
{
	// test_then_block -> [IF | ELSEIF] cond (THEN | DO | `{' | nothing) block
	int line = ls->linenumber;
	int multi;
	luaX_next(ls);  // skip IF or ELSEIF
	cond(ls, v);
	if (ls->t.token == TK_THEN || ls->t.token == TK_DO) {
		*pbrace = 0;
		multi = 1;
		luaX_next(ls);
	}
	else {
		*pbrace = 1;
		if (ls->t.token == '{') {
			multi = 1;
			luaX_next(ls);
		}
		else
			multi = 0;
	}
	block(ls, multi);
	if (*pbrace && multi)
		check_match(ls, '}', '{', line);
}

// 0x712d90
static void ifstat(LexState* ls, int line)
{
	// ifstat -> IF cond block {ELSEIF cond block} [ELSE block] END
	FuncState* fs = ls->fs;
	int escapelist = NO_JUMP;
	expdesc v;
	int brace;
	test_then_block(ls, &v, &brace);  // IF cond block
	for (;;) {
		if (brace && ls->t.token == TK_ELSE) {  // `else if' is `elseif'
			ls->lookahead.token = luaX_lex(ls, &ls->lookahead.seminfo);
			if (ls->lookahead.token == TK_IF) {
				ls->lastline = ls->linenumber;
				ls->t.seminfo = ls->lookahead.seminfo;
				ls->lookahead.token = TK_EOS;
				ls->t.token = TK_ELSEIF;
			}
		}
		if (ls->t.token != TK_ELSEIF)
			break;
		luaK_concat(fs, &escapelist, luaK_jump(fs));
		luaK_patchtohere(fs, v.f);
		test_then_block(ls, &v, &brace);  // ELSEIF cond block
	}
	if (ls->t.token == TK_ELSE) {
		int multi;
		int elseline = ls->linenumber;
		luaK_concat(fs, &escapelist, luaK_jump(fs));
		luaK_patchtohere(fs, v.f);
		luaX_next(ls);  // skip ELSE
		if (brace) {
			if (ls->t.token == '{') {
				multi = 1;
				luaX_next(ls);
			}
			else
				multi = 0;
		}
		else
			multi = 1;
		block(ls, multi);
		if (brace && multi)
			check_match(ls, '}', '{', elseline);
	}
	else
		luaK_concat(fs, &escapelist, v.f);
	luaK_patchtohere(fs, escapelist);
	if (!brace)
		check_match(ls, TK_END, TK_IF, line);
}

// 0x711a40. PopLua turns the loop around: the condition (at most 100 instructions) is compiled where the loop
// starts and then moved to the end, after the body, where a jump that precedes the body reaches it
static void whilestat(LexState* ls, int line)
{
	FuncState* fs = ls->fs;
	expdesc v;
	BlockCnt bl;
	Instruction buf[100];
	int jmp, start, n, i, condline, bodyline, kw, multi;
	luaX_next(ls);  // skip WHILE
	jmp = luaK_jump(fs);
	start = luaK_getlabel(fs);
	if (ls->t.token == '(') {
		int cline = ls->linenumber;
		check(ls, '(');
		expr(ls, &v);
		check_match(ls, ')', '(', cline);
	}
	else
		expr(ls, &v);
	if (v.k == VK)
		v.k = VTRUE;  // `trues' (numbers, strings)
	condline = ls->linenumber;
	luaK_goiffalse(fs, &v);
	luaK_concat(fs, &v.f, fs->jpc);
	fs->jpc = NO_JUMP;
	n = fs->pc - start;
	if (n > 100)
		luaX_syntaxerror(ls, "`while' condition too complex");
	for (i = 0; i < n; i++)
		buf[i] = fs->f->code[start + i];
	fs->pc = start;
	enterblock(fs, &bl, 1);
	bodyline = ls->linenumber;
	kw = ls->t.token == TK_THEN || ls->t.token == TK_DO;
	if (kw || ls->t.token == '{') {
		multi = 1;
		luaX_next(ls);
	}
	else
		multi = 0;
	fs->lasttarget = fs->pc;
	block(ls, multi);
	luaK_patchtohere(fs, jmp);  // the first jump goes to the condition
	if (v.t != NO_JUMP)
		v.t += fs->pc - start;
	if (v.f != NO_JUMP)
		v.f += fs->pc - start;
	for (i = 0; i < n; i++)
		luaK_code(fs, buf[i], condline);
	if (kw)
		check_match(ls, TK_END, TK_WHILE, line);
	else if (multi)
		check_match(ls, '}', '{', bodyline);
	leaveblock(fs);
	luaK_patchlist(fs, v.t, start);  // true conditions go back to the body
	luaK_patchtohere(fs, v.f);       // false conditions finish the loop
}

// 0x711fa0
static void repeatstat(LexState* ls, int line)
{
	// repeatstat -> REPEAT block UNTIL cond
	FuncState* fs = ls->fs;
	int repeat_init = luaK_getlabel(fs);
	BlockCnt bl;
	expdesc v;
	int bline, brace;
	enterblock(fs, &bl, 1);
	luaX_next(ls);  // skip REPEAT
	bline = ls->linenumber;
	brace = ls->t.token == '{';
	if (brace)
		luaX_next(ls);
	block(ls, 1);
	if (brace)
		check_match(ls, '}', '{', bline);
	check_match(ls, TK_UNTIL, TK_REPEAT, line);
	cond(ls, &v);
	luaK_patchlist(fs, v.f, repeat_init);
	leaveblock(fs);
}

// 0x712350..: exp1 -> expr, in the next register
static void exp1(LexState* ls)
{
	expdesc e;
	expr(ls, &e);
	luaK_exp2nextreg(ls->fs, &e);
}

// 0x7121b0
static void forbody(LexState* ls, int base, int line, int nvars, int isnum, int* pbrace)
{
	// forbody -> (THEN | DO | `{' | nothing) block; the control variables and the loop jump are made by the caller
	BlockCnt bl;
	FuncState* fs = ls->fs;
	int line0 = ls->linenumber;
	int multi, blockinit, endfor, list;
	adjustlocalvars(ls, nvars);  // scope for all variables
	if (ls->t.token == TK_THEN || ls->t.token == TK_DO) {
		*pbrace = 0;
		multi = 1;
		luaX_next(ls);
	}
	else {
		*pbrace = 1;
		if (ls->t.token == '{') {
			multi = 1;
			luaX_next(ls);
		}
		else
			multi = 0;
	}
	enterblock(fs, &bl, 1);
	blockinit = luaK_getlabel(fs);
	block(ls, multi);
	luaK_patchtohere(fs, blockinit - 1);  // the jump before the body comes to the end test
	luaK_patchtohere(fs, bl.continuelist);  // and so does `continue'
	if (isnum)
		endfor = luaK_codeAsBx(fs, OP_FORLOOP, base, NO_JUMP);
	else
		endfor = luaK_codeABC(fs, OP_TFORLOOP, base, 0, nvars - 3);
	luaK_fixline(fs, line);  // pretend that `Lua' instruction is at `for' line
	list = isnum ? endfor : luaK_jump(fs);
	luaK_patchlist(fs, list, blockinit);
	leaveblock(fs);
	if (*pbrace && multi)
		check_match(ls, '}', '{', line0);
}

// 0x712350: fornum -> NAME = exp1,exp1[,exp1] forbody
static void fornum(LexState* ls, TString* varname, int hasparen, int line, int* pbrace)
{
	FuncState* fs = ls->fs;
	int base = fs->freereg;
	new_localvar(ls, varname, 0);
	new_localvar(ls, luaS_newlstr(ls->L, "_forlimit", 9), 1);
	new_localvar(ls, luaS_newlstr(ls->L, "_forstep", 8), 2);
	check(ls, '=');
	exp1(ls);  // initial value
	check(ls, ',');
	exp1(ls);  // limit
	if (testnext(ls, ','))
		exp1(ls);  // optional step
	else {  // default step = 1
		luaK_codeABx(fs, OP_LOADK, fs->freereg, luaK_intK(fs, 1));
		luaK_reserveregs(fs, 1);
	}
	luaK_codeABC(fs, OP_SUB, fs->freereg - 3, fs->freereg - 3, fs->freereg - 1);  // the loop adds the step first
	luaK_jump(fs);  // to the loop's end test
	if (hasparen)
		check(ls, ')');
	forbody(ls, base, line, 3, 1, pbrace);
}

// 0x712570: `for NAME < exp1': from 0 up to exp1 - 1
static void fornum2(LexState* ls, TString* varname, int hasparen, int line, int* pbrace)
{
	FuncState* fs = ls->fs;
	int base = fs->freereg;
	new_localvar(ls, varname, 0);
	new_localvar(ls, luaS_newlstr(ls->L, "_forlimit", 9), 1);
	new_localvar(ls, luaS_newlstr(ls->L, "_forstep", 8), 2);
	check(ls, '<');
	luaK_codeABx(fs, OP_LOADK, fs->freereg, luaK_intK(fs, 0));  // initial value 0
	luaK_reserveregs(fs, 1);
	exp1(ls);  // limit
	luaK_codeABx(fs, OP_LOADK, fs->freereg, luaK_intK(fs, 1));  // step 1
	luaK_reserveregs(fs, 1);
	luaK_codeABC(fs, OP_SUB, fs->freereg - 3, fs->freereg - 3, fs->freereg - 1);
	luaK_codeABC(fs, OP_SUB, fs->freereg - 2, fs->freereg - 2, fs->freereg - 1);  // the limit is exclusive
	luaK_jump(fs);
	if (hasparen)
		check(ls, ')');
	forbody(ls, base, line, 3, 1, pbrace);
}

// 0x7127c0: forlist -> NAME {,NAME} IN explist1 forbody
static void forlist(LexState* ls, TString* indexname, int hasparen, int* pbrace)
{
	FuncState* fs = ls->fs;
	expdesc e;
	int nvars = 3;  // the generator, the state and the first name
	int base = fs->freereg;
	int line, nexps;
	new_localvar(ls, luaS_new(ls->L, "(for generator)"), 0);
	new_localvar(ls, luaS_new(ls->L, "(for state)"), 1);
	new_localvar(ls, indexname, 2);
	while (testnext(ls, ','))
		new_localvar(ls, str_checkname(ls), nvars++);
	check(ls, TK_IN);
	line = ls->linenumber;
	nexps = explist1(ls, &e);
	adjust_assign(ls, nvars, nexps, &e);
	luaK_checkstack(fs, 3);  // extra space to call generator
	luaK_codeAsBx(fs, OP_TFORPREP, base, NO_JUMP);
	if (hasparen)
		check(ls, ')');
	forbody(ls, base, line, nvars, 0, pbrace);
}

// 0x712a00
static void forstat(LexState* ls, int line)
{
	// forstat -> [`('] FOR NAME (fornum | fornum2 | forlist) [`)'] END
	FuncState* fs = ls->fs;
	BlockCnt bl;
	TString* varname;
	int hasparen = 0;
	int brace = 0;
	enterblock(fs, &bl, 0);  // scope for loop and control variables
	luaX_next(ls);  // skip `for'
	if (ls->t.token == '(') {
		hasparen = 1;
		luaX_next(ls);
	}
	varname = str_checkname(ls);  // first variable name
	switch (ls->t.token) {
	case '=':
		fornum(ls, varname, hasparen, line, &brace);
		break;
	case '<':
		fornum2(ls, varname, hasparen, line, &brace);
		break;
	case ',':
	case TK_IN:
		forlist(ls, varname, hasparen, &brace);
		break;
	default:
		luaX_syntaxerror(ls, "`=' or `in' expected");
	}
	if (!brace)
		check_match(ls, TK_END, TK_FOR, line);
	leaveblock(fs);  // loop scope (`break' jumps to this point)
}

// 0x7131e0
static void localstat(LexState* ls)
{
	// stat -> LOCAL NAME {`,' NAME} [`=' explist1]
	int nvars = 0;
	int nexps;
	expdesc e;
	do {
		new_localvar(ls, str_checkname(ls), nvars++);
	} while (testnext(ls, ','));
	if (testnext(ls, '='))
		nexps = explist1(ls, &e);
	else {
		e.k = VVOID;
		nexps = 0;
	}
	adjust_assign(ls, nvars, nexps, &e);
	adjustlocalvars(ls, nvars);
}

// 0x712fb0
static void localfunc(LexState* ls)
{
	expdesc v, b;
	FuncState* fs = ls->fs;
	new_localvar(ls, str_checkname(ls), 0);
	init_exp(&v, VLOCAL, fs->freereg);
	luaK_reserveregs(fs, 1);
	adjustlocalvars(ls, 1);
	body(ls, &b, 0, ls->linenumber, 1);
	luaK_storevar(fs, &v, &b);
	// debug information will only see the variable after this point!
	getlocvar(fs, fs->nactvar - 1).startpc = fs->pc;
}

// 0x713d80: funcname -> NAME {field} [`:' NAME]; the result is true for a method. A method's first name is the class
// that `super' refers to
static int funcname(LexState* ls, expdesc* v)
{
	TString* name;
	int needself = 0;
	if (ls->t.token != TK_NAME)
		luaX_syntaxerror(ls, "<name> expected");
	name = ls->t.seminfo.ts;
	luaX_next(ls);
	singlevaraux(ls->fs, name, v, 1);
	while (ls->t.token == '.')
		field(ls, v);
	if (ls->t.token == ':') {
		ls->fs->curclass = name;
		needself = 1;
		field(ls, v);
	}
	return needself;
}

// 0x713e80. A global function name is DEFINED (DEFGLOBAL); the exe also stores up to four constant indices of the
// name's parts in the new Proto (+0x54..+0x60), which the port's Proto has no room for
static void funcstat(LexState* ls, int line)
{
	// funcstat -> FUNCTION funcname body
	int needself;
	expdesc v, b;
	luaX_next(ls);  // skip FUNCTION
	needself = funcname(ls, &v);
	body(ls, &b, needself, line, 1);
	ls->fs->curclass = NULL;
	if (v.k == VGLOBAL)
		luaK_defvar(ls->fs, &v, &b);
	else
		luaK_storevar(ls->fs, &v, &b);
	luaK_fixline(ls->fs, line);
}

// 0x714120
static void retstat(LexState* ls)
{
	// stat -> RETURN explist
	FuncState* fs = ls->fs;
	expdesc e;
	int first, nret;  // registers with returned values
	luaX_next(ls);  // skip RETURN
	if (block_follow(ls->t.token) || ls->t.token == ';')
		first = nret = 0;  // return no values
	else {
		nret = explist1(ls, &e);  // optional return values
		if (e.k == VCALL) {
			Instruction* i = &getcode(fs, &e);
			SETARG_C(*i, 0);  // luaK_setcallreturns(MULTRET)
			if (nret == 1) {  // tail call?
				if (GET_OPCODE(*i) == OP_CALLSELFORGLOBAL)
					SET_OPCODE(*i, OP_TAILCALLSELFORGLOBAL);
				else
					SET_OPCODE(*i, OP_TAILCALL);
			}
			first = fs->nactvar;
			nret = LUA_MULTRET;  // return all values
		}
		else {
			if (nret == 1)  // only one single value?
				first = luaK_exp2anyreg(fs, &e);
			else {
				luaK_exp2nextreg(fs, &e);  // values must go to the `stack'
				first = fs->nactvar;  // return all `active' values
			}
		}
	}
	luaK_codeABC(fs, OP_RETURN, first, nret + 1, 0);
}

// 0x714240 / 0x7142e0: the jump goes to the end of the loop (break) or its next iteration (continue). PopLua
// never patches the `continue' jumps of while and repeat loops (they stay unresolved, a jump to itself)
static void breakstat(LexState* ls)
{
	FuncState* fs = ls->fs;
	BlockCnt* bl = fs->bl;
	int upval = 0;
	luaX_next(ls);  // skip BREAK
	while (bl && !bl->isbreakable) {
		upval |= bl->upval;
		bl = bl->previous;
	}
	if (!bl)
		luaX_syntaxerror(ls, "no loop to break");
	if (upval)
		luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0);
	luaK_concat(fs, &bl->breaklist, luaK_jump(fs));
}

static void continuestat(LexState* ls)
{
	FuncState* fs = ls->fs;
	BlockCnt* bl = fs->bl;
	int upval = 0;
	luaX_next(ls);  // skip CONTINUE
	while (bl && !bl->isbreakable) {
		upval |= bl->upval;
		bl = bl->previous;
	}
	if (!bl)
		luaX_syntaxerror(ls, "no loop to continue");
	if (upval)
		luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0);
	luaK_concat(fs, &bl->continuelist, luaK_jump(fs));
}

// 0x713670: class NAME [`:' NAME] [`(' parlist `)'] block. The class table, its metatable and `new' (a function
// written here instruction by instruction: it makes the object, gives it the class's metatable and calls
// Init with the arguments) are set up by fixed code; the constructor Init is the block that follows
static void classstat(LexState* ls)
{
	FuncState* fs;
	FuncState new_fs;
	TString* name;
	TString* parent = NULL;
	expdesc e;
	int base, kname, kclassname, knew, kmt, kindex;
	if (ls->t.token != TK_NAME)
		luaX_syntaxerror(ls, "<name> expected");
	name = ls->t.seminfo.ts;
	luaX_next(ls);
	if (ls->t.token == ':' && testnext(ls, ':')) {
		if (ls->t.token != TK_NAME)
			luaX_syntaxerror(ls, "<name> expected");
		parent = ls->t.seminfo.ts;
		luaX_next(ls);
	}
	fs = ls->fs;
	base = fs->freereg;
	luaK_reserveregs(fs, 5);
	kname = luaK_stringK(fs, name);
	kclassname = literalK(ls, "classname");
	knew = literalK(ls, "new");
	kmt = literalK(ls, "mt");
	kindex = literalK(ls, "__index");
	luaK_codeABC(fs, OP_NEWTABLE, base, 0, 0);
	luaK_codeABx(fs, OP_DEFGLOBAL, base, kname);
	luaK_codeABx(fs, OP_LOADK, base + 1, kclassname);
	luaK_codeABx(fs, OP_LOADK, base + 2, kname);
	luaK_codeABC(fs, OP_SETTABLE, base, base + 1, base + 2);  // class.classname = name
	luaK_codeABC(fs, OP_NEWTABLE, base + 1, 0, 1);
	luaK_codeABx(fs, OP_LOADK, base + 4, kindex);
	luaK_codeABC(fs, OP_SETTABLE, base + 1, base + 4, base);  // mt.__index = class
	luaK_codeABx(fs, OP_LOADK, base + 4, kmt);
	luaK_codeABC(fs, OP_SETTABLE, base, base + 4, base + 1);  // class.mt = mt
	if (parent != NULL) {
		int kparent = luaK_stringK(fs, parent);
		int kgetclass = literalK(ls, "getclass");
		int kbase = literalK(ls, "base");
		int ksetmt = literalK(ls, "setmetatable");
		luaK_codeABx(fs, OP_GETGLOBAL, base + 1, kgetclass);
		luaK_codeABx(fs, OP_LOADK, base + 2, kparent);
		luaK_codeABC(fs, OP_CALL, base + 1, 2, 2);  // getclass(parent)
		luaK_codeABx(fs, OP_LOADK, base + 4, kbase);
		luaK_codeABC(fs, OP_SETTABLE, base, base + 4, base + 1);  // class.base = parent class
		luaK_codeABx(fs, OP_LOADK, base + 4, kmt);
		luaK_codeABC(fs, OP_GETTABLE, base + 3, base + 1, base + 4);
		luaK_codeABx(fs, OP_GETGLOBAL, base + 1, ksetmt);
		luaK_codeABC(fs, OP_MOVE, base + 2, base, 0);
		luaK_codeABC(fs, OP_CALL, base + 1, 3, 1);  // setmetatable(class, parent.mt)
	}
	// the function `new(self, ...)', in 14 instructions and the final return: obj = {}; setprot(obj); setmetatable(obj, self.mt);
	// obj:Init(unpack(arg)); return obj
	open_func(ls, &new_fs);
	new_fs.f->lineDefined = ls->linenumber;
	new_fs.f->selfflag = 1;
	new_fs.f->maxstacksize = 7;
	new_localvarstr(ls, "self");
	parlist_end(ls, 0, 1);
	new_localvarstr(ls, "obj");
	literalK(ls, "setprot");      // K0
	literalK(ls, "setmetatable");  // K1
	literalK(ls, "mt");           // K2
	literalK(ls, "Init");         // K3
	literalK(ls, "unpack");       // K4
	{
		static const Instruction code[] = {
			0x0200000a,  // NEWTABLE  2 0 0
			0x03000005,  // GETGLOBAL 3 K0
			0x04010000,  // MOVE      4 2
			0x0301005b,  // CALL      3 2 1
			0x03000045,  // GETGLOBAL 3 K1
			0x04010000,  // MOVE      4 2
			0x05003f06,  // GETTABLE  5 0 K2
			0x0301805b,  // CALL      3 3 1
			0x03013f4b,  // SELF      3 2 K3
			0x05000105,  // GETGLOBAL 5 K4
			0x06008000,  // MOVE      6 1
			0x0501001b,  // CALL      5 2 0
			0x0300005b,  // CALL      3 0 1
			0x0201001d,  // RETURN    2 2
		};
		size_t i;
		for (i = 0; i < sizeof code / sizeof code[0]; i++)
			luaK_code(ls->fs, code[i], ls->lastline);
	}
	close_func(ls);
	pushclosure(ls, &new_fs, &e);
	luaK_exp2reg(fs, &e, base + 1);
	luaK_codeABx(fs, OP_LOADK, base + 4, knew);
	luaK_codeABC(fs, OP_SETTABLE, base, base + 4, base + 1);  // class.new = the function
	fs->freereg -= 5;
	if (ls->t.token != '(' && ls->t.token != '{') {
		if (ls->t.token == ';' && testnext(ls, ';'))
			return;  // no constructor
		luaX_syntaxerror(ls, "syntax error");
	}
	{
		LHS_assign var;
		int base2 = fs->freereg;
		int kinit;
		luaK_reserveregs(fs, 2);
		kinit = literalK(ls, "Init");
		luaK_codeABx(fs, OP_GETGLOBAL, base2, kname);
		luaK_codeABx(fs, OP_LOADK, base2 + 1, kinit);
		init_exp(&var.v, VINDEXED, base2);
		var.v.aux = base2 + 1;
		fs->curclass = name;
		body(ls, &e, 1, ls->linenumber, ls->t.token == '(');
		ls->fs->curclass = NULL;
		luaK_defvar(ls->fs, &var.v, &e);  // class.Init = the constructor
	}
}

// 0x714380
static int statement(LexState* ls)
{
	int line = ls->linenumber;  // may be needed for error messages
	switch (ls->t.token) {
	case TK_IF:  // stat -> ifstat
		ifstat(ls, line);
		return 0;
	case TK_WHILE:  // stat -> whilestat
		whilestat(ls, line);
		return 0;
	case TK_DO:  // stat -> DO block END
		luaX_next(ls);  // skip DO
		block(ls, 1);
		check_match(ls, TK_END, TK_DO, line);
		return 0;
	case '{':  // stat -> `{' block `}'
		luaX_next(ls);
		block(ls, 1);
		check_match(ls, '}', '{', line);
		return 0;
	case TK_FOR:  // stat -> forstat
		forstat(ls, line);
		return 0;
	case TK_REPEAT:  // stat -> repeatstat
		repeatstat(ls, line);
		return 0;
	case TK_FUNCTION:  // stat -> funcstat
		if (ls->fs->prev != NULL)
			luaX_syntaxerror(ls, "nested functions must be defined as local");
		funcstat(ls, line);
		return 0;
	case TK_CONST:
		luaX_next(ls);
		conststat(ls);
		return 0;
	case TK_DEF:
		luaX_next(ls);
		defstat(ls);
		return 0;
	case TK_CLASS:
		luaX_next(ls);
		classstat(ls);
		return 0;
	case TK_SUPER:
		superstat(ls);
		return 0;
	case TK_LOCAL:  // stat -> localstat
		luaX_next(ls);  // skip LOCAL
		if (ls->t.token == TK_FUNCTION && testnext(ls, TK_FUNCTION))
			localfunc(ls);
		else
			localstat(ls);
		return 0;
	case TK_RETURN:  // stat -> retstat
		retstat(ls);
		return 1;  // must be last statement
	case TK_BREAK:  // stat -> breakstat
		breakstat(ls);
		return 1;  // must be last statement
	case TK_CONTINUE:  // stat -> continuestat
		continuestat(ls);
		return 1;  // must be last statement
	default:
		exprstat(ls);
		return 0;  // to avoid warnings
	}
}

// 0x714600
static void chunk(LexState* ls, int multi)
{
	// chunk -> { stat [`;'] }; with `multi' false, only one statement
	int islast;
	if (++ls->nestlevel > MAXSYNTAXLEVELS)
		luaX_syntaxerror(ls, "too many syntax levels");
	while (!block_follow(ls->t.token)) {
		islast = statement(ls);
		testnext(ls, ';');
		ls->fs->freereg = ls->fs->nactvar;  // free registers
		if (!multi || islast)
			break;
	}
	ls->nestlevel--;
}

// 0x70f310
Proto* luaY_parser(lua_State* L, ZIO* z)
{
	LexState lexstate;
	FuncState funcstate;
	if (luaS_newliteral(L, "and")->reserved == 0)
		luaX_init(L);  // the reserved words are not in the string table yet
	luaX_setinput(L, &lexstate, z, luaS_new(L, zname(z)));
	open_func(&lexstate, &funcstate);
	luaX_next(&lexstate);  // read first token
	chunk(&lexstate, 1);
	if (lexstate.t.token != TK_EOS)
		luaX_syntaxerror(&lexstate, "<eof> expected");
	close_func(&lexstate);
	return funcstate.f;
}
