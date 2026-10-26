// The lexer (0x71a7b0-0x71c4a0): Lua 5.0.2's llex.c with PopLua's changes. Comments are C's (`//' to the end of
// the line, `/* */' which do not nest), there are no long strings and no `--' comments; `&&' and `||' are `and'
// and `or', `!=' is a second `~='; integer literals have their own token (TK_INT, read with strtol), hex
// literals are TK_INT too (strtoul); the compound assignment tokens are not lexed (see lparser.cpp).
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "llex.h"

// the names of the tokens, in the exe's table order (0x79b490, tokens from FIRST_RESERVED on)
static const char* const token2string[] = {
	"and", "break", "continue", "do", "else", "elseif", "end", "false", "const", "def", "class", "super", "cobj",
	"Table", "for", "function", "if", "in", "local", "nil", "not", "!", "or", "repeat", "return", "then", "true",
	"until", "int", "__POS", "#dbg", "while",
	"*name", "..", "...", "==", ">=", "<=", "~=", "!=", "*number", "*int", "*string", "<eof>",
	"+=", "-=", "*=", "/=", "%="};

// PopLua's start line of the chunk being compiled is the exe's lua_State+0x60 (set by the in-game console
// around a compile, 0x6d73b0); it is a global here
int luaX_linebase = 0;

// ---- the buffer: G(L)->buff, grown with luaM_realloc (the exe's checkbuffer) ----------------------------

#define EXTRABUFF 32

static void checkbuffer(LexState* LS, size_t len)
{
	global_State* g = G(LS->L);
	if (g->buffsize < len + EXTRABUFF) {
		size_t n = len + EXTRABUFF;
		if (n < g->buffsize * 2)
			n = g->buffsize * 2;
		g->buff = (char*)luaM_realloc(LS->L, g->buff, g->buffsize, n);
		memset(g->buff + g->buffsize, 0, n - g->buffsize);  // (the port clears it: a token's text can be read stale)
		g->buffsize = n;
	}
}

#define save(LS, c, l) (G((LS)->L)->buff[(l)++] = (char)(c))

#define next(LS) ((LS)->current = zgetc((LS)->z))
#define save_and_next(LS, l) (save(LS, (LS)->current, l), next(LS))

// ---- 0x71a8a0 luaX_init: enters the reserved words in the string table ----------------------------------

void luaX_init(lua_State* L)
{
	int i;
	for (i = 0; i < NUM_RESERVED; i++) {
		TString* ts = luaS_new(L, token2string[i]);
		luaS_fix(ts);
		ts->reserved = (lu_byte)(i + 1);  // reserved word
	}
}

// ---- errors -----------------------------------------------------------------------------------------------

// 0x71a900
void luaX_errorline(LexState* LS, const char* msg, const char* token, int line)
{
	lua_State* L = LS->L;
	char buff[80];
	luaO_chunkid(buff, getstr(LS->source), sizeof(buff));
	luaO_pushfstring(L, "%s:%d: %s near `%s'", buff, line, msg, token);
	luaD_throw(L, LUA_ERRSYNTAX);
}

// the text of a token (inlined in 0x71a970 and the lexer's error calls)
const char* luaX_token2str(LexState* LS, int token)
{
	if (token < FIRST_RESERVED)
		return luaO_pushfstring(LS->L, "%c", token);
	return token2string[token - FIRST_RESERVED];
}

// 0x71a970: the error is reported near the current token
void luaX_syntaxerror(LexState* LS, const char* msg)
{
	const char* lasttoken;
	int tok = LS->t.token;
	if (tok == TK_NAME)
		lasttoken = getstr(LS->t.seminfo.ts);
	else if (tok == TK_NUMBER || tok == TK_INT || tok == TK_STRING) {
		// The exe reads the text of these tokens from the lexer's buffer, the one luaV_concat also uses to join
		// strings, so a message built just before (luaO_pushfstring) may have overwritten it, and then the text
		// is stale bytes (the exe's string library reads on until a NUL). The port reads the text within the
		// buffer only, so a clobbered buffer cannot be read past its end.
		global_State* g = G(LS->L);
		size_t n = 0;
		while (n < g->buffsize && g->buff[n] != '\0')
			n++;
		TString* ts = luaS_newlstr(LS->L, g->buff, n);  // (kept on the stack: no collection runs while parsing)
		setsvalue2s(LS->L->top, ts);
		incr_top(LS->L);
		lasttoken = getstr(ts);
	}
	else
		lasttoken = luaX_token2str(LS, tok);
	luaX_errorline(LS, msg, lasttoken, LS->linenumber);
}

// "too many %s (limit=%d)", reported near the current token
void luaX_errorlimit(LexState* LS, const char* what, int limit)
{
	luaX_syntaxerror(LS, luaO_pushfstring(LS->L, "too many %s (limit=%d)", what, limit));
}

// ---- 0x71aa30 inclinenumber -----------------------------------------------------------------------------

static void inclinenumber(LexState* LS)
{
	next(LS);  // skip `\n'
	LS->linenumber++;
	if (LS->linenumber > MAX_INT)
		luaX_errorlimit(LS, "lines in a chunk", MAX_INT);
}

// ---- 0x71aab0 luaX_setinput ---------------------------------------------------------------------------------

void luaX_setinput(lua_State* L, LexState* LS, ZIO* z, TString* source)
{
	LS->L = L;
	LS->lookahead.token = TK_EOS;  // no look-ahead token
	LS->z = z;
	LS->fs = NULL;
	LS->linenumber = luaX_linebase + 1;
	LS->lastline = LS->linenumber;
	LS->source = source;
	LS->nestlevel = 0;
	next(LS);  // read first char
	if (LS->current == '#') {  // skip the first line
		do {
			next(LS);
		} while (LS->current != '\n' && LS->current != EOZ);
	}
}

// ---- tokens -----------------------------------------------------------------------------------------------

// 0x71abb0: the characters of a name (the first is the current one) up to the NUL; the length
static size_t read_name(LexState* LS)
{
	size_t l = 0;
	do {
		checkbuffer(LS, l);
		save_and_next(LS, l);
	} while (isalnum(LS->current) || LS->current == '_');
	save(LS, '\0', l);
	return l - 1;
}

// 0x71acf0: the whole buffer must be a number in base 16 (strtoul, trailing spaces allowed)
static int str2hex(const char* s, lua_Int* result)
{
	char* endptr;
	unsigned long long v = strtoull(s, &endptr, 16);
	if (endptr == s)
		return 0;  // no conversion
	while (isspace((unsigned char)*endptr))
		endptr++;
	if (*endptr != '\0')
		return 0;  // invalid trailing characters?
	if (v > 0xffffffffULL)
		v = 0xffffffffULL;  // the CRT's strtoul saturates
	*result = (lua_Int)(uint32_t)v;
	return 1;
}

// 0x71ad60: `0x' (the `0' is saved, the current char is the `x') and the hex digits; an INT
void luaX_readhex(LexState* LS, int x, SemInfo* seminfo)
{
	size_t l = 0;
	checkbuffer(LS, l);
	save(LS, '0', l);
	checkbuffer(LS, l);
	save(LS, x, l);
	next(LS);
	while (isxdigit(LS->current)) {
		checkbuffer(LS, l);
		save_and_next(LS, l);
	}
	checkbuffer(LS, l);
	save(LS, '\0', l);
	if (!str2hex(G(LS->L)->buff, &seminfo->i))
		luaX_errorline(LS, "malformed hexadecimal number", G(LS->L)->buff, LS->linenumber);
}

// 0x71af50: a decimal numeral; `first' is the character already read ('0' or '.', or 0 when the current
// character is the first digit). The token is TK_INT until a `.' or an exponent makes it a TK_NUMBER
static int read_numeral(LexState* LS, int first, SemInfo* seminfo)
{
	size_t l = 0;
	int token = first != '.' ? TK_INT : TK_NUMBER;
	checkbuffer(LS, l);
	if (first != 0)
		save(LS, first, l);
	while (isdigit(LS->current)) {
		checkbuffer(LS, l);
		save_and_next(LS, l);
	}
	if (LS->current == '.') {
		save_and_next(LS, l);
		token = TK_NUMBER;
		if (LS->current == '.') {
			save_and_next(LS, l);
			save(LS, '\0', l);
			luaX_errorline(LS, "ambiguous syntax (decimal point x string concatenation)", G(LS->L)->buff,
				LS->linenumber);
		}
	}
	while (isdigit(LS->current)) {
		checkbuffer(LS, l);
		save_and_next(LS, l);
	}
	if (LS->current == 'e' || LS->current == 'E') {
		save_and_next(LS, l);  // read `E'
		token = TK_NUMBER;
		if (LS->current == '+' || LS->current == '-')
			save_and_next(LS, l);  // optional exponent sign
		while (isdigit(LS->current)) {
			checkbuffer(LS, l);
			save_and_next(LS, l);
		}
	}
	checkbuffer(LS, l);
	save(LS, '\0', l);
	if (token == TK_INT) {
		long long v = strtoll(G(LS->L)->buff, NULL, 10);  // strtol: saturates at the 32-bit limits
		if (v > INT32_MAX)
			v = INT32_MAX;
		else if (v < INT32_MIN)
			v = INT32_MIN;
		seminfo->i = (lua_Int)v;
	}
	else if (!luaO_str2d(G(LS->L)->buff, &seminfo->r))
		luaX_errorline(LS, "malformed number", G(LS->L)->buff, LS->linenumber);
	return token;
}

// 0x71b460: the comment after `/*' (the current char is the `*'); C comments do not nest
static void read_long_comment(LexState* LS)
{
	int cont = 0;
	size_t l = 0;
	checkbuffer(LS, l);
	save(LS, LS->current, l);  // save the `*'
	next(LS);
	if (LS->current == '\n')  // starts with a newline?
		inclinenumber(LS);  // skip it
	for (;;) {
		checkbuffer(LS, l);
		if (LS->current == EOZ) {
			save(LS, '\0', l);
			luaX_errorline(LS, "unfinished long comment", token2string[TK_EOS - FIRST_RESERVED], LS->linenumber);
		}
		else if (LS->current == '\n') {
			save(LS, '\n', l);
			inclinenumber(LS);
			l = 0;  // reset buffer to avoid wasting space
		}
		else if (LS->current == '*') {
			save_and_next(LS, l);
			if (LS->current == '/') {
				if (cont == 0)
					break;
				cont--;
				save_and_next(LS, l);
			}
		}
		else
			save_and_next(LS, l);
	}
	save(LS, LS->current, l);  // the `/'
	next(LS);
	save(LS, '\0', l);
}

// 0x71b6b0
static void read_string(LexState* LS, int del, SemInfo* seminfo)
{
	size_t l = 0;
	checkbuffer(LS, l);
	save_and_next(LS, l);
	while (LS->current != del) {
		checkbuffer(LS, l);
		switch (LS->current) {
		case EOZ:
			save(LS, '\0', l);
			luaX_errorline(LS, "unfinished string", token2string[TK_EOS - FIRST_RESERVED], LS->linenumber);
		case '\n':
			save(LS, '\0', l);
			luaX_errorline(LS, "unfinished string", G(LS->L)->buff, LS->linenumber);
		case '\\':
			next(LS);  // do not save the `\'
			switch (LS->current) {
			case 'a': save(LS, '\a', l); next(LS); break;
			case 'b': save(LS, '\b', l); next(LS); break;
			case 'f': save(LS, '\f', l); next(LS); break;
			case 'n': save(LS, '\n', l); next(LS); break;
			case 'r': save(LS, '\r', l); next(LS); break;
			case 't': save(LS, '\t', l); next(LS); break;
			case 'v': save(LS, '\v', l); next(LS); break;
			case '\n':  // backslash followed by a newline
				save(LS, '\n', l);
				inclinenumber(LS);
				break;
			case EOZ:
				break;  // will raise an error next loop
			default:
				if (!isdigit(LS->current))
					save_and_next(LS, l);  // handles \\, \", \', and \?
				else {  // \xxx
					int c = 0;
					int i = 0;
					do {
						c = 10 * c + (LS->current - '0');
						next(LS);
					} while (++i < 3 && isdigit(LS->current));
					if (c > 255) {
						save(LS, '\0', l);
						luaX_errorline(LS, "escape sequence too large", G(LS->L)->buff, LS->linenumber);
					}
					save(LS, c, l);
				}
			}
			break;
		default:
			save_and_next(LS, l);
		}
	}
	save_and_next(LS, l);  // skip delimiter
	save(LS, '\0', l);
	seminfo->ts = luaS_newlstr(LS->L, G(LS->L)->buff + 1, l - 3);
}

// 0x71bbf0
int luaX_lex(LexState* LS, SemInfo* seminfo)
{
	for (;;) {
		switch (LS->current) {
		case '\n':
			inclinenumber(LS);
			continue;
		case '!':
			next(LS);
			if (LS->current != '=')
				return '!';
			next(LS);
			return TK_NE2;
		case '"':
		case '\'':
			read_string(LS, LS->current, seminfo);
			return TK_STRING;
		case '&':
			next(LS);
			if (LS->current != '&')
				return '&';
			next(LS);
			return TK_AND;
		case '.':
			next(LS);
			if (LS->current != '.') {
				if (!isdigit(LS->current))
					return '.';
				return read_numeral(LS, '.', seminfo);
			}
			next(LS);
			if (LS->current != '.')
				return TK_CONCAT;
			next(LS);
			return TK_DOTS;
		case '/':
			next(LS);
			if (LS->current == '/') {  // line comment
				while (LS->current != '\n' && LS->current != EOZ)
					next(LS);
				continue;
			}
			if (LS->current != '*')
				return '/';
			read_long_comment(LS);
			continue;
		case '0':
			next(LS);
			if (LS->current != 'x' && LS->current != 'X')
				return read_numeral(LS, '0', seminfo);
			luaX_readhex(LS, LS->current, seminfo);
			return TK_INT;
		case '<':
			next(LS);
			if (LS->current != '=')
				return '<';
			next(LS);
			return TK_LE;
		case '=':
			next(LS);
			if (LS->current != '=')
				return '=';
			next(LS);
			return TK_EQ;
		case '>':
			next(LS);
			if (LS->current != '=')
				return '>';
			next(LS);
			return TK_GE;
		case '|':
			next(LS);
			if (LS->current != '|')
				return '|';
			next(LS);
			return TK_OR;
		case '~':
			next(LS);
			if (LS->current != '=')
				return '~';
			next(LS);
			return TK_NE;
		case EOZ:
			return TK_EOS;
		default:
			if (isspace(LS->current)) {
				next(LS);
				continue;
			}
			else if (isdigit(LS->current))
				return read_numeral(LS, 0, seminfo);
			else if (isalpha(LS->current) || LS->current == '_') {  // identifier or reserved word
				size_t l = read_name(LS);
				TString* ts = luaS_newlstr(LS->L, G(LS->L)->buff, l);
				if (ts->reserved > 0)  // reserved word?
					return ts->reserved - 1 + FIRST_RESERVED;
				seminfo->ts = ts;
				return TK_NAME;
			}
			else {
				int c = LS->current;
				if (iscntrl(c))
					luaX_errorline(LS, "invalid control char", luaO_pushfstring(LS->L, "char(%d)", c), LS->linenumber);
				next(LS);
				return c;  // single-char tokens (+ - * % ^ ( ) ...)
			}
		}
	}
}

// 0x70e490 (luaX_next with no result) and 0x70e540 (the same, returning 1: the exe's testnext/check call it)
void luaX_next(LexState* LS)
{
	LS->lastline = LS->linenumber;
	if (LS->lookahead.token != TK_EOS) {  // is there a look-ahead token?
		LS->t = LS->lookahead;  // use this one
		LS->lookahead.token = TK_EOS;  // and discharge it
	}
	else
		LS->t.token = luaX_lex(LS, &LS->t.seminfo);  // read next token
}

void luaX_lookahead(LexState* LS)
{
	LS->lookahead.token = luaX_lex(LS, &LS->lookahead.seminfo);
}
