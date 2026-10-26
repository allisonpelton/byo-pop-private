// The code generator (0x71d260-0x71ed90): Lua 5.0.2's lcode.h with PopLua's expression kinds (VSELFGLOBAL) and
// operators (OPR_MOD, the `int' and `*' unary operators).
#ifndef POPLUA_LCODE_H
#define POPLUA_LCODE_H

#include "llex.h"
#include "lopcodes.h"

#define NO_JUMP (-1)

// binary operators (the arithmetic ones in the order of their opcodes, OP_ADD + op)
enum BinOpr {
	OPR_ADD, OPR_SUB, OPR_MULT, OPR_DIV, OPR_POW, OPR_MOD, OPR_CONCAT,
	OPR_NE, OPR_EQ, OPR_LT, OPR_LE, OPR_GT, OPR_GE,
	OPR_AND, OPR_OR,
	OPR_NOBINOPR
};

enum UnOpr { OPR_MINUS, OPR_NOT, OPR_INTCAST, OPR_SIZ, OPR_NOUNOPR };

enum expkind {
	VVOID,        // no value
	VNIL,
	VTRUE,
	VFALSE,
	VK,           // info = index of constant in `k'
	VLOCAL,       // info = local register
	VUPVAL,       // info = index of upvalue in `upvalues'; aux = 1 for an upvalue of the debugger's function
	VGLOBAL,      // info = index of global name in `k'
	VSELFGLOBAL,  // PopLua: info = index of name in `k'; self's field or else the global (GETSELFORGLOBAL)
	VINDEXED,     // info = table register, aux = index register (or `k')
	VJMP,         // info = instruction pc
	VRELOCABLE,   // info = instruction pc
	VNONRELOC,    // info = result register
	VCALL         // info = instruction pc
};

struct expdesc {
	expkind k;
	int info, aux;
	int t;  // patch list of `exit when true'
	int f;  // patch list of `exit when false'
};

struct FuncState;

#define getcode(fs, e) ((fs)->f->code[(e)->info])

#define luaK_codeAsBx(fs, o, A, sBx) luaK_codeABx(fs, o, A, (sBx) + MAXARG_sBx)

#define hasjumps(e) ((e)->t != (e)->f)

int luaK_codeABx(FuncState* fs, OpCode o, int A, unsigned int Bx);
int luaK_codeABC(FuncState* fs, OpCode o, int A, int B, int C);
void luaK_fixline(FuncState* fs, int line);
void luaK_nil(FuncState* fs, int from, int n);
void luaK_reserveregs(FuncState* fs, int n);
void luaK_checkstack(FuncState* fs, int n);
int luaK_stringK(FuncState* fs, TString* s);
int luaK_numberK(FuncState* fs, lua_Number r);
int luaK_intK(FuncState* fs, lua_Int n);
void luaK_dischargevars(FuncState* fs, expdesc* e);
void luaK_exp2reg(FuncState* fs, expdesc* e, int reg);
int luaK_exp2anyreg(FuncState* fs, expdesc* e);
void luaK_exp2nextreg(FuncState* fs, expdesc* e);
void luaK_exp2val(FuncState* fs, expdesc* e);
int luaK_exp2RK(FuncState* fs, expdesc* e);
void luaK_self(FuncState* fs, expdesc* e, expdesc* key);
void luaK_altself(FuncState* fs, expdesc* e, expdesc* e2, expdesc* key);
void luaK_indexed(FuncState* fs, expdesc* t, expdesc* k);
void luaK_goiftrue(FuncState* fs, expdesc* e);
void luaK_goiffalse(FuncState* fs, expdesc* e);
void luaK_storevar(FuncState* fs, const expdesc* var, expdesc* e);
void luaK_defvar(FuncState* fs, const expdesc* var, expdesc* e);
void luaK_constvar(FuncState* fs, const expdesc* var, expdesc* e);
void luaK_setcallreturns(FuncState* fs, expdesc* e, int nresults);
int luaK_jump(FuncState* fs);
void luaK_patchlist(FuncState* fs, int list, int target);
void luaK_patchtohere(FuncState* fs, int list);
void luaK_concat(FuncState* fs, int* l1, int l2);
int luaK_getlabel(FuncState* fs);
void luaK_prefix(FuncState* fs, UnOpr op, expdesc* v);
void luaK_infix(FuncState* fs, BinOpr op, expdesc* v);
void luaK_posfix(FuncState* fs, BinOpr op, expdesc* v1, expdesc* v2);
int luaK_code(FuncState* fs, Instruction i, int line);

#endif
