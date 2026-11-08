# Code standards: what a readable port is

The readability pass makes a finished port's code easy to read without changing anything it does. These rules were written
on Bejeweled Deluxe (the pilot, 2026-11-08) while applying them to all of `games/bejeweled/src`, and changed wherever one
failed in practice; the last section says how that went. They are the user's to approve before any other game takes its
pass (the user, 2026-11-08: "Use one game to set the full standards for what constitutes readable and see that they
actually make sense").

Two things hold above every rule. **Nothing the port does changes:** the same random draws in the same order, the same
screens and the same saves, proved by the game's frozen baseline (and, for Bejeweled, the oracle against the original).
**The proofs keep working:** each function keeps its address comment, so the coverage check can pair it with the export;
a loop keeps the original's shape where the order of draws hangs on it; `override` stays on framework hooks; private
blocks stay private. A rule that would break either one gives way, and the doc says so where it happens.

`tools/namecheck.py` checks every rule marked **(checked)** and exits nonzero on any finding:

```
python tools/namecheck.py games/<game>
```

Its exceptions live in `games/<game>/namecheck.txt`, one a line, each with its reason after ` -- `.

## 1. Names come from the original first

A class, function, global or static takes its name from the game's Mac build where it has one (CLAUDE.md, "Mac versions";
`tools/macmatch.py`), as the Mac spells it, even where the port had already chosen another. The one exception is a plain
misspelling, which is corrected. Then a name comes from how the code uses the thing; last, from watching a run (the port's
logs, origrun's `--watch` on the original). The PC exe stays the authority for behaviour: a Mac name never changes what a
function does.

```cpp
// before                                  // after (the Mac's Board::FindSets, WinDMApp::DoColorDepthExplination)
bool Board::ProcessMatches()               bool Board::FindSets()
void WinDMApp::ShowColorDepthDialog()      void WinDMApp::DoColorDepthExplanation()
```

A name the Mac build gives a function also renames what is built on it: `ProcessMatchesVariant` became `FindSetsVariant`.
Text that files and logs carry (a replay's field names, a trace line's labels, a script command) is a format, not a name,
and keeps its spelling unless every reader of it changes in the same commit.

## 2. Every name says what it holds (checked)

No name stands in for one. That rules out:

- a decompiler's names: `local_1c`, `param_2`, `iVar3`, `uStack_8`, `DAT_00506560`, `FUN_004cb182`, `field_0x1c`;
- names that say they are unknown: `mUnknown`, `mUnk`, `unk_10`;
- a generic word with a number or an offset where the name should be: `mUnused218`, `mPad04`, `mValue2`, `aInt3`, `aDummy`;
- a comment that puts "unknown" in place of a name: `// +0x218 purpose unknown`.

A number that carries meaning is fine: `mSwapPiece1` and `mSwapPiece2` are the swap's first and second gem, `mX0, mY0, mX1,
mY1` a rectangle's corners. A name that is about unknown things, such as `UnknownKey` (the warning for a key the rules do not
know), goes into `namecheck.txt` with that reason.

```cpp
// before                                              // after
std::string aDummy;                                    std::string anIgnoredError;
std::string& anErr = theError ? *theError : aDummy;    std::string& anErr = theError ? *theError : anIgnoredError;
```

## 3. A field nobody reads is named for what it holds

Where the original writes a value and never reads it, the name says what it holds and the comment says that nothing reads
it, so that a reader does not go looking for its use. It is removed only where nothing depends on it; a save file's
layout, a struct written raw, the state hash and the tracehash baselines all do.

```cpp
// before
int     mUnused218;         // +0x218 only ever zeroed (ctor, first run of a move)
int     mPad04;             // +0x04 (padding before the double; raw garbage in original saves)
SexyString mUnusedString;   // +0x2c8 constructed/destroyed only
// after
int     mClearedAtMoveStart;    // +0x218 zeroed by the ctor and by a move's first scored run; never read
int     mPadBeforeX;            // +0x04 padding before mX (raw garbage in original saves)
// +0x2c8 a SexyString the original constructs and destroys and never uses: left out
```

Padding is named for the field it aligns. A field that only ever holds one value is named for that value
(`ScorePopup::mAlwaysMinusOne`).

## 4. The framework's prefixes

The port keeps SexyAppFramework's conventions, which the original's code and the Mac names follow: `m` for members, `the`
for parameters, `a`/`an` for locals, `g` for globals, `s` for statics, `is`/`has` for flags passed as parameters, and
UPPER_CASE for constants and enum values. `Graphics* g` stays as the framework writes it.

## 5. One letter only where the type is on the same screen (checked)

`i`, `j` and `k` count, `x`, `y` and `z` are coordinates, `g` is a `Graphics*`. Any other one-letter name is fine only
while it is used within 10 lines of where it is declared (a lambda's parameters, a three-line helper); past that it gets a
word. The checker measures the span; `namecheck.txt` adds letters for code where they are the subject's own notation (the
colour channels `r g b a` in pixel code, the eight working variables of SHA-256).

```cpp
// before (GameRules.cpp, 40 lines)        // after
ByteWriter w;                              ByteWriter aWriter;
w.U8(FORMAT_VERSION);                      aWriter.U8(FORMAT_VERSION);
```

## 6. Every function names its address, or says it is the port's (checked)

The comment directly above a function definition carries the original's address (`// 0x4cd287`), or the port's marker
(`port:` or `port(variant):`) saying that the original has no such function. A helper the port split out of an original
function names that function and its address. A file whose first comment carries the marker holds only the port's own
functions and needs no marker on each.

```cpp
// before
static bool ReadSaveBool(const Buffer& theBuffer)
// after
// part of LoadGame (0x4cca98): one saved bool, a byte that is true when not 0
static bool ReadSaveBool(const Buffer& theBuffer)
```

## 7. Numbers: named when they mean something twice

A number gets a name when the same value means the same thing in more than one place (the gem's 49 px, the cell's 51 px,
the gem sheet's 104 px rows), or when its meaning is not plain from the line and its comment. These need no name: 0, 1 and
-1 as identities and sentinels, halving by 2, a number in a table whose declaration says what it is (`gMatchBaseScores`), a
widget's layout in its one `Resize` call, and a tuning number used once beside a comment that says what it is. Keeping
those literal keeps them easy to find in the export. Not checked: no tool tells a meaningful 51 from a coincidental one.

```cpp
// before
ScorePopup aPopup = {anAvgX + 24, anAvgY + 16, 110, -1, aPoints};
// after
ScorePopup aPopup = {anAvgX + ScorePopup::OFFSET_X, anAvgY + ScorePopup::OFFSET_Y, ScorePopup::START_LIFE, -1, aPoints};
```

## 8. Comments describe the code as it is

A comment says what the code does and why, in the present: the original's behaviour, what a value holds, why the port
departs from the original. Where the port departs, a `port:` marker says so (`port(variant):` for the code mods' paths,
`port(mods):` for the mods' hooks); the markers stay, since the deviations list and the reviews rely on them. How the code
came to be (which stage added a field, what a line used to say) belongs in the commit messages and the design docs, not in
the code; a pointer to a design doc's section stays.

```cpp
// before
// Read since stage 5: mAny (the v2 save, the variant trace line), mInvalid ... since stage 6 mUndo ...
// after
// Read by: mAny (the v2 save, the variant trace line), mInvalid (UpdateSwapping keeps a swap that made no set), ...
```

## 9. Mods, test hooks and the headless tools

The same rules hold for the code mods' paths in `src`, for `src/testhooks/` and for `src/headless/`: the names rules (1 to
5) without exception, since a test reader needs names as much as a game reader. Rule 6 asks less of them: their files carry
the port marker in their first comment, so they need no address on each function. A mod's own folder under `mods/` holds
data and art, not code, and has no rules here.

## How it worked out

(Written at the end of the pilot.)
