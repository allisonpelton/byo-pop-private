#!/bin/bash
# Disassemble a range of the staged MahjongEscapeAJ.exe: dis.sh <start hex> <end hex>
OD=/e/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
"$OD" -d --no-show-raw-insn -M intel --start-address=0x$1 --stop-address=0x$2 "$ROOT/re/mahjongjapan/bin/MahjongEscapeAJ.exe" | tail -n +7 | grep -v "^\s*$" | sed 's/<\.text+0x[0-9a-f]*>//'
