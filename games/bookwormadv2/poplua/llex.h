// The lexer (0x71a7b0-0x71c4a0): Lua 5.0.2's llex.h with PopLua's tokens. The reserved words are the 32 of
// 0x79b490 (token numbers 257-288), the other tokens follow them; the compound assignment tokens (`+=' ...) have
// names in the exe's table but the lexer never returns them (the parser reads `+' and `=' separately).
#ifndef POPLUA_LLEX_H
#define POPLUA_LLEX_H

#include "lcore.h"

#define FIRST_RESERVED 257

// the order is the exe's table order: the reserved words' numbers are index + FIRST_RESERVED
enum RESERVED {
	TK_AND = FIRST_RESERVED, TK_BREAK, TK_CONTINUE, TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE,
	TK_CONST, TK_DEF, TK_CLASS, TK_SUPER, TK_COBJ, TK_TABLE, TK_FOR, TK_FUNCTION,
	TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_BANG, TK_OR, TK_REPEAT,
	TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_INTCAST, TK_POS, TK_DBG, TK_WHILE,
	// other terminal symbols
	TK_NAME, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE, TK_NE2, TK_NUMBER, TK_INT, TK_STRING, TK_EOS,
	TK_ADDEQ, TK_SUBEQ, TK_MULEQ, TK_DIVEQ, TK_MODEQ
};

#define NUM_RESERVED ((int)(TK_WHILE - FIRST_RESERVED + 1))

// `int' literals (TK_INT) are in SemInfo.i, numbers in .r, names and strings in .ts
union SemInfo {
	lua_Number r;
	lua_Int i;
	TString* ts;
};

struct Token {
	int token;
	SemInfo seminfo;
};

struct FuncState;

struct LexState {
	int current;      // current character
	int linenumber;   // input line counter
	int lastline;     // line of last token `consumed'
	Token t;          // current token
	Token lookahead;  // look ahead token
	FuncState* fs;    // `FuncState' is private to the parser
	lua_State* L;
	ZIO* z;           // input stream
	TString* source;  // current source name
	int nestlevel;    // levels of nested chunks / expressions
};

void luaX_init(lua_State* L);
void luaX_setinput(lua_State* L, LexState* LS, ZIO* z, TString* source);
int luaX_lex(LexState* LS, SemInfo* seminfo);
void luaX_readhex(LexState* LS, int x, SemInfo* seminfo);  // 0x71ad60: `0x' (the x is given) and the hex digits
void luaX_next(LexState* LS);
void luaX_lookahead(LexState* LS);
const char* luaX_token2str(LexState* LS, int token);
[[noreturn]] void luaX_errorline(LexState* LS, const char* msg, const char* token, int line);
[[noreturn]] void luaX_syntaxerror(LexState* LS, const char* msg);
[[noreturn]] void luaX_errorlimit(LexState* LS, const char* what, int limit);

#endif
