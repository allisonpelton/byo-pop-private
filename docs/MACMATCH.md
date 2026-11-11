# macmatch: names from a game's Mac build

Several of PopCap's Mac builds still carry their symbols, and where the user found one it sits in the main checkout's
`re/<game>/mac/` (CLAUDE.md, "Mac versions"). `tools/macmatch.py` pairs the functions of such a Mac binary with the
functions of the PC exe's Ghidra export and carries the Mac's demangled names over, so that a new decompilation starts
from an export in which most of the game's own functions and much of the framework already have names. The names guide
the port; the PC exe stays the authority for behaviour, since the Mac build is often another version. Everything the
tool writes comes from the user's game files, so its output goes under `re/<game>/macmatch/`, which is git-ignored, and
is never committed or published.

## Running it

```
python tools/macmatch.py --mac re/<game>/mac/<binary> --exe re/<game>/bin/<Game>.exe \
    --functions re/<game>/gh/functions.jsonl --out re/<game>/macmatch/<game>.match.jsonl \
    --apply-names re/<game>/macmatch/<game>.names.jsonl --cache <scratch folder>
```

A second `--mac` names a framework library the game loads, as Bejeweled 2 loads `SexyApp`; the game's calls into it are
then resolved by name and both binaries are matched in one run. `--rtti` takes the classes file of
`tools/rtti_msvc70.py` when the game has one; without it the tool runs `rtti_msvc70.py` into the cache folder. A run
takes seconds. An Intel Mac binary is disassembled by llvm-objdump once and the listing kept in the cache; a PowerPC one
is decoded on every run, which is quick. The run prints one line for each pass with the number of pairs it made, and one
coverage line for each Mac binary: how many of its functions found a PC partner, split into the game's classes, the
framework's classes, free functions and C library code.

The match file has one line for each PC function that was paired, in the format of `tools/funcmatch.py`: the PC address
(`a`), the Mac address (`b`), the pass that made the pair (`how`), the Mac name made usable in Ghidra (`name`), the full
demangled Mac signature (`comment`) and a confidence between 0 and 1 (`score`); `mac` names the binary when it is not
the first. A Mac deleting destructor becomes `Class::deleting_destructor`, a thunk `Class::thunk_<method>`. The names
file holds the same pairs in `ApplySymbols.java`'s format, only for the PC functions whose export name is still a
placeholder (`FUN_`, `vfNN`, `ctor_or_dtor_`; every pair with `--rename-all`), and is applied like any other symbol
file: `GHIDRA_PROJECT=<project> GHIDRA_PROGRAM=<Game>.exe EXPORT_DIR=... build/native/rundesk.exe bash
tools/ghidra_headless.sh apply re/<game>/macmatch/<game>.names.jsonl`. Each comment says which Mac function, pass and
score the name came from.

## How it reads the two builds

The Mac side is read without any outside disassembler for PowerPC. The llvm-mingw toolchain has no PowerPC target, and
its llvm-objdump also refuses the prebound executables these games are, so the tool parses the Mach-O file itself (thin
or universal, executable or library: segments, sections, the symbol table, the indirect symbols behind each stub, the
external relocations of a library) and decodes the PowerPC instructions it needs: the calls in order (`bl`, and tail
branches to other functions), the register arithmetic that builds an address (`lis`, `addis`, `addi`, `ori`, the
`bcl`/`mflr` pair that loads the code's own address), the loads and stores through such addresses, and loads of pointers
kept in data for code built without position-independent addressing. From the addresses it learns which C strings,
CFStrings, float constants, vtables and other functions each function uses. An Intel Mac binary goes through
`llvm-objdump --macho -d`, whose listing the tool reads for the same facts. The demangled names come from
`llvm-cxxfilt`. The constructor and destructor variants GCC and CodeWarrior emit (complete, base and allocating
constructors; complete, base and deleting destructors, often one a stub of another) are folded into the one that holds
the code, and each class's vtables, secondary vtables and typeinfo give its slots and its primary base.

The PC side reads the exe itself next to Ghidra's `functions.jsonl`, which gives the function bounds and strings: the
direct calls of each function in order, its calls through the import table, its x87 float constants, the stack it cleans
up on return (`ret N`, so the number of its stack arguments), whether it is a thunk or an adjustor thunk, and the
vtables its stores install. The classes, their vtables and bases come from `rtti_msvc70.py`.

## The passes

The passes run strongest first, and every pair stays one to one: a candidate is taken only when each function is the
other's clear best, by a margin, and nothing already matched contradicts it.

The string pass pairs functions that use a string or float constant no other function uses in either build, when the two
functions' whole sets of strings and constants agree; paths are compared by their last part, since the Mac build spells
them with slashes. Sets of constants that are unique as sets pair too.

The vtable pass pairs classes by their RTTI name, and later through a matched constructor that installs one vtable in
each build, then maps each class's Mac slots onto its PC slots. The Mac's two destructor slots are the PC's one deleting
destructor, and MSVC's slot order for overloads (grouped at the first declaration, in reverse) is rebuilt on the Mac
side. The framework versions of the two builds often differ: Bejeweled's Mac `Widget` has 43 slots and its PC one 70,
because the PC framework added virtuals and moved `Update` and `Draw` into `WidgetContainer`. So the map does not assume
the order or the count. A class starts from its base's map; for the slots it adds, a pair is sure when the derived
classes override both slots or neither and their overriding functions already matched agree, and when it is each slot's
clear best, even where sure pairs cross. Between the sure pairs that keep the order, the slots left go in order when
there are as many on each side and they look alike. A deleting destructor pairs only with a destructor (the PC one is
known by its test of the flag argument), and a pure virtual only with `_purecall`. A PC function that MSVC folded is
left out: one that sits at different slot numbers, one that slots of different Mac functions map to (the shared deleting
destructor of classes that add nothing, for one), and bodies of a few bytes. Slots inherited on either side are left to
the base's pair. Thunks in secondary vtables pair with adjustor thunks, and their targets with each other.

The import pass pairs the C library functions the Mac imports with the PC's copies Ghidra's function ID named, as
anchors for the next pass; names that are not unique are left out. The call pass spreads from the pairs made so far: the
callees of a matched pair, aligned by their order around the calls already matched, and the callers of a matched
function vote for pairs, which are checked against the strings, the PC stack cleanup against the Mac's parameter list,
the size ratio of the two builds and the neighbours already matched. The order pass pairs the functions between two
matched pairs that lie next to each other in both builds' address order, when the gaps hold the same number of functions
and the functions look alike. The vtable, call and order passes repeat until a round adds nothing.

## How well it works

The tool measures itself against what the finished ports know (`--check-src games/<game>/src`, `--check-names`): the
address comments above the ports' definitions, the export's own meaningful names, and an alias file of the renames a
game's `MAC_COMPARISON.md` lists (`--check-alias`, `{"classes": {port: mac}, "methods": {"PortClass::Method":
"MacClass::Method"}}`; `--check-out` writes the pairs whose names differ). A pair counts as the same name when the names
agree after the aliases. Every other pair was judged by hand, and most turned out to be the port's own names for the
same function, such as BookWorm's `ShowHelp` for the Mac's `DoHelp`; the wrong ones are counted below. Coverage is the
share of the Mac's functions that found a PC partner. Its denominators include the Mac's platform layer, framework
functions the PC version does not have, and functions one compiler inlined, so it understates what is left to find.

Measured on Alchemy, Bejeweled and Seven Seas (PowerPC), BookWorm (Intel) and Bejeweled 2 (PowerPC, with the framework
in the `SexyApp` library), a run gives about 390 to 1030 pairs, and 37% to 46% of a Mac build's functions find a PC
partner (72% of Bejeweled 2's game functions). The wrong pairs are a handful in each game, of the same kinds:
framework slots where the two framework versions differ most (`CommitBits` on the PC's `BitsChanged`,
`ReadFromRegistry`, `LoadingThreadCompleted`; `AddedToManager`, `RemovedFromManager` and `Update` are `Widget`'s in the
Mac build and `WidgetContainer`'s in the PC one), functions MSVC folded (the `Insets` constructor and `Color`'s
four-int one, a `Color` getter and a `Font` one), and neighbours the call pass confused (`DDInterface::AddDDImage` on
`DDImage::Init`, `Quantize8Bit` on `Quantize32Bit`, `Board::ClearBoard` on `GameWon`).

Across the measured games the string, vtable and order passes are right nearly every time once the renames are set
aside, and the call pass is wrong for under 2% of its checkable pairs.

## Limits

The names describe the Mac build. Where the framework versions differ, a framework name can belong to the neighbouring
slot or to the base class that holds the function in the PC version, and that is where most wrong pairs are; a framework
name is best checked against PopCap's released framework source before it is trusted. MSVC folds identical functions
(small getters, empty bodies, constructors of the same layout), and a folded PC function can carry only one of the names
it stands for; the vtable pass leaves such functions out, the call pass can still name one. The check cannot tell a
rename from a wrong pair, so its "other name" count is an upper bound for the errors, and the judgements above were made
by hand.

Functions with neither strings nor calls nor a vtable slot are found only by the order pass, so small leaf functions and
plain C code stay mostly unpaired, and the C and C++ libraries are used only as anchors. The PowerPC decoder knows what
GCC 3.3, GCC 4 and CodeWarrior emitted for these games, not every instruction; the Intel path has been tried on BookWorm
only, and a 64-bit Mac binary is not read at all.
