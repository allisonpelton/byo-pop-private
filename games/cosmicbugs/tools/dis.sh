#!/usr/bin/env bash
# Disassembles CosmicBugs.exe between two addresses (Intel syntax), for the code Ghidra keeps as labels (the pixel
# readers and writers 0x42d920-0x42db40 that 0x42dbc0 calls through a pointer, the blitters' inner loops).
#   bash games/cosmicbugs/tools/dis.sh 42d920 42db40
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OBJDUMP="${OBJDUMP:-E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe}"
"$OBJDUMP" -d --no-show-raw-insn -M intel --start-address=0x$1 --stop-address=0x$2 "$ROOT/re/cosmicbugs/bin/CosmicBugs.exe" | tail -n +7
