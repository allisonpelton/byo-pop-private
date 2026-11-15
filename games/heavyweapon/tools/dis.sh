#!/bin/bash
# Disassembles a range of the Heavy Weapon image (Intel syntax) with llvm-objdump.
#   games/heavyweapon/tools/dis.sh 428580 428610
# The addresses are virtual addresses in hex, without 0x.
here="$(cd "$(dirname "$0")/../../.." && pwd)"
exe="$here/re/heavyweapon/bin/HeavyWeaponGame.exe"
objdump="E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe"
"$objdump" -d -M intel --no-show-raw-insn --start-address=0x$1 --stop-address=0x$2 "$exe" | grep -v "^$" | tail -n +5
