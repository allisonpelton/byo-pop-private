// Self-checking tests of PopLua's compiler: the dialect's statements and expressions are compiled by loadstring or
// by this file's own compilation and the results checked by running them. Run it with
//     build/<dir>/bwa_luarun -gamedir=games/bookwormadv/tests/poplua dialect.lua
// (it prints "dialect: N checks passed", or the failing check and exits with an error).
// All the names and texts here are the tests' own.

def nchecks = 0

function check(cond, what) {
	nchecks++
	if (!cond) { error("check failed: " .. what, 2) }
}

function checkeq(a, b, what) {
	nchecks++
	if (a != b) { error("check failed: " .. what .. " (" .. tostring(a) .. " ~= " .. tostring(b) .. ")", 2) }
}

// a chunk that must fail to compile, with a part of the message
function checkerr(src, part) {
	local f, msg = loadstring(src)
	nchecks++
	if (f != nil) { error("check failed: compiled: " .. src, 2) }
	if (!string.find(msg, part, 0, true)) { error("check failed: error for " .. src .. " was " .. msg, 2) }
}

// ---- lexer: numbers, strings, comments, operators ----------------------------------------------------

checkeq(0x10, 16, "hex")
checkeq(loadstring("return 0XfF")(), 254 + 1, "hex upper")
checkeq(1e2, 100, "exponent")
checkeq(type(1e2), "number", "float type")
checkeq(type(10), "int", "int type")
checkeq(7 / 2, 3, "int division")
checkeq(7.0 / 2, 3.5, "float division")
checkeq(7 % 3, 1, "mod")
checkeq(.5 + .25, 0.75, "leading dot")
checkeq("a\tb", "a" .. string.char(9) .. "b", "tab escape")
checkeq("\65\066", "AB", "decimal escape")
checkeq('it\'s', "it's", "quote escape")
checkeq("a\
b", "a\nb", "escaped newline")
// line comment
/* block
   comment */ checkeq(1 /* inline */ + 1, 2, "block comment")
checkeq(1 != 2, true, "!=")
checkeq(1 ~= 1, false, "~=")
checkeq(!nil, true, "!")
checkeq(true && false, false, "&&")
checkeq(false || 5, 5, "||")
checkeq(not 1, false, "not")
checkeq(1 and 2, 2, "and")
checkeq(nil or 3, 3, "or")
checkeq(2 ^ 3, 8, "pow")
checkeq(-2 ^ 2, -4, "unary minus binds looser than ^")
checkeq("a" .. 1 .. 2, "a12", "concat")
checkeq(int 3.7, 3, "int cast")
checkeq(int -3.7, -3, "int cast negative")
checkeq(type(int 3.7), "int", "int cast type")
checkeq(-5, 0 - 5, "negative constant")
checkeq(-0.5, 0 - 0.5, "negative float constant")

// ---- tables ---------------------------------------------------------------------------------------------

local t = {10, 20, 30, x = 1, ["y"] = 2, [5] = 50; 40}
checkeq(t[0], 10, "array starts at 0")
checkeq(t[3], 40, "array item after ;")
checkeq(t.x + t.y, 3, "record fields")
checkeq(t[5], 50, "indexed field")
local sz = {10, 20, 30; 40}
checkeq(*sz, 4, "size")
local big = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, 32, 33, 34, 35, 36, 37, 38, 39, 40}
checkeq(*big, 40, "constructor flush")
checkeq(big[39], 40, "last item")
local function three() { return 1, 2, 3 }
local multi = {three()}
checkeq(*multi, 3, "open call in constructor")
local multi2 = {three(), 9}
checkeq(*multi2, 2, "call in the middle gives one value")
checkeq(multi2[1], 9, "value after")

// ---- assignment ---------------------------------------------------------------------------------------------

local a, b = 1, 2
a, b = b, a
checkeq(a, 2, "swap a")
checkeq(b, 1, "swap b")
local c, d, e = three()
checkeq(e, 3, "multiple results")
local i = 5
local tbl = {}
i, tbl[i] = i + 1, 20
checkeq(tbl[5], 20, "multiple assignment evaluates the key first")
checkeq(i, 6, "multiple assignment")
local x = 10
x += 5
checkeq(x, 15, "+=")
x -= 3
checkeq(x, 12, "-=")
x *= 2
checkeq(x, 24, "*=")
x /= 5
checkeq(x, 4, "/=")
x %= 3
checkeq(x, 1, "%=")
x++
checkeq(x, 2, "++")
x--
checkeq(x, 1, "--")
tbl.n = 1
tbl.n += 2
checkeq(tbl.n, 3, "+= on a field")
tbl[7] = 1
tbl[7]++
checkeq(tbl[7], 2, "++ on an indexed field")
G1 = 1
G1 += 4
checkeq(G1, 5, "+= on a global")
local function counter() {
	local n = 0
	return function() { n++ return n }
}
local cnt = counter()
cnt()
checkeq(cnt(), 2, "++ on an upvalue")

// ---- control flow -----------------------------------------------------------------------------------------

local sum = 0
for i = 1, 10 { sum += i }
checkeq(sum, 55, "for")
sum = 0
for i = 10, 1, -3 { sum += i }
checkeq(sum, 10 + 7 + 4 + 1, "for with a negative step")
sum = 0
for i < 5 { sum += i }
checkeq(sum, 0 + 1 + 2 + 3 + 4, "for i < n")
sum = 0
for (i = 1, 4) { sum += i }
checkeq(sum, 10, "for in parentheses")
sum = 0
for k, v in {5, 6, 7} do sum += k * v end
checkeq(sum, 0 * 5 + 1 * 6 + 2 * 7, "for in")
sum = 0
for (k, v in {5, 6, 7}) sum += v
checkeq(sum, 18, "for in, one statement")
sum = 0
for i = 1, 10 {
	if (i % 2 == 0) { continue }
	if (i > 7) { break }
	sum += i
}
checkeq(sum, 1 + 3 + 5 + 7, "continue and break in for")
local n = 0
while (n < 10) { n++ }
checkeq(n, 10, "while")
n = 0
while n < 10 do n = n + 2 end
checkeq(n, 10, "while ... do ... end")
n = 0
while (true) { n++ if (n == 5) { break } }
checkeq(n, 5, "break in while")
n = 0
repeat { n++ } until (n >= 3)
checkeq(n, 3, "repeat")
n = 0
repeat n = n + 1 until n == 4
checkeq(n, 4, "repeat without braces")
n = 0
while (n < 3) n++
checkeq(n, 3, "while with one statement")
local r
if (1 > 2) { r = "a" } else if (2 > 3) { r = "b" } else { r = "c" }
checkeq(r, "c", "else if")
if (1 < 2) then r = "d" elseif (2 > 3) then r = "e" else r = "f" end
checkeq(r, "d", "if then elseif else end")
if (nil) { r = "g" } else r = "h"
checkeq(r, "h", "else with one statement")
if (1 < 2 && 2 < 3 || false) { r = "i" }
checkeq(r, "i", "&& and ||")
if (!(1 > 2) && (2 < 3)) { r = "j" }
checkeq(r, "j", "parenthesised conditions")
if ((1 < 2) && (3 < 2)) { r = "k" } else { r = "l" }
checkeq(r, "l", "parenthesised conditions with operators after")
do local inner = 5 r = inner end
checkeq(r, 5, "{ } block")
{
	local inner2 = 6
	r = inner2
}
checkeq(r, 6, "bare block")
local fns = {}
for i = 1, 3 { local j = i fns[i] = function() { return j * 10 } }
checkeq(fns[1](), 10, "closure over a body local")
checkeq(fns[3](), 30, "closure over a body local, last")
for i = 1, 3 { fns[i] = function() { return i } }
checkeq(fns[1](), fns[3](), "the loop variable is one upvalue")
local k = 0
while (k < 3) { local j = k fns[k] = function() { return j } k++ }
checkeq(fns[2](), 2, "closure in a while body")

// ---- functions ----------------------------------------------------------------------------------------------

local function va(a, ...) { return a, arg[0], *arg }
local v1, v2, v3 = va(1, 2, 3)
checkeq(v1 + v2 + v3, 1 + 2 + 2, "varargs")
function fact(n) { if (n <= 1) { return 1 } return n * fact(n - 1) }
checkeq(fact(5), 120, "recursion")
local function tail(n) { if (n == 0) { return "done" } return tail(n - 1) }
checkeq(tail(1000), "done", "tail calls")
local obj = {v = 4}
function obj:get(d) { return self.v + d }
function obj.sq(x) { return x * x }
checkeq(obj:get(1), 5, "method")
checkeq(obj.sq(3), 9, "function in a table")
checkeq((function(a) { return a + 1 })(1), 2, "anonymous function")
local s = "text"
checkeq(string.len(s), 4, "string function")
checkeq(string.upper("abc"), "ABC", "string function on a literal")
checkeq(tostring"x", "x", "call with a string argument")
checkeq(type"x", "string", "call with a string literal")

// ---- classes ---------------------------------------------------------------------------------------------

class Animal (name) {
	def mName = name
	def mLegs = 4
}
function Animal:Name() { return mName }
function Animal:Legs() { return mLegs }
function Animal:Describe() { return Name() .. " has " .. Legs() .. " legs" }
class Bird : Animal (name) {
	super:Init(name)
	mLegs = 2
}
function Bird:Legs() { return super:Legs() + 0 }
function Bird:Cry() { return "tweet" }
local a1 = Animal:new("cat")
local b1 = Bird:new("owl")
checkeq(a1:Name(), "cat", "class instance field")
checkeq(a1:Describe(), "cat has 4 legs", "implicit self calls")
checkeq(b1:Describe(), "owl has 2 legs", "inherited method, overridden field")
checkeq(b1:Cry(), "tweet", "own method")
checkeq(b1.mName, "owl", "super:Init ran")
checkeq(getclass(b1), Bird, "getclass")
checkeq(Bird.classname, "Bird", "classname")
checkeq(Bird.base, Animal, "base")
class Empty ;
checkeq(Empty.classname, "Empty", "class without constructor")
class Cnt {
	def mN = 0
}
function Cnt:Inc() { mN++ return mN }
local c1 = Cnt:new()
c1:Inc()
checkeq(c1:Inc(), 2, "++ on a field of self")
checkeq(c1.mN, 2, "field updated in place")

// def and const
def gDefined = 5
checkeq(gDefined, 5, "def global")
const gConst = 6
checkeq(gConst, 6, "const global")
local dk = {}
def dk.kept;
checkeq(next(dk), "kept", "def keeps a key whose value is nil")
def tD = {}
def tD.f = 1
const tD.g = 2
checkeq(tD.f + tD.g, 3, "def and const on a table")
def p1, p2 = 1, 2
checkeq(p1 + p2, 3, "def with two names")
def dnil;
checkeq(dnil, nil, "def without value")
function Animal:Names() {
	def mFirst, mSecond = "a", "b"
	return mFirst .. mSecond
}
checkeq(a1:Names(), "ab", "def in a method")

// the call of a global through :
local sv = 1
function Animal:Shadow() { return :sv }
sv = nil
checkeq(a1:Shadow(), nil, ":name is the global (a local does not count)")
gHidden = "global"
function Animal:Hidden() { local gHidden = "local" return gHidden .. :gHidden }
checkeq(a1:Hidden(), "localglobal", ":name beside a local")

// FuncTable@SelfTable:Function
gFuncs = {}
function gFuncs:Add(n) { return self.base + n }
local selfobj = {base = 100}
checkeq(gFuncs@selfobj:Add(5), 105, "FuncTable@SelfTable:Function (the table is a global: a local would be overwritten)")

// ---- loadstring and errors ----------------------------------------------------------------------------------

checkeq(loadstring("return 1 + 2")(), 3, "loadstring")
checkerr("local a = ...", "unexpected symbol")
checkeq(loadstring("local function f(...) { return *arg } return f(1, 2, 3)")(), 3, "varargs in a loadstring chunk")
checkerr("x = ", "unexpected symbol")
checkerr("x = 1 +", "unexpected symbol")
checkerr("for i = 1 do end", "`,' expected")
checkerr("if x then", "`end' expected")
checkerr("if (x) { y = 1", "`}' expected")
checkerr("local function f() function g() end end", "nested functions must be defined as local")
checkerr("break", "no loop to break")
checkerr("continue", "no loop to continue")
checkerr("x = 'abc", "unfinished string")
checkerr("x = \"abc\n\"", "unfinished string")
checkerr("x = 1.2.3", "unexpected symbol near `.3'")
checkerr("x = 1e", "malformed number")
checkerr("x = 1..2", "ambiguous syntax (decimal point x string concatenation)")
checkerr("x = 0xg", "malformed hexadecimal number")
checkerr("/* never closed", "unfinished long comment")
checkerr("x = '\\300'", "escape sequence too large")
checkerr("super:f()", "'super' only allowed in method calls")
checkerr("function f() { super:g() }", "'super' only allowed in method calls")
check(loadstring("class X () super.y = 1 end") != nil, "super.field inside a class constructor")
checkerr("def 1 = 2", "unexpected symbol")
checkerr("def local x = 1", "unexpected symbol near `local'")
checkerr("x ? y", "`=' expected near `?'")
checkerr("f(", "unexpected symbol")
checkerr("a.b:c = 1", "function arguments expected")
check(loadstring("f() g()") != nil, "statements without a separator")
checkerr("x = x@y", "Invalid syntax")
checkerr("return 1 x = 2", "<eof> expected near `x'")
checkerr("x = {", "unexpected symbol near `<eof>'")
checkerr("x = {1", "`}' expected")
checkerr("x = #", "unexpected symbol")
local f2, m2 = loadstring("\n\n  x = = 1", "=chunk")
check(string.find(m2, "chunk:3:", 0, true) == 0, "error line numbers")
check(loadstring("// only a comment") != nil, "comment-only chunk")
check(loadstring("") != nil, "empty chunk")
check(loadstring("#!shebang line\nreturn 7")() == 7, "first line starting with #")
check(loadstring("x = __POS") != nil, "__POS")

print("dialect: " .. nchecks .. " checks passed")
