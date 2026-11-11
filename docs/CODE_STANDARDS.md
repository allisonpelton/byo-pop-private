# Code standards

The readability pass makes a finished port's code easy to read without changing what it does. Ordinary good practice
applies; these are the rules particular to a decompiled port, and the tools that enforce them.

Two things hold above every rule:

- **Nothing the port does changes:** the same random draws in the same order, the same screens and saves.
- **The proofs keep working:** each function keeps its address, a loop keeps the original's shape where the order of
  draws depends on it, `override` stays on framework hooks, and private blocks stay private.

## Rules

1. **Names come from the original first.** From the game's Mac build where it has one, spelled as there (a plain
   misspelling is corrected); then from how the code uses the thing; last from watching a run. Text that files and logs
   carry (a save's field names, a script command) is a format and keeps its spelling.
2. **Every name says what it holds** (checked). No decompiler names (`local_1c`, `param_2`, `DAT_00506560`,
   `FUN_004cb182`), no `mUnknown` or `mUnk`, no generic word with a number (`mPad04`, `aInt3`, `aDummy`), no word
   followed by an offset or address (`mScale288`, `Rand85B970`), and no comment that says "unknown" in place of a name.
   A number that carries meaning is fine (`mSwapPiece1`).
3. **A field nobody reads** is named for what it holds, and its comment says that nothing reads it. It comes out only
   where nothing depends on it: a save's layout, a struct written raw, a state hash and the order of random draws all
   do. Padding is named for the field it aligns.
4. **The framework's prefixes:** `m` for members, `the` for parameters, `a` or `an` for locals, `g` for globals, `s` for
   statics, UPPER_CASE for constants and enum values.
5. **One-letter names** (checked): `i`, `j`, `k` count, `x`, `y`, `z` are coordinates, `g` is a `Graphics*`. Any other
   is used only within 10 lines of its declaration.
6. **Every function names its address, or says it is the port's** (checked): `// 0x4cd287` above the definition or on
   its declaration, or a `port:` marker (`port(variant):` for a code mod's path, `port(mods):` for a mod hook). A helper
   split out of an original function names that function. A file whose first comment carries the marker needs none on
   each function.
7. **A number gets a name** when the same value means the same thing in more than one place, or when the line and its
   comment do not make its meaning plain.
8. **Comments describe the code as it is:** what it does and why, the original's behaviour, why the port departs from
   it. No history, and nothing a reader of the code cannot follow: not which development stage, milestone or phase
   added something ("stage 7", "since phase two"), and not what a line used to say. A comment names the feature itself
   ("the gold board") and points to a doc by a section name a reader can find.
9. **Mods' code paths, test hooks and headless tools** under `src` follow the same rules. Their files carry the port
   marker in the first comment, so rule 6 asks no address of each function.

## Tools

- `python tools/namecheck.py games/<game>` checks rules 2, 5 and 6 and exits nonzero on any finding. `--list` also
  prints what the exceptions allow, and `--checks` picks checks. Exceptions go in `games/<game>/namecheck.txt`, one a
  line, each with its reason after ` -- `.
- `tools/macmatch.py` pairs a Mac build's names with the export's functions (`docs/MACMATCH.md`).
- `tools/check.sh <game>` proves that play is unchanged: the game's suite against its frozen baseline.
- A rename is proved by the object code: every unit compiles to the same instructions before and after
  (`llvm-objdump -d`). For many renames at once, rename the declarations, compile with `-fsyntax-only`, and replace the
  old name exactly where the compiler reports it missing, so that classes sharing a name are never confused.
- The coverage check reads rule 6's addresses (`docs/COVERAGE.md`).

Each game's notes record what its pass changed.
