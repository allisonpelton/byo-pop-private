# PopLua: the Lua fork inside Bookworm Adventures

> Addresses in this file are Bookworm Adventures Deluxe's exe (Volume 1). Volume 2's PopLua is the same fork: the port's VM undumps and runs all 462 of its compiled scripts and its compiler turns the 8 `scripts/common/*.lua` sources into chunks identical to the shipped `.luc` files (instruction, constant, line and local-variable tables); its `luaV_execute` (0x67b640, 7,879 bytes against Volume 1's 7,481) is laid out differently by the later compiler, and its handlers have not yet been read case by case against this file.

PopLua is PopCap's fork of Lua 5.0.2 (`_VERSION` is still "Lua 5.0.2"). The bytecode format and most of the
VM are 5.0.2's; the changes below were read from the Ghidra decompile and the disassembly of
`BookwormAdventures.exe` (addresses are the exe's). This file is the specification the port's VM
(`games/bookwormadv2/poplua/`) is checked against. Nothing here was learned by running the original.

The unchanged Lua 5.0.2 sources to diff PopLua against are not in the repository (nothing builds plain Lua 5.0):
`E:/retools/lua-5.0.2-dl/lua-5.0.2.tar.gz` from lua.org, SHA-256
`a6c85d85f912e1c321723084389d63dee7660b81b8292452b190ea7190dd73bc`, extracted beside it in `x/lua-5.0.2/`.

## Values

`TObject` keeps 5.0's layout: `int tt` at offset 0, the value at offset 8 (16 bytes with MSVC alignment).
The type tags are 5.0's with an int32 type inserted after number, and a "method" tag used only in registers:

| tag | type | notes |
|---|---|---|
| 0 | nil | |
| 1 | boolean | |
| 2 | light userdata | |
| 3 | number | double |
| 4 | int | int32 (constant tag 4 in chunks); collectable types are those with tag > 4 |
| 5 | string | hash at TString+8 |
| 6 | table | |
| 7 | function | |
| 8 | userdata | |
| 9 | thread | |
| 10 | method | a function in a register that was fetched as a method (SELF, ALTSELF, SELFORGLOBAL); CALL turns it back into 7 |

## Arithmetic (luaV_execute 0x717540)

- ADD, SUB, MUL: number op number gives a number; int op int gives an int (32-bit wrap-around); anything else goes
  through the arithmetic helper (0x7173b0 / 0x716d20), which converts ints and numeric strings to doubles and gives a
  number, or calls the metamethod.
- DIV: number/number is a number; int/int is C's truncating integer division, with the error "attempt to divide by
  zero" for a zero divisor; mixed operands are converted to doubles.
- MOD (new, op 17): only int % int (C's remainder); "attempt to perform modulo on non-integers" otherwise, and
  "attempt to mod by zero".
- POW: int^int gives an int (pow, then a conversion to int); otherwise the helper. No script uses POW.
- UNM: an int stays an int (-x); a number stays a number; otherwise the metamethod.
- INT (op 46): R(A) := int(R(B)): an int is copied, a number is converted (0x765380, the CRT's float-to-int, i.e.
  truncation), anything else is an arithmetic error.
- SIZ (op 20, the `*t` operator): a table gives an int (0x703a40, the table's size); otherwise the size metamethod
  ("get size of").
- EQ: equal tags compare with luaV_equalobj (0x717090); a number and an int also go to luaV_equalobj, so `1 == 1.0`.

## Loops and table constructors

- FORLOOP: when the index, limit and step are all ints the loop stays in ints: idx += step, and it continues while
  idx <= limit (step >= 1) or idx >= limit (step <= 0). Otherwise all three are converted to numbers (strings are
  accepted for the limit and step, as in 5.0) and the 5.0 loop runs.
- TFORLOOP and TFORPREP are 5.0.2's.
- SETLIST/SETLISTO store item n of a constructor at the int key `bc + n - 1`: **arrays are 0-based**. A constructor
  `{a, b, c}` sets keys 0, 1 and 2.

## Tables (0x702830-0x703b00)

- Node layout (40 bytes): key TObject (0), value TObject (16), flags (32), next (36).
- A node counts as used when its value is not nil **or** its flags have bit 0 set: a key defined with DEFGLOBAL /
  DEFTABLE stays in the table with a nil value, so a lookup finds it and does not fall through to `__index`.
- The array part holds the **int** keys 0 .. 2^24 (5.0.2's MAXBITS 24, shifted to 0-based); `next` returns array
  keys as ints. Only int-tagged keys are counted for the array part.
- Main positions (0x702830): boolean `b & (size-1)`; light userdata `p % ((size-1)|1)`; number: `n + 1.0`, the two
  32-bit halves added, `% ((size-1)|1)` (5.0.2's hashnum); int `(k - 1) % ((size-1)|1)` (unsigned); string
  `hash & (size-1)`; other collectable objects `pointer % ((size-1)|1)`.

## Globals and the class dialect

- Closures keep 5.0's layout: proto at +0xc, the environment table TObject at +0x10, upvalues at +0x20.
- GETGLOBAL / SETGLOBAL / GETSELFORGLOBAL / SETSELFORGLOBAL / SELFORGLOBAL / GETTABLE / SETTABLE use an inline cache
  per instruction (8 bytes per instruction at Proto+0x50, the global 0x86d340); it changes nothing a script can see
  and the port has none.
- DEFGLOBAL A Bx: define global K[Bx] := R(A) (0x716b90 with flags 1). CONSTGLOBAL: the same with flags 3.
  DEFTABLE A B C: define R(A)[RK(B)] := RK(C) with flags 1; CONSTTABLE with flags 3.
- ALTSELF A B C (`Class::Method(obj, ...)`): R(A) := R(B)[RK(C)] fetched as a method (a function becomes tag 10),
  and R(A+1) := the value the instruction before it moved (every ALTSELF in the scripts follows a MOVE to A+2 or
  A+1).
- SELF: as 5.0, except the key must be a string and a function result is tagged 10.
- CALL / TAILCALL / CALLSELFORGLOBAL / TAILCALLSELFORGLOBAL share one handler: a callee tagged 10 becomes 7 and is
  called with the self argument in place; a plain callee of CALLSELFORGLOBAL/TAILCALLSELFORGLOBAL has the self slot
  removed (the arguments move down by one), so `f(x)` inside a method calls either `self:f(x)` or the global `f(x)`.
- BREAK (op 47) does nothing at run time.

## The compiler (lexer 0x71a7b0-0x71c4a0, parser 0x70e430-0x714600, code generator 0x71d260-0x71ed90)

The game ships only compiled scripts, but the exe keeps the whole compiler (for `loadstring`, `dofile` of a source
file and its developer console), and the port has it too (`llex`, `lparser`, `lcode`; `ldump` is the port's own,
since the exe never writes a chunk). It reads the dialect the compiled scripts were written in:
- C-style syntax: `//` and `/* */` comments, `{ }` blocks, `else if`, `continue`, `&&`, `||`, `!`, `!=`, hex and
  int literals (TK_INT, kept as ints), `+=`, `-=`, `*=`, `/=`, `%=`, `++`, `--`, and `for i < n`.
- Classes: `class Name [: Base] { ... }` with a generated `new`, `super`, `def` and `const` (DEFGLOBAL/DEFTABLE,
  CONSTGLOBAL/CONSTTABLE), names inside methods that are the object's fields or the globals (the SELFORGLOBAL
  instructions), `:name` and `*t`, `cobj`, `Table` and `__POS`, and `FuncTable@SelfTable:Function`.
- 5.0.2's code generator with the fork's opcodes (MOD, SIZ, ALTSELF, the SELFORGLOBAL family, DEFGLOBAL/DEFTABLE,
  CONSTGLOBAL/CONSTTABLE) and int constants with constant folding; `while` loops are inverted (the test at the end).

The exe's quirks are kept: `continue` inside `while` or `repeat` is never patched (it jumps to itself);
`FuncTable@Self:Fn` overwrites a local FuncTable (0x71e220 takes no fresh register), so only a global works; an
error message can show stale bytes of the lexer's buffer, which the lexer shares with `luaV_concat`.

The compiler is checked by compiling source rebuilt from the disassembly of game functions and comparing the
chunks instruction by instruction (`tests/poplua/cmpluc.py`: code, constants, upvalues, parameters, stack size,
line information and locals all identical), and by `tests/poplua/dialect.lua`, 150 self-checks of the dialect
(`bwa2_luarun -gamedir=games/bookwormadv2/tests/poplua dialect.lua`). `bwa2_luarun -compile in.lua out.luc [name]`
writes a chunk.
