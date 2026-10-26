// PopLua's instruction set: Lua 5.0's encoding (OP:6 C:9 B:9 A:8 from the low bit) and opcodes, with MOD and
// SIZ inserted after DIV/NOT and twelve PopLua opcodes before CLOSURE (docs/POPLUA.md).
#ifndef POPLUA_LOPCODES_H
#define POPLUA_LOPCODES_H

#include "lcore.h"

enum OpMode { iABC, iABx, iAsBx };

#define SIZE_C 9
#define SIZE_B 9
#define SIZE_Bx (SIZE_C + SIZE_B)
#define SIZE_A 8
#define SIZE_OP 6
#define POS_C SIZE_OP
#define POS_B (POS_C + SIZE_C)
#define POS_Bx POS_C
#define POS_A (POS_B + SIZE_B)

#define MAXARG_Bx ((1 << SIZE_Bx) - 1)
#define MAXARG_sBx (MAXARG_Bx >> 1)
#define MAXARG_A ((1 << SIZE_A) - 1)
#define MAXARG_B ((1 << SIZE_B) - 1)
#define MAXARG_C ((1 << SIZE_C) - 1)

#define MASK1(n, p) ((~((~(Instruction)0) << n)) << p)
#define MASK0(n, p) (~MASK1(n, p))

#define GET_OPCODE(i) ((OpCode)(((i) >> 0) & MASK1(SIZE_OP, 0)))
#define SET_OPCODE(i, o) ((i) = (((i) & MASK0(SIZE_OP, 0)) | (Instruction)(o)))
#define GETARG_A(i) ((int)((i) >> POS_A))
#define SETARG_A(i, u) ((i) = (((i) & MASK0(SIZE_A, POS_A)) | ((Instruction)(u) << POS_A)))
#define GETARG_B(i) ((int)(((i) >> POS_B) & MASK1(SIZE_B, 0)))
#define SETARG_B(i, b) ((i) = (((i) & MASK0(SIZE_B, POS_B)) | ((Instruction)(b) << POS_B)))
#define GETARG_C(i) ((int)(((i) >> POS_C) & MASK1(SIZE_C, 0)))
#define SETARG_C(i, b) ((i) = (((i) & MASK0(SIZE_C, POS_C)) | ((Instruction)(b) << POS_C)))
#define GETARG_Bx(i) ((int)(((i) >> POS_Bx) & MASK1(SIZE_Bx, 0)))
#define SETARG_Bx(i, b) ((i) = (((i) & MASK0(SIZE_Bx, POS_Bx)) | ((Instruction)(b) << POS_Bx)))
#define GETARG_sBx(i) (GETARG_Bx(i) - MAXARG_sBx)
#define SETARG_sBx(i, b) SETARG_Bx((i), (unsigned int)((b) + MAXARG_sBx))

#define CREATE_ABC(o, a, b, c) ((Instruction)(o) | ((Instruction)(a) << POS_A) | ((Instruction)(b) << POS_B) | ((Instruction)(c) << POS_C))
#define CREATE_ABx(o, a, bc) ((Instruction)(o) | ((Instruction)(a) << POS_A) | ((Instruction)(bc) << POS_Bx))

#define NO_REG MAXARG_A

enum OpCode {
	OP_MOVE,        // A B     R(A) := R(B)
	OP_LOADK,       // A Bx    R(A) := Kst(Bx)
	OP_LOADBOOL,    // A B C   R(A) := (Bool)B; if (C) PC++
	OP_LOADNIL,     // A B     R(A) := ... := R(B) := nil
	OP_GETUPVAL,    // A B     R(A) := UpValue[B]
	OP_GETGLOBAL,   // A Bx    R(A) := Gbl[Kst(Bx)]
	OP_GETTABLE,    // A B C   R(A) := R(B)[RK(C)]
	OP_SETGLOBAL,   // A Bx    Gbl[Kst(Bx)] := R(A)
	OP_SETUPVAL,    // A B     UpValue[B] := R(A)
	OP_SETTABLE,    // A B C   R(A)[RK(B)] := RK(C)
	OP_NEWTABLE,    // A B C   R(A) := {} (size = B,C)
	OP_SELF,        // A B C   R(A+1) := R(B); R(A) := R(B)[RK(C)] (a method)
	OP_ADD,         // A B C   R(A) := RK(B) + RK(C)
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_POW,
	OP_MOD,         // PopLua: int % int
	OP_UNM,
	OP_NOT,
	OP_SIZ,         // PopLua: R(A) := *R(B)
	OP_CONCAT,      // A B C   R(A) := R(B).. ... ..R(C)
	OP_JMP,         // sBx     PC += sBx
	OP_EQ,          // A B C   if ((RK(B) == RK(C)) ~= A) then pc++
	OP_LT,
	OP_LE,
	OP_TEST,        // A B C   if (R(B) <=> C) then R(A) := R(B) else pc++
	OP_CALL,        // A B C   R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
	OP_TAILCALL,    // A B C   return R(A)(R(A+1), ... ,R(A+B-1))
	OP_RETURN,      // A B     return R(A), ... ,R(A+B-2)
	OP_FORLOOP,     // A sBx   R(A)+=R(A+2); if R(A) <?= R(A+1) then PC+= sBx
	OP_TFORLOOP,    // A C     R(A+2), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+2) ~= nil then pc++
	OP_TFORPREP,    // A sBx   if type(R(A)) == table then R(A+1):=R(A), R(A):=next; PC += sBx
	OP_SETLIST,     // A Bx    R(A)[Bx-Bx%FPF+i-1] := R(A+i), 1 <= i <= Bx%FPF+1 (0-based keys)
	OP_SETLISTO,    // A Bx
	OP_CLOSE,       // A       close all variables in the stack up to (>=) R(A)
	OP_ALTSELF,     // A B C   R(A+1) := (the value the preceding MOVE moved); R(A) := R(B)[RK(C)] (a method)
	OP_CONSTGLOBAL, // A Bx    define constant Gbl[Kst(Bx)] := R(A)
	OP_CONSTTABLE,  // A B C   define constant R(A)[RK(B)] := RK(C)
	OP_DEFGLOBAL,   // A Bx    define Gbl[Kst(Bx)] := R(A)
	OP_DEFTABLE,    // A B C   define R(A)[RK(B)] := RK(C)
	OP_SETSELFORGLOBAL,      // A Bx   self[Kst(Bx)] := R(A) if self has it, else Gbl[Kst(Bx)] := R(A)
	OP_GETSELFORGLOBAL,      // A Bx   R(A) := self[Kst(Bx)] if self has it, else Gbl[Kst(Bx)]
	OP_SELFORGLOBAL,         // A B C  R(A+1) := R(B); R(A) := self[RK(C)] or Gbl[RK(C)] (a method if it is one)
	OP_CALLSELFORGLOBAL,     // A B C  CALL; without a method the self argument R(A+1) is dropped
	OP_TAILCALLSELFORGLOBAL, // A B C  TAILCALL, the same way
	OP_INT,         // A B     R(A) := int(R(B))
	OP_BREAK,       //         nothing (a marker for the debugger)
	OP_CLOSURE,     // A Bx    R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
	NUM_OPCODES
};

#define ISK(x) ((x) >= MAXSTACK)

#endif
