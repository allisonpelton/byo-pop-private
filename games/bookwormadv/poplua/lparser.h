// The parser's state (0x70e430-0x714600): Lua 5.0.2's lparser.h with PopLua's additions (`continue' lists in
// the blocks, the class being defined in the function state).
#ifndef POPLUA_LPARSER_H
#define POPLUA_LPARSER_H

#include "lcode.h"

#define MAXVARS 200
#define MAXUPVALUES 32
#define MAXPARAMS 100
#define MAXSYNTAXLEVELS 200

// nodes for block list (list of active blocks)
struct BlockCnt {
	BlockCnt* previous;  // chain
	int breaklist;       // list of jumps out of this loop
	int continuelist;    // PopLua: list of jumps to the loop's continue point
	int nactvar;         // # active local variables outside the block
	int upval;           // true if some variable in the block is an upvalue
	int isbreakable;     // true if `block' is a loop
};

// state needed to generate code for a given function
struct FuncState {
	Proto* f;           // current function header
	Table* h;           // table to find (and reuse) elements in `k'
	FuncState* prev;    // enclosing function
	LexState* ls;       // lexical state
	lua_State* L;       // copy of the Lua state
	BlockCnt* bl;       // chain of current blocks
	int pc;             // next position to code (equivalent to `ncode')
	int lasttarget;     // `pc' of last `jump target'
	int jpc;            // list of pending jumps to `pc'
	int freereg;        // first free register
	int nk;             // number of elements in `k'
	int np;             // number of elements in `p'
	int nlocvars;       // number of elements in `locvars'
	int nactvar;        // number of active local variables
	expdesc upvalues[MAXUPVALUES];  // upvalues
	int actvar[MAXVARS];            // declared-variable stack
	TString* curclass;  // PopLua (the exe's FuncState+0x5d8): name of the class whose method is being compiled
};

#endif
