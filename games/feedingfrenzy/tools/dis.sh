#!/bin/sh
# Disassembles a range of the exe (Intel syntax) with llvm-objdump from E:/retools: dis.sh <start> <stop> (hex addresses)
ROOT=$(git rev-parse --show-toplevel)
OD=$(ls E:/retools/llvm-mingw-*/bin/llvm-objdump.exe | head -1)
"$OD" -d -M intel --no-show-raw-insn --start-address=0x$1 --stop-address=0x$2 "$ROOT/re/feedingfrenzy/bin/FeedingFrenzy.exe" | grep -v '^$'
