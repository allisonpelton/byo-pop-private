#!/bin/bash
# objcode.sh <name>: the object code of the port's units, for the readability pass's proof that a rename changes nothing
# (docs/CODE_STANDARDS.md, "Tools"). From the worktree root, after `TARGET=noahsark bash tools/build.sh native`. Writes one file a
# unit to build/readab/<name>/: the unit's instructions with their relocations (llvm-objdump -d -r, no raw bytes) and the
# contents of its read-only and initialised data. Two states of the source compile to the same code when
#     diff -rq build/readab/before build/readab/after
# prints nothing. A renamed function or method changes its symbol, which shows as differing lines that hold only the two names.
cd "$(dirname "$0")/../../../.."
LLVM=/e/retools/llvm-mingw-20261006-ucrt-x86_64/bin
OBJ=build/native/games/noahsark/CMakeFiles/noahsark.dir/src
OUT="build/readab/$1"
rm -rf "$OUT"; mkdir -p "$OUT"
for f in $(cd "$OBJ" && find . -name '*.obj'); do
	n=$(echo "$f" | sed 's|^\./||; s|/|_|g; s|\.cpp\.obj$||')
	{ "$LLVM/llvm-objdump" -d -r --no-show-raw-insn "$OBJ/$f"; "$LLVM/llvm-objdump" -s -j .rdata -j .data "$OBJ/$f"; } 2>/dev/null |
		grep -v 'file format' > "$OUT/$n.txt"
done
echo "$1: $(ls "$OUT" | wc -l) units, $(cat "$OUT"/*.txt | wc -l) lines"
