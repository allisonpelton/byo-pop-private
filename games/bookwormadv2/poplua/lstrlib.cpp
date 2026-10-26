// The string library (0x705240-0x708380): Lua 5.0.2's lstrlib.c with PopLua's changes. Positions are 0-based
// (sub, byte, find, a negative position counts from the end), string.sub takes a count instead of an end,
// string.find returns the start and the length of the match, lengths and counts are ints, and tonum,
// addslashes, split and the trims are PopLua's. string.dump and string.join are not ported.
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "lauxlib.h"

#define uchar(c) ((unsigned char)(c))

// 0x705240
static int str_len(lua_State* L)
{
	size_t l;
	luaL_checklstring(L, 1, &l);
	lua_pushint(L, (lua_Int)l);
	return 1;
}

// 0x7052f0: sub(s, start [, count]); start < 0 counts from the end
static int str_sub(lua_State* L)
{
	size_t l;
	const char* s = luaL_checklstring(L, 1, &l);
	int len = (int)l;
	int start = lua_ftol(luaL_checknumber(L, 2));
	if (start < 0)
		start += len;
	int count = lua_ftol(luaL_optnumber(L, 3, (lua_Number)(unsigned int)(len - start)));
	if (start < 0)
		start = 0;
	if (count + start >= len)
		count = len - start;
	if (count < 1)
		lua_pushliteral(L, "");
	else
		lua_pushlstring(L, s + start, (size_t)count);
	return 1;
}

// 0x7054f0
static int str_lower(lua_State* L)
{
	size_t l;
	size_t i;
	luaL_Buffer b;
	const char* s = luaL_checklstring(L, 1, &l);
	luaL_buffinit(L, &b);
	for (i = 0; i < l; i++)
		luaL_putchar(&b, tolower(uchar(s[i])));
	luaL_pushresult(&b);
	return 1;
}

// 0x705680
static int str_upper(lua_State* L)
{
	size_t l;
	size_t i;
	luaL_Buffer b;
	const char* s = luaL_checklstring(L, 1, &l);
	luaL_buffinit(L, &b);
	for (i = 0; i < l; i++)
		luaL_putchar(&b, toupper(uchar(s[i])));
	luaL_pushresult(&b);
	return 1;
}

// 0x705810
static int str_rep(lua_State* L)
{
	size_t l;
	luaL_Buffer b;
	const char* s = luaL_checklstring(L, 1, &l);
	int n = luaL_checkint(L, 2);
	luaL_buffinit(L, &b);
	while (n-- > 0)
		luaL_addlstring(&b, s, l);
	luaL_pushresult(&b);
	return 1;
}

// 0x705990: byte(s [, i]); the byte is signed (the original's char), nothing is returned out of range
static int str_byte(lua_State* L)
{
	size_t l;
	const char* s = luaL_checklstring(L, 1, &l);
	int pos = lua_ftol(luaL_optnumber(L, 2, 0));
	if (pos < 0)
		pos += (int)l;
	if (pos < 0 || (size_t)pos >= l)
		return 0;
	lua_pushint(L, (signed char)s[pos]);
	return 1;
}

// 0x705a90
static int str_char(lua_State* L)
{
	int n = lua_gettop(L);  // number of arguments
	int i;
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for (i = 1; i <= n; i++) {
		int c = luaL_checkint(L, i);
		luaL_putchar(&b, uchar(c));
	}
	luaL_pushresult(&b);
	return 1;
}

// 0x705bb0: an int when the string has no '.', else a number; nothing when it is not one
static int str_tonum(lua_State* L)
{
	const char* s = luaL_checkstring(L, 1);
	if (strchr(s, '.') == NULL) {
		lua_Int i;
		if (lua_str2int(s, &i)) {
			lua_pushint(L, i);
			return 1;
		}
	}
	else {
		lua_Number d;
		if (lua_str2number(s, &d)) {
			lua_pushnumber(L, d);
			return 1;
		}
	}
	return 0;
}

// 0x705c90: split(s, sep) returns the pieces
static int str_split(lua_State* L)
{
	const char* s = luaL_checkstring(L, 1);
	const char* sep = luaL_checkstring(L, 2);
	size_t lsep = strlen(sep);
	int n = 0;
	for (;;) {
		const char* e = strstr(s, sep);
		if (e == NULL) {
			lua_pushstring(L, s);
			return n + 1;
		}
		lua_pushlstring(L, s, (size_t)(e - s));
		n++;
		s = e + lsep;
	}
}

// 0x708120, 0x708200, 0x7082c0
static int aux_trim(lua_State* L, int left, int right)
{
	const char* s = luaL_checkstring(L, 1);
	const char* e = s + strlen(s);
	if (left)
		while (isspace(uchar(*s)))
			s++;
	if (right)
		while (e > s && isspace(uchar(e[-1])))
			e--;
	lua_pushlstring(L, s, (size_t)(e - s));
	return 1;
}

static int str_trim(lua_State* L)
{
	return aux_trim(L, 1, 1);
}

static int str_ltrim(lua_State* L)
{
	return aux_trim(L, 1, 0);
}

static int str_rtrim(lua_State* L)
{
	return aux_trim(L, 0, 1);
}

// 0x707c70: escapes for a quoted string. The original prints other bytes with "%03d" (signed) into a
// 4-byte slot, so the slot holds the digits and a '\0' and no backslash; this is kept.
static int str_addslashes(lua_State* L)
{
	size_t l;
	const char* s = luaL_checklstring(L, 1, &l);
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for (size_t i = 0; i < l; i++) {
		char c = s[i];
		const char* esc = NULL;
		switch (c) {
		case '\a': esc = "\\a"; break;
		case '\b': esc = "\\b"; break;
		case '\t': esc = "\\t"; break;
		case '\n': esc = "\\n"; break;
		case '\v': esc = "\\v"; break;
		case '\f': esc = "\\f"; break;
		case '\r': esc = "\\r"; break;
		case '"': esc = "\\\""; break;
		case '\'': esc = "\\'"; break;
		case '[': esc = "\\["; break;
		case ']': esc = "\\]"; break;
		case '\\': esc = "\\\\"; break;
		default: break;
		}
		if (esc)
			luaL_addlstring(&b, esc, 2);
		else if (uchar(c) < 10) {
			char e[4] = {'\\', '0', '0', (char)('0' + c)};
			luaL_addlstring(&b, e, 4);
		}
		else if (uchar(c - 0x20) <= 0x5f)
			luaL_putchar(&b, c);
		else {
			char e[8] = {0};
			snprintf(e, sizeof(e), "%03d", (int)(signed char)c);
			luaL_addlstring(&b, e, 4);
		}
	}
	luaL_pushresult(&b);
	return 1;
}

// ---- pattern matching (5.0.2) ------------------------------------------------------------------------

#define CAP_UNFINISHED (-1)
#define CAP_POSITION (-2)
#define MAX_CAPTURES 32

struct MatchState {
	const char* src_init;  // init of source string
	const char* src_end;   // end (`\0') of source string
	lua_State* L;
	int level;  // total number of captures (finished or unfinished)
	struct {
		const char* init;
		int len;
	} capture[MAX_CAPTURES];
};

#define ESC '%'
#define SPECIALS "^$*+?.([%-"

static int check_capture(MatchState* ms, int l)
{
	l -= '1';
	if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
		return luaL_error(ms->L, "invalid capture index");
	return l;
}

static int capture_to_close(MatchState* ms)
{
	int level = ms->level;
	for (level--; level >= 0; level--)
		if (ms->capture[level].len == CAP_UNFINISHED)
			return level;
	return luaL_error(ms->L, "invalid pattern capture");
}

static const char* luaI_classend(MatchState* ms, const char* p)
{
	switch (*p++) {
	case ESC: {
		if (*p == '\0')
			luaL_error(ms->L, "malformed pattern (ends with `%%')");
		return p + 1;
	}
	case '[': {
		if (*p == '^')
			p++;
		do {  // look for a `]'
			if (*p == '\0')
				luaL_error(ms->L, "malformed pattern (missing `]')");
			if (*(p++) == ESC && *p != '\0')
				p++;  // skip escapes (e.g. `%]')
		} while (*p != ']');
		return p + 1;
	}
	default: {
		return p;
	}
	}
}

static int match_class(int c, int cl)
{
	int res;
	switch (tolower(cl)) {
	case 'a': res = isalpha(c); break;
	case 'c': res = iscntrl(c); break;
	case 'd': res = isdigit(c); break;
	case 'l': res = islower(c); break;
	case 'p': res = ispunct(c); break;
	case 's': res = isspace(c); break;
	case 'u': res = isupper(c); break;
	case 'w': res = isalnum(c); break;
	case 'x': res = isxdigit(c); break;
	case 'z': res = (c == 0); break;
	default: return (cl == c);
	}
	if (islower(cl))
		return res;
	else
		return !res;
}

static int matchbracketclass(int c, const char* p, const char* ec)
{
	int sig = 1;
	if (*(p + 1) == '^') {
		sig = 0;
		p++;  // skip the `^'
	}
	while (++p < ec) {
		if (*p == ESC) {
			p++;
			if (match_class(c, uchar(*p)))
				return sig;
		}
		else if ((*(p + 1) == '-') && (p + 2 < ec)) {
			p += 2;
			if (uchar(*(p - 2)) <= c && c <= uchar(*p))
				return sig;
		}
		else if (uchar(*p) == c)
			return sig;
	}
	return !sig;
}

static int luaI_singlematch(int c, const char* p, const char* ep)
{
	switch (*p) {
	case '.': return 1;  // matches any char
	case ESC: return match_class(c, uchar(*(p + 1)));
	case '[': return matchbracketclass(c, p, ep - 1);
	default: return (uchar(*p) == c);
	}
}

static const char* match(MatchState* ms, const char* s, const char* p);

static const char* matchbalance(MatchState* ms, const char* s, const char* p)
{
	if (*p == 0 || *(p + 1) == 0)
		luaL_error(ms->L, "unbalanced pattern");
	if (*s != *p)
		return NULL;
	else {
		int b = *p;
		int e = *(p + 1);
		int cont = 1;
		while (++s < ms->src_end) {
			if (*s == e) {
				if (--cont == 0)
					return s + 1;
			}
			else if (*s == b)
				cont++;
		}
	}
	return NULL;  // string ends out of balance
}

static const char* max_expand(MatchState* ms, const char* s, const char* p, const char* ep)
{
	int i = 0;  // counts maximum expand for item
	while ((s + i) < ms->src_end && luaI_singlematch(uchar(*(s + i)), p, ep))
		i++;
	// keeps trying to match with the maximum repetitions
	while (i >= 0) {
		const char* res = match(ms, (s + i), ep + 1);
		if (res)
			return res;
		i--;  // else didn't match; reduce 1 repetition to try again
	}
	return NULL;
}

static const char* min_expand(MatchState* ms, const char* s, const char* p, const char* ep)
{
	for (;;) {
		const char* res = match(ms, s, ep + 1);
		if (res != NULL)
			return res;
		else if (s < ms->src_end && luaI_singlematch(uchar(*s), p, ep))
			s++;  // try with one more repetition
		else
			return NULL;
	}
}

static const char* start_capture(MatchState* ms, const char* s, const char* p, int what)
{
	const char* res;
	int level = ms->level;
	if (level >= MAX_CAPTURES)
		luaL_error(ms->L, "too many captures");
	ms->capture[level].init = s;
	ms->capture[level].len = what;
	ms->level = level + 1;
	if ((res = match(ms, s, p)) == NULL)  // match failed?
		ms->level--;  // undo capture
	return res;
}

static const char* end_capture(MatchState* ms, const char* s, const char* p)
{
	int l = capture_to_close(ms);
	const char* res;
	ms->capture[l].len = (int)(s - ms->capture[l].init);  // close capture
	if ((res = match(ms, s, p)) == NULL)  // match failed?
		ms->capture[l].len = CAP_UNFINISHED;  // undo capture
	return res;
}

static const char* match_capture(MatchState* ms, const char* s, int l)
{
	size_t len;
	l = check_capture(ms, l);
	len = (size_t)ms->capture[l].len;
	if ((size_t)(ms->src_end - s) >= len && memcmp(ms->capture[l].init, s, len) == 0)
		return s + len;
	else
		return NULL;
}

static const char* match(MatchState* ms, const char* s, const char* p)
{
init:  // using goto's to optimize tail recursion
	switch (*p) {
	case '(': {  // start capture
		if (*(p + 1) == ')')  // position capture?
			return start_capture(ms, s, p + 2, CAP_POSITION);
		else
			return start_capture(ms, s, p + 1, CAP_UNFINISHED);
	}
	case ')': {  // end capture
		return end_capture(ms, s, p + 1);
	}
	case ESC: {
		switch (*(p + 1)) {
		case 'b': {  // balanced string?
			s = matchbalance(ms, s, p + 2);
			if (s == NULL)
				return NULL;
			p += 4;
			goto init;  // else return match(ms, s, p+4);
		}
		default: {
			if (isdigit(uchar(*(p + 1)))) {  // capture results (%0-%9)?
				s = match_capture(ms, s, *(p + 1));
				if (s == NULL)
					return NULL;
				p += 2;
				goto init;  // else return match(ms, s, p+2)
			}
			goto dflt;  // case default
		}
		}
	}
	case '\0': {  // end of pattern
		return s;  // match succeeded
	}
	case '$': {
		if (*(p + 1) == '\0')  // is the `$' the last char in pattern?
			return (s == ms->src_end) ? s : NULL;  // check end of string
		else
			goto dflt;
	}
	default:
	dflt: {  // it is a pattern item
		const char* ep = luaI_classend(ms, p);  // points to what is next
		int m = s < ms->src_end && luaI_singlematch(uchar(*s), p, ep);
		switch (*ep) {
		case '?': {  // optional
			const char* res;
			if (m && ((res = match(ms, s + 1, ep + 1)) != NULL))
				return res;
			p = ep + 1;
			goto init;  // else return match(ms, s, ep+1);
		}
		case '*': {  // 0 or more repetitions
			return max_expand(ms, s, p, ep);
		}
		case '+': {  // 1 or more repetitions
			return (m ? max_expand(ms, s + 1, p, ep) : NULL);
		}
		case '-': {  // 0 or more repetitions (minimum)
			return min_expand(ms, s, p, ep);
		}
		default: {
			if (!m)
				return NULL;
			s++;
			p = ep;
			goto init;  // else return match(ms, s+1, ep);
		}
		}
	}
	}
}

static const char* lmemfind(const char* s1, size_t l1, const char* s2, size_t l2)
{
	if (l2 == 0)
		return s1;  // empty strings are everywhere
	else if (l2 > l1)
		return NULL;  // avoids a negative `l1'
	else {
		const char* init;  // to search for a `*s2' inside `s1'
		l2--;  // 1st char will be checked by `memchr'
		l1 = l1 - l2;  // `s2' cannot be found after that
		while (l1 > 0 && (init = (const char*)memchr(s1, *s2, l1)) != NULL) {
			init++;  // 1st char is already checked
			if (memcmp(init, s2 + 1, l2) == 0)
				return init - 1;
			else {  // correct `l1' and `s1' to try again
				l1 -= (size_t)(init - s1);
				s1 = init;
			}
		}
		return NULL;  // not found
	}
}

// a position capture is pushed as an int counted from 1 (5.0.2's arithmetic)
static void push_onecapture(MatchState* ms, int i)
{
	int l = ms->capture[i].len;
	if (l == CAP_UNFINISHED)
		luaL_error(ms->L, "unfinished capture");
	if (l == CAP_POSITION)
		lua_pushint(ms->L, (lua_Int)(ms->capture[i].init - ms->src_init + 1));
	else
		lua_pushlstring(ms->L, ms->capture[i].init, (size_t)l);
}

// 0x706a60
static int push_captures(MatchState* ms, const char* s, const char* e)
{
	int i;
	luaL_checkstack(ms->L, ms->level, "too many captures");
	if (ms->level == 0 && s) {  // no explicit captures?
		lua_pushlstring(ms->L, s, (size_t)(e - s));  // return whole match
		return 1;
	}
	else {  // return all captures
		for (i = 0; i < ms->level; i++)
			push_onecapture(ms, i);
		return ms->level;  // number of strings pushed
	}
}

// 0x706b60: find(s, pattern [, init [, plain]]) returns the 0-based start and the length of the match
static int str_find(lua_State* L)
{
	size_t l1, l2;
	const char* s = luaL_checklstring(L, 1, &l1);
	const char* p = luaL_checklstring(L, 2, &l2);
	int init = lua_ftol(luaL_optnumber(L, 3, 0));
	if (init < 0)
		init += (int)l1;
	if (init < 0)
		init = 0;
	else if ((size_t)init > l1)
		init = (int)l1;
	if (lua_toboolean(L, 4) ||  // explicit request?
	    strpbrk(p, SPECIALS) == NULL) {  // or no special characters?
		// do a plain search
		const char* s2 = lmemfind(s + init, l1 - (size_t)init, p, l2);
		if (s2) {
			lua_pushint(L, (lua_Int)(s2 - s));
			lua_pushint(L, (lua_Int)l2);
			return 2;
		}
	}
	else {
		MatchState ms;
		int anchor = (*p == '^') ? (p++, 1) : 0;
		const char* s1 = s + init;
		ms.L = L;
		ms.src_init = s;
		ms.src_end = s + l1;
		do {
			const char* res;
			ms.level = 0;
			if ((res = match(&ms, s1, p)) != NULL) {
				lua_pushint(L, (lua_Int)(s1 - s));  // start
				lua_pushint(L, (lua_Int)(res - s1));  // length
				return push_captures(&ms, NULL, 0) + 2;
			}
		} while (s1++ < ms.src_end && !anchor);
	}
	lua_pushnil(L);  // not found
	return 1;
}

static int gfind_aux(lua_State* L)
{
	MatchState ms;
	const char* s = lua_tostring(L, lua_upvalueindex(1));
	size_t ls = lua_strlen(L, lua_upvalueindex(1));
	const char* p = lua_tostring(L, lua_upvalueindex(2));
	const char* src;
	ms.L = L;
	ms.src_init = s;
	ms.src_end = s + ls;
	for (src = s + (size_t)lua_toint(L, lua_upvalueindex(3)); src <= ms.src_end; src++) {
		const char* e;
		ms.level = 0;
		if ((e = match(&ms, src, p)) != NULL) {
			int newstart = (int)(e - s);
			if (e == src)
				newstart++;  // empty match? go at least one position
			lua_pushint(L, newstart);
			lua_replace(L, lua_upvalueindex(3));
			return push_captures(&ms, src, e);
		}
	}
	return 0;  // not found
}

// 0x707080
static int gfind(lua_State* L)
{
	luaL_checkstring(L, 1);
	luaL_checkstring(L, 2);
	lua_settop(L, 2);
	lua_pushint(L, 0);
	lua_pushcclosure(L, gfind_aux, 3);
	return 1;
}

static void add_s(MatchState* ms, luaL_Buffer* b, const char* s, const char* e)
{
	lua_State* L = ms->L;
	if (lua_isstring(L, 3)) {
		const char* news = lua_tostring(L, 3);
		size_t l = lua_strlen(L, 3);
		size_t i;
		for (i = 0; i < l; i++) {
			if (news[i] != ESC)
				luaL_putchar(b, news[i]);
			else {
				i++;  // skip ESC
				if (!isdigit(uchar(news[i])))
					luaL_putchar(b, news[i]);
				else {
					int level = check_capture(ms, news[i]);
					push_onecapture(ms, level);
					luaL_addvalue(b);  // add capture to accumulated result
				}
			}
		}
	}
	else {  // is a function
		int n;
		lua_pushvalue(L, 3);
		n = push_captures(ms, s, e);
		lua_call(L, n, 1);
		if (lua_isstring(L, -1))
			luaL_addvalue(b);  // add return to accumulated result
		else
			lua_pop(L, 1);  // function result is not a string: pop it
	}
}

// 0x707390
static int str_gsub(lua_State* L)
{
	size_t srcl;
	const char* src = luaL_checklstring(L, 1, &srcl);
	const char* p = luaL_checkstring(L, 2);
	int max_s = luaL_optint(L, 4, (lua_Int)srcl + 1);
	int anchor = (*p == '^') ? (p++, 1) : 0;
	int n = 0;
	MatchState ms;
	luaL_Buffer b;
	luaL_argcheck(L, lua_gettop(L) >= 3 && (lua_isstring(L, 3) || lua_isfunction(L, 3)), 3,
	              "string or function expected");
	luaL_buffinit(L, &b);
	ms.L = L;
	ms.src_init = src;
	ms.src_end = src + srcl;
	while (n < max_s) {
		const char* e;
		ms.level = 0;
		e = match(&ms, src, p);
		if (e) {
			n++;
			add_s(&ms, &b, src, e);
		}
		if (e && e > src)  // non empty match?
			src = e;  // skip it
		else if (src < ms.src_end)
			luaL_putchar(&b, *src++);
		else
			break;
		if (anchor)
			break;
	}
	luaL_addlstring(&b, src, (size_t)(ms.src_end - src));
	luaL_pushresult(&b);
	lua_pushint(L, n);  // number of substitutions
	return 2;
}

// ---- format ------------------------------------------------------------------------------------------

#define MAX_ITEM 512
#define MAX_FORMAT 20

static void luaI_addquoted(lua_State* L, luaL_Buffer* b, int arg)
{
	size_t l;
	const char* s = luaL_checklstring(L, arg, &l);
	luaL_putchar(b, '"');
	while (l--) {
		switch (*s) {
		case '"':
		case '\\':
		case '\n': {
			luaL_putchar(b, '\\');
			luaL_putchar(b, *s);
			break;
		}
		case '\0': {
			luaL_addlstring(b, "\\000", 4);
			break;
		}
		default: {
			luaL_putchar(b, *s);
			break;
		}
		}
		s++;
	}
	luaL_putchar(b, '"');
}

static const char* scanformat(lua_State* L, const char* strfrmt, char* form, int* hasprecision)
{
	const char* p = strfrmt;
	while (*p != '\0' && strchr("-+ #0", *p))
		p++;  // skip flags
	if (isdigit(uchar(*p)))
		p++;  // skip width
	if (isdigit(uchar(*p)))
		p++;  // (2 digits at most)
	if (*p == '.') {
		p++;
		*hasprecision = 1;
		if (isdigit(uchar(*p)))
			p++;  // skip precision
		if (isdigit(uchar(*p)))
			p++;  // (2 digits at most)
	}
	if (isdigit(uchar(*p)))
		luaL_error(L, "invalid format (width or precision too long)");
	if (p - strfrmt + 2 > MAX_FORMAT)  // +2 to include `%' and the specifier
		luaL_error(L, "invalid format (too long)");
	form[0] = '%';
	strncpy(form + 1, strfrmt, (size_t)(p - strfrmt + 1));
	form[p - strfrmt + 2] = 0;
	return p;
}

// the original's CRT writes exponents with three digits
static void crt_exponent(char* buff)
{
	char* e = strpbrk(buff, "eE");
	if (e != NULL && (e[1] == '+' || e[1] == '-') && isdigit(uchar(e[2])) && isdigit(uchar(e[3])) &&
	    !isdigit(uchar(e[4]))) {
		memmove(e + 3, e + 2, strlen(e + 2) + 1);
		e[2] = '0';
	}
}

// 0x707990
static int str_format(lua_State* L)
{
	int arg = 1;
	size_t sfl;
	const char* strfrmt = luaL_checklstring(L, arg, &sfl);
	const char* strfrmt_end = strfrmt + sfl;
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	while (strfrmt < strfrmt_end) {
		if (*strfrmt != '%')
			luaL_putchar(&b, *strfrmt++);
		else if (*++strfrmt == '%')
			luaL_putchar(&b, *strfrmt++);  // %%
		else {  // format item
			char form[MAX_FORMAT];  // to store the format (`%...')
			char buff[MAX_ITEM];  // to store the formatted item
			int hasprecision = 0;
			if (isdigit(uchar(*strfrmt)) && *(strfrmt + 1) == '$')
				return luaL_error(L, "obsolete option (d$) to `format'");
			arg++;
			strfrmt = scanformat(L, strfrmt, form, &hasprecision);
			switch (*strfrmt++) {
			case 'c':
			case 'd':
			case 'i': {
				snprintf(buff, sizeof(buff), form, (int)luaL_checkint(L, arg));
				break;
			}
			case 'o':
			case 'u':
			case 'x':
			case 'X': {
				snprintf(buff, sizeof(buff), form, (unsigned int)luaL_checkint(L, arg));
				break;
			}
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G': {
				snprintf(buff, sizeof(buff), form, (double)luaL_checknumber(L, arg));
				crt_exponent(buff);
				break;
			}
			case 'q': {
				luaI_addquoted(L, &b, arg);
				continue;  // skip the `addsize' at the end
			}
			case 's': {
				size_t l;
				const char* s = luaL_checklstring(L, arg, &l);
				if (!hasprecision && l >= 100) {
					// no precision and string is too long to be formatted; keep original string
					lua_pushvalue(L, arg);
					luaL_addvalue(&b);
					continue;  // skip the `addsize' at the end
				}
				else {
					snprintf(buff, sizeof(buff), form, s);
					break;
				}
			}
			default: {  // also treat cases `pnLlh'
				return luaL_error(L, "invalid option to `format'");
			}
			}
			luaL_addlstring(&b, buff, strlen(buff));
		}
	}
	luaL_pushresult(&b);
	return 1;
}

static const luaL_reg strlib[] = {
	{"len", str_len},
	{"sub", str_sub},
	{"lower", str_lower},
	{"upper", str_upper},
	{"char", str_char},
	{"rep", str_rep},
	{"byte", str_byte},
	{"format", str_format},
	{"find", str_find},
	{"gfind", gfind},
	{"gsub", str_gsub},
	{"tonum", str_tonum},
	{"addslashes", str_addslashes},
	{"split", str_split},
	{"trim", str_trim},
	{"ltrim", str_ltrim},
	{"rtrim", str_rtrim},
	{NULL, NULL}
};

// 0x708380
int luaopen_string(lua_State* L)
{
	luaL_openlib(L, LUA_STRLIBNAME, strlib, 0);
	return 1;
}
