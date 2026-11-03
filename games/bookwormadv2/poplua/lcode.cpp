// The code generator (0x71d260-0x71ed90): Lua 5.0.2's lcode.c with PopLua's additions: the expression kind
// VSELFGLOBAL (names inside methods: GETSELFORGLOBAL / SETSELFORGLOBAL), the `def' and `const' stores
// (DEFGLOBAL / DEFTABLE, CONSTGLOBAL / CONSTTABLE), ALTSELF, the `int' and `*' (size) unary operators, `%',
// integer constants (VK of an int), and integer folding of unary minus and `int'.
#include <cstdlib>

#include "lparser.h"

#define hasjumps(e) ((e)->t != (e)->f)

// only the jump-testing opcodes have the T bit of the exe's opcode mode table (0x79baa0)
static int testOpModeT(OpCode op)
{
	return op == OP_EQ || op == OP_LT || op == OP_LE || op == OP_TEST || op == OP_TFORLOOP;
}

// 0x71d260
void luaK_nil(FuncState* fs, int from, int n)
{
	Instruction* previous;
	if (fs->pc > fs->lasttarget) {  // no jumps to current position?
		previous = &fs->f->code[fs->pc - 1];
		if (GET_OPCODE(*previous) == OP_LOADNIL) {
			int pfrom = GETARG_A(*previous);
			int pto = GETARG_B(*previous);
			if (pfrom <= from && from <= pto + 1) {  // can connect both?
				if (from + n - 1 > pto)
					SETARG_B(*previous, from + n - 1);
				return;
			}
		}
	}
	luaK_codeABC(fs, OP_LOADNIL, from, from + n - 1, 0);  // else no optimization
}

// 0x71d2f0
int luaK_jump(FuncState* fs)
{
	int jpc = fs->jpc;  // save list of jumps to here
	int j;
	fs->jpc = NO_JUMP;
	j = luaK_codeAsBx(fs, OP_JMP, 0, NO_JUMP);
	luaK_concat(fs, &j, jpc);  // keep them on hold
	return j;
}

// 0x71d330
static int luaK_condjump(FuncState* fs, OpCode op, int A, int B, int C)
{
	luaK_codeABC(fs, op, A, B, C);
	return luaK_jump(fs);
}

static int getjump(FuncState* fs, int pc)
{
	int offset = GETARG_sBx(fs->f->code[pc]);
	if (offset == NO_JUMP)  // point to itself represents end of list
		return NO_JUMP;     // end of list
	else
		return (pc + 1) + offset;  // turn offset into absolute position
}

// 0x71d390
static void fixjump(FuncState* fs, int pc, int dest)
{
	Instruction* jmp = &fs->f->code[pc];
	int offset = dest - (pc + 1);
	if (abs(offset) > MAXARG_sBx)
		luaX_syntaxerror(fs->ls, "control structure too long");
	SETARG_sBx(*jmp, offset);
}

// 0x71d6d0
void luaK_concat(FuncState* fs, int* l1, int l2)
{
	if (l2 == NO_JUMP)
		return;
	if (*l1 == NO_JUMP)
		*l1 = l2;
	else {
		int list = *l1;
		int next;
		while ((next = getjump(fs, list)) != NO_JUMP)  // find last element
			list = next;
		fixjump(fs, list, l2);
	}
}

// the instruction that controls the jump at pc (a test, or the jump itself)
static Instruction* getjumpcontrol(FuncState* fs, int pc)
{
	Instruction* pi = &fs->f->code[pc];
	if (pc >= 1 && testOpModeT(GET_OPCODE(*(pi - 1))))
		return pi - 1;
	else
		return pi;
}

// 0x71d440: does any jump of the list produce a value (anything but a TEST of the given sense)?
static int need_value(FuncState* fs, int list, int cond)
{
	for (; list != NO_JUMP; list = getjump(fs, list)) {
		Instruction i = *getjumpcontrol(fs, list);
		if (GET_OPCODE(i) != OP_TEST || GETARG_A(i) != cond)
			return 1;
	}
	return 0;  // not found
}

static void patchtestreg(Instruction* i, int reg)
{
	if (reg != NO_REG)
		SETARG_A(*i, reg);
	else  // no register to put value: change the TEST to a jump-test-without-value
		SETARG_A(*i, GETARG_B(*i));
}

// 0x71d4f0
static void luaK_patchlistaux(FuncState* fs, int list, int ttarget, int treg, int ftarget, int freg, int dtarget)
{
	while (list != NO_JUMP) {
		int next = getjump(fs, list);
		Instruction* i = getjumpcontrol(fs, list);
		if (GET_OPCODE(*i) != OP_TEST) {
			fixjump(fs, list, dtarget);  // jump to default target
		}
		else {
			if (GETARG_C(*i)) {
				patchtestreg(i, treg);
				fixjump(fs, list, ttarget);
			}
			else {
				patchtestreg(i, freg);
				fixjump(fs, list, ftarget);
			}
		}
		list = next;
	}
}

// 0x71d640
static void dischargejpc(FuncState* fs)
{
	luaK_patchlistaux(fs, fs->jpc, fs->pc, NO_REG, fs->pc, NO_REG, fs->pc);
	fs->jpc = NO_JUMP;
}

void luaK_patchlist(FuncState* fs, int list, int target)
{
	if (target == fs->pc)
		luaK_patchtohere(fs, list);
	else
		luaK_patchlistaux(fs, list, target, NO_REG, target, NO_REG, target);
}

void luaK_patchtohere(FuncState* fs, int list)
{
	luaK_getlabel(fs);
	luaK_concat(fs, &fs->jpc, list);
}

int luaK_getlabel(FuncState* fs)
{
	fs->lasttarget = fs->pc;
	return fs->pc;
}

// 0x71eb30
int luaK_code(FuncState* fs, Instruction i, int line)
{
	Proto* f = fs->f;
	dischargejpc(fs);  // `pc' will change
	// put new instruction in code array
	luaM_growvector(fs->L, f->code, fs->pc, f->sizecode, Instruction, MAX_INT, "code size overflow");
	f->code[fs->pc] = i;
	// save corresponding line information
	luaM_growvector(fs->L, f->lineinfo, fs->pc, f->sizelineinfo, int, MAX_INT, "code size overflow");
	f->lineinfo[fs->pc] = line;
	return fs->pc++;
}

// 0x71ebe0
int luaK_codeABC(FuncState* fs, OpCode o, int a, int b, int c)
{
	return luaK_code(fs, CREATE_ABC(o, a, b, c), fs->ls->lastline);
}

// 0x71ec10
int luaK_codeABx(FuncState* fs, OpCode o, int a, unsigned int bc)
{
	return luaK_code(fs, CREATE_ABx(o, a, bc), fs->ls->lastline);
}

// (inlined in the exe)
void luaK_fixline(FuncState* fs, int line)
{
	fs->f->lineinfo[fs->pc - 1] = line;
}

// 0x71d740: luaK_checkstack and luaK_reserveregs in one
void luaK_checkstack(FuncState* fs, int n)
{
	int newstack = fs->freereg + n;
	if (newstack > fs->f->maxstacksize) {
		if (newstack >= MAXSTACK)
			luaX_syntaxerror(fs->ls, "function or expression too complex");
		fs->f->maxstacksize = (lu_byte)newstack;
	}
}

void luaK_reserveregs(FuncState* fs, int n)
{
	luaK_checkstack(fs, n);
	fs->freereg += n;
}

// 0x71d7a0
static void freeexp(FuncState* fs, expdesc* e)
{
	if (e->k == VNONRELOC && e->info >= fs->nactvar && e->info < MAXSTACK)
		fs->freereg--;
}

// ---- constants ---------------------------------------------------------------------------------------

// 0x71d7c0: the constants table `h' maps a constant to its index in `k'
static int addk(FuncState* fs, TObject* k, TObject* v)
{
	TObject* idx = luaH_set(fs->L, fs->h, k);
	if (ttisnumber(idx))  // already there
		return (int)nvalue(idx);
	else {  // constant not found; create a new entry
		Proto* f = fs->f;
		int oldsize = f->sizek;
		setnvalue(idx, (lua_Number)fs->nk);
		luaM_growvector(fs->L, f->k, fs->nk, f->sizek, TObject, MAXARG_Bx, "constant table overflow");
		while (oldsize < f->sizek)
			setnilvalue(&f->k[oldsize++]);
		setobj2n(&f->k[fs->nk], v);
		return fs->nk++;
	}
}

// 0x71d8b0
int luaK_stringK(FuncState* fs, TString* s)
{
	TObject o;
	setsvalue(&o, s);
	return addk(fs, &o, &o);
}

// 0x71d8d0
int luaK_numberK(FuncState* fs, lua_Number r)
{
	TObject o;
	setnvalue(&o, r);
	return addk(fs, &o, &o);
}

// 0x71d900
int luaK_intK(FuncState* fs, lua_Int n)
{
	TObject o;
	setivalue(&o, n);
	return addk(fs, &o, &o);
}

// 0x71d920
static int nil_constant(FuncState* fs)
{
	TObject k, v;
	setnilvalue(&v);
	sethvalue(&k, fs->h);  // cannot use nil as key; instead use table itself to represent nil
	return addk(fs, &k, &v);
}

// ---- expressions -------------------------------------------------------------------------------------

// 0x71d990
void luaK_dischargevars(FuncState* fs, expdesc* e)
{
	switch (e->k) {
	case VLOCAL:
		e->k = VNONRELOC;
		break;
	case VUPVAL:
		e->info = luaK_codeABC(fs, OP_GETUPVAL, 0, e->info, 0);
		e->k = VRELOCABLE;
		break;
	case VGLOBAL:
		e->info = luaK_codeABx(fs, OP_GETGLOBAL, 0, e->info);
		e->k = VRELOCABLE;
		break;
	case VSELFGLOBAL:
		e->info = luaK_codeABx(fs, OP_GETSELFORGLOBAL, 0, e->info);
		e->k = VRELOCABLE;
		break;
	case VINDEXED:
		if (e->aux >= fs->nactvar && e->aux < MAXSTACK)
			fs->freereg--;
		if (e->info >= fs->nactvar && e->info < MAXSTACK)
			fs->freereg--;
		e->info = luaK_codeABC(fs, OP_GETTABLE, 0, e->info, e->aux);
		e->k = VRELOCABLE;
		break;
	case VCALL:
		luaK_setcallreturns(fs, e, 1);
		break;
	default:
		break;  // there is one value available (somewhere)
	}
}

// 0x71db00
static void discharge2reg(FuncState* fs, expdesc* e, int reg)
{
	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VNIL:
		luaK_nil(fs, reg, 1);
		break;
	case VFALSE:
	case VTRUE:
		luaK_codeABC(fs, OP_LOADBOOL, reg, e->k == VTRUE, 0);
		break;
	case VK:
		luaK_codeABx(fs, OP_LOADK, reg, e->info);
		break;
	case VRELOCABLE: {
		Instruction* pc = &getcode(fs, e);
		SETARG_A(*pc, reg);
		break;
	}
	case VNONRELOC:
		if (reg != e->info)
			luaK_codeABC(fs, OP_MOVE, reg, e->info, 0);
		break;
	default:
		return;  // nothing to do... (VVOID, VJMP)
	}
	e->info = reg;
	e->k = VNONRELOC;
}

// 0x71dc10
static void discharge2anyreg(FuncState* fs, expdesc* e)
{
	if (e->k != VNONRELOC) {
		luaK_reserveregs(fs, 1);
		discharge2reg(fs, e, fs->freereg - 1);
	}
}

// 0x71dc60
void luaK_exp2reg(FuncState* fs, expdesc* e, int reg)
{
	discharge2reg(fs, e, reg);
	if (e->k == VJMP)
		luaK_concat(fs, &e->t, e->info);  // put this jump in `t' list
	if (hasjumps(e)) {
		int final;  // position after whole expression
		int p_f = NO_JUMP;  // position of an eventual PUSH false
		int p_t = NO_JUMP;  // position of an eventual PUSH true
		if (need_value(fs, e->t, 1) || need_value(fs, e->f, 0)) {
			int fj = NO_JUMP;  // first jump (over LOAD ops.)
			if (e->k != VJMP)
				fj = luaK_jump(fs);
			luaK_getlabel(fs);
			p_f = luaK_codeABC(fs, OP_LOADBOOL, reg, 0, 1);
			luaK_getlabel(fs);
			p_t = luaK_codeABC(fs, OP_LOADBOOL, reg, 1, 0);
			luaK_patchtohere(fs, fj);
		}
		final = luaK_getlabel(fs);
		luaK_patchlistaux(fs, e->f, p_f, NO_REG, final, reg, p_f);
		luaK_patchlistaux(fs, e->t, final, reg, p_t, NO_REG, p_t);
	}
	e->f = e->t = NO_JUMP;
	e->info = reg;
	e->k = VNONRELOC;
}

// 0x71dd90
void luaK_exp2nextreg(FuncState* fs, expdesc* e)
{
	luaK_dischargevars(fs, e);
	freeexp(fs, e);
	luaK_reserveregs(fs, 1);
	luaK_exp2reg(fs, e, fs->freereg - 1);
}

// 0x71de00
int luaK_exp2anyreg(FuncState* fs, expdesc* e)
{
	luaK_dischargevars(fs, e);
	if (e->k == VNONRELOC) {
		if (!hasjumps(e))
			return e->info;  // exp is already in a register
		if (e->info >= fs->nactvar) {  // reg. is not a local?
			luaK_exp2reg(fs, e, e->info);  // put value on it
			return e->info;
		}
	}
	luaK_exp2nextreg(fs, e);  // default
	return e->info;
}

// (inlined in the exe)
void luaK_exp2val(FuncState* fs, expdesc* e)
{
	if (hasjumps(e))
		luaK_exp2anyreg(fs, e);
	else
		luaK_dischargevars(fs, e);
}

// 0x71de70
int luaK_exp2RK(FuncState* fs, expdesc* e)
{
	luaK_exp2val(fs, e);
	switch (e->k) {
	case VNIL:
		if (fs->nk + MAXSTACK <= MAXARG_C) {  // constant fit in argC?
			e->info = nil_constant(fs);
			e->k = VK;
			return e->info + MAXSTACK;
		}
		else
			break;
	case VK:
		if (e->info + MAXSTACK <= MAXARG_C)  // constant fit in argC?
			return e->info + MAXSTACK;
		else
			break;
	default:
		break;
	}
	// not a constant in the right range: put it in a register
	return luaK_exp2anyreg(fs, e);
}

// (inlined in the exe) a call with a fixed number of results
void luaK_setcallreturns(FuncState* fs, expdesc* e, int nresults)
{
	if (e->k == VCALL) {  // expression is an open function call?
		SETARG_C(getcode(fs, e), nresults + 1);
		if (nresults == 1) {  // `regular' expression?
			e->k = VNONRELOC;
			e->info = GETARG_A(getcode(fs, e));
		}
	}
}

// (inlined in the exe) t[k]: `k' goes to a register or constant
void luaK_indexed(FuncState* fs, expdesc* t, expdesc* k)
{
	t->aux = luaK_exp2RK(fs, k);
	t->k = VINDEXED;
}

// 0x71e140
void luaK_self(FuncState* fs, expdesc* e, expdesc* key)
{
	int func;
	luaK_exp2anyreg(fs, e);
	freeexp(fs, e);
	func = fs->freereg;
	luaK_reserveregs(fs, 2);
	luaK_codeABC(fs, OP_SELF, func, e->info, luaK_exp2RK(fs, key));
	freeexp(fs, key);
	e->info = func;
	e->k = VNONRELOC;
}

// 0x71e220: FuncTable@SelfTable:Function. The function is fetched from `e' (as a method); the self argument is
// the value of `e2', which a MOVE to the register after the call's frame hands to ALTSELF
void luaK_altself(FuncState* fs, expdesc* e, expdesc* e2, expdesc* key)
{
	int func, selfreg;
	luaK_exp2anyreg(fs, e);
	func = e->info;
	luaK_exp2anyreg(fs, e2);
	selfreg = e2->info;
	(void)selfreg;
	freeexp(fs, e);
	freeexp(fs, e2);
	luaK_reserveregs(fs, 2);
	luaK_codeABC(fs, OP_MOVE, fs->freereg, e2->info, 0);
	luaK_codeABC(fs, OP_ALTSELF, func, e->info, luaK_exp2RK(fs, key));
	freeexp(fs, key);
	e->info = func;
	e->k = VNONRELOC;
}

// 0x71e3b0
static void invertjump(FuncState* fs, expdesc* e)
{
	Instruction* pc = getjumpcontrol(fs, e->info);
	SETARG_A(*pc, !(GETARG_A(*pc)));
}

// 0x71e400
static int jumponcond(FuncState* fs, expdesc* e, int cond)
{
	if (e->k == VRELOCABLE) {
		Instruction ie = getcode(fs, e);
		if (GET_OPCODE(ie) == OP_NOT) {
			fs->pc--;  // remove previous OP_NOT
			return luaK_condjump(fs, OP_TEST, NO_REG, GETARG_B(ie), !cond);
		}
		// else go through
	}
	discharge2anyreg(fs, e);
	freeexp(fs, e);
	return luaK_condjump(fs, OP_TEST, NO_REG, e->info, cond);
}

// 0x71e480
void luaK_goiftrue(FuncState* fs, expdesc* e)
{
	int pc;  // pc of last jump
	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VK:
	case VTRUE:
		pc = NO_JUMP;  // always true; do nothing
		break;
	case VFALSE:
		pc = luaK_jump(fs);  // always jump
		break;
	case VJMP:
		invertjump(fs, e);
		pc = e->info;
		break;
	default:
		pc = jumponcond(fs, e, 0);
		break;
	}
	luaK_concat(fs, &e->f, pc);  // insert last jump in `f' list
}

// 0x71e520
void luaK_goiffalse(FuncState* fs, expdesc* e)
{
	int pc;  // pc of last jump
	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VNIL:
	case VFALSE:
		pc = NO_JUMP;  // always false; do nothing
		break;
	case VTRUE:
		pc = luaK_jump(fs);  // always jump
		break;
	case VJMP:
		pc = e->info;
		break;
	default:
		pc = jumponcond(fs, e, 1);
		break;
	}
	luaK_concat(fs, &e->t, pc);  // insert last jump in `t' list
}

// 0x71e5c0
static void codenot(FuncState* fs, expdesc* e)
{
	luaK_dischargevars(fs, e);
	switch (e->k) {
	case VNIL:
	case VFALSE:
		e->k = VTRUE;
		break;
	case VK:
	case VTRUE:
		e->k = VFALSE;
		break;
	case VJMP:
		invertjump(fs, e);
		break;
	case VRELOCABLE:
	case VNONRELOC:
		discharge2anyreg(fs, e);
		freeexp(fs, e);
		e->info = luaK_codeABC(fs, OP_NOT, 0, e->info, 0);
		e->k = VRELOCABLE;
		break;
	default:
		break;  // cannot happen
	}
	// interchange true and false lists
	{
		int temp = e->f;
		e->f = e->t;
		e->t = temp;
	}
}

// 0x71dee0: `var = e'
void luaK_storevar(FuncState* fs, const expdesc* var, expdesc* ex)
{
	switch (var->k) {
	case VLOCAL:
		freeexp(fs, ex);
		luaK_exp2reg(fs, ex, var->info);
		return;
	case VUPVAL: {
		int e = luaK_exp2anyreg(fs, ex);
		luaK_codeABC(fs, OP_SETUPVAL, e, var->info, 0);
		break;
	}
	case VGLOBAL: {
		int e = luaK_exp2anyreg(fs, ex);
		luaK_codeABx(fs, OP_SETGLOBAL, e, var->info);
		break;
	}
	case VSELFGLOBAL: {
		int e = luaK_exp2anyreg(fs, ex);
		luaK_codeABx(fs, OP_SETSELFORGLOBAL, e, var->info);
		break;
	}
	case VINDEXED: {
		int e = luaK_exp2RK(fs, ex);
		luaK_codeABC(fs, OP_SETTABLE, var->info, var->aux, e);
		break;
	}
	default:
		break;  // invalid var kind to store
	}
	freeexp(fs, ex);
}

// 0x71dfe0: `def var = e' (a global, a field of self in a method, or a table field)
void luaK_defvar(FuncState* fs, const expdesc* cvar, expdesc* ex)
{
	expdesc* var = (expdesc*)cvar;
	switch (var->k) {
	case VGLOBAL: {
		int e = luaK_exp2anyreg(fs, ex);
		luaK_codeABx(fs, OP_DEFGLOBAL, e, var->info);
		break;
	}
	case VSELFGLOBAL: {  // self[name] := e; self is register 0 of the method
		int key, e;
		var->k = VK;
		key = luaK_exp2RK(fs, var);
		e = luaK_exp2RK(fs, ex);
		luaK_codeABC(fs, OP_DEFTABLE, 0, key, e);
		break;
	}
	case VINDEXED: {
		int e = luaK_exp2RK(fs, ex);
		luaK_codeABC(fs, OP_DEFTABLE, var->info, var->aux, e);
		break;
	}
	default:
		break;
	}
	freeexp(fs, ex);
}

// 0x71e090: `const var = e'
void luaK_constvar(FuncState* fs, const expdesc* cvar, expdesc* ex)
{
	expdesc* var = (expdesc*)cvar;
	switch (var->k) {
	case VGLOBAL: {
		int e = luaK_exp2anyreg(fs, ex);
		luaK_codeABx(fs, OP_CONSTGLOBAL, e, var->info);
		break;
	}
	case VSELFGLOBAL: {
		int key, e;
		var->k = VK;
		key = luaK_exp2RK(fs, var);
		e = luaK_exp2RK(fs, ex);
		luaK_codeABC(fs, OP_CONSTTABLE, 0, key, e);
		break;
	}
	case VINDEXED: {
		int e = luaK_exp2RK(fs, ex);
		luaK_codeABC(fs, OP_CONSTTABLE, var->info, var->aux, e);
		break;
	}
	default:
		break;
	}
	freeexp(fs, ex);
}

// ---- operators ---------------------------------------------------------------------------------------

// 0x71e6c0
void luaK_prefix(FuncState* fs, UnOpr op, expdesc* e)
{
	switch (op) {
	case OPR_MINUS: {
		luaK_exp2val(fs, e);
		if (e->k == VK) {  // a numeric constant: fold the sign
			TObject* o = &fs->f->k[e->info];
			if (ttype(o) == LUA_TNUMBER) {
				e->info = luaK_numberK(fs, -nvalue(o));
				return;
			}
			else if (ttype(o) == LUA_TINT) {
				e->info = luaK_intK(fs, -ivalue(o));
				return;
			}
		}
		luaK_exp2anyreg(fs, e);
		freeexp(fs, e);
		e->info = luaK_codeABC(fs, OP_UNM, 0, e->info, 0);
		e->k = VRELOCABLE;
		break;
	}
	case OPR_NOT:
		codenot(fs, e);
		break;
	case OPR_INTCAST: {
		luaK_exp2val(fs, e);
		if (e->k == VK) {
			TObject* o = &fs->f->k[e->info];
			if (ttype(o) == LUA_TNUMBER) {
				e->info = luaK_intK(fs, luaV_ftol(nvalue(o)));
				return;
			}
			else if (ttype(o) == LUA_TINT)
				return;
		}
		luaK_exp2anyreg(fs, e);
		freeexp(fs, e);
		e->info = luaK_codeABC(fs, OP_INT, 0, e->info, 0);
		e->k = VRELOCABLE;
		break;
	}
	case OPR_SIZ:
		luaK_exp2anyreg(fs, e);
		freeexp(fs, e);
		e->info = luaK_codeABC(fs, OP_SIZ, 0, e->info, 0);
		e->k = VRELOCABLE;
		break;
	default:
		break;
	}
}

// 0x71e840
void luaK_infix(FuncState* fs, BinOpr op, expdesc* v)
{
	switch (op) {
	case OPR_AND:
		luaK_goiftrue(fs, v);
		luaK_patchtohere(fs, v->t);
		v->t = NO_JUMP;
		break;
	case OPR_OR:
		luaK_goiffalse(fs, v);
		luaK_patchtohere(fs, v->f);
		v->f = NO_JUMP;
		break;
	case OPR_CONCAT:
		luaK_exp2nextreg(fs, v);  // operand must be on the `stack'
		break;
	default:
		luaK_exp2RK(fs, v);
		break;
	}
}

// 0x71e8c0: arithmetic (an instruction) or comparison (a conditional jump) of two operands
static void codebinop(FuncState* fs, BinOpr op, expdesc* e1, int o1, int o2)
{
	if (op <= OPR_MOD) {
		e1->info = luaK_codeABC(fs, (OpCode)(OP_ADD + op), 0, o1, o2);
		e1->k = VRELOCABLE;
	}
	else {  // comparison
		static const OpCode ops[] = {OP_EQ, OP_EQ, OP_LT, OP_LE, OP_LT, OP_LE};  // NE EQ LT LE GT GE
		int cond = 1;
		if (op >= OPR_GT) {  // a > b is b < a
			int temp = o1;
			o1 = o2;
			o2 = temp;
		}
		else if (op == OPR_NE)
			cond = 0;
		e1->info = luaK_condjump(fs, ops[op - OPR_NE], cond, o1, o2);
		e1->k = VJMP;
	}
}

// 0x71e940
void luaK_posfix(FuncState* fs, BinOpr op, expdesc* e1, expdesc* e2)
{
	switch (op) {
	case OPR_AND:
		luaK_dischargevars(fs, e2);
		luaK_concat(fs, &e1->f, e2->f);
		e1->k = e2->k;
		e1->info = e2->info;
		e1->aux = e2->aux;
		e1->t = e2->t;
		break;
	case OPR_OR:
		luaK_dischargevars(fs, e2);
		luaK_concat(fs, &e1->t, e2->t);
		e1->k = e2->k;
		e1->info = e2->info;
		e1->aux = e2->aux;
		e1->f = e2->f;
		break;
	case OPR_CONCAT:
		luaK_exp2val(fs, e2);
		if (e2->k == VRELOCABLE && GET_OPCODE(getcode(fs, e2)) == OP_CONCAT) {
			freeexp(fs, e1);
			SETARG_B(getcode(fs, e2), e1->info);
			e1->k = e2->k;
			e1->info = e2->info;
		}
		else {
			luaK_exp2nextreg(fs, e2);
			freeexp(fs, e2);
			freeexp(fs, e1);
			e1->info = luaK_codeABC(fs, OP_CONCAT, 0, e1->info, e2->info);
			e1->k = VRELOCABLE;
		}
		break;
	default: {
		int o1 = luaK_exp2RK(fs, e1);
		int o2 = luaK_exp2RK(fs, e2);
		freeexp(fs, e2);
		freeexp(fs, e1);
		codebinop(fs, op, e1, o1, o2);
		break;
	}
	}
}
