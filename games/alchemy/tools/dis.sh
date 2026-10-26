#!/bin/bash
E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe -d -M intel --no-show-raw-insn --start-address=$1 --stop-address=$2 re/alchemy/bin/AlchemyGame.exe 2>&1 | grep -E '^ +[0-9a-f]+:' | sed -E 's/^ +//; s/<\.text\+0x[0-9a-f]+>//; s/\t/ /g'
